/* ============================================================================
 *  TzdStrings.h - TzdTools 字符串资源快速访问头文件
 *
 *  功能: 集中管理项目中所有错误提示、输出信息和通用字符串常量。
 *        通过 constexpr 常量实现零开销访问，同时兼容 printf 格式化模板。
 *
 *  用法: #include "Res/TzdStrings.h"
 *
 *  分类说明:
 *    TzdErr_*     -> 运行时错误/系统错误模板
 *    TzdCmd_*     -> 命令系统的提示/错误信息
 *    TzdPdb_*     -> PDB 读取器相关的消息
 *    TzdStk_*     -> 堆栈跟踪模块的消息
 *    TzdRte_*     -> 解释器运行时错误
 *    TzdMth_*     -> 数学/本机模块的错误
 *    TzdSys_*     -> 系统信息/状态字符串
 *    TzdGen_*     -> 通用/工具字符串
 *
 *  作者: tzdwindows 7
 *  创建: 2026-06-06
 * ============================================================================
 */
#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <filesystem>

// ============================================================================
//  1. 运行时错误 (Runtime Error) 模板
// ============================================================================
namespace TzdErr {
    constexpr const char* HEADER_LINE   = "==================================================";
    constexpr const char* SEPARATOR_LINE = "--------------------------------------------------";
    constexpr const char* TITLE         = "[Tzd \u8fd0\u884c\u65f6\u9519\u8bef] \u884c %zu:%zu";
    constexpr const char* DETAIL        = "[\u9519\u8bef\u8be6\u60c5] %s";
    constexpr const char* POINTER       = "^--- \u8fd9\u91cc";
    constexpr const char* SYSTEM        = "[Tzd \u7cfb\u7edf\u9519\u8bef] %s";
    constexpr const char* RUNTIME       = "[Tzd \u8fd0\u884c\u65f6\u9519\u8bef] %s";
    constexpr const char* EXECUTION     = "[Tzd \u6267\u884c\u5f02\u5e38] %s";
    constexpr const char* SYNTAX_HEADER = "Exception in thread \"main\" %s: %s";
    constexpr const char* SYNTAX_MISMATCH  = "\u8f93\u5165\u7b26\u53f7\u4e0d\u5339\u914d (\u8bed\u6cd5\u9519\u8bef)";
    constexpr const char* SYNTAX_EXTRANEOUS = "\u53d1\u73b0\u591a\u4f59\u7684\u8f93\u5165\u5b57\u7b26";
    constexpr const char* SYNTAX_MISSING = "\u7f3a\u5c11";
    constexpr const char* SYNTAX_NOVIABLE = "\u65e0\u6cd5\u8bc6\u522b\u7684\u8bed\u6cd5\u7ed3\u6784";

    constexpr const char* SYNTAX_ERROR_MSG = "Tzd \\u8bed\\u6cd5\\u9519\\u8bef";
} // namespace TzdErr

// ============================================================================
//  2. 命令系统 (Command System) 提示/错误
// ============================================================================
namespace TzdCmd {
    constexpr const char* USAGE_DEMANGLE  = "[Tzd] \u7528\u6cd5: Demangle <\u4fee\u9970\u540d\u5b57\u7b26\u4e32>";
    constexpr const char* USAGE_PDBINFO   = "[Tzd] \u7528\u6cd5: PdbInfo <Pdb\u8def\u5f84>";
    constexpr const char* USAGE_SCANFUNC  = "[Tzd] \u7528\u6cd5: ScanFunc <PID|\u8fdb\u7a0b\u540d> [-m \u6a21\u5757\u540d] [-p PDB\u8def\u5f84] [-g (\u5f00\u542fGUI)]";
    constexpr const char* USAGE_MEMORYASM = "[Tzd] \u7528\u6cd5: MemoryAsm <PID|Name>;";
    constexpr const char* USAGE_STACKTRACE = "[Tzd] \u9519\u8bef: \u8bf7\u63d0\u4f9b PID \u6216\u8fdb\u7a0b\u540d\u3002\u7528\u6cd5: StackTrace <Target>;";

    constexpr const char* PROCESS_NOT_FOUND      = "[Tzd] \u9519\u8bef: \u627e\u4e0d\u5230\u76ee\u6807\u8fdb\u7a0b: %s";
    constexpr const char* PROCESS_NOT_FOUND_SIMPLE = "[Tzd] \u9519\u8bef: \u627e\u4e0d\u5230\u76ee\u6807\u8fdb\u7a0b\u3002";
    constexpr const char* PROCESS_NOT_FOUND_NAMED = "[Tzd] \u9519\u8bef: \u627e\u4e0d\u5230\u540d\u4e3a '%s' \u7684\u8fdb\u7a0b\u3002";
    constexpr const char* PROCESS_FOUND          = "[Tzd] \u5df2\u627e\u5230\u8fdb\u7a0b %s\uff0c\u5bf9\u5e94 PID: %lu";

    constexpr const char* HELP_NOT_FOUND = "\u672a\u627e\u5230\u547d\u4ee4 '%s' \u7684\u5e2e\u52a9\u4fe1\u606f\u3002";
    constexpr const char* REDIRECT_SYNTAX = "[Tzd \u8bed\u6cd5\u9519\u8bef] \u91cd\u5b9a\u5411\u7b26\u53f7 '=>' \u540e\u7f3a\u5c11\u6587\u4ef6\u540d\u3002";
    constexpr const char* FILE_CREATE_FAILED = "[Tzd \u9519\u8bef] \u65e0\u6cd5\u521b\u5efa\u8f93\u51fa\u6587\u4ef6";
    constexpr const char* OUTPUT_SAVED    = "[Tzd] \u8f93\u51fa\u5df2\u4fdd\u5b58\u81f3: %s";
    constexpr const char* WORK_DIR_WARN   = "[Tzd] \u8b66\u544a: \u65e0\u6cd5\u8bbe\u7f6e\u5de5\u4f5c\u76ee\u5f55: %s";

