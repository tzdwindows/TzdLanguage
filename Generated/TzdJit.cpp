#include "../Res/TzdStrings.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <csetjmp>
#include <unordered_map>

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

// ======= 尾递归优化(TRE)上下文 =======
static std::vector<std::string> s_currentFuncParamNames;
static llvm::BasicBlock* s_tailRecurseBB = nullptr;
static bool s_inTailPosition = false;
static llvm::Function* s_currentWorkerFunc = nullptr;
static std::unordered_map<std::string, void*> s_workerPointers;

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
        if (!funcObjPtr) return g_JitPool.next();

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

        // 为了防止 dest 里的 std::string 和 std::vector 结构由于垃圾字节引发后续崩溃
        // 先从 JIT 池获取一个“合法且安全”的空对象
        TzdValue* safe = g_JitPool.next();
        safe->type = TzdValue::DOUBLE;
        safe->dVal = val;

        // 用 memcpy 直接暴力覆盖未初始化内存，使其变为合法状态
        memcpy(dest, safe, sizeof(TzdValue));
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
        v->instanceVal = new TzdInstance(def);
        return v;
    }

    void* rt_create_inst_args(const char* name, int argCount, TzdValue* args) {
        TzdClassDef* def = TzdOopManager::getClass(name);
        if (!def) {
            if (g_CurrentInterpreter) g_CurrentInterpreter->reportJitError(std::string("找不到类定义: '") + name + "' (是否忘记 import?)");
            return g_JitPool.next();
        }

        TzdValue* instVal = g_JitPool.next();
        instVal->type = TzdValue::INSTANCE;
        instVal->instanceVal = new TzdInstance(def);

        ClassConstructor* ctor = def->findConstructor((size_t)argCount);
        if (!ctor && argCount == 0 && !def->constructors.empty()) {
            ctor = def->findConstructor(0);
        }
        if (!ctor) return instVal;

        if (ctor->jittedPtr) {
            TzdValue* ignored = g_JitPool.next();
            constexpr int kInlineCtorArgs = 8;
            if (argCount + 1 <= kInlineCtorArgs) {
                TzdValue frameInline[kInlineCtorArgs];
                frameInline[0] = *instVal;
                for (int i = 0; i < argCount; ++i) frameInline[i + 1] = args[i];
                g_CurrentInterpreter->m_argPtrStack.push_back(frameInline);

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
                g_CurrentInterpreter->m_argPtrStack.pop_back();
            }
            else {
                std::vector<TzdValue> frame;
                frame.reserve((size_t)argCount + 1);
                frame.push_back(*instVal);
                for (int i = 0; i < argCount; ++i) frame.push_back(args[i]);
                g_CurrentInterpreter->m_argPtrStack.push_back(frame.data());

                std::string fileLoc = formatSourcePath(ctor->sourceFile);
                int line = ctor->line;
                std::string frameName = std::string(def->fullName) + "." + def->simpleName;
                frameName += " (" + fileLoc;
                if (line > 0) frameName += ":" + std::to_string(line);
                frameName += ") (JIT Compiled)";

                g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
                ctor->jittedPtr(g_CurrentInterpreter, ignored);
                g_CurrentInterpreter->m_callStackFrames.pop_back();
                g_CurrentInterpreter->m_argPtrStack.pop_back();
            }
        }
        else if (g_CurrentInterpreter && ctor->body) {
            std::vector<TzdValue> callArgs;
            callArgs.reserve(argCount);
            for (int i = 0; i < argCount; ++i) callArgs.push_back(args[i]);
            TzdValue ctorFunc(def->simpleName, ctor->params, ctor->body);
            ctorFunc.jittedPtr = ctor->jittedPtr;
            ctorFunc.instanceVal = instVal->instanceVal;

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

    void* rt_get_member(void* inst, const char* name) {
        TzdValue* v = (TzdValue*)inst;
        if (v->type == TzdValue::CLASS_DEF && v->classDefVal) {
            if (TzdValue* direct = v->classDefVal->findStaticValue(name)) return direct;
            if (ClassMethod* method = v->classDefVal->findMethod(name)) {
                TzdValue* res = g_JitPool.next();
                res->type = method->isNative ? TzdValue::NATIVE_FUNCTION : TzdValue::FUNCTION;
                res->sourceFile = method->sourceFile;
                res->line = method->line;
                res->column = method->column;
                res->name = method->name;
                res->params = method->params;
                res->funcBody = method->body;
                res->jittedPtr = method->jittedPtr;
                if (method->isNative) res->nativeFunc = method->nativeWrapper;
                return res;
            }
        }
        if (v->type == TzdValue::INSTANCE && v->instanceVal) {
            if (TzdValue* direct = v->instanceVal->getMemberPtr(name)) {
                return direct;
            }

            if (v->instanceVal->definition) {
                if (ClassMethod* method = v->instanceVal->definition->findMethod(name)) {
                    TzdValue* res = g_JitPool.next();
                    res->name = method->name;
                    res->instanceVal = v->instanceVal;
                    res->jittedPtr = method->jittedPtr;
                    res->sourceFile = method->sourceFile;
                    res->line = method->line;
                    res->column = method->column;

                    if (method->isNative) {
                        res->type = TzdValue::NATIVE_FUNCTION;
                        res->nativeFunc = method->nativeWrapper;
                    }
                    else {
                        res->type = TzdValue::FUNCTION;
                        // In the hot path (JIT on + method already jitted), avoid copying
                        // params/body vectors every call; call bridge uses jittedPtr directly.
                        if (!(method->jittedPtr && !TzdDebugger::g_DebugActive)) {
                            res->funcBody = method->body;
                            res->params = method->params;
                            res->paramTypes = method->paramTypes;
                        }
                    }
                    return res;
                }
            }
        }
        return g_JitPool.next();
    }

    void rt_store_member(void* inst, const char* name, void* val) {
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
        ctorFunc.instanceVal = inst;

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
        delete static_cast<jmp_buf*>(buf);
    }

    int rt_enter_try_buf(void* buf) {
        g_tzdCatchJmp = static_cast<jmp_buf*>(buf);
        return setjmp(*g_tzdCatchJmp);
    }

    void rt_leave_try() {
        g_tzdCatchJmp = nullptr;
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

        if (g_tzdCatchJmp) {
            longjmp(*g_tzdCatchJmp, 1);
        }

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
    workerFunc->setDLLStorageClass(GlobalValue::DLLExportStorageClass);
    workerFunc->setVisibility(GlobalValue::DefaultVisibility);
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    // 2. Entry 依然返回 void
    std::vector<Type*> entryArgs = { m_ptrTy, m_ptrTy };
    Function* entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get()
    );
    entryFunc->setDLLStorageClass(GlobalValue::DLLExportStorageClass);
    entryFunc->setVisibility(GlobalValue::DefaultVisibility);

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
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { destPtr, argRaw });
        }
    }
    m_builder.CreateCall(workerFunc, { interp, retVal, argsArray });
    m_builder.CreateRetVoid();

    BasicBlock* workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
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

            Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
            AllocaInst* boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argPtr, boxedAlloc);
            m_namedValues[pName] = boxedAlloc;

            // 注意：已经彻底去除了向 m_nativeDoubleLocals 的写入，确保形参保留原始 TzdValue 指针类型
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
    m_currentRetPtr = func->getArg(1);

    Value* thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(0) });
    AllocaInst* thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;

    if (methodCtx->paramList()) {
        auto pList = methodCtx->paramList()->param();
        for (int i = 0; i < (int)pList.size(); ++i) {
            std::string pName = getParamName(pList[i]);
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
    m_currentRetPtr = func->getArg(1);

    Value* thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), { m_builder.getInt32(0) });
    AllocaInst* thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;

        if (ctorCtx->paramList()) {
            auto pList = ctorCtx->paramList()->param();
            for (int i = 0; i < (int)pList.size(); ++i) {
                std::string pName = getParamName(pList[i]);
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
            agentLogJit("D", "compileConstructor.stmt", stmt->getText().substr(0, 80).c_str());
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
    workerFunc->setDLLStorageClass(GlobalValue::DLLExportStorageClass);
    workerFunc->setVisibility(GlobalValue::DefaultVisibility);
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    // 2. Entry 依然返回 void 供外部 C++ 解释器调用
    std::vector<Type*> entryArgs = { m_ptrTy, m_ptrTy };
    Function* entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get()
    );
    entryFunc->setDLLStorageClass(GlobalValue::DLLExportStorageClass);
    entryFunc->setVisibility(GlobalValue::DefaultVisibility);

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
    m_builder.CreateRetVoid();

    BasicBlock* workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
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

            Value* argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));

            AllocaInst* boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argPtr, boxedAlloc);
            m_namedValues[pName] = boxedAlloc;

            // 注意：已经彻底去除了向 m_nativeDoubleLocals 的写入，确保形参保留原始 TzdValue 指针类型
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

