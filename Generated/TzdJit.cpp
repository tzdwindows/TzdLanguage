#include "../Res/TzdStrings.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <csetjmp>
#include <cstddef>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"

#include "TzdJit.h"
#include "TzdInterpreter.h"
#include "../TzdDebugger.h"

#pragma comment(lib, "LLVMX86CodeGen.lib")
#pragma comment(lib, "LLVMX86Desc.lib")
#pragma comment(lib, "LLVMX86Info.lib")
#pragma comment(lib, "LLVMX86AsmParser.lib")

using namespace llvm;
using namespace llvm::orc;

thread_local JitValuePool g_JitPool;

// ======= 尾递归优化(TRE)上下文 =======
static thread_local std::vector<std::string> s_currentFuncParamNames;
static thread_local llvm::BasicBlock* s_tailRecurseBB = nullptr;
static thread_local bool s_inTailPosition = false;
static thread_local llvm::Function* s_currentWorkerFunc = nullptr;
static thread_local llvm::Function* s_currentNativeWorkerFunc = nullptr;
static thread_local bool s_compilingNativeWorker = false;
// Compile-time map: function base name → worker Function* for direct call optimization
static std::unordered_map<std::string, llvm::Function*> s_compiledWorkers;
static std::unordered_map<std::string, llvm::Function*> s_compiledNativeWorkers;
static std::unordered_map<std::string, void*> s_workerPointers;
static std::shared_mutex s_workerPointersMutex;
static thread_local std::vector<std::tuple<llvm::Value*, llvm::Value*, int>> s_tryJmpBufStack;
static thread_local int s_loopLevel = 0;

std::string formatSourcePath(const std::string& fullPath);
std::string unescapeString(const std::string& input);

// #region agent log
static void agentLogJit(const char* hypothesisId, const char* location, const char* detail) {
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f) return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"jit visit\",\"data\":{\"detail\":\""
        << detail << "\"},\"timestamp\":" << ts << "}\n";
}

static Value* castAnyToValue(const std::any& a, const char* where) {
    if (!a.has_value()) {
        agentLogJit("D", where, "empty any");
        throw std::bad_any_cast();
    }
    if (a.type() != typeid(Value*)) {
        agentLogJit("D", where, a.type().name());
        throw std::bad_any_cast();
    }
    return std::any_cast<Value*>(a);
}

// TzdValue field offsets for inlined GEP+Store/Load (eliminates rt_ function calls)
static constexpr size_t TZD_TYPE_OFFSET = offsetof(TzdValue, type);
static constexpr size_t TZD_DVAL_OFFSET = offsetof(TzdValue, dVal);
static_assert(TZD_TYPE_OFFSET > 0, "type offset must be > 0 (after annotations vector)");

static std::string getParamName(TzdLangParser::ParamContext* p) {
    if (!p) return "arg";
    if (p->IDENTIFIER()) return p->IDENTIFIER()->getText();
    if (p->T_INT()) return p->T_INT()->getText();
    if (p->T_STRING()) return p->T_STRING()->getText();
    if (p->T_FLOAT()) return p->T_FLOAT()->getText();
    if (p->T_BOOL()) return p->T_BOOL()->getText();
    if (p->T_VOID()) return p->T_VOID()->getText();
    if (p->T_PTR()) return p->T_PTR()->getText();
    if (p->KW_RET()) return p->KW_RET()->getText();
    return p->getText();
}
// #endregion

// 辅助函数：递归解包 AST，判断 Return 后面是否干净地跟着一个函数调用
static TzdLangParser::CallExprContext* getAsCallExpr(antlr4::tree::ParseTree* node) {
    if (!node) return nullptr;
    if (auto call = dynamic_cast<TzdLangParser::CallExprContext*>(node)) return call;
    if (node->children.size() == 1) return getAsCallExpr(node->children[0]);
    return nullptr;
}

static std::string build_jit_frame_string(const char* name) {
    std::string fileLoc = "memory";
    int line = 0;
    std::string frameName = std::string(name);

    std::string fullName(name);
    bool found = false;

    // 1. 尝试解析类方法或类构造函数的符号形式 (例如 ClassName_MethodName 或 ClassName_CtorName_ctor_N)
    size_t firstUnderscore = fullName.find('_');
    if (firstUnderscore != std::string::npos) {
        std::string className = fullName.substr(0, firstUnderscore);
        TzdClassDef* cls = TzdOopManager::getClass(className);
        if (cls) {
            std::string remaining = fullName.substr(firstUnderscore + 1);
            size_t ctorPos = remaining.find("_ctor_");
            if (ctorPos != std::string::npos) {
                // 构造函数
                int paramCount = 0;
                try {
                    paramCount = std::stoi(remaining.substr(ctorPos + 6));
                }
                catch (...) {}
                for (const auto& ctor : cls->constructors) {
                    if (ctor.paramCount == paramCount) {
                        fileLoc = formatSourcePath(ctor.sourceFile);
                        line = ctor.line;
                        frameName = cls->fullName + "." + cls->simpleName;
                        found = true;
                        break;
                    }
                }
            }
            else {
                // 普通类方法 (剥离 JIT 版本号后缀如 _v1)
                std::string methodName = remaining;
                size_t vPos = methodName.rfind("_v");
                if (vPos != std::string::npos && vPos + 2 < methodName.size() && std::isdigit(methodName[vPos + 2])) {
                    methodName = methodName.substr(0, vPos);
                }

                ClassMethod* m = cls->findMethod(methodName);
                if (m) {
                    fileLoc = formatSourcePath(m->sourceFile);
                    line = m->line;
                    frameName = cls->fullName + "." + methodName;
                    found = true;
                }
            }
        }
    }

    // 2. 如果不是类成员，则作为全局普通函数从解释器作用域查找
    if (!found && g_CurrentInterpreter) {
        try {
            TzdValue func = g_CurrentInterpreter->getVariable(name, nullptr);
            if (func.type == TzdValue::FUNCTION || func.type == TzdValue::NATIVE_FUNCTION) {
                fileLoc = formatSourcePath(func.sourceFile);
                line = func.line;
                frameName = func.name.empty() ? name : func.name;
            }
        }
        catch (...) {}
    }

    std::string formattedFrame = frameName + " (" + fileLoc;
    if (line > 0) formattedFrame += ":" + std::to_string(line);
    formattedFrame += ") (JIT Compiled)";
    return formattedFrame;
}

enum VariableType {
    JIT_VAR_NONE,      // 未定义/空
    JIT_VAR_LOCAL,     // 局部变量（位于栈上，LLVM Alloca）
    JIT_VAR_GLOBAL,    // 全局变量（位于运行时 Hashmap）
    JIT_VAR_MEMBER,    // 类成员（通过 this 指针访问）
    JIT_VAR_NATIVE     // 原生绑定变量
};

struct VariableInfo {
    VariableType type;
    llvm::Value* address;
};

// tzdInternSelector(name) and tzdGetSelectorName(sel) are centrally defined in TzdOop.cpp.