    constexpr const char* SCANNING    = "[Tzd] \u6b63\u5728\u626b\u63cf\u8fdb\u7a0b: %s (%lu)";
    constexpr const char* FILTER_MOD  = "[Tzd] \u8fc7\u6ee4\u6a21\u5757: %s";
    constexpr const char* SYMBOL_FILE = "[Tzd] \u7b26\u53f7\u6587\u4ef6: %s";
    constexpr const char* GUI_MODE    = "[Tzd] \u6a21\u5f0f: GUI \u89c6\u56fe";
    constexpr const char* NO_RESULTS  = "[Tzd] \u8b66\u544a: \u672a\u626b\u63cf\u5230\u4efb\u4f55\u7b26\u5408\u7279\u5f81\u7684\u51fd\u6570\uff0cGUI \u672a\u542f\u52a8\u3002";
    constexpr const char* COMPLETE    = "[Tzd] \u4efb\u52a1\u5df2\u63d0\u4ea4\uff0c\u626b\u63cf\u5b8c\u6210\u3002";

    // Demangle 输出标签
    constexpr const char* RAW_LABEL    = "[\u539f\u59cb] ";
    constexpr const char* RESULT_LABEL = "[\u7ed3\u679c] ";

    constexpr const char* META_STACKTRACE_NAME = "StackTrace";
    constexpr const char* META_STACKTRACE_CHNAME = "\u8fdb\u7a0b\u5806\u6808\u8dfb\u8e2a";
    constexpr const char* META_STACKTRACE_FMT = "StackTrace <PID|\u8fdb\u7a0b\u540d>";
    constexpr const char* META_STACKTRACE_DESC = "\u8f93\u51fa\u8c03\u7528\u6808\u3002\u652f\u6301\u4e0d\u533a\u5206\u5927\u5c0f\u5199\u7684\u8fdb\u7a0b\u540d\u67e5\u627e\u3002";
    constexpr const char* META_STACKTRACE_EXP = "\u793a\u4f8b: StackTrace qq.exe;";

    constexpr const char* META_MEMORYASM_NAME = "MemoryAsm";
    constexpr const char* META_MEMORYASM_CHNAME = "\u5185\u5b58\u53cd\u6c47\u7f16\u5206\u6790";
    constexpr const char* META_MEMORYASM_FMT = "MemoryAsm <PID|ProcessName>";
    constexpr const char* META_MEMORYASM_DESC = "\u626b\u63cf\u76ee\u6807\u8fdb\u7a0b\u7684\u6240\u6709\u53ef\u6267\u884c\u4ee3\u7801\u6bb5\u5e76\u756b\u8bd1\u4e3a\u6c47\u7f16\u6307\u4ee4\u3002";
    constexpr const char* META_MEMORYASM_EXP = "\u5371\u9669\u64cd\u4f5c\uff1a\u53ef\u80fd\u5bfc\u81f4\u76ee\u6807\u8fdb\u7a0b\u77ed\u6682\u5361\u987f\u3002";

    constexpr const char* META_SCANFUNC_NAME = "ScanFunc";
    constexpr const char* META_SCANFUNC_CHNAME = "\u626b\u63cf\u8fdb\u7a0b\u51fd\u6570(\u652f\u6301PDB\u4e0eGUI)";
    constexpr const char* META_SCANFUNC_FMT = "ScanFunc <PID|Name> [-m Module] [-p PdbPath] [-g]";
    constexpr const char* META_SCANFUNC_DESC = "\u901a\u8fc7\u7279\u5f81\u7801\u5b9a\u4f4d\u51fd\u6570\u5e76\u8fdb\u884c\u7b2f\u53f7\u8fd8\u539f\u3002\n  -m: \u6307\u5b9a\u76ee\u6807\u6a21\u5757\u540d (\u5982: UnityPlayer.dll)\n  -p: \u6307\u5b9a\u5916\u90e8 PDB \u7b2f\u53f7\u8def\u5f84\n  -g: \u5f00\u542f DirectX 11 GUI \u4ea4\u4e92\u754c\u9762 (\u652f\u6301\u641c\u7d22/\u8fc7\u6ee4)";
    constexpr const char* META_SCANFUNC_EXP = "\u793a\u4f8b: ScanFunc notepad.exe -m notepad.exe -g";

    constexpr const char* META_DEMANGLE_NAME = "Demangle";
    constexpr const char* META_DEMANGLE_CHNAME = "C++\u7b2f\u53f7\u53bb\u4fee\u9970";
    constexpr const char* META_DEMANGLE_FMT = "Demangle <MangledName>";
    constexpr const char* META_DEMANGLE_DESC = "\u5c06 MSVC \u7f16\u8bd1\u5668\u7684\u4fee\u9970\u540d\u8f6c\u6362\u4e3a\u53ef\u8bfb\u7684\u51fd\u6570\u7b7e\u540d\u3002";
    constexpr const char* META_DEMANGLE_EXP = "\u793a\u4f8b: Demangle ?init@System@@QAEXXZ";

    constexpr const char* META_PDBINFO_NAME = "PdbInfo";
    constexpr const char* META_PDBINFO_CHNAME = "PDB\u6587\u4ef6\u5206\u6790\u5de5\u5177";
    constexpr const char* META_PDBINFO_FMT = "PdbInfo <PdbPath>";
    constexpr const char* META_PDBINFO_DESC = "\u8bfb\u53d6 PDB \u6587\u4ef6\u5934\u4fe1\u606f\u5e76\u5c1d\u8bd5\u63d0\u53d6\u524d 10 \u4e2a\u516c\u5f00\u7b2f\u53f7\u3002";
    constexpr const char* META_PDBINFO_EXP = "\u793a\u4f8b: PdbInfo C:\\Symbols\\game.pdb";

    constexpr const char* META_RUN_NAME = "Run";
    constexpr const char* META_RUN_CHNAME = "\u8fd0\u884c\u811a\u672c";
    constexpr const char* META_RUN_FMT = "Run <Code|FilePath>";
    constexpr const char* META_RUN_DESC = "\u89e3\u6790\u5e76\u8fd0\u884c TzdLang \u811a\u672c\u4ee3\u7801\u3002";
    constexpr const char* META_RUN_EXP = "\u793a\u4f8b: Run \"a = gxxx 100; b = a ^ 2;\"";
} // namespace TzdCmd