TzdJitEngine::~TzdJitEngine() {}

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
    addFunc("rt_enter_try_buf", { m_ptrTy }, Type::getInt32Ty(m_context));
    addFunc("rt_leave_try", {}, m_voidTy);
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

void TzdJitEngine::addModule(ThreadSafeModule TSM) {
    if (!m_lljit) {
        errs() << "addModule: LLJIT is not initialized\n";
        return;
    }

    if (auto Err = m_lljit->addIRModule(std::move(TSM))) {
        errs() << "addIRModule failed: " << toString(std::move(Err)) << "\n";
        consumeError(std::move(Err));
    }
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
    bind("rt_get_member", (void*)&rt_get_member);
    bind("rt_store_member", (void*)&rt_store_member);
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
    bind("rt_push_arg_frame",(void*)&rt_push_arg_frame);
    bind("rt_pop_arg_frame",(void*)&rt_pop_arg_frame);
    bind("rt_init_tzd_value", (void*)&rt_init_tzd_value);
    bind("rt_write_fast_ret", (void*)&rt_write_fast_ret);
    bind("rt_get_worker_ptr", (void*)&rt_get_worker_ptr);
    bind("rt_alloc_jmp_buf", (void*)&rt_alloc_jmp_buf);
    bind("rt_free_jmp_buf", (void*)&rt_free_jmp_buf);
    bind("rt_enter_try_buf", (void*)&rt_enter_try_buf);
    bind("rt_leave_try", (void*)&rt_leave_try);
    bind("rt_throw", (void*)&rt_throw);
    bind("rt_get_thrown", (void*)&rt_get_thrown);
    bind("rt_push_catch_scope", (void*)&rt_push_catch_scope);
    bind("rt_pop_catch_scope", (void*)&rt_pop_catch_scope);
    bind("rt_set_location", (void*)&rt_set_location);
    bind("rt_push_jit_frame", (void*)&rt_push_jit_frame);
    bind("rt_pop_jit_frame", (void*)&rt_pop_jit_frame);
    bind("rt_replace_jit_frame", (void*)&rt_replace_jit_frame);
    bind("rt_create_lambda_value", (void*)&rt_create_lambda_value);


    cantFail(m_lljit->getMainJITDylib().define(absoluteSymbols(symbols)));
}