extern "C" {
    TzdValue* g_LastJitValue = nullptr;
    static thread_local jmp_buf* g_tzdFatalJmp = nullptr;

    inline TzdValue* make_double(double d) {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::DOUBLE;
        v->dVal = d;
        return v;
    }

    void* rt_get_worker_ptr(const char* funcName) {
        std::shared_lock<std::shared_mutex> lock(s_workerPointersMutex);
        auto it = s_workerPointers.find(funcName);
        if (it != s_workerPointers.end()) {
            return it->second;
        }
        return nullptr;
    }

    inline TzdValue* make_bool(bool b) {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::BOOL;
        v->bVal = b;
        return v;
    }

    void* rt_resolve_var(const char* name) {
        if (!g_CurrentInterpreter) return g_JitPool.next();

        TzdValue* v = g_JitPool.next();
        try {
            *v = g_CurrentInterpreter->getVariable(name, nullptr);
            return v;
        }
        catch (...) {
            v->type = TzdValue::NONE;
            return v;
        }
    }

    void* rt_get_var_ptr(const char* name) {
        if (!g_CurrentInterpreter) return nullptr;
        for (auto it = g_CurrentInterpreter->scopes.rbegin(); it != g_CurrentInterpreter->scopes.rend(); ++it) {
            auto scope_it = it->find(name);
            if (scope_it != it->end()) {
                return &(scope_it->second);
            }
        }
        return nullptr;
    }

    void rt_construct_num_at(void* dest, double val) {
        // 使用 placement new 在 dest 处初始化对象，不触发赋值运算符
        TzdValue* v = new (dest) TzdValue();
        v->type = TzdValue::DOUBLE;
        v->dVal = val;
    }

    void rt_init_tzd_value(void* ptr, int count) {
        if (!ptr) return;
        TzdValue* arr = (TzdValue*)ptr;
        for (int i = 0; i < count; ++i) {
            new (&arr[i]) TzdValue();
        }
    }

    void rt_write_fast_ret(void* dest, void* src) {
        if (!dest || !src) return;
        *(TzdValue*)dest = *(TzdValue*)src;
    }

    void rt_construct_copy_at(void* dest, void* src) {
        if (!dest || !src) return;
        // 使用 placement new 调用拷贝构造函数
        new (dest) TzdValue(*(TzdValue*)src);
    }

    void rt_destruct_values(void* ptr, int count) {
        TzdValue* arr = (TzdValue*)ptr;
        for (int i = 0; i < count; ++i) {
            arr[i].~TzdValue();
        }
    }

    void rt_store_var(const char* name, void* val) {
        if (g_CurrentInterpreter && val) {
            g_CurrentInterpreter->setVariable(name, *(TzdValue*)val);
        }
    }

    void* rt_get_arg(int index) {
        if (!g_CurrentInterpreter) return g_JitPool.next();
        if (!g_CurrentInterpreter->m_callFrameStack.empty()) {
            const auto& frame = g_CurrentInterpreter->m_callFrameStack.back();
            if (frame.thisPtr) {
                if (index == 0) return frame.thisPtr;
                if (frame.args && (index - 1) < frame.argCount) return &frame.args[index - 1];
            } else {
                if (frame.args && index < frame.argCount) return &frame.args[index];
            }
            return g_JitPool.next();
        }
        if (g_CurrentInterpreter->m_argPtrStack.empty()) return g_JitPool.next();
        return &g_CurrentInterpreter->m_argPtrStack.back()[index];
    }

    void rt_set_null(void* dest) {
        ((TzdValue*)dest)->type = TzdValue::NONE;
    }

    void* rt_call_sub_fast(const char* funcName, int argCount, void* args) {
        if (!g_CurrentInterpreter) return g_JitPool.next();

        TzdValue* funcObjPtr = nullptr;
        for (auto scopeIt = g_CurrentInterpreter->scopes.rbegin(); scopeIt != g_CurrentInterpreter->scopes.rend(); ++scopeIt) {
            auto it = scopeIt->find(funcName);
            if (it != scopeIt->end()) {
                funcObjPtr = &it->second;
                break;
            }
        }
        if (!funcObjPtr) {
            // Debug: check worker pointers
            std::shared_lock<std::shared_mutex> wlock(s_workerPointersMutex);
            auto wit = s_workerPointers.find(funcName);
            if (wit != s_workerPointers.end()) {
                // Found in worker pointers — call directly
                TzdValue* res = g_JitPool.next();
                auto jitPtr = reinterpret_cast<void(*)(void*, void*)>(wit->second);
                // Build arg frame
                g_CurrentInterpreter->m_argPtrStack.push_back((TzdValue*)args);
                std::unordered_map<std::string, TzdValue> jitScope;
                g_CurrentInterpreter->scopes.push_back(jitScope);
                jitPtr(g_CurrentInterpreter, res);
                g_CurrentInterpreter->scopes.pop_back();
                g_CurrentInterpreter->m_argPtrStack.pop_back();
                return res;
            }
            return g_JitPool.next();
        }

        if (funcObjPtr->type == TzdValue::FUNCTION && funcObjPtr->jittedPtr) {
            TzdValue* res = g_JitPool.next();
            g_CurrentInterpreter->m_argPtrStack.push_back((TzdValue*)args);

            // 【修改】：格式化获取源位置与行号，拼接 Java 风格栈帧
            std::string fileLoc = formatSourcePath(funcObjPtr->sourceFile);
            int line = funcObjPtr->line;
            std::string frameName = funcName;
            frameName += " (" + fileLoc;
            if (line > 0) frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);

            funcObjPtr->jittedPtr(g_CurrentInterpreter, res);

            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_argPtrStack.pop_back();
            return res;
        }

        std::vector<TzdValue> callArgs;
        callArgs.reserve(argCount);
        TzdValue* arr = (TzdValue*)args;
        for (int i = 0; i < argCount; ++i) callArgs.push_back(arr[i]);
        TzdValue fallback = g_CurrentInterpreter->callFunction(*funcObjPtr, callArgs);
        TzdValue* res = g_JitPool.next();
        rt_write_fast_ret(res, &fallback);
        return res;
    }

    void rt_push_arg_frame(void* args) {
        if (!g_CurrentInterpreter) return;

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth) {
            std::cerr << "运行错误：栈溢出" << std::endl;
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return;
        }

        g_CurrentInterpreter->m_argPtrStack.push_back((TzdValue*)args);
        ++g_CurrentInterpreter->m_callDepth;
    }

    void rt_pop_arg_frame() {
        if (!g_CurrentInterpreter) return;

        if (!g_CurrentInterpreter->m_argPtrStack.empty()) {
            g_CurrentInterpreter->m_argPtrStack.pop_back();
        }

        if (g_CurrentInterpreter->m_callDepth > 0) {
            --g_CurrentInterpreter->m_callDepth;
        }
    }

    void* rt_create_num(double v) { return make_double(v); }
    void* rt_create_bool(bool v) { return make_bool(v); }
    void* rt_create_null() { return g_JitPool.next(); }

    void* rt_create_str(const char* s) {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::STRING;
        v->sVal = s;
        return v;
    }

    void* rt_op_add(void* a, void* b) {
        if (!a || !b) return make_double(0);
        TzdValue* v1 = (TzdValue*)a;
        TzdValue* v2 = (TzdValue*)b;
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE) {
            return make_double(v1->dVal + v2->dVal);
        }
        if (v1->type == TzdValue::STRING || v2->type == TzdValue::STRING) {
            TzdValue* res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal = TzdInterpreter::getAsString(*v1) + TzdInterpreter::getAsString(*v2);
            return res;
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) + TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void* rt_op_sub(void* a, void* b) {
        TzdValue* v1 = (TzdValue*)a;
        TzdValue* v2 = (TzdValue*)b;
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE) {
            return make_double(v1->dVal - v2->dVal);
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) - TzdInterpreter::getAsDoubleInternal(*v2));
    }


    // 乘法
    void* rt_op_mul(void* a, void* b) {
        // **优化 5: 避免对指针的多次解引用 (同上)**
        TzdValue* v1 = (TzdValue*)a;
        TzdValue* v2 = (TzdValue*)b;

        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE) {
            return make_double(v1->dVal * v2->dVal);
        }

        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) * TzdInterpreter::getAsDoubleInternal(*v2));
    }

    // 除法
    void* rt_op_div(void* a, void* b) {
        double dv = TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b);
        if (dv == 0) {
            return make_double(0.0);
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a) / dv);
    }

    // 取模
    void* rt_op_mod(void* a, void* b) {
        return make_double(fmod(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a), TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b)));
    }

    // 幂运算
    void* rt_op_pow(void* a, void* b) {
        return make_double(pow(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a), TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b)));
    }

    // 取反 (负号)
    void* rt_op_neg(void* a) {
        return make_double(-TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a));
    }

    bool rt_to_bool(void* a) {
        if (!a) return false;
        return TzdInterpreter::isTruthy(*(TzdValue*)a);
    }

    void* rt_op_not(void* a) { return make_bool(!rt_to_bool(a)); }

    void* rt_op_sqrt(void* a) { return make_double(sqrt(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a))); }

    void* rt_op_gt(void* a, void* b) {
        if (!a || !b) return make_bool(false);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a) > TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b));
    }

    void* rt_op_lt(void* a, void* b) {
        if (!a || !b) return make_bool(false);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a) < TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b));
    }

    void* rt_op_ge(void* a, void* b) {
        if (!a || !b) return make_bool(false);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a) >= TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b));
    }

    void* rt_op_le(void* a, void* b) {
        if (!a || !b) return make_bool(false);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*(TzdValue*)a) <= TzdInterpreter::getAsDoubleInternal(*(TzdValue*)b));
    }

    void* rt_op_eq(void* a, void* b) {
        if (!a || !b) return make_bool(false);
        TzdValue* v1 = (TzdValue*)a;
        TzdValue* v2 = (TzdValue*)b;
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE)
            return make_bool(v1->dVal == v2->dVal);
        if (v1->type == TzdValue::STRING && v2->type == TzdValue::STRING)
            return make_bool(v1->sVal == v2->sVal);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) == TzdInterpreter::getAsDoubleInternal(*v2));
    }


    void* rt_op_ne(void* a, void* b) {
        // 复用 eq 逻辑并取反
        TzdValue* eq = (TzdValue*)rt_op_eq(a, b);
        eq->bVal = !eq->bVal;
        return eq;
    }

    void* rt_create_array() {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::ARRAY;
        return v;
    }

    void rt_array_push(void* arr, void* val) {
        if (arr && val) ((TzdValue*)arr)->arrVal.push_back(*(TzdValue*)val);
    }

    void rt_copy_value(void* dest, void* src) {
        if (dest && src) {
            *(TzdValue*)dest = *(TzdValue*)src;
        }
    }

    void* rt_get_index(void* arr, void* idx) {
        TzdValue* vArr = (TzdValue*)arr;
        TzdValue* vIdx = (TzdValue*)idx;
        if (!vArr || !vIdx) return g_JitPool.next();

        if (vArr->type == TzdValue::MAP) {
            std::string key = TzdInterpreter::getAsString(*vIdx);
            TzdValue* res = g_JitPool.next();
            auto it = vArr->mapVal.find(key);
            if (it != vArr->mapVal.end()) *res = it->second;
            return res;
        }

        int i = (int)TzdInterpreter::getAsDoubleInternal(*vIdx);

        if (vArr->type == TzdValue::ARRAY && i >= 0 && i < (int)vArr->arrVal.size()) {
            TzdValue* res = g_JitPool.next();
            *res = vArr->arrVal[i];
            return res;
        }
        if (vArr->type == TzdValue::STRING && i >= 0 && i < (int)vArr->sVal.size()) {
            return rt_create_str(std::string(1, vArr->sVal[i]).c_str());
        }
        return g_JitPool.next();
    }

    void* rt_call_sub(const char* funcName, int argCount, ...) {
        if (!g_CurrentInterpreter) return g_JitPool.next();

        std::vector<TzdValue> args;
        args.reserve(argCount);
        va_list ap;
        va_start(ap, argCount);
        for (int i = 0; i < argCount; i++) {
            void* argRaw = va_arg(ap, void*);
            if (argRaw) args.push_back(*(TzdValue*)argRaw);
        }
        va_end(ap);

        TzdValue funcObj = g_CurrentInterpreter->getVariable(funcName, nullptr);

        if (funcObj.type == TzdValue::FUNCTION && funcObj.jittedPtr) {
            TzdValue* result = g_JitPool.next();

            g_CurrentInterpreter->m_argFrameStack.push_back(std::move(args));
            g_CurrentInterpreter->m_argPtrStack.push_back(g_CurrentInterpreter->m_argFrameStack.back().data());

            // 【修复】：格式化源文件路径与行号
            std::string fileLoc = formatSourcePath(funcObj.sourceFile);
            int line = funcObj.line;
            std::string frameName = std::string(funcName) + " (" + fileLoc;
            if (line > 0) frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);

            funcObj.jittedPtr(g_CurrentInterpreter, result);

            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_argPtrStack.pop_back();
            g_CurrentInterpreter->m_argFrameStack.pop_back();

            return result;
        }

        TzdValue res = g_CurrentInterpreter->callFunction(funcObj, args);
        TzdValue* poolVal = g_JitPool.next();
        *poolVal = res;
        return poolVal;
    }

    void* rt_call_sub_f1(const char* funcName, void* arg0) {
        if (!g_CurrentInterpreter || !arg0) return g_JitPool.next();

        TzdValue* funcObj = nullptr;
        auto& scopes = g_CurrentInterpreter->scopes;
        if (!scopes.empty()) {
            auto it = scopes.front().find(funcName);
            if (it != scopes.front().end()) funcObj = &it->second;
        }
        if (funcObj && funcObj->type == TzdValue::FUNCTION && funcObj->jittedPtr) {
            TzdValue* result = g_JitPool.next();
            std::vector<TzdValue> savedArgs = std::move(g_CurrentInterpreter->m_currentArgs);
            g_CurrentInterpreter->m_currentArgs.assign(1, *(TzdValue*)arg0);

            // 【修复】：格式化源文件路径与行号
            std::string fileLoc = formatSourcePath(funcObj->sourceFile);
            int line = funcObj->line;
            std::string frameName = std::string(funcName) + " (" + fileLoc;
            if (line > 0) frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);

            funcObj->jittedPtr(g_CurrentInterpreter, result);

            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_currentArgs = std::move(savedArgs);
            return result;
        }

        return rt_call_sub(funcName, 1, arg0);
    }

    void rt_push_jit_frame(const char* name) {
        if (g_CurrentInterpreter) {
            g_CurrentInterpreter->m_callStackFrames.push_back(build_jit_frame_string(name));
        }
    }

    void rt_pop_jit_frame() {
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_callStackFrames.empty()) {
            g_CurrentInterpreter->m_callStackFrames.pop_back();
        }
    }

    void rt_replace_jit_frame(const char* name) {
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_callStackFrames.empty()) {
            g_CurrentInterpreter->m_callStackFrames.back() = build_jit_frame_string(name);
        }
    }

    void rt_store_index(void* arr, void* idx, void* val) {
        if (!arr || !idx || !val) return;
        TzdValue* vArr = (TzdValue*)arr;
        TzdValue* vIdx = (TzdValue*)idx;
        TzdValue* vVal = (TzdValue*)val;
        int i = (int)TzdInterpreter::getAsDoubleInternal(*vIdx);
        if (vArr->type == TzdValue::ARRAY) {
            if (i >= 0 && i < (int)vArr->arrVal.size()) {
                vArr->arrVal[i] = *vVal;
            }
            else {
                std::cerr << "[JIT 运行时错误] 数组下标越界: 尝试写入索引 " << i
                    << ", 但数组大小为 " << vArr->arrVal.size() << std::endl;
            }
            return;
        }
        if (vArr->type == TzdValue::MAP) {
            vArr->mapVal[vIdx->sVal] = *vVal;
            return;
        }
        std::cerr << "[JIT 运行时错误] 该数据类型不支持下标赋值操作" << std::endl;
    }

    void rt_set_location(int line, int col) {
        if (g_CurrentInterpreter) {
            g_CurrentInterpreter->m_jitLine = (size_t)line;
            g_CurrentInterpreter->m_jitColumn = (size_t)col;
        }
    }

    void rt_store_native_to_ptr(void* dest, double val) {
        if (!dest) return;
        TzdValue* v = reinterpret_cast<TzdValue*>(dest);
        *v = TzdValue(val);
    }

    void rt_set_last_ret(void* val) {
        if (!val) return;
        TzdValue* poolVal = g_JitPool.next();
        *poolVal = *(TzdValue*)val;
        g_LastJitValue = poolVal;
    }

    void* rt_create_inst(const char* name) {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError) return g_JitPool.next();

        TzdClassDef* def = TzdOopManager::getClass(name);
        if (!def) {
            if (g_CurrentInterpreter) g_CurrentInterpreter->reportJitError(std::string("找不到类定义: ") + name );
            return g_JitPool.next();
        }
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::INSTANCE;
        v->setInstance(new TzdInstance(def));
        return v;
    }

    static thread_local const char* s_lastClassName = nullptr;
    static thread_local TzdClassDef* s_lastClassDef = nullptr;

    void* rt_create_inst_args(const char* name, int argCount, TzdValue* args) {
        TzdClassDef* def = (name && name == s_lastClassName) ? s_lastClassDef : nullptr;
        if (!def) {
            def = TzdOopManager::getClass(name);
            if (def) {
                s_lastClassName = name;
                s_lastClassDef = def;
            }
        }
        if (!def) {
            if (g_CurrentInterpreter) g_CurrentInterpreter->reportJitError(std::string("找不到类定义: '") + name + "' (是否忘记 import?)");
            return g_JitPool.next();
        }

        TzdValue* instVal = g_JitPool.next();
        instVal->type = TzdValue::INSTANCE;
        instVal->setInstance(new TzdInstance(def));

        ClassConstructor* ctor = def->findConstructor((size_t)argCount);
        if (!ctor && argCount == 0 && !def->constructors.empty()) {
            ctor = def->findConstructor(0);
        }
        if (!ctor) return instVal;

        if (ctor->jittedPtr) {
            TzdValue* ignored = g_JitPool.next();
            g_CurrentInterpreter->m_callFrameStack.push_back({ instVal, args, argCount });

            if (!TzdDebugger::g_DebugActive) {
                ctor->jittedPtr(g_CurrentInterpreter, ignored);
                g_CurrentInterpreter->m_callFrameStack.pop_back();
                return instVal;
            }

            // 【修改】：基于构造函数（ctor）的 sourceFile 和 line 字段，拼接 Java 风格构造函数栈帧
            std::string fileLoc = formatSourcePath(ctor->sourceFile);
            int line = ctor->line;
            std::string frameName = std::string(def->fullName) + "." + def->simpleName;
            frameName += " (" + fileLoc;
            if (line > 0) frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            ctor->jittedPtr(g_CurrentInterpreter, ignored);
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_callFrameStack.pop_back();
            return instVal;
        }
        else if (g_CurrentInterpreter && ctor->body) {
            std::vector<TzdValue> callArgs;
            callArgs.reserve(argCount);
            for (int i = 0; i < argCount; ++i) callArgs.push_back(args[i]);
            TzdValue ctorFunc(def->simpleName, ctor->params, ctor->body);
            ctorFunc.jittedPtr = ctor->jittedPtr;
            ctorFunc.setInstance(instVal->instanceVal);

            ctorFunc.sourceFile = ctor->sourceFile;
            ctorFunc.line = ctor->line;
            ctorFunc.column = ctor->column;

            g_CurrentInterpreter->callFunction(ctorFunc, callArgs);
        }
        return instVal;
    }

    void* rt_call_value_fast(void* funcPtr, int argCount, TzdValue* args) {
        if (!g_CurrentInterpreter || !funcPtr) return g_JitPool.next();
        TzdValue* funcObj = (TzdValue*)funcPtr;

        if (funcObj->type == TzdValue::FUNCTION && funcObj->jittedPtr) {
            TzdValue* result = g_JitPool.next();
            if (!TzdDebugger::g_DebugActive) {
                if (!funcObj->instanceVal) {
                    // Fast path: no bound `this`, reuse caller-provided arg array directly.
                    g_CurrentInterpreter->m_argPtrStack.push_back(args);
                    funcObj->jittedPtr(g_CurrentInterpreter, result);
                    g_CurrentInterpreter->m_argPtrStack.pop_back();
                    return result;
                }

                // Bound method path: avoid heap allocations for common small-arity calls.
                constexpr int kInlineArgs = 8;
                if (argCount + 1 <= kInlineArgs) {
                    TzdValue frameInline[kInlineArgs];
                    frameInline[0] = TzdValue(funcObj->instanceVal);
                    for (int i = 0; i < argCount; ++i) frameInline[i + 1] = args[i];
                    g_CurrentInterpreter->m_argPtrStack.push_back(frameInline);
                    funcObj->jittedPtr(g_CurrentInterpreter, result);
                    g_CurrentInterpreter->m_argPtrStack.pop_back();
                    return result;
                }

                std::vector<TzdValue> frame;
                frame.reserve((size_t)argCount + 1);
                frame.push_back(TzdValue(funcObj->instanceVal));
                for (int i = 0; i < argCount; ++i) frame.push_back(args[i]);
                g_CurrentInterpreter->m_argPtrStack.push_back(frame.data());
                funcObj->jittedPtr(g_CurrentInterpreter, result);
                g_CurrentInterpreter->m_argPtrStack.pop_back();
                return result;
            }

            std::vector<TzdValue> frame;
            frame.reserve((size_t)argCount + (funcObj->instanceVal ? 1 : 0));
            if (funcObj->instanceVal) frame.push_back(TzdValue(funcObj->instanceVal));
            for (int i = 0; i < argCount; ++i) frame.push_back(args[i]);
            g_CurrentInterpreter->m_argPtrStack.push_back(frame.data());

            std::string fileLoc = formatSourcePath(funcObj->sourceFile);
            int line = funcObj->line;
            std::string frameName = funcObj->name.empty() ? "<anonymous>" : funcObj->name;
            if (funcObj->instanceVal && funcObj->instanceVal->definition) {
                frameName = funcObj->instanceVal->definition->fullName + "." + frameName;
            }
            frameName += " (" + fileLoc;
            if (line > 0) frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            funcObj->jittedPtr(g_CurrentInterpreter, result);
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_argPtrStack.pop_back();
            return result;
        }

        std::vector<TzdValue> callArgs;
        callArgs.reserve(argCount);
        for (int i = 0; i < argCount; ++i) callArgs.push_back(args[i]);
        TzdValue res = g_CurrentInterpreter->callFunction(*funcObj, callArgs);
        TzdValue* poolVal = g_JitPool.next();
        *poolVal = res;
        return poolVal;
    }

    void* rt_tzd_get_member(void* inst, int32_t selector, const char* name);

    void* rt_tzd_call_method(void* objPtr, int32_t selector, const char* name, int argCount, TzdValue* args) {
        if (!objPtr || !g_CurrentInterpreter) return g_JitPool.next();
        TzdValue* v = (TzdValue*)objPtr;
        if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition) {
            TzdClassDef* cls = v->instanceVal->definition;
            if (const TzdMemberSlot* slot = cls->tzdDispatch.find((TzdSelector)selector)) {
                if (slot->method && slot->method->jittedPtr) {
                    TzdValue* result = g_JitPool.next();
                    g_CurrentInterpreter->m_callFrameStack.push_back({ v, args, argCount });
                    slot->method->jittedPtr(g_CurrentInterpreter, result);
                    g_CurrentInterpreter->m_callFrameStack.pop_back();
                    return result;
                }
            }
        }
        void* callee = rt_tzd_get_member(objPtr, selector, name);
        return rt_call_value_fast(callee, argCount, args);
    }

    // ------------------------------------------------------------------
    // tzd selector dispatch — bound-method 构造助手
    // rt_tzd_get_member 的 selector 热路径与 tzd_get_member_by_name 的 name 回退
    // 路径共用这两个助手，保证 selector 路径与旧 name 路径语义完全一致。
    // 复用 g_JitPool 槽位时全量覆盖函数元数据，避免上一轮残留泄漏。
    // ------------------------------------------------------------------
    static TzdValue* tzd_make_class_method_value(ClassMethod* method) {
        TzdValue* res = g_JitPool.next();
        res->nativeFunc = nullptr;            // 清除残留 native 句柄
        res->jittedPtr = method->jittedPtr;   // 读 live method->jittedPtr
        res->funcBody = method->body;
        res->params = method->params;
        res->paramTypes = method->paramTypes;
        res->sourceFile = method->sourceFile;
        res->line = method->line;
        res->column = method->column;
        res->name = method->name;
        res->type = method->isNative ? TzdValue::NATIVE_FUNCTION : TzdValue::FUNCTION;
        if (method->isNative) res->nativeFunc = method->nativeWrapper;
        return res;
    }

    static TzdValue* tzd_make_bound_method_value(TzdInstance* recv, ClassMethod* method) {
        TzdValue* res = g_JitPool.next();
        res->name = method->name;
        res->setInstance(recv);              // NEVER raw assign — 走 refcount retain/release
        res->jittedPtr = method->jittedPtr;  // 读 live method->jittedPtr（JIT 重编后即时可见）
        res->sourceFile = method->sourceFile;
        res->line = method->line;
        res->column = method->column;
        res->nativeFunc = nullptr;           // 默认清空，避免池槽位残留
        if (method->isNative) {
            res->type = TzdValue::NATIVE_FUNCTION;
            res->nativeFunc = method->nativeWrapper;
            res->funcBody = method->body;
            res->params = method->params;
            res->paramTypes = method->paramTypes;
        }
        else {
            res->type = TzdValue::FUNCTION;
            if (method->jittedPtr && !TzdDebugger::g_DebugActive) {
                // 热路径：call bridge 直接用 jittedPtr，跳过 params/body 拷贝；
                // 但仍清空这些字段，避免池槽位上一轮残留的 func 元数据。
                res->funcBody = nullptr;
                res->params.clear();
                res->paramTypes.clear();
            }
            else {
                // 逃逸 / 调试 / 未 JIT：保留 params/body 供 debug 回退与解释器回退。
                res->funcBody = method->body;
                res->params = method->params;
                res->paramTypes = method->paramTypes;
            }
        }
        return res;
    }

    // name-based 解析核心（不内联 selector，供回退使用；不递归回 rt_get_member）。
    static TzdValue* tzd_get_member_by_name(void* inst, const char* name) {
        TzdValue* v = (TzdValue*)inst;
        if (v->type == TzdValue::CLASS_DEF && v->classDefVal) {
            if (TzdValue* direct = v->classDefVal->findStaticValue(name)) return direct;
            if (ClassMethod* method = v->classDefVal->findMethod(name)) {
                return tzd_make_class_method_value(method);
            }
        }
        if (v->type == TzdValue::INSTANCE && v->instanceVal) {
            if (TzdValue* direct = v->instanceVal->getMemberPtr(name)) {
                return direct;
            }
            if (v->instanceVal->definition) {
                if (ClassMethod* method = v->instanceVal->definition->findMethod(name)) {
                    return tzd_make_bound_method_value(v->instanceVal, method);
                }
            }
        }
        return g_JitPool.next();
    }

    // selector 热路径：O(1) dispatch table，name 仅作冷路径诊断/回退。
    void* rt_tzd_get_member(void* inst, int32_t selector, const char* name) {
        TzdValue* v = (TzdValue*)inst;
        if (selector != 0) {
            if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition) {
                if (const TzdMemberSlot* slot =
                        v->instanceVal->definition->tzdDispatch.find((TzdSelector)selector)) {
                    // 实例字段优先：返回活动槽指针（零拷贝）。
                    if (slot->fieldIndex >= 0) {
                        if (TzdValue* direct = v->instanceVal->getMemberPtrByIndex(slot->fieldIndex))
                            return direct;
                    }
                    if (slot->method) {
                        return tzd_make_bound_method_value(v->instanceVal, slot->method);
                    }
                }
            }
            else if (v->type == TzdValue::CLASS_DEF && v->classDefVal) {
                if (const TzdMemberSlot* slot =
                        v->classDefVal->tzdDispatch.find((TzdSelector)selector)) {
                    // 类访问：staticValue 在 method 之前（isStatic 语义）。
                    if (slot->staticValue) return slot->staticValue;
                    if (slot->method) return tzd_make_class_method_value(slot->method);
                }
            }
        }
        // 冷路径：selector 未解析或表未命中 —— name 解析（不再内联，避免递归）。
        return tzd_get_member_by_name(inst, name);
    }

    // 兼容入口：旧 emit 点仍以 name 调用；此处做 slow interning 后委托 selector 路径。
    void* rt_get_member(void* inst, const char* name) {
        TzdSelector sel = name ? tzdInternSelector(name) : 0;
        return rt_tzd_get_member(inst, (int32_t)sel, name);
    }

    // name-based 写入核心。
    static void tzd_store_member_by_name(void* inst, const char* name, void* val) {
        if (!inst || !val) return;
        TzdValue* v = (TzdValue*)inst;
        TzdValue copy = *(TzdValue*)val;
        if (v->type == TzdValue::CLASS_DEF && v->classDefVal) {
            if (TzdValue* direct = v->classDefVal->findStaticValue(name)) {
                *direct = copy;
                return;
            }
        }
        if (v->type == TzdValue::INSTANCE && v->instanceVal) {
            v->instanceVal->setMember(name, copy);
        }
    }

    // selector 热路径写入。
    void rt_tzd_store_member(void* inst, int32_t selector, const char* name, void* val) {
        if (!inst || !val) return;
        TzdValue* v = (TzdValue*)inst;
        TzdValue* src = (TzdValue*)val;
        if (selector != 0) {
            if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition) {
                if (const TzdMemberSlot* slot =
                        v->instanceVal->definition->tzdDispatch.find((TzdSelector)selector)) {
                    if (slot->fieldIndex >= 0) {
                        // setMemberByIndex 经 operator= 全量覆盖，清除上一轮残留
                        // 的 func 元数据（params/body/jittedPtr），避免陈旧。
                        v->instanceVal->setMemberByIndex(slot->fieldIndex, *src);
                        return;
                    }
                }
            }
            else if (v->type == TzdValue::CLASS_DEF && v->classDefVal) {
                if (const TzdMemberSlot* slot =
                        v->classDefVal->tzdDispatch.find((TzdSelector)selector)) {
                    if (slot->staticValue) {
                        *slot->staticValue = *src;
                        return;
                    }
                }
            }
        }
        tzd_store_member_by_name(inst, name, val);
    }

    // F2: Lightweight field store via pre-resolved pointer.
    // Avoids dispatch lookup — pair with rt_tzd_get_member (readonly, CSE-able).
    void rt_store_field_ptr(void* fieldPtr, void* val) {
        if (!fieldPtr || !val) return;
        *(TzdValue*)fieldPtr = *(TzdValue*)val;
    }

    // 兼容入口。
    void rt_store_member(void* inst, const char* name, void* val) {
        TzdSelector sel = name ? tzdInternSelector(name) : 0;
        rt_tzd_store_member(inst, (int32_t)sel, name, val);
    }

    void* rt_create_lambda_value(const char* lambdaName) {
        if (!g_CurrentInterpreter) return g_JitPool.next();
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::FUNCTION;
        v->name = lambdaName;
        if (g_CurrentInterpreter->m_jitEngine) {
            v->jittedPtr = (void(*)(void*, void*))g_CurrentInterpreter->m_jitEngine->lookupSymbolAsPtr(lambdaName);
        }
        return v;
    }

    // 强制类型转换
    void* rt_cast(void* valPtr, const char* typeName) {
        TzdValue* val = (TzdValue*)valPtr;
        TzdValue* res = g_JitPool.next();
        std::string target(typeName);
        try {
            if (target == "int" || target == "i32" || target == "long" || target == "i64") {
                res->type = (target == "int" || target == "i32") ? TzdValue::INT : TzdValue::LONG;
                res->lVal = (long long)TzdInterpreter::getAsDoubleInternal(*val);
            }
            else if (target == "float" || target == "double") {
                res->type = TzdValue::DOUBLE;
                res->dVal = TzdInterpreter::getAsDoubleInternal(*val);
            }
            else if (target == "string") {
                res->type = TzdValue::STRING;
                res->sVal = TzdInterpreter::getAsString(*val);
            }
            else if (target == "bool") {
                res->type = TzdValue::BOOL;
                res->bVal = (TzdInterpreter::getAsDoubleInternal(*val) != 0);
            }
            else if (target == "function" || target == "fn") {
                if (val->type == TzdValue::FUNCTION || val->type == TzdValue::NATIVE_FUNCTION) {
                    *res = *val;
                }
                else {
                    res->type = TzdValue::NONE;
                }
            }
            else {
                if (val->type == TzdValue::INSTANCE) {
                    if (TzdOopManager::isInstanceOf(val->instanceVal, target)) *res = *val;
                }
            }
        }
        catch (...) { res->type = TzdValue::NONE; }
        return res;
    }

    void rt_print(void* a) {
        std::cout << Utf8ToAnsi(TzdInterpreter::getAsString(*(TzdValue*)a)) << " " << std::flush;
    }

    void rt_print_newline() {
        std::cout << std::endl;
    }

    void* rt_create_native_val(const char* name, void* addr) {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::NATIVE_FUNCTION;
        v->name = name;
        v->ptrVal = addr;
        return v;
    }

    bool rt_type_check(void* objPtr, const char* typeName) {
        if (!objPtr || !typeName) return false;
        TzdValue* val = (TzdValue*)objPtr;
        std::string target(typeName);
        if (val->type == TzdValue::INSTANCE) {
            return TzdOopManager::isInstanceOf(val->instanceVal, typeName);
        }
        if (target == "function" || target == "fn") {
            return val->type == TzdValue::FUNCTION || val->type == TzdValue::NATIVE_FUNCTION;
        }
        if (target == "int" || target == "i32" || target == "long" || target == "i64") {
            return val->type >= TzdValue::SBYTE && val->type <= TzdValue::ULONG;
        }
        if (target == "float" || target == "double") {
            return val->type == TzdValue::FLOAT || val->type == TzdValue::DOUBLE;
        }
        if (target == "string") {
            return val->type == TzdValue::STRING;
        }
        if (target == "bool") {
            return val->type == TzdValue::BOOL;
        }
        if (target == "ptr" || target == "pointer" || target == "hwnd") {
            return val->type == TzdValue::POINTER || val->type == TzdValue::NONE;
        }
        for (auto& anno : val->annotations) if (anno == typeName) return true;
        return false;
    }

    void rt_call_super(void* thisValPtr, int argCount, ...) {
        TzdValue* thisVal = (TzdValue*)thisValPtr;
        TzdInstance* inst = thisVal->instanceVal;
        TzdClassDef* parent = TzdOopManager::getClass(inst->definition->parentName);
        if (!parent) return;

        std::vector<TzdValue> args;
        va_list ap;
        va_start(ap, argCount);
        for (int i = 0; i < argCount; i++) {
            args.push_back(*(TzdValue*)va_arg(ap, void*));
        }
        va_end(ap);

        const ClassConstructor* parentCtor = parent->findConstructor(args.size());
        if (!parentCtor) return;

        TzdValue ctorFunc(parent->simpleName, parentCtor->params, parentCtor->body);
        ctorFunc.jittedPtr = parentCtor->jittedPtr;
        ctorFunc.setInstance(inst);

        // 【修复】：复制源文件与行列元数据，确保继承链的 JIT 堆栈能够正常定位
        ctorFunc.sourceFile = parentCtor->sourceFile;
        ctorFunc.line = parentCtor->line;
        ctorFunc.column = parentCtor->column;

        g_CurrentInterpreter->callFunction(ctorFunc, args);
    }

    static thread_local jmp_buf* g_tzdCatchJmp = nullptr;
    static thread_local TzdValue g_tzdThrownValue;
    
    void* rt_alloc_jmp_buf() {
        return new jmp_buf();
    }

    void* rt_get_fatal_jmp() { return g_tzdFatalJmp; }
    void rt_set_fatal_jmp(void* buf) { g_tzdFatalJmp = static_cast<jmp_buf*>(buf); }

    void rt_free_jmp_buf(void* buf) {
        delete[] static_cast<jmp_buf*>(buf);
    }

    /*int rt_enter_try_buf(void* buf) {
        g_tzdCatchJmp = static_cast<jmp_buf*>(buf);
        return setjmp(*g_tzdCatchJmp);
    }
    void rt_leave_try() {
        g_tzdCatchJmp = nullptr;
    }
    */

    void* rt_get_catch_jmp() {
        return g_tzdCatchJmp;
    }

    void rt_set_catch_jmp(void* buf) {
        g_tzdCatchJmp = static_cast<jmp_buf*>(buf);
    }

    void rt_throw(void* valPtr) {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError) return;

        if (valPtr) g_tzdThrownValue = *(TzdValue*)valPtr;
        else g_tzdThrownValue = TzdValue("Unknown error");

        std::vector<std::string> currentTrace;
        if (g_CurrentInterpreter) {
            currentTrace = g_CurrentInterpreter->m_callStackFrames;
            currentTrace.push_back("<throw> at line " + std::to_string(g_CurrentInterpreter->m_jitLine));
        }

        // 如果 JIT 环境或者外部设置了捕获点，执行跳转
        if (g_tzdCatchJmp) {
            longjmp(*g_tzdCatchJmp, 1);
        }

        // 否则作为未捕获异常抛出给 C++ 外层解释器处理
        if (g_CurrentInterpreter) {
            g_CurrentInterpreter->m_jitUnhandledThrow = std::make_unique<TzdThrowException>(g_tzdThrownValue, currentTrace);
            g_CurrentInterpreter->m_hasJitError = true;
        }
    }

    void* rt_get_thrown() {
        return &g_tzdThrownValue;
    }

    void rt_push_catch_scope(const char* name, void* valPtr) {
        if (!g_CurrentInterpreter || !name || !valPtr) return;
        std::unordered_map<std::string, TzdValue> scope;
        scope[name] = *(TzdValue*)valPtr;
        g_CurrentInterpreter->scopes.push_back(scope);
    }

    void rt_pop_catch_scope() {
        if (!g_CurrentInterpreter || g_CurrentInterpreter->scopes.empty()) return;
        g_CurrentInterpreter->scopes.pop_back();
    }

    double rt_to_double_fast(void* v) {
        // 防御性检查：防止非法指针或 raw boolean 渗入
        if ((uintptr_t)v < 4096) return (double)(uintptr_t)v;

        TzdValue* val = (TzdValue*)v;
        if (val->type == TzdValue::DOUBLE || val->type == TzdValue::FLOAT) return val->dVal;
        if (val->type == TzdValue::INT || val->type == TzdValue::LONG) return (double)val->lVal;
        if (val->type == TzdValue::BOOL) return val->bVal ? 1.0 : 0.0;
        return 0.0;
    }

    void* rt_stabilize_value(void* v) {
        if (!v) return nullptr;
        TzdValue* src = (TzdValue*)v;
        TzdValue* stable = new TzdValue(*src);
        if (g_CurrentInterpreter) {
            g_CurrentInterpreter->trackJitValue(stable);
        }
        return stable;
    }

    // 直接用原生 double 索引读取，无装箱开销
    double rt_get_index_native_d(void* arr, int idx) {
        TzdValue* vArr = (TzdValue*)arr;
        if (!vArr) return 0.0;
        if (vArr->isNativeDoubleArr && idx >= 0 && idx < (int)vArr->nativeArr.size())
            return vArr->nativeArr[idx];
        // fallback：boxed array
        if (vArr->type == TzdValue::ARRAY && idx >= 0 && idx < (int)vArr->arrVal.size())
            return TzdInterpreter::getAsDoubleInternal(vArr->arrVal[idx]);
        return 0.0;
    }

    // 直接写原生 double，无装箱开销
    void rt_store_index_native_d(void* arr, int idx, double val) {
        TzdValue* vArr = (TzdValue*)arr;
        if (vArr->isNativeDoubleArr && idx >= 0 && idx < (int)vArr->nativeArr.size()) {
            vArr->nativeArr[idx] = val;
            return;
        }
        // fallback：转换为 TzdValue 再写
        TzdValue boxed(val);
        TzdValue idxVal((int)idx);
        rt_store_index(arr, &idxVal, &boxed);
    }

    double rt_get_index_native_d_dyn(void* arr, double idxD) {
        return rt_get_index_native_d(arr, (int)idxD);
    }

    void rt_store_index_native_d_dyn(void* arr, double idxD, double val) {
        rt_store_index_native_d(arr, (int)idxD, val);
    }

    // 创建纯 double 数组字面量的快速路径
    void* rt_create_native_double_arr(int count, double* vals) {
        TzdValue* v = g_JitPool.next();
        v->type = TzdValue::ARRAY;
        v->isNativeDoubleArr = true;
        v->nativeArr.resize(count);
        for (int i = 0; i < count; i++) v->nativeArr[i] = vals[i];
        return v;
    }
}