// ============================================================================
//  3. PDB 读取器 (PdbReader) 消息
// ============================================================================
namespace TzdPdb {
    constexpr const char* FILE_OPEN_FAILED    = "[PdbReader] \u9519\u8bef: \u65e0\u6cd5\u6253\u5f00\u6587\u4ef6";
    constexpr const char* SIGNATURE_MISMATCH  = "[PdbReader] \u9519\u8bef: \u6587\u4ef6\u7b7e\u540d\u4e0d\u5339\u914d";
    constexpr const char* NO_VALID_STREAMS    = "[PdbReader] \u9519\u8bef: \u6ca1\u6709\u627e\u5230\u6709\u6548\u7684\u7b26\u53f7\u6d41\u7d22\u5f15\u3002";
    constexpr const char* STREAM_WARNING      = "[PdbReader] \u8b66\u544a: \u626b\u63cf\u4e86\u6d41 ";
    constexpr const char* PARSE_SUCCESS_FMT   = "[PdbReader] \u89e3\u6790\u6210\u529f: \u627e\u5230 %d \u4e2a\u7b26\u53f7\u3002";

    constexpr const char* READING     = "[Tzd] \u6b63\u5728\u8bfb\u53d6 PDB: %s ...";
    constexpr const char* INVALID     = "[Tzd] \u9519\u8bef: PDB \u6587\u4ef6\u65e0\u6548\u6216\u65e0\u6cd5\u6253\u5f00\u3002";
    constexpr const char* VALID       = "[Tzd] PDB \u6587\u4ef6\u6821\u9a8c\u901a\u8fc7 (MSF \u683c\u5f0f\u6709\u6548)\u3002";
    constexpr const char* SYMBOLS_OK  = "[Tzd] \u6210\u529f\u89e3\u6790\u7b26\u53f7\u8868\uff0c\u5171\u627e\u5230 %zu \u4e2a\u516c\u5f00\u7b26\u53f7\u3002";
    constexpr const char* STREAM_FAIL = "[Tzd] \u8b66\u544a: \u7b26\u53f7\u6d41\u89e3\u6790\u5931\u8d25\u3002";
} // namespace TzdPdb

// ============================================================================
//  4. 堆栈跟踪 (StackTrace) 消息
// ============================================================================
namespace TzdStk {
    constexpr const char* ANALYZING      = ">> \u6b63\u5728\u5206\u6790\u8fdb\u7a0b PID: %lu \u7684\u8c03\u7528\u6808...";
    constexpr const char* OPEN_FAILED    = "[Error] \u65e0\u6cd5\u6253\u5f00\u8fdb\u7a0b\uff0c\u8bf7\u68c0\u67e5\u6743\u9650 (\u9700\u7ba1\u7406\u5458\u6743\u9650)\u3002";
    constexpr const char* THREAD_LABEL   = "\n--- \u7ebf\u7a0b ID: %lu ---";
    constexpr const char* UNKNOWN_SYMBOL = " (\u672a\u77e5\u7b26\u53f7)";
    constexpr const char* SYMBOL_FMT     = "  [0x%llx] %s + 0x%llx";
    constexpr const char* MODULE_FMT     = "  [0x%llx] %s + 0x%llx";
    constexpr const char* UNKNOWN_FMT    = "  [0x%llx] (\u672a\u77e5\u7b26\u53f7)";
} // namespace TzdStk

// ============================================================================
//  5. 解释器运行时错误 (Interpreter / Runtime) 消息
// ============================================================================
namespace TzdRte {
    constexpr const char* SYMBOL_REDEFINED   = "\u7b26\u53f7\u91cd\u5b9a\u4e49: '%s' \u5df2\u5728\u5f53\u524d\u4f5c\u7528\u57df\u5b9a\u4e49";
    constexpr const char* SYMBOL_CONFLICT    = "\u7b26\u53f7\u51b2\u7a81: '%s' \u5df2\u88ab\u5b9a\u4e49\u4e3a\u7c7b\u3001\u679a\u4e3e\u6216\u6ce8\u89e3";
    constexpr const char* PARAM_MISMATCH     = "\u51fd\u6570\u8c03\u7528\u53c2\u6570\u4e0d\u5339\u914d: %s \u9700\u8981 %zu \u4e2a";
    constexpr const char* NOT_CALLABLE       = "\u5c1d\u8bd5\u8c03\u7528\u4e00\u4e2a\u975e\u51fd\u6570\u7c7b\u578b\u5bf9\u8c61 (\u5b9e\u9645\u7c7b\u578b: %s)";
    constexpr const char* CLASS_NOT_FOUND    = "\u627e\u4e0d\u5230\u7c7b\u5b9a\u4e49: %s";
    constexpr const char* CTOR_NOT_FOUND     = "\u627e\u4e0d\u5230\u5339\u914d\u7684\u6784\u9020\u51fd\u6570: '%s' \u9700\u8981 %zu \u4e2a\u53c2\u6570";
    constexpr const char* SYMBOL_UNRESOLVABLE = "\u65e0\u6cd5\u89e3\u6790\u7b26\u53f7: %s";
    constexpr const char* SUPER_OUTSIDE_CLASS  = "super() \u5fc5\u987b\u5728\u7c7b\u5b9a\u4e49\u7684\u5185\u90e8\u4f7f\u7528";
    constexpr const char* SUPER_NO_PARENT      = "\u7c7b '%s' \u6ca1\u6709\u7236\u7c7b\uff0c\u65e0\u6cd5\u8c03\u7528 super()";
    constexpr const char* PARENT_NOT_FOUND     = "\u627e\u4e0d\u5230\u7236\u7c7b\u5b9a\u4e49: %s";
    constexpr const char* SUPER_LOST_THIS      = "super() \u8c03\u7528\u4e0a\u4e0b\u6587\u4e22\u5931 'this' \u6307\u9488";
    constexpr const char* PARENT_NO_CTOR_ARGS  = "\u7236\u7c7b '%s' \u65e0\u6784\u9020\u51fd\u6570\uff0c\u4f46 super() \u4f20\u5165\u4e86\u53c2\u6570";
    constexpr const char* SUPER_CTOR_NOT_FOUND = "super() \u627e\u4e0d\u5230\u5339\u914d\u7684\u7236\u7c7b\u6784\u9020\u51fd\u6570";
    constexpr const char* NEW_MISSING_CLASS    = "New \u8868\u8fbe\u5f0f\u7f3a\u5931\u7c7b\u540d";
    constexpr const char* JIT_COMPILE_FAILED   = "[Error] Failed to extract ThreadSafeModule in compileScriptToMemory";