TzdCompiler::TzdCompiler(TzdJitEngine& jit, const std::string& modName)
    : m_jitEngine(jit),
    m_context(jit.getContext()),
    m_builder(m_context),
    m_tsc(jit.getThreadSafeContext())
{
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


llvm::orc::ThreadSafeModule TzdCompiler::extractThreadSafeModule() {
    if (!m_module) {
        return llvm::orc::ThreadSafeModule(nullptr, m_tsc);
    }

    // 1. 首先物理清理所有无前驱的死代码块
    for (auto& F : *m_module) {
        if (!F.isDeclaration()) {
            llvm::removeUnreachableBlocks(F);
        }
    }

    // 2. 验证模块合法性
    if (llvm::verifyModule(*m_module, &llvm::errs())) {
        llvm::errs() << "\n>>> [JIT FATAL ERROR]: Module verification failed! \n";
        llvm::errs() << "========== [ DUMPING GENERATED LLVM IR CODE ] ==========\n";
        m_module->print(llvm::errs(), nullptr);
        llvm::errs() << "========================================================\n\n";
        return llvm::orc::ThreadSafeModule(nullptr, m_tsc);
    }

    //m_module->print(llvm::errs(), nullptr);

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    ModulePassManager MPM;
    FunctionPassManager FPM;
    FPM.addPass(llvm::PromotePass());
    FPM.addPass(llvm::SimplifyCFGPass());
    FPM.addPass(llvm::EarlyCSEPass(true));
    FPM.addPass(llvm::DCEPass());
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
    MPM.run(*m_module, MAM);

    return llvm::orc::ThreadSafeModule(std::move(m_module), m_tsc);
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
    Value* initVal = nullptr;
    if (ctx->variableDeclaration()->expression()) {
        initVal = std::any_cast<Value*>(visit(ctx->variableDeclaration()->expression()));
    }
    else {
        initVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
    }
    if (initVal->getType()->isDoubleTy()) {
        AllocaInst* alloc = m_builder.CreateAlloca(m_doubleTy, nullptr, name + "_native");
        m_builder.CreateStore(initVal, alloc);
        m_nativeDoubleLocals[name] = alloc;
        return std::any();
    }
    Value* boxedVal = boxToTzdValue(initVal);
    Value* stableVal = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxedVal });
    AllocaInst* alloca = CreateEntryBlockAlloca(name);
    m_builder.CreateStore(stableVal, alloca);
    m_namedValues[name] = alloca;
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
    BasicBlock* elseBB = BasicBlock::Create(m_context, "else");
    BasicBlock* mergeBB = BasicBlock::Create(m_context, "ifcont");

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
    visit(ctx->statement());
    if (!m_builder.GetInsertBlock()->getTerminator()) {
        m_builder.CreateBr(condBB);
    }

    m_builder.SetInsertPoint(endBB);
    m_loopStack.pop_back();
    return std::any();
}