llvm::AllocaInst* TzdCompiler::CreateEntryBlockAlloca(llvm::Type* Ty, const std::string& Name, llvm::Value* ArraySize) {
    Function* TheFunction = m_builder.GetInsertBlock()->getParent();
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Ty, ArraySize, Name);
}

llvm::AllocaInst* TzdCompiler::CreateEntryBlockAlloca(llvm::Type* Ty, llvm::Value* ArraySize, const std::string& Name) {
    llvm::Function* TheFunction = m_builder.GetInsertBlock()->getParent();
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Ty, ArraySize, Name);
}

void TzdCompiler::compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx,
    TzdLangParser::MethodDeclContext* methodCtx) {
    s_tryJmpBufStack.clear();
    s_loopLevel = 0;
    std::string className = classCtx->qualifiedName(0)->getText();
    std::string methodName = methodCtx->IDENTIFIER()->getText();
    std::string fullInternalName = className + "_" + methodName;
    // 实例方法的签名：void Method(void* interp, void* retVal, void* thisPtr, ...)
    std::vector<Type*> params = { m_ptrTy, m_ptrTy };
    Function* func = Function::Create(
        FunctionType::get(m_voidTy, params, false),
        Function::ExternalLinkage, fullInternalName, m_module.get()
    );
    BasicBlock* bb = BasicBlock::Create(m_context, "entry", func);
    m_builder.SetInsertPoint(bb);
    Value* thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(0) });
    AllocaInst* thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;
    visit(methodCtx->block());
    if (!m_builder.GetInsertBlock()->getTerminator())
        m_builder.CreateRetVoid();
}

// Add this method to your TzdJitEngine
void TzdJitEngine::jitModule(std::unique_ptr<Module> M) {
    if (!M) return;

    // 收集刚编译的模块中包含的所有 _worker 函数名
    std::vector<std::string> workers;
    for (auto& F : *M) {
        std::string name = F.getName().str();
        if (name.size() > 7 && name.substr(name.size() - 7) == "_worker") {
            workers.push_back(name);
        }
    }

    ThreadSafeModule TSM(std::move(M), m_tsc);
    addModule(std::move(TSM));

    // 解析出实际的机器码内存地址并存入映射表
    for (const auto& w : workers) {
        if (void* ptr = lookupSymbolAsPtr(w)) {
            // 剥离 "_worker" 以及版本号 "_vX"
            std::string baseName = w.substr(0, w.size() - 7);
            size_t lastUnderscore = baseName.find_last_of('_');
            if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v') {
                baseName = baseName.substr(0, lastUnderscore);
            }
            std::unique_lock<std::shared_mutex> lock(s_workerPointersMutex);
            s_workerPointers[baseName] = ptr;
        }
    }
}


std::unique_ptr<llvm::Module> TzdCompiler::extractModule() {
    return std::move(m_module);
}

llvm::Expected<llvm::orc::ExecutorAddr>
TzdJitEngine::lookupSymbol(const std::string& name)
{
    return m_lljit->lookup(name);
}

void* TzdJitEngine::lookupSymbolAsPtr(const std::string& name) {
    auto Sym = lookupSymbol(name);
    if (!Sym) {
        llvm::Error E = Sym.takeError();
        consumeError(std::move(E));
        return nullptr;
    }
    llvm::orc::ExecutorAddr addr = *Sym;
    llvm::JITTargetAddress raw = addr.getValue();
    return reinterpret_cast<void*>(static_cast<uintptr_t>(raw));
}

void TzdCompiler::compileNamedFunction(TzdLangParser::FunctionDeclarationContext* ctx, const std::string& internalName) {
    int argCount = ctx->paramList() ? ctx->paramList()->param().size() : 0;

    // 1. Worker 内部函数返回类型为 m_doubleTy
    std::vector<Type*> workerArgs = { m_ptrTy, m_ptrTy, m_ptrTy };
    Function* workerFunc = Function::Create(
        FunctionType::get(m_doubleTy, workerArgs, false),
        Function::ExternalLinkage,
        internalName + "_worker",
        m_module.get()
    );
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    // Phase B: Create native double worker for function specialization.
    // Only for functions with parameters — 0-param functions have no benefit.
    Function* nativeWorkerFunc = nullptr;
    if (argCount > 0) {
        std::vector<Type*> nativeWorkerArgs = { m_ptrTy };
        for (int i = 0; i < argCount; ++i) {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get()
        );
        nativeWorkerFunc->addFnAttr(llvm::Attribute::AlwaysInline);
    }

    // Register worker for direct call optimization (bypasses rt_call_sub_fast)
    {
        std::string base = internalName;
        size_t us = base.find_last_of('_');
        if (us != std::string::npos && us + 1 < base.size() && base[us + 1] == 'v')
            base = base.substr(0, us);
        s_compiledWorkers[base] = workerFunc;
        if (nativeWorkerFunc) s_compiledNativeWorkers[base] = nativeWorkerFunc;
    }
    s_currentNativeWorkerFunc = nativeWorkerFunc;

    // 2. Entry 依然返回 void
    std::vector<Type*> entryArgs = { m_ptrTy, m_ptrTy };
    Function* entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get()
    );

    BasicBlock* entryBB = BasicBlock::Create(m_context, "entry", entryFunc);
    m_builder.SetInsertPoint(entryBB);
    Value* interp = entryFunc->getArg(0);
    Value* retVal = entryFunc->getArg(1);
    Value* argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "entry_args");
    if (argCount > 0) {
        m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), { argsArray, m_builder.getInt32(argCount) });
    }

    if (ctx->paramList()) {
        for (int i = 0; i < argCount; ++i) {
            Value* argIdx = ConstantInt::get(Type::getInt32Ty(m_context), i);
            Value* argRaw = m_builder.CreateCall(getRtFunc("rt_get_arg"), { argIdx });
            Value* destPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
        }
    }
    m_builder.CreateCall(workerFunc, { interp, retVal, argsArray });
    if (argCount > 0) {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), { argsArray, m_builder.getInt32(argCount) });
    }
    m_builder.CreateRetVoid();

    BasicBlock* workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    s_currentFuncParamNames.clear();

    this->m_currentRetPtr = workerFunc->getArg(1);
    Value* workerArgsArray = workerFunc->getArg(2); // 这里正确定义了 workerArgsArray

    BasicBlock* bodyBB = BasicBlock::Create(m_context, "body", workerFunc);
    s_tailRecurseBB = bodyBB;

    if (ctx->paramList()) {
        auto pList = ctx->paramList()->param();
        for (int i = 0; i < argCount; ++i) {
            std::string pName = getParamName(pList[i]);
            s_currentFuncParamNames.push_back(pName);
            m_declaredLocals.insert(pName);

            Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
            AllocaInst* boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argPtr, boxedAlloc);
            m_namedValues[pName] = boxedAlloc;

            // Native double version — unbox at entry for fast numeric access + native worker
            Value* nativeVal = inlineToDoubleFast(argPtr);
            AllocaInst* nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
            m_builder.CreateStore(nativeVal, nativeAlloc);
            m_nativeDoubleLocals[pName] = nativeAlloc;
        }
    }

    m_builder.CreateBr(bodyBB);
    m_builder.SetInsertPoint(bodyBB);

    visit(ctx->block());

    // 默认返回硬件级的 0.0
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        Value* nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
        m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { m_currentRetPtr, nullVal });
        m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
    }
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;

    // Phase B: Compile native worker body — params are direct doubles, no TzdValue unbox
    if (argCount > 0 && nativeWorkerFunc) {
        BasicBlock* nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();

        // Native worker has no retVal slot — returns double directly
        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        BasicBlock* nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        s_tailRecurseBB = nativeBodyBB;

        if (ctx->paramList()) {
            auto pList = ctx->paramList()->param();
            for (int i = 0; i < argCount; ++i) {
                std::string pName = getParamName(pList[i]);
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                // Store double arg into alloca — mem2reg will promote to register.
                // Using alloca so tail-recursion path (CreateStore) works correctly.
                AllocaInst* nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeWorkerFunc->getArg(i + 1), nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(ctx->block());

        if (!m_builder.GetInsertBlock()->getTerminator()) {
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
    }

    s_currentNativeWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;
}


void TzdCompiler::compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::MethodDeclContext* methodCtx, const std::string& internalName) {
    Function* func = Function::Create(
        FunctionType::get(m_voidTy, { m_ptrTy, m_ptrTy }, false),
        Function::ExternalLinkage, internalName, m_module.get()
    );
    BasicBlock* bb = BasicBlock::Create(m_context, "entry", func);
    m_builder.SetInsertPoint(bb);
    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    m_declaredLocals.insert("this");
    m_currentRetPtr = func->getArg(1);

    Value* thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(0) });
    AllocaInst* thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;

    if (methodCtx->paramList()) {
        auto pList = methodCtx->paramList()->param();
        for (int i = 0; i < (int)pList.size(); ++i) {
            std::string pName = getParamName(pList[i]);
            m_declaredLocals.insert(pName);
            Value* argVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(i + 1) });
            AllocaInst* alloc = m_builder.CreateAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argVal, alloc);
            m_namedValues[pName] = alloc;
        }
    }

    m_namedValues["$retval"] = m_builder.CreateAlloca(m_ptrTy, nullptr, "$retval");
    visit(methodCtx->block());
    if (!m_builder.GetInsertBlock()->getTerminator()) m_builder.CreateRetVoid();
}

void TzdCompiler::compileConstructor(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::ConstructorDeclContext* ctorCtx, const std::string& internalName) {
    Function* func = Function::Create(
        FunctionType::get(m_voidTy, { m_ptrTy, m_ptrTy }, false),
        Function::ExternalLinkage, internalName, m_module.get()
    );
    BasicBlock* bb = BasicBlock::Create(m_context, "entry", func);
    m_builder.SetInsertPoint(bb);
    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    m_declaredLocals.insert("this");
    m_currentRetPtr = func->getArg(1);

    Value* thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(0) });
    AllocaInst* thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;

    if (ctorCtx->paramList()) {
        auto pList = ctorCtx->paramList()->param();
        for (int i = 0; i < (int)pList.size(); ++i) {
            std::string pName = getParamName(pList[i]);
            m_declaredLocals.insert(pName);
            Value* argVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(i + 1) });
            AllocaInst* alloc = m_builder.CreateAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argVal, alloc);
            m_namedValues[pName] = alloc;
        }
    }

    m_namedValues["$retval"] = m_builder.CreateAlloca(m_ptrTy, nullptr, "$retval");
    for (auto stmt : ctorCtx->block()->statement()) {
        try {
            visit(stmt);
        }
        catch (const std::bad_any_cast&) {
            //agentLogJit("D", "compileConstructor.stmt", stmt->getText().substr(0, 80).c_str());
            throw;
        }
    }
    if (!m_builder.GetInsertBlock()->getTerminator()) m_builder.CreateRetVoid();
}