    // 格式化的源路径短名称（使用 formatSourcePath 风格）
    static inline std::string formatSourcePath(const std::string& fullPath) {
        if (fullPath.empty() || fullPath == "memory") return "memory";
        std::string path = fullPath;
        std::replace(path.begin(), path.end(), '/', '\\');
        size_t pos = path.find("\\stdlib\\");
        if (pos == std::string::npos && path.rfind("stdlib\\", 0) == 0) pos = 0;
        return (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }

    constexpr const char* UNDEFINED_IDENTIFIER_PREFIX = "\u672a\u5b9a\u4e49\u7684\u6807\u8bc6\u7b2f: '";
    constexpr const char* QUOTE_SUFFIX = "'";
    constexpr const char* CLASS_PREFIX = "\u7c7b '";
    constexpr const char* CLASS_NO_STATIC_MEMBER = "' \u4e2d\u4e0d\u5b58\u5728\u9759\u6001\u6210\u5458: ";
    constexpr const char* BASIC_TYPE_PREFIX = "\u57fa\u7840\u7c7b\u578b '";
    constexpr const char* BASIC_TYPE_MEMBER_ACCESS_FAIL = "' \u65e0\u6cd5\u8bbf\u95ee\u6210\u5458 '";
    constexpr const char* SCRIPT_SYNTAX_ERROR = "\u811a\u672c\u5305\u542b\u8bed\u6cd5\u9585\u8bef\uff0c\u672a\u52a0\u8f7d\u3002";
    constexpr const char* RUNTIME_ERROR_HEADER = "\n[Tzd \u8fd0\u884c\u65f6\u9519\u8bef] ";
    constexpr const char* UNCAUGHT_EXCEPTION_HEADER = "\n[Tzd \u672a\u6355\u83b7\u5f02\u5e38] ";
    constexpr const char* SYSTEM_EXCEPTION_HEADER = "\n[\u7cfb\u7edf\u5f02\u5e38] ";
    constexpr const char* CANNOT_OPEN_FILE = "\u65e0\u6cd5\u6253\u5f00\u6587\u4ef6: ";
    constexpr const char* NOT_CALLABLE_SIMPLE = "\u5c1d\u8bd5\u8c03\u7528\u4e3a\u4e00\u4e2a\u975e\u51fd\u6570\u5bf9\u8c61";
    constexpr const char* FUNCTION_PREFIX = "\u51fd\u6570 '";
    constexpr const char* PARAM_MISMATCH_EXPECT = "' \u53c2\u6570\u4e0d\u5339\u914d: \u671f\u671b ";
    constexpr const char* PARAM_MISMATCH_ACTUAL = " \u4e2a\uff0c\u5b9e\u9645 ";
    constexpr const char* PARAM_MISMATCH_SUFFIX = " \u4e2a";
    constexpr const char* PARAM_TYPE_MISMATCH_INDEX = "' \u7b2c ";
    constexpr const char* PARAM_TYPE_MISMATCH_EXPECT = " \u4e2a\u53c2\u6570\u7c7b\u578b\u4e0d\u5339\u914d: \u671f\u671b ";
    constexpr const char* PARAM_TYPE_MISMATCH_ACTUAL = "\uff0c\u5b9e\u9645 ";
    constexpr const char* ARRAY_INDEX_OUT_OF_BOUNDS_PREFIX = "\u6570\u7ec4\u7d22\u5f15\u8d8a\u754c: \u5c1d\u8bd5\u8bbf\u95ee\u7d22\u5f15 ";
    constexpr const char* ARRAY_INDEX_OUT_OF_BOUNDS_MID = ", \u4f46\u6570\u7ec4\u957f\u5ea6\u4e3a ";
    constexpr const char* STRING_INDEX_OUT_OF_BOUNDS_PREFIX = "\u5b57\u7b26\u4e32\u7d22\u5f15\u8d8a\u754c: \u5c1d\u8bd5\u8bbf\u95ee\u7d22\u5f15 ";
    constexpr const char* STRING_INDEX_OUT_OF_BOUNDS_MID = ", \u4f46\u5b57\u7b26\u4e32\u957f\u5ea6\u4e3a ";
    constexpr const char* INDEX_ACCESS_UNSUPPORTED = "\u7c7b\u5e8b\u9519\u8bef: \u8be5\u7c7b\u5e8b\u4e0d\u652f\u6301\u4e0b\u6807\u8bbf\u95ee";
    constexpr const char* CALL_NATIVE_PREFIX = "\u8c03\u7528\u53f7\u751f\u51fd\u6570 '";
    constexpr const char* CALL_NATIVE_MID = "' \u65f6\u53d1\u751f\u9519\u8bef: ";
    constexpr const char* NATIVE_PARAM_MISMATCH_EXPECT = "' \u8c03\u7528\u53c2\u6570\u4e0d\u5339\u914d: \u9700\u8981 ";
    constexpr const char* NATIVE_PARAM_MISMATCH_ACTUAL = " \u4e2a\uff0c\u5b9e\u9645\u63d0\u4f9b ";
    constexpr const char* NATIVE_PARAM_MISMATCH_SUFFIX = " \u4e2a\u3002";
    constexpr const char* TRY_CALL_NON_FUNCTION = "\u5c1d\u8bd5\u8c03\u7528\u975e\u51fd\u6570\u5bf9\u8c61: ";
    constexpr const char* UNDEFINED_TYPE_PREFIX = "\u672a\u5b9a\u4e49\u7684\u7c7b\u5e8b: '";
    constexpr const char* TYPE_MISMATCH_ASSIGN_PREFIX = "\u7c7b\u5e8b\u4e0d\u5339\u914d: \u65e0\u6cd5\u5c06 ";
    constexpr const char* TYPE_MISMATCH_ASSIGN_MID = " \u8d4b\u503c\u7ed8 ";
    constexpr const char* INDEX_ASSIGN_INVALID = "\u65e0\u6cd5\u5bf9\u975e\u6cd5\u7684\u4e8b\u6807\u4f4d\u7f6e\u8fdb\u884c\u8d4b\u503c";
    constexpr const char* INC_DEC_NON_EXISTENT_MEMBER = "\u5c1d\u8bd5\u5bf9\u4e0d\u5b58\u5728\u7684\u6210\u5458\u8fdb\u884c\u81ea\u589e/\u81ea\u51cf: ";
    constexpr const char* ARRAY_INDEX_OUT_OF_BOUNDS_OR_NOT_ARRAY = "\u6570\u7ec4\u7d22\u5f15\u8d8a\u754c\u6216\u76ee\u6807\u4e0d\u662f\u6570\u7ec4";
    constexpr const char* ARRAY_NOT_FOUND = "\u672a\u627e\u5230\u6570\u7ec4: ";
    constexpr const char* COMPILE_ERR_CAST_PREFIX = "[\u7f16\u8bd1\u9519\u8bef] \u65e0\u656c\u8f6c\u6362 '";
    constexpr const char* COMPILE_ERR_CAST_MID = "' \u4e3a ";
    constexpr const char* COMPILE_ERR_UNSUPPORTED_TYPE = "[\u7f16\u8bd1\u9519\u8bef] \u4e0d\u652f\u6301\u7684\u76ee\u6807\u7c7b\u5e8b\uff1a ";
    constexpr const char* IMPORT_ERROR_PREFIX = "[Import Error] \u627e\u4e0d\u5230\u6a21\u5757\u6216\u6587\u4ef6: ";
    constexpr const char* CLASS_REDEFINED_PREFIX = "\u7c7b\u91cd\u5b9a\u4e49: '";
    constexpr const char* CLASS_REDEFINED_SUFFIX = "' \u5df2\u5b58\u5728";
    constexpr const char* FFI_LOAD_ERROR = "FFI \u9519\u8bef: \u65e0\u6cd5\u52a0\u8f7d\u539f\u751f\u65b9\u6cd5 ";
    constexpr const char* UNKNOWN_TYPE_PREFIX = "\u672a\u77e5\u7c7b\u5e8b: ";
    constexpr const char* CTOR_NAME_PREFIX = "\u6784\u9020\u51fd\u6570\u540d '";
    constexpr const char* CTOR_NAME_SUFFIX = "' \u5fc5\u987b\u4e0e\u7c7b\u540d\u4e00\u81f4";
    constexpr const char* CTOR_CONFLICT_PREFIX = "\u6784\u9020\u51fd\u6570\u91cd\u8f7d\u51b2\u7a81: '";
    constexpr const char* CTOR_CONFLICT_MID = "' \u5df2\u6709 ";
    constexpr const char* CTOR_CONFLICT_SUFFIX = " \u4e2a\u53c2\u6570\u7684\u6784\u9020\u51fd\u6570";
    constexpr const char* UNDEFINED_ANNO_PREFIX = "\u672a\u5b9a\u4e49\u7684\u6ce8\u89e3: '@";
    constexpr const char* UNDEFINED_ANNO_MID = "'\u3002\u8bf9\u5148\u5b9a\u4e49\u8be5\u6ce8\u89e3\uff0c\u4f8b\u5982: class @";
    constexpr const char* UNDEFINED_ANNO_SUFFIX = " {}";
    constexpr const char* CTOR_NOT_FOUND_PREFIX = "\u627e\u4e0d\u5230\u5339\u914d\u7684\u6784\u9020\u51fd\u6570: '";
    constexpr const char* CTOR_NOT_FOUND_MID = "' \u9700\u8981 ";
    constexpr const char* CTOR_NOT_FOUND_SUFFIX = " \u4e2a\u53c2\u6570";
    constexpr const char* UNRESOLVED_SYMBOL_PREFIX = "\u65e0\u6cd5\u89e3\u6790\u7b2f\u53f7: ";
    constexpr const char* SYMBOL_REDEFINED_PREFIX = "\u7b2f\u53f7\u91cd\u5b9a\u4e49: '";
    constexpr const char* SYMBOL_REDEFINED_SUFFIX = "' \u5df2\u5728\u5f53\u524d\u4f5c\u7528\u57df\u5b9a\u4e49";
    constexpr const char* SYMBOL_CONFLICT_PREFIX = "\u7b2f\u53f7\u51b2\u7a81: '";
    constexpr const char* SYMBOL_CONFLICT_SUFFIX = "' \u5df2\u88ab\u5b9a\u4e49\u4e3a\u7c7b\u3001\u679a\u4e3e\u6216\u6ce8\u89e3";
    constexpr const char* SUPER_NO_PARENT_SUFFIX = "' \u6ca1\u6709\u7236\u7c7b\uff0c\u65e0\u6cd5\u8c03\u7528 super()";
    constexpr const char* PARENT_NOT_FOUND_PREFIX = "\u627e\u4e0d\u5230\u7236\u7c7b\u5b9a\u4e49: ";
    constexpr const char* PARENT_CLASS_PREFIX = "\u7236\u7c7b '";
    constexpr const char* PARENT_NO_CTOR_ARGS_SUFFIX = "' \u65e0\u6784\u9020\u51fd\u6570\uff0c\u4f46 super() \u4f20\u5165\u4e86\u53c2\u6570";

    // OOP Errors
    constexpr const char* OOP_MEMBER_NOT_FOUND_PREFIX = "[OOP Error] \u5c1d\u8bd5\u8bbf\u95ee\u672a\u5b9a\u4e49\u7684\u6210\u5458: ";
    constexpr const char* OOP_MEMBER_NOT_FOUND_MID = " (\u4f4d\u4e8e ";
    constexpr const char* OOP_MEMBER_NOT_FOUND_SUFFIX = " \u5b9a\u4e49\u4e2d)";
    constexpr const char* OOP_ASSIGN_NOT_FOUND_PREFIX = "[OOP Error] \u5c1d\u8bd5\u4e3a\u672a\u5b9a\u4e49\u7684\u6210\u5458\u8d4b\u503c: ";
    constexpr const char* OOP_ASSIGN_NOT_FOUND_MID = " (\u7c7b: ";
    constexpr const char* OOP_DUPLICATE_CLASS_PREFIX = "\u7c7b '";
    constexpr const char* OOP_DUPLICATE_CLASS_SUFFIX = "' \u5b58\u5728\u91cd\u540d\uff0c\u8bf9\u4f7f\u7528\u5168\u8def\u5f84\u7c7b\u540d\uff0c\u53ef\u9009\u7684\u7c7b\u6709: ";

    // JIT Errors
    constexpr const char* JIT_ARRAY_INDEX_OUT_OF_BOUNDS_PREFIX = "[JIT \u8fd0\u884c\u65f6\u9519\u8bef] \u6570\u7ec4\u7d22\u5f15\u8d8a\u754c: \u5c1d\u8bd5\u5199\u5165\u7d22\u5f15 ";
    constexpr const char* JIT_ARRAY_INDEX_OUT_OF_BOUNDS_MID = ", \u4f46\u6570\u7ec4\u5927\u5c0f\u4e3a ";
    constexpr const char* JIT_INDEX_ASSIGN_UNSUPPORTED = "[JIT \u8fd0\u884c\u65f6\u9519\u8bef] \u8be5\u6570\u636e\u7c7b\u5e8b\u4e0d\u652f\u6301\u4e0b\u6807\u8d4b\u503c\u64cd\u4f5c";
    constexpr const char* JIT_CLASS_NOT_FOUND_PREFIX = "\u627e\u4e0d\u5230\u7c7b\u5b9a\u4e49: ";
    constexpr const char* JIT_CLASS_NOT_FOUND_IMPORT_PREFIX = "\u627e\u4e0d\u5230\u7c7b\u5b9a\u4e49: '";
    constexpr const char* JIT_CLASS_NOT_FOUND_IMPORT_SUFFIX = "' (\u662f\u5426\u5fd8\u8bb0 import?)";
    constexpr const char* STACK_OVERFLOW = "\u8fd0\u884c\u9519\u8bef\uff1a\u6808\u6ea2\u51fa";
} // namespace TzdRte

// ============================================================================
//  6. 数学/本机模块 (Math / Native Module) 错误
// ============================================================================
namespace TzdMth {
    constexpr const char* SOLVE_EQ_ERROR   = "Error: solveEq requires a function as the 1st argument.";
    constexpr const char* SOLVE_SYM_ERROR  = "Error: solveSym requires an equation string as the 1st argument.";
    constexpr const char* SIMPLIFY_SYM_ERR = "Error: simplifySym requires an expression string.";
    constexpr const char* SOLVE_INEQ_ERROR = "Error: solveIneq(func, opStr, [low, high, asFraction])";
    constexpr const char* PLOT_ERROR       = "Error: plot(func, start, end, [step])";
    constexpr const char* NO_REAL_ROOTS    = "No real roots found in range";
    constexpr const char* NO_SYM_SOLUTIONS = "No symbolic solutions found.";
    constexpr const char* SOLVER_ERROR     = "Symbolic Solver Error: ";
    constexpr const char* SIMPLIFY_ERROR   = "Symbolic Simplify Error: ";
    constexpr const char* UNKNOWN_SOLVER   = "Unknown error in Fintamath solver.";
    constexpr const char* FILE_NOT_FOUND   = "Error: File not found";
    constexpr const char* BASE_OUT_RANGE   = "Error: Base out of range (2-36)";
    constexpr const char* SIZE_MISMATCH    = "Error: Size mismatch";
    constexpr const char* NOT_SQUARE       = "Error: Not square";
    constexpr const char* THREAD_START_ERR = "Thread start requires a function target.";