std::any TzdCompiler::visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) {
    (void)ctx;
    if (!m_switchEndStack.empty()) {
        m_builder.CreateBr(m_switchEndStack.back());
    }
    else if (!m_loopStack.empty()) {
        m_builder.CreateBr(m_loopStack.back().breakBB);
    }
    return std::any();
}

std::any TzdCompiler::visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) {
    (void)ctx;
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
            // 检查是否是赋值表达式 (i = 0)
            if (auto assignCtx = dynamic_cast<TzdLangParser::AssignmentExprContext*>(expr)) {

                std::string varName = assignCtx->expression(0)->getText();
                auto valExpr = assignCtx->expression(1);

                // 1. 先编译右值 (拿到 Value*)
                Value* initVal = std::any_cast<Value*>(visit(valExpr));

                // 2. [关键判定] 只要右值是原生 double，且变量未定义，就强制提升为原生变量
                if (initVal->getType()->isDoubleTy() &&
                    m_nativeDoubleLocals.find(varName) == m_nativeDoubleLocals.end() &&
                    m_namedValues.find(varName) == m_namedValues.end()) {

                    // 3. 在栈上分配原生内存
                    AllocaInst* alloc = m_builder.CreateAlloca(m_doubleTy, nullptr, varName + "_loop_native");
                    m_builder.CreateStore(initVal, alloc);

                    // 4. 注册到原生表
                    m_nativeDoubleLocals[varName] = alloc;
                    initHandled = true; // 标记已处理
                }
            }
        }
    }
    // -----------------------------------

    // 如果未被优化逻辑处理，则执行默认访问
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
        // 使用之前实现的 toNativeBool 避免装箱
        Value* isTrue = toNativeBool(condValue);
        m_builder.CreateCondBr(isTrue, bodyBB, afterBB);
    }
    else {
        m_builder.CreateBr(bodyBB);
    }

    m_loopStack.push_back({ stepBB, afterBB });

    m_builder.SetInsertPoint(bodyBB);
    if (ctx->statement()) {
        visit(ctx->statement());
    }
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
        Value* val = m_builder.CreateLoad(m_doubleTy, m_nativeDoubleLocals[name], name);
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
    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        Value* res = ctx->PLUS() ? m_builder.CreateFAdd(L, R, "addtmp")
            : m_builder.CreateFSub(L, R, "subtmp");
        return std::any((Value*)res);
    }
    if (L->getType()->isDoubleTy() && R->getType()->isPointerTy()) {
        Value* rNative = m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { R });
        Value* res = ctx->PLUS() ? m_builder.CreateFAdd(L, rNative, "addtmp")
            : m_builder.CreateFSub(L, rNative, "subtmp");
        return std::any((Value*)res);
    }
    if (R->getType()->isDoubleTy() && L->getType()->isPointerTy()) {
        Value* lNative = m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { L });
        Value* res = ctx->PLUS() ? m_builder.CreateFAdd(lNative, R, "addtmp")
            : m_builder.CreateFSub(lNative, R, "subtmp");
        return std::any((Value*)res);
    }
    Value* boxedL = boxToTzdValue(L);
    Value* boxedR = boxToTzdValue(R);
    const char* rtFunc = ctx->PLUS() ? "rt_op_add" : "rt_op_sub";
    Value* res = m_builder.CreateCall(getRtFunc(rtFunc), { boxedL, boxedR });
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
        Value* nameStr = m_builder.CreateGlobalStringPtr(memberCtx->IDENTIFIER()->getText());
        Value* boxedRhs = boxToTzdValue(rhs);
        if (isCompound) {
            Value* oldVal = m_builder.CreateCall(getRtFunc("rt_get_member"), { obj, nameStr });
            const char* opFn = opText == "+=" ? "rt_op_add" :
                opText == "-=" ? "rt_op_sub" :
                opText == "*=" ? "rt_op_mul" : "rt_op_div";
            boxedRhs = m_builder.CreateCall(getRtFunc(opFn), { oldVal, boxedRhs });
        }
        Value* stableRhs = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxedRhs });
        m_builder.CreateCall(getRtFunc("rt_store_member"), { obj, nameStr, stableRhs });
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
        Value* stableRhs = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxedRhs });
        m_builder.CreateCall(getRtFunc("rt_store_member"), { thisPtr, nameStr, stableRhs });
    }
    else {
        // 第一次出现的指针类型变量：直接用 alloca 管理，不写 scope hashmap
        Value* stable = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxedRhs });
        AllocaInst* alloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, name + "_local");
        m_builder.CreateStore(stable, alloc);
        m_namedValues[name] = alloc;
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
    m_builder.CreateCall(getRtFunc("rt_set_location"), {
        m_builder.getInt32(ctx->getStart()->getLine()),
        m_builder.getInt32(ctx->getStart()->getCharPositionInLine())
        });
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
            m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { argPtr, argRaw });
        }
        else {
            m_builder.CreateCall(getRtFunc("rt_copy_value"), { argPtr, boxToTzdValue(argRaw) });
        }
    }

    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_inst_args"), { name, m_builder.getInt32(argCount), argsArray });
}