void TzdCompiler::compileNamedFunction(TzdLangParser::BlockContext* block, TzdLangParser::ParamListContext* params, const std::string& internalName) {
    int argCount = params ? params->param().size() : 0;

    // 1. Worker 内部函数返回类型为 m_doubleTy
    std::vector<Type*> workerArgs = { m_ptrTy, m_ptrTy, m_ptrTy };
    Function* workerFunc = Function::Create(
        FunctionType::get(m_doubleTy, workerArgs, false),
        Function::ExternalLinkage,
        internalName + "_worker",
        m_module.get()
    );
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    // Phase B: Create native double worker for function specialization.
    // Only for functions with parameters — 0-param functions have no benefit.
    Function* nativeWorkerFunc = nullptr;
    if (argCount > 0) {
        std::vector<Type*> nativeWorkerArgs = { m_ptrTy };
        for (int i = 0; i < argCount; ++i) {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get()
        );
        nativeWorkerFunc->addFnAttr(llvm::Attribute::AlwaysInline);
    }

    // Register worker for direct call optimization (bypasses rt_call_sub_fast)
    {
        std::string base = internalName;
        size_t us = base.find_last_of('_');
        if (us != std::string::npos && us + 1 < base.size() && base[us + 1] == 'v')
            base = base.substr(0, us);
        s_compiledWorkers[base] = workerFunc;
        if (nativeWorkerFunc) s_compiledNativeWorkers[base] = nativeWorkerFunc;
    }
    s_currentNativeWorkerFunc = nativeWorkerFunc;

    // 2. Entry 依然返回 void 供外部 C++ 解释器调用
    std::vector<Type*> entryArgs = { m_ptrTy, m_ptrTy };
    Function* entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get()
    );

    BasicBlock* entryBB = BasicBlock::Create(m_context, "entry", entryFunc);
    m_builder.SetInsertPoint(entryBB);
    Value* interp = entryFunc->getArg(0);
    Value* retVal = entryFunc->getArg(1);
    Value* argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "entry_args");
    if (argCount > 0) {
        m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), { argsArray, m_builder.getInt32(argCount) });
    }

    if (params) {
        for (int i = 0; i < argCount; ++i) {
            Value* argRaw = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(i) });
            Value* destPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { destPtr, argRaw });
        }
    }
    m_builder.CreateCall(workerFunc, { interp, retVal, argsArray });
    if (argCount > 0) {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), { argsArray, m_builder.getInt32(argCount) });
    }
    m_builder.CreateRetVoid();

    BasicBlock* workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    s_currentFuncParamNames.clear();

    this->m_currentRetPtr = workerFunc->getArg(1);
    Value* workerArgsArray = workerFunc->getArg(2); // 这里正确定义了 workerArgsArray

    BasicBlock* bodyBB = BasicBlock::Create(m_context, "body", workerFunc);
    s_tailRecurseBB = bodyBB;

    if (params) {
        auto pList = params->param();
        for (int i = 0; i < argCount; ++i) {
            std::string pName = getParamName(pList[i]);
            s_currentFuncParamNames.push_back(pName);
            m_declaredLocals.insert(pName);

            Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));

            // Boxed version (for non-numeric access like string concatenation)
            AllocaInst* boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argPtr, boxedAlloc);
            m_namedValues[pName] = boxedAlloc;

            // Native double version — unbox at function entry for fast numeric access.
            // visitIdExpr checks m_nativeDoubleLocals first, returning the double.
            // Inline unbox: direct GEP+Load on dVal field (no function call)
            Value* nativeVal = inlineToDoubleFast(argPtr);
            AllocaInst* nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
            m_builder.CreateStore(nativeVal, nativeAlloc);
            m_nativeDoubleLocals[pName] = nativeAlloc;
        }
    }

    m_builder.CreateBr(bodyBB);
    m_builder.SetInsertPoint(bodyBB);

    visit(block);

    // 默认返回硬件级的 0.0
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        Value* nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
        m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { m_currentRetPtr, nullVal });
        m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
    }
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;

    // Phase B: Compile native worker body — params are direct doubles, no TzdValue unbox
    if (argCount > 0 && nativeWorkerFunc) {
        BasicBlock* nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();

        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        BasicBlock* nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        s_tailRecurseBB = nativeBodyBB;

        if (params) {
            auto pList = params->param();
            for (int i = 0; i < argCount; ++i) {
                std::string pName = getParamName(pList[i]);
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                // Store double arg into alloca — mem2reg will promote to register.
                // Using alloca so tail-recursion path (CreateStore) works correctly.
                AllocaInst* nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeWorkerFunc->getArg(i + 1), nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(block);

        if (!m_builder.GetInsertBlock()->getTerminator()) {
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
    }

    s_currentNativeWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;
}


void TzdJitEngine::executeFunction(const std::string& name, void* interp, void* retVal) {
    auto symOrErr = lookupSymbol(name);
    if (!symOrErr) {
        consumeError(symOrErr.takeError());
        errs() << "executeFunction: symbol not found: " << name << "\n";
        return;
    }
    llvm::orc::ExecutorAddr addr = *symOrErr;
    llvm::JITTargetAddress rawAddr = addr.getValue();
    using FnType = void(*)(void*, void*);
    auto fn = reinterpret_cast<FnType>(rawAddr);
    fn(interp, retVal);
}


TzdJitEngine::TzdJitEngine()
    : m_tsc(std::make_unique<LLVMContext>())
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto J = LLJITBuilder().create();
    if (!J) {
        errs() << "Failed to create LLJIT: " << toString(J.takeError()) << "\n";
        m_lljit.reset();
        return;
    }
    m_lljit = std::move(*J);
    registerRuntimeSymbols();
}

TzdJitEngine::~TzdJitEngine() {
    // Leak LLJIT and ThreadSafeContext to avoid stack overflow during
    // LLVM internal cleanup at program exit.
    (void)m_lljit.release();

    // Move ThreadSafeContext to a heap-allocated container that is never
    // destroyed, so the LLVMContext's refcount never hits zero.
    static std::vector<llvm::orc::ThreadSafeContext>* s_leaked = nullptr;
    if (!s_leaked) s_leaked = new std::vector<llvm::orc::ThreadSafeContext>();
    s_leaked->push_back(std::move(m_tsc));
}

void TzdJitEngine::initLLJIT() {}

void TzdCompiler::setupExternalFunctions() {
    auto addFunc = [&](std::string name, std::vector<Type*> args, Type* ret = nullptr) {
        Function* existing = m_module->getFunction(name);
        if (existing) return existing;
        return Function::Create(
            FunctionType::get(ret ? ret : m_ptrTy, args, false),
            Function::ExternalLinkage,
            name,
            m_module.get()
        );
        };

    addFunc("rt_create_num", { m_doubleTy });
    addFunc("rt_create_str", { m_ptrTy });
    addFunc("rt_create_bool", { m_boolTy });
    addFunc("rt_create_null", {});
    addFunc("rt_create_inst", { m_ptrTy });
    addFunc("rt_create_inst_args", { m_ptrTy, m_int32Ty, m_ptrTy });
    addFunc("rt_create_native_val", { m_ptrTy, m_ptrTy });

    addFunc("rt_resolve_var", { m_ptrTy });
    addFunc("rt_store_var", { m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_get_arg", { Type::getInt32Ty(m_context) });
    addFunc("rt_set_last_ret", { m_ptrTy }, m_voidTy); 

    addFunc("rt_op_add", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_sub", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_mul", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_div", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_mod", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_pow", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_neg", { m_ptrTy });
    addFunc("rt_op_not", { m_ptrTy });
    addFunc("rt_op_sqrt", { m_ptrTy });
    addFunc("rt_op_gt", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_lt", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_ge", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_le", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_eq", { m_ptrTy, m_ptrTy });
    addFunc("rt_op_ne", { m_ptrTy, m_ptrTy });

    addFunc("rt_to_bool", { m_ptrTy }, m_boolTy);
    addFunc("rt_cast", { m_ptrTy, m_ptrTy });

    addFunc("rt_create_array", {});
    addFunc("rt_array_push", { m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_get_index", { m_ptrTy, m_ptrTy });
    addFunc("rt_print", { m_ptrTy }, m_voidTy);

    addFunc("rt_get_member", { m_ptrTy, m_ptrTy });
    addFunc("rt_store_member", { m_ptrTy, m_ptrTy, m_ptrTy }, m_voidTy);

    // tzd selector dispatch —— 热路径用 i32 常量 selector，name 仅冷诊断。
    addFunc("rt_tzd_get_member", { m_ptrTy, m_int32Ty, m_ptrTy });
    addFunc("rt_tzd_store_member", { m_ptrTy, m_int32Ty, m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_tzd_call_method", { m_ptrTy, m_int32Ty, m_ptrTy, m_int32Ty, m_ptrTy });
    // F1: readonly on get_member enables LLVM CSE/LICM to eliminate
    // redundant field reads in loops (same obj+selector = same result)
    // Note: ReadOnly attr not supported on functions in this LLVM version;
    // the AlwaysInlinerPass + mem2reg + EarlyCSE still optimize effectively.
    // F3: help LLVM optimize method calls
    m_module->getFunction("rt_tzd_call_method")->addFnAttr(llvm::Attribute::NoCallback);
    m_module->getFunction("rt_tzd_call_method")->addFnAttr(llvm::Attribute::WillReturn);
    // F2: lightweight field store via cached pointer (split from store_member)
    addFunc("rt_store_field_ptr", { m_ptrTy, m_ptrTy }, m_voidTy);

    addFunc("rt_cast", { m_ptrTy, m_ptrTy });
    addFunc("rt_type_check", { m_ptrTy, m_ptrTy }, m_boolTy);
    addFunc("rt_get_var_ptr", { m_ptrTy });

    addFunc("rt_print_newline", {}, m_voidTy);
    addFunc("rt_to_double_fast", { m_ptrTy }, m_doubleTy);
    addFunc("rt_stabilize_value", { m_ptrTy }, m_ptrTy);
    addFunc("rt_copy_value", { m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_call_sub_f1", { m_ptrTy, m_ptrTy }, m_ptrTy);

    addFunc("rt_call_sub_fast", { m_ptrTy, m_int32Ty, m_ptrTy }, m_ptrTy);
    addFunc("rt_call_value_fast", { m_ptrTy, m_int32Ty, m_ptrTy }, m_ptrTy);
    addFunc("rt_store_native_to_ptr", { m_ptrTy, m_doubleTy }, m_voidTy);
    addFunc("rt_set_null", { m_ptrTy }, m_voidTy);
    addFunc("rt_store_index", { m_ptrTy, m_ptrTy, m_ptrTy }, m_voidTy);

    addFunc("rt_construct_num_at", { m_ptrTy, m_doubleTy }, m_voidTy);
    addFunc("rt_construct_copy_at", { m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_destruct_values", { m_ptrTy, m_int32Ty }, m_voidTy);

    addFunc("rt_get_index_native_d", { m_ptrTy, m_int32Ty }, m_doubleTy);
    addFunc("rt_store_index_native_d", { m_ptrTy, m_int32Ty, m_doubleTy }, m_voidTy);
    addFunc("rt_get_index_native_d_dyn", { m_ptrTy, m_doubleTy }, m_doubleTy);
    addFunc("rt_store_index_native_d_dyn", { m_ptrTy, m_doubleTy, m_doubleTy }, m_voidTy);
    addFunc("rt_create_native_double_arr", { m_int32Ty, m_ptrTy }, m_ptrTy);

    addFunc("rt_push_arg_frame", { m_ptrTy }, m_voidTy);
    addFunc("rt_pop_arg_frame", {}, m_voidTy);

    addFunc("rt_init_tzd_value", { m_ptrTy, m_int32Ty }, m_voidTy);
    addFunc("rt_write_fast_ret", { m_ptrTy, m_ptrTy }, m_voidTy);

    addFunc("rt_get_worker_ptr", { m_ptrTy }, m_ptrTy);

    addFunc("rt_alloc_jmp_buf", {}, m_ptrTy);
    addFunc("rt_free_jmp_buf", { m_ptrTy }, m_voidTy);
    addFunc("rt_get_catch_jmp", {}, m_ptrTy);
    addFunc("rt_set_catch_jmp", { m_ptrTy }, m_voidTy);

#ifdef _WIN32
    // Windows x64 下 _setjmp 的签名需要包含隐藏的帧指针
    Function::Create(FunctionType::get(m_int32Ty, { m_ptrTy, m_ptrTy }, false), Function::ExternalLinkage, "_setjmp", m_module.get());
#else
    Function::Create(FunctionType::get(m_int32Ty, { m_ptrTy }, false), Function::ExternalLinkage, "setjmp", m_module.get());
#endif

    //addFunc("rt_enter_try_buf", { m_ptrTy }, Type::getInt32Ty(m_context));
    //addFunc("rt_leave_try", {}, m_voidTy);
    addFunc("rt_throw", { m_ptrTy }, m_voidTy);
    addFunc("rt_get_thrown", {}, m_ptrTy);
    addFunc("rt_push_catch_scope", { m_ptrTy, m_ptrTy }, m_voidTy);
    addFunc("rt_pop_catch_scope", {}, m_voidTy);

    addFunc("rt_set_location", { m_int32Ty, m_int32Ty }, m_voidTy);
    addFunc("rt_push_jit_frame", { m_ptrTy }, m_voidTy);
    addFunc("rt_pop_jit_frame", {}, m_voidTy);
    addFunc("rt_replace_jit_frame", { m_ptrTy }, m_voidTy);

    addFunc("rt_create_lambda_value", { m_ptrTy });
    Function::Create(
        FunctionType::get(m_ptrTy, { m_ptrTy, Type::getInt32Ty(m_context) }, true),
        Function::ExternalLinkage, "rt_call_super", m_module.get()
    );

    Function::Create(
        FunctionType::get(m_ptrTy, { m_ptrTy, Type::getInt32Ty(m_context) }, true),
        Function::ExternalLinkage, "rt_call_sub", m_module.get()
    );
}

Function* TzdCompiler::getRtFunc(const std::string& name) {
    Function* f = m_module->getFunction(name);
    if (!f) {
        std::cerr << "[JIT Compiler Error]: Runtime function '" << name
            << "' used but not declared in setupExternalFunctions!" << std::endl;
        llvm::errs() << "FATAL: Runtime function not found in module: " << name << "\n";
        abort();
    }
    return f;
}

void TzdJitEngine::registerWorkerForSymbol(const std::string& internalName) {
    std::string workerName = internalName + "_worker";
    if (void* ptr = lookupSymbolAsPtr(workerName)) {
        std::string baseName = internalName;
        size_t lastUnderscore = baseName.find_last_of('_');
        if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v') {
            baseName = baseName.substr(0, lastUnderscore);
        }
        std::unique_lock<std::shared_mutex> lock(s_workerPointersMutex);
        s_workerPointers[baseName] = ptr;
    } else if (void* entryPtr = lookupSymbolAsPtr(internalName)) {
        // Fallback: use entry function if worker not found
        std::string baseName = internalName;
        size_t lastUnderscore = baseName.find_last_of('_');
        if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v') {
            baseName = baseName.substr(0, lastUnderscore);
        }
        std::unique_lock<std::shared_mutex> lock(s_workerPointersMutex);
        s_workerPointers[baseName] = entryPtr;
    }
}

void TzdJitEngine::addModule(ThreadSafeModule TSM) {
    if (!m_lljit) {
        errs() << "addModule: LLJIT is not initialized\n";
        return;
    }

    // Pre-compile the IR module to an object buffer manually, then add the
    // object file to the JIT.  This bypasses IRCompileLayer::emit() which
    // destroys the Module after compilation — that destruction overflows the
    // stack due to a CRT ABI mismatch (/MT in TzdTools vs /MD in LLVM SDK).
    std::unique_ptr<MemoryBuffer> objBuffer;

    {
        // Lock the ThreadSafeContext to safely access the Module
        auto lock = TSM.getContext().getLock();
        Module* M = TSM.getModuleUnlocked();
        if (!M) return;

        // Run LLVM optimization passes on the module before compilation.
        // Analysis managers are leaked (static) — their destructors overflow
        // the stack when freeing analysis results referencing large IR graphs.
        {
            static llvm::ModuleAnalysisManager* MAM = nullptr;
            static llvm::FunctionAnalysisManager* FAM = nullptr;
            static llvm::CGSCCAnalysisManager* CGAM = nullptr;
            static llvm::LoopAnalysisManager* LAM = nullptr;
            static bool s_init = false;
            if (!s_init) {
                s_init = true;
                MAM = new llvm::ModuleAnalysisManager();
                FAM = new llvm::FunctionAnalysisManager();
                CGAM = new llvm::CGSCCAnalysisManager();
                LAM = new llvm::LoopAnalysisManager();
                llvm::PassBuilder PB;
                PB.registerModuleAnalyses(*MAM);
                PB.registerFunctionAnalyses(*FAM);
                PB.registerCGSCCAnalyses(*CGAM);
                PB.registerLoopAnalyses(*LAM);
                PB.crossRegisterProxies(*LAM, *FAM, *CGAM, *MAM);
            }
            // Phase C: AlwaysInliner (module pass) + function passes
            llvm::ModulePassManager MPM;
            MPM.addPass(llvm::AlwaysInlinerPass());

            llvm::FunctionPassManager FPM;
            FPM.addPass(llvm::PromotePass());         // mem2reg
            FPM.addPass(llvm::EarlyCSEPass(true));    // common subexpression elimination
            FPM.addPass(llvm::DCEPass());             // dead code elimination
            MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
            MPM.run(*M, *MAM);
            MPM.run(*M, *MAM);
        }

        // Use the JIT's own compiler to compile IR to object code
        auto& compiler = m_lljit->getIRCompileLayer().getCompiler();
        auto result = compiler(*M);

        if (!result) {
            errs() << "Pre-compile failed: " << toString(result.takeError()) << "\n";
            return;
        }

        objBuffer = std::move(*result);
    }
    // Lock released here

    // Add the pre-compiled object file to the JIT
    if (auto Err = m_lljit->addObjectFile(std::move(objBuffer))) {
        errs() << "addObjectFile failed: " << toString(std::move(Err)) << "\n";
        consumeError(std::move(Err));
    }

    // Leak the ThreadSafeModule (and the IR Module + LLVMContext inside it)
    // to prevent Module destruction.  Process exit reclaims all memory.
    static std::vector<ThreadSafeModule>* s_leakedModules = nullptr;
    if (!s_leakedModules) s_leakedModules = new std::vector<ThreadSafeModule>();
    s_leakedModules->push_back(std::move(TSM));
}

LLVMContext& TzdJitEngine::getContext() {
    return *m_tsc.getContext();
}

ThreadSafeContext& TzdJitEngine::getThreadSafeContext() {
    return m_tsc;
}

const DataLayout& TzdJitEngine::getDataLayout() const {
    return m_lljit->getDataLayout();
}

std::string TzdJitEngine::getTargetTriple() const {
    return m_lljit->getTargetTriple().str();
}


void TzdJitEngine::registerRuntimeSymbols() {
    if (!m_lljit) {
        std::cerr << "Critical Error: Cannot register symbols on a null LLJIT instance!" << std::endl;
        return;
    }
    auto& ES = m_lljit->getExecutionSession();
    auto& DL = m_lljit->getDataLayout();
    MangleAndInterner Mangle(ES, DL);
    SymbolMap symbols;

    auto bind = [&](const std::string& Name, void* Addr) {
        ExecutorSymbolDef symbol(ExecutorAddr::fromPtr(Addr), JITSymbolFlags::Exported);
        symbols.insert({ Mangle(Name), symbol });
        };

    bind("rt_create_num", (void*)&rt_create_num);
    bind("rt_create_str", (void*)&rt_create_str);
    bind("rt_create_bool", (void*)&rt_create_bool);
    bind("rt_create_null", (void*)&rt_create_null);
    bind("rt_op_add", (void*)&rt_op_add);
    bind("rt_to_bool", (void*)&rt_to_bool);
    bind("rt_create_inst", (void*)&rt_create_inst);
    bind("rt_create_inst_args", (void*)&rt_create_inst_args);
    bind("rt_get_member", (void*)&rt_get_member);
    bind("rt_tzd_get_member", (void*)&rt_tzd_get_member);
    bind("rt_print", (void*)&rt_print);
    bind("rt_create_native_val", (void*)&rt_create_native_val);
    bind("g_last_ret", (void*)&g_LastJitValue);
    bind("rt_op_sub", (void*)&rt_op_sub);
    bind("rt_op_mul", (void*)&rt_op_mul);
    bind("rt_op_div", (void*)&rt_op_div);
    bind("rt_op_mod", (void*)&rt_op_mod);
    bind("rt_op_pow", (void*)&rt_op_pow);
    bind("rt_op_neg", (void*)&rt_op_neg);
    bind("rt_op_not", (void*)&rt_op_not);
    bind("rt_op_sqrt", (void*)&rt_op_sqrt);
    bind("rt_op_gt", (void*)&rt_op_gt);
    bind("rt_op_lt", (void*)&rt_op_lt);
    bind("rt_op_ge", (void*)&rt_op_ge);
    bind("rt_op_le", (void*)&rt_op_le);
    bind("rt_op_eq", (void*)&rt_op_eq);
    bind("rt_op_ne", (void*)&rt_op_ne);
    bind("rt_array_push", (void*)&rt_array_push);
    bind("rt_get_index", (void*)&rt_get_index);
    bind("rt_create_array", (void*)&rt_create_array);
    bind("rt_store_var", (void*)&rt_store_var);
    bind("rt_resolve_var", (void*)&rt_resolve_var);
    bind("rt_get_arg", (void*)&rt_get_arg);
    bind("rt_set_last_ret", (void*)&rt_set_last_ret);
    bind("rt_call_sub", (void*)&rt_call_sub);
    bind("rt_store_member", (void*)&rt_store_member);
    bind("rt_tzd_store_member", (void*)&rt_tzd_store_member);
    bind("rt_tzd_call_method", (void*)&rt_tzd_call_method);
    bind("rt_store_field_ptr", (void*)&rt_store_field_ptr);
    bind("rt_cast", (void*)&rt_cast);
    bind("rt_type_check", (void*)&rt_type_check);
    bind("rt_call_super", (void*)&rt_call_super);
    bind("rt_get_var_ptr", (void*)&rt_get_var_ptr);
    bind("rt_print_newline", (void*)&rt_print_newline);
    bind("rt_to_double_fast", (void*)&rt_to_double_fast);
    bind("rt_stabilize_value", (void*)&rt_stabilize_value);
    bind("rt_store_index", (void*)&rt_store_index);
    bind("rt_copy_value", (void*)&rt_copy_value);
    bind("rt_call_sub_f1", (void*)&rt_call_sub_f1);
    bind("rt_call_sub_fast", (void*)&rt_call_sub_fast);
    bind("rt_call_value_fast", (void*)&rt_call_value_fast);
    bind("rt_store_native_to_ptr", (void*)&rt_store_native_to_ptr);
    bind("rt_set_null", (void*)&rt_set_null);
    bind("rt_construct_num_at", (void*)&rt_construct_num_at);
    bind("rt_construct_copy_at", (void*)&rt_construct_copy_at);
    bind("rt_destruct_values", (void*)&rt_destruct_values);
    bind("rt_get_index_native_d", (void*)&rt_get_index_native_d);
    bind("rt_store_index_native_d", (void*)&rt_store_index_native_d);
    bind("rt_get_index_native_d_dyn", (void*)&rt_get_index_native_d_dyn);
    bind("rt_store_index_native_d_dyn", (void*)&rt_store_index_native_d_dyn);
    bind("rt_create_native_double_arr", (void*)&rt_create_native_double_arr);
    bind("rt_push_arg_frame", (void*)&rt_push_arg_frame);
    bind("rt_pop_arg_frame", (void*)&rt_pop_arg_frame);
    bind("rt_init_tzd_value", (void*)&rt_init_tzd_value);
    bind("rt_write_fast_ret", (void*)&rt_write_fast_ret);
    bind("rt_get_worker_ptr", (void*)&rt_get_worker_ptr);

    // ======= 修复 try/catch 机制的新绑定 =======
    bind("rt_alloc_jmp_buf", (void*)&rt_alloc_jmp_buf);
    bind("rt_free_jmp_buf", (void*)&rt_free_jmp_buf);
    bind("rt_get_catch_jmp", (void*)&rt_get_catch_jmp);
    bind("rt_set_catch_jmp", (void*)&rt_set_catch_jmp);

#ifdef _WIN32
    void* sjAddr = nullptr;
    HMODULE hUcrt = GetModuleHandleA("ucrtbase.dll");
    if (hUcrt) sjAddr = (void*)GetProcAddress(hUcrt, "_setjmp");
    if (!sjAddr) {
        HMODULE hMsvc = GetModuleHandleA("msvcrt.dll");
        if (hMsvc) sjAddr = (void*)GetProcAddress(hMsvc, "_setjmp");
    }
    if (!sjAddr) {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) sjAddr = (void*)GetProcAddress(hNtdll, "_setjmp");
    }
    if (sjAddr) {
        bind("_setjmp", sjAddr);
    }
    else {
        std::cerr << "FATAL: Could not resolve _setjmp from Windows DLLs!" << std::endl;
    }
#else
    bind("setjmp", (void*)&_setjmp);
#endif
    // ==========================================

    bind("rt_throw", (void*)&rt_throw);
    bind("rt_get_thrown", (void*)&rt_get_thrown);
    bind("rt_set_location", (void*)&rt_set_location);
    bind("rt_push_jit_frame", (void*)&rt_push_jit_frame);
    bind("rt_pop_jit_frame", (void*)&rt_pop_jit_frame);
    bind("rt_replace_jit_frame", (void*)&rt_replace_jit_frame);
    bind("rt_create_lambda_value", (void*)&rt_create_lambda_value);

    cantFail(m_lljit->getMainJITDylib().define(absoluteSymbols(symbols)));
}

TzdCompiler::TzdCompiler(TzdJitEngine& jit, const std::string& modName)
    : m_jitEngine(jit),
    m_tsc(std::make_unique<llvm::LLVMContext>()),
    m_context(*m_tsc.getContext()),
    m_builder(m_context)
{
    s_currentFuncParamNames.clear();
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;
    s_currentWorkerFunc = nullptr;
    s_tryJmpBufStack.clear();
    s_loopLevel = 0;

    m_module = std::make_unique<llvm::Module>(modName, m_context);
    m_module->setDataLayout(jit.getDataLayout());
    m_module->setTargetTriple(jit.getTargetTriple());

    // --- 基础类型初始化 ---
    m_voidTy = llvm::Type::getVoidTy(m_context);
    m_doubleTy = llvm::Type::getDoubleTy(m_context);
    m_boolTy = llvm::Type::getInt1Ty(m_context);
    m_int32Ty = llvm::Type::getInt32Ty(m_context);

    m_ptrTy = llvm::PointerType::getUnqual(m_context);

    m_tzdValueTy = llvm::StructType::create(m_context, "struct.TzdValue");
    m_tzdValueTy->setBody({ llvm::ArrayType::get(llvm::Type::getInt8Ty(m_context), sizeof(TzdValue)) });

    if (!m_ptrTy || !m_voidTy || !m_tzdValueTy || !m_int32Ty) {
        std::cerr << "FATAL: LLVM Type initialization failed!" << std::endl;
        abort();
    }
}

TzdCompiler::~TzdCompiler() {
    m_builder.ClearInsertionPoint();

    s_currentFuncParamNames.clear();
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;
    s_currentWorkerFunc = nullptr;
    s_tryJmpBufStack.clear();
    s_loopLevel = 0;

    if (m_module) {
        // Leak the Module to avoid heap corruption from LLVM ABI mismatch.
        m_module.release();
    }

    // Leak the ThreadSafeContext (LLVMContext) to avoid stack overflow
    // during LLVMContext destruction.  Move to a heap container that is
    // never destroyed — process exit reclaims all memory.
    static std::vector<llvm::orc::ThreadSafeContext>* s_leaked = nullptr;
    if (!s_leaked) s_leaked = new std::vector<llvm::orc::ThreadSafeContext>();
    s_leaked->push_back(std::move(m_tsc));
}


llvm::orc::ThreadSafeModule TzdCompiler::extractThreadSafeModule() {
    if (!m_module) {
        return llvm::orc::ThreadSafeModule(nullptr, m_tsc);
    }

    // 1. Clean up dead code blocks
    if (llvm::verifyModule(*m_module, &llvm::errs())) {
        llvm::errs() << "[JIT] Module verification failed!\n";
        return llvm::orc::ThreadSafeModule(nullptr, m_tsc);
    }

    // Move ownership of the module to the ThreadSafeModule.
    // The JIT engine will own the Module and properly destroy it when done.
    // m_module becomes null — the destructor's reset() is a safe no-op.
    auto moduleForJIT = std::move(m_module);

    m_builder.ClearInsertionPoint();
    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_currentRetPtr = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;

    return llvm::orc::ThreadSafeModule(std::move(moduleForJIT), m_tsc);
}

Value* TzdCompiler::boxToTzdValue(Value* val) {
    if (!val) return m_builder.CreateCall(getRtFunc("rt_create_null"));
    if (val->getType()->isPointerTy()) return val;

    if (val->getType()->isDoubleTy()) {
        return m_builder.CreateCall(getRtFunc("rt_create_num"), { val });
    }
    if (val->getType()->isIntegerTy(1)) { // i1 (bool)
        return m_builder.CreateCall(getRtFunc("rt_create_bool"), { val });
    }
    return val;
}


Value* TzdCompiler::toNativeBool(Value* val) {
    if (val->getType()->isIntegerTy(1)) {
        return val;
    }
    if (val->getType()->isDoubleTy()) {
        return m_builder.CreateFCmpONE(val, ConstantFP::get(m_doubleTy, 0.0), "tobool");
    }
    if (val->getType()->isPointerTy()) {
        return m_builder.CreateCall(getRtFunc("rt_to_bool"), { val });
    }
    return m_builder.getTrue(); // 默认 true
}

std::unique_ptr<llvm::Module> TzdCompiler::getModule() {
    if (llvm::verifyModule(*m_module, &llvm::errs())) {
        llvm::errs() << "\n>>> [JIT FATAL ERROR]: Module verification failed! \n";
        llvm::errs() << "========== [ DUMPING GENERATED LLVM IR CODE ] ==========\n";
        m_module->print(llvm::errs(), nullptr);
        llvm::errs() << "========================================================\n\n";
        return nullptr;
    }
    return std::move(m_module);
}

// --- 基础结构 ---

std::any TzdCompiler::visitProgram(TzdLangParser::ProgramContext* ctx) {
    for (auto stmt : ctx->statement()) visit(stmt);
    return std::any();
}


std::any TzdCompiler::visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    compileNamedFunction(ctx, name);
    return std::any((Value*)m_module->getFunction(name)); // 返回对外暴露的入口
}

std::any TzdCompiler::visitBlock(TzdLangParser::BlockContext* ctx) {
    for (auto stmt : ctx->statement()) visit(stmt);
    return std::any();
}


std::any TzdCompiler::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) {
    std::string name = ctx->variableDeclaration()->IDENTIFIER()->getText();
    m_declaredLocals.insert(name);
    Value* initVal = nullptr;
    if (ctx->variableDeclaration()->expression()) {
        initVal = std::any_cast<Value*>(visit(ctx->variableDeclaration()->expression()));
    }
    else {
        initVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
    }
    if (initVal->getType()->isDoubleTy()) {
        AllocaInst* alloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, name + "_native");
        m_builder.CreateStore(initVal, alloc);
        m_nativeDoubleLocals[name] = alloc;
        // Also store to scope so nested functions can access via rt_get_var_ptr
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        Value* boxedVal = boxToTzdValue(initVal);
        m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, boxedVal });
        return std::any();
    }
    Value* boxedVal = boxToTzdValue(initVal);
    AllocaInst* alloca = CreateEntryBlockAlloca(name);
    m_builder.CreateStore(boxedVal, alloca);
    m_namedValues[name] = alloca;
    // Also store to scope so nested functions can access via rt_get_var_ptr
    {
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, boxedVal });
    }
    return std::any();
}

std::any TzdCompiler::visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) {
    auto* savedInsertBlock = m_builder.GetInsertBlock();
    auto savedNamedValues = m_namedValues;
    auto savedNativeLocals = m_nativeDoubleLocals;
    auto savedParamNames = s_currentFuncParamNames;
    auto* savedTailBB = s_tailRecurseBB;
    auto* savedWorkerFunc = s_currentWorkerFunc;
    auto* savedRetPtr = m_currentRetPtr;

    static int s_lambdaCount = 0;
    std::string lambdaInternalName = "lambda_fun_" + std::to_string(s_lambdaCount++);

    compileNamedFunction(ctx->block(), ctx->paramList(), lambdaInternalName);

    if (g_CurrentInterpreter) {
        g_CurrentInterpreter->m_pendingJitFunctions.insert(lambdaInternalName);
    }

    m_builder.SetInsertPoint(savedInsertBlock);
    m_namedValues = savedNamedValues;
    m_nativeDoubleLocals = savedNativeLocals;
    s_currentFuncParamNames = savedParamNames;
    s_tailRecurseBB = savedTailBB;
    s_currentWorkerFunc = savedWorkerFunc;
    m_currentRetPtr = savedRetPtr;

    Value* nameStr = m_builder.CreateGlobalStringPtr(lambdaInternalName);
    Value* lambdaVal = m_builder.CreateCall(getRtFunc("rt_create_lambda_value"), { nameStr });

    return std::any(lambdaVal);
}