    constexpr const char* PLOT_RANGE_EQUAL = "\n[Tzd \u8b66\u544a] \u8d77\u59cb\u70b9 (%g) \u548c\u7ec8\u6b62\u70b9 (%g) \u76f8\u7b49\uff0c\u65e0\u6cd5\u7ed8\u56fe";
    constexpr const char* PLOT_SWAP_WARN   = "[Tzd \u8b66\u544a] \u8d77\u59cb\u70b9\u5927\u4e8e\u7ec8\u6b62\u70b9\uff0c\u5df2\u81ea\u52a8\u8c03\u6362";

    // 数值转换辅助
    static inline std::string doubleToFraction(double val, double tolerance = 1e-5) {
        if (std::isnan(val)) return "NaN";
        if (std::isinf(val)) return "Infinity";
        bool negative = val < 0;
        if (negative) val = -val;
        long long h1 = 1, h2 = 0, k1 = 0, k2 = 1;
        double b = val;
        for (int i = 0; i < 15; ++i) {
            long long a = (long long)std::floor(b);
            long long aux_h = h1; h1 = a * h1 + h2; h2 = aux_h;
            long long aux_k = k1; k1 = a * k1 + k2; k2 = aux_k;
            if (k1 == 0) break;
            if (std::abs(val - (double)h1 / k1) <= tolerance) break;
            double diff = b - a;
            if (std::abs(diff) < 1e-7) break;
            b = 1.0 / diff;
        }
        if (k1 == 0) return "0/1";
        long long final_num = negative ? -h1 : h1;
        return (k1 == 1) ? std::to_string(final_num) : std::to_string(final_num) + "/" + std::to_string(k1);
    }
} // namespace TzdMth

// ============================================================================
//  7. 系统信息 (System Info) 常量
// ============================================================================
namespace TzdSys {
    constexpr const char* APP_NAME    = "Tzd Engine";
    constexpr const char* AUTHOR      = "tzdwindows 7";
    constexpr const char* VERSION     = "1.0.0 Alpha";