std::any TzdCompiler::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) {
    Value* obj = std::any_cast<Value*>(visit(ctx->atom()));
    Value* name = m_builder.CreateGlobalStringPtr(ctx->IDENTIFIER()->getText());
    return (Value*)m_builder.CreateCall(getRtFunc("rt_get_member"), { obj, name });
}

std::any TzdCompiler::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    Value* retValRaw = nullptr;
    if (ctx->expression()) {
        bool isCall = (getAsCallExpr(ctx->expression()) != nullptr);
        bool oldTailState = s_inTailPosition;
        s_inTailPosition = isCall;

        retValRaw = std::any_cast<Value*>(visit(ctx->expression()));

        s_inTailPosition = oldTailState;

        if (m_currentRetPtr) {
            if (retValRaw && retValRaw->getType()->isDoubleTy()) {
                // 如果是原生数字，直接写值！拒绝调用 C++ 的 rt_write_fast_ret/rt_create_num
                m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { m_currentRetPtr, retValRaw });
            }
            else {
                Value* boxed = boxToTzdValue(retValRaw);
                Value* stable = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxed });
                m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { m_currentRetPtr, stable });
            }
        }

        Function* currentFunc = m_builder.GetInsertBlock()->getParent();
        std::string currentName = currentFunc->getName().str();

        // 绝不让高频自递归污染和更新全局 g_last_ret
        if (currentName.find("_worker") == std::string::npos) {
            Value* boxed = boxToTzdValue(retValRaw);
            Value* stable = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { boxed });
            m_builder.CreateCall(getRtFunc("rt_set_last_ret"), { stable });
        }
    }

    // [绝杀修改]：如果是 Worker 函数，直接用底层寄存器返回原生数字！
    Function* currentFunc = m_builder.GetInsertBlock()->getParent();
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

    // 路径 A：常量整数索引 → rt_get_index_native_d(container, const_int) → double
    if (auto* constFP = llvm::dyn_cast<llvm::ConstantFP>(index_raw)) {
        int idx = (int)constFP->getValueAPF().convertToDouble();
        return (Value*)m_builder.CreateCall(
            getRtFunc("rt_get_index_native_d"),
            { container, m_builder.getInt32(idx) }
        );
    }

    // 路径 B：native double 变量索引 → rt_get_index_native_d_dyn(container, double) → double
    if (index_raw->getType()->isDoubleTy()) {
        return (Value*)m_builder.CreateCall(
            getRtFunc("rt_get_index_native_d_dyn"),
            { container, index_raw }
        );
    }

    // 路径 C：原有 boxed 路径（字符串 key 等）
    Value* boxedIndex = boxToTzdValue(index_raw);
    return (Value*)m_builder.CreateCall(getRtFunc("rt_get_index"), { container, boxedIndex });
}
std::any TzdCompiler::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) {
    Value* L = std::any_cast<Value*>(visit(ctx->expression(0)));
    Value* R = std::any_cast<Value*>(visit(ctx->expression(1)));

    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        Value* res = nullptr;
        if (ctx->MUL()) res = m_builder.CreateFMul(L, R, "multmp");
        else if (ctx->DIV()) res = m_builder.CreateFDiv(L, R, "divtmp");
        else res = m_builder.CreateFRem(L, R, "modtmp");
        return std::any((Value*)res);
    }

    Value* boxedL = boxToTzdValue(L);
    Value* boxedR = boxToTzdValue(R);
    const char* rtFunc = ctx->MUL() ? "rt_op_mul" :
        ctx->DIV() ? "rt_op_div" : "rt_op_mod";
    Value* res = m_builder.CreateCall(getRtFunc(rtFunc), { boxedL, boxedR });
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
    Value* nameStr = m_builder.CreateGlobalStringPtr(memCtx->IDENTIFIER()->getText());
    Value* oldVal = m_builder.CreateCall(getRtFunc("rt_get_member"), { obj, nameStr });
    Value* one = m_builder.CreateCall(getRtFunc("rt_create_num"), { ConstantFP::get(m_doubleTy, 1.0) });
    const char* opFunc = isInc ? "rt_op_add" : "rt_op_sub";
    Value* newVal = m_builder.CreateCall(getRtFunc(opFunc), { oldVal, one });
    Value* stableNew = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { newVal });
    m_builder.CreateCall(getRtFunc("rt_store_member"), { obj, nameStr, stableNew });
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
        Value* stableNew = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), { newVal });
        m_builder.CreateCall(getRtFunc("rt_store_member"), { thisPtr, nameStr, stableNew });
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

    // 剥离版本号和 _worker 后缀，获取真正的函数原始名字
    std::string baseName = currentName;
    if (currentName.size() > 7 && currentName.substr(currentName.size() - 7) == "_worker") {
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
        return std::any((Value*)ConstantFP::get(m_doubleTy, 0.0));
    }

    // ==============================================================
    // 2. [直接自递归旁路] (调用自身的 Worker，不走 C++ 运行时)
    // ==============================================================
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
                m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { argPtr, argRaw });
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
            m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { argPtr, argRaw });
        }
        else {
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), { argPtr, boxToTzdValue(argRaw) });
        }
    }

    if (!dynamic_cast<TzdLangParser::IdExprContext*>(ctx->atom())) {
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
        fastRes->setTailCallKind(CallInst::TCK_MustTail);
        m_builder.CreateRet(fastRes);

        // -- Slow Path: 回退到普通 C++ 函数 --
        m_builder.SetInsertPoint(slowCallBB);
        Value* slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), { funcNameStr, m_builder.getInt32(argCount), argsArray });
        Value* slowRes = m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { slowResPtr });
        m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), { m_currentRetPtr, slowRes });
        m_builder.CreateRet(slowRes);

        BasicBlock* deadBB = BasicBlock::Create(m_context, "tail_call_unreachable", currentFunc);
        m_builder.SetInsertPoint(deadBB);

        return std::any((Value*)ConstantFP::get(m_doubleTy, 0.0));
    }

    // --- 非尾调用位置，保持原有的 merge 逻辑 ---
    BasicBlock* mergeBB = BasicBlock::Create(m_context, "merge_call", currentFunc);
    m_builder.CreateCondBr(isWorkerValid, fastCallBB, slowCallBB);

    AllocaInst* resDoubleSlot = CreateEntryBlockAlloca(m_doubleTy, nullptr, "dyn_call_res");

    m_builder.SetInsertPoint(fastCallBB);

    // 【新增】：推入当前的 JIT 栈帧
    m_builder.CreateCall(getRtFunc("rt_push_jit_frame"), { funcNameStr });

    std::vector<Type*> workerSignature = { m_ptrTy, m_ptrTy, m_ptrTy };
    FunctionType* workerFTy = FunctionType::get(m_doubleTy, workerSignature, false);
    Value* interp = currentFunc->getArg(0);
    Value* dummyResSlotFast = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
    CallInst* fastRes = m_builder.CreateCall(workerFTy, workerPtr, { interp, dummyResSlotFast, argsArray });
    m_builder.CreateStore(fastRes, resDoubleSlot);

    // 【新增】：执行完毕，弹出 JIT 栈帧
    m_builder.CreateCall(getRtFunc("rt_pop_jit_frame"));

    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(slowCallBB);
    Value* slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), { funcNameStr, m_builder.getInt32(argCount), argsArray });
    Value* slowRes = m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { slowResPtr });
    m_builder.CreateStore(slowRes, resDoubleSlot);
    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(mergeBB);
    Value* finalRes = m_builder.CreateLoad(m_doubleTy, resDoubleSlot, "call_res");
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
    Value* enterRes = m_builder.CreateCall(getRtFunc("rt_enter_try_buf"), { jmpBuf });
    Value* isCatch = m_builder.CreateICmpNE(enterRes, m_builder.getInt32(0));
    m_builder.CreateCondBr(isCatch, catchBB, tryBB);

    m_builder.SetInsertPoint(tryBB);
    visit(ctx->block(0));
    m_builder.CreateCall(getRtFunc("rt_leave_try"));
    m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { jmpBuf });
    m_builder.CreateBr(afterBB);

    m_builder.SetInsertPoint(catchBB);
    m_builder.CreateCall(getRtFunc("rt_leave_try"));
    Value* thrown = m_builder.CreateCall(getRtFunc("rt_get_thrown"));
    Value* catchName = m_builder.CreateGlobalStringPtr(ctx->IDENTIFIER()->getText());
    m_builder.CreateCall(getRtFunc("rt_push_catch_scope"), { catchName, thrown });
    visit(ctx->block(1));
    m_builder.CreateCall(getRtFunc("rt_pop_catch_scope"));
    m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), { jmpBuf });
    m_builder.CreateBr(afterBB);

    m_builder.SetInsertPoint(afterBB);
    return std::any((Value*)ConstantPointerNull::get(cast<PointerType>(m_ptrTy)));
}

std::any TzdCompiler::visitNullExpr(TzdLangParser::NullExprContext* ctx) {
    return (Value*)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) { return visit(ctx->expression()); }
std::any TzdCompiler::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) { return visit(ctx->functionDeclaration()); }

Value* TzdCompiler::castToNativeDouble(Value* val) {
    if (!val) return ConstantFP::get(m_doubleTy, 0.0);

    // 1. 如果已经是 double，直接用
    if (val->getType()->isDoubleTy()) return val;

    // 2. [关键]：如果是 i1 (布尔值)，直接用 LLVM 指令转为 double (0.0 或 1.0)
    // 绝对不能把 i1 当作指针传给 rt_to_double_fast
    if (val->getType()->isIntegerTy(1)) {
        return m_builder.CreateUIToFP(val, m_doubleTy, "bool2double");
    }

    // 3. 只有当它是 TzdValue* 指针时，才调用运行时函数
    return m_builder.CreateCall(getRtFunc("rt_to_double_fast"), { val });
}

llvm::Value* TzdCompiler::boxDouble(llvm::Value* nativeVal) {
    if (nativeVal->getType()->isPointerTy()) return nativeVal;
    return m_builder.CreateCall(getRtFunc("rt_create_num"), { nativeVal });
}
