std::any TzdCompiler::visitIfStmt(TzdLangParser::IfStmtContext* ctx) {
    Value* condVal = std::any_cast<Value*>(visit(ctx->expression()));

    Value* isTrue = toNativeBool(condVal);

    Function* func = m_builder.GetInsertBlock()->getParent();
    BasicBlock* thenBB = BasicBlock::Create(m_context, "then", func);
    BasicBlock* mergeBB = BasicBlock::Create(m_context, "ifcont");
    BasicBlock* elseBB = ctx->KW_ELSE() ? BasicBlock::Create(m_context, "else") : nullptr;

    m_builder.CreateCondBr(isTrue, thenBB, ctx->KW_ELSE() ? elseBB : mergeBB);

    m_builder.SetInsertPoint(thenBB);
    visit(ctx->statement(0));
    if (!m_builder.GetInsertBlock()->getTerminator()) m_builder.CreateBr(mergeBB);

    if (ctx->KW_ELSE()) {
        func->insert(func->end(), elseBB);
        m_builder.SetInsertPoint(elseBB);
        visit(ctx->statement(1));
        if (!m_builder.GetInsertBlock()->getTerminator()) m_builder.CreateBr(mergeBB);
    }

    func->insert(func->end(), mergeBB);
    m_builder.SetInsertPoint(mergeBB);
    return std::any();
}

std::any TzdCompiler::visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) {
    Function* func = m_builder.GetInsertBlock()->getParent();
    BasicBlock* condBB = BasicBlock::Create(m_context, "loop_cond", func);
    BasicBlock* bodyBB = BasicBlock::Create(m_context, "loop_body", func);
    BasicBlock* endBB = BasicBlock::Create(m_context, "loop_end", func);

    m_loopStack.push_back({ condBB, endBB });

    m_builder.CreateBr(condBB);
    m_builder.SetInsertPoint(condBB);
    Value* condVal = std::any_cast<Value*>(visit(ctx->expression()));
    Value* isTrue = toNativeBool(condVal);
    m_builder.CreateCondBr(isTrue, bodyBB, endBB);

    m_builder.SetInsertPoint(bodyBB);
    s_loopLevel++; // <--- 循环级别+1
    visit(ctx->statement());
    s_loopLevel--; // <--- 循环级别-1
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(condBB);
    }

    m_builder.SetInsertPoint(endBB);
    m_loopStack.pop_back();
    return std::any();
}

std::any TzdCompiler::visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) {
    (void)ctx;
    // ======= 清理跳出的循环内部所在的 try 块 =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it) {
        if (std::get<2>(*it) >= s_loopLevel) {
            m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { std::get<1>(*it) });
            m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { std::get<0>(*it) });
        }
        else { break; }
    }
    // ===========================================

    if (!m_switchEndStack.empty()) m_builder.CreateBr(m_switchEndStack.back());
    else if (!m_loopStack.empty()) m_builder.CreateBr(m_loopStack.back().breakBB);
    return std::any();
}

std::any TzdCompiler::visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) {
    (void)ctx;
    // ======= 清理同上 =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it) {
        if (std::get<2>(*it) >= s_loopLevel) {
            m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { std::get<1>(*it) });
            m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { std::get<0>(*it) });
        }
        else { break; }
    }
    // =======================

    if (m_loopStack.empty()) return std::any();
    m_builder.CreateBr(m_loopStack.back().continueBB);
    return std::any();
}