    constexpr const char* MOD_STACKTRACE    = "StackTrace ... [OK]";
    constexpr const char* MOD_MEMORYASM     = "MemoryAsm  ... [OK]";
    constexpr const char* MOD_PDBREADER     = "PdbReader  ... [OK]";
    constexpr const char* MOD_GUI           = "GUI System ... [Standby]";

    constexpr const char* SHELL_PROMPT       = "Tzd> ";
    constexpr const char* SHELL_PROMPT_CONT  = "   > ";
    constexpr const char* SHELL_HELP         = "\u8f93\u5165 'help' \u67e5\u770b\u5b8c\u6574\u547d\u4ee4\u5217\u8868\u3002";
    constexpr const char* SHELL_EXIT         = "\u8f93\u5165 'exit' \u9000\u51fa\u7a0b\u5e8f\u3002";
    constexpr const char* SHELL_MULTILINE    = "\u652f\u6301\u591a\u884c\u8f93\u5165 (\u76f4\u5230\u5927\u62ec\u53f7\u95ed\u5408\u6216\u9047\u5230\u5206\u53f7)\u3002";

    constexpr const char* ARCH_X64 = "x64 (64-bit)";
    constexpr const char* ARCH_X86 = "x86 (32-bit)";

    constexpr const char* MOD_FREE_HOOK = "[HACK] \u5df2\u6302\u94a9 free \u51fd\u6570\uff0c\u51c6\u5907\u62e6\u622a\u975e\u6cd5\u91ca\u653e...";
    constexpr const char* SHELL_TITLE_FMT = "\n--- %s \u547d\u4ee4\u7cfb\u7edf (\u4f5c\u8005: %s) ---";
    constexpr const char* SHELL_INTRO = "\u76f4\u63a5\u8f93\u5165\u547d\u4ee4\u5e76\u4ee5\u5206\u53f7(;)\u7ed3\u675f\u3002\u5e38\u7528\u547d\u4ee4\u5982\u4e0b:";
    constexpr const char* SHELL_CMD_FMT = "  > %s \t [%s]";
    constexpr const char* SHELL_HELP_SPECIFIC = "\u8f93\u5165 'help <\u547d\u4ee4\u540d>;' \u67e5\u770b\u5177\u4f53\u8be6\u60c5\u3002";
    constexpr const char* HELP_CHNAME = "\n\u3010\u547d\u4ee4\u4e2d\u6587\u540d\u3011: ";
    constexpr const char* HELP_FORMAT = "\u3010\u4f7f\u7528\u683c\u5f0f\u3011  : ";
    constexpr const char* HELP_DESC = "\u3010\u529f\u80fd\u4ecb\u7ecd\u3011  : ";
    constexpr const char* HELP_SUBOPTS = "\u3010\u5b50\u9009\u9879/\u6269\u5c55\u3011: ";
    constexpr const char* SHELL_SYSINFO = " [System Info]";
    constexpr const char* SHELL_VERSION_LABEL = "   * \u7248\u672c\u53f7  : ";
    constexpr const char* SHELL_BUILD = "   * \u6784\u5efa\u4e8e  : ";
    constexpr const char* SHELL_AUTHOR = "   * \u5f00\u53d1\u8005  : ";
    constexpr const char* SHELL_ARCH_LABEL = "   * \u67b6\u6784    : ";
    constexpr const char* SHELL_MODSTATUS = "\n [Module Status]";
    constexpr const char* SHELL_MOD_PREFIX = "   * ";
    constexpr const char* SHELL_INTERACTIVE = "\n [Interactive Shell]";
    constexpr const char* SHELL_BULLET = "   ";
    constexpr const char* SHELL_SEP = " --------------------------------------------------------------------\n";
} // namespace TzdSys

// ============================================================================
//  8. 通用 (Generic) 工具字符串
// ============================================================================
namespace TzdGen {
    constexpr const char* NULL_STR        = "null";
    constexpr const char* NULL_INSTANCE   = "null instance";
    constexpr const char* UNKNOWN         = "unknown";
    constexpr const char* NA              = "N/A";
    constexpr const char* TRUE_STR        = "true";
    constexpr const char* FALSE_STR       = "false";
    constexpr const char* MEMORY          = "memory";
    constexpr const char* ERROR_STR       = "Error";
    constexpr const char* DEFAULT_VALUE   = "0/1";

