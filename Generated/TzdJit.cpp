#include "../Res/TzdStrings.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <csetjmp>
#include <cstddef>
#include <charconv>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <immintrin.h>

#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Metadata.h"
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
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"
#include "llvm/Transforms/Scalar/Float2Int.h"
#include "llvm/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvm/Transforms/Scalar/ConstraintElimination.h"
#include "llvm/Transforms/Scalar/SimpleLoopUnswitch.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Vectorize/LoopVectorize.h"
#include "llvm/Transforms/Vectorize/SLPVectorizer.h"
#include "llvm/Transforms/Vectorize/VectorCombine.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
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
static thread_local llvm::BasicBlock *s_tailRecurseBB = nullptr;
static thread_local bool s_inTailPosition = false;
static thread_local llvm::Function *s_currentWorkerFunc = nullptr;
static thread_local llvm::Function *s_currentNativeWorkerFunc = nullptr;
static thread_local bool s_compilingNativeWorker = false;
static thread_local int s_currentRecursionPeelDepth = 0;
static thread_local std::string s_currentCompilingFuncName;
static thread_local TzdLangParser::BlockContext *s_currentFunctionBlock = nullptr;
static thread_local std::vector<std::string> s_currentFunctionParams;
// Note: Cross-module direct LLVM Function* calls are invalid in LLVM due to per-module LLVMContext isolation.
// Dynamic cross-module calls use runtime function pointers via s_workerPointers and rt_get_worker_ptr.
static std::unordered_map<std::string, void *> s_workerPointers;
static std::shared_mutex s_workerPointersMutex;
static thread_local std::vector<std::tuple<llvm::Value *, llvm::Value *, int>> s_tryJmpBufStack;
static thread_local int s_loopLevel = 0;

// ======= JIT 全局配置与内联/调试接口 =======
static TzdJitConfig s_jitConfig;
static std::vector<JittedFunctionInfo> s_registeredJitFunctions;
static std::unordered_map<std::string, JittedFunctionInfo> s_registeredJitMap;
static std::shared_mutex s_jitRegistryMutex;
static std::atomic<size_t> s_totalInlinedCalls{0};
static std::atomic<bool> s_jitDebugHookActive{false};

static inline bool jitTrackFrames()
{
    return TzdDebugger::g_DebugActive || s_jitDebugHookActive.load(std::memory_order_relaxed);
}

struct InlineReturnTarget
{
    llvm::BasicBlock *returnBB = nullptr;
    llvm::AllocaInst *retNativeDouble = nullptr;
    llvm::AllocaInst *retBoxed = nullptr;
    bool expectsDouble = false;
};
static thread_local std::vector<InlineReturnTarget> s_inlineReturnStack;
static thread_local std::unordered_set<std::string> s_inlinedFunctionsInStack;
static thread_local int s_currentInlineDepth = 0;
static thread_local size_t s_inlineUid = 0;

TzdJitConfig &TzdJitEngine::getConfig() { return s_jitConfig; }

void TzdJitEngine::setOptLevel(int level)
{
    s_jitConfig.optLevel = std::clamp(level, 0, 3);
    if (s_jitConfig.optLevel == 0)
    {
        s_jitConfig.enableAstInlining = false;
        s_jitConfig.inlineThreshold = 0;
        s_jitConfig.enableLoopUnroll = false;
    }
    else if (s_jitConfig.optLevel == 1)
    {
        s_jitConfig.enableAstInlining = true;
        s_jitConfig.inlineThreshold = 100;
        s_jitConfig.maxInlineStmts = 10;
        s_jitConfig.maxInlineDepth = 2;
        s_jitConfig.enableLoopUnroll = false;
    }
    else if (s_jitConfig.optLevel == 2)
    {
        s_jitConfig.enableAstInlining = true;
        s_jitConfig.inlineThreshold = 250;
        s_jitConfig.maxInlineStmts = 25;
        s_jitConfig.maxInlineDepth = 4;
        s_jitConfig.enableLoopUnroll = true;
    }
    else
    { // 3
        s_jitConfig.enableAstInlining = true;
        s_jitConfig.inlineThreshold = 500;
        s_jitConfig.maxInlineStmts = 60;
        s_jitConfig.maxInlineDepth = 8;
        s_jitConfig.enableLoopUnroll = true;
    }
}

int TzdJitEngine::getOptLevel() { return s_jitConfig.optLevel; }
void TzdJitEngine::setInlineThreshold(int threshold) { s_jitConfig.inlineThreshold = threshold; }
int TzdJitEngine::getInlineThreshold() { return s_jitConfig.inlineThreshold; }
void TzdJitEngine::setJitDebugEnabled(bool enabled)
{
    s_jitConfig.enableJitDebug = enabled;
    s_jitDebugHookActive.store(enabled, std::memory_order_relaxed);
}
bool TzdJitEngine::isJitDebugEnabled() { return s_jitConfig.enableJitDebug; }
void TzdJitEngine::setAstInliningEnabled(bool enabled) { s_jitConfig.enableAstInlining = enabled; }
bool TzdJitEngine::isAstInliningEnabled() { return s_jitConfig.enableAstInlining; }

void TzdJitEngine::registerJittedFunction(const JittedFunctionInfo &info)
{
    std::unique_lock<std::shared_mutex> lock(s_jitRegistryMutex);
    s_registeredJitMap[info.name] = info;
    s_registeredJitMap[info.internalName] = info;
    s_registeredJitFunctions.push_back(info);
}

std::vector<JittedFunctionInfo> TzdJitEngine::getJittedFunctions()
{
    std::shared_lock<std::shared_mutex> lock(s_jitRegistryMutex);
    return s_registeredJitFunctions;
}

JittedFunctionInfo *TzdJitEngine::getJittedFunction(const std::string &name)
{
    std::shared_lock<std::shared_mutex> lock(s_jitRegistryMutex);
    auto it = s_registeredJitMap.find(name);
    if (it != s_registeredJitMap.end())
        return &it->second;
    return nullptr;
}

std::string TzdJitEngine::dumpJitIR(const std::string &name)
{
    std::shared_lock<std::shared_mutex> lock(s_jitRegistryMutex);
    auto it = s_registeredJitMap.find(name);
    if (it != s_registeredJitMap.end() && !it->second.irDump.empty())
    {
        return it->second.irDump;
    }
    for (const auto &[k, info] : s_registeredJitMap)
    {
        if ((k.find(name) != std::string::npos || info.name == name || info.internalName == name) && !info.irDump.empty())
        {
            return info.irDump;
        }
    }
    return "; No IR dump available for " + name + "\n";
}

size_t TzdJitEngine::getJitCompiledCount()
{
    std::shared_lock<std::shared_mutex> lock(s_jitRegistryMutex);
    return s_registeredJitFunctions.size();
}

size_t TzdJitEngine::getTotalInlinedCalls()
{
    return s_totalInlinedCalls.load(std::memory_order_relaxed);
}

void TzdJitEngine::recordInlinedCall()
{
    s_totalInlinedCalls.fetch_add(1, std::memory_order_relaxed);
}

std::string formatSourcePath(const std::string &fullPath);
std::string unescapeString(const std::string &input);

// #region agent log
static void agentLogJit(const char *hypothesisId, const char *location, const char *detail)
{
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f)
        return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
      << "\",\"location\":\"" << location << "\",\"message\":\"jit visit\",\"data\":{\"detail\":\""
      << detail << "\"},\"timestamp\":" << ts << "}\n";
}

static Value *castAnyToValue(const std::any &a, const char *where)
{
    if (!a.has_value())
    {
        agentLogJit("D", where, "empty any");
        throw std::bad_any_cast();
    }
    if (a.type() != typeid(Value *))
    {
        agentLogJit("D", where, a.type().name());
        throw std::bad_any_cast();
    }
    return std::any_cast<Value *>(a);
}

// TzdValue field offsets for inlined GEP+Store/Load (eliminates rt_ function calls)
static constexpr size_t TZD_TYPE_OFFSET = offsetof(TzdValue, type);
static constexpr size_t TZD_DVAL_OFFSET = offsetof(TzdValue, dVal);
static_assert(TZD_TYPE_OFFSET > 0, "type offset must be > 0 (after annotations vector)");

static std::string getParamName(TzdLangParser::ParamContext *p)
{
    if (!p)
        return "arg";
    if (p->IDENTIFIER())
        return p->IDENTIFIER()->getText();
    if (p->T_FUNCTION())
        return p->T_FUNCTION()->getText();
    if (p->T_INT())
        return p->T_INT()->getText();
    if (p->T_STRING())
        return p->T_STRING()->getText();
    if (p->T_FLOAT())
        return p->T_FLOAT()->getText();
    if (p->T_BOOL())
        return p->T_BOOL()->getText();
    if (p->T_VOID())
        return p->T_VOID()->getText();
    if (p->T_PTR())
        return p->T_PTR()->getText();
    if (p->KW_RET())
        return p->KW_RET()->getText();
    return p->getText();
}

static std::string getParamType(TzdLangParser::ParamContext *p)
{
    if (!p || !p->typeType())
        return "";
    return p->typeType()->getText();
}

static bool isNonNumericParam(TzdLangParser::ParamContext *p)
{
    if (!p)
        return false;
    if (p->T_FUNCTION())
        return true;
    if (p->T_STRING())
        return true;
    if (p->T_PTR())
        return true;
    if (p->T_VOID())
        return true;
    if (p->typeType())
    {
        std::string t = p->typeType()->getText();
        if (t == "function" || t == "fn" || t == "string" || t == "ptr" ||
            t == "pointer" || t == "void" || t.find("[]") != std::string::npos)
        {
            return true;
        }
        if (t != "int" && t != "float" && t != "double" && t != "number" && t != "bool")
        {
            return true;
        }
    }
    return false;
}

static bool isExplicitNumericType(const std::string &t)
{
    return (t == "int" || t == "float" || t == "double" || t == "number" ||
            t == "i32" || t == "i64" || t == "long" || t == "short" ||
            t == "byte" || t == "sbyte" || t == "uint" || t == "ulong" ||
            t == "ushort" || t == "char");
}

static bool isExplicitNumericParam(TzdLangParser::ParamContext *p)
{
    if (!p)
        return false;
    if (p->typeType())
    {
        return isExplicitNumericType(p->typeType()->getText());
    }
    return false;
}

static bool isParamNonDouble(antlr4::tree::ParseTree *tree, const std::string &paramName)
{
    if (!tree)
        return false;

    // 1. 作为容器或对象使用: param[...], param.xxx, param(...)
    if (auto idx = dynamic_cast<TzdLangParser::IndexExprContext *>(tree))
    {
        if (idx->expression(0) && idx->expression(0)->getText() == paramName)
            return true;
    }
    if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(tree))
    {
        if (mem->atom() && mem->atom()->getText() == paramName)
            return true;
    }
    if (auto call = dynamic_cast<TzdLangParser::CallExprContext *>(tree))
    {
        if (call->atom() && call->atom()->getText() == paramName)
            return true;
    }

    // 2. 作为 super(...) 或 new Cls(...) 的实参
    if (auto sup = dynamic_cast<TzdLangParser::SuperExprContext *>(tree))
    {
        if (sup->exprList())
        {
            for (auto expr : sup->exprList()->expression())
            {
                if (expr->getText() == paramName)
                    return true;
            }
        }
    }
    if (auto nw = dynamic_cast<TzdLangParser::NewExprContext *>(tree))
    {
        if (nw->exprList())
        {
            for (auto expr : nw->exprList()->expression())
            {
                if (expr->getText() == paramName)
                    return true;
            }
        }
    }

    // 3. 赋值给对象成员或容器元素 — REMOVED: being the RHS of a[idx]=v or obj.f=v
    // does NOT make the param non-double. The store path handles both double and boxed
    // RHS (rt_store_index_native_d_dyn for doubles, rt_store_index_val_dyn for boxed).
    // Classifying a numeric RHS as non-double caused it to be boxed via inlineStoreNativeToPtr
    // which wrote type=LONG/dVal=0 (probe4 bug: array-param-size-0).

    // 4. 与 null 比较
    if (auto eq = dynamic_cast<TzdLangParser::EqualityExprContext *>(tree))
    {
        std::string lText = eq->expression(0) ? eq->expression(0)->getText() : "";
        std::string rText = eq->expression(1) ? eq->expression(1)->getText() : "";
        if ((lText == paramName && rText == "null") || (rText == paramName && lText == "null"))
            return true;
    }

    // 5. 参与字符串拼接
    if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(tree))
    {
        std::string lText = add->expression(0) ? add->expression(0)->getText() : "";
        std::string rText = add->expression(1) ? add->expression(1)->getText() : "";
        bool hasStr = (lText.size() >= 2 && (lText.front() == '"' || lText.front() == '\'')) ||
                      (rText.size() >= 2 && (rText.front() == '"' || rText.front() == '\''));
        if (hasStr && (lText == paramName || rText == paramName))
            return true;
    }

    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (isParamNonDouble(tree->children[i], paramName))
            return true;
    }
    return false;
}

static bool isParamUsedAsContainer(antlr4::tree::ParseTree *tree, const std::string &paramName)
{
    return isParamNonDouble(tree, paramName);
}

// 辅助函数：判断 AST 是否为纯数值计算（无对象、数组索引、成员访问、非纯外部调用）
static bool isTreePureNumeric(antlr4::tree::ParseTree *tree, const std::string &selfFuncName)
{
    if (!tree)
        return true;
    if (dynamic_cast<TzdLangParser::IndexExprContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::MemberAccessExprContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::NewExprContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::SuperExprContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::ThrowStmtContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::TryCatchStmtContext *>(tree))
        return false;

    if (auto call = dynamic_cast<TzdLangParser::CallExprContext *>(tree))
    {
        if (!call->atom())
            return false;
        std::string callee = call->atom()->getText();
        static const std::unordered_set<std::string> s_pureMath = {
            "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
            "sinh", "cosh", "tanh", "exp", "log", "log10", "log2",
            "sqrt", "cbrt", "ceil", "floor", "round", "trunc", "abs",
            "pow", "fmod", "hypot", "min", "max", "clamp"};
        if (callee != selfFuncName && s_pureMath.find(callee) == s_pureMath.end())
        {
            return false;
        }
    }

    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (!isTreePureNumeric(tree->children[i], selfFuncName))
            return false;
    }
    return true;
}
// #endregion

static bool isBlockReturningDouble(antlr4::tree::ParseTree *block);

// 辅助函数：判断表达式是否明确为非纯数值类型（例如对象、this、字符串、容器、布尔等）
static bool isExprNonDouble(antlr4::tree::ParseTree *tree, antlr4::tree::ParseTree *block = nullptr)
{
    if (!tree)
        return false;

    if (auto paren = dynamic_cast<TzdLangParser::ParenExprContext *>(tree))
    {
        return isExprNonDouble(paren->expression(), block);
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(tree))
    {
        return isExprNonDouble(atomExpr->atom(), block);
    }
    if (tree->children.size() == 1)
    {
        return isExprNonDouble(tree->children[0], block);
    }

    if (dynamic_cast<TzdLangParser::StringExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::BoolTrueExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::BoolFalseExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::NullExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::NewExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::ArrayLiteralExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::MapLiteralExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::SuperExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::LambdaExprContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::PrintFunExprContext *>(tree))
        return true;
    if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(tree))
    {
        std::string field = mem->IDENTIFIER() ? mem->IDENTIFIER()->getText() : "";
        for (const auto &[cName, cls] : TzdOopManager::getClasses())
        {
            if (cls)
            {
                if (ClassField *f = cls->findField(field))
                {
                    if (f->type == "float" || f->type == "double" || f->type == "int" || f->type == "number" || f->type.empty())
                        return false;
                }
            }
        }
        return true;
    }
    if (dynamic_cast<TzdLangParser::IndexExprContext *>(tree))
        return true;

    std::string text = tree->getText();
    if (text == "this" || text == "null" || text == "true" || text == "false")
        return true;
    if (text.size() >= 2 && (text.front() == '"' || text.front() == '\''))
        return true;
    if (text.rfind("new", 0) == 0 && text.find('(') != std::string::npos)
        return true;

    if (auto call = dynamic_cast<TzdLangParser::CallExprContext *>(tree))
    {
        std::string callee = call->atom() ? call->atom()->getText() : "";
        static const std::unordered_set<std::string> mathFuncs = {
            "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
            "sinh", "cosh", "tanh", "exp", "log", "log10", "log2",
            "sqrt", "cbrt", "ceil", "floor", "round", "trunc", "abs",
            "pow", "fmod", "hypot", "min", "max", "clamp"};
        if (mathFuncs.find(callee) != mathFuncs.end())
        {
            return false;
        }
        if (!s_currentCompilingFuncName.empty() && callee == s_currentCompilingFuncName)
        {
            return false; // 递归自调用纯数值函数视为 double 返回
        }
        if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(call->atom()))
        {
            std::string methodName = mem->IDENTIFIER() ? mem->IDENTIFIER()->getText() : "";
            for (const auto &[cName, cls] : TzdOopManager::getClasses())
            {
                if (cls)
                {
                    if (ClassMethod *m = cls->findMethod(methodName))
                    {
                        if (m->body && m->body != block && isBlockReturningDouble(m->body))
                        {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    if (block && !text.empty())
    {
        if (TzdOopManager::getClass(text))
            return true;
        if (isParamNonDouble(block, text))
            return true;

        bool isDeclNonDouble = false;
        std::function<void(antlr4::tree::ParseTree *)> checkDecls = [&](antlr4::tree::ParseTree *node)
        {
            if (!node || isDeclNonDouble)
                return;
            if (auto varDecl = dynamic_cast<TzdLangParser::VariableDeclarationContext *>(node))
            {
                if (varDecl->IDENTIFIER() && varDecl->IDENTIFIER()->getText() == text)
                {
                    if (varDecl->typeType())
                    {
                        std::string t = varDecl->typeType()->getText();
                        if (t != "int" && t != "float" && t != "double" && t != "number" && t != "i32" && t != "i64")
                        {
                            isDeclNonDouble = true;
                            return;
                        }
                    }
                    if (varDecl->expression() && isExprNonDouble(varDecl->expression(), block))
                    {
                        isDeclNonDouble = true;
                        return;
                    }
                }
            }
            for (size_t i = 0; i < node->children.size(); ++i)
            {
                checkDecls(node->children[i]);
            }
        };
        checkDecls(block);
        if (isDeclNonDouble)
            return true;
    }

    return false;
}

// 辅助函数：判断代码块是否明确且仅返回数值（double/float/int）
// 如果无 return 语句（返回 void/null），或存在返回非数值（如 this、对象、字符串等），返回 false
static bool isBlockReturningDouble(antlr4::tree::ParseTree *block)
{
    if (!block)
        return false;

    std::vector<TzdLangParser::ReturnStmtContext *> returns;
    std::function<void(antlr4::tree::ParseTree *)> findReturns = [&](antlr4::tree::ParseTree *node)
    {
        if (!node)
            return;
        if (auto ret = dynamic_cast<TzdLangParser::ReturnStmtContext *>(node))
        {
            returns.push_back(ret);
            return;
        }
        if (dynamic_cast<TzdLangParser::FunctionDeclarationContext *>(node))
            return;
        if (dynamic_cast<TzdLangParser::ClassDeclarationContext *>(node))
            return;
        if (dynamic_cast<TzdLangParser::LambdaExprContext *>(node))
            return;

        for (size_t i = 0; i < node->children.size(); ++i)
        {
            findReturns(node->children[i]);
        }
    };
    findReturns(block);

    if (returns.empty())
        return false;

    for (auto *ret : returns)
    {
        if (!ret->expression())
            return false;
        if (isExprNonDouble(ret->expression(), block))
            return false;
    }

    return true;
}

// 辅助函数：递归解包 AST，判断 Return 后面是否干净地跟着一个函数调用
static TzdLangParser::CallExprContext *getAsCallExpr(antlr4::tree::ParseTree *node)
{
    if (!node)
        return nullptr;
    if (auto call = dynamic_cast<TzdLangParser::CallExprContext *>(node))
        return call;
    if (node->children.size() == 1)
        return getAsCallExpr(node->children[0]);
    return nullptr;
}

// 辅助函数：递归提取 new 表达式对应的类名 (例如 new MathBench() -> "MathBench")
static std::string getNewExprClassName(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return "";
    if (auto paren = dynamic_cast<TzdLangParser::ParenExprContext *>(tree))
    {
        return getNewExprClassName(paren->expression());
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(tree))
    {
        return getNewExprClassName(atomExpr->atom());
    }
    if (auto newExpr = dynamic_cast<TzdLangParser::NewExprContext *>(tree))
    {
        if (newExpr->qualifiedName())
        {
            return newExpr->qualifiedName()->getText();
        }
    }
    return "";
}

static std::vector<std::string> resolveConstructorFields(TzdClassDef *def, int argCount)
{
    std::vector<std::string> fields;
    if (!def)
        return fields;

    const ClassConstructor *matchedCtor = def->findConstructor(argCount);

    if (matchedCtor && matchedCtor->body)
    {
        if (matchedCtor->body->statement().size() != (size_t)argCount)
        {
            return {};
        }

        std::map<int, std::string> paramToField;
        for (auto stmt : matchedCtor->body->statement())
        {
            if (auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(stmt))
            {
                if (auto assignExpr = dynamic_cast<TzdLangParser::AssignmentExprContext *>(exprStmt->expression()))
                {
                    std::string lhs = assignExpr->expression(0)->getText();
                    std::string rhs = assignExpr->expression(1)->getText();
                    std::string fName;
                    if (lhs.rfind("this.", 0) == 0)
                        fName = lhs.substr(5);
                    else if (def->findField(lhs))
                        fName = lhs;

                    if (!fName.empty())
                    {
                        for (size_t pIdx = 0; pIdx < matchedCtor->params.size(); ++pIdx)
                        {
                            if (matchedCtor->params[pIdx] == rhs)
                            {
                                paramToField[(int)pIdx] = fName;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if ((int)paramToField.size() == argCount)
        {
            fields.resize(argCount);
            for (int i = 0; i < argCount; ++i)
                fields[i] = paramToField[i];
            return fields;
        }
        return {};
    }

    if (matchedCtor)
    {
        return {};
    }

    if (def->constructors.empty() && (int)def->fieldIndices.size() >= argCount)
    {
        for (int i = 0; i < argCount; ++i)
        {
            fields.push_back(def->fieldIndices[i].first);
        }
        return fields;
    }

    return {};
}

static std::string deduceExprClassName(antlr4::tree::ParseTree *tree,
                                       const std::unordered_map<std::string, std::string> &varTypes,
                                       TzdClassDef *currentDef)
{
    if (!tree)
        return "";
    std::string newCls = getNewExprClassName(tree);
    if (!newCls.empty())
        return newCls;
    std::string text = tree->getText();
    auto it = varTypes.find(text);
    if (it != varTypes.end())
        return it->second;
    if (text == "this" && currentDef)
        return currentDef->simpleName;
    size_t dot = text.find('.');
    if (dot != std::string::npos)
    {
        std::string root = text.substr(0, dot);
        std::string sub = text.substr(dot + 1);
        std::string rootCls = "";
        if (root == "this" && currentDef)
            rootCls = currentDef->simpleName;
        else
        {
            auto rIt = varTypes.find(root);
            if (rIt != varTypes.end())
                rootCls = rIt->second;
        }
        if (!rootCls.empty())
        {
            TzdClassDef *pDef = TzdOopManager::getClass(rootCls);
            if (pDef)
            {
                ClassField *f = pDef->findField(sub);
                if (f && !f->type.empty())
                    return f->type;
            }
        }
    }
    return "";
}

static std::string build_jit_frame_string(const char *name)
{
    if (!name)
        return "";
    thread_local std::unordered_map<std::string, std::string> s_jitFrameCache;
    auto it = s_jitFrameCache.find(name);
    if (it != s_jitFrameCache.end())
    {
        return it->second;
    }

    std::string fileLoc = "memory";
    int line = 0;
    std::string frameName = std::string(name);

    std::string fullName(name);
    bool found = false;

    // 1. 尝试解析类方法或类构造函数的符号形式 (例如 ClassName_MethodName 或 ClassName_CtorName_ctor_N)
    size_t firstUnderscore = fullName.find('_');
    if (firstUnderscore != std::string::npos)
    {
        std::string className = fullName.substr(0, firstUnderscore);
        TzdClassDef *cls = TzdOopManager::getClass(className);
        if (cls)
        {
            std::string remaining = fullName.substr(firstUnderscore + 1);
            size_t ctorPos = remaining.find("_ctor_");
            if (ctorPos != std::string::npos)
            {
                // 构造函数
                int paramCount = 0;
                try
                {
                    paramCount = std::stoi(remaining.substr(ctorPos + 6));
                }
                catch (...)
                {
                }
                for (const auto &ctor : cls->constructors)
                {
                    if (ctor.paramCount == paramCount)
                    {
                        fileLoc = formatSourcePath(ctor.sourceFile);
                        line = ctor.line;
                        frameName = cls->fullName + "." + cls->simpleName;
                        found = true;
                        break;
                    }
                }
            }
            else
            {
                // 普通类方法 (剥离 JIT 版本号后缀如 _v1)
                std::string methodName = remaining;
                size_t vPos = methodName.rfind("_v");
                if (vPos != std::string::npos && vPos + 2 < methodName.size() && std::isdigit(methodName[vPos + 2]))
                {
                    methodName = methodName.substr(0, vPos);
                }

                ClassMethod *m = cls->findMethod(methodName);
                if (m)
                {
                    fileLoc = formatSourcePath(m->sourceFile);
                    line = m->line;
                    frameName = cls->fullName + "." + methodName;
                    found = true;
                }
            }
        }
    }

    // 2. 如果不是类成员，则作为全局普通函数从解释器作用域查找
    if (!found && g_CurrentInterpreter)
    {
        try
        {
            TzdValue func = g_CurrentInterpreter->getVariable(name, nullptr);
            if (func.type == TzdValue::FUNCTION || func.type == TzdValue::NATIVE_FUNCTION)
            {
                fileLoc = formatSourcePath(func.sourceFile);
                line = func.line;
                frameName = func.name.empty() ? name : func.name;
                found = true;
            }
        }
        catch (...)
        {
        }

        if (!found)
        {
            std::string rawName(name);
            size_t vPos = rawName.rfind("_v");
            if (vPos != std::string::npos && vPos + 2 < rawName.size() && std::isdigit(rawName[vPos + 2]))
            {
                std::string base = rawName.substr(0, vPos);
                try
                {
                    TzdValue func = g_CurrentInterpreter->getVariable(base, nullptr);
                    if (func.type == TzdValue::FUNCTION || func.type == TzdValue::NATIVE_FUNCTION)
                    {
                        fileLoc = formatSourcePath(func.sourceFile);
                        line = func.line;
                        frameName = func.name.empty() ? base : func.name;
                        found = true;
                    }
                }
                catch (...)
                {
                }
            }
        }
    }

    std::string formattedFrame = frameName + " (" + fileLoc;
    if (line > 0)
        formattedFrame += ":" + std::to_string(line);
    formattedFrame += ") (JIT Compiled)";
    s_jitFrameCache[name] = formattedFrame;
    return formattedFrame;
}

enum VariableType
{
    JIT_VAR_NONE,   // 未定义/空
    JIT_VAR_LOCAL,  // 局部变量（位于栈上，LLVM Alloca）
    JIT_VAR_GLOBAL, // 全局变量（位于运行时 Hashmap）
    JIT_VAR_MEMBER, // 类成员（通过 this 指针访问）
    JIT_VAR_NATIVE  // 原生绑定变量
};

struct VariableInfo
{
    VariableType type;
    llvm::Value *address;
};

// tzdInternSelector(name) and tzdGetSelectorName(sel) are centrally defined in TzdOop.cpp.

static std::unordered_map<std::string, TzdValue *> s_internedStrings;
static std::unordered_set<const void *> s_internedSet;
static std::mutex s_internMtx;

static TzdValue *internStringLiteral(const std::string &str)
{
    std::lock_guard<std::mutex> lock(s_internMtx);
    auto it = s_internedStrings.find(str);
    if (it != s_internedStrings.end())
    {
        return it->second;
    }
    TzdValue *v = new TzdValue();
    v->type = TzdValue::STRING;
    v->sVal = str;
    s_internedStrings[str] = v;
    s_internedSet.insert(v);
    return v;
}

static inline bool isInternedLiteral(const void *ptr)
{
    std::lock_guard<std::mutex> lock(s_internMtx);
    return s_internedSet.count(ptr) > 0;
}

extern "C"
{
    TzdValue *g_LastJitValue = nullptr;
    static thread_local jmp_buf *g_tzdFatalJmp = nullptr;
    static thread_local jmp_buf *g_tzdCatchJmp = nullptr;

    inline TzdValue *make_double(double d)
    {
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::DOUBLE;
        v->dVal = d;
        return v;
    }

    void *rt_get_worker_ptr(const char *funcName)
    {
        if (!funcName)
            return nullptr;
        {
            std::shared_lock<std::shared_mutex> lock(s_workerPointersMutex);
            auto it = s_workerPointers.find(funcName);
            if (it != s_workerPointers.end())
            {
                return it->second;
            }
        }
        if (g_CurrentInterpreter)
        {
            for (auto scopeIt = g_CurrentInterpreter->scopes.rbegin(); scopeIt != g_CurrentInterpreter->scopes.rend(); ++scopeIt)
            {
                auto it = scopeIt->find(funcName);
                if (it != scopeIt->end() && it->second.type == TzdValue::FUNCTION)
                {
                    void *worker = nullptr;
                    if (!it->second.jitInternalName.empty())
                    {
                        std::shared_lock<std::shared_mutex> lock(s_workerPointersMutex);
                        auto wit = s_workerPointers.find(it->second.jitInternalName);
                        if (wit != s_workerPointers.end())
                        {
                            worker = wit->second;
                        }
                    }
                    if (!worker && !it->second.jitInternalName.empty() && g_CurrentInterpreter->m_jitEngine)
                    {
                        std::string workerName = it->second.jitInternalName + "_worker";
                        auto symOrErr = g_CurrentInterpreter->m_jitEngine->lookupSymbol(workerName);
                        if (symOrErr)
                        {
                            worker = reinterpret_cast<void *>(symOrErr->getValue());
                        }
                        else
                        {
                            llvm::consumeError(symOrErr.takeError());
                        }
                    }
                    if (worker)
                    {
                        std::unique_lock<std::shared_mutex> ulock(s_workerPointersMutex);
                        s_workerPointers[funcName] = worker;
                        return worker;
                    }
                }
            }
        }
        return nullptr;
    }

    inline TzdValue *make_bool(bool b)
    {
        static TzdValue s_trueVal(true);
        static TzdValue s_falseVal(false);
        return b ? &s_trueVal : &s_falseVal;
    }

    void *rt_resolve_var(const char *name)
    {
        if (!g_CurrentInterpreter)
            return g_JitPool.next();

        TzdValue *v = g_JitPool.next();
        try
        {
            *v = g_CurrentInterpreter->getVariable(name, nullptr);
            return v;
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError(e.what());
            }
            v->type = TzdValue::NONE;
            return v;
        }
        catch (...)
        {
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("未定义的标识符: '" + std::string(name ? name : "unknown") + "'");
            }
            v->type = TzdValue::NONE;
            return v;
        }
    }

    void *rt_get_var_ptr(const char *name, int line, int col)
    {
        if (!g_CurrentInterpreter)
            return nullptr;

        struct Entry
        {
            const char *key;
            void *ptr;
            size_t depth;
            const void *base;
        };
        static thread_local Entry s_cache[16];
        auto &scopesRef = g_CurrentInterpreter->scopes;
        const void *base = scopesRef.empty() ? nullptr : (const void *)&scopesRef.front();
        const size_t depth = scopesRef.size();
        Entry &e = s_cache[(((uintptr_t)name) >> 4) & 15];
        if (e.key == name && e.depth == depth && e.base == base && e.ptr)
            return e.ptr;

        // 1. 查找局部/全局作用域
        for (auto it = scopesRef.rbegin(); it != scopesRef.rend(); ++it)
        {
            auto scope_it = it->find(name);
            if (scope_it != it->end())
            {
                e = {name, &(scope_it->second), depth, base};
                return e.ptr;
            }
        }

        // 2. 查找 this 实例成员
        for (auto it = g_CurrentInterpreter->scopes.rbegin(); it != g_CurrentInterpreter->scopes.rend(); ++it)
        {
            auto thisIt = it->find("this");
            if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE)
            {
                TzdInstance *inst = thisIt->second.instanceVal;
                if (inst)
                {
                    if (TzdValue *ptr = inst->getMemberPtr(name))
                    {
                        return ptr;
                    }
                    try
                    {
                        TzdValue mem = inst->getMember(name);
                        TzdValue *v = g_JitPool.next();
                        *v = mem;
                        return v;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        // 3. 查找 ClassDef
        TzdClassDef *cls = TzdOopManager::getClass(name);
        if (cls)
        {
            TzdValue *v = g_JitPool.next();
            *v = TzdValue(cls);
            return v;
        }

        // 4. 检查 worker pointers (多线程 JIT 函数)
        {
            std::shared_lock<std::shared_mutex> wlock(s_workerPointersMutex);
            auto wit = s_workerPointers.find(name);
            if (wit != s_workerPointers.end())
            {
                TzdValue *v = g_JitPool.next();
                v->type = TzdValue::FUNCTION;
                v->name = name;
                v->jittedPtr = reinterpret_cast<void (*)(void *, void *)>(wit->second);
                return v;
            }
        }

        // 5. 未定义标识符，上报运行时错误
        if (!g_CurrentInterpreter->m_hasJitError)
        {
            g_CurrentInterpreter->reportJitError("未定义的标识符: '" + std::string(name ? name : "unknown") + "'", line, col);
        }
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::NONE;
        return v;
    }

    void rt_construct_num_at(void *dest, double val)
    {
        // 使用 placement new 在 dest 处初始化对象，不触发赋值运算符
        TzdValue *v = new (dest) TzdValue();
        v->type = TzdValue::DOUBLE;
        v->dVal = val;
    }

    void rt_init_tzd_value(void *ptr, int count)
    {
        if (!ptr)
            return;
        TzdValue *arr = (TzdValue *)ptr;
        for (int i = 0; i < count; ++i)
        {
            new (&arr[i]) TzdValue();
        }
    }

    void rt_write_fast_ret(void *dest, void *src)
    {
        if (!dest || !src || dest == src)
            return;
        TzdValue *d = (TzdValue *)dest;
        TzdValue *s = (TzdValue *)src;
        if (s->isNativeDoubleArr && !s->nativeArr.empty())
        {
            *d = std::move(*s);
            d->syncFastBuf();
            return;
        }
        *d = *s;
        d->syncFastBuf();
    }

    void rt_construct_copy_at(void *dest, void *src)
    {
        if (!dest || !src)
            return;
        // 使用 placement new 调用拷贝构造函数
        new (dest) TzdValue(*(TzdValue *)src);
    }

    void rt_destruct_values(void *ptr, int count)
    {
        if (!ptr || count <= 0)
            return;
        TzdValue *arr = (TzdValue *)ptr;
        for (int i = 0; i < count; ++i)
        {
            arr[i].~TzdValue();
            new (&arr[i]) TzdValue();
        }
    }

    bool rt_tzd_check_var_type(const char *name, const char *expectedType, void *valPtr)
    {
        if (!name || !expectedType || !valPtr)
            return true;
        TzdValue *v = (TzdValue *)valPtr;
        TzdValue converted;
        if (!isTzdValueCompatibleWithType(expectedType, *v, &converted))
        {
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError(
                    "类型不匹配: 无法将 " + getTzdValueTypeName(*v) + " 类型的值赋给 " +
                    std::string(expectedType) + " 类型的变量 '" + std::string(name) + "'");
            }
            if (g_tzdCatchJmp)
            {
                longjmp(*g_tzdCatchJmp, 1);
            }
            return false;
        }
        *v = converted;
        return true;
    }

    void rt_store_var(const char *name, void *val)
    {
        if (g_CurrentInterpreter && val)
        {
            try
            {
                g_CurrentInterpreter->setVariable(name, *(TzdValue *)val);
            }
            catch (const std::exception &e)
            {
                if (!g_CurrentInterpreter->m_hasJitError)
                    g_CurrentInterpreter->reportJitError(e.what());
                if (g_tzdCatchJmp)
                    longjmp(*g_tzdCatchJmp, 1);
            }
        }
    }

    void *rt_get_arg(int index)
    {
        if (!g_CurrentInterpreter)
            return g_JitPool.next();
        if (!g_CurrentInterpreter->m_callFrameStack.empty())
        {
            const auto &frame = g_CurrentInterpreter->m_callFrameStack.back();
            if (frame.thisPtr)
            {
                if (index == 0)
                    return frame.thisPtr;
                if (frame.args && (index - 1) < frame.argCount)
                    return &frame.args[index - 1];
            }
            else
            {
                if (frame.args && index < frame.argCount)
                    return &frame.args[index];
            }
        }
        return g_JitPool.next();
    }

    void rt_set_null(void *dest)
    {
        ((TzdValue *)dest)->type = TzdValue::NONE;
    }

    void *rt_tzd_call_method(void *objPtr, int32_t selector, const char *name, int argCount, TzdValue *args);

    // Cached method frame string — avoids per-call string concatenation/heap allocation.
    static const std::string &getCachedMethodFrame(const std::string &fullName, const char *name,
                                                   const std::string &sourceFile, int line)
    {
        thread_local std::unordered_map<std::string, std::string> s_cache;
        std::string key = fullName + "." + (name ? name : "");
        auto it = s_cache.find(key);
        if (it != s_cache.end())
            return it->second;
        std::string fileLoc = formatSourcePath(sourceFile);
        std::string frameName = fullName + "." + (name ? name : "unknown") + " (" + fileLoc;
        if (line > 0)
            frameName += ":" + std::to_string(line);
        frameName += ") (JIT Compiled)";
        auto [ins, _] = s_cache.emplace(key, std::move(frameName));
        return ins->second;
    }

    void *rt_call_sub_fast(const char *funcName, int argCount, void *args)
    {
        if (!g_CurrentInterpreter)
            return g_JitPool.next();
        if (g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        TzdValue *funcObjPtr = nullptr;
        for (auto scopeIt = g_CurrentInterpreter->scopes.rbegin(); scopeIt != g_CurrentInterpreter->scopes.rend(); ++scopeIt)
        {
            auto it = scopeIt->find(funcName);
            if (it != scopeIt->end())
            {
                funcObjPtr = &it->second;
                break;
            }
        }
        if (!funcObjPtr)
        {
            // Check active scopes for `this` instance or class methods
            for (auto scopeIt = g_CurrentInterpreter->scopes.rbegin(); scopeIt != g_CurrentInterpreter->scopes.rend(); ++scopeIt)
            {
                auto thisIt = scopeIt->find("this");
                if (thisIt != scopeIt->end())
                {
                    if (thisIt->second.type == TzdValue::INSTANCE && thisIt->second.instanceVal)
                    {
                        TzdInstance *inst = thisIt->second.instanceVal;
                        if (inst->definition && (inst->definition->findMethod(funcName) || inst->definition->findField(funcName)))
                        {
                            TzdSelector sel = tzdInternSelector(funcName ? funcName : "");
                            return rt_tzd_call_method(&thisIt->second, (int32_t)sel, funcName, argCount, (TzdValue *)args);
                        }
                    }
                    else if (thisIt->second.type == TzdValue::CLASS_DEF && thisIt->second.classDefVal)
                    {
                        TzdClassDef *cls = thisIt->second.classDefVal;
                        if (cls->findMethod(funcName) || cls->findField(funcName))
                        {
                            TzdSelector sel = tzdInternSelector(funcName ? funcName : "");
                            return rt_tzd_call_method(&thisIt->second, (int32_t)sel, funcName, argCount, (TzdValue *)args);
                        }
                    }
                }
            }
            if (!g_CurrentInterpreter->m_callFrameStack.empty() && g_CurrentInterpreter->m_callFrameStack.back().thisPtr)
            {
                TzdValue *thisVal = g_CurrentInterpreter->m_callFrameStack.back().thisPtr;
                if (thisVal->type == TzdValue::INSTANCE && thisVal->instanceVal && thisVal->instanceVal->definition)
                {
                    if (thisVal->instanceVal->definition->findMethod(funcName) || thisVal->instanceVal->definition->findField(funcName))
                    {
                        TzdSelector sel = tzdInternSelector(funcName ? funcName : "");
                        return rt_tzd_call_method(thisVal, (int32_t)sel, funcName, argCount, (TzdValue *)args);
                    }
                }
            }
            // Debug: check worker pointers
            std::shared_lock<std::shared_mutex> wlock(s_workerPointersMutex);
            auto wit = s_workerPointers.find(funcName);
            if (wit != s_workerPointers.end())
            {
                TzdValue *res = g_JitPool.next();
                auto jitPtr = reinterpret_cast<void (*)(void *, void *)>(wit->second);
                g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, (TzdValue *)args, argCount});
                std::unordered_map<std::string, TzdValue> jitScope;
                g_CurrentInterpreter->scopes.push_back(jitScope);
                ++g_CurrentInterpreter->m_callDepth;
                std::string frameName = build_jit_frame_string(funcName);
                g_CurrentInterpreter->pushCallStack(frameName, "");

                jitPtr(g_CurrentInterpreter, res);

                if (g_CurrentInterpreter->m_callDepth > 0)
                    --g_CurrentInterpreter->m_callDepth;
                g_CurrentInterpreter->popCallStack();
                g_CurrentInterpreter->scopes.pop_back();
                g_CurrentInterpreter->m_callFrameStack.pop_back();
                return res;
            }
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError("未定义的函数: '" + std::string(funcName ? funcName : "unknown") + "'");
            }
            return g_JitPool.next();
        }

        if (funcObjPtr->type == TzdValue::FUNCTION && funcObjPtr->jittedPtr)
        {
            TzdValue *res = g_JitPool.next();
            g_CurrentInterpreter->m_argPtrStack.push_back((TzdValue *)args);
            g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, (TzdValue *)args, argCount});
            // Cached frame string — avoids per-call string concatenation
            const std::string &frameName = getCachedMethodFrame(std::string(funcName ? funcName : ""), funcName, funcObjPtr->sourceFile, funcObjPtr->line);

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            ++g_CurrentInterpreter->m_callDepth;

            funcObjPtr->jittedPtr(g_CurrentInterpreter, res);

            if (g_CurrentInterpreter->m_callDepth > 0)
                --g_CurrentInterpreter->m_callDepth;
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_argPtrStack.pop_back();
            g_CurrentInterpreter->m_callFrameStack.pop_back();
            return res;
        }

        try
        {
            std::vector<TzdValue> callArgs;
            callArgs.reserve(argCount);
            TzdValue *arr = (TzdValue *)args;
            for (int i = 0; i < argCount; ++i)
                callArgs.push_back(arr[i]);
            TzdValue fallback = g_CurrentInterpreter->callFunction(*funcObjPtr, callArgs);
            TzdValue *res = g_JitPool.next();
            rt_write_fast_ret(res, &fallback);
            return res;
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(e.what(), funcObjPtr->line, funcObjPtr->column);
            }
            return g_JitPool.next();
        }
    }

    void rt_push_arg_frame(void *args)
    {
        if (!g_CurrentInterpreter)
            return;
        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出");
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return;
        }
        g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, (TzdValue *)args, 255});
        ++g_CurrentInterpreter->m_callDepth;
    }

    void rt_pop_arg_frame()
    {
        if (!g_CurrentInterpreter)
            return;
        if (!g_CurrentInterpreter->m_callFrameStack.empty())
            g_CurrentInterpreter->m_callFrameStack.pop_back();
        if (g_CurrentInterpreter->m_callDepth > 0)
            --g_CurrentInterpreter->m_callDepth;
    }

    bool rt_check_recursion(void *interpPtr)
    {
        TzdInterpreter *interp = interpPtr ? (TzdInterpreter *)interpPtr : g_CurrentInterpreter;
        if (!interp)
            return false;
        ++interp->m_callDepth;
        if (interp->m_callDepth > interp->m_maxCallDepth)
        {
            if (!interp->m_hasJitError)
            {
                interp->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(interp->m_maxCallDepth) + ")");
            }
            interp->m_hadRuntimeError = true;
            return true;
        }
        return false;
    }

    void rt_pop_call_depth(void *interpPtr)
    {
        TzdInterpreter *interp = interpPtr ? (TzdInterpreter *)interpPtr : g_CurrentInterpreter;
        if (interp && interp->m_callDepth > 0)
        {
            --interp->m_callDepth;
        }
    }

    void *rt_get_class_def(const char *name)
    {
        if (!name)
            return g_JitPool.next();
        TzdClassDef *cls = TzdOopManager::getClass(name);
        TzdValue *val = g_JitPool.next();
        if (cls)
        {
            *val = TzdValue(cls);
        }
        else
        {
            *val = TzdValue();
        }
        return val;
    }

    void *rt_create_num(double v) { return make_double(v); }
    void *rt_create_bool(bool v) { return make_bool(v); }
    void *rt_create_null()
    {
        static TzdValue s_nullVal;
        return &s_nullVal;
    }

    void *rt_create_str(const char *s)
    {
        if (!s)
            return g_JitPool.next();
        return internStringLiteral(s);
    }

    // Fast int/double → string conversion (avoids interpreter bridge for toString()).
    // Used by the JIT when toString is called with a single numeric argument.
    void *rt_to_string_num(double v)
    {
        TzdValue *res = g_JitPool.next();
        res->type = TzdValue::STRING;
        long long lVal = (long long)v;
        if ((double)lVal == v && std::abs(v) < 9e18)
        {
            char buf[32];
            char *p = buf + 31;
            *p = '\0';
            bool neg = lVal < 0;
            unsigned long long uVal = neg ? (unsigned long long)(-lVal) : (unsigned long long)lVal;
            do
            {
                *--p = '0' + (char)(uVal % 10);
                uVal /= 10;
            } while (uVal > 0);
            if (neg)
                *--p = '-';
            size_t len = (buf + 31) - p;
            res->sVal.assign(p, len);
            uint32_t h = 2166136261U;
            for (size_t k = 0; k < len; ++k)
                h = (h ^ (unsigned char)p[k]) * 16777619U;
            res->ulVal = (h == 0) ? 1 : h;
        }
        else
        {
            char buf[64];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v);
            size_t len = ptr - buf;
            res->sVal.assign(buf, len);
            uint32_t h = 2166136261U;
            for (size_t k = 0; k < len; ++k)
                h = (h ^ (unsigned char)buf[k]) * 16777619U;
            res->ulVal = (h == 0) ? 1 : h;
        }
        return res;
    }

    void *rt_op_add(void *a, void *b);

    // 快速字符串拼接：v1 + v2
    void *rt_str_concat(void *a, void *b)
    {
        if (!a || !b)
            return g_JitPool.next();
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::STRING && v2->type == TzdValue::STRING)
        {
            if (g_JitPool.contains(v1))
            {
                v1->sVal.append(v2->sVal);
                return v1;
            }
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            res->sVal.reserve(v1->sVal.size() + v2->sVal.size());
            res->sVal.append(v1->sVal);
            res->sVal.append(v2->sVal);
            return res;
        }
        if (v1->type == TzdValue::STRING || v2->type == TzdValue::STRING)
        {
            if (v1->type == TzdValue::STRING && g_JitPool.contains(v1))
            {
                TzdInterpreter::appendValueToString(v1->sVal, *v2);
                return v1;
            }
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            TzdInterpreter::appendValueToString(res->sVal, *v1);
            TzdInterpreter::appendValueToString(res->sVal, *v2);
            return res;
        }
        return rt_op_add(a, b);
    }

    // 快速数字与字符串拼接：num + str
    void *rt_str_concat_num_str(double a, void *b)
    {
        if (!b)
            return rt_create_num(a);
        TzdValue *v2 = (TzdValue *)b;
        if (v2->type == TzdValue::STRING)
        {
            char buf[64];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), a);
            size_t numLen = ptr - buf;
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            res->sVal.reserve(numLen + v2->sVal.size());
            res->sVal.append(buf, numLen);
            res->sVal.append(v2->sVal);
            return res;
        }
        if (v2->type == TzdValue::DOUBLE || v2->type == TzdValue::FLOAT)
        {
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::DOUBLE;
            res->dVal = a + v2->dVal;
            return res;
        }
        if (v2->type == TzdValue::INT || v2->type == TzdValue::LONG)
        {
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::DOUBLE;
            res->dVal = a + (double)v2->lVal;
            return res;
        }
        return rt_op_add(rt_create_num(a), b);
    }

    // 快速字符串与数字拼接：str + num
    void *rt_str_concat_str_num(void *a, double b)
    {
        if (!a)
            return rt_create_num(b);
        TzdValue *v1 = (TzdValue *)a;
        if (v1->type == TzdValue::STRING)
        {
            char buf[64];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), b);
            size_t numLen = ptr - buf;
            if (g_JitPool.contains(v1))
            {
                v1->sVal.append(buf, numLen);
                return v1;
            }
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            res->sVal.reserve(v1->sVal.size() + numLen);
            res->sVal.append(v1->sVal);
            res->sVal.append(buf, numLen);
            return res;
        }
        if (v1->type == TzdValue::DOUBLE || v1->type == TzdValue::FLOAT)
        {
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::DOUBLE;
            res->dVal = v1->dVal + b;
            return res;
        }
        if (v1->type == TzdValue::INT || v1->type == TzdValue::LONG)
        {
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::DOUBLE;
            res->dVal = (double)v1->lVal + b;
            return res;
        }
        return rt_op_add(a, rt_create_num(b));
    }

    // 快速三元拼接：str + num + str（针对 "prefix_" + i + "_suffix"）
    void *rt_str_concat_str_num_str(void *a, double b, void *c)
    {
        if (!a || !c)
            return g_JitPool.next();
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v3 = (TzdValue *)c;
        if (v1->type == TzdValue::STRING && v3->type == TzdValue::STRING)
        {
            char buf[64];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), b);
            size_t numLen = ptr - buf;
            if (g_JitPool.contains(v1))
            {
                v1->sVal.append(buf, numLen);
                v1->sVal.append(v3->sVal);
                return v1;
            }
            size_t totalLen = v1->sVal.size() + numLen + v3->sVal.size();
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            res->sVal.reserve(totalLen);
            res->sVal.append(v1->sVal);
            res->sVal.append(buf, numLen);
            res->sVal.append(v3->sVal);
            return res;
        }
        void *ab = rt_str_concat_str_num(a, b);
        return rt_str_concat(ab, c);
    }

    // 快速三元字符串拼接：str + str + str
    void *rt_str_concat_3(void *a, void *b, void *c)
    {
        if (!a || !b || !c)
            return g_JitPool.next();
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        TzdValue *v3 = (TzdValue *)c;
        if (v1->type == TzdValue::STRING && v2->type == TzdValue::STRING && v3->type == TzdValue::STRING)
        {
            if (g_JitPool.contains(v1))
            {
                v1->sVal.append(v2->sVal);
                v1->sVal.append(v3->sVal);
                return v1;
            }
            size_t totalLen = v1->sVal.size() + v2->sVal.size() + v3->sVal.size();
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            res->sVal.reserve(totalLen);
            res->sVal.append(v1->sVal);
            res->sVal.append(v2->sVal);
            res->sVal.append(v3->sVal);
            return res;
        }
        TzdValue *res = g_JitPool.next();
        res->type = TzdValue::STRING;
        res->sVal.clear();
        if (v1)
            TzdInterpreter::appendValueToString(res->sVal, *v1);
        if (v2)
            TzdInterpreter::appendValueToString(res->sVal, *v2);
        if (v3)
            TzdInterpreter::appendValueToString(res->sVal, *v3);
        return res;
    }

    // 快速就地字符串追加：s += rhs / s = s + rhs
    void *rt_str_append(void *a, void *b)
    {
        if (!a)
            return b;
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::STRING)
        {
            if (isInternedLiteral(v1) || g_JitPool.contains(v1))
            {
                TzdValue *fresh = new TzdValue(*v1);
                if (g_CurrentInterpreter)
                    g_CurrentInterpreter->trackJitValue(fresh);
                v1 = fresh;
            }
            if (v2 && v2->type == TzdValue::STRING)
            {
                v1->sVal.append(v2->sVal);
            }
            else if (v2)
            {
                TzdInterpreter::appendValueToString(v1->sVal, *v2);
            }
            return v1;
        }
        return rt_op_add(a, b);
    }

    // 快速就地数字追加：s += num / s = s + num
    void *rt_str_append_num(void *a, double b)
    {
        if (!a)
            return rt_create_num(b);
        TzdValue *v1 = (TzdValue *)a;
        if (v1->type == TzdValue::STRING)
        {
            if (isInternedLiteral(v1) || g_JitPool.contains(v1))
            {
                TzdValue *fresh = new TzdValue(*v1);
                if (g_CurrentInterpreter)
                    g_CurrentInterpreter->trackJitValue(fresh);
                v1 = fresh;
            }
            char buf[64];
            auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), b);
            v1->sVal.append(buf, ptr - buf);
            return v1;
        }
        return rt_op_add(a, rt_create_num(b));
    }

    void *rt_op_add(void *a, void *b)
    {
        if (!a || !b)
            return make_double(0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        // String concatenation (highest priority for +)
        if (v1->type == TzdValue::STRING || v2->type == TzdValue::STRING)
        {
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::STRING;
            res->sVal.clear();
            if (v1->type == TzdValue::STRING && v2->type == TzdValue::STRING)
            {
                res->sVal.reserve(v1->sVal.size() + v2->sVal.size());
                res->sVal.append(v1->sVal);
                res->sVal.append(v2->sVal);
            }
            else
            {
                TzdInterpreter::appendValueToString(res->sVal, *v1);
                TzdInterpreter::appendValueToString(res->sVal, *v2);
            }
            return res;
        }
        // BIGINT/RATIONAL arithmetic (exact)
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT ||
            v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
        {
            if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            {
                std::string r = rational_add(to_rational_str(*v1), to_rational_str(*v2));
                if (r == "inf")
                    return make_double(std::numeric_limits<double>::infinity());
                TzdValue *res = g_JitPool.next();
                res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
                res->sVal = r;
                return res;
            }
            std::string r = bigint_add(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE)
        {
            return make_double(v1->dVal + v2->dVal);
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) + TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_sub(void *a, void *b)
    {
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT ||
            v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
        {
            if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            {
                std::string r = rational_sub(to_rational_str(*v1), to_rational_str(*v2));
                if (r == "inf")
                    return make_double(std::numeric_limits<double>::infinity());
                TzdValue *res = g_JitPool.next();
                res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
                res->sVal = r;
                return res;
            }
            std::string r = bigint_sub(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE)
        {
            return make_double(v1->dVal - v2->dVal);
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) - TzdInterpreter::getAsDoubleInternal(*v2));
    }

    // 乘法
    void *rt_op_mul(void *a, void *b)
    {
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT ||
            v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
        {
            if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            {
                std::string r = rational_mul(to_rational_str(*v1), to_rational_str(*v2));
                if (r == "inf")
                    return make_double(std::numeric_limits<double>::infinity());
                TzdValue *res = g_JitPool.next();
                res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
                res->sVal = r;
                return res;
            }
            std::string r = bigint_mul(to_bigint_str(*v1), to_bigint_str(*v2));
            if (r == "inf")
                return make_double(std::numeric_limits<double>::infinity());
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE)
        {
            return make_double(v1->dVal * v2->dVal);
        }
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) * TzdInterpreter::getAsDoubleInternal(*v2));
    }

    // 除法
    void *rt_op_div(void *a, void *b)
    {
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT ||
            v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
        {
            if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            {
                std::string r = rational_div(to_rational_str(*v1), to_rational_str(*v2));
                if (r == "inf")
                    return make_double(std::numeric_limits<double>::infinity());
                TzdValue *res = g_JitPool.next();
                res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
                res->sVal = r;
                return res;
            }
            std::string r = bigint_div(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        double dv = TzdInterpreter::getAsDoubleInternal(*v2);
        if (dv == 0)
            return make_double(0.0);
        return make_double(TzdInterpreter::getAsDoubleInternal(*v1) / dv);
    }

    // 取模
    void *rt_op_mod(void *a, void *b)
    {
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
        {
            std::string r = bigint_mod(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        return make_double(fmod(TzdInterpreter::getAsDoubleInternal(*v1), TzdInterpreter::getAsDoubleInternal(*v2)));
    }

    // 幂运算
    void *rt_op_pow(void *a, void *b)
    {
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        // Integer power → exact BIGINT (like Python)
        bool leftIsInt = v1->type >= TzdValue::SBYTE && v1->type <= TzdValue::ULONG;
        bool rightIsInt = v2->type >= TzdValue::SBYTE && v2->type <= TzdValue::ULONG;
        if ((leftIsInt || v1->type == TzdValue::BIGINT) && rightIsInt)
        {
            std::string base = to_bigint_str(*v1), exp = to_bigint_str(*v2);
            std::string r = bigint_pow(base, exp);
            if (r == "inf")
                return make_double(std::numeric_limits<double>::infinity());
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        // JIT converts all integer literals to DOUBLE, so check for whole-number doubles
        // that are within safe integer range (< 2^53). This gives exact BIGINT results
        // for expressions like 10^100 even when JIT-compiled.
        if (v1->type == TzdValue::DOUBLE && v2->type == TzdValue::DOUBLE)
        {
            double d1 = v1->dVal, d2 = v2->dVal;
            if (d1 == std::floor(d1) && d2 == std::floor(d2) &&
                std::abs(d1) < 1e15 && std::abs(d2) < 1e15)
            {
                // Both are whole numbers — use exact BIGINT power
                std::string base = std::to_string((long long)d1);
                std::string exp = std::to_string((long long)d2);
                if (bigint_is_neg(exp))
                {
                    // Negative exponent → rational (1 / base^|exp|)
                    std::string den = bigint_pow(base, bigint_abs(exp));
                    if (den == "inf")
                        return make_double(std::numeric_limits<double>::infinity());
                    std::string r = rational_make("1", den);
                    TzdValue *res = g_JitPool.next();
                    res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
                    res->sVal = r;
                    return res;
                }
                std::string r = bigint_pow(base, exp);
                if (r == "inf")
                    return make_double(std::numeric_limits<double>::infinity());
                TzdValue *res = g_JitPool.next();
                res->type = TzdValue::BIGINT;
                res->sVal = r;
                return res;
            }
        }
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
        {
            std::string r = rational_pow(to_rational_str(*v1), to_bigint_str(*v2));
            if (r == "inf")
                return make_double(std::numeric_limits<double>::infinity());
            TzdValue *res = g_JitPool.next();
            res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        return make_double(pow(TzdInterpreter::getAsDoubleInternal(*v1), TzdInterpreter::getAsDoubleInternal(*v2)));
    }

    // 取反 (负号)
    void *rt_op_neg(void *a)
    {
        TzdValue *v = (TzdValue *)a;
        if (v->type == TzdValue::BIGINT)
        {
            std::string r = bigint_sub("0", v->sVal);
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        if (v->type == TzdValue::RATIONAL)
        {
            std::string num, den;
            rational_parse(v->sVal, num, den);
            std::string r = rational_make(bigint_sub("0", num), den);
            TzdValue *res = g_JitPool.next();
            res->type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        return make_double(-TzdInterpreter::getAsDoubleInternal(*v));
    }

    bool rt_to_bool(void *a)
    {
        if (!a)
            return false;
        TzdValue *v = (TzdValue *)a;
        if (v->type == TzdValue::BOOL)
            return v->bVal;
        if (v->type == TzdValue::DOUBLE || v->type == TzdValue::FLOAT)
            return v->dVal != 0.0;
        if (v->type >= TzdValue::SBYTE && v->type <= TzdValue::ULONG)
            return v->lVal != 0;
        return TzdInterpreter::isTruthy(*v);
    }

    void *rt_op_not(void *a) { return make_bool(!rt_to_bool(a)); }

    int64_t rt_to_int64_fast(void *v)
    {
        if (!v)
            return 0;
        if ((uintptr_t)v < 4096)
            return (int64_t)(uintptr_t)v;
        TzdValue *val = (TzdValue *)v;
        return TzdInterpreter::getAsInt64Internal(*val);
    }

    void *rt_op_bitand(void *a, void *b)
    {
        if (!a || !b)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
        {
            std::string r = bigint_and(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        return make_double((double)(v1_i & v2_i));
    }

    void *rt_op_bitor(void *a, void *b)
    {
        if (!a || !b)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
        {
            std::string r = bigint_or(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        return make_double((double)(v1_i | v2_i));
    }

    void *rt_op_bitxor(void *a, void *b)
    {
        if (!a || !b)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
        {
            std::string r = bigint_xor(to_bigint_str(*v1), to_bigint_str(*v2));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        return make_double((double)(v1_i ^ v2_i));
    }

    void *rt_op_bitnot(void *a)
    {
        if (!a)
            return make_double(-1.0);
        TzdValue *v1 = (TzdValue *)a;
        if (v1->type == TzdValue::BIGINT)
        {
            std::string r = bigint_not(to_bigint_str(*v1));
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        return make_double((double)(~v1_i));
    }

    void *rt_op_shl(void *a, void *b)
    {
        if (!a)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || (v2 && v2->type == TzdValue::BIGINT))
        {
            int64_t shift = rt_to_int64_fast(b);
            std::string r = bigint_shl(to_bigint_str(*v1), shift);
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        if (v2_i >= 64 || v2_i < 0 || (v2_i > 0 && v1_i != 0 && (v2_i >= 62 || (uint64_t)std::abs(v1_i) > (0x7FFFFFFFFFFFFFFFULL >> v2_i))))
        {
            std::string r = bigint_shl(std::to_string(v1_i), v2_i);
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        return make_double((double)(v1_i << (v2_i & 63)));
    }

    void *rt_op_shr(void *a, void *b)
    {
        if (!a)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || (v2 && v2->type == TzdValue::BIGINT))
        {
            int64_t shift = rt_to_int64_fast(b);
            std::string r = bigint_shr(to_bigint_str(*v1), shift);
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        int64_t v1_i = rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        if (v2_i >= 64)
            return make_double(v1_i < 0 ? -1.0 : 0.0);
        if (v2_i < 0)
            return make_double(0.0);
        return make_double((double)(v1_i >> (v2_i & 63)));
    }

    void *rt_op_ushr(void *a, void *b)
    {
        if (!a)
            return make_double(0.0);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::BIGINT || (v2 && v2->type == TzdValue::BIGINT))
        {
            int64_t shift = rt_to_int64_fast(b);
            std::string r = bigint_ushr(to_bigint_str(*v1), shift);
            TzdValue *res = g_JitPool.next();
            res->type = TzdValue::BIGINT;
            res->sVal = r;
            return res;
        }
        uint64_t v1_i = (uint64_t)rt_to_int64_fast(a);
        int64_t v2_i = rt_to_int64_fast(b);
        if (v2_i >= 64 || v2_i < 0)
            return make_double(0.0);
        return make_double((double)(v1_i >> (v2_i & 63)));
    }

    void *rt_op_sqrt(void *a) { return make_double(sqrt(TzdInterpreter::getAsDoubleInternal(*(TzdValue *)a))); }

    void *rt_op_gt(void *a, void *b)
    {
        if (!a || !b)
            return make_bool(false);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) > 0);
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
            return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) > 0);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) > TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_lt(void *a, void *b)
    {
        if (!a || !b)
            return make_bool(false);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) < 0);
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
            return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) < 0);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) < TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_ge(void *a, void *b)
    {
        if (!a || !b)
            return make_bool(false);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) >= 0);
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
            return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) >= 0);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) >= TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_le(void *a, void *b)
    {
        if (!a || !b)
            return make_bool(false);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) <= 0);
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
            return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) <= 0);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) <= TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_eq(void *a, void *b)
    {
        if (!a || !b)
            return make_bool(false);
        TzdValue *v1 = (TzdValue *)a;
        TzdValue *v2 = (TzdValue *)b;
        if (v1->type == v2->type)
        {
            if (v1->type == TzdValue::NONE)
                return make_bool(true);
            if (v1->type == TzdValue::BOOL)
                return make_bool(v1->bVal == v2->bVal);
            if (v1->type == TzdValue::STRING)
                return make_bool(v1->sVal == v2->sVal);
            if (v1->type == TzdValue::INSTANCE)
                return make_bool(v1->instanceVal == v2->instanceVal);
            if (v1->type == TzdValue::CLASS_DEF)
                return make_bool(v1->classDefVal == v2->classDefVal);
            if (v1->type == TzdValue::POINTER)
                return make_bool(v1->ptrVal == v2->ptrVal);
            if (v1->type == TzdValue::RATIONAL)
                return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) == 0);
            if (v1->type == TzdValue::BIGINT)
                return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) == 0);
            if (v1->type == TzdValue::DOUBLE || v1->type == TzdValue::FLOAT)
                return make_bool(v1->dVal == v2->dVal);
            if (v1->type >= TzdValue::SBYTE && v1->type <= TzdValue::ULONG)
                return make_bool(v1->lVal == v2->lVal);
        }
        if (v1->type == TzdValue::NONE || v2->type == TzdValue::NONE)
            return make_bool(false);
        if (v1->type == TzdValue::RATIONAL || v2->type == TzdValue::RATIONAL)
            return make_bool(rational_compare(to_rational_str(*v1), to_rational_str(*v2)) == 0);
        if (v1->type == TzdValue::BIGINT || v2->type == TzdValue::BIGINT)
            return make_bool(bigint_compare(to_bigint_str(*v1), to_bigint_str(*v2)) == 0);
        bool v1Num = (v1->type >= TzdValue::SBYTE && v1->type <= TzdValue::DOUBLE);
        bool v2Num = (v2->type >= TzdValue::SBYTE && v2->type <= TzdValue::DOUBLE);
        if (!v1Num || !v2Num)
            return make_bool(false);
        return make_bool(TzdInterpreter::getAsDoubleInternal(*v1) == TzdInterpreter::getAsDoubleInternal(*v2));
    }

    void *rt_op_ne(void *a, void *b)
    {
        TzdValue *eq = (TzdValue *)rt_op_eq(a, b);
        return make_bool(!eq->bVal);
    }

    void *rt_create_array()
    {
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::ARRAY;
        return v;
    }

    void *rt_create_map()
    {
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::MAP;
        v->mapVal.clear();
        v->ptrVal = nullptr;
        return v;
    }

    void rt_map_set(void *mapPtr, void *keyVal, void *val)
    {
        if (mapPtr && keyVal && val)
        {
            TzdValue *vMap = (TzdValue *)mapPtr;
            TzdValue *vKey = (TzdValue *)keyVal;
            TzdValue *vVal = (TzdValue *)val;
            // Avoid string copy when key is already a STRING (the common case from toString())
            if (vKey->type == TzdValue::STRING || vKey->type == TzdValue::BIGINT || vKey->type == TzdValue::RATIONAL)
                vMap->mapVal[vKey->sVal] = *vVal;
            else
                vMap->mapVal[TzdInterpreter::getAsString(*vKey)] = *vVal;
        }
    }

    void rt_map_set_str(void *mapPtr, const char *key, void *val)
    {
        if (mapPtr && key && val)
        {
            TzdValue *vMap = (TzdValue *)mapPtr;
            TzdValue *vVal = (TzdValue *)val;
            vMap->mapVal[key] = *vVal;
        }
    }

    void rt_array_push(void *arr, void *val)
    {
        if (arr && val)
            ((TzdValue *)arr)->arrVal.push_back(*(TzdValue *)val);
    }

    void rt_copy_value(void *dest, void *src)
    {
        if (!dest || !src || dest == src)
            return;
        TzdValue *d = (TzdValue *)dest;
        TzdValue *s = (TzdValue *)src;
        if (s->isNativeDoubleArr && !s->nativeArr.empty())
        {
            *d = std::move(*s);
            d->syncFastBuf();
            return;
        }
        *d = *s;
        d->syncFastBuf();
    }

    void *rt_get_index(void *arr, void *idx)
    {
        TzdValue *vArr = (TzdValue *)arr;
        TzdValue *vIdx = (TzdValue *)idx;
        if (!vArr || !vIdx)
            return g_JitPool.next();

        if (vArr->type == TzdValue::MAP)
        {
            // Avoid string copy when key is already a STRING
            const std::string &k = (vIdx->type == TzdValue::STRING || vIdx->type == TzdValue::BIGINT || vIdx->type == TzdValue::RATIONAL)
                                       ? vIdx->sVal
                                       : TzdInterpreter::getAsString(*vIdx);
            if (vArr->ptrVal)
            {
                auto *m = (TzdFastDoubleMap *)vArr->ptrVal;
                uint32_t h = (vIdx->type == TzdValue::STRING && vIdx->ulVal != 0) ? (uint32_t)vIdx->ulVal : 0;
                double outVal = 0.0;
                if (m->find(k, outVal, h))
                    return make_double(outVal);
            }
            auto it = vArr->mapVal.find(k);
            if (it != vArr->mapVal.end())
                return &it->second;
            return g_JitPool.next();
        }

        int i = (int)TzdInterpreter::getAsDoubleInternal(*vIdx);

        if (vArr->type == TzdValue::ARRAY)
        {
            if (vArr->isNativeDoubleArr)
            {
                if (i >= 0 && i < (int)vArr->nativeArr.size())
                {
                    return make_double(vArr->nativeArr[i]);
                }
                if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                {
                    g_CurrentInterpreter->reportJitError("数组索引越界: 尝试访问索引 " + std::to_string(i) +
                                                         ", 但数组长度为 " + std::to_string(vArr->nativeArr.size()));
                }
                return g_JitPool.next();
            }

            if (i >= 0 && i < (int)vArr->arrVal.size())
            {
                TzdValue *res = g_JitPool.next();
                *res = vArr->arrVal[i];
                return res;
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("数组索引越界: 尝试访问索引 " + std::to_string(i) +
                                                     ", 但数组长度为 " + std::to_string(vArr->arrVal.size()));
            }
            return g_JitPool.next();
        }
        if (vArr->type == TzdValue::STRING)
        {
            if (i >= 0 && i < (int)vArr->sVal.size())
            {
                return rt_create_str(std::string(1, vArr->sVal[i]).c_str());
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("字符串索引越界: 尝试访问索引 " + std::to_string(i) +
                                                     ", 但字符串长度为 " + std::to_string(vArr->sVal.size()));
            }
            return g_JitPool.next();
        }
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
        {
            g_CurrentInterpreter->reportJitError("类型错误: 该类型不支持下标访问");
        }
        return g_JitPool.next();
    }

    void *rt_call_sub(const char *funcName, int argCount, ...)
    {
        if (!g_CurrentInterpreter)
            return g_JitPool.next();
        if (g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        std::vector<TzdValue> args;
        args.reserve(argCount);
        va_list ap;
        va_start(ap, argCount);
        for (int i = 0; i < argCount; i++)
        {
            void *argRaw = va_arg(ap, void *);
            if (argRaw)
                args.push_back(*(TzdValue *)argRaw);
        }
        va_end(ap);

        TzdValue funcObj;
        try
        {
            funcObj = g_CurrentInterpreter->getVariable(funcName, nullptr);
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError("未定义的函数: '" + std::string(funcName ? funcName : "unknown") + "'");
            }
            return g_JitPool.next();
        }

        if (funcObj.type == TzdValue::FUNCTION && funcObj.jittedPtr)
        {
            TzdValue *result = g_JitPool.next();

            g_CurrentInterpreter->m_argFrameStack.push_back(std::move(args));
            g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, g_CurrentInterpreter->m_argFrameStack.back().data(), argCount});
            g_CurrentInterpreter->m_argPtrStack.push_back(g_CurrentInterpreter->m_argFrameStack.back().data());

            // 【修复】：格式化源文件路径与行号
            std::string fileLoc = formatSourcePath(funcObj.sourceFile);
            int line = funcObj.line;
            std::string frameName = std::string(funcName) + " (" + fileLoc;
            if (line > 0)
                frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            ++g_CurrentInterpreter->m_callDepth;

            funcObj.jittedPtr(g_CurrentInterpreter, result);

            if (g_CurrentInterpreter->m_callDepth > 0)
                --g_CurrentInterpreter->m_callDepth;
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_argPtrStack.pop_back();
            g_CurrentInterpreter->m_callFrameStack.pop_back();
            g_CurrentInterpreter->m_argFrameStack.pop_back();
            return result;
        }

        if (funcObj.type != TzdValue::FUNCTION && funcObj.type != TzdValue::NATIVE_FUNCTION)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError("未定义的函数: '" + std::string(funcName ? funcName : "unknown") + "'");
            }
            return g_JitPool.next();
        }

        try
        {
            TzdValue res = g_CurrentInterpreter->callFunction(funcObj, args);
            TzdValue *poolVal = g_JitPool.next();
            *poolVal = res;
            return poolVal;
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(e.what(), funcObj.line, funcObj.column);
            }
            return g_JitPool.next();
        }
    }

    void *rt_call_sub_f1(const char *funcName, void *arg0)
    {
        if (!g_CurrentInterpreter || !arg0)
            return g_JitPool.next();
        if (g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        TzdValue *funcObj = nullptr;
        auto &scopes = g_CurrentInterpreter->scopes;
        if (!scopes.empty())
        {
            auto it = scopes.front().find(funcName);
            if (it != scopes.front().end())
                funcObj = &it->second;
        }
        if (funcObj && funcObj->type == TzdValue::FUNCTION && funcObj->jittedPtr)
        {
            TzdValue *result = g_JitPool.next();
            std::vector<TzdValue> savedArgs = std::move(g_CurrentInterpreter->m_currentArgs);
            g_CurrentInterpreter->m_currentArgs.assign(1, *(TzdValue *)arg0);

            // 【修复】：格式化源文件路径与行号
            std::string fileLoc = formatSourcePath(funcObj->sourceFile);
            int line = funcObj->line;
            std::string frameName = std::string(funcName) + " (" + fileLoc;
            if (line > 0)
                frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, (TzdValue *)arg0, 1});

            ++g_CurrentInterpreter->m_callDepth;

            funcObj->jittedPtr(g_CurrentInterpreter, result);

            if (g_CurrentInterpreter->m_callDepth > 0)
                --g_CurrentInterpreter->m_callDepth;
            g_CurrentInterpreter->m_callFrameStack.pop_back();
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            g_CurrentInterpreter->m_currentArgs = std::move(savedArgs);
            return result;
        }

        return rt_call_sub(funcName, 1, arg0);
    }

    void rt_push_jit_frame(const char *name)
    {
        if (!g_CurrentInterpreter)
            return;
        g_CurrentInterpreter->pushCallStack(build_jit_frame_string(name), "");
    }

    void rt_pop_jit_frame()
    {
        if (!g_CurrentInterpreter)
            return;
        g_CurrentInterpreter->popCallStack();
    }

    void rt_replace_jit_frame(const char *name)
    {
        if (!g_CurrentInterpreter)
            return;
        std::lock_guard<std::mutex> lock(g_CurrentInterpreter->m_stackTraceMutex);
        if (!g_CurrentInterpreter->m_callStackFrames.empty())
            g_CurrentInterpreter->m_callStackFrames.back() = build_jit_frame_string(name);
        else
            g_CurrentInterpreter->m_callStackFrames.push_back(build_jit_frame_string(name));
    }

    int rt_get_call_stack_depth()
    {
        if (!g_CurrentInterpreter)
            return 0;
        std::lock_guard<std::mutex> lock(g_CurrentInterpreter->m_stackTraceMutex);
        return (int)g_CurrentInterpreter->m_callStackFrames.size();
    }

    void rt_restore_call_stack_depth(int depth)
    {
        if (!g_CurrentInterpreter || depth < 0)
            return;
        std::lock_guard<std::mutex> lock(g_CurrentInterpreter->m_stackTraceMutex);
        while ((int)g_CurrentInterpreter->m_callStackFrames.size() > depth)
        {
            g_CurrentInterpreter->m_callStackFrames.pop_back();
            if (!g_CurrentInterpreter->m_debugFileStack.empty())
                g_CurrentInterpreter->m_debugFileStack.pop_back();
        }
    }

    void rt_store_index(void *arr, void *idx, void *val)
    {
        if (!arr || !idx || !val)
            return;
        TzdValue *vArr = (TzdValue *)arr;
        TzdValue *vIdx = (TzdValue *)idx;
        TzdValue *vVal = (TzdValue *)val;
        if (vArr->type == TzdValue::MAP)
        {
            if (vIdx->type == TzdValue::STRING || vIdx->type == TzdValue::BIGINT || vIdx->type == TzdValue::RATIONAL)
            {
                if (g_JitPool.contains(vIdx))
                {
                    auto [it, inserted] = vArr->mapVal.try_emplace(std::move(vIdx->sVal), *vVal);
                    if (!inserted)
                        it->second = *vVal;
                }
                else
                {
                    auto [it, inserted] = vArr->mapVal.try_emplace(vIdx->sVal, *vVal);
                    if (!inserted)
                        it->second = *vVal;
                }
            }
            else
            {
                auto [it, inserted] = vArr->mapVal.try_emplace(TzdInterpreter::getAsString(*vIdx), *vVal);
                if (!inserted)
                    it->second = *vVal;
            }
            return;
        }

        int i = (int)TzdInterpreter::getAsDoubleInternal(*vIdx);
        if (vArr->type == TzdValue::ARRAY)
        {
            if (vArr->isNativeDoubleArr)
            {
                if (i >= 0 && i < (int)vArr->nativeArr.size())
                {
                    if (vVal->type == TzdValue::DOUBLE || vVal->type == TzdValue::FLOAT)
                    {
                        vArr->nativeArr[i] = vVal->dVal;
                    }
                    else if (vVal->type == TzdValue::BOOL)
                    {
                        vArr->nativeArr[i] = vVal->bVal ? 1.0 : 0.0;
                    }
                    else
                    {
                        vArr->arrVal.reserve(vArr->nativeArr.size());
                        for (double d : vArr->nativeArr)
                            vArr->arrVal.push_back(TzdValue(d));
                        vArr->nativeArr.clear();
                        vArr->isNativeDoubleArr = false;
                        vArr->syncFastBuf();
                        vArr->arrVal[i] = *vVal;
                    }
                    return;
                }
                if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                {
                    g_CurrentInterpreter->reportJitError("数组下标越界: 尝试写入索引 " + std::to_string(i) + ", 但数组大小为 " + std::to_string(vArr->nativeArr.size()));
                }
                return;
            }

            if (i >= 0 && i < (int)vArr->arrVal.size())
            {
                vArr->arrVal[i] = *vVal;
            }
            else
            {
                if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                {
                    g_CurrentInterpreter->reportJitError("数组下标越界: 尝试写入索引 " + std::to_string(i) + ", 但数组大小为 " + std::to_string(vArr->arrVal.size()));
                }
            }
            return;
        }
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
        {
            g_CurrentInterpreter->reportJitError("类型错误: 该数据类型不支持下标赋值操作");
        }
    }

    void rt_store_index_d(void *arr, void *idx, double val)
    {
        if (!arr || !idx)
            return;
        TzdValue *vArr = (TzdValue *)arr;
        TzdValue *vIdx = (TzdValue *)idx;
        if (vArr->type == TzdValue::MAP)
        {
            if (!vArr->ptrVal)
            {
                vArr->ptrVal = new TzdFastDoubleMap(524288);
            }
            auto *m = (TzdFastDoubleMap *)vArr->ptrVal;
            const std::string &key = (vIdx->type == TzdValue::STRING || vIdx->type == TzdValue::BIGINT || vIdx->type == TzdValue::RATIONAL)
                                         ? vIdx->sVal
                                         : TzdInterpreter::getAsString(*vIdx);
            uint32_t h = (vIdx->type == TzdValue::STRING && vIdx->ulVal != 0) ? (uint32_t)vIdx->ulVal : 0;
            m->insert(key, val, h);
            return;
        }
        TzdValue boxed(val);
        rt_store_index(arr, idx, &boxed);
    }

    void *rt_get_index_fast(void *arr, double idxD)
    {
        TzdValue *vArr = (TzdValue *)arr;
        if (!vArr)
            return rt_create_null();
        int i = (int)idxD;
        if (vArr->isNativeDoubleArr)
        {
            if (i >= 0 && i < (int)vArr->nativeArr.size())
            {
                double d = vArr->nativeArr[i];
                if (d == 1.0)
                    return make_bool(true);
                if (d == 0.0)
                    return make_bool(false);
                return make_double(d);
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                int len = (int)vArr->nativeArr.size();
                g_CurrentInterpreter->reportJitError("数组索引越界: 尝试访问索引 " + std::to_string(i) +
                                                     ", 但数组长度为 " + std::to_string(len));
            }
            return rt_create_null();
        }
        if (vArr->type == TzdValue::ARRAY)
        {
            if (i >= 0 && i < (int)vArr->arrVal.size())
            {
                return &vArr->arrVal[i];
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                int len = (int)vArr->arrVal.size();
                g_CurrentInterpreter->reportJitError("数组索引越界: 尝试访问索引 " + std::to_string(i) +
                                                     ", 但数组长度为 " + std::to_string(len));
            }
            return rt_create_null();
        }
        if (vArr->type == TzdValue::STRING)
        {
            if (i >= 0 && i < (int)vArr->sVal.size())
            {
                return rt_create_str(std::string(1, vArr->sVal[i]).c_str());
            }
        }
        TzdValue idxBox(idxD);
        return rt_get_index(arr, &idxBox);
    }

    void rt_store_index_bool(void *arr, double idxD, bool val)
    {
        if (!arr)
            return;
        TzdValue *vArr = (TzdValue *)arr;
        int i = (int)idxD;
        if (vArr->isNativeDoubleArr)
        {
            if (i >= 0 && i < (int)vArr->nativeArr.size())
            {
                vArr->nativeArr[i] = val ? 1.0 : 0.0;
                return;
            }
        }
        if (vArr->type == TzdValue::ARRAY)
        {
            if (i >= 0 && i < (int)vArr->arrVal.size())
            {
                TzdValue &target = vArr->arrVal[i];
                target.type = TzdValue::BOOL;
                target.bVal = val;
                return;
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                int len = (int)vArr->arrVal.size();
                g_CurrentInterpreter->reportJitError("数组下标越界: 尝试写入索引 " + std::to_string(i) + ", 但数组大小为 " + std::to_string(len));
            }
            return;
        }
        TzdValue bVal(val);
        TzdValue idxBox(idxD);
        rt_store_index(arr, &idxBox, &bVal);
    }

    void rt_store_index_val_dyn(void *arr, double idxD, void *val)
    {
        if (!arr || !val)
            return;
        TzdValue *vArr = (TzdValue *)arr;
        TzdValue *vVal = (TzdValue *)val;
        int i = (int)idxD;
        if (vArr->isNativeDoubleArr && i >= 0 && i < (int)vArr->nativeArr.size())
        {
            if (vVal->type == TzdValue::DOUBLE || vVal->type == TzdValue::FLOAT)
            {
                vArr->nativeArr[i] = vVal->dVal;
                return;
            }
            if (vVal->type == TzdValue::BOOL)
            {
                vArr->nativeArr[i] = vVal->bVal ? 1.0 : 0.0;
                return;
            }
        }
        if (vArr->type == TzdValue::ARRAY)
        {
            if (i >= 0 && i < (int)vArr->arrVal.size())
            {
                TzdValue &target = vArr->arrVal[i];
                if (vVal->type == TzdValue::BOOL)
                {
                    target.type = TzdValue::BOOL;
                    target.bVal = vVal->bVal;
                    return;
                }
                if (vVal->type == TzdValue::DOUBLE || vVal->type == TzdValue::FLOAT)
                {
                    target.type = TzdValue::DOUBLE;
                    target.dVal = vVal->dVal;
                    return;
                }
                if (target.instanceVal)
                {
                    target.instanceVal->release();
                    target.instanceVal = nullptr;
                }
                if (target.type == TzdValue::TENSOR && target.ptrVal)
                {
                    tzdTensorRelease(target.ptrVal);
                    target.ptrVal = nullptr;
                }
                target.type = vVal->type;
                target.bVal = vVal->bVal;
                target.dVal = vVal->dVal;
                target.lVal = vVal->lVal;
                target.ulVal = vVal->ulVal;
                target.ptrVal = vVal->ptrVal;
                if (vVal->instanceVal)
                {
                    target.instanceVal = vVal->instanceVal;
                    target.instanceVal->retain();
                }
                if (!vVal->sVal.empty())
                    target.sVal = vVal->sVal;
                else
                    target.sVal.clear();
                if (!vVal->arrVal.empty())
                    target.arrVal = vVal->arrVal;
                else
                    target.arrVal.clear();
                if (!vVal->mapVal.empty())
                    target.mapVal = vVal->mapVal;
                else
                    target.mapVal.clear();
                return;
            }
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                int len = (int)vArr->arrVal.size();
                g_CurrentInterpreter->reportJitError("数组下标越界: 尝试写入索引 " + std::to_string(i) + ", 但数组大小为 " + std::to_string(len));
            }
            return;
        }
        TzdValue idxBox(idxD);
        rt_store_index(arr, &idxBox, val);
    }

    void rt_set_location(int line, int col)
    {
        if (g_CurrentInterpreter)
        {
            g_CurrentInterpreter->m_jitLine = (size_t)line;
            g_CurrentInterpreter->m_jitColumn = (size_t)col;
        }
    }

    void rt_store_native_to_ptr(void *dest, double val)
    {
        if (!dest)
            return;
        TzdValue *v = reinterpret_cast<TzdValue *>(dest);
        *v = TzdValue(val);
    }

    void rt_set_last_ret(void *val)
    {
        if (!val)
            return;
        TzdValue *poolVal = g_JitPool.next();
        *poolVal = *(TzdValue *)val;
        g_LastJitValue = poolVal;
    }

    void *rt_create_inst(const char *name)
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        TzdClassDef *def = TzdOopManager::getClass(name);
        if (!def)
        {
            if (g_CurrentInterpreter)
                g_CurrentInterpreter->reportJitError(std::string("找不到类定义: ") + name);
            return g_JitPool.next();
        }
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::INSTANCE;
        v->setInstance(TzdInstance::create(def));
        return v;
    }

    static thread_local const char *s_lastClassName = nullptr;
    static thread_local TzdClassDef *s_lastClassDef = nullptr;

    void *rt_create_inst_args(const char *name, int argCount, TzdValue *args)
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        TzdClassDef *def = (name && name == s_lastClassName) ? s_lastClassDef : nullptr;
        if (!def)
        {
            def = TzdOopManager::getClass(name);
            if (def)
            {
                s_lastClassName = name;
                s_lastClassDef = def;
            }
        }
        if (!def)
        {
            if (g_CurrentInterpreter)
                g_CurrentInterpreter->reportJitError(std::string("找不到类定义: '") + name + "' (是否忘记 import?)");
            return g_JitPool.next();
        }

        TzdValue *instVal = g_JitPool.next();
        instVal->type = TzdValue::INSTANCE;
        instVal->setInstance(TzdInstance::create(def));

        ClassConstructor *ctor = def->findConstructor((size_t)argCount);
        if (!ctor && argCount == 0 && !def->constructors.empty())
        {
            ctor = def->findConstructor(0);
        }
        if (!ctor)
        {
            if (!def->constructors.empty())
            {
                if (g_CurrentInterpreter)
                {
                    g_CurrentInterpreter->reportJitError("类 '" + def->fullName + "' 未找到接受 " + std::to_string(argCount) + " 个参数的构造函数");
                }
            }
            return instVal;
        }

        if (ctor->jittedPtr)
        {
            int endLine = (ctor->body && ctor->body->getStop()) ? (int)ctor->body->getStop()->getLine() : -1;
            bool hasBp = TzdDebugger::g_DebugActive && (TzdDebugger::isStepping() || TzdDebugger::hasBreakpointsInFunction(ctor->sourceFile, ctor->line, endLine));
            if (!hasBp)
            {
                TzdValue *ignored = g_JitPool.next();
                g_CurrentInterpreter->m_callFrameStack.push_back({instVal, args, argCount});

                if (!TzdDebugger::g_DebugActive)
                {
                    ++g_CurrentInterpreter->m_callDepth;
                    ctor->jittedPtr(g_CurrentInterpreter, ignored);
                    if (g_CurrentInterpreter->m_callDepth > 0)
                        --g_CurrentInterpreter->m_callDepth;
                    g_CurrentInterpreter->m_callFrameStack.pop_back();
                    return instVal;
                }

                // 【修改】：基于构造函数（ctor）的 sourceFile 和 line 字段，拼接 Java 风格构造函数栈帧
                std::string fileLoc = formatSourcePath(ctor->sourceFile);
                int line = ctor->line;
                std::string frameName = std::string(def->fullName) + "." + def->simpleName;
                frameName += " (" + fileLoc;
                if (line > 0)
                    frameName += ":" + std::to_string(line);
                frameName += ") (JIT Compiled)";

                g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
                ++g_CurrentInterpreter->m_callDepth;
                ctor->jittedPtr(g_CurrentInterpreter, ignored);
                if (g_CurrentInterpreter->m_callDepth > 0)
                    --g_CurrentInterpreter->m_callDepth;
                g_CurrentInterpreter->m_callStackFrames.pop_back();
                g_CurrentInterpreter->m_callFrameStack.pop_back();
                return instVal;
            }
        }
        if (g_CurrentInterpreter && ctor->body)
        {
            std::vector<TzdValue> callArgs;
            callArgs.reserve(argCount);
            for (int i = 0; i < argCount; ++i)
                callArgs.push_back(args[i]);
            TzdValue ctorFunc(def->simpleName, ctor->params, ctor->body);
            ctorFunc.jittedPtr = ctor->jittedPtr;
            ctorFunc.setInstance(instVal->instanceVal);

            ctorFunc.sourceFile = ctor->sourceFile;
            ctorFunc.line = ctor->line;
            ctorFunc.column = ctor->column;

            try
            {
                g_CurrentInterpreter->callFunction(ctorFunc, callArgs);
            }
            catch (const std::exception &e)
            {
                if (g_CurrentInterpreter)
                {
                    g_CurrentInterpreter->reportJitError(e.what(), ctor->line, ctor->column);
                }
            }
        }
        return instVal;
    }

    void *rt_create_inst_sroa(const char *name, int count, const double *vals)
    {
        static thread_local const char *s_cachedSroaName = nullptr;
        static thread_local TzdClassDef *s_cachedSroaDef = nullptr;
        if (!s_cachedSroaDef || s_cachedSroaName != name)
        {
            s_cachedSroaDef = TzdOopManager::getClass(name);
            s_cachedSroaName = name;
        }
        TzdValue *instVal = g_JitPool.next();
        instVal->type = TzdValue::INSTANCE;
        TzdInstance *in = TzdInstance::create(s_cachedSroaDef);
        instVal->setInstance(in);
        if (in && count > 0 && vals)
        {
            size_t n = (std::min)((size_t)count, in->fieldValues.size());
            for (size_t i = 0; i < n; ++i)
            {
                in->fieldValues[i].type = TzdValue::DOUBLE;
                in->fieldValues[i].dVal = vals[i];
            }
        }
        return instVal;
    }

    void *rt_create_inst_2d(const char *name, double a1, double a2)
    {
        double vals[2] = {a1, a2};
        return rt_create_inst_sroa(name, 2, vals);
    }

    void *rt_call_value_fast(void *funcPtr, int argCount, TzdValue *args)
    {
        if (!g_CurrentInterpreter || !funcPtr)
            return g_JitPool.next();
        if (g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        TzdValue *funcObj = (TzdValue *)funcPtr;
        if (funcObj->type != TzdValue::FUNCTION && funcObj->type != TzdValue::NATIVE_FUNCTION)
        {
            std::string msg = "尝试调用一个非函数对象";
            if (!funcObj->name.empty())
            {
                msg = "未定义的函数或非函数对象: '" + funcObj->name + "'";
            }
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(msg);
            }
            return g_JitPool.next();
        }

        int endLine = (funcObj->funcBody && funcObj->funcBody->getStop()) ? (int)funcObj->funcBody->getStop()->getLine() : -1;
        bool hasBp = TzdDebugger::g_DebugActive && (TzdDebugger::isStepping() || TzdDebugger::hasBreakpointsInFunction(funcObj->sourceFile, funcObj->line, endLine));

        if (funcObj->type == TzdValue::FUNCTION && funcObj->jittedPtr && !hasBp)
        {
            TzdValue *result = g_JitPool.next();
            std::string fileLoc = formatSourcePath(funcObj->sourceFile);
            int line = funcObj->line;
            std::string frameName = funcObj->name.empty() ? "<anonymous>" : funcObj->name;
            if (funcObj->instanceVal && funcObj->instanceVal->definition)
            {
                frameName = funcObj->instanceVal->definition->fullName + "." + frameName;
            }
            frameName += " (" + fileLoc;
            if (line > 0)
                frameName += ":" + std::to_string(line);
            frameName += ") (JIT Compiled)";

            TzdValue localThis;
            if (funcObj->instanceVal)
                localThis = TzdValue(funcObj->instanceVal);

            g_CurrentInterpreter->m_callStackFrames.push_back(frameName);
            g_CurrentInterpreter->m_callFrameStack.push_back({funcObj->instanceVal ? &localThis : nullptr, (TzdValue *)args, argCount});
            ++g_CurrentInterpreter->m_callDepth;

            funcObj->jittedPtr(g_CurrentInterpreter, result);

            if (g_CurrentInterpreter->m_callDepth > 0)
                --g_CurrentInterpreter->m_callDepth;
            g_CurrentInterpreter->m_callFrameStack.pop_back();
            g_CurrentInterpreter->m_callStackFrames.pop_back();

            return result;
        }

        try
        {
            std::vector<TzdValue> callArgs;
            callArgs.reserve(argCount);
            for (int i = 0; i < argCount; ++i)
                callArgs.push_back(args[i]);
            TzdValue res = g_CurrentInterpreter->callFunction(*funcObj, callArgs);
            TzdValue *poolVal = g_JitPool.next();
            *poolVal = res;
            return poolVal;
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(e.what(), funcObj->line, funcObj->column);
            }
            return g_JitPool.next();
        }
    }

    void *rt_tzd_get_member(void *inst, int32_t selector, const char *name);

    void *rt_tzd_get_member_ic(void *inst, int32_t selector, const char *name, uint64_t *ic)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition)
        {
            constexpr uint64_t kMask = 0x0000FFFFFFFFFFFFull;
            TzdInstance *in = v->instanceVal;
            const uint64_t defBits = (uint64_t)(uintptr_t)in->definition & kMask;
            const uint64_t packed = *ic;
            if (packed != 0 && (packed & kMask) == defBits)
            {
                if (TzdValue *p = in->getMemberPtrByIndex((int)(packed >> 48)))
                    return p;
            }
            else if (const TzdMemberSlot *slot = in->definition->tzdDispatch.find((TzdSelector)selector))
            {
                if (slot->fieldIndex >= 0 && slot->fieldIndex < 0xFFFF)
                    if (TzdValue *p = in->getMemberPtrByIndex(slot->fieldIndex))
                    {
                        *ic = ((uint64_t)slot->fieldIndex << 48) | defBits;
                        return p;
                    }
            }
        }
        return rt_tzd_get_member(inst, selector, name);
    }

    double rt_tzd_get_field_d(void *inst, int fieldIndex)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            TzdInstance *in = v->instanceVal;
            if (fieldIndex >= 0 && (size_t)fieldIndex < in->fieldValues.size())
            {
                const auto &fv = in->fieldValues[fieldIndex];
                if (fv.type == TzdValue::DOUBLE || fv.type == TzdValue::FLOAT)
                    return fv.dVal;
                if (fv.type == TzdValue::ULONG || fv.type == TzdValue::UINT)
                    return (double)fv.ulVal;
                return (double)fv.lVal;
            }
        }
        return 0.0;
    }

    int64_t rt_tzd_get_field_i64(void *inst, int fieldIndex)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            TzdInstance *in = v->instanceVal;
            if (fieldIndex >= 0 && (size_t)fieldIndex < in->fieldValues.size())
            {
                const auto &fv = in->fieldValues[fieldIndex];
                if (fv.type == TzdValue::DOUBLE || fv.type == TzdValue::FLOAT)
                    return (int64_t)fv.dVal;
                if (fv.type == TzdValue::ULONG || fv.type == TzdValue::UINT)
                    return (int64_t)fv.ulVal;
                return fv.lVal;
            }
        }
        return 0;
    }

    void rt_tzd_set_field_i64(void *inst, int fieldIndex, int64_t val)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            TzdInstance *in = v->instanceVal;
            if (fieldIndex >= 0 && (size_t)fieldIndex < in->fieldValues.size())
            {
                in->fieldValues[fieldIndex].type = TzdValue::INT;
                in->fieldValues[fieldIndex].lVal = val;
                in->fieldValues[fieldIndex].dVal = (double)val;
            }
        }
    }

    void *rt_tzd_get_field_val(void *inst, int fieldIndex)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            TzdInstance *in = v->instanceVal;
            if (fieldIndex >= 0 && (size_t)fieldIndex < in->fieldValues.size())
            {
                return &in->fieldValues[fieldIndex];
            }
        }
        return g_JitPool.next();
    }

    void rt_tzd_set_field_d(void *inst, int fieldIndex, double val)
    {
        TzdValue *v = (TzdValue *)inst;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            TzdInstance *in = v->instanceVal;
            if (fieldIndex >= 0 && (size_t)fieldIndex < in->fieldValues.size())
            {
                in->fieldValues[fieldIndex].type = TzdValue::DOUBLE;
                in->fieldValues[fieldIndex].dVal = val;
                in->fieldValues[fieldIndex].lVal = (long long)val;
            }
        }
    }

    void rt_tzd_set_field_val(void *inst, int fieldIndex, void *val)
    {
        TzdValue *v = (TzdValue *)inst;
        TzdValue *src = (TzdValue *)val;
        if (v && v->type == TzdValue::INSTANCE && v->instanceVal && src)
        {
            TzdInstance *in = v->instanceVal;
            try
            {
                in->setMemberByIndex(fieldIndex, *src);
            }
            catch (const std::exception &e)
            {
                if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                    g_CurrentInterpreter->reportJitError(e.what());
            }
        }
    }

    void *rt_tzd_call_method(void *objPtr, int32_t selector, const char *name, int argCount, TzdValue *args)
    {
        if (!objPtr || !g_CurrentInterpreter)
            return g_JitPool.next();
        if (g_CurrentInterpreter->m_hasJitError)
            return g_JitPool.next();

        if (g_CurrentInterpreter->m_callDepth >= g_CurrentInterpreter->m_maxCallDepth)
        {
            if (!g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("运行错误：调用栈溢出 (超出最大调用深度 " + std::to_string(g_CurrentInterpreter->m_maxCallDepth) + ")");
            }
            g_CurrentInterpreter->m_hadRuntimeError = true;
            return g_JitPool.next();
        }

        TzdValue *v = (TzdValue *)objPtr;
        if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition)
        {
            TzdClassDef *cls = v->instanceVal->definition;
            if (const TzdMemberSlot *slot = cls->tzdDispatch.find((TzdSelector)selector))
            {
                if (slot->method && slot->method->jittedPtr)
                {
                    bool hasBp = false;
                    if (TzdDebugger::g_DebugActive)
                    {
                        int endLine = (slot->method->body && slot->method->body->getStop()) ? (int)slot->method->body->getStop()->getLine() : -1;
                        hasBp = TzdDebugger::isStepping() || TzdDebugger::hasBreakpointsInFunction(slot->method->sourceFile, slot->method->line, endLine);
                    }
                    if (!hasBp)
                    {
                        TzdValue *result = g_JitPool.next();
                        const std::string &frameName = getCachedMethodFrame(cls->fullName, name, slot->method->sourceFile, slot->method->line);
                        g_CurrentInterpreter->pushCallStack(frameName, slot->method->sourceFile);
                        g_CurrentInterpreter->m_callFrameStack.push_back({v, args, argCount});
                        ++g_CurrentInterpreter->m_callDepth;

                        slot->method->jittedPtr(g_CurrentInterpreter, result);

                        if (g_CurrentInterpreter->m_callDepth > 0)
                            --g_CurrentInterpreter->m_callDepth;
                        g_CurrentInterpreter->m_callFrameStack.pop_back();
                        g_CurrentInterpreter->popCallStack();
                        return result;
                    }
                }
            }
        }
        else if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
        {
            TzdClassDef *cls = v->classDefVal;
            if (const TzdMemberSlot *slot = cls->tzdDispatch.find((TzdSelector)selector))
            {
                if (slot->method && slot->method->jittedPtr)
                {
                    int endLine = (slot->method->body && slot->method->body->getStop()) ? (int)slot->method->body->getStop()->getLine() : -1;
                    bool hasBp = TzdDebugger::g_DebugActive && (TzdDebugger::isStepping() || TzdDebugger::hasBreakpointsInFunction(slot->method->sourceFile, slot->method->line, endLine));
                    if (!hasBp)
                    {
                        TzdValue *result = g_JitPool.next();
                        const std::string &frameName = getCachedMethodFrame(cls->fullName, name, slot->method->sourceFile, slot->method->line);

                        g_CurrentInterpreter->pushCallStack(frameName, slot->method->sourceFile);
                        g_CurrentInterpreter->m_callFrameStack.push_back({nullptr, args, argCount});
                        ++g_CurrentInterpreter->m_callDepth;

                        slot->method->jittedPtr(g_CurrentInterpreter, result);

                        if (g_CurrentInterpreter->m_callDepth > 0)
                            --g_CurrentInterpreter->m_callDepth;
                        g_CurrentInterpreter->m_callFrameStack.pop_back();
                        g_CurrentInterpreter->popCallStack();
                        return result;
                    }
                }
            }
        }
        void *callee = rt_tzd_get_member(objPtr, selector, name);
        TzdValue *calleeVal = (TzdValue *)callee;
        if (!calleeVal || (calleeVal->type != TzdValue::FUNCTION && calleeVal->type != TzdValue::NATIVE_FUNCTION))
        {
            std::string recvDesc = "对象";
            if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition)
            {
                recvDesc = "类 '" + v->instanceVal->definition->fullName + "' 的实例";
            }
            else if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
            {
                recvDesc = "类 '" + v->classDefVal->fullName + "'";
            }
            else if (v->type == TzdValue::MAP)
            {
                recvDesc = "Map 对象";
            }
            else if (v->type == TzdValue::NONE)
            {
                recvDesc = "空对象 (null/none)";
            }
            std::string msg = recvDesc + " 没有成员或方法: '" + (name ? name : "unknown") + "'";
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(msg);
            }
            return g_JitPool.next();
        }
        return rt_call_value_fast(callee, argCount, args);
    }

    // ------------------------------------------------------------------
    // tzd selector dispatch — bound-method 构造助手
    // rt_tzd_get_member 的 selector 热路径与 tzd_get_member_by_name 的 name 回退
    // 路径共用这两个助手，保证 selector 路径与旧 name 路径语义完全一致。
    // 复用 g_JitPool 槽位时全量覆盖函数元数据，避免上一轮残留泄漏。
    // ------------------------------------------------------------------
    static TzdValue *tzd_make_class_method_value(ClassMethod *method)
    {
        TzdValue *res = g_JitPool.next();
        res->nativeFunc = nullptr;          // 清除残留 native 句柄
        res->jittedPtr = method->jittedPtr; // 读 live method->jittedPtr
        res->funcBody = method->body;
        res->params = method->params;
        res->paramTypes = method->paramTypes;
        res->sourceFile = method->sourceFile;
        res->line = method->line;
        res->column = method->column;
        res->name = method->name;
        res->type = method->isNative ? TzdValue::NATIVE_FUNCTION : TzdValue::FUNCTION;
        if (method->isNative)
            res->nativeFunc = method->nativeWrapper;
        return res;
    }

    static TzdValue *tzd_make_bound_method_value(TzdInstance *recv, ClassMethod *method)
    {
        TzdValue *res = g_JitPool.next();
        res->name = method->name;
        res->setInstance(recv);             // NEVER raw assign — 走 refcount retain/release
        res->jittedPtr = method->jittedPtr; // 读 live method->jittedPtr（JIT 重编后即时可见）
        res->sourceFile = method->sourceFile;
        res->line = method->line;
        res->column = method->column;
        res->nativeFunc = nullptr; // 默认清空，避免池槽位残留
        if (method->isNative)
        {
            res->type = TzdValue::NATIVE_FUNCTION;
            res->nativeFunc = method->nativeWrapper;
            res->funcBody = method->body;
            res->params = method->params;
            res->paramTypes = method->paramTypes;
        }
        else
        {
            res->type = TzdValue::FUNCTION;
            if (method->jittedPtr && !TzdDebugger::g_DebugActive)
            {
                // 热路径：call bridge 直接用 jittedPtr，跳过 params/body 拷贝；
                // 但仍清空这些字段，避免池槽位上一轮残留的 func 元数据。
                res->funcBody = nullptr;
                res->params.clear();
                res->paramTypes.clear();
            }
            else
            {
                // 逃逸 / 调试 / 未 JIT：保留 params/body 供 debug 回退与解释器回退。
                res->funcBody = method->body;
                res->params = method->params;
                res->paramTypes = method->paramTypes;
            }
        }
        return res;
    }

    // name-based 解析核心（不内联 selector，供回退使用；不递归回 rt_get_member）。
    static TzdValue *tzd_get_member_by_name(void *inst, const char *name)
    {
        TzdValue *v = (TzdValue *)inst;
        if (!v || !name)
            return g_JitPool.next();
        if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
        {
            if (TzdValue *direct = v->classDefVal->findStaticValue(name))
                return direct;
            if (ClassMethod *method = v->classDefVal->findMethod(name))
            {
                return tzd_make_class_method_value(method);
            }
        }
        if (v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            if (TzdValue *direct = v->instanceVal->getMemberPtr(name))
            {
                return direct;
            }
            if (v->instanceVal->definition)
            {
                if (ClassMethod *method = v->instanceVal->definition->findMethod(name))
                {
                    return tzd_make_bound_method_value(v->instanceVal, method);
                }
            }
        }
        if (v->type == TzdValue::MAP)
        {
            auto it = v->mapVal.find(name);
            if (it != v->mapVal.end())
                return &it->second;
            return &v->mapVal[name];
        }
        return g_JitPool.next();
    }

    // selector 热路径：O(1) dispatch table，name 仅作冷路径诊断/回退。
    void *rt_tzd_get_member(void *inst, int32_t selector, const char *name)
    {
        if (!inst)
        {
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("尝试在空对象 (null) 上访问成员: '" + std::string(name ? name : "") + "'");
            }
            return g_JitPool.next();
        }
        TzdValue *v = (TzdValue *)inst;
        if (v->type == TzdValue::MAP && name)
        {
            auto it = v->mapVal.find(name);
            if (it != v->mapVal.end())
                return &it->second;
            return &v->mapVal[name];
        }
        if (selector != 0)
        {
            if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition)
            {
                if (const TzdMemberSlot *slot =
                        v->instanceVal->definition->tzdDispatch.find((TzdSelector)selector))
                {
                    // 实例字段优先：返回活动槽指针（零拷贝）。
                    if (slot->fieldIndex >= 0)
                    {
                        if (TzdValue *direct = v->instanceVal->getMemberPtrByIndex(slot->fieldIndex))
                            return direct;
                    }
                    if (slot->method)
                    {
                        return tzd_make_bound_method_value(v->instanceVal, slot->method);
                    }
                }
            }
            else if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
            {
                if (const TzdMemberSlot *slot =
                        v->classDefVal->tzdDispatch.find((TzdSelector)selector))
                {
                    // 类访问：staticValue 在 method 之前（isStatic 语义）。
                    if (slot->staticValue)
                        return slot->staticValue;
                    if (slot->method)
                        return tzd_make_class_method_value(slot->method);
                }
            }
        }
        // 冷路径：selector 未解析或表未命中 —— name 解析（不再内联，避免递归）。
        return tzd_get_member_by_name(inst, name);
    }

    // 兼容入口：旧 emit 点仍以 name 调用；此处做 slow interning 后委托 selector 路径。
    void *rt_get_member(void *inst, const char *name)
    {
        TzdSelector sel = name ? tzdInternSelector(name) : 0;
        return rt_tzd_get_member(inst, (int32_t)sel, name);
    }

    // name-based 写入核心。
    static void tzd_store_member_by_name(void *inst, const char *name, void *val)
    {
        if (!inst || !val || !name)
            return;
        TzdValue *v = (TzdValue *)inst;
        TzdValue copy = *(TzdValue *)val;
        if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
        {
            if (ClassField *f = v->classDefVal->findField(name))
            {
                if (f->isStatic && !f->type.empty() && f->type != "var" && f->type != "any" && f->type != "auto")
                {
                    TzdValue converted;
                    if (!isTzdValueCompatibleWithType(f->type, copy, &converted))
                    {
                        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                            g_CurrentInterpreter->reportJitError("类型不匹配: 无法将 " + getTzdValueTypeName(copy) + " 类型的值赋给 " + f->type + " 类型的静态成员 '" + name + "' (位于类 " + v->classDefVal->fullName + " 中)");
                        return;
                    }
                    if (TzdValue *direct = v->classDefVal->findStaticValue(name))
                    {
                        *direct = converted;
                        return;
                    }
                }
            }
            if (TzdValue *direct = v->classDefVal->findStaticValue(name))
            {
                *direct = copy;
                return;
            }
        }
        if (v->type == TzdValue::INSTANCE && v->instanceVal)
        {
            try
            {
                v->instanceVal->setMember(name, copy);
            }
            catch (const std::exception &e)
            {
                if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                    g_CurrentInterpreter->reportJitError(e.what());
            }
            return;
        }
        if (v->type == TzdValue::MAP)
        {
            v->mapVal[name] = copy;
            return;
        }
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
        {
            g_CurrentInterpreter->reportJitError("无法给该类型的成员赋值: '" + std::string(name ? name : "") + "'");
        }
    }

    // selector 热路径写入。
    void rt_tzd_store_member(void *inst, int32_t selector, const char *name, void *val)
    {
        if (!inst || !val)
        {
            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
            {
                g_CurrentInterpreter->reportJitError("尝试在空对象 (null) 上赋值属性: '" + std::string(name ? name : "") + "'");
            }
            return;
        }
        TzdValue *v = (TzdValue *)inst;
        TzdValue *src = (TzdValue *)val;
        if (v->type == TzdValue::MAP && name)
        {
            v->mapVal[name] = *src;
            return;
        }
        if (selector != 0)
        {
            if (v->type == TzdValue::INSTANCE && v->instanceVal && v->instanceVal->definition)
            {
                if (const TzdMemberSlot *slot =
                        v->instanceVal->definition->tzdDispatch.find((TzdSelector)selector))
                {
                    if (slot->fieldIndex >= 0)
                    {
                        // setMemberByIndex 经 operator= 全量覆盖，清除上一轮残留
                        // 的 func 元数据（params/body/jittedPtr），避免陈旧。
                        try
                        {
                            v->instanceVal->setMemberByIndex(slot->fieldIndex, *src);
                        }
                        catch (const std::exception &e)
                        {
                            if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                                g_CurrentInterpreter->reportJitError(e.what());
                        }
                        return;
                    }
                }
            }
            else if (v->type == TzdValue::CLASS_DEF && v->classDefVal)
            {
                if (const TzdMemberSlot *slot =
                        v->classDefVal->tzdDispatch.find((TzdSelector)selector))
                {
                    if (slot->staticValue)
                    {
                        std::string memberName = name ? name : tzdGetSelectorName((TzdSelector)selector);
                        if (ClassField *f = v->classDefVal->findField(memberName))
                        {
                            if (f->isStatic && !f->type.empty() && f->type != "var" && f->type != "any" && f->type != "auto")
                            {
                                TzdValue converted;
                                if (!isTzdValueCompatibleWithType(f->type, *src, &converted))
                                {
                                    if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
                                        g_CurrentInterpreter->reportJitError("类型不匹配: 无法将 " + getTzdValueTypeName(*src) + " 类型的值赋给 " + f->type + " 类型的静态成员 (位于类 " + v->classDefVal->fullName + " 中)");
                                    return;
                                }
                                *slot->staticValue = converted;
                                return;
                            }
                        }
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
    void rt_store_field_ptr(void *fieldPtr, void *val)
    {
        if (!fieldPtr || !val)
            return;
        *(TzdValue *)fieldPtr = *(TzdValue *)val;
    }

    // 兼容入口。
    void rt_store_member(void *inst, const char *name, void *val)
    {
        TzdSelector sel = name ? tzdInternSelector(name) : 0;
        rt_tzd_store_member(inst, (int32_t)sel, name, val);
    }

    void *rt_create_lambda_value(const char *lambdaName)
    {
        if (!g_CurrentInterpreter)
            return g_JitPool.next();
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::FUNCTION;
        v->name = lambdaName;
        v->closureScope = std::make_shared<std::unordered_map<std::string, TzdValue>>();
        for (const auto &sc : g_CurrentInterpreter->scopes)
        {
            for (const auto &[k, val] : sc)
            {
                (*v->closureScope)[k] = val;
            }
        }
        if (g_CurrentInterpreter->m_jitEngine)
        {
            v->jittedPtr = (void (*)(void *, void *))g_CurrentInterpreter->m_jitEngine->lookupSymbolAsPtr(lambdaName);
        }
        return v;
    }

    // 强制类型转换
    void *rt_cast(void *valPtr, const char *typeName)
    {
        TzdValue *val = (TzdValue *)valPtr;
        TzdValue *res = g_JitPool.next();
        std::string target(typeName);
        try
        {
            if (target == "int" || target == "i32" || target == "long" || target == "i64")
            {
                res->type = (target == "int" || target == "i32") ? TzdValue::INT : TzdValue::LONG;
                res->lVal = (long long)TzdInterpreter::getAsDoubleInternal(*val);
            }
            else if (target == "float" || target == "double")
            {
                res->type = TzdValue::DOUBLE;
                res->dVal = TzdInterpreter::getAsDoubleInternal(*val);
            }
            else if (target == "string")
            {
                res->type = TzdValue::STRING;
                res->sVal = TzdInterpreter::getAsString(*val);
            }
            else if (target == "bool")
            {
                res->type = TzdValue::BOOL;
                res->bVal = (TzdInterpreter::getAsDoubleInternal(*val) != 0);
            }
            else if (target == "function" || target == "fn")
            {
                if (val->type == TzdValue::FUNCTION || val->type == TzdValue::NATIVE_FUNCTION)
                {
                    *res = *val;
                }
                else
                {
                    res->type = TzdValue::NONE;
                }
            }
            else
            {
                if (val->type == TzdValue::INSTANCE)
                {
                    if (TzdOopManager::isInstanceOf(val->instanceVal, target))
                        *res = *val;
                }
            }
        }
        catch (...)
        {
            res->type = TzdValue::NONE;
        }
        return res;
    }

    void rt_print(void *a)
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError)
            return;
        if (!a)
            return;
        std::cout << Utf8ToAnsi(TzdInterpreter::getAsString(*(TzdValue *)a)) << " ";
    }

    void rt_print_newline()
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError)
            return;
        std::cout << std::endl;
    }

    void *rt_create_native_val(const char *name, void *addr)
    {
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::NATIVE_FUNCTION;
        v->name = name;
        v->ptrVal = addr;
        return v;
    }

    void *rt_create_bigint(const char *digits)
    {
        // Create a BIGINT TzdValue from a decimal digit string
        TzdValue *v = g_JitPool.next();
        if (!digits || !*digits)
        {
            v->type = TzdValue::BIGINT;
            v->sVal = "0";
            return v;
        }
        // Fast path: find start of significant digits (skip sign + leading zeros)
        const char *p = digits;
        bool neg = (*p == '-');
        if (neg || *p == '+')
            p++;
        const char *digitsStart = p;
        while (*p == '0')
            p++;
        if (!*p)
        {
            // All zeros
            v->type = TzdValue::BIGINT;
            v->sVal = "0";
            return v;
        }
        // Build the string in one pass — no redundant copies
        size_t significantLen = strlen(p);
        std::string s;
        s.reserve(significantLen + (neg ? 1 : 0));
        if (neg)
            s.push_back('-');
        s.append(p, significantLen);
        v->type = TzdValue::BIGINT;
        v->sVal = std::move(s);
        return v;
    }

    bool rt_type_check(void *objPtr, const char *typeName)
    {
        if (!objPtr || !typeName)
            return false;
        TzdValue *val = (TzdValue *)objPtr;
        std::string target(typeName);
        if (val->type == TzdValue::INSTANCE)
        {
            return TzdOopManager::isInstanceOf(val->instanceVal, typeName);
        }
        if (target == "function" || target == "fn")
        {
            return val->type == TzdValue::FUNCTION || val->type == TzdValue::NATIVE_FUNCTION;
        }
        if (target == "int" || target == "i32" || target == "long" || target == "i64")
        {
            return val->type >= TzdValue::SBYTE && val->type <= TzdValue::ULONG;
        }
        if (target == "float" || target == "double")
        {
            return val->type == TzdValue::FLOAT || val->type == TzdValue::DOUBLE;
        }
        if (target == "string")
        {
            return val->type == TzdValue::STRING;
        }
        if (target == "bool")
        {
            return val->type == TzdValue::BOOL;
        }
        if (target == "ptr" || target == "pointer" || target == "hwnd")
        {
            return val->type == TzdValue::POINTER || val->type == TzdValue::NONE;
        }
        for (auto &anno : val->annotations)
            if (anno == typeName)
                return true;
        return false;
    }

    void rt_call_super(void *thisValPtr, int argCount, ...)
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError)
            return;
        TzdValue *thisVal = (TzdValue *)thisValPtr;
        TzdInstance *inst = thisVal ? thisVal->instanceVal : nullptr;
        if (!inst || !inst->definition)
        {
            if (g_CurrentInterpreter)
                g_CurrentInterpreter->reportJitError("super() 调用失败: 无效的实例指针");
            return;
        }
        if (inst->definition->parentName.empty())
        {
            if (g_CurrentInterpreter)
                g_CurrentInterpreter->reportJitError("super() 调用失败: 类 '" + inst->definition->fullName + "' 没有父类");
            return;
        }
        TzdClassDef *parent = TzdOopManager::getClass(inst->definition->parentName);
        if (!parent)
        {
            if (g_CurrentInterpreter)
                g_CurrentInterpreter->reportJitError("super() 调用失败: 找不到基类/父类定义 '" + inst->definition->parentName + "'");
            return;
        }

        std::vector<TzdValue> args;
        va_list ap;
        va_start(ap, argCount);
        for (int i = 0; i < argCount; i++)
        {
            args.push_back(*(TzdValue *)va_arg(ap, void *));
        }
        va_end(ap);

        const ClassConstructor *parentCtor = parent->findConstructor(args.size());
        if (!parentCtor)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError("super() 调用失败: 父类 '" + parent->fullName + "' 未找到接受 " + std::to_string(args.size()) + " 个参数的构造函数");
            }
            return;
        }

        TzdValue ctorFunc(parent->simpleName, parentCtor->params, parentCtor->body);
        ctorFunc.jittedPtr = parentCtor->jittedPtr;
        ctorFunc.setInstance(inst);

        // 【修复】：复制源文件与行列元数据，确保继承链的 JIT 堆栈能够正常定位
        ctorFunc.sourceFile = parentCtor->sourceFile;
        ctorFunc.line = parentCtor->line;
        ctorFunc.column = parentCtor->column;

        try
        {
            g_CurrentInterpreter->callFunction(ctorFunc, args);
        }
        catch (const std::exception &e)
        {
            if (g_CurrentInterpreter)
            {
                g_CurrentInterpreter->reportJitError(e.what(), parentCtor->line, parentCtor->column);
            }
        }
    }

    static thread_local TzdValue g_tzdThrownValue;

    void *rt_alloc_jmp_buf()
    {
        return new jmp_buf();
    }

    void *rt_get_fatal_jmp() { return g_tzdFatalJmp; }
    void rt_set_fatal_jmp(void *buf) { g_tzdFatalJmp = static_cast<jmp_buf *>(buf); }

    void rt_free_jmp_buf(void *buf) { delete static_cast<jmp_buf *>(buf); }

    /*int rt_enter_try_buf(void* buf) {
        g_tzdCatchJmp = static_cast<jmp_buf*>(buf);
        return setjmp(*g_tzdCatchJmp);
    }
    void rt_leave_try() {
        g_tzdCatchJmp = nullptr;
    }
    */

    void *rt_get_catch_jmp()
    {
        return g_tzdCatchJmp;
    }

    void rt_set_catch_jmp(void *buf)
    {
        g_tzdCatchJmp = static_cast<jmp_buf *>(buf);
    }

    void rt_throw(void *valPtr)
    {
        if (g_CurrentInterpreter && g_CurrentInterpreter->m_hasJitError)
            return;

        if (valPtr)
            g_tzdThrownValue = *(TzdValue *)valPtr;
        else
            g_tzdThrownValue = TzdValue("Unknown error");

        std::vector<std::string> currentTrace;
        if (g_CurrentInterpreter)
        {
            currentTrace = g_CurrentInterpreter->m_callStackFrames;
            currentTrace.push_back("<throw> at line " + std::to_string(g_CurrentInterpreter->m_jitLine));
        }

        // 如果 JIT 环境或者外部设置了捕获点，执行跳转
        if (g_tzdCatchJmp)
        {
            longjmp(*g_tzdCatchJmp, 1);
        }

        // 否则作为未捕获异常抛出给 C++ 外层解释器处理
        if (g_CurrentInterpreter)
        {
            g_CurrentInterpreter->m_jitUnhandledThrow = std::make_unique<TzdThrowException>(g_tzdThrownValue, currentTrace);
            g_CurrentInterpreter->m_hasJitError = true;
        }
    }

    void *rt_get_thrown()
    {
        return &g_tzdThrownValue;
    }

    void rt_push_catch_scope(const char *name, void *valPtr)
    {
        if (!g_CurrentInterpreter || !name || !valPtr)
            return;
        std::unordered_map<std::string, TzdValue> scope;
        scope[name] = *(TzdValue *)valPtr;
        g_CurrentInterpreter->scopes.push_back(scope);
    }

    void rt_pop_catch_scope()
    {
        if (!g_CurrentInterpreter || g_CurrentInterpreter->scopes.empty())
            return;
        g_CurrentInterpreter->scopes.pop_back();
    }

    double rt_to_double_fast(void *v)
    {
        if ((uintptr_t)v < 4096)
            return (double)(uintptr_t)v;

        TzdValue *val = (TzdValue *)v;
        if (val->type == TzdValue::DOUBLE || val->type == TzdValue::FLOAT)
            return val->dVal;
        if (val->type == TzdValue::INT || val->type == TzdValue::LONG)
            return (double)val->lVal;
        if (val->type == TzdValue::BOOL)
            return val->bVal ? 1.0 : 0.0;
        return 0.0;
    }

    void *rt_get_fast_buf(void *v)
    {
        if (!v)
            return nullptr;
        return ((TzdValue *)v)->fastBuf;
    }
    double rt_get_fast_len(void *v)
    {
        if (!v)
            return 0.0;
        return (double)((TzdValue *)v)->fastLen;
    }

    void *rt_stabilize_value(void *v)
    {
        if (!v)
            return nullptr;
        TzdValue *src = (TzdValue *)v;
        if (!g_JitPool.contains(src))
        {
            return src;
        }
        TzdValue *stable = new TzdValue();
        *stable = std::move(*src);
        stable->syncFastBuf();
        if (g_CurrentInterpreter)
        {
            g_CurrentInterpreter->trackJitValue(stable);
        }
        return stable;
    }

    void *rt_fast_range(double start, double end, double step)
    {
        if (step == 0.0)
            step = 1.0;
        TzdValue *res = new TzdValue();
        res->type = TzdValue::ARRAY;
        res->isNativeDoubleArr = true;
        int count = 0;
        if (step > 0.0 && end > start)
        {
            count = (int)std::ceil((end - start) / step);
        }
        else if (step < 0.0 && start > end)
        {
            count = (int)std::ceil((start - end) / (-step));
        }
        if (count > 0)
        {
            res->nativeArr.resize((size_t)count);
            double *data = res->nativeArr.data();
            if (step == 1.0)
            {
#if defined(_M_X64) || defined(__x86_64__)
                int i = 0;
                while (i < count && (reinterpret_cast<uintptr_t>(data + i) & 31) != 0)
                {
                    data[i] = start + (double)i;
                    i++;
                }

                __m256d vOffsets0 = _mm256_set_pd(3.0, 2.0, 1.0, 0.0);
                __m256d vOffsets1 = _mm256_set_pd(7.0, 6.0, 5.0, 4.0);
                __m256d vOffsets2 = _mm256_set_pd(11.0, 10.0, 9.0, 8.0);
                __m256d vOffsets3 = _mm256_set_pd(15.0, 14.0, 13.0, 12.0);
                __m256d vStep16 = _mm256_set1_pd(16.0);
                __m256d vBase = _mm256_set1_pd(start + (double)i);
                __m256d vVal0 = _mm256_add_pd(vBase, vOffsets0);
                __m256d vVal1 = _mm256_add_pd(vBase, vOffsets1);
                __m256d vVal2 = _mm256_add_pd(vBase, vOffsets2);
                __m256d vVal3 = _mm256_add_pd(vBase, vOffsets3);
                int limit16 = count - 15;

                if (count >= 4096)
                {
                    for (; i < limit16; i += 16)
                    {
                        _mm256_stream_pd(data + i, vVal0);
                        _mm256_stream_pd(data + i + 4, vVal1);
                        _mm256_stream_pd(data + i + 8, vVal2);
                        _mm256_stream_pd(data + i + 12, vVal3);
                        vVal0 = _mm256_add_pd(vVal0, vStep16);
                        vVal1 = _mm256_add_pd(vVal1, vStep16);
                        vVal2 = _mm256_add_pd(vVal2, vStep16);
                        vVal3 = _mm256_add_pd(vVal3, vStep16);
                    }
                    _mm_sfence();
                }
                else
                {
                    for (; i < limit16; i += 16)
                    {
                        _mm256_store_pd(data + i, vVal0);
                        _mm256_store_pd(data + i + 4, vVal1);
                        _mm256_store_pd(data + i + 8, vVal2);
                        _mm256_store_pd(data + i + 12, vVal3);
                        vVal0 = _mm256_add_pd(vVal0, vStep16);
                        vVal1 = _mm256_add_pd(vVal1, vStep16);
                        vVal2 = _mm256_add_pd(vVal2, vStep16);
                        vVal3 = _mm256_add_pd(vVal3, vStep16);
                    }
                }
                for (; i < count; ++i)
                {
                    data[i] = start + (double)i;
                }
#else
                for (int i = 0; i < count; ++i)
                    data[i] = start + i;
#endif
            }
            else
            {
                double v = start;
                for (int i = 0; i < count; ++i)
                {
                    data[i] = v;
                    v += step;
                }
            }
        }
        res->syncFastBuf();
        if (g_CurrentInterpreter)
        {
            g_CurrentInterpreter->trackJitValue(res);
        }
        return res;
    }

    // 直接用原生 double 索引读取，无装箱开销
    double rt_get_index_native_d(void *arr, int idx)
    {
        TzdValue *vArr = (TzdValue *)arr;
        if (!vArr)
            return 0.0;
        if (vArr->isNativeDoubleArr && idx >= 0 && idx < (int)vArr->nativeArr.size())
            return vArr->nativeArr[idx];
        // fallback：boxed array
        if (vArr->type == TzdValue::ARRAY && idx >= 0 && idx < (int)vArr->arrVal.size())
        {
            const TzdValue &target = vArr->arrVal[idx];
            if (target.type == TzdValue::DOUBLE || target.type == TzdValue::FLOAT)
                return target.dVal;
            if (target.type == TzdValue::BOOL)
                return target.bVal ? 1.0 : 0.0;
            return TzdInterpreter::getAsDoubleInternal(target);
        }
        if (g_CurrentInterpreter && !g_CurrentInterpreter->m_hasJitError)
        {
            int len = vArr->isNativeDoubleArr ? (int)vArr->nativeArr.size() : (vArr->type == TzdValue::ARRAY ? (int)vArr->arrVal.size() : 0);
            g_CurrentInterpreter->reportJitError("数组索引越界: 尝试访问索引 " + std::to_string(idx) +
                                                 ", 但数组长度为 " + std::to_string(len));
        }
        return 0.0;
    }

    // 直接写原生 double，无装箱开销
    void rt_store_index_native_d(void *arr, int idx, double val)
    {
        if (!arr)
            return;
        TzdValue *vArr = (TzdValue *)arr;
        if (vArr->isNativeDoubleArr && idx >= 0 && idx < (int)vArr->nativeArr.size())
        {
            vArr->nativeArr[idx] = val;
            return;
        }
        if (vArr->type == TzdValue::ARRAY && idx >= 0 && idx < (int)vArr->arrVal.size())
        {
            TzdValue &target = vArr->arrVal[idx];
            target.type = TzdValue::DOUBLE;
            target.dVal = val;
            return;
        }
        // fallback：转换为 TzdValue 再写
        TzdValue boxed(val);
        TzdValue idxVal((int)idx);
        rt_store_index(arr, &idxVal, &boxed);
    }

    double rt_get_index_native_d_dyn(void *arr, double idxD)
    {
        return rt_get_index_native_d(arr, (int)idxD);
    }

    double rt_get_index_native_d_ptr(void *arr, void *idx)
    {
        if (!arr || !idx)
            return 0.0;
        TzdValue *vArr = (TzdValue *)arr;
        TzdValue *vIdx = (TzdValue *)idx;
        if (vArr->type == TzdValue::MAP)
        {
            const std::string &key = (vIdx->type == TzdValue::STRING || vIdx->type == TzdValue::BIGINT || vIdx->type == TzdValue::RATIONAL)
                                         ? vIdx->sVal
                                         : TzdInterpreter::getAsString(*vIdx);
            if (vArr->ptrVal)
            {
                auto *m = (TzdFastDoubleMap *)vArr->ptrVal;
                uint32_t h = (vIdx->type == TzdValue::STRING && vIdx->ulVal != 0) ? (uint32_t)vIdx->ulVal : 0;
                double outVal = 0.0;
                if (m->find(key, outVal, h))
                    return outVal;
            }
            auto it = vArr->mapVal.find(key);
            if (it != vArr->mapVal.end())
            {
                if (it->second.type == TzdValue::DOUBLE || it->second.type == TzdValue::FLOAT)
                    return it->second.dVal;
                if (it->second.type == TzdValue::INT || it->second.type == TzdValue::LONG)
                    return (double)it->second.lVal;
                return TzdInterpreter::getAsDoubleInternal(it->second);
            }
            return 0.0;
        }
        return 0.0;
    }

    void rt_store_index_native_d_dyn(void *arr, double idxD, double val)
    {
        rt_store_index_native_d(arr, (int)idxD, val);
    }

    // 创建纯 double 数组字面量的快速路径
    void *rt_create_native_double_arr(int count, double *vals)
    {
        TzdValue *v = g_JitPool.next();
        v->type = TzdValue::ARRAY;
        v->isNativeDoubleArr = true;
        v->nativeArr.resize(count);
        for (int i = 0; i < count; i++)
            v->nativeArr[i] = vals[i];
        v->syncFastBuf();
        return v;
    }
}

llvm::AllocaInst *TzdCompiler::CreateEntryBlockAlloca(llvm::Type *Ty, const std::string &Name, llvm::Value *ArraySize)
{
    Function *TheFunction = m_builder.GetInsertBlock()->getParent();
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Ty, ArraySize, Name);
}

llvm::AllocaInst *TzdCompiler::CreateEntryBlockAlloca(llvm::Type *Ty, llvm::Value *ArraySize, const std::string &Name)
{
    llvm::Function *TheFunction = m_builder.GetInsertBlock()->getParent();
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Ty, ArraySize, Name);
}

void TzdCompiler::compileClassMethod(TzdLangParser::ClassDeclarationContext *classCtx,
                                     TzdLangParser::MethodDeclContext *methodCtx)
{
    std::string className = classCtx->qualifiedName(0)->getText();
    std::string methodName = methodCtx->IDENTIFIER()->getText();
    compileClassMethod(classCtx, methodCtx, className + "_" + methodName);
}

// Add this method to your TzdJitEngine
void TzdJitEngine::jitModule(std::unique_ptr<Module> M)
{
    if (!M)
        return;

    // 收集刚编译的模块中包含的所有 _worker 函数名
    std::vector<std::string> workers;
    for (auto &F : *M)
    {
        std::string name = F.getName().str();
        if (name.size() > 7 && name.substr(name.size() - 7) == "_worker")
        {
            workers.push_back(name);
        }
    }

    ThreadSafeModule TSM(std::move(M), m_tsc);
    addModule(std::move(TSM));

    // 解析出实际的机器码内存地址并存入映射表
    for (const auto &w : workers)
    {
        if (void *ptr = lookupSymbolAsPtr(w))
        {
            // 剥离 "_worker" 以及版本号 "_vX"
            std::string baseName = w.substr(0, w.size() - 7);
            size_t lastUnderscore = baseName.find_last_of('_');
            if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v')
            {
                baseName = baseName.substr(0, lastUnderscore);
            }
            std::unique_lock<std::shared_mutex> lock(s_workerPointersMutex);
            s_workerPointers[baseName] = ptr;
            s_workerPointers[w.substr(0, w.size() - 7)] = ptr;
            s_workerPointers[w] = ptr;
        }
    }
}

std::unique_ptr<llvm::Module> TzdCompiler::extractModule()
{
    return std::move(m_module);
}

llvm::Expected<llvm::orc::ExecutorAddr>
TzdJitEngine::lookupSymbol(const std::string &name)
{
    return m_lljit->lookup(name);
}

void *TzdJitEngine::lookupSymbolAsPtr(const std::string &name)
{
    auto Sym = lookupSymbol(name);
    if (!Sym)
    {
        llvm::Error E = Sym.takeError();
        consumeError(std::move(E));
        return nullptr;
    }
    llvm::orc::ExecutorAddr addr = *Sym;
    llvm::JITTargetAddress raw = addr.getValue();
    return reinterpret_cast<void *>(static_cast<uintptr_t>(raw));
}

// Forward declarations for function-level array hoisting (definitions below ~line 6279)
static void collectIndexedContainers(antlr4::tree::ParseTree *tree, std::set<std::string> &containers);
static void collectAssignedVariables(antlr4::tree::ParseTree *tree, std::set<std::string> &vars);
static void collectDeclaredVariables(antlr4::tree::ParseTree *tree, std::set<std::string> &vars);

void TzdCompiler::compileNamedFunction(TzdLangParser::FunctionDeclarationContext *ctx, const std::string &internalName)
{
    std::string savedCompilingFunc = s_currentCompilingFuncName;
    std::string baseInternal = internalName;
    size_t uPos = baseInternal.find_last_of('_');
    if (uPos != std::string::npos && uPos + 1 < baseInternal.size() && baseInternal[uPos + 1] == 'v')
    {
        baseInternal = baseInternal.substr(0, uPos);
    }
    s_currentCompilingFuncName = baseInternal;
    s_currentFunctionBlock = ctx->block();
    s_currentFunctionParams.clear();
    if (ctx->paramList())
    {
        for (auto p : ctx->paramList()->param())
        {
            s_currentFunctionParams.push_back(getParamName(p));
        }
    }

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;

    int argCount = ctx->paramList() ? ctx->paramList()->param().size() : 0;

    // 1. Worker 内部函数返回类型为 m_doubleTy
    std::vector<Type *> workerArgs = {m_ptrTy, m_ptrTy, m_ptrTy};
    Function *workerFunc = Function::Create(
        FunctionType::get(m_doubleTy, workerArgs, false),
        Function::ExternalLinkage,
        internalName + "_worker",
        m_module.get());
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    // Phase B: Create native double worker for function specialization.
    // Only for functions with parameters — 0-param functions have no benefit.
    Function *nativeWorkerFunc = nullptr;
    bool hasNonNumericParam = false;
    if (ctx->paramList())
    {
        auto pList = ctx->paramList()->param();
        for (int i = 0; i < argCount; ++i)
        {
            std::string pName = getParamName(pList[i]);
            if (isExplicitNumericParam(pList[i]))
            {
                continue;
            }
            if (isNonNumericParam(pList[i]) || isParamNonDouble(ctx->block(), pName))
            {
                hasNonNumericParam = true;
                break;
            }
        }
    }

    std::string explicitRetType = ctx->typeType() ? ctx->typeType()->getText() : "";
    bool returnsDouble = !explicitRetType.empty()
                             ? isExplicitNumericType(explicitRetType)
                             : isBlockReturningDouble(ctx->block());

    if (argCount >= 0 && !hasNonNumericParam && returnsDouble)
    {
        std::vector<Type *> nativeWorkerArgs;
        for (int i = 0; i < argCount; ++i)
        {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get());
        nativeWorkerFunc->addFnAttr(llvm::Attribute::InlineHint);
        nativeWorkerFunc->setCallingConv(llvm::CallingConv::Fast);
        if (isTreePureNumeric(ctx->block(), baseInternal))
        {
            nativeWorkerFunc->setDoesNotAccessMemory();
            nativeWorkerFunc->setDoesNotThrow();
            nativeWorkerFunc->addFnAttr(llvm::Attribute::WillReturn);
        }
    }

    s_currentNativeWorkerFunc = nativeWorkerFunc;

    // 2. Entry 依然返回 void
    std::vector<Type *> entryArgs = {m_ptrTy, m_ptrTy};
    Function *entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get());

    BasicBlock *entryBB = BasicBlock::Create(m_context, "entry", entryFunc);
    m_builder.SetInsertPoint(entryBB);
    Value *interp = entryFunc->getArg(0);
    Value *retVal = entryFunc->getArg(1);
    Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "entry_args");
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
    }

    if (ctx->paramList())
    {
        for (int i = 0; i < argCount; ++i)
        {
            Value *argIdx = ConstantInt::get(Type::getInt32Ty(m_context), i);
            Value *argRaw = m_builder.CreateCall(getRtFunc("rt_get_arg"), {argIdx});
            Value *destPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {destPtr, argRaw});
        }
    }
    m_builder.CreateCall(workerFunc, {interp, retVal, argsArray});
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
    }
    m_builder.CreateRetVoid();

    BasicBlock *workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    Value *isOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {workerFunc->getArg(0)});
    BasicBlock *ofBB = BasicBlock::Create(m_context, "rec_overflow", workerFunc);
    BasicBlock *okBB = BasicBlock::Create(m_context, "rec_ok", workerFunc);
    m_builder.CreateCondBr(isOverflow, ofBB, okBB);

    m_builder.SetInsertPoint(ofBB);
    m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));

    m_builder.SetInsertPoint(okBB);

    if (nativeWorkerFunc)
    {
        Value *workerRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2);
        std::vector<Value *> nativeArgs;
        for (int i = 0; i < argCount; ++i)
        {
            Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
            nativeArgs.push_back(inlineToDoubleFast(argPtr));
        }
        CallInst *res = m_builder.CreateCall(nativeWorkerFunc, nativeArgs);
        res->setCallingConv(llvm::CallingConv::Fast);
        m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
        inlineStoreNativeToPtr(workerRetPtr, res);
        m_builder.CreateRet(res);
    }
    else
    {
        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_varDeclaredTypes.clear();

        this->m_currentRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2); // 这里正确定义了 workerArgsArray

        BasicBlock *bodyBB = BasicBlock::Create(m_context, "body", workerFunc);
        s_tailRecurseBB = bodyBB;

        if (ctx->paramList())
        {
            auto pList = ctx->paramList()->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                std::string pType = getParamType(pList[i]);
                if (!pType.empty())
                {
                    m_varDeclaredTypes[pName] = pType;
                    if (TzdOopManager::getClass(pType) != nullptr)
                    {
                        m_varClassTypes[pName] = pType;
                    }
                }
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
                AllocaInst *boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
                m_builder.CreateStore(argPtr, boxedAlloc);
                m_namedValues[pName] = boxedAlloc;

                // Native double version — unbox at entry for fast numeric access + native worker
                bool isNumeric = isExplicitNumericParam(pList[i]) ||
                                 (returnsDouble && !isNonNumericParam(pList[i]) && !isParamUsedAsContainer(ctx->block(), pName));
                if (isNumeric)
                {
                    Value *nativeVal = inlineToDoubleFast(argPtr);
                    AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                    m_builder.CreateStore(nativeVal, nativeAlloc);
                    m_nativeDoubleLocals[pName] = nativeAlloc;
                }
            }
        }

        m_hoistedArrays.clear();
        std::set<std::string> fnIndexedContainers;
        collectIndexedContainers(ctx->block(), fnIndexedContainers);
        std::set<std::string> fnAssignedVars;
        collectAssignedVariables(ctx->block(), fnAssignedVars);
        std::set<std::string> fnDeclaredVars;
        collectDeclaredVariables(ctx->block(), fnDeclaredVars);
        for (const std::string &cName : fnIndexedContainers)
        {
            if (fnAssignedVars.count(cName) == 0 && fnDeclaredVars.count(cName) == 0)
            {
                Value *container = nullptr;
                if (m_namedValues.count(cName) > 0)
                {
                    container = m_builder.CreateLoad(m_ptrTy, m_namedValues[cName], cName + "_fn_cont");
                }
                else
                {
                    Value *nameStr = m_builder.CreateGlobalStringPtr(cName);
                    int line = ctx->block()->getStart() ? (int)ctx->block()->getStart()->getLine() : 0;
                    int col = ctx->block()->getStart() ? (int)ctx->block()->getStart()->getCharPositionInLine() : 0;
                    container = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr, m_builder.getInt32(line), m_builder.getInt32(col)});
                }
                Value *fastBuf = m_builder.CreateLoad(m_ptrTy, container, cName + "_fn_buf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, cName + "_fn_lenPtr");
                Value *fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, cName + "_fn_len");
                Value *hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), cName + "_fn_has_buf");
                m_hoistedArrays[cName] = {container, fastBuf, fastLen, hasBuf, "", nullptr, false};
            }
        }

        m_builder.CreateBr(bodyBB);
        m_builder.SetInsertPoint(bodyBB);

        visit(ctx->block());
        m_hoistedArrays.clear();

        // 默认返回硬件级的 0.0
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
            Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, nullVal});
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
    }
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;

    // Phase B: Compile native worker body — params are direct doubles, no TzdValue unbox
    if (nativeWorkerFunc)
    {
        s_compilingNativeWorker = true;
        BasicBlock *nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_currentClassDef = nullptr;
        m_varFieldAllocas.clear();
        m_lastNewFieldAllocas.clear();

        // Native worker has no retVal slot — returns double directly
        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        BasicBlock *nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        s_tailRecurseBB = nativeBodyBB;

        if (ctx->paramList())
        {
            auto pList = ctx->paramList()->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                std::string pType = getParamType(pList[i]);
                if (!pType.empty() && TzdOopManager::getClass(pType) != nullptr)
                {
                    m_varClassTypes[pName] = pType;
                }
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                // Store double arg into alloca — mem2reg will promote to register.
                // Using alloca so tail-recursion path (CreateStore) works correctly.
                AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeWorkerFunc->getArg(i), nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_hoistedArrays.clear();
        std::set<std::string> fnIndexedContainers;
        collectIndexedContainers(ctx->block(), fnIndexedContainers);
        std::set<std::string> fnAssignedVars;
        collectAssignedVariables(ctx->block(), fnAssignedVars);
        std::set<std::string> fnDeclaredVars;
        collectDeclaredVariables(ctx->block(), fnDeclaredVars);
        for (const std::string &cName : fnIndexedContainers)
        {
            if (fnAssignedVars.count(cName) == 0 && fnDeclaredVars.count(cName) == 0)
            {
                Value *container = nullptr;
                if (m_namedValues.count(cName) > 0)
                {
                    container = m_builder.CreateLoad(m_ptrTy, m_namedValues[cName], cName + "_fn_cont");
                }
                else
                {
                    Value *nameStr = m_builder.CreateGlobalStringPtr(cName);
                    int line = ctx->block()->getStart() ? (int)ctx->block()->getStart()->getLine() : 0;
                    int col = ctx->block()->getStart() ? (int)ctx->block()->getStart()->getCharPositionInLine() : 0;
                    container = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr, m_builder.getInt32(line), m_builder.getInt32(col)});
                }
                Value *fastBuf = m_builder.CreateLoad(m_ptrTy, container, cName + "_fn_buf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, cName + "_fn_lenPtr");
                Value *fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, cName + "_fn_len");
                Value *hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), cName + "_fn_has_buf");
                m_hoistedArrays[cName] = {container, fastBuf, fastLen, hasBuf, "", nullptr, false};
            }
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(ctx->block());
        m_hoistedArrays.clear();

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
        s_compilingNativeWorker = false;
    }

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_currentFuncParamNames.clear();
    m_currentRetPtr = nullptr;
    s_inTailPosition = false;
    s_currentFunctionBlock = nullptr;
    s_currentFunctionParams.clear();
    s_currentCompilingFuncName = savedCompilingFunc;
}

void TzdCompiler::compileClassMethod(TzdLangParser::ClassDeclarationContext *classCtx, TzdLangParser::MethodDeclContext *methodCtx, const std::string &internalName)
{
    std::string className = classCtx->qualifiedName(0)->getText();
    std::string methodName = methodCtx->IDENTIFIER()->getText();
    int argCount = methodCtx->paramList() ? (int)methodCtx->paramList()->param().size() : 0;

    // Check if parameters are all numeric (for native worker specialization)
    Function *nativeWorkerFunc = nullptr;
    bool hasNonNumericParam = false;
    if (methodCtx->paramList())
    {
        auto pList = methodCtx->paramList()->param();
        for (int i = 0; i < argCount; ++i)
        {
            std::string pName = getParamName(pList[i]);
            if (isExplicitNumericParam(pList[i]))
            {
                continue;
            }
            if (isNonNumericParam(pList[i]) || isParamNonDouble(methodCtx->block(), pName))
            {
                hasNonNumericParam = true;
                break;
            }
        }
    }

    std::string explicitRetType = methodCtx->typeType() ? methodCtx->typeType()->getText() : "";
    bool returnsDouble = !explicitRetType.empty()
                             ? isExplicitNumericType(explicitRetType)
                             : isBlockReturningDouble(methodCtx->block());

    if (argCount > 0 && !hasNonNumericParam && returnsDouble)
    {
        // Native worker: (interp, this, double arg0, double arg1, ...) -> double
        std::vector<Type *> nativeWorkerArgs = {m_ptrTy, m_ptrTy};
        for (int i = 0; i < argCount; ++i)
        {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get());
        nativeWorkerFunc->addFnAttr(llvm::Attribute::AlwaysInline);
        nativeWorkerFunc->setCallingConv(llvm::CallingConv::Fast);

        BasicBlock *nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        BasicBlock *nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        Value *nativeIsOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {nativeWorkerFunc->getArg(0)});
        BasicBlock *nativeOfBB = BasicBlock::Create(m_context, "native_rec_overflow", nativeWorkerFunc);
        BasicBlock *nativeOkBB = BasicBlock::Create(m_context, "native_rec_ok", nativeWorkerFunc);
        m_builder.CreateCondBr(nativeIsOverflow, nativeOfBB, nativeOkBB);

        m_builder.SetInsertPoint(nativeOfBB);
        m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));

        m_builder.SetInsertPoint(nativeOkBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_varClassTypes["this"] = className;
        m_currentClassDef = TzdOopManager::getClass(className);

        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        s_tailRecurseBB = nativeBodyBB;
        s_currentNativeWorkerFunc = nativeWorkerFunc;

        // Bind this
        AllocaInst *thisAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, "this");
        m_builder.CreateStore(nativeWorkerFunc->getArg(1), thisAlloc);
        m_namedValues["this"] = thisAlloc;
        m_declaredLocals.insert("this");

        if (methodCtx->paramList())
        {
            auto pList = methodCtx->paramList()->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);
                AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeWorkerFunc->getArg(2 + i), nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(methodCtx->block());

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {nativeWorkerFunc->getArg(0)});
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
        s_currentNativeWorkerFunc = nullptr;
        s_tailRecurseBB = nullptr;
    }

    // Entry wrapper (void(interp, retPtr)) for dynamic runtime / interpreter compatibility
    Function *func = Function::Create(
        FunctionType::get(m_voidTy, {m_ptrTy, m_ptrTy}, false),
        Function::ExternalLinkage, internalName, m_module.get());
    BasicBlock *bb = BasicBlock::Create(m_context, "entry", func);
    m_builder.SetInsertPoint(bb);
    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    s_currentFuncParamNames.clear();
    m_varClassTypes.clear();
    m_varClassTypes["this"] = className;
    m_currentClassDef = TzdOopManager::getClass(className);
    m_declaredLocals.insert("this");
    m_currentRetPtr = func->getArg(1);

    if (nativeWorkerFunc)
    {
        Value *interp = func->getArg(0);
        Value *thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(0)});
        std::vector<Value *> callArgs = {interp, thisVal};
        for (int i = 0; i < argCount; ++i)
        {
            Value *argVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(i + 1)});
            callArgs.push_back(inlineToDoubleFast(argVal));
        }
        CallInst *dRes = m_builder.CreateCall(nativeWorkerFunc, callArgs);
        dRes->setCallingConv(llvm::CallingConv::Fast);
        inlineStoreNativeToPtr(m_currentRetPtr, dRes);
        m_builder.CreateRetVoid();
    }
    else
    {
        Value *isOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {func->getArg(0)});
        BasicBlock *funcOfBB = BasicBlock::Create(m_context, "func_rec_overflow", func);
        BasicBlock *funcOkBB = BasicBlock::Create(m_context, "func_rec_ok", func);
        m_builder.CreateCondBr(isOverflow, funcOfBB, funcOkBB);

        m_builder.SetInsertPoint(funcOfBB);
        m_builder.CreateRetVoid();

        m_builder.SetInsertPoint(funcOkBB);

        Value *thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(0)});
        AllocaInst *thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
        m_builder.CreateStore(thisVal, thisAlloc);
        m_namedValues["this"] = thisAlloc;

        if (methodCtx->paramList())
        {
            auto pList = methodCtx->paramList()->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                std::string pType = getParamType(pList[i]);
                if (!pType.empty() && TzdOopManager::getClass(pType) != nullptr)
                {
                    m_varClassTypes[pName] = pType;
                }
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);
                Value *argVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(i + 1)});
                AllocaInst *alloc = m_builder.CreateAlloca(m_ptrTy, nullptr, pName);
                m_builder.CreateStore(argVal, alloc);
                m_namedValues[pName] = alloc;

                bool isNumeric = isExplicitNumericParam(pList[i]) ||
                                 (returnsDouble && !isNonNumericParam(pList[i]) && !isParamUsedAsContainer(methodCtx->block(), pName));
                if (isNumeric)
                {
                    Value *nativeVal = inlineToDoubleFast(argVal);
                    AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                    m_builder.CreateStore(nativeVal, nativeAlloc);
                    m_nativeDoubleLocals[pName] = nativeAlloc;
                }
            }
        }

        m_namedValues["$retval"] = m_builder.CreateAlloca(m_ptrTy, nullptr, "$retval");
        visit(methodCtx->block());
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {func->getArg(0)});
            Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, nullVal});
            m_builder.CreateRetVoid();
        }
    }
    s_currentFuncParamNames.clear();
    m_currentRetPtr = nullptr;
    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;
}

void TzdCompiler::compileConstructor(TzdLangParser::ClassDeclarationContext *classCtx, TzdLangParser::ConstructorDeclContext *ctorCtx, const std::string &internalName)
{
    Function *func = Function::Create(
        FunctionType::get(m_voidTy, {m_ptrTy, m_ptrTy}, false),
        Function::ExternalLinkage, internalName, m_module.get());
    BasicBlock *bb = BasicBlock::Create(m_context, "entry", func);
    m_builder.SetInsertPoint(bb);

    Value *isOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {func->getArg(0)});
    BasicBlock *ctorOfBB = BasicBlock::Create(m_context, "ctor_rec_overflow", func);
    BasicBlock *ctorOkBB = BasicBlock::Create(m_context, "ctor_rec_ok", func);
    m_builder.CreateCondBr(isOverflow, ctorOfBB, ctorOkBB);

    m_builder.SetInsertPoint(ctorOfBB);
    m_builder.CreateRetVoid();

    m_builder.SetInsertPoint(ctorOkBB);

    m_namedValues.clear();
    m_nativeDoubleLocals.clear();
    m_declaredLocals.clear();
    s_currentFuncParamNames.clear();
    m_varClassTypes.clear();
    std::string className = classCtx->qualifiedName(0)->getText();
    m_varClassTypes["this"] = className;
    m_currentClassDef = TzdOopManager::getClass(className);
    m_declaredLocals.insert("this");
    m_currentRetPtr = func->getArg(1);

    Value *thisVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(0)});
    AllocaInst *thisAlloc = m_builder.CreateAlloca(m_ptrTy, nullptr, "this");
    m_builder.CreateStore(thisVal, thisAlloc);
    m_namedValues["this"] = thisAlloc;

    if (ctorCtx->paramList())
    {
        auto pList = ctorCtx->paramList()->param();
        for (int i = 0; i < (int)pList.size(); ++i)
        {
            std::string pName = getParamName(pList[i]);
            s_currentFuncParamNames.push_back(pName);
            m_declaredLocals.insert(pName);
            Value *argVal = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(i + 1)});
            AllocaInst *alloc = m_builder.CreateAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argVal, alloc);
            m_namedValues[pName] = alloc;

            if (isExplicitNumericParam(pList[i]))
            {
                Value *nativeVal = inlineToDoubleFast(argVal);
                AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeVal, nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }
    }

    m_namedValues["$retval"] = m_builder.CreateAlloca(m_ptrTy, nullptr, "$retval");
    for (auto stmt : ctorCtx->block()->statement())
    {
        try
        {
            visit(stmt);
        }
        catch (const std::bad_any_cast &)
        {
            // agentLogJit("D", "compileConstructor.stmt", stmt->getText().substr(0, 80).c_str());
            throw;
        }
    }
    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {func->getArg(0)});
        Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
        m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, nullVal});
        m_builder.CreateRetVoid();
    }
    s_currentFuncParamNames.clear();
    m_currentRetPtr = nullptr;
}

void TzdCompiler::compileNamedFunction(TzdLangParser::BlockContext *block, TzdLangParser::ParamListContext *params, const std::string &internalName, const std::string &explicitReturnType)
{
    std::string savedCompilingFunc = s_currentCompilingFuncName;
    std::string baseInternal = internalName;
    size_t uPos = baseInternal.find_last_of('_');
    if (uPos != std::string::npos && uPos + 1 < baseInternal.size() && baseInternal[uPos + 1] == 'v')
    {
        baseInternal = baseInternal.substr(0, uPos);
    }
    s_currentCompilingFuncName = baseInternal;
    s_currentFunctionBlock = block;
    s_currentFunctionParams.clear();
    if (params)
    {
        for (auto p : params->param())
        {
            s_currentFunctionParams.push_back(getParamName(p));
        }
    }

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;

    int argCount = params ? params->param().size() : 0;

    // 1. Worker 内部函数返回类型为 m_doubleTy
    std::vector<Type *> workerArgs = {m_ptrTy, m_ptrTy, m_ptrTy};
    Function *workerFunc = Function::Create(
        FunctionType::get(m_doubleTy, workerArgs, false),
        Function::ExternalLinkage,
        internalName + "_worker",
        m_module.get());
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    bool returnsDouble = !explicitReturnType.empty()
                             ? isExplicitNumericType(explicitReturnType)
                             : isBlockReturningDouble(block);

    // Phase B: Create native double worker for function specialization.
    // Only for functions with parameters — 0-param functions have no benefit.
    Function *nativeWorkerFunc = nullptr;
    bool hasNonNumericParam = false;
    if (params)
    {
        auto pList = params->param();
        for (int i = 0; i < argCount; ++i)
        {
            std::string pName = getParamName(pList[i]);
            if (isExplicitNumericParam(pList[i]))
            {
                continue;
            }
            if (isNonNumericParam(pList[i]) || isParamNonDouble(block, pName))
            {
                hasNonNumericParam = true;
                break;
            }
        }
    }
    if (argCount > 0 && !hasNonNumericParam && returnsDouble)
    {
        std::vector<Type *> nativeWorkerArgs;
        for (int i = 0; i < argCount; ++i)
        {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get());
        nativeWorkerFunc->addFnAttr(llvm::Attribute::InlineHint);
        nativeWorkerFunc->setCallingConv(llvm::CallingConv::Fast);
        if (isTreePureNumeric(block, baseInternal))
        {
            nativeWorkerFunc->setDoesNotAccessMemory();
            nativeWorkerFunc->setDoesNotThrow();
            nativeWorkerFunc->addFnAttr(llvm::Attribute::WillReturn);
        }
    }

    s_currentNativeWorkerFunc = nativeWorkerFunc;

    // 2. Entry 依然返回 void 供外部 C++ 解释器调用
    std::vector<Type *> entryArgs = {m_ptrTy, m_ptrTy};
    Function *entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get());

    BasicBlock *entryBB = BasicBlock::Create(m_context, "entry", entryFunc);
    m_builder.SetInsertPoint(entryBB);
    Value *interp = entryFunc->getArg(0);
    Value *retVal = entryFunc->getArg(1);
    Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "entry_args");
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
    }

    if (params)
    {
        for (int i = 0; i < argCount; ++i)
        {
            Value *argRaw = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(i)});
            Value *destPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {destPtr, argRaw});
        }
    }
    m_builder.CreateCall(workerFunc, {interp, retVal, argsArray});
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
    }
    m_builder.CreateRetVoid();

    BasicBlock *workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    Value *isOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {workerFunc->getArg(0)});
    BasicBlock *ofBB = BasicBlock::Create(m_context, "rec_overflow", workerFunc);
    BasicBlock *okBB = BasicBlock::Create(m_context, "rec_ok", workerFunc);
    m_builder.CreateCondBr(isOverflow, ofBB, okBB);

    m_builder.SetInsertPoint(ofBB);
    m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));

    m_builder.SetInsertPoint(okBB);

    if (nativeWorkerFunc)
    {
        Value *workerRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2);
        std::vector<Value *> nativeArgs;
        for (int i = 0; i < argCount; ++i)
        {
            Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
            nativeArgs.push_back(inlineToDoubleFast(argPtr));
        }
        CallInst *res = m_builder.CreateCall(nativeWorkerFunc, nativeArgs);
        res->setCallingConv(llvm::CallingConv::Fast);
        m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
        inlineStoreNativeToPtr(workerRetPtr, res);
        m_builder.CreateRet(res);
    }
    else
    {
        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_varDeclaredTypes.clear();
        m_currentClassDef = nullptr;

        this->m_currentRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2); // 这里正确定义了 workerArgsArray

        BasicBlock *bodyBB = BasicBlock::Create(m_context, "body", workerFunc);
        s_tailRecurseBB = bodyBB;

        if (params)
        {
            auto pList = params->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                std::string pType = getParamType(pList[i]);
                if (!pType.empty())
                {
                    m_varDeclaredTypes[pName] = pType;
                    if (TzdOopManager::getClass(pType) != nullptr)
                    {
                        m_varClassTypes[pName] = pType;
                    }
                }
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));

                // Boxed version (for non-numeric access like string concatenation)
                AllocaInst *boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
                m_builder.CreateStore(argPtr, boxedAlloc);
                m_namedValues[pName] = boxedAlloc;

                bool isNumeric = isExplicitNumericParam(pList[i]) ||
                                 (returnsDouble && !isNonNumericParam(pList[i]) && !isParamUsedAsContainer(block, pName));
                if (pName != "this" && isNumeric)
                {
                    Value *nativeVal = inlineToDoubleFast(argPtr);
                    AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                    m_builder.CreateStore(nativeVal, nativeAlloc);
                    m_nativeDoubleLocals[pName] = nativeAlloc;
                }
            }
        }

        m_hoistedArrays.clear();
        std::set<std::string> fnIndexedContainers;
        collectIndexedContainers(block, fnIndexedContainers);
        std::set<std::string> fnAssignedVars;
        collectAssignedVariables(block, fnAssignedVars);
        std::set<std::string> fnDeclaredVars;
        collectDeclaredVariables(block, fnDeclaredVars);
        for (const std::string &cName : fnIndexedContainers)
        {
            if (fnAssignedVars.count(cName) == 0 && fnDeclaredVars.count(cName) == 0)
            {
                Value *container = nullptr;
                if (m_namedValues.count(cName) > 0)
                {
                    container = m_builder.CreateLoad(m_ptrTy, m_namedValues[cName], cName + "_fn_cont");
                }
                else
                {
                    Value *nameStr = m_builder.CreateGlobalStringPtr(cName);
                    int line = block->getStart() ? (int)block->getStart()->getLine() : 0;
                    int col = block->getStart() ? (int)block->getStart()->getCharPositionInLine() : 0;
                    container = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr, m_builder.getInt32(line), m_builder.getInt32(col)});
                }
                Value *fastBuf = m_builder.CreateLoad(m_ptrTy, container, cName + "_fn_buf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, cName + "_fn_lenPtr");
                Value *fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, cName + "_fn_len");
                Value *hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), cName + "_fn_has_buf");
                m_hoistedArrays[cName] = {container, fastBuf, fastLen, hasBuf, "", nullptr, false};
            }
        }

        m_builder.CreateBr(bodyBB);
        m_builder.SetInsertPoint(bodyBB);

        visit(block);
        m_hoistedArrays.clear();

        // 默认返回硬件级的 0.0
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
            Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, nullVal});
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
    }
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;

    // Phase B: Compile native worker body — params are direct doubles, no TzdValue unbox
    if (nativeWorkerFunc)
    {
        s_compilingNativeWorker = true;
        BasicBlock *nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_currentClassDef = nullptr;
        m_varFieldAllocas.clear();
        m_lastNewFieldAllocas.clear();

        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        BasicBlock *nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        s_tailRecurseBB = nativeBodyBB;

        if (params)
        {
            auto pList = params->param();
            for (int i = 0; i < argCount; ++i)
            {
                std::string pName = getParamName(pList[i]);
                std::string pType = getParamType(pList[i]);
                if (!pType.empty() && TzdOopManager::getClass(pType) != nullptr)
                {
                    m_varClassTypes[pName] = pType;
                }
                s_currentFuncParamNames.push_back(pName);
                m_declaredLocals.insert(pName);

                // Store double arg into alloca — mem2reg will promote to register.
                // Using alloca so tail-recursion path (CreateStore) works correctly.
                AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeWorkerFunc->getArg(i), nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_hoistedArrays.clear();
        std::set<std::string> fnIndexedContainers;
        collectIndexedContainers(block, fnIndexedContainers);
        std::set<std::string> fnAssignedVars;
        collectAssignedVariables(block, fnAssignedVars);
        std::set<std::string> fnDeclaredVars;
        collectDeclaredVariables(block, fnDeclaredVars);
        for (const std::string &cName : fnIndexedContainers)
        {
            if (fnAssignedVars.count(cName) == 0 && fnDeclaredVars.count(cName) == 0)
            {
                Value *container = nullptr;
                if (m_namedValues.count(cName) > 0)
                {
                    container = m_builder.CreateLoad(m_ptrTy, m_namedValues[cName], cName + "_fn_cont");
                }
                else
                {
                    Value *nameStr = m_builder.CreateGlobalStringPtr(cName);
                    int line = block->getStart() ? (int)block->getStart()->getLine() : 0;
                    int col = block->getStart() ? (int)block->getStart()->getCharPositionInLine() : 0;
                    container = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr, m_builder.getInt32(line), m_builder.getInt32(col)});
                }
                Value *fastBuf = m_builder.CreateLoad(m_ptrTy, container, cName + "_fn_buf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, cName + "_fn_lenPtr");
                Value *fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, cName + "_fn_len");
                Value *hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), cName + "_fn_has_buf");
                m_hoistedArrays[cName] = {container, fastBuf, fastLen, hasBuf, "", nullptr, false};
            }
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(block);
        m_hoistedArrays.clear();

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
        s_compilingNativeWorker = false;
    }

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_currentFuncParamNames.clear();
    m_currentRetPtr = nullptr;
    s_inTailPosition = false;
    s_currentFunctionBlock = nullptr;
    s_currentFunctionParams.clear();
    s_currentCompilingFuncName = savedCompilingFunc;
}

// Overload for bytecode JIT bridge: takes param names as a vector<string>
// instead of ParamListContext, so we can compile from TzdValue::funcBody + params.
void TzdCompiler::compileNamedFunction(TzdLangParser::BlockContext *block,
                                       const std::vector<std::string> &paramNames,
                                       const std::string &internalName,
                                       const std::string &explicitReturnType)
{
    std::string savedCompilingFunc = s_currentCompilingFuncName;
    std::string baseInternal = internalName;
    size_t uPos = baseInternal.find_last_of('_');
    if (uPos != std::string::npos && uPos + 1 < baseInternal.size() && baseInternal[uPos + 1] == 'v')
    {
        baseInternal = baseInternal.substr(0, uPos);
    }
    s_currentCompilingFuncName = baseInternal;
    s_currentFunctionBlock = block;
    s_currentFunctionParams = paramNames;

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;

    int argCount = (int)paramNames.size();

    // 1. Worker 内部函数返回类型为 m_doubleTy
    std::vector<Type *> workerArgs = {m_ptrTy, m_ptrTy, m_ptrTy};
    Function *workerFunc = Function::Create(
        FunctionType::get(m_doubleTy, workerArgs, false),
        Function::ExternalLinkage,
        internalName + "_worker",
        m_module.get());
    workerFunc->addFnAttr(llvm::Attribute::NoInline);
    s_currentWorkerFunc = workerFunc;

    bool returnsDouble = !explicitReturnType.empty()
                             ? isExplicitNumericType(explicitReturnType)
                             : isBlockReturningDouble(block);

    // Phase B: Create native double worker for function specialization.
    // Skip if any param is "this" (pointer, not double) — native worker
    // only makes sense for all-numeric self-recursion.
    Function *nativeWorkerFunc = nullptr;
    bool hasThisParam = false;
    for (auto &p : paramNames)
    {
        if (p == "this")
        {
            hasThisParam = true;
            break;
        }
    }
    if (argCount > 0 && !hasThisParam && returnsDouble)
    {
        std::vector<Type *> nativeWorkerArgs;
        for (int i = 0; i < argCount; ++i)
        {
            nativeWorkerArgs.push_back(m_doubleTy);
        }
        nativeWorkerFunc = Function::Create(
            FunctionType::get(m_doubleTy, nativeWorkerArgs, false),
            Function::ExternalLinkage,
            internalName + "_worker_native",
            m_module.get());
        nativeWorkerFunc->addFnAttr(llvm::Attribute::InlineHint);
        nativeWorkerFunc->setCallingConv(llvm::CallingConv::Fast);
        if (isTreePureNumeric(block, baseInternal))
        {
            nativeWorkerFunc->setDoesNotAccessMemory();
            nativeWorkerFunc->setDoesNotThrow();
            nativeWorkerFunc->addFnAttr(llvm::Attribute::WillReturn);
        }
    }

    s_currentNativeWorkerFunc = nativeWorkerFunc;

    // 2. Entry returns void for external C++ interpreter calls
    std::vector<Type *> entryArgs = {m_ptrTy, m_ptrTy};
    Function *entryFunc = Function::Create(
        FunctionType::get(m_voidTy, entryArgs, false),
        Function::ExternalLinkage,
        internalName,
        m_module.get());

    BasicBlock *entryBB = BasicBlock::Create(m_context, "entry", entryFunc);
    m_builder.SetInsertPoint(entryBB);
    Value *interp = entryFunc->getArg(0);
    Value *retVal = entryFunc->getArg(1);
    Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "entry_args");
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
        for (int i = 0; i < argCount; ++i)
        {
            Value *argRaw = m_builder.CreateCall(getRtFunc("rt_get_arg"), {m_builder.getInt32(i)});
            Value *destPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {destPtr, argRaw});
        }
    }
    m_builder.CreateCall(workerFunc, {interp, retVal, argsArray});
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
    }
    m_builder.CreateRetVoid();

    BasicBlock *workerEntryBB = BasicBlock::Create(m_context, "entry", workerFunc);
    m_builder.SetInsertPoint(workerEntryBB);

    Value *isOverflow = m_builder.CreateCall(getRtFunc("rt_check_recursion"), {workerFunc->getArg(0)});
    BasicBlock *ofBB = BasicBlock::Create(m_context, "rec_overflow", workerFunc);
    BasicBlock *okBB = BasicBlock::Create(m_context, "rec_ok", workerFunc);
    m_builder.CreateCondBr(isOverflow, ofBB, okBB);

    m_builder.SetInsertPoint(ofBB);
    m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));

    m_builder.SetInsertPoint(okBB);

    if (nativeWorkerFunc)
    {
        Value *workerRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2);
        std::vector<Value *> nativeArgs;
        for (int i = 0; i < argCount; ++i)
        {
            Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));
            nativeArgs.push_back(inlineToDoubleFast(argPtr));
        }
        CallInst *res = m_builder.CreateCall(nativeWorkerFunc, nativeArgs);
        res->setCallingConv(llvm::CallingConv::Fast);
        m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
        inlineStoreNativeToPtr(workerRetPtr, res);
        m_builder.CreateRet(res);
    }
    else
    {
        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_varDeclaredTypes.clear();
        m_currentClassDef = nullptr;

        this->m_currentRetPtr = workerFunc->getArg(1);
        Value *workerArgsArray = workerFunc->getArg(2);

        BasicBlock *bodyBB = BasicBlock::Create(m_context, "body", workerFunc);
        s_tailRecurseBB = bodyBB;

        for (int i = 0; i < argCount; ++i)
        {
            const std::string &pName = paramNames[i];
            s_currentFuncParamNames.push_back(pName);
            m_declaredLocals.insert(pName);

            Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, workerArgsArray, m_builder.getInt32(i));

            AllocaInst *boxedAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, pName);
            m_builder.CreateStore(argPtr, boxedAlloc);
            m_namedValues[pName] = boxedAlloc;

            // Skip "this" — it's an instance pointer, not a numeric value!
            if (pName != "this" && returnsDouble && !isParamUsedAsContainer(block, pName))
            {
                Value *nativeVal = inlineToDoubleFast(argPtr);
                AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
                m_builder.CreateStore(nativeVal, nativeAlloc);
                m_nativeDoubleLocals[pName] = nativeAlloc;
            }
        }

        m_builder.CreateBr(bodyBB);
        m_builder.SetInsertPoint(bodyBB);

        visit(block);

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {workerFunc->getArg(0)});
            Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, nullVal});
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
    }
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    m_currentRetPtr = nullptr;

    // Phase B: Compile native worker body
    if (argCount > 0 && nativeWorkerFunc)
    {
        s_compilingNativeWorker = true;
        BasicBlock *nativeEntryBB = BasicBlock::Create(m_context, "entry", nativeWorkerFunc);
        m_builder.SetInsertPoint(nativeEntryBB);

        m_namedValues.clear();
        m_nativeDoubleLocals.clear();
        m_declaredLocals.clear();
        s_currentFuncParamNames.clear();
        m_varClassTypes.clear();
        m_currentClassDef = nullptr;

        m_currentRetPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        BasicBlock *nativeBodyBB = BasicBlock::Create(m_context, "body", nativeWorkerFunc);
        s_tailRecurseBB = nativeBodyBB;

        for (int i = 0; i < argCount; ++i)
        {
            const std::string &pName = paramNames[i];
            s_currentFuncParamNames.push_back(pName);
            m_declaredLocals.insert(pName);

            // Skip "this" — it's a pointer, not a native double
            if (pName == "this")
                continue;

            AllocaInst *nativeAlloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, pName + "_native");
            m_builder.CreateStore(nativeWorkerFunc->getArg(i), nativeAlloc);
            m_nativeDoubleLocals[pName] = nativeAlloc;
        }

        m_builder.CreateBr(nativeBodyBB);
        m_builder.SetInsertPoint(nativeBodyBB);

        visit(block);

        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        }
        s_compilingNativeWorker = false;
    }

    s_currentNativeWorkerFunc = nullptr;
    s_currentWorkerFunc = nullptr;
    s_tailRecurseBB = nullptr;
    s_currentFuncParamNames.clear();
    m_currentRetPtr = nullptr;
    s_inTailPosition = false;
    s_currentFunctionBlock = nullptr;
    s_currentFunctionParams.clear();
    s_currentCompilingFuncName = savedCompilingFunc;
}

void TzdJitEngine::executeFunction(const std::string &name, void *interp, void *retVal)
{
    auto symOrErr = lookupSymbol(name);
    if (!symOrErr)
    {
        consumeError(symOrErr.takeError());
        errs() << "executeFunction: symbol not found: " << name << "\n";
        return;
    }
    llvm::orc::ExecutorAddr addr = *symOrErr;
    llvm::JITTargetAddress rawAddr = addr.getValue();
    using FnType = void (*)(void *, void *);
    auto fn = reinterpret_cast<FnType>(rawAddr);
    fn(interp, retVal);
}

TzdJitEngine::TzdJitEngine()
    : m_tsc(std::make_unique<LLVMContext>())
{
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (JTMB)
    {
        JTMB->setCodeGenOptLevel(llvm::CodeGenOptLevel::Aggressive);
        auto J = LLJITBuilder().setJITTargetMachineBuilder(std::move(*JTMB)).create();
        if (!J)
        {
            errs() << "Failed to create LLJIT: " << toString(J.takeError()) << "\n";
            m_lljit.reset();
            return;
        }
        m_lljit = std::move(*J);
    }
    else
    {
        llvm::consumeError(JTMB.takeError());
        auto J = LLJITBuilder().create();
        if (!J)
        {
            errs() << "Failed to create LLJIT: " << toString(J.takeError()) << "\n";
            m_lljit.reset();
            return;
        }
        m_lljit = std::move(*J);
    }
    registerRuntimeSymbols();
}

TzdJitEngine::~TzdJitEngine()
{
    // Leak LLJIT and ThreadSafeContext to avoid stack overflow during
    // LLVM internal cleanup at program exit.
    (void)m_lljit.release();

    // Move ThreadSafeContext to a heap-allocated container that is never
    // destroyed, so the LLVMContext's refcount never hits zero.
    static std::vector<llvm::orc::ThreadSafeContext> *s_leaked = nullptr;
    if (!s_leaked)
        s_leaked = new std::vector<llvm::orc::ThreadSafeContext>();
    s_leaked->push_back(std::move(m_tsc));
}

void TzdJitEngine::initLLJIT() {}

void TzdCompiler::setupExternalFunctions()
{
    auto addFunc = [&](std::string name, std::vector<Type *> args, Type *ret = nullptr)
    {
        Function *existing = m_module->getFunction(name);
        if (existing)
            return existing;
        Function *F = Function::Create(
            FunctionType::get(ret ? ret : m_ptrTy, args, false),
            Function::ExternalLinkage,
            name,
            m_module.get());

        F->addFnAttr(llvm::Attribute::NoUnwind);
        F->addFnAttr(llvm::Attribute::WillReturn);

        return F;
    };

    addFunc("rt_create_num", {m_doubleTy});
    addFunc("rt_create_str", {m_ptrTy});
    addFunc("rt_to_string_num", {m_doubleTy}, m_ptrTy);
    addFunc("rt_create_bigint", {m_ptrTy}, m_ptrTy);
    addFunc("rt_create_bool", {m_boolTy});
    addFunc("rt_create_null", {});
    addFunc("rt_create_inst", {m_ptrTy});
    addFunc("rt_create_inst_args", {m_ptrTy, m_int32Ty, m_ptrTy});
    addFunc("rt_create_inst_sroa", {m_ptrTy, m_int32Ty, m_ptrTy});
    addFunc("rt_create_inst_2d", {m_ptrTy, m_doubleTy, m_doubleTy});
    addFunc("rt_tzd_get_field_d", {m_ptrTy, m_int32Ty}, m_doubleTy);
    addFunc("rt_tzd_get_field_i64", {m_ptrTy, m_int32Ty}, m_builder.getInt64Ty());
    addFunc("rt_tzd_get_field_val", {m_ptrTy, m_int32Ty}, m_ptrTy);
    addFunc("rt_tzd_set_field_d", {m_ptrTy, m_int32Ty, m_doubleTy}, m_voidTy);
    addFunc("rt_tzd_set_field_i64", {m_ptrTy, m_int32Ty, m_builder.getInt64Ty()}, m_voidTy);
    addFunc("rt_tzd_set_field_val", {m_ptrTy, m_int32Ty, m_ptrTy}, m_voidTy);
    addFunc("rt_tzd_check_var_type", {m_ptrTy, m_ptrTy, m_ptrTy}, m_builder.getInt1Ty());
    addFunc("rt_create_native_val", {m_ptrTy, m_ptrTy});

    addFunc("rt_resolve_var", {m_ptrTy});
    addFunc("rt_store_var", {m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_get_arg", {Type::getInt32Ty(m_context)});
    addFunc("rt_op_add", {m_ptrTy, m_ptrTy});
    addFunc("rt_str_concat", {m_ptrTy, m_ptrTy});
    addFunc("rt_str_concat_num_str", {m_doubleTy, m_ptrTy});
    addFunc("rt_str_concat_str_num", {m_ptrTy, m_doubleTy});
    addFunc("rt_str_concat_3", {m_ptrTy, m_ptrTy, m_ptrTy});
    addFunc("rt_str_concat_str_num_str", {m_ptrTy, m_doubleTy, m_ptrTy});
    addFunc("rt_str_append", {m_ptrTy, m_ptrTy});
    addFunc("rt_str_append_num", {m_ptrTy, m_doubleTy});
    addFunc("rt_op_sub", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_mul", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_div", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_mod", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_pow", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_bitand", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_bitor", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_bitxor", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_bitnot", {m_ptrTy});
    addFunc("rt_op_shl", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_shr", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_ushr", {m_ptrTy, m_ptrTy});
    addFunc("rt_to_int64_fast", {m_ptrTy}, m_builder.getInt64Ty());
    addFunc("rt_op_neg", {m_ptrTy});
    addFunc("rt_op_not", {m_ptrTy});
    addFunc("rt_op_sqrt", {m_ptrTy});
    addFunc("rt_op_gt", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_lt", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_ge", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_le", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_eq", {m_ptrTy, m_ptrTy});
    addFunc("rt_op_ne", {m_ptrTy, m_ptrTy});

    addFunc("rt_to_bool", {m_ptrTy}, m_boolTy);
    addFunc("rt_cast", {m_ptrTy, m_ptrTy});

    addFunc("rt_create_array", {});
    addFunc("rt_create_map", {});
    addFunc("rt_map_set", {m_ptrTy, m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_map_set_str", {m_ptrTy, m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_array_push", {m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_get_index", {m_ptrTy, m_ptrTy});
    addFunc("rt_print", {m_ptrTy}, m_voidTy);

    addFunc("rt_get_member", {m_ptrTy, m_ptrTy});
    addFunc("rt_store_member", {m_ptrTy, m_ptrTy, m_ptrTy}, m_voidTy);

    // tzd selector dispatch —— 热路径用 i32 常量 selector，name 仅冷诊断。
    addFunc("rt_tzd_get_member", {m_ptrTy, m_int32Ty, m_ptrTy});
    addFunc("rt_tzd_store_member", {m_ptrTy, m_int32Ty, m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_tzd_call_method", {m_ptrTy, m_int32Ty, m_ptrTy, m_int32Ty, m_ptrTy});
    // F1: readonly on get_member enables LLVM CSE/LICM to eliminate
    // redundant field reads in loops (same obj+selector = same result)
    // Note: ReadOnly attr not supported on functions in this LLVM version;
    // the AlwaysInlinerPass + mem2reg + EarlyCSE still optimize effectively.
    // F3: help LLVM optimize method calls
    m_module->getFunction("rt_tzd_call_method")->addFnAttr(llvm::Attribute::NoCallback);
    m_module->getFunction("rt_tzd_call_method")->addFnAttr(llvm::Attribute::WillReturn);
    // F2: lightweight field store via cached pointer (split from store_member)
    addFunc("rt_store_field_ptr", {m_ptrTy, m_ptrTy}, m_voidTy);

    addFunc("rt_tzd_get_member_ic", {m_ptrTy, m_int32Ty, m_ptrTy, m_ptrTy});
    addFunc("rt_cast", {m_ptrTy, m_ptrTy});
    addFunc("rt_type_check", {m_ptrTy, m_ptrTy}, m_boolTy);
    addFunc("rt_get_var_ptr", {m_ptrTy, m_int32Ty, m_int32Ty});
    addFunc("rt_get_fast_buf", {m_ptrTy}, m_ptrTy);
    addFunc("rt_get_fast_len", {m_ptrTy}, m_doubleTy);
    addFunc("rt_print_newline", {}, m_voidTy);
    addFunc("rt_to_double_fast", {m_ptrTy}, m_doubleTy);
    addFunc("rt_stabilize_value", {m_ptrTy}, m_ptrTy);
    addFunc("rt_copy_value", {m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_fast_range", {m_doubleTy, m_doubleTy, m_doubleTy}, m_ptrTy);
    addFunc("rt_call_sub_f1", {m_ptrTy, m_ptrTy}, m_ptrTy);

    addFunc("rt_call_sub_fast", {m_ptrTy, m_int32Ty, m_ptrTy}, m_ptrTy);
    addFunc("rt_call_value_fast", {m_ptrTy, m_int32Ty, m_ptrTy}, m_ptrTy);
    addFunc("rt_store_native_to_ptr", {m_ptrTy, m_doubleTy}, m_voidTy);
    addFunc("rt_set_null", {m_ptrTy}, m_voidTy);
    addFunc("rt_store_index", {m_ptrTy, m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_store_index_d", {m_ptrTy, m_ptrTy, m_doubleTy}, m_voidTy);

    addFunc("rt_construct_num_at", {m_ptrTy, m_doubleTy}, m_voidTy);
    addFunc("rt_construct_copy_at", {m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_destruct_values", {m_ptrTy, m_int32Ty}, m_voidTy);

    addFunc("rt_get_index_native_d", {m_ptrTy, m_int32Ty}, m_doubleTy);
    addFunc("rt_store_index_native_d", {m_ptrTy, m_int32Ty, m_doubleTy}, m_voidTy);
    addFunc("rt_get_index_native_d_dyn", {m_ptrTy, m_doubleTy}, m_doubleTy);
    addFunc("rt_get_index_native_d_ptr", {m_ptrTy, m_ptrTy}, m_doubleTy);
    addFunc("rt_store_index_native_d_dyn", {m_ptrTy, m_doubleTy, m_doubleTy}, m_voidTy);
    addFunc("rt_get_index_fast", {m_ptrTy, m_doubleTy}, m_ptrTy);
    addFunc("rt_store_index_bool", {m_ptrTy, m_doubleTy, m_boolTy}, m_voidTy);
    addFunc("rt_store_index_val_dyn", {m_ptrTy, m_doubleTy, m_ptrTy}, m_voidTy);
    addFunc("rt_create_native_double_arr", {m_int32Ty, m_ptrTy}, m_ptrTy);

    addFunc("rt_push_arg_frame", {m_ptrTy}, m_voidTy);
    addFunc("rt_pop_arg_frame", {}, m_voidTy);

    addFunc("rt_init_tzd_value", {m_ptrTy, m_int32Ty}, m_voidTy);
    addFunc("rt_write_fast_ret", {m_ptrTy, m_ptrTy}, m_voidTy);

    addFunc("rt_get_worker_ptr", {m_ptrTy}, m_ptrTy);
    addFunc("rt_check_recursion", {m_ptrTy}, m_boolTy);
    addFunc("rt_pop_call_depth", {m_ptrTy}, m_voidTy);
    addFunc("rt_get_class_def", {m_ptrTy}, m_ptrTy);

    addFunc("rt_alloc_jmp_buf", {}, m_ptrTy);
    addFunc("rt_free_jmp_buf", {m_ptrTy}, m_voidTy);
    addFunc("rt_get_catch_jmp", {}, m_ptrTy);
    addFunc("rt_set_catch_jmp", {m_ptrTy}, m_voidTy);

#ifdef _WIN32
    Function *sjDecl = Function::Create(FunctionType::get(m_int32Ty, {m_ptrTy, m_ptrTy}, false),
                                        Function::ExternalLinkage, "_setjmp", m_module.get());
#else
    Function *sjDecl = Function::Create(FunctionType::get(m_int32Ty, {m_ptrTy}, false),
                                        Function::ExternalLinkage, "setjmp", m_module.get());
#endif
    sjDecl->addFnAttr(llvm::Attribute::ReturnsTwice);

    // addFunc("rt_enter_try_buf", { m_ptrTy }, Type::getInt32Ty(m_context));
    // addFunc("rt_leave_try", {}, m_voidTy);
    addFunc("rt_throw", {m_ptrTy}, m_voidTy);
    addFunc("rt_get_thrown", {}, m_ptrTy);
    addFunc("rt_push_catch_scope", {m_ptrTy, m_ptrTy}, m_voidTy);
    addFunc("rt_pop_catch_scope", {}, m_voidTy);

    addFunc("rt_set_location", {m_int32Ty, m_int32Ty}, m_voidTy);
    addFunc("rt_push_jit_frame", {m_ptrTy}, m_voidTy);
    addFunc("rt_pop_jit_frame", {}, m_voidTy);
    addFunc("rt_replace_jit_frame", {m_ptrTy}, m_voidTy);
    addFunc("rt_get_call_stack_depth", {}, m_int32Ty);
    addFunc("rt_restore_call_stack_depth", {m_int32Ty}, m_voidTy);

    addFunc("rt_create_lambda_value", {m_ptrTy});
    Function::Create(
        FunctionType::get(m_ptrTy, {m_ptrTy, Type::getInt32Ty(m_context)}, true),
        Function::ExternalLinkage, "rt_call_super", m_module.get());

    Function::Create(
        FunctionType::get(m_ptrTy, {m_ptrTy, Type::getInt32Ty(m_context)}, true),
        Function::ExternalLinkage, "rt_call_sub", m_module.get());

    for (const char *n : {"rt_to_double_fast", "rt_get_fast_buf", "rt_get_fast_len"})
        if (Function *F = m_module->getFunction(n))
            F->setOnlyReadsMemory();
}

Function *TzdCompiler::getRtFunc(const std::string &name)
{
    Function *f = m_module->getFunction(name);
    if (!f)
    {
        std::cerr << "[JIT Compiler Error]: Runtime function '" << name
                  << "' used but not declared in setupExternalFunctions!" << std::endl;
        llvm::errs() << "FATAL: Runtime function not found in module: " << name << "\n";
        abort();
    }
    return f;
}

void TzdJitEngine::registerWorkerForSymbol(const std::string &internalName)
{
    std::string workerName = internalName + "_worker";
    if (void *ptr = lookupSymbolAsPtr(workerName))
    {
        std::string baseName = internalName;
        size_t lastUnderscore = baseName.find_last_of('_');
        if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v')
        {
            baseName = baseName.substr(0, lastUnderscore);
        }
        std::unique_lock<std::shared_mutex> lock(s_workerPointersMutex);
        s_workerPointers[baseName] = ptr;
        s_workerPointers[internalName] = ptr;
        s_workerPointers[workerName] = ptr;
    }
}

void TzdJitEngine::addModule(ThreadSafeModule TSM)
{
    if (!m_lljit)
    {
        errs() << "addModule: LLJIT is not initialized\n";
        return;
    }

    // Pre-compile the IR module to an object buffer manually, then add the
    // object file to the JIT.  This bypasses IRCompileLayer::emit() which
    // destroys the Module after compilation — that destruction overflows the
    // stack due to a CRT ABI mismatch (/MT in TzdTools vs /MD in LLVM SDK).
    std::unique_ptr<MemoryBuffer> objBuffer;
    std::vector<std::string> definedFuncSymbols;

    {
        // Lock the ThreadSafeContext to safely access the Module
        auto lock = TSM.getContext().getLock();
        Module *M = TSM.getModuleUnlocked();
        if (!M)
            return;

        // Run LLVM optimization passes on the module before compilation.
        // Analysis managers are leaked (static) — their destructors overflow
        // the stack when freeing analysis results referencing large IR graphs.
        {
            static llvm::ModuleAnalysisManager *MAM = nullptr;
            static llvm::FunctionAnalysisManager *FAM = nullptr;
            static llvm::CGSCCAnalysisManager *CGAM = nullptr;
            static llvm::LoopAnalysisManager *LAM = nullptr;
            static llvm::PassBuilder *PB = nullptr;
            static std::unique_ptr<llvm::TargetMachine> s_optTM;
            static bool s_init = false;
            if (!s_init)
            {
                s_init = true;
                if (auto JTMB = llvm::orc::JITTargetMachineBuilder::detectHost())
                {
                    JTMB->setCodeGenOptLevel(llvm::CodeGenOptLevel::Aggressive);
                    if (auto TM = JTMB->createTargetMachine())
                        s_optTM = std::move(*TM);
                    else
                        llvm::consumeError(TM.takeError());
                }
                else
                    llvm::consumeError(JTMB.takeError());
                MAM = new llvm::ModuleAnalysisManager();
                FAM = new llvm::FunctionAnalysisManager();
                CGAM = new llvm::CGSCCAnalysisManager();
                LAM = new llvm::LoopAnalysisManager();
                PB = new llvm::PassBuilder(s_optTM.get());
                PB->registerModuleAnalyses(*MAM);
                PB->registerFunctionAnalyses(*FAM);
                PB->registerCGSCCAnalyses(*CGAM);
                PB->registerLoopAnalyses(*LAM);
                PB->crossRegisterProxies(*LAM, *FAM, *CGAM, *MAM);
            }

            int optLvl = s_jitConfig.optLevel;
            llvm::ModulePassManager MPM;

            if (optLvl <= 0)
            {
                // -O0: Minimal passes (mem2reg only)
                llvm::FunctionPassManager FPM;
                FPM.addPass(llvm::PromotePass());
                MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                MPM.run(*M, *MAM);
            }
            else if (optLvl == 1)
            {
                // -O1: AlwaysInliner + basic scalar optimizations
                MPM.addPass(llvm::AlwaysInlinerPass());
                llvm::FunctionPassManager FPM;
                FPM.addPass(llvm::PromotePass());
                FPM.addPass(llvm::EarlyCSEPass(true));
                FPM.addPass(llvm::DCEPass());
                MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                MPM.run(*M, *MAM);
            }
            else
            {
                // -O2 / -O3: Full inlining + scalar + loop rotation + LICM + unrolling
                int threshold = optLvl == 3 ? (s_jitConfig.inlineThreshold > 0 ? s_jitConfig.inlineThreshold : 500)
                                            : s_jitConfig.inlineThreshold;
                MPM.addPass(llvm::ModuleInlinerWrapperPass(llvm::getInlineParams(threshold)));

                llvm::FunctionPassManager FPM;
                FPM.addPass(llvm::PromotePass());
                FPM.addPass(llvm::SROAPass(llvm::SROAOptions::ModifyCFG));
                FPM.addPass(llvm::EarlyCSEPass(true));
                FPM.addPass(llvm::ReassociatePass());
                FPM.addPass(llvm::Float2IntPass());
                FPM.addPass(llvm::CorrelatedValuePropagationPass());
                FPM.addPass(llvm::ConstraintEliminationPass());
                FPM.addPass(llvm::InstCombinePass());
                FPM.addPass(llvm::SimplifyCFGPass());
                FPM.addPass(llvm::TailCallElimPass());

                // Loop canonicalization, rotation, invariant hoisting, induction variable simplify
                FPM.addPass(llvm::LoopSimplifyPass());
                FPM.addPass(llvm::LCSSAPass());
                FPM.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::LoopRotatePass()));
                FPM.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::LICMPass(llvm::LICMOptions()), /*UseMemorySSA=*/true));
                FPM.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::SimpleLoopUnswitchPass(/*NonTrivial=*/true)));
                FPM.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::IndVarSimplifyPass()));

                FPM.addPass(llvm::GVNPass());
                FPM.addPass(llvm::InstCombinePass());

                FPM.addPass(llvm::LoopVectorizePass());
                FPM.addPass(llvm::SLPVectorizerPass());
                FPM.addPass(llvm::VectorCombinePass());

                if (s_jitConfig.enableLoopUnroll)
                {
                    llvm::LoopUnrollOptions unrollOpts;
                    unrollOpts.setOptLevel(optLvl == 3 ? 3 : 2);
                    unrollOpts.setPartial(true);
                    unrollOpts.setRuntime(true);
                    unrollOpts.setPeeling(true);
                    unrollOpts.setUpperBound(true);
                    FPM.addPass(llvm::LoopUnrollPass(unrollOpts));

                    FPM.addPass(llvm::ReassociatePass());
                    FPM.addPass(llvm::EarlyCSEPass(true));
                    FPM.addPass(llvm::InstCombinePass());
                    FPM.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::LICMPass(llvm::LICMOptions()), /*UseMemorySSA=*/true));
                }

                FPM.addPass(llvm::SimplifyCFGPass());
                FPM.addPass(llvm::InstCombinePass());
                FPM.addPass(llvm::TailCallElimPass());
                FPM.addPass(llvm::DCEPass());

                MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                MPM.run(*M, *MAM);
            }
        }

        // Record Jitted Function Info for all defined functions in M (for debugger & inspection)
        for (auto &F : *M)
        {
            if (!F.isDeclaration())
            {
                std::string sym = F.getName().str();
                definedFuncSymbols.push_back(sym);

                JittedFunctionInfo info;
                info.internalName = sym;
                info.internalSymbolName = sym;

                // Derive human-readable name: strip _worker, _worker_native, _vX, etc.
                std::string base = sym;
                if (base.size() > 14 && base.substr(base.size() - 14) == "_worker_native")
                {
                    base = base.substr(0, base.size() - 14);
                }
                else if (base.size() > 7 && base.substr(base.size() - 7) == "_worker")
                {
                    base = base.substr(0, base.size() - 7);
                }
                size_t us = base.find_last_of('_');
                if (us != std::string::npos && us + 1 < base.size() && base[us + 1] == 'v')
                {
                    base = base.substr(0, us);
                }
                info.name = base;
                info.functionName = base;
                info.paramCount = (int)F.arg_size();
                info.optLevel = s_jitConfig.optLevel;
                info.inlined = F.hasFnAttribute(llvm::Attribute::AlwaysInline);
                info.isInlined = info.inlined;

                std::string irStr;
                llvm::raw_string_ostream rso(irStr);
                F.print(rso);
                info.irDump = rso.str();
                info.llvmIR = rso.str();
                if (info.name == "countPrimes" || info.internalName.find("countPrimes") != std::string::npos)
                {
                    std::ofstream ofs("primes_ir.txt", std::ios::app);
                    ofs << "\n; ==========================================\n; Function: " << sym << "\n; ==========================================\n";
                    ofs << info.irDump;
                }
                info.entryAddress = nullptr;
                info.nativeAddress = nullptr;

                registerJittedFunction(info);
            }
        }

        // Use the JIT's own compiler to compile IR to object code
        auto &compiler = m_lljit->getIRCompileLayer().getCompiler();
        auto result = compiler(*M);

        if (!result)
        {
            errs() << "Pre-compile failed: " << toString(result.takeError()) << "\n";
            return;
        }

        objBuffer = std::move(*result);
    }
    // Lock released here

    // Add the pre-compiled object file to the JIT
    if (auto Err = m_lljit->addObjectFile(std::move(objBuffer)))
    {
        errs() << "addObjectFile failed: " << toString(std::move(Err)) << "\n";
        consumeError(std::move(Err));
    }

    // Now resolve symbols for registered functions and update nativeAddress
    for (const auto &sym : definedFuncSymbols)
    {
        if (void *ptr = lookupSymbolAsPtr(sym))
        {
            std::unique_lock<std::shared_mutex> regLock(s_jitRegistryMutex);
            if (auto it = s_registeredJitMap.find(sym); it != s_registeredJitMap.end())
            {
                it->second.nativeAddress = ptr;
                it->second.entryAddress = ptr;
            }
            for (auto &fInfo : s_registeredJitFunctions)
            {
                if (fInfo.internalSymbolName == sym || fInfo.internalName == sym)
                {
                    fInfo.nativeAddress = ptr;
                    fInfo.entryAddress = ptr;
                    break;
                }
            }
        }
    }

    // Leak the ThreadSafeModule (and the IR Module + LLVMContext inside it)
    // to prevent Module destruction.  Process exit reclaims all memory.
    static std::vector<ThreadSafeModule> *s_leakedModules = nullptr;
    if (!s_leakedModules)
        s_leakedModules = new std::vector<ThreadSafeModule>();
    s_leakedModules->push_back(std::move(TSM));
}

LLVMContext &TzdJitEngine::getContext()
{
    return *m_tsc.getContext();
}

ThreadSafeContext &TzdJitEngine::getThreadSafeContext()
{
    return m_tsc;
}

const DataLayout &TzdJitEngine::getDataLayout() const
{
    return m_lljit->getDataLayout();
}

std::string TzdJitEngine::getTargetTriple() const
{
    return m_lljit->getTargetTriple().str();
}

void TzdJitEngine::registerRuntimeSymbols()
{
    if (!m_lljit)
    {
        std::cerr << "Critical Error: Cannot register symbols on a null LLJIT instance!" << std::endl;
        return;
    }
    auto &ES = m_lljit->getExecutionSession();
    auto &DL = m_lljit->getDataLayout();
    MangleAndInterner Mangle(ES, DL);
    SymbolMap symbols;

    auto bind = [&](const std::string &Name, void *Addr)
    {
        ExecutorSymbolDef symbol(ExecutorAddr::fromPtr(Addr), JITSymbolFlags::Exported);
        symbols.insert({Mangle(Name), symbol});
    };

    bind("rt_create_num", (void *)&rt_create_num);
    bind("rt_create_str", (void *)&rt_create_str);
    bind("rt_to_string_num", (void *)&rt_to_string_num);
    bind("rt_create_bigint", (void *)&rt_create_bigint);
    bind("rt_create_bool", (void *)&rt_create_bool);
    bind("rt_create_null", (void *)&rt_create_null);
    bind("rt_op_add", (void *)&rt_op_add);
    bind("rt_str_concat", (void *)&rt_str_concat);
    bind("rt_str_concat_num_str", (void *)&rt_str_concat_num_str);
    bind("rt_str_concat_str_num", (void *)&rt_str_concat_str_num);
    bind("rt_str_concat_3", (void *)&rt_str_concat_3);
    bind("rt_str_concat_str_num_str", (void *)&rt_str_concat_str_num_str);
    bind("rt_str_append", (void *)&rt_str_append);
    bind("rt_str_append_num", (void *)&rt_str_append_num);
    bind("rt_to_bool", (void *)&rt_to_bool);
    bind("rt_create_inst", (void *)&rt_create_inst);
    bind("rt_create_inst_args", (void *)&rt_create_inst_args);
    bind("rt_create_inst_sroa", (void *)&rt_create_inst_sroa);
    bind("rt_create_inst_2d", (void *)&rt_create_inst_2d);
    bind("rt_tzd_get_field_d", (void *)&rt_tzd_get_field_d);
    bind("rt_tzd_get_field_i64", (void *)&rt_tzd_get_field_i64);
    bind("rt_tzd_get_field_val", (void *)&rt_tzd_get_field_val);
    bind("rt_tzd_set_field_d", (void *)&rt_tzd_set_field_d);
    bind("rt_tzd_set_field_i64", (void *)&rt_tzd_set_field_i64);
    bind("rt_tzd_set_field_val", (void *)&rt_tzd_set_field_val);
    bind("rt_tzd_check_var_type", (void *)&rt_tzd_check_var_type);
    bind("rt_get_member", (void *)&rt_get_member);
    bind("rt_tzd_get_member", (void *)&rt_tzd_get_member);
    bind("rt_print", (void *)&rt_print);
    bind("rt_create_native_val", (void *)&rt_create_native_val);
    bind("g_last_ret", (void *)&g_LastJitValue);
    bind("rt_op_sub", (void *)&rt_op_sub);
    bind("rt_op_mul", (void *)&rt_op_mul);
    bind("rt_op_div", (void *)&rt_op_div);
    bind("rt_op_mod", (void *)&rt_op_mod);
    bind("rt_op_pow", (void *)&rt_op_pow);
    bind("rt_op_bitand", (void *)&rt_op_bitand);
    bind("rt_op_bitor", (void *)&rt_op_bitor);
    bind("rt_op_bitxor", (void *)&rt_op_bitxor);
    bind("rt_op_bitnot", (void *)&rt_op_bitnot);
    bind("rt_op_shl", (void *)&rt_op_shl);
    bind("rt_op_shr", (void *)&rt_op_shr);
    bind("rt_op_ushr", (void *)&rt_op_ushr);
    bind("rt_to_int64_fast", (void *)&rt_to_int64_fast);
    bind("rt_op_neg", (void *)&rt_op_neg);
    bind("rt_op_not", (void *)&rt_op_not);
    bind("rt_op_sqrt", (void *)&rt_op_sqrt);
    bind("rt_op_gt", (void *)&rt_op_gt);
    bind("rt_op_lt", (void *)&rt_op_lt);
    bind("rt_op_ge", (void *)&rt_op_ge);
    bind("rt_op_le", (void *)&rt_op_le);
    bind("rt_op_eq", (void *)&rt_op_eq);
    bind("rt_op_ne", (void *)&rt_op_ne);
    bind("rt_array_push", (void *)&rt_array_push);
    bind("rt_get_index", (void *)&rt_get_index);
    bind("rt_create_array", (void *)&rt_create_array);
    bind("rt_create_map", (void *)&rt_create_map);
    bind("rt_map_set", (void *)&rt_map_set);
    bind("rt_map_set_str", (void *)&rt_map_set_str);
    bind("rt_store_var", (void *)&rt_store_var);
    bind("rt_resolve_var", (void *)&rt_resolve_var);
    bind("rt_get_arg", (void *)&rt_get_arg);
    bind("rt_get_fast_buf", (void *)&rt_get_fast_buf);
    bind("rt_get_fast_len", (void *)&rt_get_fast_len);
    bind("rt_set_last_ret", (void *)&rt_set_last_ret);
    bind("rt_call_sub", (void *)&rt_call_sub);
    bind("rt_store_member", (void *)&rt_store_member);
    bind("rt_tzd_store_member", (void *)&rt_tzd_store_member);
    bind("rt_tzd_call_method", (void *)&rt_tzd_call_method);
    bind("rt_store_field_ptr", (void *)&rt_store_field_ptr);
    bind("rt_cast", (void *)&rt_cast);
    bind("rt_type_check", (void *)&rt_type_check);
    bind("rt_call_super", (void *)&rt_call_super);
    bind("rt_get_var_ptr", (void *)&rt_get_var_ptr);
    bind("rt_print_newline", (void *)&rt_print_newline);
    bind("rt_to_double_fast", (void *)&rt_to_double_fast);
    bind("rt_stabilize_value", (void *)&rt_stabilize_value);
    bind("rt_store_index", (void *)&rt_store_index);
    bind("rt_store_index_d", (void *)&rt_store_index_d);
    bind("rt_copy_value", (void *)&rt_copy_value);
    bind("rt_fast_range", (void *)&rt_fast_range);
    bind("rt_call_sub_f1", (void *)&rt_call_sub_f1);
    bind("rt_call_sub_fast", (void *)&rt_call_sub_fast);
    bind("rt_call_value_fast", (void *)&rt_call_value_fast);
    bind("rt_store_native_to_ptr", (void *)&rt_store_native_to_ptr);
    bind("rt_set_null", (void *)&rt_set_null);
    bind("rt_construct_num_at", (void *)&rt_construct_num_at);
    bind("rt_construct_copy_at", (void *)&rt_construct_copy_at);
    bind("rt_destruct_values", (void *)&rt_destruct_values);
    bind("rt_get_index_native_d", (void *)&rt_get_index_native_d);
    bind("rt_store_index_native_d", (void *)&rt_store_index_native_d);
    bind("rt_get_index_native_d_dyn", (void *)&rt_get_index_native_d_dyn);
    bind("rt_get_index_native_d_ptr", (void *)&rt_get_index_native_d_ptr);
    bind("rt_store_index_native_d_dyn", (void *)&rt_store_index_native_d_dyn);
    bind("rt_get_index_fast", (void *)&rt_get_index_fast);
    bind("rt_store_index_bool", (void *)&rt_store_index_bool);
    bind("rt_store_index_val_dyn", (void *)&rt_store_index_val_dyn);
    bind("rt_create_native_double_arr", (void *)&rt_create_native_double_arr);
    bind("rt_push_arg_frame", (void *)&rt_push_arg_frame);
    bind("rt_pop_arg_frame", (void *)&rt_pop_arg_frame);
    bind("rt_init_tzd_value", (void *)&rt_init_tzd_value);
    bind("rt_write_fast_ret", (void *)&rt_write_fast_ret);
    bind("rt_get_worker_ptr", (void *)&rt_get_worker_ptr);
    bind("rt_check_recursion", (void *)&rt_check_recursion);
    bind("rt_pop_call_depth", (void *)&rt_pop_call_depth);
    bind("rt_get_class_def", (void *)&rt_get_class_def);
    bind("rt_tzd_get_member_ic", (void *)&rt_tzd_get_member_ic);

    // ======= 修复 try/catch 机制的新绑定 =======
    bind("rt_alloc_jmp_buf", (void *)&rt_alloc_jmp_buf);
    bind("rt_free_jmp_buf", (void *)&rt_free_jmp_buf);
    bind("rt_get_catch_jmp", (void *)&rt_get_catch_jmp);
    bind("rt_set_catch_jmp", (void *)&rt_set_catch_jmp);

#ifdef _WIN32
    void *sjAddr = nullptr;
    HMODULE hUcrt = GetModuleHandleA("ucrtbase.dll");
    if (hUcrt)
        sjAddr = (void *)GetProcAddress(hUcrt, "_setjmp");
    if (!sjAddr)
    {
        HMODULE hMsvc = GetModuleHandleA("msvcrt.dll");
        if (hMsvc)
            sjAddr = (void *)GetProcAddress(hMsvc, "_setjmp");
    }
    if (!sjAddr)
    {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll)
            sjAddr = (void *)GetProcAddress(hNtdll, "_setjmp");
    }
    if (sjAddr)
    {
        bind("_setjmp", sjAddr);
    }
    else
    {
        std::cerr << "FATAL: Could not resolve _setjmp from Windows DLLs!" << std::endl;
    }
#else
    bind("setjmp", (void *)&_setjmp);
#endif
    // ==========================================

    bind("rt_throw", (void *)&rt_throw);
    bind("rt_get_thrown", (void *)&rt_get_thrown);
    bind("rt_set_location", (void *)&rt_set_location);
    bind("rt_push_jit_frame", (void *)&rt_push_jit_frame);
    bind("rt_pop_jit_frame", (void *)&rt_pop_jit_frame);
    bind("rt_replace_jit_frame", (void *)&rt_replace_jit_frame);
    bind("rt_get_call_stack_depth", (void *)&rt_get_call_stack_depth);
    bind("rt_restore_call_stack_depth", (void *)&rt_restore_call_stack_depth);
    bind("rt_create_lambda_value", (void *)&rt_create_lambda_value);

    cantFail(m_lljit->getMainJITDylib().define(absoluteSymbols(symbols)));
}

TzdCompiler::TzdCompiler(TzdJitEngine &jit, const std::string &modName)
    : m_jitEngine(jit),
      m_tsc(std::make_unique<llvm::LLVMContext>()),
      m_context(*m_tsc.getContext()),
      m_builder(m_context)
{
    llvm::FastMathFlags fmf;
    fmf.setAllowReassoc();
    fmf.setNoSignedZeros();
    fmf.setAllowContract();
    m_builder.setFastMathFlags(fmf);

    s_currentFuncParamNames.clear();
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;
    s_currentWorkerFunc = nullptr;
    s_currentNativeWorkerFunc = nullptr;
    s_compilingNativeWorker = false;
    s_currentCompilingFuncName.clear();
    s_inlineReturnStack.clear();
    s_inlinedFunctionsInStack.clear();
    s_currentInlineDepth = 0;
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
    m_tzdValueTy->setBody({llvm::ArrayType::get(llvm::Type::getInt8Ty(m_context), sizeof(TzdValue))});

    if (!m_ptrTy || !m_voidTy || !m_tzdValueTy || !m_int32Ty)
    {
        std::cerr << "FATAL: LLVM Type initialization failed!" << std::endl;
        abort();
    }
}

TzdCompiler::~TzdCompiler()
{
    m_builder.ClearInsertionPoint();

    s_currentFuncParamNames.clear();
    s_tailRecurseBB = nullptr;
    s_inTailPosition = false;
    s_currentWorkerFunc = nullptr;
    s_tryJmpBufStack.clear();
    s_loopLevel = 0;

    if (m_module)
    {
        // Leak the Module to avoid heap corruption from LLVM ABI mismatch.
        m_module.release();
    }

    // Leak the ThreadSafeContext (LLVMContext) to avoid stack overflow
    // during LLVMContext destruction.  Move to a heap container that is
    // never destroyed — process exit reclaims all memory.
    static std::vector<llvm::orc::ThreadSafeContext> *s_leaked = nullptr;
    if (!s_leaked)
        s_leaked = new std::vector<llvm::orc::ThreadSafeContext>();
    s_leaked->push_back(std::move(m_tsc));
}

llvm::orc::ThreadSafeModule TzdCompiler::extractThreadSafeModule()
{
    if (!m_module)
    {
        return llvm::orc::ThreadSafeModule(nullptr, m_tsc);
    }

    // 1. Clean up dead code blocks
    for (auto &F : *m_module)
    {
        if (F.isDeclaration())
            continue;
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto it = F.begin(); it != F.end();)
            {
                llvm::BasicBlock &BB = *it++;
                if (&BB != &F.getEntryBlock() && llvm::pred_empty(&BB))
                {
                    BB.dropAllReferences();
                    BB.eraseFromParent();
                    changed = true;
                }
            }
        }
    }
    std::string verifyErrors;
    llvm::raw_string_ostream os(verifyErrors);
    if (llvm::verifyModule(*m_module, &os))
    {
        llvm::errs() << "[JIT] Module verification failed:\n"
                     << verifyErrors << "\n";
        m_module->print(llvm::errs(), nullptr);
        if (g_CurrentInterpreter)
        {
            g_CurrentInterpreter->reportJitError("JIT 模块校验失败: " + verifyErrors);
        }
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
    s_currentNativeWorkerFunc = nullptr;
    s_compilingNativeWorker = false;
    s_tailRecurseBB = nullptr;
    s_currentFuncParamNames.clear();
    s_currentCompilingFuncName.clear();
    s_inTailPosition = false;
    s_inlineReturnStack.clear();
    s_inlinedFunctionsInStack.clear();
    s_currentInlineDepth = 0;
    s_tryJmpBufStack.clear();
    s_loopLevel = 0;

    return llvm::orc::ThreadSafeModule(std::move(moduleForJIT), m_tsc);
}

Value *TzdCompiler::boxToTzdValue(Value *val)
{
    if (!val)
        return m_builder.CreateCall(getRtFunc("rt_create_null"));
    if (val->getType()->isPointerTy())
        return val;

    if (val->getType()->isDoubleTy())
    {
        return m_builder.CreateCall(getRtFunc("rt_create_num"), {val});
    }
    if (val->getType()->isIntegerTy(1))
    { // i1 (bool)
        return m_builder.CreateCall(getRtFunc("rt_create_bool"), {val});
    }
    if (val->getType()->isIntegerTy())
    {
        Value *dbl = m_builder.CreateSIToFP(val, m_doubleTy);
        return m_builder.CreateCall(getRtFunc("rt_create_num"), {dbl});
    }
    return val;
}

Value *TzdCompiler::toNativeBool(Value *val)
{
    if (val->getType()->isIntegerTy(1))
    {
        return val;
    }
    if (val->getType()->isIntegerTy())
    {
        return m_builder.CreateICmpNE(val, ConstantInt::get(val->getType(), 0), "tobool");
    }
    if (val->getType()->isDoubleTy())
    {
        return m_builder.CreateFCmpONE(val, ConstantFP::get(m_doubleTy, 0.0), "tobool");
    }
    if (val->getType()->isPointerTy())
    {
        return m_builder.CreateCall(getRtFunc("rt_to_bool"), {val});
    }
    return m_builder.getTrue(); // 默认 true
}

std::unique_ptr<llvm::Module> TzdCompiler::getModule()
{
    for (auto &F : *m_module)
    {
        if (F.isDeclaration())
            continue;
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto it = F.begin(); it != F.end();)
            {
                llvm::BasicBlock &BB = *it++;
                if (&BB != &F.getEntryBlock() && llvm::pred_empty(&BB))
                {
                    BB.dropAllReferences();
                    BB.eraseFromParent();
                    changed = true;
                }
            }
        }
    }
    if (llvm::verifyModule(*m_module, &llvm::errs()))
    {
        llvm::errs() << "\n>>> [JIT FATAL ERROR]: Module verification failed! \n";
        llvm::errs() << "========== [ DUMPING GENERATED LLVM IR CODE ] ==========\n";
        m_module->print(llvm::errs(), nullptr);
        llvm::errs() << "========================================================\n\n";
        return nullptr;
    }
    return std::move(m_module);
}

// --- 基础结构 ---

std::any TzdCompiler::visitProgram(TzdLangParser::ProgramContext *ctx)
{
    for (auto stmt : ctx->statement())
        visit(stmt);
    return std::any();
}

std::any TzdCompiler::visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext *ctx)
{
    std::string name = ctx->IDENTIFIER()->getText();
    compileNamedFunction(ctx, name);
    return std::any((Value *)m_module->getFunction(name)); // 返回对外暴露的入口
}

std::any TzdCompiler::visitBlock(TzdLangParser::BlockContext *ctx)
{
    for (auto stmt : ctx->statement())
        visit(stmt);
    return std::any();
}

std::any TzdCompiler::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext *ctx)
{
    return visit(ctx->variableDeclaration());
}

std::any TzdCompiler::visitVariableDeclaration(TzdLangParser::VariableDeclarationContext *decl)
{
    std::string name = decl->IDENTIFIER()->getText();
    m_declaredLocals.insert(name);

    std::string declaredType = "";
    if (decl->typeType())
    {
        declaredType = decl->typeType()->getText();
        m_varDeclaredTypes[name] = declaredType;
        if (TzdOopManager::getClass(declaredType))
        {
            m_varClassTypes[name] = declaredType;
        }
    }
    if (decl->expression())
    {
        std::string newClass = deduceExprClassName(decl->expression(), m_varClassTypes, m_currentClassDef);
        if (newClass.empty() && decl->typeType())
        {
            newClass = decl->typeType()->getText();
        }
        if (!newClass.empty())
        {
            m_varClassTypes[name] = newClass;
        }
    }
    else if (decl->typeType())
    {
        std::string t = decl->typeType()->getText();
        if (TzdOopManager::getClass(t))
        {
            m_varClassTypes[name] = t;
        }
    }

    Value *initVal = nullptr;
    if (decl->expression())
    {
        initVal = std::any_cast<Value *>(visit(decl->expression()));
        if (!declaredType.empty() && declaredType != "var" && declaredType != "any" && declaredType != "auto")
        {
            Value *boxedVal = boxToTzdValue(initVal);
            Value *nameStr = m_builder.CreateGlobalStringPtr(name);
            Value *typeStr = m_builder.CreateGlobalStringPtr(declaredType);
            Value *ok = m_builder.CreateCall(getRtFunc("rt_tzd_check_var_type"), {nameStr, typeStr, boxedVal});

            BasicBlock *contBB = BasicBlock::Create(m_context, "decl_type_ok", m_builder.GetInsertBlock()->getParent());
            BasicBlock *errBB = BasicBlock::Create(m_context, "decl_type_err", m_builder.GetInsertBlock()->getParent());
            m_builder.CreateCondBr(ok, contBB, errBB);

            m_builder.SetInsertPoint(errBB);
            if (s_currentWorkerFunc && s_currentWorkerFunc->arg_size() > 0)
            {
                m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {s_currentWorkerFunc->getArg(0)});
            }
            if (m_builder.GetInsertBlock()->getParent()->getReturnType()->isDoubleTy())
                m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
            else if (m_builder.GetInsertBlock()->getParent()->getReturnType()->isVoidTy())
                m_builder.CreateRetVoid();
            else
                m_builder.CreateRet(Constant::getNullValue(m_builder.GetInsertBlock()->getParent()->getReturnType()));

            m_builder.SetInsertPoint(contBB);
        }
        if (!m_lastNewFieldAllocas.empty())
        {
            m_varFieldAllocas[name] = m_lastNewFieldAllocas;
            m_lastNewFieldAllocas.clear();
            if (m_varClassTypes.count(name) == 0)
            {
                std::string cls = deduceExprClassName(decl->expression(), m_varClassTypes, m_currentClassDef);
                if (!cls.empty())
                    m_varClassTypes[name] = cls;
            }
            if (s_compilingNativeWorker)
            {
                Value *nullPtr = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
                Value *slot = CreateEntryBlockAlloca(m_ptrTy, nullptr, name);
                m_namedValues[name] = slot;
                m_builder.CreateStore(nullPtr, slot);
                return std::any();
            }
        }
    }
    else
    {
        initVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
    }

    Function *curF = m_builder.GetInsertBlock()->getParent();
    bool isTopLevel = (curF && curF->getName() == "main_module_entry");

    if (initVal->getType()->isDoubleTy() || initVal->getType()->isIntegerTy())
    {
        Value *dVal = castToNativeDouble(initVal);
        Value *alloc = nullptr;
        auto it = m_nativeDoubleLocals.find(name);
        if (it != m_nativeDoubleLocals.end())
        {
            alloc = it->second;
        }
        else
        {
            alloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, name + "_native");
            m_nativeDoubleLocals[name] = alloc;
        }
        m_builder.CreateStore(dVal, alloc);

        if (isTopLevel)
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(name);
            Value *boxedVal = boxToTzdValue(dVal);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
        }
        return std::any();
    }

    Value *boxedVal = boxToTzdValue(initVal);
    Value *alloca = nullptr;
    auto it = m_namedValues.find(name);
    Value *destPtr = nullptr;
    if (it != m_namedValues.end())
    {
        alloca = it->second;
        destPtr = m_builder.CreateLoad(m_ptrTy, alloca, name + "_ptr");
        m_builder.CreateCall(getRtFunc("rt_copy_value"), {destPtr, boxedVal});
    }
    else
    {
        alloca = CreateEntryBlockAlloca(m_ptrTy, nullptr, name);
        m_namedValues[name] = alloca;
        destPtr = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), {boxedVal});
        m_builder.CreateStore(destPtr, alloca);
    }

    if (isTopLevel)
    {
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, destPtr});
    }
    return std::any();
}

std::any TzdCompiler::visitLambdaExpr(TzdLangParser::LambdaExprContext *ctx)
{
    auto *savedInsertBlock = m_builder.GetInsertBlock();
    auto savedNamedValues = m_namedValues;
    auto savedNativeLocals = m_nativeDoubleLocals;
    auto savedParamNames = s_currentFuncParamNames;
    auto *savedTailBB = s_tailRecurseBB;
    auto *savedWorkerFunc = s_currentWorkerFunc;
    auto *savedNativeWorkerFunc = s_currentNativeWorkerFunc;
    bool savedCompilingNative = s_compilingNativeWorker;
    auto *savedRetPtr = m_currentRetPtr;

    static int s_lambdaCount = 0;
    std::string lambdaInternalName = "lambda_fun_" + std::to_string(s_lambdaCount++);

    compileNamedFunction(ctx->block(), ctx->paramList(), lambdaInternalName);

    if (g_CurrentInterpreter)
    {
        g_CurrentInterpreter->m_pendingJitFunctions.insert(lambdaInternalName);
    }

    m_builder.SetInsertPoint(savedInsertBlock);
    m_namedValues = savedNamedValues;
    m_nativeDoubleLocals = savedNativeLocals;
    s_currentFuncParamNames = savedParamNames;
    s_tailRecurseBB = savedTailBB;
    s_currentWorkerFunc = savedWorkerFunc;
    s_currentNativeWorkerFunc = savedNativeWorkerFunc;
    s_compilingNativeWorker = savedCompilingNative;
    m_currentRetPtr = savedRetPtr;

    Value *nameStr = m_builder.CreateGlobalStringPtr(lambdaInternalName);
    Value *lambdaVal = m_builder.CreateCall(getRtFunc("rt_create_lambda_value"), {nameStr});

    return std::any(lambdaVal);
}

Value *TzdCompiler::emitTruthyCond(TzdLangParser::ExpressionContext *exprCtx)
{
    if (auto idxCtx = dynamic_cast<TzdLangParser::IndexExprContext *>(exprCtx))
    {
        std::string containerName = idxCtx->expression(0)->getText();
        Value *container = nullptr;
        Value *fastBuf = nullptr;
        Value *fastLen = nullptr;
        Value *hasBuf = nullptr;

        auto hIt = m_hoistedArrays.find(containerName);
        if (hIt != m_hoistedArrays.end())
        {
            container = hIt->second.container;
            fastBuf = hIt->second.fastBuf;
            fastLen = hIt->second.fastLen;
            hasBuf = hIt->second.hasBuf;
        }
        else
        {
            if (m_namedValues.count(containerName))
            {
                container = m_builder.CreateLoad(m_ptrTy, m_namedValues[containerName]);
            }
            else
            {
                container = std::any_cast<Value *>(visit(idxCtx->expression(0)));
                if (container->getType()->isDoubleTy())
                {
                    container = boxToTzdValue(container);
                }
            }
            fastBuf = m_builder.CreateLoad(m_ptrTy, container, "fastBuf");
            Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, "fastLenPtr");
            fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, "fastLen");
            hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), "hasBuf");
        }
        Value *index_raw = std::any_cast<Value *>(visit(idxCtx->expression(1)));
        if (index_raw->getType()->isDoubleTy())
        {
            std::string idxExprText = idxCtx->expression(1)->getText();
            Value *idxI64 = nullptr;
            auto sIt = m_shadowI64IndVars.find(idxExprText);
            if (sIt != m_shadowI64IndVars.end())
            {
                idxI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), sIt->second, idxExprText + "_idx_i64");
            }
            else
            {
                idxI64 = m_builder.CreateFPToSI(index_raw, m_builder.getInt64Ty(), "idx_i64");
            }

            if (hIt != m_hoistedArrays.end() && hIt->second.inFastLoop &&
                (hIt->second.safeIndexVar.empty() || hIt->second.safeIndexVar == idxExprText))
            {
                Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
                Value *fastValD = m_builder.CreateLoad(m_doubleTy, slot, "fastElem");
                return m_builder.CreateFCmpONE(fastValD, ConstantFP::get(m_doubleTy, 0.0), "fastBool");
            }

            Function *curFunc = m_builder.GetInsertBlock()->getParent();
            BasicBlock *fastLoadBB = BasicBlock::Create(m_context, "idx_bool_fast", curFunc);
            BasicBlock *slowLoadBB = BasicBlock::Create(m_context, "idx_bool_slow", curFunc);
            BasicBlock *mergeLoadBB = BasicBlock::Create(m_context, "idx_bool_merge", curFunc);

            Value *canFast = nullptr;
            if (hIt != m_hoistedArrays.end() && !hIt->second.safeIndexVar.empty() &&
                hIt->second.safeIndexVar == idxExprText && hIt->second.safeFastCond)
            {
                canFast = hIt->second.safeFastCond;
            }
            else
            {
                Value *inBounds = m_builder.CreateICmpULT(idxI64, fastLen, "inBounds");
                canFast = m_builder.CreateAnd(hasBuf, inBounds, "canFast");
            }

            AllocaInst *boolSlot = CreateEntryBlockAlloca(m_boolTy, nullptr, "idx_bool_res");

            m_builder.CreateCondBr(canFast, fastLoadBB, slowLoadBB);

            m_builder.SetInsertPoint(fastLoadBB);
            Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
            Value *fastValD = m_builder.CreateLoad(m_doubleTy, slot, "fastElem");
            Value *fastBool = m_builder.CreateFCmpONE(fastValD, ConstantFP::get(m_doubleTy, 0.0), "fastBool");
            m_builder.CreateStore(fastBool, boolSlot);
            m_builder.CreateBr(mergeLoadBB);

            m_builder.SetInsertPoint(slowLoadBB);
            Value *slowBoxed = m_builder.CreateCall(getRtFunc("rt_get_index_fast"), {container, index_raw});
            Value *slowBool = m_builder.CreateCall(getRtFunc("rt_to_bool"), {slowBoxed});
            m_builder.CreateStore(slowBool, boolSlot);
            m_builder.CreateBr(mergeLoadBB);

            m_builder.SetInsertPoint(mergeLoadBB);
            Value *resBool = m_builder.CreateLoad(m_boolTy, boolSlot, "idx_cond_bool");
            return resBool;
        }
    }
    Value *condVal = std::any_cast<Value *>(visit(exprCtx));
    return toNativeBool(condVal);
}

// ============================================================================
// Canonical Loop Idiom Recognition: Arithmetic Progression Sum Reduction
// ============================================================================
static std::string getExprIdentifier(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return "";
    if (auto paren = dynamic_cast<TzdLangParser::ParenExprContext *>(tree))
    {
        return getExprIdentifier(paren->expression());
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(tree))
    {
        if (auto idCtx = dynamic_cast<TzdLangParser::IdExprContext *>(atomExpr->atom()))
        {
            return idCtx->IDENTIFIER()->getText();
        }
    }
    if (auto idCtx = dynamic_cast<TzdLangParser::IdExprContext *>(tree))
    {
        return idCtx->IDENTIFIER()->getText();
    }
    return "";
}

static bool getExprConstantNumber(antlr4::tree::ParseTree *tree, double &outVal)
{
    if (!tree)
        return false;
    if (auto paren = dynamic_cast<TzdLangParser::ParenExprContext *>(tree))
    {
        return getExprConstantNumber(paren->expression(), outVal);
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(tree))
    {
        if (auto intCtx = dynamic_cast<TzdLangParser::IntExprContext *>(atomExpr->atom()))
        {
            try
            {
                outVal = std::stod(intCtx->INTEGER()->getText());
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
        if (auto floatCtx = dynamic_cast<TzdLangParser::FloatExprContext *>(atomExpr->atom()))
        {
            try
            {
                outVal = std::stod(floatCtx->FLOAT()->getText());
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    }
    if (auto intCtx = dynamic_cast<TzdLangParser::IntExprContext *>(tree))
    {
        try
        {
            outVal = std::stod(intCtx->INTEGER()->getText());
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    if (auto floatCtx = dynamic_cast<TzdLangParser::FloatExprContext *>(tree))
    {
        try
        {
            outVal = std::stod(floatCtx->FLOAT()->getText());
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    return false;
}

static bool isIntegerValuedExpr(antlr4::tree::ParseTree *tree, int depth = 0)
{
    if (!tree || depth > 8)
        return false;
    double c = 0.0;
    if (getExprConstantNumber(tree, c))
    {
        return std::floor(c) == c && std::abs(c) < 4.5e15;
    }
    if (auto paren = dynamic_cast<TzdLangParser::ParenExprContext *>(tree))
    {
        return isIntegerValuedExpr(paren->expression(), depth + 1);
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(tree))
    {
        if (dynamic_cast<TzdLangParser::IntExprContext *>(atomExpr->atom()))
            return true;
        return isIntegerValuedExpr(atomExpr->atom(), depth + 1);
    }
    if (dynamic_cast<TzdLangParser::IntExprContext *>(tree))
        return true;
    if (auto castExpr = dynamic_cast<TzdLangParser::CastExprContext *>(tree))
    {
        if (castExpr->typeType())
        {
            std::string t = castExpr->typeType()->getText();
            if (t == "int" || t == "i32" || t == "long" || t == "i64")
                return true;
        }
    }
    if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(tree))
    {
        return isIntegerValuedExpr(add->expression(0), depth + 1) &&
               isIntegerValuedExpr(add->expression(1), depth + 1);
    }
    if (auto mul = dynamic_cast<TzdLangParser::MultiplicativeExprContext *>(tree))
    {
        if (mul->MOD())
            return true;
        if (mul->MUL())
            return isIntegerValuedExpr(mul->expression(0), depth + 1) &&
                   isIntegerValuedExpr(mul->expression(1), depth + 1);
        return false;
    }
    if (auto post = dynamic_cast<TzdLangParser::PostfixExprContext *>(tree))
    {
        return isIntegerValuedExpr(post->expression(), depth + 1);
    }
    if (auto pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(tree))
    {
        return isIntegerValuedExpr(pre->expression(), depth + 1);
    }
    if (dynamic_cast<TzdLangParser::ShiftExprContext *>(tree) ||
        dynamic_cast<TzdLangParser::BitAndExprContext *>(tree) ||
        dynamic_cast<TzdLangParser::BitXorExprContext *>(tree) ||
        dynamic_cast<TzdLangParser::BitOrExprContext *>(tree))
    {
        return true;
    }
    std::string id = getExprIdentifier(tree);
    if (!id.empty())
    {
        return true;
    }
    return false;
}

static bool treeContainsId(antlr4::tree::ParseTree *tree, const std::string &id)
{
    if (!tree)
        return false;
    if (getExprIdentifier(tree) == id)
        return true;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (treeContainsId(tree->children[i], id))
            return true;
    }
    return false;
}

static bool isPureExpression(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return true;
    if (dynamic_cast<TzdLangParser::CallExprContext *>(tree))
        return false;
    if (dynamic_cast<TzdLangParser::AssignmentExprContext *>(tree))
        return false;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (!isPureExpression(tree->children[i]))
            return false;
    }
    return true;
}

static bool matchIfConversionAccumulation(TzdLangParser::StatementContext *stmt,
                                          std::string &outVar,
                                          TzdLangParser::ExpressionContext *&outDeltaExpr,
                                          bool &outIsAdd)
{
    outVar.clear();
    outDeltaExpr = nullptr;
    outIsAdd = true;
    if (!stmt)
        return false;

    TzdLangParser::StatementContext *single = stmt;
    if (auto blockStmt = dynamic_cast<TzdLangParser::BlockStmtContext *>(stmt))
    {
        if (!blockStmt->block() || blockStmt->block()->statement().size() != 1)
            return false;
        single = blockStmt->block()->statement(0);
    }

    auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(single);
    if (!exprStmt || !exprStmt->expression())
        return false;
    auto expr = exprStmt->expression();

    if (auto post = dynamic_cast<TzdLangParser::PostfixExprContext *>(expr))
    {
        if (post->INC())
        {
            outVar = getExprIdentifier(post->expression());
            outIsAdd = true;
            return !outVar.empty();
        }
        if (post->DEC())
        {
            outVar = getExprIdentifier(post->expression());
            outIsAdd = false;
            return !outVar.empty();
        }
    }
    if (auto pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(expr))
    {
        if (pre->INC())
        {
            outVar = getExprIdentifier(pre->expression());
            outIsAdd = true;
            return !outVar.empty();
        }
        if (pre->DEC())
        {
            outVar = getExprIdentifier(pre->expression());
            outIsAdd = false;
            return !outVar.empty();
        }
    }

    if (auto assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(expr))
    {
        if (!assign->expression(0) || !assign->expression(1))
            return false;
        std::string lhs = getExprIdentifier(assign->expression(0));
        if (lhs.empty())
            return false;

        if (assign->PLUS_ASSIGN())
        {
            if (!isPureExpression(assign->expression(1)))
                return false;
            outVar = lhs;
            outDeltaExpr = assign->expression(1);
            outIsAdd = true;
            return true;
        }
        if (assign->MIN_ASSIGN())
        {
            if (!isPureExpression(assign->expression(1)))
                return false;
            outVar = lhs;
            outDeltaExpr = assign->expression(1);
            outIsAdd = false;
            return true;
        }
        if (assign->ASSIGN())
        {
            if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(assign->expression(1)))
            {
                if (add->PLUS() && add->expression(0) && add->expression(1))
                {
                    if (getExprIdentifier(add->expression(0)) == lhs && isPureExpression(add->expression(1)))
                    {
                        outVar = lhs;
                        outDeltaExpr = add->expression(1);
                        outIsAdd = true;
                        return true;
                    }
                    if (getExprIdentifier(add->expression(1)) == lhs && isPureExpression(add->expression(0)))
                    {
                        outVar = lhs;
                        outDeltaExpr = add->expression(0);
                        outIsAdd = true;
                        return true;
                    }
                }
                if (add->MINUS() && add->expression(0) && add->expression(1))
                {
                    if (getExprIdentifier(add->expression(0)) == lhs && isPureExpression(add->expression(1)))
                    {
                        outVar = lhs;
                        outDeltaExpr = add->expression(1);
                        outIsAdd = false;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

std::any TzdCompiler::visitIfStmt(TzdLangParser::IfStmtContext *ctx)
{
    std::string accVar;
    TzdLangParser::ExpressionContext *deltaExpr = nullptr;
    bool isAdd = true;
    if (!ctx->KW_ELSE() && matchIfConversionAccumulation(ctx->statement(0), accVar, deltaExpr, isAdd))
    {
        if (m_nativeDoubleLocals.count(accVar) > 0 && (!deltaExpr || !treeContainsId(deltaExpr, accVar)))
        {
            Value *isTrue = emitTruthyCond(ctx->expression());
            Value *varAlloc = m_nativeDoubleLocals[accVar];
            Value *curVal = m_builder.CreateLoad(m_doubleTy, varAlloc, accVar + "_cur");
            Value *deltaVal = nullptr;
            if (!deltaExpr)
            {
                deltaVal = ConstantFP::get(m_doubleTy, 1.0);
            }
            else
            {
                double constNum = 0.0;
                if (getExprConstantNumber(deltaExpr, constNum))
                {
                    deltaVal = ConstantFP::get(m_doubleTy, constNum);
                }
                else
                {
                    Value *deltaRaw = castAnyToValue(visit(deltaExpr), (accVar + "_delta").c_str());
                    deltaVal = castToNativeDouble(deltaRaw);
                }
            }
            Value *zero = ConstantFP::get(m_doubleTy, 0.0);
            Value *selectedDelta = m_builder.CreateSelect(isTrue, deltaVal, zero, accVar + "_sel_delta");
            Value *newVal = isAdd ? m_builder.CreateFAdd(curVal, selectedDelta, accVar + "_new")
                                  : m_builder.CreateFSub(curVal, selectedDelta, accVar + "_new");
            m_builder.CreateStore(newVal, varAlloc);
            if (!m_declaredLocals.count(accVar))
            {
                Value *nameStr = m_builder.CreateGlobalStringPtr(accVar);
                Value *boxedVal = boxToTzdValue(newVal);
                m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
            }
            return std::any();
        }
    }

    Value *isTrue = emitTruthyCond(ctx->expression());

    Function *func = m_builder.GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(m_context, "then", func);
    BasicBlock *mergeBB = BasicBlock::Create(m_context, "ifcont");
    BasicBlock *elseBB = ctx->KW_ELSE() ? BasicBlock::Create(m_context, "else") : nullptr;

    m_builder.CreateCondBr(isTrue, thenBB, ctx->KW_ELSE() ? elseBB : mergeBB);

    m_builder.SetInsertPoint(thenBB);
    visit(ctx->statement(0));
    if (!m_builder.GetInsertBlock()->getTerminator())
        m_builder.CreateBr(mergeBB);

    if (ctx->KW_ELSE())
    {
        func->insert(func->end(), elseBB);
        m_builder.SetInsertPoint(elseBB);
        visit(ctx->statement(1));
        if (!m_builder.GetInsertBlock()->getTerminator())
            m_builder.CreateBr(mergeBB);
    }

    func->insert(func->end(), mergeBB);
    m_builder.SetInsertPoint(mergeBB);
    return std::any();
}

static bool matchAccumulationStmt(TzdLangParser::StatementContext *stmt,
                                  const std::string &varI,
                                  std::string &outSumVar,
                                  double &outCoeff)
{
    auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(stmt);
    if (!exprStmt || !exprStmt->expression())
        return false;
    auto assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(exprStmt->expression());
    if (!assign || !assign->expression(0) || !assign->expression(1))
        return false;

    std::string lhs = getExprIdentifier(assign->expression(0));
    if (lhs.empty() || lhs == varI)
        return false;

    // Pattern 1: lhs += varI  or  lhs += c * varI
    if (assign->PLUS_ASSIGN())
    {
        std::string rhsId = getExprIdentifier(assign->expression(1));
        if (rhsId == varI)
        {
            outSumVar = lhs;
            outCoeff = 1.0;
            return true;
        }
        if (auto mul = dynamic_cast<TzdLangParser::MultiplicativeExprContext *>(assign->expression(1)))
        {
            if (mul->MUL() && mul->expression(0) && mul->expression(1))
            {
                double c = 0.0;
                if (getExprIdentifier(mul->expression(0)) == varI && getExprConstantNumber(mul->expression(1), c))
                {
                    outSumVar = lhs;
                    outCoeff = c;
                    return true;
                }
                if (getExprIdentifier(mul->expression(1)) == varI && getExprConstantNumber(mul->expression(0), c))
                {
                    outSumVar = lhs;
                    outCoeff = c;
                    return true;
                }
            }
        }
    }

    // Pattern 2: lhs = lhs + varI  OR  lhs = varI + lhs  (or with coeff)
    if (assign->ASSIGN())
    {
        if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(assign->expression(1)))
        {
            if (add->PLUS() && add->expression(0) && add->expression(1))
            {
                auto op0 = add->expression(0);
                auto op1 = add->expression(1);
                std::string id0 = getExprIdentifier(op0);
                std::string id1 = getExprIdentifier(op1);

                if (id0 == lhs)
                {
                    if (id1 == varI)
                    {
                        outSumVar = lhs;
                        outCoeff = 1.0;
                        return true;
                    }
                    if (auto mul = dynamic_cast<TzdLangParser::MultiplicativeExprContext *>(op1))
                    {
                        if (mul->MUL() && mul->expression(0) && mul->expression(1))
                        {
                            double c = 0.0;
                            if (getExprIdentifier(mul->expression(0)) == varI && getExprConstantNumber(mul->expression(1), c))
                            {
                                outSumVar = lhs;
                                outCoeff = c;
                                return true;
                            }
                            if (getExprIdentifier(mul->expression(1)) == varI && getExprConstantNumber(mul->expression(0), c))
                            {
                                outSumVar = lhs;
                                outCoeff = c;
                                return true;
                            }
                        }
                    }
                }
                if (id1 == lhs)
                {
                    if (id0 == varI)
                    {
                        outSumVar = lhs;
                        outCoeff = 1.0;
                        return true;
                    }
                    if (auto mul = dynamic_cast<TzdLangParser::MultiplicativeExprContext *>(op0))
                    {
                        if (mul->MUL() && mul->expression(0) && mul->expression(1))
                        {
                            double c = 0.0;
                            if (getExprIdentifier(mul->expression(0)) == varI && getExprConstantNumber(mul->expression(1), c))
                            {
                                outSumVar = lhs;
                                outCoeff = c;
                                return true;
                            }
                            if (getExprIdentifier(mul->expression(1)) == varI && getExprConstantNumber(mul->expression(0), c))
                            {
                                outSumVar = lhs;
                                outCoeff = c;
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

static bool matchStepStmt(TzdLangParser::StatementContext *stmt,
                          const std::string &varI,
                          double &outStep)
{
    auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(stmt);
    if (!exprStmt || !exprStmt->expression())
        return false;

    // Pattern 1: varI++
    if (auto post = dynamic_cast<TzdLangParser::PostfixExprContext *>(exprStmt->expression()))
    {
        if (post->INC() && getExprIdentifier(post->expression()) == varI)
        {
            outStep = 1.0;
            return true;
        }
    }

    // Pattern 2: ++varI
    if (auto pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(exprStmt->expression()))
    {
        if (pre->INC() && getExprIdentifier(pre->expression()) == varI)
        {
            outStep = 1.0;
            return true;
        }
    }

    // Pattern 3: varI += step
    if (auto assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(exprStmt->expression()))
    {
        if (assign->expression(0) && getExprIdentifier(assign->expression(0)) == varI && assign->expression(1))
        {
            if (assign->PLUS_ASSIGN())
            {
                double c = 0.0;
                if (getExprConstantNumber(assign->expression(1), c))
                {
                    outStep = c;
                    return true;
                }
            }
            // Pattern 4: varI = varI + step  OR  varI = step + varI
            if (assign->ASSIGN())
            {
                if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(assign->expression(1)))
                {
                    if (add->PLUS() && add->expression(0) && add->expression(1))
                    {
                        std::string op0 = getExprIdentifier(add->expression(0));
                        std::string op1 = getExprIdentifier(add->expression(1));
                        double c = 0.0;
                        if (op0 == varI && getExprConstantNumber(add->expression(1), c))
                        {
                            outStep = c;
                            return true;
                        }
                        if (op1 == varI && getExprConstantNumber(add->expression(0), c))
                        {
                            outStep = c;
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool TzdCompiler::tryEmitCanonicalSumReduction(TzdLangParser::WhileStmtContext *ctx)
{
    auto relCtx = dynamic_cast<TzdLangParser::RelationalExprContext *>(ctx->expression());
    if (!relCtx || !relCtx->expression(0) || !relCtx->expression(1))
        return false;

    bool isLT = (relCtx->LT() != nullptr);
    bool isLE = (relCtx->LE() != nullptr);
    if (!isLT && !isLE)
        return false;

    std::string varI = getExprIdentifier(relCtx->expression(0));
    if (varI.empty() || !m_nativeDoubleLocals.count(varI))
        return false;

    // Limit expression must not contain loop variables
    if (treeContainsId(relCtx->expression(1), varI))
        return false;

    // Body statements
    std::vector<TzdLangParser::StatementContext *> stmts;
    if (auto blockStmt = dynamic_cast<TzdLangParser::BlockStmtContext *>(ctx->statement()))
    {
        if (!blockStmt->block())
            return false;
        stmts = blockStmt->block()->statement();
    }
    else
    {
        stmts.push_back(ctx->statement());
    }
    if (stmts.size() != 2)
        return false;

    std::string varSum;
    double coeff = 1.0;
    double stepVal = 1.0;
    bool sumFirst = false;

    if (matchAccumulationStmt(stmts[0], varI, varSum, coeff) && matchStepStmt(stmts[1], varI, stepVal))
    {
        sumFirst = true;
    }
    else if (matchStepStmt(stmts[0], varI, stepVal) && matchAccumulationStmt(stmts[1], varI, varSum, coeff))
    {
        sumFirst = false;
    }
    else
    {
        return false;
    }

    if (varSum.empty() || varSum == varI)
        return false;
    if (!m_nativeDoubleLocals.count(varSum))
        return false;
    if (treeContainsId(relCtx->expression(1), varSum))
        return false;
    if (stepVal <= 0.0)
        return false;

    // Evaluate limit expression
    Value *limitRaw = castAnyToValue(visit(relCtx->expression(1)), "sum_red.limit");
    Value *limitVal = castToNativeDouble(limitRaw);

    Value *iPtr = m_nativeDoubleLocals[varI];
    Value *sumPtr = m_nativeDoubleLocals[varSum];
    Value *initI = m_builder.CreateLoad(m_doubleTy, iPtr, varI + "_init");
    Value *initSum = m_builder.CreateLoad(m_doubleTy, sumPtr, varSum + "_init");

    Function *func = m_builder.GetInsertBlock()->getParent();
    BasicBlock *fastBB = BasicBlock::Create(m_context, "sum_red.fast", func);
    BasicBlock *doneBB = BasicBlock::Create(m_context, "sum_red.done", func);

    Value *cond = isLE ? m_builder.CreateFCmpOLE(initI, limitVal, "sum_red.cond") : m_builder.CreateFCmpOLT(initI, limitVal, "sum_red.cond");
    m_builder.CreateCondBr(cond, fastBB, doneBB);

    m_builder.SetInsertPoint(fastBB);

    Value *diff = m_builder.CreateFSub(limitVal, initI, "sum_red.diff");
    Value *stepValConst = ConstantFP::get(m_doubleTy, stepVal);
    if (stepVal != 1.0)
    {
        diff = m_builder.CreateFDiv(diff, stepValConst, "sum_red.diff_scaled");
    }

    Value *k = nullptr;
    if (isLE)
    {
        Function *floorFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::floor, {m_doubleTy});
        Value *floorVal = m_builder.CreateCall(floorFunc, {diff}, "sum_red.floor");
        k = m_builder.CreateFAdd(floorVal, ConstantFP::get(m_doubleTy, 1.0), "sum_red.k");
    }
    else
    {
        Function *ceilFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::ceil, {m_doubleTy});
        k = m_builder.CreateCall(ceilFunc, {diff}, "sum_red.k");
    }

    Value *kAdj = sumFirst ? m_builder.CreateFSub(k, ConstantFP::get(m_doubleTy, 1.0), "sum_red.k_minus_1") : m_builder.CreateFAdd(k, ConstantFP::get(m_doubleTy, 1.0), "sum_red.k_plus_1");

    Value *kTimesAdj = m_builder.CreateFMul(k, kAdj, "sum_red.k_adj_prod");
    Value *halfKTimesAdj = m_builder.CreateFMul(kTimesAdj, ConstantFP::get(m_doubleTy, 0.5), "sum_red.half_k");
    Value *stepPart = (stepVal == 1.0) ? halfKTimesAdj : m_builder.CreateFMul(halfKTimesAdj, stepValConst, "sum_red.step_part");

    Value *kTimesInitI = m_builder.CreateFMul(k, initI, "sum_red.k_initI");
    Value *sumAdded = m_builder.CreateFAdd(kTimesInitI, stepPart, "sum_red.sum_added");
    if (coeff != 1.0)
    {
        sumAdded = m_builder.CreateFMul(sumAdded, ConstantFP::get(m_doubleTy, coeff), "sum_red.sum_scaled");
    }
    Value *finalSum = m_builder.CreateFAdd(initSum, sumAdded, "sum_red.final_sum");

    Value *kSteps = (stepVal == 1.0) ? k : m_builder.CreateFMul(k, stepValConst, "sum_red.k_steps");
    Value *finalI = m_builder.CreateFAdd(initI, kSteps, "sum_red.final_i");

    m_builder.CreateStore(finalSum, sumPtr);
    m_builder.CreateStore(finalI, iPtr);

    if (!m_declaredLocals.count(varSum))
    {
        Value *nameStr = m_builder.CreateGlobalStringPtr(varSum);
        Value *boxedVal = boxToTzdValue(finalSum);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
    }
    if (!m_declaredLocals.count(varI))
    {
        Value *nameStr = m_builder.CreateGlobalStringPtr(varI);
        Value *boxedVal = boxToTzdValue(finalI);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
    }

    m_builder.CreateBr(doneBB);
    m_builder.SetInsertPoint(doneBB);
    return true;
}

static void collectIndexedContainers(antlr4::tree::ParseTree *tree, std::set<std::string> &containers)
{
    if (!tree)
        return;
    if (auto *idx = dynamic_cast<TzdLangParser::IndexExprContext *>(tree))
    {
        if (idx->expression(0))
        {
            std::string cName = idx->expression(0)->getText();
            if (!cName.empty() && (isalpha((unsigned char)cName[0]) || cName[0] == '_'))
            {
                containers.insert(cName);
            }
        }
    }
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        collectIndexedContainers(tree->children[i], containers);
    }
}

static void collectAssignedVariables(antlr4::tree::ParseTree *tree, std::set<std::string> &vars)
{
    if (!tree)
        return;
    if (auto *assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(tree))
    {
        if (assign->expression(0))
        {
            std::string vName = assign->expression(0)->getText();
            if (!vName.empty() && (isalpha((unsigned char)vName[0]) || vName[0] == '_'))
            {
                vars.insert(vName);
            }
        }
    }
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        collectAssignedVariables(tree->children[i], vars);
    }
}

static void collectDeclaredVariables(antlr4::tree::ParseTree *tree, std::set<std::string> &vars)
{
    if (!tree)
        return;
    if (auto *decl = dynamic_cast<TzdLangParser::VariableDeclarationContext *>(tree))
    {
        if (decl->IDENTIFIER())
        {
            std::string vName = decl->IDENTIFIER()->getText();
            if (!vName.empty())
                vars.insert(vName);
        }
    }
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        collectDeclaredVariables(tree->children[i], vars);
    }
}

static bool treeContainsBreakOrContinue(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return false;
    if (dynamic_cast<TzdLangParser::BreakStmtContext *>(tree))
        return true;
    if (dynamic_cast<TzdLangParser::ContinueStmtContext *>(tree))
        return true;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (treeContainsBreakOrContinue(tree->children[i]))
            return true;
    }
    return false;
}

static bool treeContainsCallExpr(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return false;
    if (dynamic_cast<TzdLangParser::CallExprContext *>(tree))
        return true;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (treeContainsCallExpr(tree->children[i]))
            return true;
    }
    return false;
}

static bool treeContainsNestedLoop(antlr4::tree::ParseTree *tree)
{
    if (!tree)
        return false;
    if (dynamic_cast<TzdLangParser::WhileStmtContext *>(tree) ||
        dynamic_cast<TzdLangParser::ForStmtContext *>(tree))
        return true;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (treeContainsNestedLoop(tree->children[i]))
            return true;
    }
    return false;
}

static int countVariableAssignments(antlr4::tree::ParseTree *tree, const std::string &varName)
{
    if (!tree)
        return 0;
    int cnt = 0;
    if (auto *assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(tree))
    {
        if (assign->expression(0) && getExprIdentifier(assign->expression(0)) == varName)
        {
            cnt++;
        }
    }
    else if (auto *post = dynamic_cast<TzdLangParser::PostfixExprContext *>(tree))
    {
        if ((post->INC() || post->DEC()) && getExprIdentifier(post->expression()) == varName)
        {
            cnt++;
        }
    }
    else if (auto *pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(tree))
    {
        if ((pre->INC() || pre->DEC()) && getExprIdentifier(pre->expression()) == varName)
        {
            cnt++;
        }
    }
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        cnt += countVariableAssignments(tree->children[i], varName);
    }
    return cnt;
}

static bool matchInductionStep(TzdLangParser::StatementContext *stmt,
                               const std::string &varName,
                               TzdLangParser::ExpressionContext *&outStepExpr,
                               bool &outIsPositive)
{
    outStepExpr = nullptr;
    outIsPositive = true;
    if (!stmt)
        return false;
    auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(stmt);
    if (!exprStmt || !exprStmt->expression())
        return false;
    auto expr = exprStmt->expression();

    // Pattern 1: varI++ or ++varI
    if (auto post = dynamic_cast<TzdLangParser::PostfixExprContext *>(expr))
    {
        if (post->INC() && getExprIdentifier(post->expression()) == varName)
        {
            outIsPositive = true;
            return true;
        }
        if (post->DEC() && getExprIdentifier(post->expression()) == varName)
        {
            outIsPositive = false;
            return true;
        }
    }
    if (auto pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(expr))
    {
        if (pre->INC() && getExprIdentifier(pre->expression()) == varName)
        {
            outIsPositive = true;
            return true;
        }
        if (pre->DEC() && getExprIdentifier(pre->expression()) == varName)
        {
            outIsPositive = false;
            return true;
        }
    }

    // Pattern 2: varI += step  or  varI -= step
    if (auto assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(expr))
    {
        if (assign->expression(0) && getExprIdentifier(assign->expression(0)) == varName && assign->expression(1))
        {
            if (assign->PLUS_ASSIGN())
            {
                outStepExpr = assign->expression(1);
                outIsPositive = true;
                return true;
            }
            if (assign->MIN_ASSIGN())
            {
                outStepExpr = assign->expression(1);
                outIsPositive = false;
                return true;
            }
            // Pattern 3: varI = varI + step  or  varI = step + varI
            if (assign->ASSIGN())
            {
                if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(assign->expression(1)))
                {
                    if (add->PLUS() && add->expression(0) && add->expression(1))
                    {
                        if (getExprIdentifier(add->expression(0)) == varName)
                        {
                            outStepExpr = add->expression(1);
                            outIsPositive = true;
                            return true;
                        }
                        if (getExprIdentifier(add->expression(1)) == varName)
                        {
                            outStepExpr = add->expression(0);
                            outIsPositive = true;
                            return true;
                        }
                    }
                    if (add->MINUS() && add->expression(0) && add->expression(1))
                    {
                        if (getExprIdentifier(add->expression(0)) == varName)
                        {
                            outStepExpr = add->expression(1);
                            outIsPositive = false;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool treeContainsAnyAssigned(antlr4::tree::ParseTree *tree, const std::set<std::string> &assignedVars)
{
    if (!tree)
        return false;
    std::string id = getExprIdentifier(tree);
    if (!id.empty() && assignedVars.count(id) > 0)
        return true;
    for (size_t i = 0; i < tree->children.size(); ++i)
    {
        if (treeContainsAnyAssigned(tree->children[i], assignedVars))
            return true;
    }
    return false;
}

std::any TzdCompiler::visitWhileStmt(TzdLangParser::WhileStmtContext *ctx)
{
    if (tryEmitCanonicalSumReduction(ctx))
    {
        return std::any();
    }
    Function *func = m_builder.GetInsertBlock()->getParent();
    BasicBlock *condBB = BasicBlock::Create(m_context, "loop_cond");
    BasicBlock *bodyBB = BasicBlock::Create(m_context, "loop_body");
    BasicBlock *endBB = BasicBlock::Create(m_context, "loop_end");

    // Loop Invariant Code Motion (LICM): Hoist loop-invariant array container buffers
    auto savedHoistedArrays = m_hoistedArrays;
    auto savedShadowI64IndVars = m_shadowI64IndVars;

    bool hasNestedLoop = treeContainsNestedLoop(ctx->statement());

    std::set<std::string> indexedContainers;
    collectIndexedContainers(ctx->statement(), indexedContainers);
    std::set<std::string> assignedVars;
    collectAssignedVariables(ctx->statement(), assignedVars);
    collectDeclaredVariables(ctx->statement(), assignedVars);

    // Bounds Check Elimination (BCE) & Induction Variable Recognition
    std::string loopIdxVar = "";
    Value *safeBoundI64 = nullptr;
    bool isLT = false;
    bool isLE = false;
    bool isSquareCondition = false;
    TzdLangParser::RelationalExprContext *relCtx = dynamic_cast<TzdLangParser::RelationalExprContext *>(ctx->expression());
    if (!relCtx)
    {
        if (auto andCtx = dynamic_cast<TzdLangParser::LogicalAndExprContext *>(ctx->expression()))
        {
            relCtx = dynamic_cast<TzdLangParser::RelationalExprContext *>(andCtx->expression(0));
            if (!relCtx)
            {
                relCtx = dynamic_cast<TzdLangParser::RelationalExprContext *>(andCtx->expression(1));
            }
        }
    }

    if (relCtx)
    {
        if (relCtx->LT() || relCtx->LE())
        {
            isLT = (relCtx->LT() != nullptr);
            isLE = (relCtx->LE() != nullptr);
            std::string lhsText = getExprIdentifier(relCtx->expression(0));
            if (lhsText.empty())
            {
                if (auto mul = dynamic_cast<TzdLangParser::MultiplicativeExprContext *>(relCtx->expression(0)))
                {
                    if (mul->MUL() && mul->expression(0) && mul->expression(1))
                    {
                        std::string id0 = getExprIdentifier(mul->expression(0));
                        std::string id1 = getExprIdentifier(mul->expression(1));
                        if (!id0.empty() && id0 == id1)
                        {
                            lhsText = id0;
                            isSquareCondition = true;
                        }
                    }
                }
            }
            if (lhsText.empty())
                lhsText = relCtx->expression(0)->getText();
            if (assignedVars.count(lhsText) > 0)
            {
                loopIdxVar = lhsText;
                if (!treeContainsAnyAssigned(relCtx->expression(1), assignedVars))
                {
                    double boundConst = 0.0;
                    if (getExprConstantNumber(relCtx->expression(1), boundConst))
                    {
                        safeBoundI64 = m_builder.getInt64((int64_t)boundConst);
                    }
                    else
                    {
                        Value *boundRaw = castAnyToValue(visit(relCtx->expression(1)), "licm_bound");
                        Value *boundD = castToNativeDouble(boundRaw);
                        safeBoundI64 = m_builder.CreateFPToSI(boundD, m_builder.getInt64Ty(), "licm_bound_i64");
                    }
                }
            }
        }
    }

    for (const std::string &cName : indexedContainers)
    {
        // 【核心修复】：移除 m_namedValues.count(cName) 的限制，允许全局数组 G 提速！
        if (assignedVars.count(cName) == 0)
        {
            auto outerHIt = savedHoistedArrays.find(cName);
            Value *container = nullptr;
            Value *fastBuf = nullptr;
            Value *fastLen = nullptr;
            Value *hasBuf = nullptr;
            if (outerHIt != savedHoistedArrays.end())
            {
                container = outerHIt->second.container;
                fastBuf = outerHIt->second.fastBuf;
                fastLen = outerHIt->second.fastLen;
                hasBuf = outerHIt->second.hasBuf;
            }
            else
            {
                if (m_namedValues.count(cName) > 0)
                {
                    container = m_builder.CreateLoad(m_ptrTy, m_namedValues[cName], cName + "_licm_cont");
                }
                else
                {
                    Value *nameStr = m_builder.CreateGlobalStringPtr(cName);
                    int line = ctx->getStart() ? (int)ctx->getStart()->getLine() : 0;
                    int col = ctx->getStart() ? (int)ctx->getStart()->getCharPositionInLine() : 0;
                    container = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr, m_builder.getInt32(line), m_builder.getInt32(col)});
                }

                fastBuf = m_builder.CreateLoad(m_ptrTy, container, cName + "_licm_buf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, cName + "_licm_lenPtr");
                fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, cName + "_licm_len");
                hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), cName + "_licm_has_buf");
            }

            Value *safeFastCond = nullptr;
            if (safeBoundI64 && !loopIdxVar.empty())
            {
                Value *isPositive = m_builder.CreateICmpSGT(safeBoundI64, m_builder.getInt64(0), cName + "_bound_pos");
                Value *boundOk = m_builder.CreateICmpSLE(safeBoundI64, fastLen, cName + "_bound_ok");
                Value *validBound = m_builder.CreateAnd(isPositive, boundOk, cName + "_bound_valid");
                safeFastCond = m_builder.CreateAnd(hasBuf, validBound, cName + "_safe_fast");
            }

            m_hoistedArrays[cName] = {container, fastBuf, fastLen, hasBuf, loopIdxVar, safeFastCond, false};
        }
    }

    Value *combinedSafeFast = nullptr;
    for (auto &[cName, info] : m_hoistedArrays)
    {
        if (info.safeFastCond)
        {
            combinedSafeFast = combinedSafeFast ? m_builder.CreateAnd(combinedSafeFast, info.safeFastCond) : info.safeFastCond;
        }
    }

    // --- 核心黑科技：AST 级别的手动循环分裂克隆 (Manual Loop Versioning) ---
    // 生成一个 Lambda 函数，允许我们将同一个 AST 循环体完美发射出两套不同的 IR 机器码
    auto emitLoop = [&](bool isFast)
    {
        // 设置当前循环的快速通道标志
        for (auto &[cName, info] : m_hoistedArrays)
        {
            if (info.safeFastCond)
                info.inFastLoop = isFast;
        }

        BasicBlock *condBB = BasicBlock::Create(m_context, isFast ? "fast_loop_cond" : "slow_loop_cond", func);
        BasicBlock *bodyBB = BasicBlock::Create(m_context, isFast ? "fast_loop_body" : "slow_loop_body", func);
        BasicBlock *endBB = BasicBlock::Create(m_context, isFast ? "fast_loop_end" : "slow_loop_end", func);

        // [Shadow IndVar 循环变量原生投影逻辑]
        bool canShadowIndVar = false;
        TzdLangParser::ExpressionContext *indStepExpr = nullptr;
        bool indStepPositive = true;
        std::vector<TzdLangParser::StatementContext *> bodyStmts;
        if (!loopIdxVar.empty() && m_nativeDoubleLocals.count(loopIdxVar) > 0 && safeBoundI64 != nullptr && !treeContainsBreakOrContinue(ctx->statement()))
        {
            if (countVariableAssignments(ctx->statement(), loopIdxVar) == 1)
            {
                if (auto blockStmt = dynamic_cast<TzdLangParser::BlockStmtContext *>(ctx->statement()))
                {
                    if (blockStmt->block())
                    {
                        bodyStmts = blockStmt->block()->statement();
                        if (!bodyStmts.empty() && matchInductionStep(bodyStmts.back(), loopIdxVar, indStepExpr, indStepPositive))
                        {
                            if (!indStepExpr || !treeContainsAnyAssigned(indStepExpr, assignedVars))
                                canShadowIndVar = true;
                        }
                    }
                }
                else if (auto singleStmt = dynamic_cast<TzdLangParser::StatementContext *>(ctx->statement()))
                {
                    if (matchInductionStep(singleStmt, loopIdxVar, indStepExpr, indStepPositive))
                    {
                        if (!indStepExpr || !treeContainsAnyAssigned(indStepExpr, assignedVars))
                            canShadowIndVar = true;
                    }
                }
            }
        }

        if (canShadowIndVar)
        {
            Value *varIAlloc = m_nativeDoubleLocals[loopIdxVar];
            Value *initD = m_builder.CreateLoad(m_doubleTy, varIAlloc, loopIdxVar + "_initD");
            Value *initI64 = m_builder.CreateFPToSI(initD, m_builder.getInt64Ty(), loopIdxVar + "_initI64");

            AllocaInst *shadowAlloc = CreateEntryBlockAlloca(m_builder.getInt64Ty(), loopIdxVar + (isFast ? "_shadow_f" : "_shadow_s"));
            m_builder.CreateStore(initI64, shadowAlloc);

            Value *stepValI64 = nullptr;
            if (!indStepExpr)
            {
                stepValI64 = m_builder.getInt64(indStepPositive ? 1 : -1);
            }
            else
            {
                double c = 0.0;
                if (getExprConstantNumber(indStepExpr, c))
                {
                    int64_t stepInt = (int64_t)c;
                    if (!indStepPositive)
                        stepInt = -stepInt;
                    stepValI64 = m_builder.getInt64(stepInt);
                }
                else
                {
                    std::string stepId = getExprIdentifier(indStepExpr);
                    if (!stepId.empty() && m_shadowI64IndVars.count(stepId))
                    {
                        stepValI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[stepId], stepId + "_shadow_step_i64");
                        if (!indStepPositive)
                            stepValI64 = m_builder.CreateNeg(stepValI64, loopIdxVar + "_stepNeg");
                    }
                    else
                    {
                        Value *stepRaw = castAnyToValue(visit(indStepExpr), (loopIdxVar + "_stepRaw").c_str());
                        Value *stepD = castToNativeDouble(stepRaw);
                        stepValI64 = m_builder.CreateFPToSI(stepD, m_builder.getInt64Ty(), loopIdxVar + "_stepI64");
                        if (!indStepPositive)
                            stepValI64 = m_builder.CreateNeg(stepValI64, loopIdxVar + "_stepNeg");
                    }
                }
            }
            size_t visitCount = bodyStmts.empty() ? 0 : (bodyStmts.size() - 1);

            m_loopStack.push_back({condBB, endBB});
            m_builder.CreateBr(condBB);

            m_builder.SetInsertPoint(condBB);
            Value *curI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), shadowAlloc, loopIdxVar + "_cur_i64");

            auto buildRelIcmp = [&]() -> Value *
            {
                Value *cmpLhs = curI64;
                if (isSquareCondition)
                    cmpLhs = m_builder.CreateMul(curI64, curI64, loopIdxVar + "_sq");
                return isLT ? m_builder.CreateICmpSLT(cmpLhs, safeBoundI64, loopIdxVar + "_cond_slt")
                            : m_builder.CreateICmpSLE(cmpLhs, safeBoundI64, loopIdxVar + "_cond_sle");
            };

            Value *isTrue = nullptr;
            if (auto andCtx = dynamic_cast<TzdLangParser::LogicalAndExprContext *>(ctx->expression()))
            {
                AllocaInst *andRes = CreateEntryBlockAlloca(m_boolTy, nullptr, "loop_and_res");
                if (andCtx->expression(0) == relCtx)
                {
                    Value *leftTrue = buildRelIcmp();
                    BasicBlock *rightBB = BasicBlock::Create(m_context, "loop_and_right", func);
                    BasicBlock *mergeBB = BasicBlock::Create(m_context, "loop_and_merge", func);

                    m_builder.CreateStore(m_builder.getFalse(), andRes);
                    m_builder.CreateCondBr(leftTrue, rightBB, mergeBB);

                    m_builder.SetInsertPoint(rightBB);
                    Value *rightTrue = emitTruthyCond(andCtx->expression(1));
                    m_builder.CreateStore(rightTrue, andRes);
                    m_builder.CreateBr(mergeBB);

                    m_builder.SetInsertPoint(mergeBB);
                    isTrue = m_builder.CreateLoad(m_boolTy, andRes);
                }
                else if (andCtx->expression(1) == relCtx)
                {
                    BasicBlock *rightBB = BasicBlock::Create(m_context, "loop_and_right", func);
                    BasicBlock *mergeBB = BasicBlock::Create(m_context, "loop_and_merge", func);

                    m_builder.CreateStore(m_builder.getFalse(), andRes);
                    Value *leftTrue = emitTruthyCond(andCtx->expression(0));
                    m_builder.CreateCondBr(leftTrue, rightBB, mergeBB);

                    m_builder.SetInsertPoint(rightBB);
                    Value *rightTrue = buildRelIcmp();
                    m_builder.CreateStore(rightTrue, andRes);
                    m_builder.CreateBr(mergeBB);

                    m_builder.SetInsertPoint(mergeBB);
                    isTrue = m_builder.CreateLoad(m_boolTy, andRes);
                }
                else
                {
                    isTrue = emitTruthyCond(ctx->expression());
                }
            }
            else if (ctx->expression() == relCtx)
            {
                isTrue = buildRelIcmp();
            }
            else
            {
                isTrue = emitTruthyCond(ctx->expression());
            }

            m_builder.CreateCondBr(isTrue, bodyBB, endBB);

            m_builder.SetInsertPoint(bodyBB);

            // 将当前影子变量绑定，供 visit(body) 读取
            m_shadowI64IndVars[loopIdxVar] = shadowAlloc;

            s_loopLevel++;
            for (size_t i = 0; i < visitCount; ++i)
            {
                visit(bodyStmts[i]);
                if (m_builder.GetInsertBlock()->getTerminator())
                    break;
            }
            s_loopLevel--;

            if (!m_builder.GetInsertBlock()->getTerminator())
            {
                Value *curI64Latch = m_builder.CreateLoad(m_builder.getInt64Ty(), shadowAlloc, loopIdxVar + "_latch_i64");
                Value *nextI64 = m_builder.CreateNSWAdd(curI64Latch, stepValI64, loopIdxVar + "_nextI64");
                m_builder.CreateStore(nextI64, shadowAlloc);
                BranchInst *latchBr = m_builder.CreateBr(condBB);

                SmallVector<Metadata *, 3> loopMD;
                loopMD.push_back(nullptr);
                if (hasNestedLoop)
                {
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.disable")}));
                }
                else
                {
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.enable")}));
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.count"),
                                                             ConstantAsMetadata::get(m_builder.getInt32(8))}));
                }
                MDNode *loopID = MDNode::getDistinct(m_context, loopMD);
                loopID->replaceOperandWith(0, loopID);
                latchBr->setMetadata("llvm.loop", loopID);
            }

            m_builder.SetInsertPoint(endBB);
            Value *exitI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), shadowAlloc, loopIdxVar + "_exit_i64");
            Value *exitD = m_builder.CreateSIToFP(exitI64, m_doubleTy, loopIdxVar + "_exitD");
            m_builder.CreateStore(exitD, varIAlloc);
            if (!m_declaredLocals.count(loopIdxVar))
            {
                Value *nameStr = m_builder.CreateGlobalStringPtr(loopIdxVar);
                Value *boxedVal = boxToTzdValue(exitD);
                m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
            }

            m_loopStack.pop_back();
        }
        else
        {
            // [普通的非纯净循环逻辑]
            m_loopStack.push_back({condBB, endBB});

            m_builder.CreateBr(condBB);
            m_builder.SetInsertPoint(condBB);
            Value *isTrue = emitTruthyCond(ctx->expression());
            m_builder.CreateCondBr(isTrue, bodyBB, endBB);

            m_builder.SetInsertPoint(bodyBB);
            s_loopLevel++;
            visit(ctx->statement());
            s_loopLevel--;
            if (!m_builder.GetInsertBlock()->getTerminator())
            {
                BranchInst *latchBr = m_builder.CreateBr(condBB);
                SmallVector<Metadata *, 3> loopMD;
                loopMD.push_back(nullptr);
                if (hasNestedLoop)
                {
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.disable")}));
                }
                else
                {
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.enable")}));
                    loopMD.push_back(MDNode::get(m_context, {MDString::get(m_context, "llvm.loop.unroll.count"),
                                                             ConstantAsMetadata::get(m_builder.getInt32(8))}));
                }
                MDNode *loopID = MDNode::getDistinct(m_context, loopMD);
                loopID->replaceOperandWith(0, loopID);
                latchBr->setMetadata("llvm.loop", loopID);
            }

            m_builder.SetInsertPoint(endBB);
            m_loopStack.pop_back();
        }
    };

    // --- 根据运行时的安全性进行克隆派发 ---
    if (combinedSafeFast)
    {
        BasicBlock *fastLoopBB = BasicBlock::Create(m_context, "loop_bce_fast_pre", func);
        BasicBlock *slowLoopBB = BasicBlock::Create(m_context, "loop_bce_slow_pre", func);
        BasicBlock *mergeBB = BasicBlock::Create(m_context, "loop_bce_merge", func);

        // 运行时，如果确认为安全的数组，走 Fast 版本，否则(比如是Map)走 Slow 版本
        m_builder.CreateCondBr(combinedSafeFast, fastLoopBB, slowLoopBB);

        // 1. 发射极限速度版本循环
        m_builder.SetInsertPoint(fastLoopBB);
        emitLoop(true);
        if (!m_builder.GetInsertBlock()->getTerminator())
            m_builder.CreateBr(mergeBB);

        // 状态重置，防止第二次发射时变量交叉污染
        m_shadowI64IndVars = savedShadowI64IndVars;
        m_hoistedArrays = savedHoistedArrays;

        // 2. 发射带兼容性的安全版本循环
        m_builder.SetInsertPoint(slowLoopBB);
        emitLoop(false);
        if (!m_builder.GetInsertBlock()->getTerminator())
            m_builder.CreateBr(mergeBB);

        m_builder.SetInsertPoint(mergeBB);
    }
    else
    {
        // 没有任何可优化的数组，直接发射一次
        emitLoop(false);
    }

    m_hoistedArrays = savedHoistedArrays;
    m_shadowI64IndVars = savedShadowI64IndVars;
    return std::any();
}

std::any TzdCompiler::visitBreakStmt(TzdLangParser::BreakStmtContext *ctx)
{
    (void)ctx;
    // ======= 清理跳出的循环内部所在的 try 块 =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it)
    {
        if (std::get<2>(*it) >= s_loopLevel)
        {
            m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {std::get<1>(*it)});
            m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), {std::get<0>(*it)});
        }
        else
        {
            break;
        }
    }
    // ===========================================

    if (!m_switchEndStack.empty())
        m_builder.CreateBr(m_switchEndStack.back());
    else if (!m_loopStack.empty())
        m_builder.CreateBr(m_loopStack.back().breakBB);
    return std::any();
}

std::any TzdCompiler::visitContinueStmt(TzdLangParser::ContinueStmtContext *ctx)
{
    (void)ctx;
    // ======= 清理同上 =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it)
    {
        if (std::get<2>(*it) >= s_loopLevel)
        {
            m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {std::get<1>(*it)});
            m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), {std::get<0>(*it)});
        }
        else
        {
            break;
        }
    }
    // =======================

    if (m_loopStack.empty())
        return std::any();
    m_builder.CreateBr(m_loopStack.back().continueBB);
    return std::any();
}

std::any TzdCompiler::visitSwitchStmt(TzdLangParser::SwitchStmtContext *ctx)
{
    Value *switchVal = boxToTzdValue(std::any_cast<Value *>(visit(ctx->expression())));
    Function *func = m_builder.GetInsertBlock()->getParent();
    BasicBlock *endBB = BasicBlock::Create(m_context, "switch.end", func);

    AllocaInst *matchedAlloca = CreateEntryBlockAlloca(m_boolTy, nullptr, "switch.matched");
    m_builder.CreateStore(m_builder.getFalse(), matchedAlloca);

    BasicBlock *prevAfterBB = nullptr;
    int caseIdx = 0;
    for (auto *caseCtx : ctx->switchCase())
    {
        BasicBlock *checkBB = BasicBlock::Create(m_context, "switch.chk" + std::to_string(caseIdx), func);
        BasicBlock *bodyBB = BasicBlock::Create(m_context, "switch.body" + std::to_string(caseIdx), func);
        BasicBlock *afterBB = BasicBlock::Create(m_context, "switch.after" + std::to_string(caseIdx), func);
        caseIdx++;

        if (prevAfterBB == nullptr)
        {
            m_builder.CreateBr(checkBB);
        }
        else
        {
            m_builder.SetInsertPoint(prevAfterBB);
            if (!m_builder.GetInsertBlock()->getTerminator())
            {
                m_builder.CreateBr(checkBB);
            }
        }

        m_builder.SetInsertPoint(checkBB);
        Value *alreadyMatched = m_builder.CreateLoad(m_boolTy, matchedAlloca);
        BasicBlock *eqCheckBB = BasicBlock::Create(m_context, "switch.eq" + std::to_string(caseIdx), func);
        m_builder.CreateCondBr(alreadyMatched, bodyBB, eqCheckBB);

        m_builder.SetInsertPoint(eqCheckBB);
        Value *caseVal = boxToTzdValue(std::any_cast<Value *>(visit(caseCtx->expression())));
        Value *eq = m_builder.CreateCall(getRtFunc("rt_op_eq"), {switchVal, caseVal});
        Value *eqBool = toNativeBool(eq);
        m_builder.CreateCondBr(eqBool, bodyBB, afterBB);

        m_builder.SetInsertPoint(bodyBB);
        m_builder.CreateStore(m_builder.getTrue(), matchedAlloca);
        for (auto *stmt : caseCtx->statement())
        {
            visit(stmt);
            if (m_builder.GetInsertBlock()->getTerminator())
                break;
        }
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(afterBB);
        }

        prevAfterBB = afterBB;
    }

    if (prevAfterBB)
    {
        m_builder.SetInsertPoint(prevAfterBB);
    }

    if (ctx->switchDefault())
    {
        BasicBlock *defaultCheckBB = BasicBlock::Create(m_context, "switch.default.chk", func);
        BasicBlock *defaultSetBB = BasicBlock::Create(m_context, "switch.default.set", func);
        BasicBlock *defaultBodyBB = BasicBlock::Create(m_context, "switch.default.body", func);
        BasicBlock *defaultAfterBB = BasicBlock::Create(m_context, "switch.default.after", func);

        m_builder.CreateBr(defaultCheckBB);
        m_builder.SetInsertPoint(defaultCheckBB);
        Value *alreadyMatched = m_builder.CreateLoad(m_boolTy, matchedAlloca);
        m_builder.CreateCondBr(alreadyMatched, defaultBodyBB, defaultSetBB);

        m_builder.SetInsertPoint(defaultSetBB);
        m_builder.CreateStore(m_builder.getTrue(), matchedAlloca);
        m_builder.CreateBr(defaultBodyBB);

        m_builder.SetInsertPoint(defaultBodyBB);
        for (auto *stmt : ctx->switchDefault()->statement())
        {
            visit(stmt);
            if (m_builder.GetInsertBlock()->getTerminator())
                break;
        }
        if (!m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(defaultAfterBB);
        }
        m_builder.SetInsertPoint(defaultAfterBB);
    }

    m_switchEndStack.push_back(endBB);

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateBr(endBB);
    }
    m_builder.SetInsertPoint(endBB);
    m_switchEndStack.pop_back();
    return std::any();
}

std::any TzdCompiler::visitForStmt(TzdLangParser::ForStmtContext *ctx)
{
    auto backupScope = m_namedValues;
    auto backupNative = m_nativeDoubleLocals;

    bool initHandled = false;

    if (ctx->forInit())
    {
        if (auto expr = ctx->forInit()->expression())
        {
            if (auto assignCtx = dynamic_cast<TzdLangParser::AssignmentExprContext *>(expr))
            {
                std::string varName = assignCtx->expression(0)->getText();
                auto valExpr = assignCtx->expression(1);

                Value *initVal = std::any_cast<Value *>(visit(valExpr));

                if (initVal->getType()->isDoubleTy() &&
                    m_nativeDoubleLocals.find(varName) == m_nativeDoubleLocals.end() &&
                    m_namedValues.find(varName) == m_namedValues.end())
                {
                    AllocaInst *alloc = m_builder.CreateAlloca(m_doubleTy, nullptr, varName + "_loop_native");
                    m_builder.CreateStore(initVal, alloc);

                    m_nativeDoubleLocals[varName] = alloc;
                    initHandled = true;
                }
            }
        }
    }

    if (!initHandled && ctx->forInit())
    {
        visit(ctx->forInit());
    }

    auto trySumReductionForFor = [&]() -> bool
    {
        if (!ctx->cond || !ctx->step || !ctx->statement())
            return false;

        auto relCtx = dynamic_cast<TzdLangParser::RelationalExprContext *>(ctx->cond);
        if (!relCtx || !relCtx->expression(0) || !relCtx->expression(1))
            return false;

        bool isLT = (relCtx->LT() != nullptr);
        bool isLE = (relCtx->LE() != nullptr);
        if (!isLT && !isLE)
            return false;

        std::string varI = getExprIdentifier(relCtx->expression(0));
        if (varI.empty() || !m_nativeDoubleLocals.count(varI))
            return false;

        if (treeContainsId(relCtx->expression(1), varI))
            return false;

        TzdLangParser::StatementContext *bodyStmt = ctx->statement();
        if (auto blockStmt = dynamic_cast<TzdLangParser::BlockStmtContext *>(bodyStmt))
        {
            if (!blockStmt->block() || blockStmt->block()->statement().size() != 1)
                return false;
            bodyStmt = blockStmt->block()->statement(0);
        }

        std::string varSum;
        double coeff = 1.0;
        if (!matchAccumulationStmt(bodyStmt, varI, varSum, coeff))
            return false;

        TzdLangParser::ExpressionContext *stepExprCtx = dynamic_cast<TzdLangParser::ExpressionContext *>(ctx->step);
        if (!stepExprCtx)
        {
            if (auto exprList = dynamic_cast<TzdLangParser::ExprListContext *>(ctx->step))
            {
                if (exprList->expression().size() == 1)
                    stepExprCtx = exprList->expression(0);
            }
            else if (auto stmtCtx = dynamic_cast<TzdLangParser::StatementContext *>(ctx->step))
            {
                if (auto exprStmt = dynamic_cast<TzdLangParser::ExprStmtContext *>(stmtCtx))
                {
                    stepExprCtx = exprStmt->expression();
                }
            }
        }
        if (!stepExprCtx)
            return false;
        double stepVal = 1.0;
        bool stepMatched = false;
        if (auto post = dynamic_cast<TzdLangParser::PostfixExprContext *>(stepExprCtx))
        {
            if (post->INC() && getExprIdentifier(post->expression()) == varI)
            {
                stepVal = 1.0;
                stepMatched = true;
            }
        }
        else if (auto pre = dynamic_cast<TzdLangParser::PrefixExprContext *>(stepExprCtx))
        {
            if (pre->INC() && getExprIdentifier(pre->expression()) == varI)
            {
                stepVal = 1.0;
                stepMatched = true;
            }
        }
        else if (auto assign = dynamic_cast<TzdLangParser::AssignmentExprContext *>(stepExprCtx))
        {
            if (assign->expression(0) && getExprIdentifier(assign->expression(0)) == varI && assign->expression(1))
            {
                if (assign->PLUS_ASSIGN())
                {
                    stepMatched = getExprConstantNumber(assign->expression(1), stepVal);
                }
                else if (assign->ASSIGN())
                {
                    if (auto add = dynamic_cast<TzdLangParser::AdditiveExprContext *>(assign->expression(1)))
                    {
                        if (add->PLUS() && add->expression(0) && add->expression(1))
                        {
                            std::string op0 = getExprIdentifier(add->expression(0));
                            std::string op1 = getExprIdentifier(add->expression(1));
                            if (op0 == varI)
                                stepMatched = getExprConstantNumber(add->expression(1), stepVal);
                            else if (op1 == varI)
                                stepMatched = getExprConstantNumber(add->expression(0), stepVal);
                        }
                    }
                }
            }
        }

        if (!stepMatched)
            return false;

        if (varSum.empty() || varSum == varI)
            return false;
        if (!m_nativeDoubleLocals.count(varSum))
            return false;
        if (treeContainsId(relCtx->expression(1), varSum))
            return false;
        if (stepVal <= 0.0)
            return false;

        Value *limitRaw = castAnyToValue(visit(relCtx->expression(1)), "sum_red.limit");
        Value *limitVal = castToNativeDouble(limitRaw);

        Value *iPtr = m_nativeDoubleLocals[varI];
        Value *sumPtr = m_nativeDoubleLocals[varSum];
        Value *initI = m_builder.CreateLoad(m_doubleTy, iPtr, varI + "_init");
        Value *initSum = m_builder.CreateLoad(m_doubleTy, sumPtr, varSum + "_init");

        Function *func = m_builder.GetInsertBlock()->getParent();
        BasicBlock *fastBB = BasicBlock::Create(m_context, "sum_red.fast", func);
        BasicBlock *doneBB = BasicBlock::Create(m_context, "sum_red.done", func);

        Value *cond = isLE ? m_builder.CreateFCmpOLE(initI, limitVal, "sum_red.cond")
                           : m_builder.CreateFCmpOLT(initI, limitVal, "sum_red.cond");
        m_builder.CreateCondBr(cond, fastBB, doneBB);

        m_builder.SetInsertPoint(fastBB);
        Value *diff = m_builder.CreateFSub(limitVal, initI, "sum_red.diff");
        Value *stepValConst = ConstantFP::get(m_doubleTy, stepVal);
        if (stepVal != 1.0)
        {
            diff = m_builder.CreateFDiv(diff, stepValConst, "sum_red.diff_scaled");
        }

        Value *k = nullptr;
        if (isLE)
        {
            Function *floorFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::floor, {m_doubleTy});
            Value *floorVal = m_builder.CreateCall(floorFunc, {diff}, "sum_red.floor");
            k = m_builder.CreateFAdd(floorVal, ConstantFP::get(m_doubleTy, 1.0), "sum_red.k");
        }
        else
        {
            Function *ceilFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::ceil, {m_doubleTy});
            k = m_builder.CreateCall(ceilFunc, {diff}, "sum_red.k");
        }

        Value *kAdj = m_builder.CreateFSub(k, ConstantFP::get(m_doubleTy, 1.0), "sum_red.k_minus_1");

        Value *kTimesAdj = m_builder.CreateFMul(k, kAdj, "sum_red.k_adj_prod");
        Value *halfKTimesAdj = m_builder.CreateFMul(kTimesAdj, ConstantFP::get(m_doubleTy, 0.5), "sum_red.half_k");
        Value *stepPart = (stepVal == 1.0) ? halfKTimesAdj : m_builder.CreateFMul(halfKTimesAdj, stepValConst, "sum_red.step_part");

        Value *kTimesInitI = m_builder.CreateFMul(k, initI, "sum_red.k_initI");
        Value *sumAdded = m_builder.CreateFAdd(kTimesInitI, stepPart, "sum_red.sum_added");
        if (coeff != 1.0)
        {
            sumAdded = m_builder.CreateFMul(sumAdded, ConstantFP::get(m_doubleTy, coeff), "sum_red.sum_scaled");
        }
        Value *finalSum = m_builder.CreateFAdd(initSum, sumAdded, "sum_red.final_sum");

        Value *kSteps = (stepVal == 1.0) ? k : m_builder.CreateFMul(k, stepValConst, "sum_red.k_steps");
        Value *finalI = m_builder.CreateFAdd(initI, kSteps, "sum_red.final_i");

        m_builder.CreateStore(finalSum, sumPtr);
        m_builder.CreateStore(finalI, iPtr);

        if (!m_declaredLocals.count(varSum))
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(varSum);
            Value *boxedVal = boxToTzdValue(finalSum);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
        }
        if (!m_declaredLocals.count(varI))
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(varI);
            Value *boxedVal = boxToTzdValue(finalI);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
        }

        m_builder.CreateBr(doneBB);
        m_builder.SetInsertPoint(doneBB);
        return true;
    };

    if (trySumReductionForFor())
    {
        m_namedValues = backupScope;
        m_nativeDoubleLocals = backupNative;
        return std::any();
    }

    Function *currentFunc = m_builder.GetInsertBlock()->getParent();
    BasicBlock *condBB = BasicBlock::Create(m_context, "for.cond", currentFunc);
    BasicBlock *bodyBB = BasicBlock::Create(m_context, "for.body", currentFunc);
    BasicBlock *stepBB = BasicBlock::Create(m_context, "for.step", currentFunc);
    BasicBlock *afterBB = BasicBlock::Create(m_context, "for.after", currentFunc);

    s_loopLevel++;
    m_builder.CreateBr(condBB);
    m_builder.SetInsertPoint(condBB);

    if (ctx->cond)
    {
        Value *condValue = std::any_cast<Value *>(visit(ctx->cond));
        Value *isTrue = toNativeBool(condValue);
        m_builder.CreateCondBr(isTrue, bodyBB, afterBB);
    }
    else
    {
        m_builder.CreateBr(bodyBB);
    }

    m_loopStack.push_back({stepBB, afterBB});

    m_builder.SetInsertPoint(bodyBB);

    if (ctx->statement())
    {
        visit(ctx->statement());
    }

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateBr(stepBB);
    }

    m_builder.SetInsertPoint(stepBB);
    if (ctx->step)
    {
        visit(ctx->step);
    }
    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateBr(condBB);
    }

    m_builder.SetInsertPoint(afterBB);
    s_loopLevel--;
    m_loopStack.pop_back();

    m_namedValues = backupScope;
    m_nativeDoubleLocals = backupNative;
    return std::any();
}

// --- 表达式 ---

std::any TzdCompiler::visitIdExpr(TzdLangParser::IdExprContext *ctx)
{
    std::string name = ctx->IDENTIFIER()->getText();
    auto sIt = m_shadowI64IndVars.find(name);
    if (sIt != m_shadowI64IndVars.end())
    {
        Value *i64Val = m_builder.CreateLoad(m_builder.getInt64Ty(), sIt->second, name + "_shadow_val");
        Value *dVal = m_builder.CreateSIToFP(i64Val, m_doubleTy, name + "_shadow_d");
        return std::any((Value *)dVal);
    }
    if (m_nativeDoubleLocals.count(name))
    {
        Value *slot = m_nativeDoubleLocals[name];
        // Direct double arg (native worker) — already a double, return as-is
        if (slot->getType()->isDoubleTy())
        {
            return std::any((Value *)slot);
        }
        Value *val = m_builder.CreateLoad(m_doubleTy, slot, name);
        return std::any((Value *)val);
    }
    if (m_namedValues.count(name))
    {
        Value *val = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        return std::any((Value *)val);
    }
    if (m_namedValues.count("this"))
    {
        auto fIt = m_varFieldAllocas.find("this");
        if (fIt != m_varFieldAllocas.end())
        {
            auto slotIt = fIt->second.find(name);
            if (slotIt != fIt->second.end() && slotIt->second)
            {
                return std::any((Value *)m_builder.CreateLoad(m_doubleTy, slotIt->second, name));
            }
        }

        if (m_currentClassDef && m_currentClassDef->findField(name))
        {
            Value *thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"], "this_v");
            auto it = m_currentClassDef->fieldNameToIndex.find(name);
            if (it != m_currentClassDef->fieldNameToIndex.end())
            {
                ClassField *f = m_currentClassDef->findField(name);
                bool isFloat = f && (f->type == "float" || f->type == "double");
                bool isInt = f && (f->type == "int" || f->type == "i64" || f->type == "long");
                if (isFloat)
                {
                    return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_field_d"),
                                                                  {thisPtr, m_builder.getInt32(it->second)}));
                }
                else if (isInt)
                {
                    return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_field_i64"),
                                                                  {thisPtr, m_builder.getInt32(it->second)}));
                }
            }
            Value *nameStr2 = m_builder.CreateGlobalStringPtr(name);
            TzdSelector sel = internSelectorConstant(name);
            GlobalVariable *ic = new GlobalVariable(*m_module, m_builder.getInt64Ty(), false,
                                                    GlobalValue::InternalLinkage, m_builder.getInt64(0), "ic");
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_member_ic"),
                                                          {thisPtr, m_builder.getInt32((int32_t)sel), nameStr2, ic}));
        }
    }
    Value *nameStr = m_builder.CreateGlobalStringPtr(name);
    int line = ctx->getStart() ? (int)ctx->getStart()->getLine() : 0;
    int col = ctx->getStart() ? (int)ctx->getStart()->getCharPositionInLine() : 0;
    Value *addr = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr,
                                                                     m_builder.getInt32(line),
                                                                     m_builder.getInt32(col)});
    return std::any((Value *)addr);
}

std::any TzdCompiler::visitIntExpr(TzdLangParser::IntExprContext *ctx)
{
    std::string raw = ctx->getText();
    size_t digitStart = (raw.size() > 0 && (raw[0] == '-' || raw[0] == '+')) ? 1 : 0;
    bool isHex = (raw.size() > 2 + digitStart && raw[digitStart] == '0' &&
                  (raw[digitStart + 1] == 'x' || raw[digitStart + 1] == 'X'));
    if (!isHex && raw.size() - digitStart > 19)
    {
        Value *str = m_builder.CreateGlobalStringPtr(raw);
        Value *bigintVal = m_builder.CreateCall(getRtFunc("rt_create_bigint"), {str});
        return std::any((Value *)bigintVal);
    }

    try
    {
        unsigned long long val = std::stoull(raw, nullptr, 0);
        return std::any((Value *)ConstantFP::get(m_doubleTy, (double)val));
    }
    catch (...)
    {
        Value *str = m_builder.CreateGlobalStringPtr(raw);
        Value *bigintVal = m_builder.CreateCall(getRtFunc("rt_create_bigint"), {str});
        return std::any((Value *)bigintVal);
    }
}

std::any TzdCompiler::visitFloatExpr(TzdLangParser::FloatExprContext *ctx)
{
    double val = std::stod(ctx->getText());
    return std::any((Value *)ConstantFP::get(m_doubleTy, val));
}

std::any TzdCompiler::visitStringExpr(TzdLangParser::StringExprContext *ctx)
{
    std::string s = ctx->getText();
    s = s.substr(1, s.size() - 2);
    s = unescapeString(s);
    TzdValue *litVal = internStringLiteral(s);
    Value *addrInt = ConstantInt::get(m_builder.getInt64Ty(), (uint64_t)(uintptr_t)litVal);
    Value *ptrVal = m_builder.CreateIntToPtr(addrInt, m_ptrTy);
    return (Value *)ptrVal;
}

std::any TzdCompiler::visitAdditiveExpr(TzdLangParser::AdditiveExprContext *ctx)
{
    if (ctx->PLUS())
    {
        // Check for 3-way concatenation: (A + B) + C (e.g. "prefix_" + i + "_suffix")
        if (auto leftAdd = dynamic_cast<TzdLangParser::AdditiveExprContext *>(ctx->expression(0)))
        {
            if (leftAdd->PLUS())
            {
                Value *A = castAnyToValue(visit(leftAdd->expression(0)), "visitAdditiveExpr.A");
                Value *B = castAnyToValue(visit(leftAdd->expression(1)), "visitAdditiveExpr.B");
                Value *C = castAnyToValue(visit(ctx->expression(1)), "visitAdditiveExpr.C");

                // Check if A is pointer, B is numeric, C is pointer ("prefix_" + i + "_suffix")
                if (A->getType()->isPointerTy() && (B->getType()->isDoubleTy() || B->getType()->isIntegerTy()) && C->getType()->isPointerTy())
                {
                    Value *dB = castToNativeDouble(B);
                    return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_str_concat_str_num_str"), {A, dB, C}));
                }
                // Check if all 3 are pointers
                if (A->getType()->isPointerTy() && B->getType()->isPointerTy() && C->getType()->isPointerTy())
                {
                    return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_str_concat_3"), {A, B, C}));
                }
                auto emitSafeAdd = [&](Value *X, Value *Y) -> Value * {
                    if (X->getType()->isIntegerTy(64) && Y->getType()->isIntegerTy(64))
                    {
                        return m_builder.CreateNSWAdd(X, Y, "addi64");
                    }
                    if (X->getType()->isDoubleTy() && Y->getType()->isDoubleTy())
                    {
                        return m_builder.CreateFAdd(X, Y, "addtmp");
                    }
                    if ((X->getType()->isDoubleTy() || X->getType()->isIntegerTy()) &&
                        (Y->getType()->isDoubleTy() || Y->getType()->isIntegerTy()) &&
                        !X->getType()->isPointerTy() && !Y->getType()->isPointerTy())
                    {
                        Value *dX = castToNativeDouble(X);
                        Value *dY = castToNativeDouble(Y);
                        return m_builder.CreateFAdd(dX, dY, "addtmp");
                    }
                    if (X->getType()->isPointerTy() && (Y->getType()->isDoubleTy() || Y->getType()->isIntegerTy()))
                    {
                        Value *dY = castToNativeDouble(Y);
                        if (s_compilingNativeWorker)
                        {
                            return m_builder.CreateFAdd(inlineToDoubleFast(X), dY, "addtmp");
                        }
                        return m_builder.CreateCall(getRtFunc("rt_str_concat_str_num"), {X, dY});
                    }
                    if ((X->getType()->isDoubleTy() || X->getType()->isIntegerTy()) && Y->getType()->isPointerTy())
                    {
                        Value *dX = castToNativeDouble(X);
                        if (s_compilingNativeWorker)
                        {
                            return m_builder.CreateFAdd(dX, inlineToDoubleFast(Y), "addtmp");
                        }
                        return m_builder.CreateCall(getRtFunc("rt_str_concat_num_str"), {dX, Y});
                    }
                    if (X->getType()->isPointerTy() && Y->getType()->isPointerTy())
                    {
                        return m_builder.CreateCall(getRtFunc("rt_str_concat"), {X, Y});
                    }
                    Value *boxedX = boxToTzdValue(X);
                    Value *boxedY = boxToTzdValue(Y);
                    return m_builder.CreateCall(getRtFunc("rt_op_add"), {boxedX, boxedY});
                };
                Value *leftRes = emitSafeAdd(A, B);
                return std::any((Value *)emitSafeAdd(leftRes, C));
            }
        }
    }

    Value *L = castAnyToValue(visit(ctx->expression(0)), "visitAdditiveExpr.L");
    Value *R = castAnyToValue(visit(ctx->expression(1)), "visitAdditiveExpr.R");

    // Both native integers: fast path
    if (L->getType()->isIntegerTy(64) && R->getType()->isIntegerTy(64))
    {
        if (ctx->PLUS())
        {
            return std::any((Value *)m_builder.CreateNSWAdd(L, R, "addi64"));
        }
        return std::any((Value *)m_builder.CreateNSWSub(L, R, "subi64"));
    }

    // Both doubles: fast path
    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy())
    {
        if (ctx->PLUS())
        {
            return std::any((Value *)m_builder.CreateFAdd(L, R, "addtmp"));
        }
        return std::any((Value *)m_builder.CreateFSub(L, R, "subtmp"));
    }

    // Mixed numeric fast path
    if ((L->getType()->isDoubleTy() || L->getType()->isIntegerTy()) &&
        (R->getType()->isDoubleTy() || R->getType()->isIntegerTy()) &&
        !L->getType()->isPointerTy() && !R->getType()->isPointerTy())
    {
        Value *dL = castToNativeDouble(L);
        Value *dR = castToNativeDouble(R);
        if (ctx->PLUS())
        {
            return std::any((Value *)m_builder.CreateFAdd(dL, dR, "addtmp"));
        }
        return std::any((Value *)m_builder.CreateFSub(dL, dR, "subtmp"));
    }

    if (ctx->PLUS())
    {
        if (L->getType()->isPointerTy() && (R->getType()->isDoubleTy() || R->getType()->isIntegerTy()))
        {
            Value *dR = castToNativeDouble(R);
            if (s_compilingNativeWorker)
            {
                return std::any((Value *)m_builder.CreateFAdd(inlineToDoubleFast(L), dR, "addtmp"));
            }
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_str_concat_str_num"), {L, dR}));
        }
        if ((L->getType()->isDoubleTy() || L->getType()->isIntegerTy()) && R->getType()->isPointerTy())
        {
            Value *dL = castToNativeDouble(L);
            if (s_compilingNativeWorker)
            {
                return std::any((Value *)m_builder.CreateFAdd(dL, inlineToDoubleFast(R), "addtmp"));
            }
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_str_concat_num_str"), {dL, R}));
        }
        if (L->getType()->isPointerTy() && R->getType()->isPointerTy())
        {
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_str_concat"), {L, R}));
        }
        Value *boxedL = boxToTzdValue(L);
        Value *boxedR = boxToTzdValue(R);
        return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_add"), {boxedL, boxedR}));
    }

    Value *boxedL = boxToTzdValue(L);
    Value *boxedR = boxToTzdValue(R);
    return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_sub"), {boxedL, boxedR}));
}
static bool collectSelfAppendOperands(TzdLangParser::ExpressionContext *expr,
                                      const std::string &varName,
                                      std::vector<TzdLangParser::ExpressionContext *> &operands)
{
    if (auto addCtx = dynamic_cast<TzdLangParser::AdditiveExprContext *>(expr))
    {
        if (addCtx->PLUS())
        {
            if (collectSelfAppendOperands(addCtx->expression(0), varName, operands))
            {
                operands.push_back(addCtx->expression(1));
                return true;
            }
        }
    }
    return (expr->getText() == varName);
}

std::any TzdCompiler::visitAssignmentExpr(TzdLangParser::AssignmentExprContext *ctx)
{
    if (ctx->getStart() && !s_compilingNativeWorker && (s_loopLevel == 0 || jitTrackFrames()))
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }
    std::string opText = "=";
    if (ctx->PLUS_ASSIGN())
        opText = "+=";
    else if (ctx->MIN_ASSIGN())
        opText = "-=";
    else if (ctx->MUL_ASSIGN())
        opText = "*=";
    else if (ctx->DIV_ASSIGN())
        opText = "/=";
    else if (ctx->MOD_ASSIGN())
        opText = "%=";
    else if (ctx->AND_ASSIGN())
        opText = "&=";
    else if (ctx->OR_ASSIGN())
        opText = "|=";
    else if (ctx->XOR_ASSIGN())
        opText = "^=";
    else if (ctx->SHL_ASSIGN())
        opText = "<<=";
    else if (ctx->SHR_ASSIGN())
        opText = ">>=";
    else if (ctx->USHR_ASSIGN())
        opText = ">>>=";
    bool isCompound = (opText != "=");

    auto emitCompoundNative = [&](Value *oldVal, Value *nativeRhs) -> Value *
    {
        if (opText == "+=")
            return m_builder.CreateFAdd(oldVal, castToNativeDouble(nativeRhs));
        if (opText == "-=")
            return m_builder.CreateFSub(oldVal, castToNativeDouble(nativeRhs));
        if (opText == "*=")
            return m_builder.CreateFMul(oldVal, castToNativeDouble(nativeRhs));
        if (opText == "/=")
            return m_builder.CreateFDiv(oldVal, castToNativeDouble(nativeRhs));
        Value *i64L = castToNativeI64(oldVal);
        Value *i64R = castToNativeI64(nativeRhs);
        if (opText == "%=")
        {
            Value *rem = m_builder.CreateSRem(i64L, i64R, "rem_i64");
            return m_builder.CreateSIToFP(rem, m_doubleTy, "rem_d");
        }
        if (opText == "&=")
        {
            Value *res = m_builder.CreateAnd(i64L, i64R, "and_i64");
            return m_builder.CreateSIToFP(res, m_doubleTy, "and_d");
        }
        if (opText == "|=")
        {
            Value *res = m_builder.CreateOr(i64L, i64R, "or_i64");
            return m_builder.CreateSIToFP(res, m_doubleTy, "or_d");
        }
        if (opText == "^=")
        {
            Value *res = m_builder.CreateXor(i64L, i64R, "xor_i64");
            return m_builder.CreateSIToFP(res, m_doubleTy, "xor_d");
        }
        Value *shiftAmt = m_builder.CreateAnd(i64R, m_builder.getInt64(63), "shift_amt");
        if (opText == "<<=")
        {
            Value *res = m_builder.CreateShl(i64L, shiftAmt, "shl_i64");
            return m_builder.CreateSIToFP(res, m_doubleTy, "shl_d");
        }
        if (opText == ">>=")
        {
            Value *res = m_builder.CreateAShr(i64L, shiftAmt, "ashr_i64");
            return m_builder.CreateSIToFP(res, m_doubleTy, "ashr_d");
        }
        if (opText == ">>>=")
        {
            Value *res = m_builder.CreateLShr(i64L, shiftAmt, "lshr_i64");
            return m_builder.CreateUIToFP(res, m_doubleTy, "lshr_d");
        }
        return nativeRhs;
    };

    auto getCompoundRtFn = [&](const std::string &op) -> const char *
    {
        if (op == "+=")
            return "rt_op_add";
        if (op == "-=")
            return "rt_op_sub";
        if (op == "*=")
            return "rt_op_mul";
        if (op == "/=")
            return "rt_op_div";
        if (op == "%=")
            return "rt_op_mod";
        if (op == "&=")
            return "rt_op_bitand";
        if (op == "|=")
            return "rt_op_bitor";
        if (op == "^=")
            return "rt_op_bitxor";
        if (op == "<<=")
            return "rt_op_shl";
        if (op == ">>=")
            return "rt_op_shr";
        if (op == ">>>=")
            return "rt_op_ushr";
        return "rt_op_add";
    };

    std::string lhsName = ctx->expression(0)->getText();
    if (!isCompound && m_namedValues.count(lhsName))
    {
        std::vector<TzdLangParser::ExpressionContext *> appendOperands;
        if (collectSelfAppendOperands(ctx->expression(1), lhsName, appendOperands) && !appendOperands.empty())
        {
            Value *destPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues[lhsName]);
            for (auto *opCtx : appendOperands)
            {
                Value *opVal = castAnyToValue(visit(opCtx), "selfAppendOperand");
                if (opVal->getType()->isDoubleTy())
                {
                    destPtr = m_builder.CreateCall(getRtFunc("rt_str_append_num"), {destPtr, opVal});
                }
                else
                {
                    destPtr = m_builder.CreateCall(getRtFunc("rt_str_append"), {destPtr, boxToTzdValue(opVal)});
                }
            }
            m_builder.CreateStore(destPtr, m_namedValues[lhsName]);
            return std::any((Value *)destPtr);
        }
    }

    Value *rhs = castAnyToValue(visit(ctx->expression(1)), "visitAssignmentExpr.rhs");

    if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext *>(ctx->expression(0)))
    {
        std::string containerName = indexCtx->expression(0)->getText();
        Value *container = nullptr;
        auto hIt = m_hoistedArrays.find(containerName);
        if (hIt != m_hoistedArrays.end())
        {
            container = hIt->second.container;
        }
        else if (m_namedValues.count(containerName))
        {
            container = m_builder.CreateLoad(m_ptrTy, m_namedValues[containerName]);
        }
        else
        {
            container = std::any_cast<Value *>(visit(indexCtx->expression(0)));
            if (container->getType()->isDoubleTy())
            {
                container = boxToTzdValue(container);
            }
        }
        Value *index_raw = std::any_cast<Value *>(visit(indexCtx->expression(1)));

        Value *oldVal = nullptr;
        bool constIdx = false;
        int constIdxVal = 0;
        bool nativeIdx = false;

        if (auto *constFP = llvm::dyn_cast<llvm::ConstantFP>(index_raw))
        {
            constIdx = true;
            constIdxVal = (int)constFP->getValueAPF().convertToDouble();
            if (isCompound)
            {
                oldVal = m_builder.CreateCall(getRtFunc("rt_get_index_native_d"),
                                              {container, m_builder.getInt32(constIdxVal)});
            }
        }
        else if (index_raw->getType()->isDoubleTy())
        {
            nativeIdx = true;
            if (isCompound)
            {
                oldVal = m_builder.CreateCall(getRtFunc("rt_get_index_native_d_dyn"),
                                              {container, index_raw});
            }
        }
        else
        {
            Value *boxedIndex = boxToTzdValue(index_raw);
            if (!isCompound && rhs->getType()->isDoubleTy())
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_d"), {container, boxedIndex, rhs});
                return rhs;
            }
            Value *boxedRhs = boxToTzdValue(rhs);
            if (isCompound)
            {
                Value *oldBoxed = m_builder.CreateCall(getRtFunc("rt_get_index"), {container, boxedIndex});
                const char *opFn = getCompoundRtFn(opText);
                boxedRhs = m_builder.CreateCall(getRtFunc(opFn), {oldBoxed, boxedRhs});
            }
            m_builder.CreateCall(getRtFunc("rt_store_index"), {container, boxedIndex, boxedRhs});
            return boxedRhs;
        }

        if (rhs->getType()->isIntegerTy(1))
        {
            if (!isCompound && (constIdx || nativeIdx))
            {
                Value *fastBuf = nullptr;
                Value *fastLen = nullptr;
                Value *hasBuf = nullptr;

                auto hIt = m_hoistedArrays.find(containerName);
                if (hIt != m_hoistedArrays.end())
                {
                    fastBuf = hIt->second.fastBuf;
                    fastLen = hIt->second.fastLen;
                    hasBuf = hIt->second.hasBuf;
                }
                else
                {
                    fastBuf = m_builder.CreateLoad(m_ptrTy, container, "fastBuf");
                    Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, "fastLenPtr");
                    fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, "fastLen");
                    hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), "hasBuf");
                }

                std::string idxExprText = indexCtx->expression(1)->getText();
                Value *idxI64 = nullptr;
                auto sIt = m_shadowI64IndVars.find(idxExprText);
                if (sIt != m_shadowI64IndVars.end())
                {
                    idxI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), sIt->second, idxExprText + "_idx_i64");
                }
                else if (constIdx)
                {
                    idxI64 = m_builder.getInt64((uint64_t)constIdxVal);
                }
                else
                {
                    idxI64 = m_builder.CreateFPToSI(index_raw, m_builder.getInt64Ty(), "idx_i64");
                }

                if (hIt != m_hoistedArrays.end() && hIt->second.inFastLoop &&
                    (hIt->second.safeIndexVar.empty() || hIt->second.safeIndexVar == idxExprText))
                {
                    Value *dVal = nullptr;
                    if (auto *cInt = dyn_cast<ConstantInt>(rhs))
                    {
                        dVal = cInt->isOne() ? ConstantFP::get(m_doubleTy, 1.0) : ConstantFP::get(m_doubleTy, 0.0);
                    }
                    else
                    {
                        dVal = m_builder.CreateSelect(rhs, ConstantFP::get(m_doubleTy, 1.0), ConstantFP::get(m_doubleTy, 0.0), "boolValD");
                    }
                    Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
                    m_builder.CreateStore(dVal, slot);
                    return rhs;
                }

                Function *curFunc = m_builder.GetInsertBlock()->getParent();
                BasicBlock *fastStoreBB = BasicBlock::Create(m_context, "fast_arr_store_b", curFunc);
                BasicBlock *slowStoreBB = BasicBlock::Create(m_context, "slow_arr_store_b", curFunc);
                BasicBlock *mergeStoreBB = BasicBlock::Create(m_context, "merge_arr_store_b", curFunc);

                Value *canFast = nullptr;
                if (hIt != m_hoistedArrays.end() && !hIt->second.safeIndexVar.empty() &&
                    hIt->second.safeIndexVar == idxExprText && hIt->second.safeFastCond)
                {
                    canFast = hIt->second.safeFastCond;
                }
                else
                {
                    Value *inBounds = m_builder.CreateICmpULT(idxI64, fastLen, "inBounds");
                    canFast = m_builder.CreateAnd(hasBuf, inBounds, "canFast");
                }

                m_builder.CreateCondBr(canFast, fastStoreBB, slowStoreBB);

                m_builder.SetInsertPoint(fastStoreBB);
                Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
                Value *dVal = m_builder.CreateSelect(rhs, ConstantFP::get(m_doubleTy, 1.0), ConstantFP::get(m_doubleTy, 0.0), "boolValD");
                m_builder.CreateStore(dVal, slot);
                m_builder.CreateBr(mergeStoreBB);

                m_builder.SetInsertPoint(slowStoreBB);
                if (constIdx)
                {
                    m_builder.CreateCall(getRtFunc("rt_store_index_bool"),
                                         {container, ConstantFP::get(m_doubleTy, (double)constIdxVal), rhs});
                }
                else
                {
                    m_builder.CreateCall(getRtFunc("rt_store_index_bool"),
                                         {container, index_raw, rhs});
                }
                m_builder.CreateBr(mergeStoreBB);

                m_builder.SetInsertPoint(mergeStoreBB);
                return rhs;
            }
            if (constIdx)
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_bool"),
                                     {container, ConstantFP::get(m_doubleTy, (double)constIdxVal), rhs});
            }
            else if (nativeIdx)
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_bool"),
                                     {container, index_raw, rhs});
            }
            else
            {
                Value *boxedIndex = boxToTzdValue(index_raw);
                Value *boxedRhs = boxToTzdValue(rhs);
                m_builder.CreateCall(getRtFunc("rt_store_index"), {container, boxedIndex, boxedRhs});
            }
            return rhs;
        }

        if (rhs->getType()->isPointerTy())
        {
            if (isCompound)
            {
                Value *boxedIndex = boxToTzdValue(index_raw);
                Value *boxedRhs = boxToTzdValue(rhs);
                Value *oldBoxed = m_builder.CreateCall(getRtFunc("rt_get_index"), {container, boxedIndex});
                const char *opFn = getCompoundRtFn(opText);
                boxedRhs = m_builder.CreateCall(getRtFunc(opFn), {oldBoxed, boxedRhs});
                m_builder.CreateCall(getRtFunc("rt_store_index"), {container, boxedIndex, boxedRhs});
                return boxedRhs;
            }
            if (constIdx)
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_val_dyn"),
                                     {container, ConstantFP::get(m_doubleTy, (double)constIdxVal), rhs});
            }
            else if (nativeIdx)
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_val_dyn"),
                                     {container, index_raw, rhs});
            }
            else
            {
                Value *boxedIndex = boxToTzdValue(index_raw);
                m_builder.CreateCall(getRtFunc("rt_store_index"), {container, boxedIndex, rhs});
            }
            return rhs;
        }

        Value *nativeRhs = castToNativeDouble(rhs);
        Value *newVal = nativeRhs;
        if (isCompound)
        {
            newVal = emitCompoundNative(oldVal, nativeRhs);
        }

        if (!isCompound && (constIdx || nativeIdx))
        {
            Value *fastBuf = nullptr;
            Value *fastLen = nullptr;
            Value *hasBuf = nullptr;

            auto hIt = m_hoistedArrays.find(containerName);
            if (hIt != m_hoistedArrays.end())
            {
                fastBuf = hIt->second.fastBuf;
                fastLen = hIt->second.fastLen;
                hasBuf = hIt->second.hasBuf;
            }
            else
            {
                fastBuf = m_builder.CreateLoad(m_ptrTy, container, "fastBuf");
                Value *fastLenPtr = m_builder.CreateConstInBoundsGEP1_32(m_builder.getInt8Ty(), container, 8, "fastLenPtr");
                fastLen = m_builder.CreateLoad(m_builder.getInt64Ty(), fastLenPtr, "fastLen");
                hasBuf = m_builder.CreateICmpNE(fastBuf, ConstantPointerNull::get(cast<PointerType>(m_ptrTy)), "hasBuf");
            }

            std::string idxExprText = indexCtx->expression(1)->getText();
            Value *idxI64 = nullptr;
            auto sIt = m_shadowI64IndVars.find(idxExprText);
            if (sIt != m_shadowI64IndVars.end())
            {
                idxI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), sIt->second, idxExprText + "_idx_i64");
            }
            else if (constIdx)
            {
                idxI64 = m_builder.getInt64((uint64_t)constIdxVal);
            }
            else
            {
                idxI64 = m_builder.CreateFPToSI(index_raw, m_builder.getInt64Ty(), "idx_i64");
            }

            if (hIt != m_hoistedArrays.end() && (hIt->second.inFastLoop || s_compilingNativeWorker) &&
                (hIt->second.safeIndexVar.empty() || hIt->second.safeIndexVar == idxExprText || s_compilingNativeWorker))
            {
                Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
                m_builder.CreateStore(newVal, slot);
                return newVal;
            }

            Function *curFunc = m_builder.GetInsertBlock()->getParent();
            BasicBlock *fastStoreBB = BasicBlock::Create(m_context, "fast_arr_store_d", curFunc);
            BasicBlock *slowStoreBB = BasicBlock::Create(m_context, "slow_arr_store_d", curFunc);
            BasicBlock *mergeStoreBB = BasicBlock::Create(m_context, "merge_arr_store_d", curFunc);

            Value *canFast = nullptr;
            if (hIt != m_hoistedArrays.end() && !hIt->second.safeIndexVar.empty() &&
                hIt->second.safeIndexVar == idxExprText && hIt->second.safeFastCond)
            {
                canFast = hIt->second.safeFastCond;
            }
            else
            {
                Value *inBounds = m_builder.CreateICmpULT(idxI64, fastLen, "inBounds");
                canFast = (hIt != m_hoistedArrays.end() && hIt->second.inFastLoop) ? inBounds : m_builder.CreateAnd(hasBuf, inBounds, "canFast");
            }

            m_builder.CreateCondBr(canFast, fastStoreBB, slowStoreBB);

            m_builder.SetInsertPoint(fastStoreBB);
            Value *slot = m_builder.CreateGEP(m_doubleTy, fastBuf, idxI64, "elemSlot");
            m_builder.CreateStore(newVal, slot);
            m_builder.CreateBr(mergeStoreBB);

            m_builder.SetInsertPoint(slowStoreBB);
            if (constIdx)
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_native_d"),
                                     {container, m_builder.getInt32(constIdxVal), newVal});
            }
            else
            {
                m_builder.CreateCall(getRtFunc("rt_store_index_native_d_dyn"),
                                     {container, index_raw, newVal});
            }
            m_builder.CreateBr(mergeStoreBB);

            m_builder.SetInsertPoint(mergeStoreBB);
            return newVal;
        }

        if (constIdx)
        {
            m_builder.CreateCall(getRtFunc("rt_store_index_native_d"),
                                 {container, m_builder.getInt32(constIdxVal), newVal});
        }
        else
        {
            m_builder.CreateCall(getRtFunc("rt_store_index_native_d_dyn"),
                                 {container, index_raw, newVal});
        }
        return newVal;
    }

    TzdLangParser::MemberAccessExprContext *memberCtx =
        dynamic_cast<TzdLangParser::MemberAccessExprContext *>(ctx->expression(0));
    if (!memberCtx)
    {
        if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(ctx->expression(0)))
        {
            memberCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(atomExpr->atom());
        }
    }
    if (memberCtx)
    {
        std::string objName = memberCtx->atom()->getText();
        std::string memberName = memberCtx->IDENTIFIER()->getText();

        // SROA slot store: supports direct (p.x = v) and nested (seg.p1.x = v, this.p1.x = v)
        std::string rootVar = objName;
        std::string fieldKey = memberName;
        size_t firstDot = objName.find('.');
        if (firstDot != std::string::npos)
        {
            rootVar = objName.substr(0, firstDot);
            fieldKey = objName.substr(firstDot + 1) + "." + memberName;
        }

        auto fIt = m_varFieldAllocas.find(rootVar);
        if (fIt != m_varFieldAllocas.end())
        {
            auto slotIt = fIt->second.find(fieldKey);
            if (slotIt != fIt->second.end() && slotIt->second)
            {
                Value *nativeRhs = castToNativeDouble(rhs);
                if (isCompound)
                {
                    Value *oldVal = m_builder.CreateLoad(m_doubleTy, slotIt->second, rootVar + "_" + fieldKey);
                    nativeRhs = emitCompoundNative(oldVal, nativeRhs);
                }
                m_builder.CreateStore(nativeRhs, slotIt->second);
                return nativeRhs;
            }
        }

        Value *obj = nullptr;
        if (m_namedValues.count(objName))
        {
            obj = m_builder.CreateLoad(m_ptrTy, m_namedValues[objName]);
        }
        else
        {
            obj = castAnyToValue(visit(memberCtx->atom()), "visitAssignmentExpr.memberObj");
            if (obj->getType()->isDoubleTy())
            {
                obj = boxToTzdValue(obj);
            }
        }

        std::string targetClass = "";
        if (objName == "this")
        {
            if (m_varClassTypes.count("this"))
                targetClass = m_varClassTypes["this"];
            else if (m_currentClassDef)
                targetClass = m_currentClassDef->simpleName;
        }
        else if (m_varClassTypes.count(objName))
        {
            targetClass = m_varClassTypes[objName];
        }
        if (!targetClass.empty())
        {
            TzdClassDef *cls = TzdOopManager::getClass(targetClass);
            if (cls)
            {
                auto it = cls->fieldNameToIndex.find(memberName);
                if (it != cls->fieldNameToIndex.end())
                {
                    ClassField *f = cls->findField(memberName);
                    bool isFloat = f && (f->type == "float" || f->type == "double" || (f->type.empty() && rhs && rhs->getType()->isDoubleTy()));
                    bool isInt = f && (f->type == "int" || f->type == "i64" || f->type == "long");
                    if (isFloat)
                    {
                        Value *nativeRhs = castToNativeDouble(rhs);
                        if (isCompound)
                        {
                            Value *oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_field_d"), {obj, m_builder.getInt32(it->second)});
                            nativeRhs = emitCompoundNative(oldVal, nativeRhs);
                        }
                        m_builder.CreateCall(getRtFunc("rt_tzd_set_field_d"), {obj, m_builder.getInt32(it->second), nativeRhs});
                        return nativeRhs;
                    }
                    else if (isInt)
                    {
                        Value *nativeRhs = castToNativeI64(rhs);
                        if (isCompound)
                        {
                            Value *oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_field_i64"), {obj, m_builder.getInt32(it->second)});
                            if (opText == "+=")
                                nativeRhs = m_builder.CreateAdd(oldVal, nativeRhs);
                            else if (opText == "-=")
                                nativeRhs = m_builder.CreateSub(oldVal, nativeRhs);
                            else if (opText == "*=")
                                nativeRhs = m_builder.CreateMul(oldVal, nativeRhs);
                            else if (opText == "/=")
                                nativeRhs = m_builder.CreateSDiv(oldVal, nativeRhs);
                            else if (opText == "%=")
                                nativeRhs = m_builder.CreateSRem(oldVal, nativeRhs);
                            else if (opText == "&=")
                                nativeRhs = m_builder.CreateAnd(oldVal, nativeRhs);
                            else if (opText == "|=")
                                nativeRhs = m_builder.CreateOr(oldVal, nativeRhs);
                            else if (opText == "^=")
                                nativeRhs = m_builder.CreateXor(oldVal, nativeRhs);
                            else if (opText == "<<=")
                                nativeRhs = m_builder.CreateShl(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                            else if (opText == ">>=")
                                nativeRhs = m_builder.CreateAShr(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                            else if (opText == ">>>=")
                                nativeRhs = m_builder.CreateLShr(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                        }
                        m_builder.CreateCall(getRtFunc("rt_tzd_set_field_i64"), {obj, m_builder.getInt32(it->second), nativeRhs});
                        return nativeRhs;
                    }
                    else
                    {
                        Value *boxedRhs = boxToTzdValue(rhs);
                        if (isCompound)
                        {
                            Value *fieldPtr = m_builder.CreateCall(getRtFunc("rt_tzd_get_field_val"), {obj, m_builder.getInt32(it->second)});
                            const char *opFn = getCompoundRtFn(opText);
                            boxedRhs = m_builder.CreateCall(getRtFunc(opFn), {fieldPtr, boxedRhs});
                        }
                        m_builder.CreateCall(getRtFunc("rt_tzd_set_field_val"), {obj, m_builder.getInt32(it->second), boxedRhs});
                        return boxedRhs;
                    }
                }
            }
        }

        Value *nameStr = m_builder.CreateGlobalStringPtr(memberName);
        TzdSelector sel = internSelectorConstant(memberName);
        Value *boxedRhs = boxToTzdValue(rhs);
        Value *fieldPtr = m_builder.CreateCall(getRtFunc("rt_tzd_get_member"),
                                               {obj, m_builder.getInt32((int32_t)sel), nameStr});
        if (isCompound)
        {
            const char *opFn = getCompoundRtFn(opText);
            boxedRhs = m_builder.CreateCall(getRtFunc(opFn), {fieldPtr, boxedRhs});
        }
        m_builder.CreateCall(getRtFunc("rt_store_field_ptr"), {fieldPtr, boxedRhs});
        return boxedRhs;
    }

    std::string name = ctx->expression(0)->getText();

    if (!m_lastNewFieldAllocas.empty())
    {
        m_varFieldAllocas[name] = m_lastNewFieldAllocas;
        m_lastNewFieldAllocas.clear();
        std::string newClass = deduceExprClassName(ctx->expression(1), m_varClassTypes, m_currentClassDef);
        if (!newClass.empty())
        {
            m_varClassTypes[name] = newClass;
        }
        if (s_compilingNativeWorker)
        {
            return rhs;
        }
    }

    std::string newClass = deduceExprClassName(ctx->expression(1), m_varClassTypes, m_currentClassDef);
    if (!newClass.empty())
    {
        m_varClassTypes[name] = newClass;
    }

    std::string expectedType = "";
    if (m_varDeclaredTypes.count(name))
    {
        expectedType = m_varDeclaredTypes[name];
    }
    else if (g_CurrentInterpreter)
    {
        for (auto it = g_CurrentInterpreter->scopes.rbegin(); it != g_CurrentInterpreter->scopes.rend(); ++it)
        {
            auto fit = it->find(name);
            if (fit != it->end() && !fit->second.declaredType.empty())
            {
                expectedType = fit->second.declaredType;
                break;
            }
        }
    }
    if (!expectedType.empty() && expectedType != "var" && expectedType != "any" && expectedType != "auto")
    {
        Value *boxedRhs = boxToTzdValue(rhs);
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        Value *typeStr = m_builder.CreateGlobalStringPtr(expectedType);
        Value *ok = m_builder.CreateCall(getRtFunc("rt_tzd_check_var_type"), {nameStr, typeStr, boxedRhs});

        BasicBlock *contBB = BasicBlock::Create(m_context, "assign_type_ok", m_builder.GetInsertBlock()->getParent());
        BasicBlock *errBB = BasicBlock::Create(m_context, "assign_type_err", m_builder.GetInsertBlock()->getParent());
        m_builder.CreateCondBr(ok, contBB, errBB);

        m_builder.SetInsertPoint(errBB);
        if (s_currentWorkerFunc && s_currentWorkerFunc->arg_size() > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {s_currentWorkerFunc->getArg(0)});
        }
        if (m_builder.GetInsertBlock()->getParent()->getReturnType()->isDoubleTy())
            m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
        else if (m_builder.GetInsertBlock()->getParent()->getReturnType()->isVoidTy())
            m_builder.CreateRetVoid();
        else
            m_builder.CreateRet(Constant::getNullValue(m_builder.GetInsertBlock()->getParent()->getReturnType()));

        m_builder.SetInsertPoint(contBB);
    }

    if (m_nativeDoubleLocals.count(name))
    {
        Value *newVal = nullptr;
        if (isCompound)
        {
            Value *oldVal = m_builder.CreateLoad(m_doubleTy, m_nativeDoubleLocals[name]);
            newVal = emitCompoundNative(oldVal, rhs);
        }
        else
        {
            newVal = castToNativeDouble(rhs);
        }
        m_builder.CreateStore(newVal, m_nativeDoubleLocals[name]);
        if (!m_declaredLocals.count(name) && (s_loopLevel == 0 || jitTrackFrames()))
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(name);
            Value *boxedVal = boxToTzdValue(newVal);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedVal});
        }
        return newVal;
    }

    if (!m_namedValues.count(name) && m_namedValues.count("this"))
    {
        auto fIt = m_varFieldAllocas.find("this");
        if (fIt != m_varFieldAllocas.end())
        {
            auto slotIt = fIt->second.find(name);
            if (slotIt != fIt->second.end() && slotIt->second)
            {
                Value *nativeRhs = castToNativeDouble(rhs);
                if (isCompound)
                {
                    Value *oldVal = m_builder.CreateLoad(m_doubleTy, slotIt->second, "this_" + name);
                    nativeRhs = emitCompoundNative(oldVal, nativeRhs);
                }
                m_builder.CreateStore(nativeRhs, slotIt->second);
                return nativeRhs;
            }
        }

        // Fast path for real instance field
        if (m_currentClassDef)
        {
            auto it = m_currentClassDef->fieldNameToIndex.find(name);
            if (it != m_currentClassDef->fieldNameToIndex.end())
            {
                Value *thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);
                ClassField *f = m_currentClassDef->findField(name);
                bool isFloat = f && (f->type == "float" || f->type == "double" || (f->type.empty() && rhs && rhs->getType()->isDoubleTy()));
                bool isInt = f && (f->type == "int" || f->type == "i64" || f->type == "long");
                if (isFloat)
                {
                    Value *nativeRhs = castToNativeDouble(rhs);
                    if (isCompound)
                    {
                        Value *oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_field_d"), {thisPtr, m_builder.getInt32(it->second)});
                        nativeRhs = emitCompoundNative(oldVal, nativeRhs);
                    }
                    m_builder.CreateCall(getRtFunc("rt_tzd_set_field_d"), {thisPtr, m_builder.getInt32(it->second), nativeRhs});
                    return nativeRhs;
                }
                else if (isInt)
                {
                    Value *nativeRhs = castToNativeI64(rhs);
                    if (isCompound)
                    {
                        Value *oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_field_i64"), {thisPtr, m_builder.getInt32(it->second)});
                        if (opText == "+=")
                            nativeRhs = m_builder.CreateAdd(oldVal, nativeRhs);
                        else if (opText == "-=")
                            nativeRhs = m_builder.CreateSub(oldVal, nativeRhs);
                        else if (opText == "*=")
                            nativeRhs = m_builder.CreateMul(oldVal, nativeRhs);
                        else if (opText == "/=")
                            nativeRhs = m_builder.CreateSDiv(oldVal, nativeRhs);
                        else if (opText == "%=")
                            nativeRhs = m_builder.CreateSRem(oldVal, nativeRhs);
                        else if (opText == "&=")
                            nativeRhs = m_builder.CreateAnd(oldVal, nativeRhs);
                        else if (opText == "|=")
                            nativeRhs = m_builder.CreateOr(oldVal, nativeRhs);
                        else if (opText == "^=")
                            nativeRhs = m_builder.CreateXor(oldVal, nativeRhs);
                        else if (opText == "<<=")
                            nativeRhs = m_builder.CreateShl(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                        else if (opText == ">>=")
                            nativeRhs = m_builder.CreateAShr(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                        else if (opText == ">>>=")
                            nativeRhs = m_builder.CreateLShr(oldVal, m_builder.CreateAnd(nativeRhs, m_builder.getInt64(63)));
                    }
                    m_builder.CreateCall(getRtFunc("rt_tzd_set_field_i64"), {thisPtr, m_builder.getInt32(it->second), nativeRhs});
                    return nativeRhs;
                }
            }
        }
    }

    if ((rhs->getType()->isDoubleTy() || rhs->getType()->isIntegerTy()) &&
        !m_namedValues.count(name) &&
        !m_namedValues.count("this"))
    {
        Value *dVal = castToNativeDouble(rhs);
        AllocaInst *alloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, name + "_native");
        m_builder.CreateStore(dVal, alloc);
        m_nativeDoubleLocals[name] = alloc;
        if (!m_declaredLocals.count(name))
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(name);
            Value *boxedRhs = boxToTzdValue(dVal);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedRhs});
        }
        return dVal;
    }

    // ---- boxed local 变量 ----
    Value *boxedRhs = boxToTzdValue(rhs);
    if (isCompound)
    {
        if (opText == "+=")
        {
            Value *oldVal = nullptr;
            if (m_namedValues.count(name))
            {
                oldVal = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
            }
            else
            {
                Value *nameStr = m_builder.CreateGlobalStringPtr(name);
                int line = ctx->getStart() ? (int)ctx->getStart()->getLine() : 0;
                int col = ctx->getStart() ? (int)ctx->getStart()->getCharPositionInLine() : 0;
                oldVal = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr,
                                                                            m_builder.getInt32(line),
                                                                            m_builder.getInt32(col)});
            }
            if (rhs->getType()->isDoubleTy())
            {
                boxedRhs = m_builder.CreateCall(getRtFunc("rt_str_append_num"), {oldVal, rhs});
            }
            else
            {
                boxedRhs = m_builder.CreateCall(getRtFunc("rt_str_append"), {oldVal, boxedRhs});
            }
        }
        else
        {
            const char *opFn = getCompoundRtFn(opText);
            Value *oldVal = nullptr;
            if (m_namedValues.count(name))
            {
                oldVal = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
            }
            else
            {
                Value *nameStr = m_builder.CreateGlobalStringPtr(name);
                int line = ctx->getStart() ? (int)ctx->getStart()->getLine() : 0;
                int col = ctx->getStart() ? (int)ctx->getStart()->getCharPositionInLine() : 0;
                oldVal = m_builder.CreateCall(getRtFunc("rt_get_var_ptr"), {nameStr,
                                                                            m_builder.getInt32(line),
                                                                            m_builder.getInt32(col)});
            }
            boxedRhs = m_builder.CreateCall(getRtFunc(opFn), {oldVal, boxedRhs});
        }
    }

    if (m_namedValues.count(name))
    {
        // 【核心修复：复用已分配的稳定指针，原地修改】
        Value *destPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        m_builder.CreateCall(getRtFunc("rt_copy_value"), {destPtr, boxedRhs});
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, destPtr});
    }
    else if (m_namedValues.count("this"))
    {
        Value *thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        TzdSelector sel = internSelectorConstant(name);
        m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
                             {thisPtr, m_builder.getInt32((int32_t)sel), nameStr, boxedRhs});
    }
    else
    {
        // 【全局变量赋值兜底】
        AllocaInst *alloca = CreateEntryBlockAlloca(m_ptrTy, nullptr, name + "_local");
        m_namedValues[name] = alloca;

        BasicBlock *curBB = m_builder.GetInsertBlock();
        Function *func = curBB->getParent();
        BasicBlock &entryBB = func->getEntryBlock();
        Instruction *insertPt = &entryBB.front();
        while (isa<AllocaInst>(insertPt))
            insertPt = insertPt->getNextNode();

        m_builder.SetInsertPoint(insertPt);
        Value *nullVal = m_builder.CreateCall(getRtFunc("rt_create_null"));
        Value *stableVal = m_builder.CreateCall(getRtFunc("rt_stabilize_value"), {nullVal});
        m_builder.CreateStore(stableVal, alloca);
        m_builder.SetInsertPoint(curBB);

        Value *destPtr = m_builder.CreateLoad(m_ptrTy, alloca);
        m_builder.CreateCall(getRtFunc("rt_copy_value"), {destPtr, boxedRhs});

        if (!m_declaredLocals.count(name))
        {
            Value *nameStr = m_builder.CreateGlobalStringPtr(name);
            m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, destPtr});
        }
    }
    return boxedRhs;
}
std::any TzdCompiler::visitPrintFunExpr(TzdLangParser::PrintFunExprContext *ctx)
{
    if (ctx->printFunction()->exprList())
    {
        for (auto e : ctx->printFunction()->exprList()->expression())
        {
            Value *v = std::any_cast<Value *>(visit(e));

            Value *boxedVal = boxToTzdValue(v);

            m_builder.CreateCall(getRtFunc("rt_print"), {boxedVal});
        }
    }
    m_builder.CreateCall(getRtFunc("rt_print_newline"));
    return std::any();
}

std::any TzdCompiler::visitNewExpr(TzdLangParser::NewExprContext *ctx)
{
    if (ctx->getStart() && !s_compilingNativeWorker && (s_loopLevel == 0 || jitTrackFrames()))
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }
    std::string className = ctx->qualifiedName()->getText();
    Value *name = m_builder.CreateGlobalStringPtr(className);

    auto exprs = ctx->exprList() ? ctx->exprList()->expression() : std::vector<TzdLangParser::ExpressionContext *>();
    int argCount = (int)exprs.size();

    // General SROA (Scalar Replacement of Aggregates) for arbitrary N fields and nested objects
    TzdClassDef *def = TzdOopManager::getClass(className);
    bool isNumericAggregate = (def != nullptr && !def->fields.empty());
    if (def)
    {
        for (const auto &[fName, fld] : def->fields)
        {
            if (fld.type != "float" && fld.type != "double" && fld.type != "int" &&
                fld.type != "long" && fld.type != "i64" && fld.type != "i32" &&
                fld.type != "short" && fld.type != "byte")
            {
                isNumericAggregate = false;
                break;
            }
        }
    }
    std::vector<std::string> fieldNames;
    if (def && isNumericAggregate)
    {
        fieldNames = resolveConstructorFields(def, argCount);
        if (fieldNames.empty() && (argCount > 0 || !def->constructors.empty()))
        {
            isNumericAggregate = false;
        }
    }
    if (def && isNumericAggregate && (argCount > 0 || !def->fieldIndices.empty()))
    {
        std::unordered_map<std::string, AllocaInst *> newSlots;
        std::vector<Value *> evaluatedDoubles;

        bool allArgsAreDoubles = true;
        for (int i = 0; i < argCount; ++i)
        {
            std::string fieldName = (i < (int)fieldNames.size()) ? fieldNames[i] : ("field_" + std::to_string(i));
            std::string argText = exprs[i]->getText();

            // 1. Check if argument is an existing SROA aggregate variable (nested object composition)
            auto varIt = m_varFieldAllocas.find(argText);
            if (varIt != m_varFieldAllocas.end() && !varIt->second.empty())
            {
                allArgsAreDoubles = false;
                for (const auto &pair : varIt->second)
                {
                    newSlots[fieldName + "." + pair.first] = pair.second;
                }
                evaluatedDoubles.push_back(ConstantFP::get(m_doubleTy, 0.0));
                continue;
            }

            // 2. Evaluate argument expression
            m_lastNewFieldAllocas.clear();
            Value *val = castAnyToValue(visit(exprs[i]), "new_arg");

            // 3. Check if evaluating argument produced a newly created SROA aggregate (inline nested `new Sub(...)`)
            if (!m_lastNewFieldAllocas.empty())
            {
                allArgsAreDoubles = false;
                for (const auto &pair : m_lastNewFieldAllocas)
                {
                    newSlots[fieldName + "." + pair.first] = pair.second;
                }
                evaluatedDoubles.push_back(ConstantFP::get(m_doubleTy, 0.0));
                m_lastNewFieldAllocas.clear();
                continue;
            }

            // 4. Scalar numeric argument: allocate stack slot and store native double
            Value *dVal = castToNativeDouble(val);
            AllocaInst *slot = CreateEntryBlockAlloca(m_doubleTy, nullptr, className + "_" + fieldName + "_slot");
            m_builder.CreateStore(dVal, slot);
            newSlots[fieldName] = slot;
            evaluatedDoubles.push_back(dVal);
        }

        // Initialize any remaining fields from class default numeric values
        for (const auto &fPair : def->fieldNameToIndex)
        {
            const std::string &fName = fPair.first;
            int fIdx = fPair.second;
            if (newSlots.count(fName) == 0)
            {
                double d = 0.0;
                if (fIdx >= 0 && fIdx < (int)def->defaultFieldValues.size())
                {
                    const auto &defVal = def->defaultFieldValues[fIdx];
                    if (defVal.type == TzdValue::DOUBLE || defVal.type == TzdValue::FLOAT)
                        d = defVal.dVal;
                    else if (defVal.type == TzdValue::INT || defVal.type == TzdValue::LONG)
                        d = (double)defVal.lVal;
                }
                AllocaInst *slot = CreateEntryBlockAlloca(m_doubleTy, nullptr, className + "_" + fName + "_slot");
                m_builder.CreateStore(ConstantFP::get(m_doubleTy, d), slot);
                newSlots[fName] = slot;
            }
        }

        m_lastNewFieldAllocas = newSlots;

        if (s_compilingNativeWorker)
        {
            // Inside specialized native worker: pure stack scalar replacement without heap allocation!
            return (Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        }

        // Outside native worker (or escaped to interpreter): construct instance fast if all fields are numeric
        if (allArgsAreDoubles && argCount > 0)
        {
            Value *valsArray = CreateEntryBlockAlloca(m_doubleTy, m_builder.getInt32(argCount), "sroa_vals");
            for (int i = 0; i < argCount; ++i)
            {
                Value *ptr = m_builder.CreateGEP(m_doubleTy, valsArray, m_builder.getInt32(i));
                m_builder.CreateStore(evaluatedDoubles[i], ptr);
            }
            return (Value *)m_builder.CreateCall(getRtFunc("rt_create_inst_sroa"), {name, m_builder.getInt32(argCount), valsArray});
        }
    }

    int slotCount = argCount > 0 ? argCount : 1;
    Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(slotCount), "ctor_args");
    m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(slotCount)});

    for (int i = 0; i < argCount; ++i)
    {
        Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
        Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
        if (argRaw->getType()->isDoubleTy())
        {
            inlineStoreNativeToPtr(argPtr, argRaw);
        }
        else
        {
            m_builder.CreateCall(getRtFunc("rt_copy_value"), {argPtr, boxToTzdValue(argRaw)});
        }
    }

    Value *instRes = (Value *)m_builder.CreateCall(getRtFunc("rt_create_inst_args"), {name, m_builder.getInt32(argCount), argsArray});
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
    }
    return instRes;
}

// 编译期内联 selector：成员名 -> 常量 TzdSelector（i32）写进 IR，
// 消除运行期字符串查找。m_selectorIds 仅缓存本编译器实例已解析映射，
// 进程级唯一性由 tzdInternSelector 保证。
TzdSelector TzdCompiler::internSelectorConstant(const std::string &name)
{
    auto it = m_selectorIds.find(name);
    if (it != m_selectorIds.end())
        return it->second;
    TzdSelector sel = tzdInternSelector(name);
    m_selectorIds.emplace(name, sel);
    return sel;
}

std::any TzdCompiler::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext *ctx)
{
    if (ctx->getStart() && !s_compilingNativeWorker && (s_loopLevel == 0 || jitTrackFrames()))
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }
    std::string objName = ctx->atom()->getText();
    std::string memberName = ctx->IDENTIFIER()->getText();

    // SROA slot lookup: supports direct (p.x) and nested (seg.p1.x, this.p1.x)
    std::string rootObj = objName;
    std::string fieldKey = memberName;
    size_t firstDot = objName.find('.');
    if (firstDot != std::string::npos)
    {
        rootObj = objName.substr(0, firstDot);
        fieldKey = objName.substr(firstDot + 1) + "." + memberName;
    }

    auto fIt = m_varFieldAllocas.find(rootObj);
    if (fIt != m_varFieldAllocas.end())
    {
        // 1. Direct scalar field match (e.g. "x" or "p1.x")
        auto it = fIt->second.find(fieldKey);
        if (it != fIt->second.end() && it->second)
        {
            return (Value *)m_builder.CreateLoad(m_doubleTy, it->second, rootObj + "_" + fieldKey);
        }

        // 2. Sub-aggregate extraction: e.g. `var p = seg.p1;` where `p1` has fields `p1.x`, `p1.y`
        m_lastNewFieldAllocas.clear();
        std::string prefix = fieldKey + ".";
        for (const auto &pair : fIt->second)
        {
            if (pair.first.rfind(prefix, 0) == 0)
            {
                m_lastNewFieldAllocas[pair.first.substr(prefix.size())] = pair.second;
            }
        }
        if (!m_lastNewFieldAllocas.empty())
        {
            return (Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        }
    }

    Value *obj = nullptr;
    if (m_namedValues.count(objName))
    {
        obj = m_builder.CreateLoad(m_ptrTy, m_namedValues[objName]);
    }
    else
    {
        obj = std::any_cast<Value *>(visit(ctx->atom()));
        if (obj->getType()->isDoubleTy())
        {
            obj = boxToTzdValue(obj);
        }
    }

    // Fast-path: direct field read when receiver class and field index are known
    std::string targetClass = "";
    if (objName == "this")
    {
        if (m_varClassTypes.count("this"))
            targetClass = m_varClassTypes["this"];
        else if (m_currentClassDef)
            targetClass = m_currentClassDef->simpleName;
    }
    else if (m_varClassTypes.count(objName))
    {
        targetClass = m_varClassTypes[objName];
    }
    if (!targetClass.empty())
    {
        TzdClassDef *cls = TzdOopManager::getClass(targetClass);
        if (cls)
        {
            auto it = cls->fieldNameToIndex.find(memberName);
            if (it != cls->fieldNameToIndex.end())
            {
                ClassField *f = cls->findField(memberName);
                bool isFloat = f && (f->type == "float" || f->type == "double");
                bool isInt = f && (f->type == "int" || f->type == "i64" || f->type == "long");
                if (isFloat)
                {
                    return (Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_field_d"),
                                                         {obj, m_builder.getInt32(it->second)});
                }
                else if (isInt)
                {
                    return (Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_field_i64"),
                                                         {obj, m_builder.getInt32(it->second)});
                }
                else
                {
                    return (Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_field_val"),
                                                         {obj, m_builder.getInt32(it->second)});
                }
            }
        }
    }

    Value *name = m_builder.CreateGlobalStringPtr(memberName);
    TzdSelector sel = internSelectorConstant(memberName);
    GlobalVariable *ic = new GlobalVariable(*m_module, m_builder.getInt64Ty(), false,
                                            GlobalValue::InternalLinkage, m_builder.getInt64(0), "ic");
    return (Value *)m_builder.CreateCall(getRtFunc("rt_tzd_get_member_ic"),
                                         {obj, m_builder.getInt32((int32_t)sel), name, ic});
}

std::any TzdCompiler::visitReturnStmt(TzdLangParser::ReturnStmtContext *ctx)
{
    Value *retValRaw = nullptr;
    if (ctx->expression())
    {
        bool isCall = (getAsCallExpr(ctx->expression()) != nullptr) && s_inlineReturnStack.empty();
        bool oldTailState = s_inTailPosition;
        s_inTailPosition = isCall;

        retValRaw = std::any_cast<Value *>(visit(ctx->expression()));

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
    if (m_builder.GetInsertBlock() && m_builder.GetInsertBlock()->getTerminator())
    {
        BasicBlock *deadAfterTail = BasicBlock::Create(
            m_context, "dead_after_tailcall",
            m_builder.GetInsertBlock()->getParent());
        m_builder.SetInsertPoint(deadAfterTail);
        return std::any();
    }

    // ======= 强制展开回收动作 (防止 try 块内 return 导致 jmp_buf 泄露) =======
    for (auto it = s_tryJmpBufStack.rbegin(); it != s_tryJmpBufStack.rend(); ++it)
    {
        m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {std::get<1>(*it)});
        m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), {std::get<0>(*it)});
    }
    // =========================================================================

    // 若当前处于 AST 内联展开上下文中，直接将返回值写回内联目标插槽并跳转至内联汇合块
    if (!s_inlineReturnStack.empty())
    {
        auto &frame = s_inlineReturnStack.back();
        if (retValRaw)
        {
            if (frame.expectsDouble)
            {
                m_builder.CreateStore(castToNativeDouble(retValRaw), frame.retNativeDouble);
            }
            else
            {
                Value *boxed = boxToTzdValue(retValRaw);
                m_builder.CreateStore(boxed, frame.retBoxed);
            }
        }
        m_builder.CreateBr(frame.returnBB);
        Function *curF = m_builder.GetInsertBlock()->getParent();
        BasicBlock *deadBB = BasicBlock::Create(m_context, "after_inl_ret", curF);
        m_builder.SetInsertPoint(deadBB);
        return std::any((Value *)nullptr);
    }

    if (m_currentRetPtr && !isa<ConstantPointerNull>(m_currentRetPtr))
    {
        if (retValRaw && retValRaw->getType()->isDoubleTy())
        {
            m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), {m_currentRetPtr, retValRaw});
        }
        else if (retValRaw && retValRaw->getType()->isIntegerTy())
        {
            Value *d = castToNativeDouble(retValRaw);
            m_builder.CreateCall(getRtFunc("rt_store_native_to_ptr"), {m_currentRetPtr, d});
        }
        else if (retValRaw)
        {
            Value *boxed = boxToTzdValue(retValRaw);
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, boxed});
        }
    }

    Function *currentFunc = m_builder.GetInsertBlock()->getParent();
    std::string currentName = currentFunc->getName().str();

    if (!s_compilingNativeWorker && currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
    {
        Value *interp = currentFunc->getArg(0);
        m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {interp});
    }

    // [绝杀修改]：如果是 Worker 函数，直接用底层寄存器返回原生数字
    if (currentFunc->getReturnType()->isDoubleTy())
    {
        Value *dRet = castToNativeDouble(retValRaw);
        m_builder.CreateRet(dRet);
    }
    else
    {
        m_builder.CreateRetVoid();
    }

    BasicBlock *deadBB = BasicBlock::Create(m_context, "unreachable", currentFunc);
    m_builder.SetInsertPoint(deadBB);

    return std::any((Value *)nullptr);
}

std::any TzdCompiler::visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext *ctx)
{
    auto decl = ctx->nativeFunctionDeclaration();
    std::string funcName = decl->IDENTIFIER()->getText();

    std::unordered_map<std::string, std::string> attrs;
    if (decl->nativeAttrList())
    {
        for (auto *attr : decl->nativeAttrList()->nativeAttr())
        {
            std::string key = attr->children[0]->getText();
            std::string val = "";
            if (attr->children.size() >= 3)
            {
                val = attr->children[2]->getText();
                if (val.size() >= 2 && (val.front() == '"' || val.front() == '\''))
                {
                    val = val.substr(1, val.size() - 2);
                }
            }
            attrs[key] = val;
        }
    }

    std::string dllPath = attrs["dll"];
    std::string realFuncName = attrs.count("fun") ? attrs["fun"] : funcName;
    void *procAddr = nullptr;

#ifdef _WIN32
    HMODULE hLib = LoadLibraryA(dllPath.c_str());
    if (hLib)
    {
        procAddr = (void *)GetProcAddress(hLib, realFuncName.c_str());
    }
#else
    void *hLib = dlopen(dllPath.c_str(), RTLD_LAZY);
    if (hLib)
    {
        procAddr = dlsym(hLib, realFuncName.c_str());
    }
#endif

    if (!procAddr)
    {
        std::cerr << "JIT Compile Error: Cannot find native function " << realFuncName
                  << " in " << dllPath << std::endl;
        return std::any();
    }

    Value *nameConst = m_builder.CreateGlobalStringPtr(funcName);

    Value *addrInt = ConstantInt::get(Type::getInt64Ty(m_context), (uintptr_t)procAddr);
    Value *addrPtr = m_builder.CreateIntToPtr(addrInt, m_ptrTy);

    Value *nativeFuncObj = m_builder.CreateCall(
        getRtFunc("rt_create_native_val"),
        {nameConst, addrPtr});

    AllocaInst *alloc = CreateEntryBlockAlloca(funcName);
    m_builder.CreateStore(nativeFuncObj, alloc);
    m_namedValues[funcName] = alloc;

    return std::any();
}
std::any TzdCompiler::visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext *ctx)
{
    return visit(ctx->classDeclaration());
}
std::any TzdCompiler::visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext *ctx)
{
    auto exprs = ctx->exprList() ? ctx->exprList()->expression()
                                 : std::vector<TzdLangParser::ExpressionContext *>();

    // 检测是否全为数值字面量 → 走 native double 数组路径
    bool allNumLit = !exprs.empty();
    for (auto e : exprs)
    {
        if (!dynamic_cast<TzdLangParser::IntExprContext *>(e) &&
            !dynamic_cast<TzdLangParser::FloatExprContext *>(e))
        {
            allNumLit = false;
            break;
        }
    }

    if (allNumLit)
    {
        int count = (int)exprs.size();
        Value *tmpBuf = CreateEntryBlockAlloca(m_doubleTy, m_builder.getInt32(count), "nat_arr_tmp");
        for (int i = 0; i < count; i++)
        {
            double v = std::stod(exprs[i]->getText());
            Value *slot = m_builder.CreateGEP(m_doubleTy, tmpBuf, m_builder.getInt32(i));
            m_builder.CreateStore(ConstantFP::get(m_doubleTy, v), slot);
        }
        return (Value *)m_builder.CreateCall(
            getRtFunc("rt_create_native_double_arr"),
            {m_builder.getInt32(count), tmpBuf});
    }

    // 原有路径（含非 double 元素）
    Value *arr = m_builder.CreateCall(getRtFunc("rt_create_array"));
    if (ctx->exprList())
    {
        for (auto e : ctx->exprList()->expression())
        {
            Value *v = std::any_cast<Value *>(visit(e));
            Value *boxedV = boxToTzdValue(v);
            m_builder.CreateCall(getRtFunc("rt_array_push"), {arr, boxedV});
        }
    }
    return arr;
}
std::any TzdCompiler::visitMapLiteralExpr(TzdLangParser::MapLiteralExprContext *ctx)
{
    Value *mapVal = m_builder.CreateCall(getRtFunc("rt_create_map"));
    if (ctx->mapEntryList())
    {
        for (auto entry : ctx->mapEntryList()->mapEntry())
        {
            auto keyCtx = entry->mapKey();
            std::string staticKey;
            bool isStaticStr = false;
            if (keyCtx->STRING())
            {
                std::string raw = keyCtx->STRING()->getText();
                if (raw.size() >= 2 && (raw.front() == '"' || raw.front() == '\''))
                {
                    staticKey = raw.substr(1, raw.size() - 2);
                }
                else
                {
                    staticKey = raw;
                }
                isStaticStr = true;
            }
            else if (keyCtx->IDENTIFIER())
            {
                staticKey = keyCtx->IDENTIFIER()->getText();
                isStaticStr = true;
            }
            else if (keyCtx->INTEGER())
            {
                staticKey = keyCtx->INTEGER()->getText();
                isStaticStr = true;
            }

            Value *valRaw = std::any_cast<Value *>(visit(entry->expression()));
            Value *boxedVal = boxToTzdValue(valRaw);

            if (isStaticStr)
            {
                const char *internedKey = internStringLiteral(staticKey)->sVal.c_str();
                Value *keyPtrVal = m_builder.getInt64((uint64_t)internedKey);
                Value *keyPtr = m_builder.CreateIntToPtr(keyPtrVal, m_ptrTy);
                m_builder.CreateCall(getRtFunc("rt_map_set_str"), {mapVal, keyPtr, boxedVal});
            }
            else if (keyCtx->expression())
            {
                Value *keyRaw = std::any_cast<Value *>(visit(keyCtx->expression()));
                Value *boxedKey = boxToTzdValue(keyRaw);
                m_builder.CreateCall(getRtFunc("rt_map_set"), {mapVal, boxedKey, boxedVal});
            }
        }
    }
    return mapVal;
}
std::any TzdCompiler::visitIndexExpr(TzdLangParser::IndexExprContext *ctx)
{
    if (ctx->getStart() && !s_compilingNativeWorker && (s_loopLevel == 0 || jitTrackFrames()))
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }
    std::string containerName = ctx->expression(0)->getText();
    Value *container = nullptr;
    auto hIt = m_hoistedArrays.find(containerName);
    if (hIt != m_hoistedArrays.end())
    {
        container = hIt->second.container;
    }
    else if (m_namedValues.count(containerName))
    {
        container = m_builder.CreateLoad(m_ptrTy, m_namedValues[containerName]);
    }
    else
    {
        container = std::any_cast<Value *>(visit(ctx->expression(0)));
        if (container->getType()->isDoubleTy())
        {
            container = boxToTzdValue(container);
        }
    }
    Value *index_raw = std::any_cast<Value *>(visit(ctx->expression(1)));
    if (index_raw->getType()->isDoubleTy())
    {
        std::string idxExprText = ctx->expression(1)->getText();
        Value *idxI64 = nullptr;
        auto sIt = m_shadowI64IndVars.find(idxExprText);
        if (sIt != m_shadowI64IndVars.end())
        {
            idxI64 = m_builder.CreateLoad(m_builder.getInt64Ty(), sIt->second, idxExprText + "_idx_i64");
        }
        else
        {
            idxI64 = m_builder.CreateFPToSI(index_raw, m_builder.getInt64Ty(), "idx_i64");
        }

        if (hIt != m_hoistedArrays.end())
        {
            if ((hIt->second.inFastLoop || s_compilingNativeWorker) &&
                (hIt->second.safeIndexVar.empty() || hIt->second.safeIndexVar == idxExprText || s_compilingNativeWorker))
            {
                Value *slot = m_builder.CreateGEP(m_doubleTy, hIt->second.fastBuf, idxI64, "elemSlot");
                return (Value *)m_builder.CreateLoad(m_doubleTy, slot, "elemVal");
            }

            Value *canFast = nullptr;
            if (!hIt->second.safeIndexVar.empty() && hIt->second.safeIndexVar == idxExprText && hIt->second.safeFastCond)
            {
                canFast = hIt->second.safeFastCond;
            }
            else
            {
                Value *inBounds = m_builder.CreateICmpULT(idxI64, hIt->second.fastLen, "inBounds");
                canFast = hIt->second.inFastLoop ? inBounds : m_builder.CreateAnd(hIt->second.hasBuf, inBounds, "canFast");
            }

            Function *curFunc = m_builder.GetInsertBlock()->getParent();
            BasicBlock *fastLoadBB = BasicBlock::Create(m_context, "fast_idx_load", curFunc);
            BasicBlock *slowLoadBB = BasicBlock::Create(m_context, "slow_idx_load", curFunc);
            BasicBlock *mergeLoadBB = BasicBlock::Create(m_context, "merge_idx_load", curFunc);

            AllocaInst *resSlot = CreateEntryBlockAlloca(m_doubleTy, nullptr, "elemSlotVal");
            m_builder.CreateCondBr(canFast, fastLoadBB, slowLoadBB);

            m_builder.SetInsertPoint(fastLoadBB);
            Value *slot = m_builder.CreateGEP(m_doubleTy, hIt->second.fastBuf, idxI64, "elemSlot");
            Value *fastVal = m_builder.CreateLoad(m_doubleTy, slot, "elemVal");
            m_builder.CreateStore(fastVal, resSlot);
            m_builder.CreateBr(mergeLoadBB);

            m_builder.SetInsertPoint(slowLoadBB);
            Value *slowD = nullptr;
            if (s_compilingNativeWorker)
            {
                slowD = m_builder.CreateCall(getRtFunc("rt_get_index_native_d_dyn"), {container, index_raw});
            }
            else
            {
                Value *slowBoxed = m_builder.CreateCall(getRtFunc("rt_get_index_fast"), {container, index_raw});
                slowD = inlineToDoubleFast(slowBoxed);
            }
            m_builder.CreateStore(slowD, resSlot);
            m_builder.CreateBr(mergeLoadBB);

            m_builder.SetInsertPoint(mergeLoadBB);
            return (Value *)m_builder.CreateLoad(m_doubleTy, resSlot, "final_elem_d");
        }
        // In native worker, return unboxed double for numeric index reads (enables
        // native-double locals like `var p = G[hi]` so comparisons avoid runtime calls).
        if (s_compilingNativeWorker)
            return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index_native_d_dyn"), {container, index_raw});
        return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index_fast"), {container, index_raw});
    }
    if (auto *constFP = llvm::dyn_cast<llvm::ConstantFP>(index_raw))
    {
        double dVal = constFP->getValueAPF().convertToDouble();
        if (s_compilingNativeWorker)
            return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index_native_d"), {container, m_builder.getInt32((int)dVal)});
        return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index_fast"), {container, ConstantFP::get(m_doubleTy, dVal)});
    }
    if (s_compilingNativeWorker && index_raw->getType()->isPointerTy())
    {
        return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index_native_d_ptr"), {container, index_raw});
    }
    Value *boxedIndex = boxToTzdValue(index_raw);
    return (Value *)m_builder.CreateCall(getRtFunc("rt_get_index"), {container, boxedIndex});
}
std::any TzdCompiler::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext *ctx)
{
    Value *L = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *R = std::any_cast<Value *>(visit(ctx->expression(1)));

    // Both native integers: fast path
    if (L->getType()->isIntegerTy(64) && R->getType()->isIntegerTy(64))
    {
        if (ctx->MUL())
            return std::any((Value *)m_builder.CreateNSWMul(L, R, "muli64"));
        else if (ctx->DIV())
            return std::any((Value *)m_builder.CreateSDiv(L, R, "divi64"));
        else
            return std::any((Value *)m_builder.CreateSRem(L, R, "remi64"));
    }

    // If either operand is a pointer (BIGINT/RATIONAL/string/etc.), use
    // runtime functions which handle all TzdValue types correctly.
    if (L->getType()->isPointerTy() || R->getType()->isPointerTy())
    {
        Value *boxedL = boxToTzdValue(L);
        Value *boxedR = boxToTzdValue(R);
        const char *fnName = ctx->MUL() ? "rt_op_mul" : (ctx->DIV() ? "rt_op_div" : "rt_op_mod");
        return std::any((Value *)m_builder.CreateCall(getRtFunc(fnName), {boxedL, boxedR}));
    }

    L = castToNativeDouble(L);
    R = castToNativeDouble(R);

    // Both doubles: fast path (no boxing needed)
    Value *res = nullptr;
    if (ctx->MUL())
    {
        std::string id0 = getExprIdentifier(ctx->expression(0));
        std::string id1 = getExprIdentifier(ctx->expression(1));
        if (!id0.empty() && !id1.empty() &&
            m_shadowI64IndVars.count(id0) && m_shadowI64IndVars.count(id1))
        {
            Value *i64_0 = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id0], id0 + "_i64_mul");
            Value *i64_1 = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id1], id1 + "_i64_mul");
            Value *i64Prod = m_builder.CreateNSWMul(i64_0, i64_1, "prod_nsw_i64");
            res = m_builder.CreateSIToFP(i64Prod, m_doubleTy, "prod_nsw_d");
        }
        else
        {
            res = m_builder.CreateFMul(L, R, "multmp");
        }
    }
    else if (ctx->DIV())
        res = m_builder.CreateFDiv(L, R, "divtmp");
    else
    {
        if (isIntegerValuedExpr(ctx->expression(0)) && isIntegerValuedExpr(ctx->expression(1)))
        {
            Value *i64L = m_builder.CreateFPToSI(L, m_builder.getInt64Ty(), "rem_lhs_i64");
            Value *i64R = m_builder.CreateFPToSI(R, m_builder.getInt64Ty(), "rem_rhs_i64");
            Value *i64Rem = m_builder.CreateSRem(i64L, i64R, "rem_i64");
            res = m_builder.CreateSIToFP(i64Rem, m_doubleTy, "rem_d");
        }
        else
        {
            res = m_builder.CreateFRem(L, R, "modtmp");
        }
    }
    return std::any((Value *)res);
}
std::any TzdCompiler::visitPowerExpr(TzdLangParser::PowerExprContext *ctx)
{
    Value *L = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *R = std::any_cast<Value *>(visit(ctx->expression(1)));

    if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy())
    {
        Function *powFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::pow, {m_doubleTy});
        return std::any((Value *)m_builder.CreateCall(powFunc, {L, R}, "powtmp"));
    }

    Value *boxedL = boxToTzdValue(L);
    Value *boxedR = boxToTzdValue(R);
    return (Value *)m_builder.CreateCall(getRtFunc("rt_op_pow"), {boxedL, boxedR});
}
std::any TzdCompiler::visitShiftExpr(TzdLangParser::ShiftExprContext *ctx)
{
    std::string id0 = getExprIdentifier(ctx->expression(0));
    std::string id1 = getExprIdentifier(ctx->expression(1));
    Value *i64L = nullptr;
    Value *i64R = nullptr;

    if (!id0.empty() && m_shadowI64IndVars.count(id0))
    {
        i64L = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id0], id0 + "_i64");
    }
    if (!id1.empty() && m_shadowI64IndVars.count(id1))
    {
        i64R = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id1], id1 + "_i64");
    }

    Value *rawL = nullptr;
    Value *rawR = nullptr;
    if (!i64L)
    {
        rawL = castAnyToValue(visit(ctx->expression(0)), "visitShiftExpr.L");
    }
    if (!i64R)
    {
        rawR = castAnyToValue(visit(ctx->expression(1)), "visitShiftExpr.R");
    }

    // If either operand is a pointer (BIGINT, string, etc.), call rt_op_shl/shr/ushr
    if ((rawL && rawL->getType()->isPointerTy()) || (rawR && rawR->getType()->isPointerTy()))
    {
        Value *boxedL = rawL ? boxToTzdValue(rawL) : boxToTzdValue(i64L);
        Value *boxedR = rawR ? boxToTzdValue(rawR) : boxToTzdValue(i64R);
        const char *rtFn = ctx->SHL() ? "rt_op_shl" : (ctx->SHR() ? "rt_op_shr" : "rt_op_ushr");
        return std::any((Value *)m_builder.CreateCall(getRtFunc(rtFn), {boxedL, boxedR}));
    }

    if (!i64L)
        i64L = castToNativeI64(rawL);
    if (!i64R)
        i64R = castToNativeI64(rawR);

    int64_t constShift = -1;
    bool hasConstShift = false;
    if (auto *cR = dyn_cast<ConstantInt>(i64R))
    {
        constShift = cR->getSExtValue();
        hasConstShift = true;
    }
    else if (rawR)
    {
        if (auto *fpR = dyn_cast<ConstantFP>(rawR))
        {
            constShift = (int64_t)fpR->getValueAPF().convertToDouble();
            hasConstShift = true;
        }
    }

    if (ctx->SHL())
    {
        if (hasConstShift)
        {
            if (constShift >= 62 || constShift < 0)
            {
                Value *boxedL = boxToTzdValue(i64L);
                Value *boxedR = boxToTzdValue(i64R);
                return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_shl"), {boxedL, boxedR}));
            }
            int64_t constBase = 0;
            bool hasConstBase = false;
            if (auto *cL = dyn_cast<ConstantInt>(i64L))
            {
                constBase = cL->getSExtValue();
                hasConstBase = true;
            }
            else if (rawL)
            {
                if (auto *fpL = dyn_cast<ConstantFP>(rawL))
                {
                    constBase = (int64_t)fpL->getValueAPF().convertToDouble();
                    hasConstBase = true;
                }
            }
            if (hasConstBase && constBase != 0 && (uint64_t)std::abs(constBase) > (0x7FFFFFFFFFFFFFFFULL >> constShift))
            {
                Value *boxedL = boxToTzdValue(i64L);
                Value *boxedR = boxToTzdValue(i64R);
                return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_shl"), {boxedL, boxedR}));
            }
        }
        else
        {
            Function *curFunc = m_builder.GetInsertBlock()->getParent();
            BasicBlock *fastShlBB = BasicBlock::Create(m_context, "shl.fast", curFunc);
            BasicBlock *slowShlBB = BasicBlock::Create(m_context, "shl.slow", curFunc);
            BasicBlock *mergeShlBB = BasicBlock::Create(m_context, "shl.merge", curFunc);

            Value *isSafe = m_builder.CreateICmpULT(i64R, m_builder.getInt64(62), "shl_safe");
            m_builder.CreateCondBr(isSafe, fastShlBB, slowShlBB);

            m_builder.SetInsertPoint(fastShlBB);
            Value *fastRes = m_builder.CreateShl(i64L, i64R, "shltmp");
            Value *fastBoxed = boxToTzdValue(fastRes);
            m_builder.CreateBr(mergeShlBB);

            m_builder.SetInsertPoint(slowShlBB);
            Value *slowBoxed = m_builder.CreateCall(getRtFunc("rt_op_shl"), {boxToTzdValue(i64L), boxToTzdValue(i64R)});
            m_builder.CreateBr(mergeShlBB);

            m_builder.SetInsertPoint(mergeShlBB);
            PHINode *phi = m_builder.CreatePHI(m_ptrTy, 2, "shl_res");
            phi->addIncoming(fastBoxed, fastShlBB);
            phi->addIncoming(slowBoxed, slowShlBB);
            return std::any((Value *)phi);
        }
    }

    if (hasConstShift && constShift >= 64)
    {
        Value *resI64 = nullptr;
        if (ctx->SHR())
        {
            Value *isNeg = m_builder.CreateICmpSLT(i64L, m_builder.getInt64(0), "isneg");
            resI64 = m_builder.CreateSelect(isNeg, m_builder.getInt64(-1), m_builder.getInt64(0), "shrtmp");
        }
        else // USHR
        {
            resI64 = m_builder.getInt64(0);
        }
        return std::any((Value *)resI64);
    }

    Value *shiftAmt = m_builder.CreateAnd(i64R, m_builder.getInt64(63), "shift_amt");
    Value *resI64 = nullptr;
    if (ctx->SHL())
    {
        resI64 = m_builder.CreateShl(i64L, shiftAmt, "shltmp");
    }
    else if (ctx->SHR())
    {
        // Arithmetic right shift (preserves sign bit)
        resI64 = m_builder.CreateAShr(i64L, shiftAmt, "ashrtmp");
    }
    else // USHR >>>
    {
        // Logical right shift (zero-fill)
        resI64 = m_builder.CreateLShr(i64L, shiftAmt, "lshrtmp");
    }
    return std::any((Value *)resI64);
}
std::any TzdCompiler::visitBitAndExpr(TzdLangParser::BitAndExprContext *ctx)
{
    std::string id0 = getExprIdentifier(ctx->expression(0));
    std::string id1 = getExprIdentifier(ctx->expression(1));
    Value *i64L = nullptr;
    Value *i64R = nullptr;

    if (!id0.empty() && m_shadowI64IndVars.count(id0))
    {
        i64L = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id0], id0 + "_i64");
    }
    if (!id1.empty() && m_shadowI64IndVars.count(id1))
    {
        i64R = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id1], id1 + "_i64");
    }

    Value *rawL = nullptr;
    Value *rawR = nullptr;
    if (!i64L)
    {
        rawL = castAnyToValue(visit(ctx->expression(0)), "visitBitAndExpr.L");
    }
    if (!i64R)
    {
        rawR = castAnyToValue(visit(ctx->expression(1)), "visitBitAndExpr.R");
    }

    if ((rawL && rawL->getType()->isPointerTy()) || (rawR && rawR->getType()->isPointerTy()))
    {
        Value *boxedL = rawL ? boxToTzdValue(rawL) : boxToTzdValue(i64L);
        Value *boxedR = rawR ? boxToTzdValue(rawR) : boxToTzdValue(i64R);
        return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_bitand"), {boxedL, boxedR}));
    }

    if (!i64L)
        i64L = castToNativeI64(rawL);
    if (!i64R)
        i64R = castToNativeI64(rawR);

    Value *resI64 = m_builder.CreateAnd(i64L, i64R, "andtmp");
    return std::any((Value *)resI64);
}
std::any TzdCompiler::visitBitXorExpr(TzdLangParser::BitXorExprContext *ctx)
{
    std::string id0 = getExprIdentifier(ctx->expression(0));
    std::string id1 = getExprIdentifier(ctx->expression(1));
    Value *i64L = nullptr;
    Value *i64R = nullptr;

    if (!id0.empty() && m_shadowI64IndVars.count(id0))
    {
        i64L = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id0], id0 + "_i64");
    }
    if (!id1.empty() && m_shadowI64IndVars.count(id1))
    {
        i64R = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id1], id1 + "_i64");
    }

    Value *rawL = nullptr;
    Value *rawR = nullptr;
    if (!i64L)
    {
        rawL = castAnyToValue(visit(ctx->expression(0)), "visitBitXorExpr.L");
    }
    if (!i64R)
    {
        rawR = castAnyToValue(visit(ctx->expression(1)), "visitBitXorExpr.R");
    }

    if ((rawL && rawL->getType()->isPointerTy()) || (rawR && rawR->getType()->isPointerTy()))
    {
        Value *boxedL = rawL ? boxToTzdValue(rawL) : boxToTzdValue(i64L);
        Value *boxedR = rawR ? boxToTzdValue(rawR) : boxToTzdValue(i64R);
        return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_bitxor"), {boxedL, boxedR}));
    }

    if (!i64L)
        i64L = castToNativeI64(rawL);
    if (!i64R)
        i64R = castToNativeI64(rawR);

    Value *resI64 = m_builder.CreateXor(i64L, i64R, "xortmp");
    return std::any((Value *)resI64);
}
std::any TzdCompiler::visitBitOrExpr(TzdLangParser::BitOrExprContext *ctx)
{
    std::string id0 = getExprIdentifier(ctx->expression(0));
    std::string id1 = getExprIdentifier(ctx->expression(1));
    Value *i64L = nullptr;
    Value *i64R = nullptr;

    if (!id0.empty() && m_shadowI64IndVars.count(id0))
    {
        i64L = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id0], id0 + "_i64");
    }
    if (!id1.empty() && m_shadowI64IndVars.count(id1))
    {
        i64R = m_builder.CreateLoad(m_builder.getInt64Ty(), m_shadowI64IndVars[id1], id1 + "_i64");
    }

    Value *rawL = nullptr;
    Value *rawR = nullptr;
    if (!i64L)
    {
        rawL = castAnyToValue(visit(ctx->expression(0)), "visitBitOrExpr.L");
    }
    if (!i64R)
    {
        rawR = castAnyToValue(visit(ctx->expression(1)), "visitBitOrExpr.R");
    }

    if ((rawL && rawL->getType()->isPointerTy()) || (rawR && rawR->getType()->isPointerTy()))
    {
        Value *boxedL = rawL ? boxToTzdValue(rawL) : boxToTzdValue(i64L);
        Value *boxedR = rawR ? boxToTzdValue(rawR) : boxToTzdValue(i64R);
        return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_bitor"), {boxedL, boxedR}));
    }

    if (!i64L)
        i64L = castToNativeI64(rawL);
    if (!i64R)
        i64R = castToNativeI64(rawR);

    Value *resI64 = m_builder.CreateOr(i64L, i64R, "ortmp");
    return std::any((Value *)resI64);
}
std::any TzdCompiler::visitUnaryExpr(TzdLangParser::UnaryExprContext *ctx)
{
    Value *V = castAnyToValue(visit(ctx->expression()), "visitUnaryExpr.V");
    if (ctx->PLUS())
    {
        return std::any(V);
    }
    if (V->getType()->isDoubleTy())
    {
        if (ctx->MINUS())
        {
            return std::any((Value *)m_builder.CreateFNeg(V, "negtmp"));
        }
        if (ctx->NOT())
        {
            Value *isZero = m_builder.CreateFCmpOEQ(V, ConstantFP::get(m_doubleTy, 0.0), "notcmp");
            return std::any((Value *)m_builder.CreateUIToFP(isZero, m_doubleTy, "notdbl"));
        }
        if (ctx->BIT_NOT())
        {
            Value *i64V = castToNativeI64(V);
            Value *notI64 = m_builder.CreateNot(i64V, "nottmp");
            return std::any((Value *)notI64);
        }
        if (ctx->GXXX())
        {
            Function *sqrtFunc = Intrinsic::getDeclaration(m_module.get(), Intrinsic::sqrt, {m_doubleTy});
            return std::any((Value *)m_builder.CreateCall(sqrtFunc, {V}, "sqrttmp"));
        }
    }
    if (ctx->BIT_NOT())
    {
        if (V->getType()->isPointerTy())
        {
            Value *boxed = boxToTzdValue(V);
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_op_bitnot"), {boxed}));
        }
        Value *i64V = castToNativeI64(V);
        Value *notI64 = m_builder.CreateNot(i64V, "nottmp");
        return std::any((Value *)notI64);
    }
    Value *boxed = boxToTzdValue(V);
    if (ctx->MINUS())
        return (Value *)m_builder.CreateCall(getRtFunc("rt_op_neg"), {boxed});
    if (ctx->NOT())
        return (Value *)m_builder.CreateCall(getRtFunc("rt_op_not"), {boxed});
    if (ctx->BIT_NOT())
        return (Value *)m_builder.CreateCall(getRtFunc("rt_op_bitnot"), {boxed});
    if (ctx->GXXX())
        return (Value *)m_builder.CreateCall(getRtFunc("rt_op_sqrt"), {boxed});

    return std::any(boxed);
}
bool TzdCompiler::emitMemberIncDec(llvm::Value *&result, TzdLangParser::ExpressionContext *lhsCtx, bool isInc, bool isPrefix)
{
    auto *atomExpr = dynamic_cast<TzdLangParser::AtomExprContext *>(lhsCtx);
    if (!atomExpr)
        return false;
    auto *memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(atomExpr->atom());
    if (!memCtx)
        return false;

    Value *obj = std::any_cast<Value *>(visit(memCtx->atom()));
    std::string memberName = memCtx->IDENTIFIER()->getText();
    Value *nameStr = m_builder.CreateGlobalStringPtr(memberName);
    TzdSelector sel = internSelectorConstant(memberName);
    Value *oldVal = m_builder.CreateCall(getRtFunc("rt_tzd_get_member"),
                                         {obj, m_builder.getInt32((int32_t)sel), nameStr});
    Value *one = m_builder.CreateCall(getRtFunc("rt_create_num"), {ConstantFP::get(m_doubleTy, 1.0)});
    const char *opFunc = isInc ? "rt_op_add" : "rt_op_sub";
    Value *newVal = m_builder.CreateCall(getRtFunc(opFunc), {oldVal, one});
    m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
                         {obj, m_builder.getInt32((int32_t)sel), nameStr, newVal});
    result = isPrefix ? newVal : oldVal;
    return true;
}

std::any TzdCompiler::visitPostfixExpr(TzdLangParser::PostfixExprContext *ctx)
{
    auto lhsCtx = ctx->expression();
    bool isLValue = false;
    if (dynamic_cast<TzdLangParser::IndexExprContext *>(lhsCtx))
    {
        isLValue = true;
    }
    else if (auto atom = dynamic_cast<TzdLangParser::AtomExprContext *>(lhsCtx))
    {
        if (dynamic_cast<TzdLangParser::IdExprContext *>(atom->atom()) ||
            dynamic_cast<TzdLangParser::MemberAccessExprContext *>(atom->atom()))
        {
            isLValue = true;
        }
    }
    if (!isLValue)
    {
        throw std::runtime_error("无效的自增/自减操作目标: '" + lhsCtx->getText() + "'。操作数必须是可赋值的变量、对象属性或数组元素 (需要左值)");
    }

    std::string name = lhsCtx->getText();

    Value *memberResult = nullptr;
    if (emitMemberIncDec(memberResult, lhsCtx, ctx->INC() != nullptr, false))
    {
        return memberResult;
    }

    // =========================================================
    // 1. 原生变量优化路径 (Native Optimization)
    // =========================================================
    // 必须最先检查！如果命中，直接生成 CPU 指令并返回，避免生成任何 Runtime 调用
    if (m_nativeDoubleLocals.count(name))
    {
        Value *ptr = m_nativeDoubleLocals[name];

        // Load: 读取原生 double
        Value *oldVal = m_builder.CreateLoad(m_doubleTy, ptr, name + "_old");

        // Math: 原生加减 (纯 CPU 指令)
        Value *one = ConstantFP::get(m_doubleTy, 1.0);
        Value *newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);

        // Store: 写回原生 double
        m_builder.CreateStore(newVal, ptr);

        // 后缀操作 (i++) 返回旧值
        return oldVal;
    }

    // =========================================================
    // 1.5 隐式 this SROA 路径 (Boxed Fallback 之前拦截)
    // =========================================================
    if (m_namedValues.count(name) == 0 && m_namedValues.count("this") > 0)
    {
        auto fIt = m_varFieldAllocas.find("this");
        if (fIt != m_varFieldAllocas.end())
        {
            auto slotIt = fIt->second.find(name);
            if (slotIt != fIt->second.end() && slotIt->second)
            {
                Value *ptr = slotIt->second;
                Value *oldVal = m_builder.CreateLoad(m_doubleTy, ptr);
                Value *one = ConstantFP::get(m_doubleTy, 1.0);
                Value *newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);
                m_builder.CreateStore(newVal, ptr);
                return oldVal;
            }
        }
    }

    // =========================================================
    // 2. 普通对象/成员路径 (Boxed Fallback)
    // =========================================================
    Value *oldValRaw = std::any_cast<Value *>(visit(lhsCtx));

    Value *boxedNew = nullptr;
    if (oldValRaw->getType()->isPointerTy())
    {
        Value *one = m_builder.CreateCall(getRtFunc("rt_create_num"), {ConstantFP::get(m_doubleTy, 1.0)});
        const char *opFunc = ctx->INC() ? "rt_op_add" : "rt_op_sub";
        boxedNew = m_builder.CreateCall(getRtFunc(opFunc), {oldValRaw, one});
    }
    else
    {
        Value *nativeOld = inlineToDoubleFast(oldValRaw);
        Value *one = ConstantFP::get(m_doubleTy, 1.0);
        Value *newVal = ctx->INC() ? m_builder.CreateFAdd(nativeOld, one) : m_builder.CreateFSub(nativeOld, one);
        boxedNew = boxToTzdValue(newVal);
    }

    if (m_namedValues.count(name))
    {
        Value *destPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        m_builder.CreateCall(getRtFunc("rt_copy_value"), {destPtr, boxedNew});
    }
    else if (m_namedValues.count("this") && !m_namedValues.count(name))
    {
        Value *thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        TzdSelector sel = internSelectorConstant(name);
        m_builder.CreateCall(getRtFunc("rt_tzd_store_member"),
                             {thisPtr, m_builder.getInt32((int32_t)sel), nameStr, boxedNew});
    }
    else
    {
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedNew});
    }

    return oldValRaw;
}
std::any TzdCompiler::visitPrefixExpr(TzdLangParser::PrefixExprContext *ctx)
{
    auto lhsCtx = ctx->expression();
    bool isLValue = false;
    if (dynamic_cast<TzdLangParser::IndexExprContext *>(lhsCtx))
    {
        isLValue = true;
    }
    else if (auto atom = dynamic_cast<TzdLangParser::AtomExprContext *>(lhsCtx))
    {
        if (dynamic_cast<TzdLangParser::IdExprContext *>(atom->atom()) ||
            dynamic_cast<TzdLangParser::MemberAccessExprContext *>(atom->atom()))
        {
            isLValue = true;
        }
    }
    if (!isLValue)
    {
        throw std::runtime_error("无效的自增/自减操作目标: '" + lhsCtx->getText() + "'。操作数必须是可赋值的变量、对象属性或数组元素 (需要左值)");
    }

    std::string name = lhsCtx->getText();

    Value *memberResult = nullptr;
    if (emitMemberIncDec(memberResult, lhsCtx, ctx->INC() != nullptr, true))
    {
        return memberResult;
    }

    // --- [1] 原生变量优化 ---
    if (m_nativeDoubleLocals.count(name))
    {
        Value *ptr = m_nativeDoubleLocals[name];
        Value *oldVal = m_builder.CreateLoad(m_doubleTy, ptr);
        Value *one = ConstantFP::get(m_doubleTy, 1.0);
        Value *newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);
        m_builder.CreateStore(newVal, ptr);
        return newVal; // 前缀返回新值
    }

    // --- [1.5] 隐式 this SROA 优化 ---
    if (m_namedValues.count(name) == 0 && m_namedValues.count("this") > 0)
    {
        auto fIt = m_varFieldAllocas.find("this");
        if (fIt != m_varFieldAllocas.end())
        {
            auto slotIt = fIt->second.find(name);
            if (slotIt != fIt->second.end() && slotIt->second)
            {
                Value *ptr = slotIt->second;
                Value *oldVal = m_builder.CreateLoad(m_doubleTy, ptr);
                Value *one = ConstantFP::get(m_doubleTy, 1.0);
                Value *newVal = ctx->INC() ? m_builder.CreateFAdd(oldVal, one) : m_builder.CreateFSub(oldVal, one);
                m_builder.CreateStore(newVal, ptr);
                return newVal;
            }
        }
    }

    // --- [2] 安全回退 ---
    Value *oldValRaw = std::any_cast<Value *>(visit(lhsCtx));
    Value *boxedNew = nullptr;
    Value *retVal = nullptr;
    if (oldValRaw->getType()->isPointerTy())
    {
        Value *one = m_builder.CreateCall(getRtFunc("rt_create_num"), {ConstantFP::get(m_doubleTy, 1.0)});
        const char *opFunc = ctx->INC() ? "rt_op_add" : "rt_op_sub";
        boxedNew = m_builder.CreateCall(getRtFunc(opFunc), {oldValRaw, one});
        retVal = boxedNew;
    }
    else
    {
        Value *nativeOld = inlineToDoubleFast(oldValRaw);
        Value *one = ConstantFP::get(m_doubleTy, 1.0);
        Value *newVal = ctx->INC() ? m_builder.CreateFAdd(nativeOld, one) : m_builder.CreateFSub(nativeOld, one);
        boxedNew = boxToTzdValue(newVal);
        retVal = newVal;
    }

    if (m_namedValues.count(name))
    {
        Value *destPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues[name]);
        m_builder.CreateCall(getRtFunc("rt_copy_value"), {destPtr, boxedNew});
    }
    else if (dynamic_cast<TzdLangParser::IdExprContext *>(lhsCtx))
    {
        Value *nameStr = m_builder.CreateGlobalStringPtr(name);
        m_builder.CreateCall(getRtFunc("rt_store_var"), {nameStr, boxedNew});
    }

    return retVal;
}
std::any TzdCompiler::visitForInit(TzdLangParser::ForInitContext *ctx)
{
    if (ctx->variableDeclaration())
        return visit(ctx->variableDeclaration());
    if (ctx->expression())
        return visit(ctx->expression());
    return std::any();
}
std::any TzdCompiler::visitRelationalExpr(TzdLangParser::RelationalExprContext *ctx)
{
    Value *L = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *R = std::any_cast<Value *>(visit(ctx->expression(1)));

    if (L->getType()->isIntegerTy(64) && R->getType()->isIntegerTy(64))
    {
        Value *res = nullptr;
        if (ctx->GT())
            res = m_builder.CreateICmpSGT(L, R);
        else if (ctx->LT())
            res = m_builder.CreateICmpSLT(L, R);
        else if (ctx->GE())
            res = m_builder.CreateICmpSGE(L, R);
        else if (ctx->LE())
            res = m_builder.CreateICmpSLE(L, R);
        else
            res = m_builder.CreateICmpEQ(L, R);
        return std::any((Value *)res);
    }

    if ((L->getType()->isDoubleTy() || L->getType()->isIntegerTy()) &&
        (R->getType()->isDoubleTy() || R->getType()->isIntegerTy()) &&
        !L->getType()->isPointerTy() && !R->getType()->isPointerTy())
    {
        Value *dL = castToNativeDouble(L);
        Value *dR = castToNativeDouble(R);
        Value *res = nullptr;
        if (ctx->GT())
            res = m_builder.CreateFCmpOGT(dL, dR);
        else if (ctx->LT())
            res = m_builder.CreateFCmpOLT(dL, dR);
        else if (ctx->GE())
            res = m_builder.CreateFCmpOGE(dL, dR);
        else if (ctx->LE())
            res = m_builder.CreateFCmpOLE(dL, dR);
        else
            res = m_builder.CreateFCmpOEQ(dL, dR);
        return std::any((Value *)res);
    }

    Value *boxedL = boxToTzdValue(L);
    Value *boxedR = boxToTzdValue(R);
    const char *rtFunc = ctx->GT() ? "rt_op_gt" : ctx->LT() ? "rt_op_lt"
                                              : ctx->GE()   ? "rt_op_ge"
                                                            : "rt_op_le";
    return std::any((Value *)m_builder.CreateCall(getRtFunc(rtFunc), {boxedL, boxedR}));
}
std::any TzdCompiler::visitEqualityExpr(TzdLangParser::EqualityExprContext *ctx)
{
    Value *L = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *R = std::any_cast<Value *>(visit(ctx->expression(1)));

    // Integer fast path: direct icmp
    if (L->getType()->isIntegerTy(64) && R->getType()->isIntegerTy(64))
    {
        if (ctx->EEQ())
            return std::any((Value *)m_builder.CreateICmpEQ(L, R, "eqi64"));
        else
            return std::any((Value *)m_builder.CreateICmpNE(L, R, "nei64"));
    }

    // Native double fast path: direct fcmp, zero heap alloc, zero external calls
    if ((L->getType()->isDoubleTy() || L->getType()->isIntegerTy()) &&
        (R->getType()->isDoubleTy() || R->getType()->isIntegerTy()) &&
        !L->getType()->isPointerTy() && !R->getType()->isPointerTy())
    {
        Value *dL = castToNativeDouble(L);
        Value *dR = castToNativeDouble(R);
        if (ctx->EEQ())
            return std::any((Value *)m_builder.CreateFCmpOEQ(dL, dR, "eqtmp"));
        else
            return std::any((Value *)m_builder.CreateFCmpONE(dL, dR, "netmp"));
    }

    // Fallback: boxed path for string/object comparison
    Value *boxedL = boxToTzdValue(L);
    Value *boxedR = boxToTzdValue(R);
    const char *func = ctx->EEQ() ? "rt_op_eq" : "rt_op_ne";
    return (Value *)m_builder.CreateCall(getRtFunc(func), {boxedL, boxedR});
}
std::any TzdCompiler::visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext *ctx)
{
    Function *func = m_builder.GetInsertBlock()->getParent();
    AllocaInst *resultSlot = CreateEntryBlockAlloca(m_boolTy, nullptr, "and.sc");

    Value *lhs = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *lhsBool = toNativeBool(lhs);
    BasicBlock *lhsEndBB = m_builder.GetInsertBlock();

    BasicBlock *rhsBB = BasicBlock::Create(m_context, "and.rhs", func);
    BasicBlock *falseBB = BasicBlock::Create(m_context, "and.false", func);
    BasicBlock *endBB = BasicBlock::Create(m_context, "and.end", func);

    m_builder.SetInsertPoint(lhsEndBB);
    m_builder.CreateCondBr(lhsBool, rhsBB, falseBB);

    m_builder.SetInsertPoint(falseBB);
    m_builder.CreateStore(m_builder.getFalse(), resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(rhsBB);
    Value *rhs = std::any_cast<Value *>(visit(ctx->expression(1)));
    Value *rhsBool = toNativeBool(rhs);
    BasicBlock *rhsEndBB = m_builder.GetInsertBlock();
    m_builder.SetInsertPoint(rhsEndBB);
    m_builder.CreateStore(rhsBool, resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(endBB);
    Value *loaded = m_builder.CreateLoad(m_boolTy, resultSlot);
    return (Value *)loaded;
}
std::any TzdCompiler::visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext *ctx)
{
    Function *func = m_builder.GetInsertBlock()->getParent();
    AllocaInst *resultSlot = CreateEntryBlockAlloca(m_boolTy, nullptr, "or.sc");

    Value *lhs = std::any_cast<Value *>(visit(ctx->expression(0)));
    Value *lhsBool = toNativeBool(lhs);
    BasicBlock *lhsEndBB = m_builder.GetInsertBlock();

    BasicBlock *rhsBB = BasicBlock::Create(m_context, "or.rhs", func);
    BasicBlock *trueBB = BasicBlock::Create(m_context, "or.true", func);
    BasicBlock *endBB = BasicBlock::Create(m_context, "or.end", func);

    m_builder.SetInsertPoint(lhsEndBB);
    m_builder.CreateCondBr(lhsBool, trueBB, rhsBB);

    m_builder.SetInsertPoint(trueBB);
    m_builder.CreateStore(m_builder.getTrue(), resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(rhsBB);
    Value *rhs = std::any_cast<Value *>(visit(ctx->expression(1)));
    Value *rhsBool = toNativeBool(rhs);
    BasicBlock *rhsEndBB = m_builder.GetInsertBlock();
    m_builder.SetInsertPoint(rhsEndBB);
    m_builder.CreateStore(rhsBool, resultSlot);
    m_builder.CreateBr(endBB);

    m_builder.SetInsertPoint(endBB);
    Value *loaded = m_builder.CreateLoad(m_boolTy, resultSlot);
    return (Value *)loaded;
}
std::any TzdCompiler::visitParenExpr(TzdLangParser::ParenExprContext *ctx)
{
    return visit(ctx->expression());
}
std::any TzdCompiler::visitCastExpr(TzdLangParser::CastExprContext *ctx)
{
    Value *val = std::any_cast<Value *>(visit(ctx->expression()));
    std::string targetType = ctx->typeType()->getText();
    if (targetType == "int" || targetType == "i32" || targetType == "long" || targetType == "i64")
    {
        Value *dVal = castToNativeDouble(val);
        Value *iVal = m_builder.CreateFPToSI(dVal, m_builder.getInt64Ty());
        return (Value *)m_builder.CreateSIToFP(iVal, m_doubleTy);
    }
    if (targetType == "double" || targetType == "float")
    {
        return (Value *)castToNativeDouble(val);
    }
    if (targetType == "bool")
    {
        Value *bVal = toNativeBool(val);
        return (Value *)bVal;
    }
    Value *boxed = boxToTzdValue(val);
    Value *typeName = m_builder.CreateGlobalStringPtr(targetType);
    return (Value *)m_builder.CreateCall(getRtFunc("rt_cast"), {boxed, typeName});
}
std::any TzdCompiler::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext *ctx) { return (Value *)m_builder.getTrue(); }
std::any TzdCompiler::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext *ctx) { return (Value *)m_builder.getFalse(); }
std::any TzdCompiler::visitImportStmt(TzdLangParser::ImportStmtContext *ctx)
{
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitClassDeclaration(TzdLangParser::ClassDeclarationContext *ctx)
{
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext *ctx)
{
    Value *obj = std::any_cast<Value *>(visit(ctx->expression()));
    std::string targetName = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();
    Value *typeNameStr = m_builder.CreateGlobalStringPtr(targetName);
    Value *boxedObj = boxToTzdValue(obj);
    Value *resBool = m_builder.CreateCall(getRtFunc("rt_type_check"), {boxedObj, typeNameStr});
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_bool"), {resBool});
}
std::any TzdCompiler::visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext *ctx)
{
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitEnumDeclaration(TzdLangParser::EnumDeclarationContext *ctx)
{
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitSuperExpr(TzdLangParser::SuperExprContext *ctx)
{
    if (!m_namedValues.count("this"))
    {
        return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
    }

    Value *thisPtr = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"]);

    std::vector<Value *> args;
    args.push_back(thisPtr);

    if (ctx->exprList())
    {
        for (auto e : ctx->exprList()->expression())
        {
            Value *v = std::any_cast<Value *>(visit(e));
            args.push_back(boxToTzdValue(v));
        }
    }
    Value *argCount = m_builder.getInt32((unsigned int)args.size() - 1);
    std::vector<Value *> callParams;
    callParams.push_back(thisPtr);
    callParams.push_back(argCount);
    for (size_t i = 1; i < args.size(); ++i)
        callParams.push_back(args[i]);
    return (Value *)m_builder.CreateCall(getRtFunc("rt_call_super"), callParams);
}

bool TzdCompiler::tryInlineMathIntrinsic(const std::string &funcName,
                                         const std::vector<TzdLangParser::ExpressionContext *> &exprs,
                                         llvm::Value *&result)
{
    if (!TzdJitEngine::getConfig().enableMathIntrinsics)
        return false;
    int argCount = (int)exprs.size();

    if (argCount == 1)
    {
        Intrinsic::ID intrinId = Intrinsic::not_intrinsic;
        if (funcName == "sqrt")
            intrinId = Intrinsic::sqrt;
        else if (funcName == "abs")
            intrinId = Intrinsic::fabs;
        else if (funcName == "floor")
            intrinId = Intrinsic::floor;
        else if (funcName == "ceil")
            intrinId = Intrinsic::ceil;
        else if (funcName == "round")
            intrinId = Intrinsic::round;
        else if (funcName == "trunc")
            intrinId = Intrinsic::trunc;
        else if (funcName == "sin")
            intrinId = Intrinsic::sin;
        else if (funcName == "cos")
            intrinId = Intrinsic::cos;
        else if (funcName == "exp")
            intrinId = Intrinsic::exp;
        else if (funcName == "log")
            intrinId = Intrinsic::log;
        else if (funcName == "log10")
            intrinId = Intrinsic::log10;
        else if (funcName == "log2")
            intrinId = Intrinsic::log2;

        if (intrinId != Intrinsic::not_intrinsic)
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *argVal = std::any_cast<Value *>(visit(exprs[0]));
            s_inTailPosition = oldTail;

            Value *argD = castToNativeDouble(argVal);
            Function *intrin = Intrinsic::getDeclaration(m_module.get(), intrinId, {m_doubleTy});
            result = m_builder.CreateCall(intrin, {argD});
            return true;
        }
    }
    else if (argCount == 2)
    {
        if (funcName == "pow")
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *a0 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[0])));
            Value *a1 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[1])));
            s_inTailPosition = oldTail;

            Function *intrin = Intrinsic::getDeclaration(m_module.get(), Intrinsic::pow, {m_doubleTy});
            result = m_builder.CreateCall(intrin, {a0, a1});
            return true;
        }
        else if (funcName == "min")
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *a0 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[0])));
            Value *a1 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[1])));
            s_inTailPosition = oldTail;

            Function *intrin = Intrinsic::getDeclaration(m_module.get(), Intrinsic::minnum, {m_doubleTy});
            result = m_builder.CreateCall(intrin, {a0, a1});
            return true;
        }
        else if (funcName == "max")
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *a0 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[0])));
            Value *a1 = castToNativeDouble(std::any_cast<Value *>(visit(exprs[1])));
            s_inTailPosition = oldTail;

            Function *intrin = Intrinsic::getDeclaration(m_module.get(), Intrinsic::maxnum, {m_doubleTy});
            result = m_builder.CreateCall(intrin, {a0, a1});
            return true;
        }
    }
    return false;
}

bool TzdCompiler::tryInlineFunction(const std::string &funcName,
                                    const std::vector<TzdLangParser::ExpressionContext *> &exprs,
                                    llvm::Value *&result,
                                    llvm::Value *receiverVal,
                                    const std::string &explicitClassName,
                                    const std::string &receiverVarName)
{
    if (!TzdJitEngine::isAstInliningEnabled() || TzdJitEngine::getOptLevel() <= 0)
    {
        return false;
    }

    if (s_currentInlineDepth >= TzdJitEngine::getConfig().maxInlineDepth)
    {
        return false;
    }

    std::string inlineKey = (!explicitClassName.empty()) ? (explicitClassName + "." + funcName) : funcName;
    if (s_inlinedFunctionsInStack.count(inlineKey) && s_currentRecursionPeelDepth == 0)
    {
        return false; // 防止递归循环调用引发死循环内联
    }

    Function *currentFunc = m_builder.GetInsertBlock()->getParent();
    std::string currentName = currentFunc->getName().str();
    std::string curBaseName = currentName;
    if (curBaseName.size() > 14 && curBaseName.substr(curBaseName.size() - 14) == "_worker_native")
    {
        curBaseName = curBaseName.substr(0, curBaseName.size() - 14);
    }
    else if (curBaseName.size() > 7 && curBaseName.substr(curBaseName.size() - 7) == "_worker")
    {
        curBaseName = curBaseName.substr(0, curBaseName.size() - 7);
    }
    size_t lastUnderscore = curBaseName.find_last_of('_');
    if (lastUnderscore != std::string::npos && lastUnderscore + 1 < curBaseName.size() && curBaseName[lastUnderscore + 1] == 'v')
    {
        curBaseName = curBaseName.substr(0, lastUnderscore);
    }

    std::string calleeNativeName = (!explicitClassName.empty()) ? (explicitClassName + "_" + funcName) : funcName;
    if (s_currentRecursionPeelDepth == 0)
    {
        if (curBaseName == funcName || curBaseName == calleeNativeName ||
            currentName == calleeNativeName ||
            currentName == calleeNativeName + "_worker" ||
            currentName == calleeNativeName + "_worker_native" ||
            currentName == funcName ||
            currentName == funcName + "_worker" ||
            currentName == funcName + "_worker_native")
        {
            return false; // 防止普通调用时的自递归死循环内联
        }
    }

    if (!g_CurrentInterpreter)
        return false;

    // 在当前解释器作用域或类结构中查找目标函数的 AST
    TzdValue calleeVal;
    bool found = false;
    std::string targetClassName = explicitClassName;
    std::string targetMethodName = funcName;

    if (!targetClassName.empty())
    {
        if (TzdClassDef *cls = TzdOopManager::getClass(targetClassName))
        {
            if (ClassMethod *m = cls->findMethod(targetMethodName))
            {
                if (m->body)
                {
                    calleeVal.funcBody = m->body;
                    calleeVal.params = m->params;
                    calleeVal.paramTypes = m->paramTypes;
                    found = true;
                }
            }
        }
    }
    else
    {
        for (auto it = g_CurrentInterpreter->scopes.rbegin(); it != g_CurrentInterpreter->scopes.rend(); ++it)
        {
            auto vIt = it->find(funcName);
            if (vIt != it->end() && vIt->second.type == TzdValue::FUNCTION && vIt->second.funcBody)
            {
                calleeVal = vIt->second;
                found = true;
                break;
            }
        }

        if (!found)
        {
            size_t dotPos = funcName.find('.');
            if (dotPos != std::string::npos)
            {
                std::string className = funcName.substr(0, dotPos);
                std::string methodName = funcName.substr(dotPos + 1);
                TzdClassDef *cls = TzdOopManager::getClass(className);
                if (!cls)
                {
                    auto it = m_varClassTypes.find(className);
                    if (it != m_varClassTypes.end())
                    {
                        cls = TzdOopManager::getClass(it->second);
                        targetClassName = it->second;
                    }
                }
                else
                {
                    targetClassName = className;
                }
                if (cls)
                {
                    if (ClassMethod *m = cls->findMethod(methodName))
                    {
                        if (m->body)
                        {
                            calleeVal.funcBody = m->body;
                            calleeVal.params = m->params;
                            calleeVal.paramTypes = m->paramTypes;
                            found = true;
                            targetMethodName = methodName;
                        }
                    }
                }
            }
        }
        if (!found && funcName == s_currentCompilingFuncName && s_currentFunctionBlock)
        {
            calleeVal.funcBody = s_currentFunctionBlock;
            calleeVal.params = s_currentFunctionParams;
            found = true;
        }
    }

    if (!found || !calleeVal.funcBody)
        return false;

    // 参数数量匹配校验
    if (calleeVal.params.size() != exprs.size())
        return false;

    // 语句行数阈值校验（小函数才内联）
    size_t stmtCount = calleeVal.funcBody->statement().size();
    if ((int)stmtCount > TzdJitEngine::getConfig().maxInlineStmts)
    {
        return false;
    }

    // 递归函数不应被内联到其他函数中，保持其在自身 native worker 中运行
    bool isCalleeRecursive = false;
    std::function<void(antlr4::tree::ParseTree *)> checkRec = [&](antlr4::tree::ParseTree *node)
    {
        if (!node || isCalleeRecursive)
            return;
        if (auto call = dynamic_cast<TzdLangParser::CallExprContext *>(node))
        {
            if (call->atom() && call->atom()->getText() == funcName)
            {
                isCalleeRecursive = true;
                return;
            }
        }
        for (size_t i = 0; i < node->children.size(); ++i)
        {
            checkRec(node->children[i]);
        }
    };
    checkRec(calleeVal.funcBody);
    if (isCalleeRecursive)
    {
        return false;
    }

    // AST 内联展开目前仅支持纯数值返回的函数（返回类型为 double）
    // 返回对象/this/字符串/void 的函数走标准调用分发，避免返回值退化为 0.0
    bool returnsDouble = !calleeVal.returnType.empty()
                             ? isExplicitNumericType(calleeVal.returnType)
                             : isBlockReturningDouble(calleeVal.funcBody);
    if (!returnsDouble)
    {
        return false;
    }

    // --- 执行 AST 级直接内联 ---
    TzdJitEngine::recordInlinedCall();
    s_inlinedFunctionsInStack.insert(inlineKey);
    s_currentInlineDepth++;

    // 1. 在调用方上下文中计算实参
    auto getAggregateSlots = [&](const std::string &varOrPath) -> std::unordered_map<std::string, AllocaInst *>
    {
        auto it = m_varFieldAllocas.find(varOrPath);
        if (it != m_varFieldAllocas.end())
        {
            return it->second;
        }
        size_t dot = varOrPath.find('.');
        if (dot != std::string::npos)
        {
            std::string root = varOrPath.substr(0, dot);
            std::string subPrefix = varOrPath.substr(dot + 1) + ".";
            auto rIt = m_varFieldAllocas.find(root);
            if (rIt != m_varFieldAllocas.end())
            {
                std::unordered_map<std::string, AllocaInst *> subMap;
                for (const auto &pair : rIt->second)
                {
                    if (pair.first.rfind(subPrefix, 0) == 0)
                    {
                        subMap[pair.first.substr(subPrefix.size())] = pair.second;
                    }
                }
                return subMap;
            }
        }
        return {};
    };

    std::vector<Value *> evaluatedArgs;
    evaluatedArgs.reserve(exprs.size());
    for (size_t i = 0; i < exprs.size(); ++i)
    {
        bool oldTail = s_inTailPosition;
        s_inTailPosition = false;
        std::string argText = exprs[i]->getText();
        Value *argVal = nullptr;
        if (s_compilingNativeWorker && !getAggregateSlots(argText).empty())
        {
            argVal = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        }
        else
        {
            argVal = std::any_cast<Value *>(visit(exprs[i]));
        }
        s_inTailPosition = oldTail;
        evaluatedArgs.push_back(argVal);
    }

    // 2. 保存调用方的局部变量作用域
    auto savedNamedValues = m_namedValues;
    auto savedNativeDoubleLocals = m_nativeDoubleLocals;
    auto savedDeclaredLocals = m_declaredLocals;
    auto savedParamNames = s_currentFuncParamNames;
    auto savedVarClassTypes = m_varClassTypes;
    auto savedClassDef = m_currentClassDef;
    auto savedVarFieldAllocas = m_varFieldAllocas;

    if (!receiverVarName.empty())
    {
        auto slots = getAggregateSlots(receiverVarName);
        if (!slots.empty())
        {
            m_varFieldAllocas["this"] = slots;
        }
    }
    for (size_t i = 0; i < exprs.size(); ++i)
    {
        std::string argText = exprs[i]->getText();
        auto slots = getAggregateSlots(argText);
        if (!slots.empty() && i < calleeVal.params.size())
        {
            m_varFieldAllocas[calleeVal.params[i]] = slots;
        }
    }

    // 3. 为被调用函数形参创建局部变量插槽 (Alloca) 并写入实参
    s_currentFuncParamNames.clear();

    if (receiverVal)
    {
        AllocaInst *thisAlloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, "__inl_this_" + std::to_string(s_inlineUid++));
        m_builder.CreateStore(boxToTzdValue(receiverVal), thisAlloc);
        m_namedValues["this"] = thisAlloc;
        m_declaredLocals.insert("this");
        if (!targetClassName.empty())
        {
            m_varClassTypes["this"] = targetClassName;
            m_currentClassDef = TzdOopManager::getClass(targetClassName);
        }
    }

    for (size_t i = 0; i < calleeVal.params.size(); ++i)
    {
        std::string pName = calleeVal.params[i];
        std::string mangled = "__inl_" + targetMethodName + "_" + std::to_string(s_inlineUid++) + "_" + pName;
        s_currentFuncParamNames.push_back(pName);
        m_declaredLocals.insert(pName);

        if (i < exprs.size())
        {
            std::string cls = deduceExprClassName(exprs[i], savedVarClassTypes, savedClassDef);
            if (!cls.empty())
            {
                m_varClassTypes[pName] = cls;
            }
            else
            {
                std::string argExprText = exprs[i]->getText();
                auto it = savedVarClassTypes.find(argExprText);
                if (it != savedVarClassTypes.end())
                {
                    m_varClassTypes[pName] = it->second;
                }
                else if (i < calleeVal.paramTypes.size() && !calleeVal.paramTypes[i].empty())
                {
                    m_varClassTypes[pName] = calleeVal.paramTypes[i];
                }
            }
        }

        Value *arg = evaluatedArgs[i];
        bool isParamNumericType = (i < calleeVal.paramTypes.size()) &&
                                  (calleeVal.paramTypes[i] == "int" || calleeVal.paramTypes[i] == "float" ||
                                   calleeVal.paramTypes[i] == "double" || calleeVal.paramTypes[i] == "number");
        if (arg->getType()->isDoubleTy() || isParamNumericType)
        {
            Value *dArg = castToNativeDouble(arg);
            AllocaInst *alloc = CreateEntryBlockAlloca(m_doubleTy, nullptr, mangled + "_d");
            m_builder.CreateStore(dArg, alloc);
            m_nativeDoubleLocals[pName] = alloc;
        }
        else
        {
            AllocaInst *alloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, mangled);
            m_builder.CreateStore(boxToTzdValue(arg), alloc);
            m_namedValues[pName] = alloc;
        }
    }

    // 4. 创建内联返回汇合块与返回值槽
    BasicBlock *inlineRetBB = BasicBlock::Create(m_context, "inlined_ret_" + inlineKey, currentFunc);
    AllocaInst *retNativeDouble = CreateEntryBlockAlloca(m_doubleTy, nullptr, "inl_ret_d");
    AllocaInst *retBoxed = CreateEntryBlockAlloca(m_ptrTy, nullptr, "inl_ret_boxed");
    m_builder.CreateStore(ConstantFP::get(m_doubleTy, 0.0), retNativeDouble);
    m_builder.CreateStore(m_builder.CreateCall(getRtFunc("rt_create_null")), retBoxed);
    s_inlineReturnStack.push_back({inlineRetBB, retNativeDouble, retBoxed, returnsDouble});

    bool savedTail = s_inTailPosition;
    s_inTailPosition = false; // 被内联函数体绝不继承调用方的尾调用状态！
    auto *savedTailBB = s_tailRecurseBB;
    s_tailRecurseBB = nullptr;
    auto *savedNativeWorker = s_currentNativeWorkerFunc;
    if (s_currentRecursionPeelDepth == 0)
    {
        s_currentNativeWorkerFunc = nullptr; // 被内联函数体不能误调用外层函数的 native worker 自递归！
    }

    // 5. 遍历并生成被内联函数体 IR
    visit(calleeVal.funcBody);

    s_inTailPosition = savedTail;
    s_tailRecurseBB = savedTailBB;
    s_currentNativeWorkerFunc = savedNativeWorker;

    // 6. 如果未显式 return 导致自然执行到底部，分支到汇合块
    BasicBlock *curBB = m_builder.GetInsertBlock();
    if (curBB && !curBB->getTerminator())
    {
        if (llvm::pred_empty(curBB))
        {
            curBB->eraseFromParent();
        }
        else
        {
            m_builder.CreateBr(inlineRetBB);
        }
    }

    // 7. 恢复内联栈与调用方作用域
    s_inlineReturnStack.pop_back();
    s_inlinedFunctionsInStack.erase(inlineKey);
    s_currentInlineDepth--;

    m_namedValues = savedNamedValues;
    m_nativeDoubleLocals = savedNativeDoubleLocals;
    m_declaredLocals = savedDeclaredLocals;
    s_currentFuncParamNames = savedParamNames;
    m_varClassTypes = savedVarClassTypes;
    m_currentClassDef = savedClassDef;
    m_varFieldAllocas = savedVarFieldAllocas;

    // 8. 汇合点加载返回值
    m_builder.SetInsertPoint(inlineRetBB);
    if (returnsDouble)
    {
        Value *resD = m_builder.CreateLoad(m_doubleTy, retNativeDouble, "inl_res_d");
        result = resD;
    }
    else
    {
        Value *resBox = m_builder.CreateLoad(m_ptrTy, retBoxed, "inl_res_ptr");
        result = resBox;
    }
    return true;
}

std::any TzdCompiler::visitCallExpr(TzdLangParser::CallExprContext *ctx)
{
    std::string funcName = ctx->atom()->getText();
    auto exprs = ctx->exprList() ? ctx->exprList()->expression()
                                 : std::vector<TzdLangParser::ExpressionContext *>();
    int argCount = (int)exprs.size();

    // 0a. 数学内建指令硬件级极速内联 (sqrt, abs, min, max, sin, cos 等)
    Value *intrinsicResult = nullptr;
    if (tryInlineMathIntrinsic(funcName, exprs, intrinsicResult))
    {
        return std::any((Value *)intrinsicResult);
    }

    // 0a2. toString intrinsic — direct C call instead of interpreter bridge
    if (funcName == "toString" && argCount == 1)
    {
        Value *argRaw = std::any_cast<Value *>(visit(exprs[0]));
        if (argRaw && argRaw->getType()->isDoubleTy())
            return std::any((Value *)m_builder.CreateCall(getRtFunc("rt_to_string_num"), {argRaw}));
    }

    // 0a3. range intrinsic — direct C call with AVX2 streaming stores
    if (funcName == "range" && argCount >= 1 && argCount <= 3)
    {
        Value *arg0 = (argCount >= 2)
                          ? castToNativeDouble(castAnyToValue(visit(exprs[0]), "range_arg0"))
                          : ConstantFP::get(m_doubleTy, 0.0);
        Value *arg1 = (argCount == 1)
                          ? castToNativeDouble(castAnyToValue(visit(exprs[0]), "range_arg1"))
                          : castToNativeDouble(castAnyToValue(visit(exprs[1]), "range_arg1"));
        Value *arg2 = (argCount >= 3)
                          ? castToNativeDouble(castAnyToValue(visit(exprs[2]), "range_arg2"))
                          : ConstantFP::get(m_doubleTy, 1.0);
        Value *rangeRes = m_builder.CreateCall(getRtFunc("rt_fast_range"), {arg0, arg1, arg2});
        return std::any((Value *)rangeRes);
    }

    Function *currentFunc = m_builder.GetInsertBlock()->getParent();
    std::string currentName = currentFunc->getName().str();

    // 剥离版本号和 _worker / _worker_native 后缀，获取真正的函数原始名字
    std::string baseName = currentName;
    if (currentName.size() > 14 && currentName.substr(currentName.size() - 14) == "_worker_native")
    {
        baseName = currentName.substr(0, currentName.size() - 14);
    }
    else if (currentName.size() > 7 && currentName.substr(currentName.size() - 7) == "_worker")
    {
        baseName = currentName.substr(0, currentName.size() - 7);
    }
    size_t lastUnderscore = baseName.find_last_of('_');
    if (lastUnderscore != std::string::npos && lastUnderscore + 1 < baseName.size() && baseName[lastUnderscore + 1] == 'v')
    {
        baseName = baseName.substr(0, lastUnderscore);
    }

    // ==============================================================
    // 2. [直接自递归旁路] (调用自身的 Native Worker，直接传 double，消除一切运行时开销)
    // ==============================================================
    if (funcName == baseName && s_currentNativeWorkerFunc)
    {
        if (s_compilingNativeWorker && s_currentRecursionPeelDepth < 3 && s_currentFunctionBlock)
        {
            if ((int)s_currentFunctionBlock->children.size() <= 15)
            {
                s_currentRecursionPeelDepth++;
                Value *inlinedRes = nullptr;
                bool ok = tryInlineFunction(funcName, exprs, inlinedRes);
                s_currentRecursionPeelDepth--;
                if (ok && inlinedRes)
                {
                    return std::any((Value *)inlinedRes);
                }
            }
        }

        std::vector<Value *> callArgs;
        for (int i = 0; i < argCount; ++i)
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
            s_inTailPosition = oldTail;
            callArgs.push_back(castToNativeDouble(argRaw));
        }
        CallInst *nativeCall = m_builder.CreateCall(s_currentNativeWorkerFunc, callArgs);
        nativeCall->setCallingConv(llvm::CallingConv::Fast);
        return std::any((Value *)nativeCall);
    }

    std::string nativeTargetName = funcName;
    if (g_CurrentInterpreter)
    {
        for (auto scopeIt = g_CurrentInterpreter->scopes.rbegin(); scopeIt != g_CurrentInterpreter->scopes.rend(); ++scopeIt)
        {
            auto it = scopeIt->find(funcName);
            if (it != scopeIt->end() && it->second.type == TzdValue::FUNCTION && !it->second.jitInternalName.empty())
            {
                nativeTargetName = it->second.jitInternalName;
                break;
            }
        }
    }

    Function *directNativeWorker = m_module->getFunction(nativeTargetName + "_worker_native");
    if (!directNativeWorker && nativeTargetName != funcName)
    {
        directNativeWorker = m_module->getFunction(funcName + "_worker_native");
    }
    if (!directNativeWorker)
    {
        std::string nativeWorkerName = nativeTargetName + "_worker_native";
        auto *info = TzdJitEngine::getJittedFunction(nativeWorkerName);
        if (!info && nativeTargetName != funcName)
        {
            nativeWorkerName = funcName + "_worker_native";
            info = TzdJitEngine::getJittedFunction(nativeWorkerName);
        }
        if (info && info->paramCount == argCount)
        {
            std::vector<Type *> nativeArgs(argCount, m_doubleTy);
            directNativeWorker = Function::Create(
                FunctionType::get(m_doubleTy, nativeArgs, false),
                Function::ExternalLinkage,
                nativeWorkerName,
                m_module.get());
            directNativeWorker->setCallingConv(llvm::CallingConv::Fast);
        }
    }
    if (directNativeWorker && (int)directNativeWorker->arg_size() == argCount)
    {
        std::vector<Value *> callArgs;
        for (int i = 0; i < argCount; ++i)
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
            s_inTailPosition = oldTail;
            callArgs.push_back(castToNativeDouble(argRaw));
        }
        CallInst *nativeCall = m_builder.CreateCall(directNativeWorker, callArgs);
        nativeCall->setCallingConv(llvm::CallingConv::Fast);
        return std::any((Value *)nativeCall);
    }

    if (ctx->getStart() && !s_compilingNativeWorker && (s_loopLevel == 0 || jitTrackFrames()))
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }

    // 0b. 成员方法与实例分发极速内联 / Direct Native Worker 优化
    if (auto memberCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(ctx->atom()))
    {
        std::string memberName = memberCtx->IDENTIFIER()->getText();
        std::string objText = memberCtx->atom()->getText();
        std::string targetClass = "";
        Value *receiverVal = nullptr;

        if (TzdClassDef *cls = TzdOopManager::getClass(objText))
        {
            targetClass = objText;
        }
        else if (m_varClassTypes.count(objText))
        {
            targetClass = m_varClassTypes[objText];
            if (s_compilingNativeWorker && (m_varFieldAllocas.count(objText) || objText.find('.') != std::string::npos))
            {
                receiverVal = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
            }
            else
            {
                receiverVal = castAnyToValue(visit(memberCtx->atom()), "visitCallExpr.memberReceiver");
            }
        }
        else if (objText == "this")
        {
            if (m_varClassTypes.count("this"))
                targetClass = m_varClassTypes["this"];
            else if (m_currentClassDef)
                targetClass = m_currentClassDef->simpleName;
            receiverVal = castAnyToValue(visit(memberCtx->atom()), "visitCallExpr.thisReceiver");
        }
        else
        {
            targetClass = deduceExprClassName(memberCtx->atom(), m_varClassTypes, m_currentClassDef);
            if (!targetClass.empty())
            {
                if (s_compilingNativeWorker)
                {
                    receiverVal = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
                }
                else
                {
                    receiverVal = castAnyToValue(visit(memberCtx->atom()), "visitCallExpr.deducedReceiver");
                }
            }
        }

        if (!targetClass.empty())
        {
            // 1. AST Inlining
            Value *inlinedRes = nullptr;
            if (tryInlineFunction(memberName, exprs, inlinedRes, receiverVal, targetClass, objText))
            {
                return std::any((Value *)inlinedRes);
            }
        }
    }

    // 0b2. 隐式 this 成员方法调用 (在类方法内部直接调用同类的方法，例如 b())
    if (m_currentClassDef && m_namedValues.count("this") && dynamic_cast<TzdLangParser::IdExprContext *>(ctx->atom()))
    {
        if (m_currentClassDef->findMethod(funcName) || m_currentClassDef->findField(funcName))
        {
            Value *thisVal = m_builder.CreateLoad(m_ptrTy, m_namedValues["this"], "this_val");
            std::string className = m_currentClassDef->simpleName;

            // 1. AST Inlining
            Value *inlinedRes = nullptr;
            if (tryInlineFunction(funcName, exprs, inlinedRes, thisVal, className, "this"))
            {
                return std::any((Value *)inlinedRes);
            }

            // 3. Dispatch through rt_tzd_call_method
            Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "this_call_args");
            if (argCount > 0)
            {
                m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
            }
            for (int i = 0; i < argCount; ++i)
            {
                bool oldTail = s_inTailPosition;
                s_inTailPosition = false;
                Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
                s_inTailPosition = oldTail;
                Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
                if (argRaw->getType()->isDoubleTy())
                {
                    inlineStoreNativeToPtr(argPtr, argRaw);
                }
                else
                {
                    m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {argPtr, boxToTzdValue(argRaw)});
                }
            }
            Value *nameStr = m_builder.CreateGlobalStringPtr(funcName);
            TzdSelector sel = internSelectorConstant(funcName);
            Value *resPtr = m_builder.CreateCall(getRtFunc("rt_tzd_call_method"),
                                                 {thisVal, m_builder.getInt32((int32_t)sel), nameStr, m_builder.getInt32(argCount), argsArray});
            if (argCount > 0)
            {
                m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
            }
            return std::any((Value *)resPtr);
        }
    }

    // 0c. 普通函数 AST 级别超强小函数内联展开 (消除一切调用开销)
    Value *inlinedResult = nullptr;
    if (tryInlineFunction(funcName, exprs, inlinedResult))
    {
        return std::any((Value *)inlinedResult);
    }

    // ==============================================================
    // 2. [直接自递归旁路] (Boxed Worker 备用路径)
    // ==============================================================
    if (funcName == baseName && s_currentWorkerFunc)
    {
        Value *argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "rec_args");
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
        }
        for (int i = 0; i < argCount; ++i)
        {
            bool oldTail = s_inTailPosition;
            s_inTailPosition = false;
            Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
            s_inTailPosition = oldTail;

            Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
            if (argRaw->getType()->isDoubleTy())
            {
                inlineStoreNativeToPtr(argPtr, argRaw);
            }
            else
            {
                m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {argPtr, boxToTzdValue(argRaw)});
            }
        }
        Value *dummyResSlot = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        Value *interp = (currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
                            ? (Value *)currentFunc->getArg(0)
                            : (Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
        Value *recNameStr = m_builder.CreateGlobalStringPtr(funcName);
        m_builder.CreateCall(getRtFunc("rt_push_jit_frame"), {recNameStr});
        Value *nativeDoubleResult = m_builder.CreateCall(s_currentWorkerFunc, {interp, dummyResSlot, argsArray});
        m_builder.CreateCall(getRtFunc("rt_pop_jit_frame"), {});
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
        }
        return std::any((Value *)nativeDoubleResult);
    }

    // ==============================================================
    // 3. 动态函数调用 / 间接相互递归 / 原生函数系统调用
    // ==============================================================

    Value *argsArray = nullptr;

    // 【核心改进 1】：若处于尾部且为自递归，且参数个数不大于当前函数参数个数，直接复用当前函数的输入参数数组，避免分配新的栈内存
    if (funcName == baseName && s_inTailPosition && s_currentWorkerFunc && argCount <= (int)s_currentFuncParamNames.size())
    {
        argsArray = s_currentWorkerFunc->getArg(2);
    }
    else
    {
        argsArray = CreateEntryBlockAlloca(m_tzdValueTy, m_builder.getInt32(argCount > 0 ? argCount : 1), "call_args");
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_init_tzd_value"), {argsArray, m_builder.getInt32(argCount)});
        }
    }

    // 提前计算所有的参数值，防止原地覆盖数组时发生数据干扰
    std::vector<Value *> evaluatedArgs;
    evaluatedArgs.reserve(argCount);
    for (int i = 0; i < argCount; ++i)
    {
        bool oldTail = s_inTailPosition;
        s_inTailPosition = false;
        Value *argRaw = std::any_cast<Value *>(visit(exprs[i]));
        s_inTailPosition = oldTail;
        evaluatedArgs.push_back(argRaw);
    }

    for (int i = 0; i < argCount; ++i)
    {
        Value *argRaw = evaluatedArgs[i];
        Value *argPtr = m_builder.CreateGEP(m_tzdValueTy, argsArray, m_builder.getInt32(i));
        if (argRaw->getType()->isDoubleTy())
        {
            inlineStoreNativeToPtr(argPtr, argRaw);
        }
        else
        {
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {argPtr, boxToTzdValue(argRaw)});
        }
    }

    if (!dynamic_cast<TzdLangParser::IdExprContext *>(ctx->atom()))
    {
        if (auto memberCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext *>(ctx->atom()))
        {
            Value *obj = castAnyToValue(visit(memberCtx->atom()), "visitCallExpr.memberObj");
            std::string memberName = memberCtx->IDENTIFIER()->getText();
            Value *nameStr = m_builder.CreateGlobalStringPtr(memberName);
            TzdSelector sel = internSelectorConstant(memberName);
            Value *objPtr = (obj && obj->getType()->isDoubleTy()) ? boxToTzdValue(obj) : obj;
            Value *resPtr = m_builder.CreateCall(getRtFunc("rt_tzd_call_method"),
                                                 {objPtr, m_builder.getInt32((int32_t)sel), nameStr, m_builder.getInt32(argCount), argsArray});
            if (argCount > 0)
            {
                m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
            }
            return std::any((Value *)resPtr);
        }
        Value *callee = castAnyToValue(visit(ctx->atom()), "visitCallExpr.callee");
        Value *calleePtr = (callee && callee->getType()->isDoubleTy()) ? boxToTzdValue(callee) : callee;
        Value *resPtr = m_builder.CreateCall(getRtFunc("rt_call_value_fast"), {calleePtr, m_builder.getInt32(argCount), argsArray});
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
        }
        return std::any((Value *)resPtr);
    }

    // If callee is a local variable or parameter holding a function/callback object (e.g. op(val)),
    // invoke it directly via rt_call_value_fast instead of resolving as a global function name.
    if (m_namedValues.count(funcName))
    {
        Value *callee = m_builder.CreateLoad(m_ptrTy, m_namedValues[funcName], funcName + "_callee");
        Value *resPtr = m_builder.CreateCall(getRtFunc("rt_call_value_fast"), {callee, m_builder.getInt32(argCount), argsArray});
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
        }
        return std::any((Value *)resPtr);
    }

    // ==============================================================
    // [终极交叉递归优化]：动态解析目标函数的 Worker 指针
    // ==============================================================
    Value *funcNameStr = m_builder.CreateGlobalStringPtr(funcName);
    PointerType *ptrTy = cast<PointerType>(m_ptrTy);
    Value *workerPtr = m_builder.CreateCall(getRtFunc("rt_get_worker_ptr"), {funcNameStr});
    Value *isWorkerValid = m_builder.CreateICmpNE(workerPtr, ConstantPointerNull::get(ptrTy));

    BasicBlock *fastCallBB = BasicBlock::Create(m_context, "fast_worker_call", currentFunc);
    BasicBlock *slowCallBB = BasicBlock::Create(m_context, "slow_interp_call", currentFunc);

    bool canTailCall = s_inTailPosition && s_inlineReturnStack.empty();
    if (canTailCall)
    {
        m_builder.CreateCondBr(isWorkerValid, fastCallBB, slowCallBB);

        // -- Fast Path: 跨函数的纯 LLVM 尾调用 --
        m_builder.SetInsertPoint(fastCallBB);

        // 【新增】：因为是尾调用优化（TCO），旧的栈帧已经被覆盖，所以这里替换当前的栈帧记录
        m_builder.CreateCall(getRtFunc("rt_replace_jit_frame"), {funcNameStr});

        std::vector<Type *> workerSignature = {m_ptrTy, m_ptrTy, m_ptrTy};
        FunctionType *workerFTy = FunctionType::get(m_doubleTy, workerSignature, false);
        Value *interp = (currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
                            ? (Value *)currentFunc->getArg(0)
                            : (Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy));

        CallInst *fastRes = m_builder.CreateCall(workerFTy, workerPtr, {interp, m_currentRetPtr, argsArray});
        fastRes->setTailCallKind(CallInst::TCK_Tail); // Use Tail (not MustTail) for safety
        if (currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {currentFunc->getArg(0)});
        }
        if (currentFunc->getReturnType()->isDoubleTy())
        {
            m_builder.CreateRet(fastRes);
        }
        else
        {
            m_builder.CreateRetVoid();
        }

        // -- Slow Path: 回退到普通 C++ 函数 --
        m_builder.SetInsertPoint(slowCallBB);
        Value *slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), {funcNameStr, m_builder.getInt32(argCount), argsArray});
        if (m_currentRetPtr && !isa<ConstantPointerNull>(m_currentRetPtr))
        {
            m_builder.CreateCall(getRtFunc("rt_write_fast_ret"), {m_currentRetPtr, slowResPtr});
        }
        Value *slowRes = inlineToDoubleFast(slowResPtr);
        if (argCount > 0)
        {
            m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
        }
        if (currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
        {
            m_builder.CreateCall(getRtFunc("rt_pop_call_depth"), {currentFunc->getArg(0)});
        }
        if (currentFunc->getReturnType()->isDoubleTy())
        {
            m_builder.CreateRet(slowRes);
        }
        else
        {
            m_builder.CreateRetVoid();
        }

        BasicBlock *deadBB = BasicBlock::Create(m_context, "tail_call_unreachable", currentFunc);
        m_builder.SetInsertPoint(deadBB);

        return std::any((Value *)ConstantFP::get(m_doubleTy, 0.0));
    }

    // --- 非尾调用位置，保持原有的 merge 逻辑 ---
    BasicBlock *mergeBB = BasicBlock::Create(m_context, "merge_call", currentFunc);
    m_builder.CreateCondBr(isWorkerValid, fastCallBB, slowCallBB);

    // Use pointer slot — both paths return TzdValue* (preserves strings/objects)
    AllocaInst *resPtrSlot = CreateEntryBlockAlloca(m_ptrTy, nullptr, "dyn_call_res");

    m_builder.SetInsertPoint(fastCallBB);

    // Direct worker call — push JIT frame
    m_builder.CreateCall(getRtFunc("rt_push_jit_frame"), {funcNameStr});

    std::vector<Type *> workerSignature = {m_ptrTy, m_ptrTy, m_ptrTy};
    FunctionType *workerFTy = FunctionType::get(m_doubleTy, workerSignature, false);
    Value *interp = (currentFunc->arg_size() > 0 && currentFunc->getArg(0)->getType() == m_ptrTy)
                        ? (Value *)currentFunc->getArg(0)
                        : (Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
    Value *resSlotFast = m_builder.CreateCall(getRtFunc("rt_create_null"), {});
    m_builder.CreateCall(workerFTy, workerPtr, {interp, resSlotFast, argsArray});
    m_builder.CreateStore(resSlotFast, resPtrSlot);

    // Pop JIT frame after worker returns
    m_builder.CreateCall(getRtFunc("rt_pop_jit_frame"), {});

    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(slowCallBB);
    Value *slowResPtr = m_builder.CreateCall(getRtFunc("rt_call_sub_fast"), {funcNameStr, m_builder.getInt32(argCount), argsArray});
    // Keep TzdValue* directly — preserves strings/objects from interpreter
    m_builder.CreateStore(slowResPtr, resPtrSlot);
    m_builder.CreateBr(mergeBB);

    m_builder.SetInsertPoint(mergeBB);
    if (argCount > 0)
    {
        m_builder.CreateCall(getRtFunc("rt_destruct_values"), {argsArray, m_builder.getInt32(argCount)});
    }
    Value *finalRes = m_builder.CreateLoad(m_ptrTy, resPtrSlot, "call_res");
    return std::any((Value *)finalRes);
}

std::any TzdCompiler::visitThrowStmt(TzdLangParser::ThrowStmtContext *ctx)
{
    if (ctx->getStart() && !s_compilingNativeWorker)
    {
        m_builder.CreateCall(getRtFunc("rt_set_location"), {m_builder.getInt32(ctx->getStart()->getLine()),
                                                            m_builder.getInt32(ctx->getStart()->getCharPositionInLine())});
    }
    Value *errVal = boxToTzdValue(std::any_cast<Value *>(visit(ctx->expression())));
    m_builder.CreateCall(getRtFunc("rt_throw"), {errVal});

    Function *currentFunc = m_builder.GetInsertBlock()->getParent();
    if (currentFunc->getReturnType()->isDoubleTy())
    {
        m_builder.CreateRet(ConstantFP::get(m_doubleTy, 0.0));
    }
    else
    {
        m_builder.CreateRetVoid();
    }

    BasicBlock *deadBB = BasicBlock::Create(m_context, "unreachable_after_throw", currentFunc);
    m_builder.SetInsertPoint(deadBB);

    return std::any((Value *)nullptr);
}

std::any TzdCompiler::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext *ctx)
{
    Function *func = m_builder.GetInsertBlock()->getParent();
    BasicBlock *tryBB = BasicBlock::Create(m_context, "try.body", func);
    BasicBlock *catchBB = BasicBlock::Create(m_context, "try.catch", func);
    BasicBlock *afterBB = BasicBlock::Create(m_context, "try.after", func);

    Value *jmpBuf = m_builder.CreateCall(getRtFunc("rt_alloc_jmp_buf"));
    Value *oldJmpBuf = m_builder.CreateCall(getRtFunc("rt_get_catch_jmp"));
    m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {jmpBuf});

    Value *enterRes = nullptr;
#ifdef _WIN32
    Function *sjF = m_module->getFunction("_setjmp");
    Value *nullFrame = ConstantPointerNull::get(cast<PointerType>(m_ptrTy));
    enterRes = m_builder.CreateCall(sjF, {jmpBuf, nullFrame}); // 直接内联调用原生的 _setjmp!
#else
    Function *sjF = m_module->getFunction("setjmp");
    enterRes = m_builder.CreateCall(sjF, {jmpBuf});
#endif

    Value *stackDepth = m_builder.CreateCall(getRtFunc("rt_get_call_stack_depth"));

    Value *isCatch = m_builder.CreateICmpNE(enterRes, m_builder.getInt32(0));
    m_builder.CreateCondBr(isCatch, catchBB, tryBB);

    m_builder.SetInsertPoint(tryBB);
    s_tryJmpBufStack.push_back({jmpBuf, oldJmpBuf, s_loopLevel});
    visit(ctx->block(0));
    s_tryJmpBufStack.pop_back();

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {oldJmpBuf});
        m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), {jmpBuf});
        m_builder.CreateBr(afterBB);
    }

    m_builder.SetInsertPoint(catchBB);
    m_builder.CreateCall(getRtFunc("rt_restore_call_stack_depth"), {stackDepth});
    m_builder.CreateCall(getRtFunc("rt_set_catch_jmp"), {oldJmpBuf});
    m_builder.CreateCall(getRtFunc("rt_free_jmp_buf"), {jmpBuf}); // 避免异常再次发生时泄露

    Value *thrown = m_builder.CreateCall(getRtFunc("rt_get_thrown"));
    std::string errName = ctx->IDENTIFIER()->getText();

    // 自动利用 JIT 的 Alloca 分配捕获的变量对象环境，完全抛弃缓慢的 Runtime Scope !
    AllocaInst *alloc = CreateEntryBlockAlloca(m_ptrTy, nullptr, errName + "_catch");
    m_builder.CreateStore(thrown, alloc);

    auto backupNamedVals = m_namedValues;
    m_namedValues[errName] = alloc;

    visit(ctx->block(1));

    m_namedValues = backupNamedVals;

    if (!m_builder.GetInsertBlock()->getTerminator())
    {
        m_builder.CreateBr(afterBB);
    }

    m_builder.SetInsertPoint(afterBB);
    return std::any((Value *)ConstantPointerNull::get(cast<PointerType>(m_ptrTy)));
}

std::any TzdCompiler::visitNullExpr(TzdLangParser::NullExprContext *ctx)
{
    return (Value *)m_builder.CreateCall(getRtFunc("rt_create_null"));
}
std::any TzdCompiler::visitExprStmt(TzdLangParser::ExprStmtContext *ctx)
{
    return visit(ctx->expression());
}
std::any TzdCompiler::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext *ctx)
{
    // Must save/restore JIT compilation context — just like visitLambdaExpr.
    // Without this, compileNamedFunction for the nested function clobbers
    // m_currentRetPtr, s_currentWorkerFunc, s_tailRecurseBB, etc.,
    // causing null operands in the parent function's IR.
    auto *savedInsertBlock = m_builder.GetInsertBlock();
    auto savedNamedValues = m_namedValues;
    auto savedNativeLocals = m_nativeDoubleLocals;
    auto savedParamNames = s_currentFuncParamNames;
    auto *savedTailBB = s_tailRecurseBB;
    auto *savedWorkerFunc = s_currentWorkerFunc;
    auto *savedNativeWorkerFunc = s_currentNativeWorkerFunc;
    bool savedCompilingNative = s_compilingNativeWorker;
    auto *savedRetPtr = m_currentRetPtr;
    bool savedTailState = s_inTailPosition;

    auto result = visit(ctx->functionDeclaration());

    // Register nested functions with the interpreter so they can be found
    // by rt_call_sub_fast and rt_get_worker_ptr at runtime.
    if (g_CurrentInterpreter)
    {
        std::string funcName = ctx->functionDeclaration()->IDENTIFIER()->getText();
        g_CurrentInterpreter->m_pendingJitFunctions.insert(funcName);
    }

    m_builder.SetInsertPoint(savedInsertBlock);
    m_namedValues = savedNamedValues;
    m_nativeDoubleLocals = savedNativeLocals;
    s_currentFuncParamNames = savedParamNames;
    s_tailRecurseBB = savedTailBB;
    s_currentWorkerFunc = savedWorkerFunc;
    s_currentNativeWorkerFunc = savedNativeWorkerFunc;
    s_compilingNativeWorker = savedCompilingNative;
    m_currentRetPtr = savedRetPtr;
    s_inTailPosition = savedTailState;

    return result;
}

Value *TzdCompiler::castToNativeDouble(Value *val)
{
    if (!val)
        return ConstantFP::get(m_doubleTy, 0.0);

    // 1. 如果已经是 double，直接用
    if (val->getType()->isDoubleTy())
        return val;

    // 2. [关键]：如果是 i1 (布尔值)，直接用 LLVM 指令转为 double (0.0 或 1.0)
    // 绝对不能把 i1 当作指针传给 rt_to_double_fast
    if (val->getType()->isIntegerTy(1))
    {
        return m_builder.CreateUIToFP(val, m_doubleTy, "bool2double");
    }
    if (val->getType()->isIntegerTy())
    {
        return m_builder.CreateSIToFP(val, m_doubleTy, "int2double");
    }

    // 3. 只有当它是 TzdValue* 指针时，inline GEP+Load dVal
    return inlineToDoubleFast(val);
}

Value *TzdCompiler::castToNativeI64(Value *val)
{
    if (!val)
        return m_builder.getInt64(0);
    if (val->getType()->isIntegerTy(64))
        return val;
    if (val->getType()->isIntegerTy(1))
        return m_builder.CreateZExt(val, m_builder.getInt64Ty(), "bool2i64");
    if (val->getType()->isIntegerTy())
        return m_builder.CreateSExt(val, m_builder.getInt64Ty(), "int2i64");
    if (val->getType()->isDoubleTy())
        return m_builder.CreateFPToSI(val, m_builder.getInt64Ty(), "dbl2i64");

    return m_builder.CreateCall(getRtFunc("rt_to_int64_fast"), {val}, "to_i64");
}

// Inline rt_store_native_to_ptr: directly write type=DOUBLE and dVal=val via GEP
// Eliminates a C function call per argument store in recursive calls
void TzdCompiler::inlineStoreNativeToPtr(Value *dest, Value *nativeDouble)
{
    // Cast to i8* for byte-offset GEP
    Value *rawPtr = m_builder.CreateBitCast(dest,
                                            llvm::PointerType::get(llvm::Type::getInt8Ty(m_context), 0));
    // Write type = DOUBLE (bitcast i8* to i32* for correct store type)
    Value *typePtr = m_builder.CreateConstGEP1_32(
        llvm::Type::getInt8Ty(m_context), rawPtr, (uint32_t)TZD_TYPE_OFFSET);
    Value *typeTypedPtr = m_builder.CreateBitCast(typePtr,
                                                  llvm::PointerType::get(m_int32Ty, 0));
    m_builder.CreateStore(m_builder.getInt32((uint32_t)TzdValue::DOUBLE), typeTypedPtr);
    // Write dVal = nativeDouble
    Value *dValPtr = m_builder.CreateConstGEP1_32(
        llvm::Type::getInt8Ty(m_context), rawPtr, (uint32_t)TZD_DVAL_OFFSET);
    Value *dValTypedPtr = m_builder.CreateBitCast(dValPtr,
                                                  llvm::PointerType::get(m_doubleTy, 0));
    m_builder.CreateStore(nativeDouble, dValTypedPtr);
}

// Inline rt_to_double_fast: just calls rt_to_double_fast.
// Full inlining (GEP+Load dVal) is unsafe for non-DOUBLE types (INT stores in lVal).
// Phase B (native worker specialization) will eliminate this call entirely for self-recursion.
Value *TzdCompiler::inlineToDoubleFast(Value *src)
{
    if (!src)
        return ConstantFP::get(m_doubleTy, 0.0);
    if (src->getType()->isDoubleTy())
        return src;
    if (src->getType()->isIntegerTy(1))
        return m_builder.CreateUIToFP(src, m_doubleTy, "bool2double");
    if (src->getType()->isIntegerTy())
        return m_builder.CreateSIToFP(src, m_doubleTy, "int2double");
    return m_builder.CreateCall(getRtFunc("rt_to_double_fast"), {src}, "to_double");
}

llvm::Value *TzdCompiler::boxDouble(llvm::Value *nativeVal)
{
    if (nativeVal->getType()->isPointerTy())
        return nativeVal;
    return m_builder.CreateCall(getRtFunc("rt_create_num"), {nativeVal});
}