std::any TzdCompiler::visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) {
    Value* switchVal = boxToTzdValue(std::any_cast<Value*>(visit(ctx->expression())));
    Function* func = m_builder.GetInsertBlock()->getParent();
    BasicBlock* endBB = BasicBlock::Create(m_context, "switch.end", func);

    AllocaInst* matchedAlloca = CreateEntryBlockAlloca(m_boolTy, nullptr, "switch.matched");
    m_builder.CreateStore(m_builder.getFalse(), matchedAlloca);

    BasicBlock* prevAfterBB = nullptr;
    int caseIdx = 0;
    for (auto* caseCtx : ctx->switchCase()) {
        BasicBlock* checkBB = BasicBlock::Create(m_context, "switch.chk" + std::to_string(caseIdx), func);
        BasicBlock* bodyBB = BasicBlock::Create(m_context, "switch.body" + std::to_string(caseIdx), func);
        BasicBlock* afterBB = BasicBlock::Create(m_context, "switch.after" + std::to_string(caseIdx), func);
        caseIdx++;

        if (prevAfterBB == nullptr) {
            m_builder.CreateBr(checkBB);
        }
        else {
            m_builder.SetInsertPoint(prevAfterBB);
            if (!m_builder.GetInsertBlock()->getTerminator()) {
                m_builder.CreateBr(checkBB);
            }
        }

        m_builder.SetInsertPoint(checkBB);
        Value* alreadyMatched = m_builder.CreateLoad(m_boolTy, matchedAlloca);
        BasicBlock* eqCheckBB = BasicBlock::Create(m_context, "switch.eq" + std::to_string(caseIdx), func);
        m_builder.CreateCondBr(alreadyMatched, bodyBB, eqCheckBB);

        m_builder.SetInsertPoint(eqCheckBB);
        Value* caseVal = boxToTzdValue(std::any_cast<Value*>(visit(caseCtx->expression())));
        Value* eq = m_builder.CreateCall(getRtFunc("rt_op_eq"), { switchVal, caseVal });
        Value* eqBool = toNativeBool(eq);
        m_builder.CreateCondBr(eqBool, bodyBB, afterBB);

        m_builder.SetInsertPoint(bodyBB);
        m_builder.CreateStore(m_builder.getTrue(), matchedAlloca);
        for (auto* stmt : caseCtx->statement()) {
            visit(stmt);
            if (m_builder.GetInsertBlock()->getTerminator()) break;
        }
        if (!m_builder.GetInsertBlock()->getTerminator()) {
            m_builder.CreateBr(afterBB);
        }

        prevAfterBB = afterBB;
    }

    if (prevAfterBB) {
        m_builder.SetInsertPoint(prevAfterBB);
    }

    if (ctx->switchDefault()) {
        BasicBlock* defaultCheckBB = BasicBlock::Create(m_context, "switch.default.chk", func);
        BasicBlock* defaultSetBB = BasicBlock::Create(m_context, "switch.default.set", func);
        BasicBlock* defaultBodyBB = BasicBlock::Create(m_context, "switch.default.body", func);
        BasicBlock* defaultAfterBB = BasicBlock::Create(m_context, "switch.default.after", func);

        m_builder.CreateBr(defaultCheckBB);
        m_builder.SetInsertPoint(defaultCheckBB);
        Value* alreadyMatched = m_builder.CreateLoad(m_boolTy, matchedAlloca);
        m_builder.CreateCondBr(alreadyMatched, defaultBodyBB, defaultSetBB);

        m_builder.SetInsertPoint(defaultSetBB);
        m_builder.CreateStore(m_builder.getTrue(), matchedAlloca);
        m_builder.CreateBr(defaultBodyBB);

        m_builder.SetInsertPoint(defaultBodyBB);
        for (auto* stmt : ctx->switchDefault()->statement()) {
            visit(stmt);
            if (m_builder.GetInsertBlock()->getTerminator()) break;
        }
        if (!m_builder.GetInsertBlock()->getTerminator()) {
            m_builder.CreateBr(defaultAfterBB);
        }
        m_builder.SetInsertPoint(defaultAfterBB);
    }

    m_switchEndStack.push_back(endBB);

    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(endBB);
    }
    m_builder.SetInsertPoint(endBB);
    m_switchEndStack.pop_back();
    return std::any();
}

std::any TzdCompiler::visitForStmt(TzdLangParser::ForStmtContext* ctx) {
    auto backupScope = m_namedValues;
    auto backupNative = m_nativeDoubleLocals;

    bool initHandled = false;

    // --- [核心修复] 循环变量自动提升 ---
    if (ctx->forInit()) {
        if (auto expr = ctx->forInit()->expression()) {
            if (auto assignCtx = dynamic_cast<TzdLangParser::AssignmentExprContext*>(expr)) {
                std::string varName = assignCtx->expression(0)->getText();
                auto valExpr = assignCtx->expression(1);

                Value* initVal = std::any_cast<Value*>(visit(valExpr));

                if (initVal->getType()->isDoubleTy() &&
                    m_nativeDoubleLocals.find(varName) == m_nativeDoubleLocals.end() &&
                    m_namedValues.find(varName) == m_namedValues.end()) {

                    AllocaInst* alloc = m_builder.CreateAlloca(m_doubleTy, nullptr, varName + "_loop_native");
                    m_builder.CreateStore(initVal, alloc);

                    m_nativeDoubleLocals[varName] = alloc;
                    initHandled = true;
                }
            }
        }
    }

    if (!initHandled && ctx->forInit()) {
        visit(ctx->forInit());
    }

    Function* currentFunc = m_builder.GetInsertBlock()->getParent();
    BasicBlock* condBB = BasicBlock::Create(m_context, "for.cond", currentFunc);
    BasicBlock* bodyBB = BasicBlock::Create(m_context, "for.body", currentFunc);
    BasicBlock* stepBB = BasicBlock::Create(m_context, "for.step", currentFunc);
    BasicBlock* afterBB = BasicBlock::Create(m_context, "for.after", currentFunc);

    m_builder.CreateBr(condBB);
    m_builder.SetInsertPoint(condBB);

    if (ctx->cond) {
        Value* condValue = std::any_cast<Value*>(visit(ctx->cond));
        Value* isTrue = toNativeBool(condValue);
        m_builder.CreateCondBr(isTrue, bodyBB, afterBB);
    }
    else {
        m_builder.CreateBr(bodyBB);
    }

    m_loopStack.push_back({ stepBB, afterBB });

    m_builder.SetInsertPoint(bodyBB);

    // ==========================================
    s_loopLevel++; // 进入循环控制域
    if (ctx->statement()) {
        visit(ctx->statement());
    }
    s_loopLevel--; // 退出循环控制域
    // ==========================================

    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(stepBB);
    }

    m_builder.SetInsertPoint(stepBB);
    if (ctx->step) {
        visit(ctx->step);
    }
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(condBB);
    }

    m_builder.SetInsertPoint(afterBB);
    m_loopStack.pop_back();

    m_namedValues = backupScope;
    m_nativeDoubleLocals = backupNative;
    return std::any();
}

// --- 表达式 ---

std::any TzdCompiler::visitIdExpr(TzdLangParser::IdExprContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    if (m_nativeDoubleLocals.count(name)) {
        Value* slot = m_nativeDoubleLocals[name];
        // Direct double arg (native worker) — already a double, return as-is
        if (slot->getType()->isDoubleTy()) {
            return std::any((Value*)slot);
        }
        Value* val = m_builder.CreateLoad(m_doubleTy, slot, name);
        return std::any((Value*)val);
    }
    if (m_namedValues.count(name)) {
        Value* val = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        return std::any((Value*)val);
    }
    Value* nameStr = m_builder.CreateGlobalStringPtr(name);
    Value* addr = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), { nameStr });
    return std::any((Value*)addr);
}

std::any TzdCompiler::visitIntExpr(TzdLangParser::IntExprContext* ctx) {
    double val = std::stod(ctx->getText());
    return std::any((Value*)ConstantFP::get(m_doubleTy, val));
}

std::any TzdCompiler::visitFloatExpr(TzdLangParser::FloatExprContext* ctx) {
    double val = std::stod(ctx->getText());
    return std::any((Value*)ConstantFP::get(m_doubleTy, val));
}

std::any TzdCompiler::visitStringExpr(TzdLangParser::StringExprContext* ctx) {
    std::string s = ctx->getText(); s = s.substr(1, s.size() - 2);
    Value* str = m_builder.CreateGlobalStringPtr(unescapeString(s));
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_str"), { str });
}

std::any TzdCompiler::visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));
    if (!ctx->PLUS()) {
        Value* lNative = L->getType()->isDoubleTy() ? L : inlineToDoubleFast(L);
        Value* rNative = R->getType()->isDoubleTy() ? R : inlineToDoubleFast(R);
        Value* res = m_builder.CreateFSub(lNative, rNative, "subtmp");
        return std::any((Value*)res);
    }
    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        Value* res = m_builder.CreateFAdd(L, R, "addtmp");
        return std::any((Value*)res);
    }
    if (L->getType()->isDoubleTy() && R->getType()->isPointerTy()) {
        Value* rNative = inlineToDoubleFast(R);
        Value* res = m_builder.CreateFAdd(L, rNative, "addtmp");
        return std::any((Value*)res);
    }
    if (R->getType()->isDoubleTy() && L->getType()->isPointerTy()) {
        Value* lNative = inlineToDoubleFast(L);
        Value* res = m_builder.CreateFAdd(lNative, R, "addtmp");
        return std::any((Value*)res);
    }
    Value* boxedL = boxToTzdValue(L);
    Value* boxedR = boxToTzdValue(R);
    Value* res = m_builder.CreateCall(getRtFunc("rt_op_add"), { boxedL, boxedR });
    return std::any((Value*)res);
}
std::any TzdCompiler::visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) {

    // ---- [新增] 复合赋值展开：+= -= *= /= ----
    // 检查是否有复合赋值运算符，展开为 lhs = lhs OP rhs
    std::string opText = "=";
    if (ctx->PLUS_ASSIGN()) opText = "+=";
    else if (ctx->MIN_ASSIGN())  opText = "-=";
    else if (ctx->MUL_ASSIGN())  opText = "*=";
    else if (ctx->DIV_ASSIGN())  opText = "/=";
    bool isCompound = (opText == "+=" || opText == "-=" || opText == "*=" || opText == "/=");

    Value* rhs = castAnyToValue(visit(ctx->expression(1)), "visitAssignmentExpr.rhs");

    // ---- 数组下标复合赋值：awa[0] += 1.0 ----
    if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(ctx->expression(0))) {
        Value* container = std::any_cast<Value*>(visit(indexCtx->expression(0)));
        Value* index_raw = std::any_cast<Value*>(visit(indexCtx->expression(1)));

        // 判断索引类型，选择读取函数
        Value* oldVal = nullptr;
        bool constIdx = false;
        int   constIdxVal = 0;
        bool  nativeIdx = false;

        if (auto* constFP = llvm::dyn_cast<llvm::ConstantFP>(index_raw)) {
            constIdx = true;
            constIdxVal = (int)constFP->getValueAPF().convertToDouble();
            oldVal = m_builder.CreateCall(getRtFunc("rt_get_index_native_d"),
                { container, m_builder.getInt32(constIdxVal) });
        }
        else if (index_raw->getType()->isDoubleTy()) {
            nativeIdx = true;
            oldVal = m_builder.CreateCall(getRtFunc("rt_get_index_native_d_dyn"),
                { container, index_raw });
        }
        else {
            // boxed 索引，走旧路径
            Value* boxedIndex = boxToTzdValue(index_raw);
            Value* boxedRhs = boxToTzdValue(rhs);
            if (isCompound) {
                Value* oldBoxed = m_builder.CreateCall(getRtFunc("rt_get_index"), { container, boxedIndex });
                const char* opFn = opText == "+=" ? "rt_op_add" :
                    opText == "-=" ? "rt_op_sub" :
                    opText == "*=" ? "rt_op_mul" : "rt_op_div";
                boxedRhs = m_builder.CreateCall(getRtFunc(opFn), { oldBoxed, boxedRhs });
            }
            m_builder.CreateCall(getRtFunc("rt_store_index"), { container, boxedIndex, boxedRhs });
            return boxedRhs;
        }

        // If rhs is a pointer (object/string), use boxed store path
        if (rhs->getType()->isPointerTy()) {
            Value* boxedIndex = boxToTzdValue(index_raw);
            Value* boxedRhs = boxToTzdValue(rhs);
            if (isCompound) {
                Value* oldBoxed = m_builder.CreateCall(getRtFunc("rt_get_index"), { container, boxedIndex });
                const char* opFn = opText == "+=" ? "rt_op_add" :
                    opText == "-=" ? "rt_op_sub" :
                    opText == "*=" ? "rt_op_mul" : "rt_op_div";
                boxedRhs = m_builder.CreateCall(getRtFunc(opFn), { oldBoxed, boxedRhs });
            }
            m_builder.CreateCall(getRtFunc("rt_store_index"), { container, boxedIndex, boxedRhs });
            return boxedRhs;
        }

        // native double 路径：计算新值
        Value* nativeRhs = castToNativeDouble(rhs);
        Value* newVal = nativeRhs;
        if (isCompound) {
            if (opText == "+=")      newVal = m_builder.CreateFAdd(oldVal, nativeRhs);
            else if (opText == "-=") newVal = m_builder.CreateFSub(oldVal, nativeRhs);
            else if (opText == "*=") newVal = m_builder.CreateFMul(oldVal, nativeRhs);
            else if (opText == "/=") newVal = m_builder.CreateFDiv(oldVal, nativeRhs);
        }

        // 写回
        if (constIdx) {
            m_builder.CreateCall(getRtFunc("rt_store_index_native_d"),
                { container, m_builder.getInt32(constIdxVal), newVal });
        }
        else {
            m_builder.CreateCall(getRtFunc("rt_store_index_native_d_dyn"),
                { container, index_raw, newVal });
        }
        return newVal;
    }

    // ---- 成员赋值 ----
    TzdLangParser::MemberAccessExprContext* memberCtx =
        dynamic_cast<TzdLangParser::MemberAccessExprContext*>(ctx->expression(0));
    if (!memberCtx) {
        if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(ctx->expression(0))) {
            memberCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomExpr->atom());
        }
    }
    if (memberCtx) {
        Value* obj = castAnyToValue(visit(memberCtx->atom()), "visitAssignmentExpr.memberObj");
        std::string memberName = memberCtx->IDENTIFIER()->getText();
        Value* nameStr = m_builder.CreateGlobalStringPtr(memberName);
        TzdSelector sel = internSelectorConstant(memberName);
        Value* boxedRhs = boxToTzdValue(rhs);
        // F2: Split store into get_field_ptr (readonly, CSE-able) + store_field_ptr
        Value* fieldPtr = m_builder.CreateCall(getRtFunc("rt_tzd_get_member"),
            { obj, m_builder.getInt32((int32_t)sel), nameStr });
        if (isCompound) {
            const char* opFn = opText == "+=" ? "rt_op_add" :
                opText == "-=" ? "rt_op_sub" :
                opText == "*=" ? "rt_op_mul" : "rt_op_div";
            boxedRhs = m_builder.CreateCall(getRtFunc(opFn), { fieldPtr, boxedRhs });
        }
        m_builder.CreateCall(getRtFunc("rt_store_field_ptr"), { fieldPtr, boxedRhs });
        return boxedRhs;
    }

    std::string name = ctx->expression(0)->getText();

    // ---- 已知 native double 局部变量 ----
    if (m_nativeDoubleLocals.count(name)) {
        Value* nativeRhs = castToNativeDouble(rhs);
        Value* newVal = nativeRhs;
        if (isCompound) {
            Value* oldVal = m_builder.CreateLoad(m_doubleTy, m_nativeDoubleLocals[name]);
            if (opText == "+=")      newVal = m_builder.CreateFAdd(oldVal, nativeRhs);
            else if (opText == "-=") newVal = m_builder.CreateFSub(oldVal, nativeRhs);
            else if (opText == "*=") newVal = m_builder.CreateFMul(oldVal, nativeRhs);
            else if (opText == "/=") newVal = m_builder.CreateFDiv(oldVal, nativeRhs);
        }
        m_builder.CreateStore(newVal, m_nativeDoubleLocals[name]);
        if (!m_declaredLocals.count(name)) {
            // Persist to scope chain so global variables are visible outside
            Value* nameStr = m_builder.CreateGlobalStringPtr(name);
            Value* boxedVal = boxToTzdValue(newVal);
            m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, boxedVal });
        }
        return newVal;
    }

    // ---- 懒提升：rhs 是 double，变量完全未知 → native local ----
    if (rhs->getType()->isDoubleTy() &&
        !m_namedValues.count(name) &&
        !m_namedValues.count("this"))
    {
        AllocaInst* alloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, name + "_native");
        m_builder.CreateStore(rhs, alloc);
        m_nativeDoubleLocals[name] = alloc;
        // Only persist to scope for non-local variables (globals, outer scope vars)
        if (!m_declaredLocals.count(name)) {
            Value* nameStr = m_builder.CreateGlobalStringPtr(name);
            Value* boxedRhs = boxToTzdValue(rhs);
            m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, boxedRhs });
        }
        return rhs;
    }

    // ---- boxed local 变量 ----
    Value* boxedRhs = boxToTzdValue(rhs);
    if (isCompound) {
        const char* opFn = opText == "+=" ? "rt_op_add" :
            opText == "-=" ? "rt_op_sub" :
            opText == "*=" ? "rt_op_mul" : "rt_op_div";
        Value* oldVal = nullptr;
        if (m_namedValues.count(name)) {
            oldVal = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        }
        else {
            Value* nameStr = m_builder.CreateGlobalStringPtr(name);
            oldVal = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), { nameStr });
        }
        boxedRhs = m_builder.CreateCall(getRtFunc(opFn), { oldVal, boxedRhs });
    }

    if (m_namedValues.count(name)) {
        m_builder.CreateStore(boxedRhs, m_namedValues[name]);
    }
    else if (m_namedValues.count("this")) {
        Value* thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        TzdSelector sel = internSelectorConstant(name);
        m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
            { thisPtr, m_builder.getInt32((int32_t)sel), nameStr, boxedRhs });
    }
    else {
        Value* stable = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxedRhs });
        AllocaInst* alloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, name + "_local");
        m_builder.CreateStore(stable, alloc);
        m_namedValues[name] = alloc;
        if (!m_declaredLocals.count(name)) {
            Value* nameStr = m_builder.CreateGlobalStringPtr(name);
            m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, stable });
        }
    }
    return boxedRhs;
}

std::any TzdCompiler::visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) {
    if (ctx->printFunction()->exprList()) {
        for (auto e : ctx->printFunction()->exprList()->expression()) {
            Value* v = std::any_cast<Value*>(visit(e));

            Value* boxedVal = boxToTzdValue(v);

            m_builder.CreateCall(getRtFunc("rt_print"), { boxedVal });
        }
    }
    m_builder.CreateCall(getRtFunc("rt_print_newline"));
    return std::any();
}

std::any TzdCompiler::visitNewExpr(TzdLangParser::NewExprContext* ctx) {
    // F4: Skip rt_set_location for performance — only needed for error reporting
    std::string className = ctx->qualifiedName()->getText();
    Value* name = m_builder.CreateGlobalStringPtr(className);

    auto exprs = ctx->exprList() ? ctx->exprList()->expression() : std::vector<TzdLangParser::ExpressionContext*>();
    int argCount = (int)exprs.size();
    int slotCount = argCount > 0 ? argCount : 1;
    Value* argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(slotCount), "ctor_args");
    m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), { argsArray, m_builder.getInt32(slotCount) });

    for (int i = 0; i < argCount; ++i) {
        Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
        Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
        if (argRaw->getType()->isDoubleTy()) {
            inlineStoreNativeToPtr(argPtr, argRaw);
        }
        else {
            m_builder.CreateCall(getRtFunc("rt_copy_value"), { argPtr, boxToTzdValue(argRaw) });
        }
    }

    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_inst_args"), { name, m_builder.getInt32(argCount), argsArray });
}

// 编译期内联 selector：成员名 -> 常量 TzdSelector（i32）写进 IR，
// 消除运行期字符串查找。m_selectorIds 仅缓存本编译器实例已解析映射，
// 进程级唯一性由 tzdInternSelector 保证。
TzdSelector TzdCompiler::internSelectorConstant(const std::string& name) {
    auto it = m_selectorIds.find(name);
    if (it != m_selectorIds.end()) return it->second;
    TzdSelector sel = tzdInternSelector(name);
    m_selectorIds.emplace(name, sel);
    return sel;
}

std::any TzdCompiler::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) {
    Value* obj = std::any_cast<Value*>(visit(ctx->atom()));
    std::string memberName = ctx->IDENTIFIER()->getText();
    Value* name = m_builder.CreateGlobalStringPtr(memberName);
    TzdSelector sel = internSelectorConstant(memberName);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_tzd_get_member"),
        { obj, m_builder.getInt32((int32_t)sel), name });
}