    // 类型名称
    constexpr const char* TYPE_NONE     = "NONE";
    constexpr const char* TYPE_FLOAT    = "FLOAT";
    constexpr const char* TYPE_STRING   = "STRING";
    constexpr const char* TYPE_BOOL     = "BOOL";
    constexpr const char* TYPE_FUNCTION = "FUNCTION";
    constexpr const char* TYPE_ARRAY    = "ARRAY";

    // 类型名数组（与 TzdValue::Type 枚举顺序对齐）
    constexpr const char* TYPE_NAME_MAP[] = {
        "NONE", "SBYTE", "BYTE", "SHORT", "USHORT",
        "INT", "UINT", "LONG", "ULONG",
        "FLOAT", "DOUBLE",
        "BOOL", "STRING", "POINTER", "ARRAY", "MAP",
        "FUNCTION", "NATIVE_FUNCTION",
        "CLASS_DEF", "INSTANCE", "ENUM_VAL", "ERROR_VAL",
        "FUTURE", "ANY_REF"
    };
    constexpr size_t TYPE_NAME_COUNT = sizeof(TYPE_NAME_MAP) / sizeof(TYPE_NAME_MAP[0]);

    // 通过枚举索引获取类型名称
    static inline const char* typeName(int typeIndex) {
        if (typeIndex >= 0 && typeIndex < (int)TYPE_NAME_COUNT)
            return TYPE_NAME_MAP[typeIndex];
        return "UNKNOWN";
    }
} // namespace TzdGen

// ============================================================================
//  8.5 GUI 界面相关常量
// ============================================================================
namespace TzdGui {
    constexpr const wchar_t* FUNC_CLASS_NAME_W = L"TzdFuncGui";
    constexpr const wchar_t* FUNC_WINDOW_TITLE_W = L"Tzd Function Scanner Result";
    constexpr const wchar_t* MEM_CLASS_NAME_W = L"TzdGui";
    constexpr const wchar_t* MEM_WINDOW_TITLE_W = L"Tzd Memory Analyzer (v5.0)";

    constexpr const char* DETAIL_NA_OFFSET = "N/A + 0x";
    constexpr const char* DETAIL_PLUS_HEX = " + 0x";

    constexpr const char* FUNC_CLASS_NAME = "TzdFuncGui";
    constexpr const char* FUNC_WINDOW_TITLE = "Tzd Function Scanner Result";
    constexpr const char* MEM_CLASS_NAME = "TzdGui";
    constexpr const char* MEM_WINDOW_TITLE = "Tzd Memory Analyzer (v5.0)";