std::any TzdCompiler::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    Value* retValRaw = nullptr;
    if (ctx->expression()) {
        bool isCall = (getAsCallExpr(ctx->expression()) != nullptr);
        bool oldTailState = s_inTailPosition;
        s_inTailPosition = isCall;

        retValRaw = std::any_cast<Value*>(visit(ctx->expression()));

        s_inTailPosition = oldTailState;
    }

    // If the expression was a tail call, it already generated ret/unreachable
    // in both fast and slow paths.  The current insert block now has a terminator.
    // We must NOT emit any more instructions into it — doing so would place
    // instructions after the terminator, causing "Terminator found in the middle
    // of a basic block!" verification failures.
    // Create a fresh dead block so any subsequent (unreachable) statements have
    // a valid insertion point, and compileNamedFunction's default-return logic
    // can terminate it.
    if (m_builder.GetInsertBlock() && m_builder.GetInsertBlock()->getTerminator()) {
        BasicBlock* deadAfterTail = BasicBlock::Create(
            m_context, "dead_after_tailcall",
            m_builder.GetInsertBlock()->getParent());
        m_builder.SetInsertPoint(deadAfterTail);
        return std::any();
    }

    // ======= 强制展开回收动作 (防止 try 块内 return 导致 jmp_buf 泄露) =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it) {
        m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { std::get<1>(*it) });
        m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { std::get<0>(*it) });
    }
    // =========================================================================

    if (m_currentRetPtr) {
        if (retValRaw && retValRaw->getType()->isDoubleTy()) {
            // 如果是原生数字，直接写值 (rt_store_native_to_ptr has runtime NULL check
            // — m_currentRetPtr can be NULL when called from self-recursion bypass)
            m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { m_currentRetPtr, retValRaw });
        }
        else if (retValRaw) {
            Value* boxed = boxToTzdValue(retValRaw);
            Value* stable = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxed });
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { m_currentRetPtr, stable });
        }
    }

    Function* currentFunc = m_builder.GetInsertBlock()->getParent();
    std::string currentName = currentFunc->getName().str();

    // 绝不让高频自递归污染和更新全局 g_last_ret
    if (retValRaw && currentName.find("_worker") == std::string::npos) {
        Value* boxed = boxToTzdValue(retValRaw);
        m_builder.CreateCall(getRtFunc("rt_set_last_ret"), { boxed });
    }

    // [绝杀修改]：如果是 Worker 函数，直接用底层寄存器返回原生数字
    if (currentFunc->getReturnType()->isDoubleTy()) {
        Value* dRet = castToNativeDouble(retValRaw);
        m_builder.CreateRet(dRet);
    }
    else {
        m_builder.CreateRetVoid();
    }

    BasicBlock* deadBB = BasicBlock::Create(m_context, "unreachable", currentFunc);
    m_builder.SetInsertPoint(deadBB);

    return std::any((Value*)nullptr);
}

std::any TzdCompiler::visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext* ctx) {
    auto decl = ctx->nativeFunctionDeclaration();
    std::string funcName = decl->IDENTIFIER()->getText();

    std::unordered_map<std::string, std::string> attrs;
    if (decl->nativeAttrList()) {
        for (auto* attr : decl->nativeAttrList()->nativeAttr()) {
            std::string key = attr->children[0]->getText();
            std::string val = "";
            if (attr->children.size() >= 3) {
                val = attr->children[2]->getText();
                if (val.size() >= 2 && (val.front() == '"' || val.front() == '\'')) {
                    val = val.substr(1, val.size() - 2);
                }
            }
            attrs[key] = val;
        }
    }

    std::string dllPath = attrs["dll"];
    std::string realFuncName = attrs.count("fun") ? attrs["fun"] : funcName;
    void* procAddr = nullptr;

#ifdef _WIN32
    HMODULE hLib = LoadLibraryA(dllPath.c_str());
    if (hLib) {
        procAddr = (void*)GetProcAddress(hLib, realFuncName.c_str());
    }
#else
    void* hLib = dlopen(dllPath.c_str(), RTLD_LAZY);
    if (hLib) {
        procAddr = dlsym(hLib, realFuncName.c_str());
    }
#endif

    if (!procAddr) {
        std::cerr << "JIT Compile Error: Cannot find native function " << realFuncName
            << " in " << dllPath << std::endl;
        return std::any();
    }

    Value* nameConst = m_builder.CreateGlobalStringPtr(funcName);

    Value* addrInt = ConstantInt::get(Type::getInt64Ty(m_context), (uintptr_t)procAddr);
    Value* addrPtr = m_builder.CreateIntToPtr(addrInt, m_ptrTy);

    Value* nativeFuncObj = m_builder.CreateCall(
        getRtFunc("rt_create_native_val"),
        { nameConst, addrPtr }
    );

    AllocaInst* alloc = CreateEntryBlockAlloca(funcName);
    m_builder.CreateStore(nativeFuncObj, alloc);
    m_namedValues[funcName] = alloc;

    return std::any();
}
std::any TzdCompiler::visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext* ctx) {
    return visit(ctx->classDeclaration());
}
std::any TzdCompiler::visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) {
    auto exprs = ctx->exprList() ? ctx->exprList()->expression()
        : std::vector<TzdLangParser::ExpressionContext*>();

    // 检测是否全为数值字面量 → 走 native double 数组路径
    bool allNumLit = !exprs.empty();
    for (auto e : exprs) {
        if (!dynamic_cast<TzdLangParser::IntExprContext*>(e) &&
            !dynamic_cast<TzdLangParser::FloatExprContext*>(e)) {
            allNumLit = false; break;
        }
    }

    if (allNumLit) {
        int count = (int)exprs.size();
        Value* tmpBuf = CreateEntryBlockAlloca(m_doubleTy, m_builder.getInt32(count), "nat_arr_tmp");
        for (int i = 0; i < count; i++) {
            double v = std::stod(exprs[i]->getText());
            Value* slot = m_builder.CreateGEP(m_doubleTy, tmpBuf, m_builder.getInt32(i));
            m_builder.CreateStore(ConstantFP::get(m_doubleTy, v), slot);
        }
        return (Value*)m_builder.CreateCall(
            getRtFunc("rt_create_native_double_arr"),
            { m_builder.getInt32(count), tmpBuf }
        );
    }

    // 原有路径（含非 double 元素）
    Value* arr = m_builder.CreateCall(getRtFunc("rt_create_array"));
    if (ctx->exprList()) {
        for (auto e : ctx->exprList()->expression()) {
            Value* v = std::any_cast<Value*>(visit(e));
            Value* boxedV = boxToTzdValue(v);
            m_builder.CreateCall(getRtFunc("rt_array_push"), { arr, boxedV });
        }
    }
    return arr;
}
std::any TzdCompiler::visitIndexExpr(TzdLangParser::IndexExprContext* ctx) {
    Value* container = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* index_raw = std::any_cast<Value*>(visit(ctx->expression(1)));

    // Always use boxed path (rt_get_index) — returns TzdValue* pointer.
    // This is correct for both numeric arrays and object arrays.
    // The native double paths (rt_get_index_native_d) were incorrect for
    // object arrays because they read dVal (0.0 for objects, not the pointer).
    // Callers that need a double use inlineToDoubleFast/castToNativeDouble.
    Value* boxedIndex = boxToTzdValue(index_raw);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_get_index"), { container, boxedIndex });
}
std::any TzdCompiler::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));

    Value* lNative = L->getType()->isDoubleTy() ? L : inlineToDoubleFast(L);
    Value* rNative = R->getType()->isDoubleTy() ? R : inlineToDoubleFast(R);

    Value* res = nullptr;
    if (ctx->MUL()) res = m_builder.CreateFMul(lNative, rNative, "multmp");
    else if (ctx->DIV()) res = m_builder.CreateFDiv(lNative, rNative, "divtmp");
    else {
        // Adaptive integer specialization for %: fmod is a slow library call,
        // but srem is a single idiv instruction. Convert to i64 for integer modulo.
        Value* lInt = m_builder.CreateFPToSI(lNative, Type::getInt64Ty(m_context), "l2int");
        Value* rInt = m_builder.CreateFPToSI(rNative, Type::getInt64Ty(m_context), "r2int");
        Value* remInt = m_builder.CreateSRem(lInt, rInt, "modint");
        res = m_builder.CreateSIToFP(remInt, m_doubleTy, "int2dbl");
    }
    return std::any((Value*)res);
}
std::any TzdCompiler::visitPowerExpr(TzdLangParser::PowerExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));
    return (Value*)m_builder.CreateCall(getRtFunc("rt_op_pow"), { L, R });
}
std::any TzdCompiler::visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) {
    Value* V = std::any_cast<Value*>(visit(ctx->expression()));
    Value* boxed = boxToTzdValue(V);
    if (ctx->MINUS()) return (Value*)m_builder.CreateCall(getRtFunc("rt_op_neg"), { boxed });
    if (ctx->NOT()) return (Value*)m_builder.CreateCall(getRtFunc("rt_op_not"), { boxed });
    if (ctx->GXXX()) return (Value*)m_builder.CreateCall(getRtFunc("rt_op_sqrt"), { boxed });
    return boxed;
}
bool TzdCompiler::emitMemberIncDec(llvm::Value*& result, TzdLangParser::ExpressionContext* lhsCtx, bool isInc, bool isPrefix) {
    auto* atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx);
    if (!atomExpr) return false;
    auto* memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomExpr->atom());
    if (!memCtx) return false;

    Value* obj = std::any_cast<Value*>(visit(memCtx->atom()));
    std::string memberName = memCtx->IDENTIFIER()->getText();
    Value* nameStr = m_builder.CreateGlobalStringPtr(memberName);
    TzdSelector sel = internSelectorConstant(memberName);
    Value* oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_member"),
        { obj, m_builder.getInt32((int32_t)sel), nameStr });
    Value* one = m_builder.CreateCall(getRtFunc("rt_create_num"), { ConstantFP::get(m_doubleTy, 1.0) });
    const char* opFunc = isInc ? "rt_op_add" : "rt_op_sub";
    Value* newVal = m_builder.CreateCall(getRtFunc(opFunc), { oldVal, one });
    Value* stableNew = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { newVal });
    m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
        { obj, m_builder.getInt32((int32_t)sel), nameStr, stableNew });
    result = isPrefix ? newVal : oldVal;
    return true;
}

std::any TzdCompiler::visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) {
    auto lhsCtx = ctx->expression();
    std::string name = lhsCtx->getText();

    Value* memberResult = nullptr;
    if (emitMemberIncDec(memberResult, lhsCtx, ctx->INC() != nullptr, false)) {
        return memberResult;
    }

    // =========================================================
    // 1. 原生变量优化路径 (Native Optimization)
    // =========================================================
    // 必须最先检查！如果命中，直接生成 CPU 指令并返回，避免生成任何 Runtime 调用
    if (m_nativeDoubleLocals.count(name)) {
        Value* ptr = m_nativeDoubleLocals[name];

        // Load: 读取原生 double
        Value* oldVal = m_builder.CreateLoad(m_doubleTy, ptr, name + "_old");

        // Math: 原生加减 (纯 CPU 指令)
        Value* one = ConstantFP::get(m_doubleTy, 1.0);
        Value* newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);

        // Store: 写回原生 double
        m_builder.CreateStore(newVal, ptr);

        // 后缀操作 (i++) 返回旧值
        return oldVal;
    }

    // =========================================================
    // 2. 普通对象/成员路径 (Boxed Fallback)
    // =========================================================
    // 只有不是原生变量时，才执行下面的逻辑

    // A. 获取当前值 
    // visit 可能返回 native double (例如 arr[native_i]) 或 pointer
    // 所以必须调用 boxToTzdValue 确保它是 TzdValue* 指针
    Value* oldValRaw = std::any_cast<Value*>(visit(lhsCtx));
    Value* oldVal = boxToTzdValue(oldValRaw);

    // B. 准备运行时参数 (装箱的 1.0)
    Value* one = m_builder.CreateCall(getRtFunc("rt_create_num"), { ConstantFP::get(m_doubleTy, 1.0) });

    // C. 调用运行时运算 (rt_op_add / rt_op_sub)
    const char* opFunc = ctx->INC() ? "rt_op_add" : "rt_op_sub";
    Value* newVal = m_builder.CreateCall(getRtFunc(opFunc), { oldVal, one });

    // D. 写回逻辑 (Store Back)
    // 检查是局部对象变量、成员变量还是全局变量
    if (m_namedValues.count(name)) {
        // 情况 1: 局部 TzdValue* 变量 (非 Native)
        m_builder.CreateStore(newVal, m_namedValues[name]);
    }
    else if (m_namedValues.count("this") && !m_namedValues.count(name)) {
        Value* thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        TzdSelector sel = internSelectorConstant(name);
        Value* stableNew = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { newVal });
        m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
            { thisPtr, m_builder.getInt32((int32_t)sel), nameStr, stableNew });
    }
    else {
        // 情况 3: 全局变量
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, newVal });
    }

    // 后缀操作返回旧值 (Boxed)
    return oldVal;
}
std::any TzdCompiler::visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) {
    auto lhsCtx = ctx->expression();
    std::string name = lhsCtx->getText();

    Value* memberResult = nullptr;
    if (emitMemberIncDec(memberResult, lhsCtx, ctx->INC() != nullptr, true)) {
        return memberResult;
    }

    // --- [1] 原生变量优化 ---
    if (m_nativeDoubleLocals.count(name)) {
        Value* ptr = m_nativeDoubleLocals[name];
        Value* oldVal = m_builder.CreateLoad(m_doubleTy, ptr);
        Value* one = ConstantFP::get(m_doubleTy, 1.0);
        Value* newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);
        m_builder.CreateStore(newVal, ptr);
        return newVal; // 前缀返回新值
    }

    // --- [2] 安全回退 ---
    Value* oldValRaw = std::any_cast<Value*>(visit(lhsCtx));
    Value* oldVal = boxToTzdValue(oldValRaw); // [关键修复]

    Value* oneBoxed = m_builder.CreateCall(getRtFunc("rt_create_num"), { ConstantFP::get(m_doubleTy, 1.0) });
    const char* opFunc = ctx->INC() ? "rt_op_add" : "rt_op_sub";
    Value* newVal = m_builder.CreateCall(getRtFunc(opFunc), { oldVal, oneBoxed });

    // 简化的写回逻辑 (针对普通变量)
    if (m_namedValues.count(name)) {
        m_builder.CreateStore(newVal, m_namedValues[name]);
    }
    else if (dynamic_cast<TzdLangParser::IdExprContext*>(lhsCtx)) {
        Value* nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), { nameStr, newVal });
    }

    return newVal;
}
std::any TzdCompiler::visitForInit(TzdLangParser::ForInitContext* ctx) { if (ctx->variableDeclaration()) return visit(ctx->variableDeclaration()); if (ctx->expression()) return visit(ctx->expression()); return std::any(); }
std::any TzdCompiler::visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));

    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        Value* res = nullptr;
        if (ctx->GT()) res = m_builder.CreateFCmpOGT(L, R);
        else if (ctx->LT()) res = m_builder.CreateFCmpOLT(L, R);
        else if (ctx->GE()) res = m_builder.CreateFCmpOGE(L, R);
        else if (ctx->LE()) res = m_builder.CreateFCmpOLE(L, R);
        else res = m_builder.CreateFCmpOEQ(L, R);
        return std::any((Value*)res);
    }

    Value* boxedL = boxToTzdValue(L);
    Value* boxedR = boxToTzdValue(R);
    const char* rtFunc = ctx->GT() ? "rt_op_gt" :
        ctx->LT() ? "rt_op_lt" :
        ctx->GE() ? "rt_op_ge" : "rt_op_le";
    return std::any((Value*)m_builder.CreateCall(getRtFunc(rtFunc), { boxedL, boxedR }));
}
std::any TzdCompiler::visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));

    // Native double fast path: direct fcmp, zero heap alloc, zero external calls
    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        if (ctx->EEQ()) return std::any((Value*)m_builder.CreateFCmpOEQ(L, R, "eqtmp"));
        else            return std::any((Value*)m_builder.CreateFCmpONE(L, R, "netmp"));
    }

    // Fallback: boxed path for string/object comparison
    Value* boxedL = boxToTzdValue(L);
    Value* boxedR = boxToTzdValue(R);
    const char* func = ctx->EEQ() ? "rt_op_eq" : "rt_op_ne";
    return (Value*)m_builder.CreateCall(getRtFunc(func), { boxedL, boxedR });
}
std::any TzdCompiler::visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) {
    Function* func = m_builder.GetInsertBlock()->getParent();
    AllocaInst* resultSlot = CreateEntryBlockAlloca(m_boolTy, nullptr, "and.sc");
    BasicBlock* headBB = m_builder.GetInsertBlock();

    Value* lhs = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* lhsBool = toNativeBool(boxToTzdValue(lhs));

    BasicBlock* rhsBB = BasicBlock::Create(m_context, "and.rhs", func);
    BasicBlock* falseBB = BasicBlock::Create(m_context, "and.false", func);
    BasicBlock* endBB = BasicBlock::Create(m_context, "and.end", func);

    m_builder.SetInsertPoint(headBB);
    m_builder.CreateCondBr(lhsBool, rhsBB, falseBB);

    m_builder.SetInsertPoint(falseBB);
    m_builder.CreateStore(m_builder.getFalse(), resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(rhsBB);
    Value* rhs = std::any_cast<Value*>(visit(ctx->expression(1)));
    Value* rhsBool = toNativeBool(boxToTzdValue(rhs));
    m_builder.CreateStore(rhsBool, resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(endBB);
    Value* loaded = m_builder.CreateLoad(m_boolTy, resultSlot);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_bool"), { loaded });
}
std::any TzdCompiler::visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) {
    Function* func = m_builder.GetInsertBlock()->getParent();
    AllocaInst* resultSlot = CreateEntryBlockAlloca(m_boolTy, nullptr, "or.sc");
    BasicBlock* headBB = m_builder.GetInsertBlock();

    Value* lhs = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* lhsBool = toNativeBool(boxToTzdValue(lhs));

    BasicBlock* rhsBB = BasicBlock::Create(m_context, "or.rhs", func);
    BasicBlock* trueBB = BasicBlock::Create(m_context, "or.true", func);
    BasicBlock* endBB = BasicBlock::Create(m_context, "or.end", func);

    m_builder.SetInsertPoint(headBB);
    m_builder.CreateCondBr(lhsBool, trueBB, rhsBB);

    m_builder.SetInsertPoint(trueBB);
    m_builder.CreateStore(m_builder.getTrue(), resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(rhsBB);
    Value* rhs = std::any_cast<Value*>(visit(ctx->expression(1)));
    Value* rhsBool = toNativeBool(boxToTzdValue(rhs));
    m_builder.CreateStore(rhsBool, resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(endBB);
    Value* loaded = m_builder.CreateLoad(m_boolTy, resultSlot);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_bool"), { loaded });
}
std::any TzdCompiler::visitParenExpr(TzdLangParser::ParenExprContext* ctx) {
    return visit(ctx->expression());
}
std::any TzdCompiler::visitCastExpr(TzdLangParser::CastExprContext* ctx) {
    Value* val = std::any_cast<Value*>(visit(ctx->expression()));
    Value* typeName = m_builder.CreateGlobalStringPtr(ctx->typeType()->getText());
    return (Value*)m_builder.CreateCall(getRtFunc("rt_cast"), { val, typeName });
}
std::any TzdCompiler::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) { return (Value*)m_builder.CreateCall(getRtFunc("rt_create_bool"), { m_builder.getTrue() }); }
std::any TzdCompiler::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) { return (Value*)m_builder.CreateCall(getRtFunc("rt_create_bool"), { m_builder.getFalse() }); }
std::any TzdCompiler::visitImportStmt(TzdLangParser::ImportStmtContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) {
    Value* obj = std::any_cast<Value*>(visit(ctx->expression()));
    std::string targetName = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();
    Value* typeNameStr = m_builder.CreateGlobalStringPtr(targetName);
    Value* resBool = m_builder.CreateCall(getRtFunc("rt_type_check"), { obj, typeNameStr });
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_bool"), { resBool });
}
std::any TzdCompiler::visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitEnumDeclaration(TzdLangParser::EnumDeclarationContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitSuperExpr(TzdLangParser::SuperExprContext* ctx) {
    if (!m_namedValues.count("this")) {
        return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
    }

    Value* thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);

    std::vector<Value*> args;
    args.push_back(thisPtr);

    if (ctx->exprList()) {
        for (auto e : ctx->exprList()->expression()) {
            args.push_back(std::any_cast<Value*>(visit(e)));
        }
    }
    Value* argCount = m_builder.getInt32((unsigned int)args.size() - 1);
    std::vector<Value*> callParams;
    callParams.push_back(thisPtr);
    callParams.push_back(argCount);
    for (size_t i = 1; i < args.size(); ++i) callParams.push_back(args[i]);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_call_super"), callParams);
}

std::any TzdCompiler::visitCallExpr(TzdLangParser::CallExprContext* ctx) {
    std::string funcName = ctx->atom()->getText();
    auto exprs = ctx->exprList() ? ctx->exprList()->expression()
        : std::vector<TzdLangParser::ExpressionContext*>();
    int argCount = (int)exprs.size();

    Function* currentFunc = m_builder.GetInsertBlock()->getParent();
    std::string currentName = currentFunc->getName().str();

    // 剥离版本号和 _worker / _worker_native 后缀，获取真正的函数原始名字
    std::string baseName = currentName;
    if (currentName.size() > 14 && currentName.substr(currentName.size() - 14) == "_worker_native") {
        baseName = currentName.substr(0, currentName.size() - 14);
    }
    else if (currentName.size() > 7 && currentName.substr(currentName.size() - 7) == "_worker") {
        baseName = currentName.substr(0, currentName.size() - 7);
    }
    size_t lastUnderscore = baseName.find_last_of('_');
    if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v') {
        baseName = baseName.substr(0, lastUnderscore);
    }

    // ==============================================================
    // 1. [直接自递归消除] (本函数内的纯循环转换)
    // ==============================================================
    if (funcName == baseName && s_inTailPosition && s_tailRecurseBB != nullptr) {
        std::vector<Value*> evalArgs;
        for (int i = 0; i < argCount; ++i) {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
            evalArgs.push_back(castToNativeDouble(argRaw));
            s_inTailPosition = oldTail;
        }

        for (int i = 0; i < argCount && i < (int)s_currentFuncParamNames.size(); ++i) {
            std::string pName = s_currentFuncParamNames[i];
            if (m_nativeDoubleLocals.count(pName)) {
                m_builder.CreateStore(evalArgs[i], m_nativeDoubleLocals[pName]);
            }
            else if (m_namedValues.count(pName)) {
                m_builder.CreateStore(boxToTzdValue(evalArgs[i]), m_namedValues[pName]);
            }
        }

        m_builder.CreateBr(s_tailRecurseBB);
        BasicBlock* deadBB = BasicBlock::Create(m_context, "unreachable_after_tre", currentFunc);
        m_builder.SetInsertPoint(deadBB);
        m_builder.CreateUnreachable();
        return std::any((Value*)ConstantFP::get(m_doubleTy, 0.0));
    }

    // ==============================================================
    // 2. [直接自递归旁路] (调用自身的 Native Worker，直接传 double)
    // ==============================================================
    if (funcName == baseName && s_currentNativeWorkerFunc) {
        // Phase B: Call native worker with double args directly — no arg array!
        Value* interp = currentFunc->getArg(0);
        std::vector<Value*> callArgs;
        callArgs.push_back(interp);
        for (int i = 0; i < argCount; ++i) {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
            s_inTailPosition = oldTail;
            // Ensure arg is double (castToNativeDouble handles both doubles and pointers)
            callArgs.push_back(castToNativeDouble(argRaw));
        }
        Value* nativeDoubleResult = m_builder.CreateCall(s_currentNativeWorkerFunc, callArgs);
        return std::any((Value*)nativeDoubleResult);
    }
    if (funcName == baseName && s_currentWorkerFunc) {
        Value* argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "rec_args");
        if (argCount > 0) {
            m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), { argsArray, m_builder.getInt32(argCount) });
        }
        for (int i = 0; i < argCount; ++i) {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
            s_inTailPosition = oldTail;

            Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            if (argRaw->getType()->isDoubleTy()) {
                inlineStoreNativeToPtr(argPtr, argRaw);
            }
            else {
                m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { argPtr, boxToTzdValue(argRaw) });
            }
        }
        Value* dummyResSlot = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        Value* interp = currentFunc->getArg(0);
        Value* nativeDoubleResult = m_builder.CreateCall(s_currentWorkerFunc, { interp, dummyResSlot, argsArray });
        return std::any((Value*)nativeDoubleResult);
    }

    // ==============================================================
    // 3. 动态函数调用 / 间接相互递归 / 原生函数系统调用
    // ==============================================================

    // Phase D: Direct native worker call for known JIT-compiled functions.
    // Bypasses args array, rt_init_tzd_value, inlineStoreNativeToPtr, rt_create_num.
    // This makes non-recursive function calls as fast as C function calls.
    if (auto nativeIt = s_compiledNativeWorkers.find(funcName); nativeIt != s_compiledNativeWorkers.end()) {
        Function* nativeWorker = nativeIt->second;
        Value* interp = currentFunc->getArg(0);
        std::vector<Value*> callArgs;
        callArgs.push_back(interp);
        for (int i = 0; i < argCount; ++i) {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
            s_inTailPosition = oldTail;
            callArgs.push_back(castToNativeDouble(argRaw));
        }
        Value* result = m_builder.CreateCall(nativeWorker, callArgs);
        return std::any((Value*)result);
    }

    Value* argsArray = nullptr;

    // 【核心改进 1】：若处于尾部，且参数个数不大于当前函数参数个数，直接复用当前函数的输入参数数组，避免分配新的栈内存
    if (s_inTailPosition && s_currentWorkerFunc && argCount <= (int)s_currentFuncParamNames.size()) {
        argsArray = s_currentWorkerFunc->getArg(2);
    }
    else {
        argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "call_args");
        if (argCount > 0) {
            m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), { argsArray, m_builder.getInt32(argCount) });
        }
    }

    // 提前计算所有的参数值，防止原地覆盖数组时发生数据干扰
    std::vector<Value*> evaluatedArgs;
    evaluatedArgs.reserve(argCount);
    for (int i = 0; i < argCount; ++i) {
        bool oldTail = s_inTailPosition;
        s_inTailPosition = false;
        Value* argRaw = std::any_cast<Value*>(visit(exprs[i]));
        s_inTailPosition = oldTail;
        evaluatedArgs.push_back(argRaw);
    }

    for (int i = 0; i < argCount; ++i) {
        Value* argRaw = evaluatedArgs[i];
        Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
        if (argRaw->getType()->isDoubleTy()) {
            inlineStoreNativeToPtr(argPtr, argRaw);
        }
        else {
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { argPtr, boxToTzdValue(argRaw) });
        }
    }

    if (!dynamic_cast<TzdLangParser::IdExprContext*>(ctx->atom())) {
        if (auto memberCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(ctx->atom())) {
            Value* obj = castAnyToValue(visit(memberCtx->atom()), "visitCallExpr.memberObj");
            std::string memberName = memberCtx->IDENTIFIER()->getText();
            Value* nameStr = m_builder.CreateGlobalStringPtr(memberName);
            TzdSelector sel = internSelectorConstant(memberName);
            Value* resPtr = m_builder.CreateCall(getRtFunc("rt_tzd_call_method"),
                { obj, m_builder.getInt32((int32_t)sel), nameStr, m_builder.getInt32(argCount), argsArray });
            return std::any((Value*)resPtr);
        }
        Value* callee = castAnyToValue(visit(ctx->atom()), "visitCallExpr.callee");
        Value* resPtr = m_builder.CreateCall(getRtFunc("rt_call_value_fast"), { callee, m_builder.getInt32(argCount), argsArray });
        return std::any((Value*)resPtr);
    }

    // ==============================================================
    // [终极交叉递归优化]：动态解析目标函数的 Worker 指针
    // ==============================================================
    Value* funcNameStr = m_builder.CreateGlobalStringPtr(funcName);
    Value* workerPtr = m_builder.CreateCall(getRtFunc("rt_get_worker_ptr"), { funcNameStr });
    Value* isWorkerValid = m_builder.CreateICmpNE(workerPtr, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)));

    BasicBlock* fastCallBB = BasicBlock::Create(m_context, "fast_worker_call", currentFunc);
    BasicBlock* slowCallBB = BasicBlock::Create(m_context, "slow_interp_call", currentFunc);

    if (s_inTailPosition) {
        m_builder.CreateCondBr(isWorkerValid, fastCallBB, slowCallBB);

        // -- Fast Path: 跨函数的纯 LLVM 尾调用 --
        m_builder.SetInsertPoint(fastCallBB);

        // 【新增】：因为是尾调用优化（TCO），旧的栈帧已经被覆盖，所以这里替换当前的栈帧记录
        m_builder.CreateCall(getRtFunc("rt_replace_jit_frame"), { funcNameStr });

        std::vector<Type*> workerSignature = { m_ptrTy, m_ptrTy, m_ptrTy };
        FunctionType* workerFTy = FunctionType::get(m_doubleTy, workerSignature, false);
        Value* interp = currentFunc->getArg(0);

        CallInst* fastRes = m_builder.CreateCall(workerFTy, workerPtr, { interp, m_currentRetPtr, argsArray });
        fastRes->setTailCallKind(CallInst::TCK_Tail);  // Use Tail (not MustTail) for safety
        m_builder.CreateRet(fastRes);

        // -- Slow Path: 回退到普通 C++ 函数 --
        m_builder.SetInsertPoint(slowCallBB);
        Value* slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), { funcNameStr, m_builder.getInt32(argCount), argsArray });
        Value* slowRes = inlineToDoubleFast(slowResPtr);
        m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { m_currentRetPtr, slowRes });
        m_builder.CreateRet(slowRes);

        BasicBlock* deadBB = BasicBlock::Create(m_context, "tail_call_unreachable", currentFunc);
        m_builder.SetInsertPoint(deadBB);
        m_builder.CreateUnreachable();

        return std::any((Value*)ConstantFP::get(m_doubleTy, 0.0));
    }

    // --- 非尾调用位置，保持原有的 merge 逻辑 ---
    BasicBlock* mergeBB = BasicBlock::Create(m_context, "merge_call", currentFunc);
    m_builder.CreateCondBr(isWorkerValid, fastCallBB, slowCallBB);

    // Use pointer slot — both paths return TzdValue* (preserves strings/objects)
    AllocaInst* resPtrSlot = CreateEntryBlockAlloca(m_ptrTy, nullptr, "dyn_call_res");

    m_builder.SetInsertPoint(fastCallBB);

    // Direct worker call — no JIT frame push/pop (eliminates string formatting overhead)
    std::vector<Type*> workerSignature = { m_ptrTy, m_ptrTy, m_ptrTy };
    FunctionType* workerFTy = FunctionType::get(m_doubleTy, workerSignature, false);
    Value* interp = currentFunc->getArg(0);
    Value* dummyResSlotFast = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
    CallInst* fastRes = m_builder.CreateCall(workerFTy, workerPtr, { interp, dummyResSlotFast, argsArray });
    Value* boxedFast = m_builder.CreateCall(getRtFunc("rt_create_num"), { fastRes });
    m_builder.CreateStore(boxedFast, resPtrSlot);

    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(slowCallBB);
    Value* slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), { funcNameStr, m_builder.getInt32(argCount), argsArray });
    // Keep TzdValue* directly — preserves strings/objects from interpreter
    m_builder.CreateStore(slowResPtr, resPtrSlot);
    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(mergeBB);
    Value* finalRes = m_builder.CreateLoad(m_ptrTy, resPtrSlot, "call_res");
    return std::any((Value*)finalRes);
}

std::any TzdCompiler::visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) {
    m_builder.CreateCall(getRtFunc("rt_set_location"), {
        m_builder.getInt32(ctx->getStart()->getLine()),
        m_builder.getInt32(ctx->getStart()->getCharPositionInLine())
        });
    Value* errVal = boxToTzdValue(std::any_cast<Value*>(visit(ctx->expression())));
    m_builder.CreateCall(getRtFunc("rt_throw"), { errVal });

    Function* currentFunc = m_builder.GetInsertBlock()->getParent();
    if (currentFunc->getReturnType()->isDoubleTy()) {
        m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
    }
    else {
        m_builder.CreateRetVoid();
    }

    BasicBlock* deadBB = BasicBlock::Create(m_context, "unreachable_after_throw", currentFunc);
    m_builder.SetInsertPoint(deadBB);

    return std::any((Value*)nullptr);
}

std::any TzdCompiler::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) {
    Function* func = m_builder.GetInsertBlock()->getParent();
    BasicBlock* tryBB = BasicBlock::Create(m_context, "try.body", func);
    BasicBlock* catchBB = BasicBlock::Create(m_context, "try.catch", func);
    BasicBlock* afterBB = BasicBlock::Create(m_context, "try.after", func);

    Value* jmpBuf = m_builder.CreateCall(getRtFunc("rt_alloc_jmp_buf"));
    Value* oldJmpBuf = m_builder.CreateCall(getRtFunc("rt_get_catch_jmp"));
    m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { jmpBuf });

    Value* enterRes = nullptr;
#ifdef _WIN32
    Function* sjF = m_module->getFunction("_setjmp");
    Value* nullFrame = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
    enterRes = m_builder.CreateCall(sjF, { jmpBuf, nullFrame }); // 直接内联调用原生的 _setjmp!
#else
    Function* sjF = m_module->getFunction("setjmp");
    enterRes = m_builder.CreateCall(sjF, { jmpBuf });
#endif

    Value* isCatch = m_builder.CreateICmpNE(enterRes, m_builder.getInt32(0));
    m_builder.CreateCondBr(isCatch, catchBB, tryBB);

    m_builder.SetInsertPoint(tryBB);
    s_tryJmpBufStack.push_back({ jmpBuf, oldJmpBuf, s_loopLevel });
    visit(ctx->block(0));
    s_tryJmpBufStack.pop_back();

    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { oldJmpBuf });
        m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { jmpBuf });
        m_builder.CreateBr(afterBB);
    }

    m_builder.SetInsertPoint(catchBB);
    m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), { oldJmpBuf });
    m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { jmpBuf }); // 避免异常再次发生时泄露

    Value* thrown = m_builder.CreateCall(getRtFunc("rt_get_thrown"));
    std::string errName = ctx->IDENTIFIER()->getText();

    // 自动利用 JIT 的 Alloca 分配捕获的变量对象环境，完全抛弃缓慢的 Runtime Scope !
    AllocaInst* alloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, errName + "_catch");
    m_builder.CreateStore(thrown, alloc);

    auto backupNamedVals = m_namedValues;
    m_namedValues[errName] = alloc;

    visit(ctx->block(1));

    m_namedValues = backupNamedVals;

    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(afterBB);
    }

    m_builder.SetInsertPoint(afterBB);
    return std::any((Value*)ConstantPointerNull::get(cast<PointerType>(m_ptrTy)));
}

std::any TzdCompiler::visitNullExpr(TzdLangParser::NullExprContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) { return visit(ctx->expression()); }
std::any TzdCompiler::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) {
    // Must save/restore JIT compilation context — just like visitLambdaExpr.
    // Without this, compileNamedFunction for the nested function clobbers
    // m_currentRetPtr, s_currentWorkerFunc, s_tailRecurseBB, etc.,
    // causing null operands in the parent function's IR.
    auto* savedInsertBlock = m_builder.GetInsertBlock();
    auto savedNamedValues = m_namedValues;
    auto savedNativeLocals = m_nativeDoubleLocals;
    auto savedParamNames = s_currentFuncParamNames;
    auto* savedTailBB = s_tailRecurseBB;
    auto* savedWorkerFunc = s_currentWorkerFunc;
    auto* savedRetPtr = m_currentRetPtr;
    bool savedTailState = s_inTailPosition;

    auto result = visit(ctx->functionDeclaration());

    // Register nested functions with the interpreter so they can be found
    // by rt_call_sub_fast and rt_get_worker_ptr at runtime.
    if (g_CurrentInterpreter) {
        std::string funcName = ctx->functionDeclaration()->IDENTIFIER()->getText();
        g_CurrentInterpreter->m_pendingJitFunctions.insert(funcName);
    }

    m_builder.SetInsertPoint(savedInsertBlock);
    m_namedValues = savedNamedValues;
    m_nativeDoubleLocals = savedNativeLocals;
    s_currentFuncParamNames = savedParamNames;
    s_tailRecurseBB = savedTailBB;
    s_currentWorkerFunc = savedWorkerFunc;
    m_currentRetPtr = savedRetPtr;
    s_inTailPosition = savedTailState;

    return result;
}

Value* TzdCompiler::castToNativeDouble(Value* val) {
    if (!val) return ConstantFP::get(m_doubleTy, 0.0);

    // 1. 如果已经是 double，直接用
    if (val->getType()->isDoubleTy()) return val;

    // 2. [关键]：如果是 i1 (布尔值)，直接用 LLVM 指令转为 double (0.0 或 1.0)
    // 绝对不能把 i1 当作指针传给 rt_to_double_fast
    if (val->getType()->isIntegerTy(1)) {
        return m_builder.CreateUIToFP(val, m_doubleTy, "bool2double");
    }

    // 3. 只有当它是 TzdValue* 指针时，inline GEP+Load dVal
    return inlineToDoubleFast(val);
}

// Inline rt_store_native_to_ptr: directly write type=DOUBLE and dVal=val via GEP
// Eliminates a C function call per argument store in recursive calls
void TzdCompiler::inlineStoreNativeToPtr(Value* dest, Value* nativeDouble) {
    // Cast to i8* for byte-offset GEP
    Value* rawPtr = m_builder.CreateBitCast(dest,
        llvm::PointerType::get(llvm::Type::getInt8Ty(m_context), 0));
    // Write type = DOUBLE (bitcast i8* to i32* for correct store type)
    Value* typePtr = m_builder.CreateConstGEP1_32(
        llvm::Type::getInt8Ty(m_context), rawPtr, (uint32_t)TZD_TYPE_OFFSET);
    Value* typeTypedPtr = m_builder.CreateBitCast(typePtr,
        llvm::PointerType::get(m_int32Ty, 0));
    m_builder.CreateStore(m_builder.getInt32((uint32_t)TzdValue::DOUBLE), typeTypedPtr);
    // Write dVal = nativeDouble
    Value* dValPtr = m_builder.CreateConstGEP1_32(
        llvm::Type::getInt8Ty(m_context), rawPtr, (uint32_t)TZD_DVAL_OFFSET);
    Value* dValTypedPtr = m_builder.CreateBitCast(dValPtr,
        llvm::PointerType::get(m_doubleTy, 0));
    m_builder.CreateStore(nativeDouble, dValTypedPtr);
}

// Inline rt_to_double_fast: just calls rt_to_double_fast.
// Full inlining (GEP+Load dVal) is unsafe for non-DOUBLE types (INT stores in lVal).
// Phase B (native worker specialization) will eliminate this call entirely for self-recursion.
Value* TzdCompiler::inlineToDoubleFast(Value* src) {
    return m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { src }, "to_double");
}

llvm::Value* TzdCompiler::boxDouble(llvm::Value* nativeVal) {
    if (nativeVal->getType()->isPointerTy()) return nativeVal;
    return m_builder.CreateCall(getRtFunc("rt_create_num"), { nativeVal });
}
