    constexpr const char* FONT_PATH_MSYH = "C:\\\\Windows\\\\Fonts\\\\msyh.ttc";
    constexpr const char* FONT_PATH_SIMSUN = "C:\\\\Windows\\\\Fonts\\\\simsun.ttc";

    constexpr const char* COMBO_ALL_RESULTS = "All Results";
    constexpr const char* COMBO_HAS_PDB = "Has PDB info";
    constexpr const char* IMGUI_SCANNER_MAIN = "ScannerMain";
    constexpr const char* IMGUI_SEARCH_FUNC = " Search";
    constexpr const char* IMGUI_SEARCH_MEM = " Search (instruction/address)";
    constexpr const char* IMGUI_FILTER = "Filter";
    constexpr const char* IMGUI_STATS_FMT = "| Total: %zu | Show: %zu";
    constexpr const char* IMGUI_RESULT_TABLE = "ResultTable";
    constexpr const char* IMGUI_ASM_TABLE = "AsmTable";
    constexpr const char* IMGUI_COL_ADDRESS = "Address";
    constexpr const char* IMGUI_COL_ASSEMBLY = "Assembly";
    constexpr const char* IMGUI_COL_HEX = "Hex";
    constexpr const char* IMGUI_COL_MODULE_BASE = "Module Base";
    constexpr const char* IMGUI_COL_LOC_INFO = "Location Information";
    constexpr const char* IMGUI_COL_ASM_INSTRUCTIONS = "Assembly instructions";
    constexpr const char* IMGUI_ANALYZER = "Analyzer";

    constexpr const char* DETAIL_ADDRESS = "Address:0x";
    constexpr const char* DETAIL_BASE_ADDRESS = "Base address:0x";
    constexpr const char* DETAIL_ASM = "asm:";
    constexpr const char* DETAIL_SYMBOL = "Symbol:";
    constexpr const char* DETAIL_CONV_BEFORE = "Change of name before conversion:";
    constexpr const char* DETAIL_RAW_PDB = "Raw PDB: %s";
    constexpr const char* DETAIL_UNKNOWN = "???";

    constexpr const char* LOC_PEB_PREFIX = "(PEB:";
    constexpr const char* LOC_PEB_SUFFIX = ")";
    constexpr const char* LOC_PEB_CH_PREFIX = "\uff08PEB\uff1a"; // （PEB：
    constexpr const char* LOC_PEB_CH_SUFFIX = "\uff09"; // ）
    constexpr const char* LOC_PEB_CH_FULL = "xxx + xxx\uff08PEB\uff1axxx\uff09"; // xxx + xxx（PEB：xxx）
    constexpr const char* LOC_NO_NAME = "Change of name: N/A + offset";
    constexpr const char* LOC_EXPORT_OFFSET = "\u76f8\u5bf9\u4e8e\u5bfc\u51fa\u51fd\u6570\u7684\u504f\u79fb"; // 相对于导出函数的偏移
    constexpr const char* LOC_PDB_OFFSET = "\u76f8\u5bf9\u4e8ePDB\u7684\u504f\u79fb"; // 相对于PDB的偏移
    constexpr const char* LOC_CONV_BEFORE_FMT = "Change of name before conversion: xxx + xx";
}

// ============================================================================
//  9. 辅助格式化工具函数
// ============================================================================
namespace TzdFmt {
    // 格式化运行时错误头
    static inline std::string runtimeErrorTitle(size_t line, size_t col) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), TzdErr::TITLE, line, col);
        return std::string(buf);
    }

    // 格式化 Demangle 输出
    static inline std::string demangleOutput(const std::string& raw, const std::string& result) {
        return std::string(TzdCmd::RAW_LABEL) + raw + "\n" + TzdCmd::RESULT_LABEL + result;
    }

    // 构造符号表行（带 RVA）
    static inline std::string symTableLine(uint64_t rva, const std::string& name) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "  [0x%08llx] ", (unsigned long long)rva);
        return std::string(buf) + name;
    }

    // 获取架构字符串
    static inline const char* archString() {
        return (sizeof(void*) == 8) ? TzdSys::ARCH_X64 : TzdSys::ARCH_X86;
    }

    // 判断字符串是否为 UTF-8
    static inline bool isUtf8(const std::string& str) {
        int nBytes = 0;
        bool allAscii = true;
        for (unsigned char ch : str) {
            if ((ch & 0x80) != 0) allAscii = false;
            if (nBytes == 0) {
                if (ch >= 0x80) {
                    if (ch >= 0xFC && ch <= 0xFD) nBytes = 6;
                    else if (ch >= 0xF8) nBytes = 5;
                    else if (ch >= 0xF0) nBytes = 4;
                    else if (ch >= 0xE0) nBytes = 3;
                    else if (ch >= 0xC0) nBytes = 2;
                    else return false;
                    nBytes--;
                }
            } else {
                if ((ch & 0xC0) != 0x80) return false;
                nBytes--;
            }
        }
        return nBytes <= 0;
    }

    // 字符串转义工具
    static inline std::string unescape(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\\' && i + 1 < input.size()) {
                switch (input[i + 1]) {
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"':  result += '"';  break;
                    case '\'': result += '\''; break;
                    default:   result += input[i + 1]; break;
                }
                ++i;
            } else {
                result += input[i];
            }
        }
        return result;
    }
} // namespace TzdFmt

// ============================================================================
//  10. 字符串资源文件加载器 (可选, 用于从 JSON 文件加载)
// ============================================================================
#ifdef TZD_STRINGS_LOADER_ENABLED
#include <nlohmann/json.hpp>
namespace TzdRes {
    // 从 JSON 文件加载字符串
    static inline bool loadFromJson(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        nlohmann::json j;
        try {
            f >> j;
        } catch (...) {
            return false;
        }
        // JSON 中的字符串可通过 (std::string)j["categories"]["runtime_error"]["RUNTIME_ERROR_TITLE"] 访问;
        // 当前实现仅验证文件格式，具体键值读取由调用方完成
        return !j.is_null();
    }
} // namespace TzdRes
#endif // TZD_STRINGS_LOADER_ENABLED
