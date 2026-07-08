#ifndef TZD_COMMAND_SYSTEM_H
#define TZD_COMMAND_SYSTEM_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>

// 3. ANTLR 运行时（必须在生成代码之前）
#include "antlr4-runtime.h"

// 4. 【核心修复】先包含 Parser ！！！
// 只有这样，后续的 Interpreter 才能正确识别 Context 类的继承关系
#include "Generated/TzdLangParser.h" 

// 5. 再包含 Interpreter 和其他
#include "Generated/TzdInterpreter.h"
#include "Generated/TzdLangLexer.h"

// 6. 其他项目模块
#include "TzdFuncScanner.h"
#include "TzdMemoryAsm.h"
#include "TzdStackTrace.h"
#include "SymbolDemangler.h"
#include "PdbReader.h"

// 存储命令的详细信息（帮助消息）
struct CommandMeta {
    std::string name;           // 命令名: Hello
    std::string chineseName;    // 命令中文名: 打招呼
    std::string format;         // 格式: Hello <int> <string> <float> <double>
    std::string description;    // 介绍: 这是一个测试命令
    std::string subOptions;     // 子选项: -f, No, XXX 等
    std::function<void(const std::vector<std::string>&)> handler;
};

class TzdCommandSystem {
public:
    TzdCommandSystem();

    // 初始化注册命令
    void init();

  

    // 启动系统 (支持命令行参数启动和交互式启动)
    void start(int argc, char* argv[]);

private:
    std::map<std::string, CommandMeta> registry;
    std::string inputBuffer;
    const std::string AUTHOR = "tzdwindows 7";
    const std::string APP_NAME = "TzdTools";

    static TzdInterpreter* interpreter;

    // 内部核心方法
    void registerCmd(std::string name, std::string cn, std::string fmt, std::string desc, std::string opts, std::function<void(const std::vector<std::string>&)> func);
    void process(std::string input);
    void printHelp(std::string cmdName = "");
    void enterInteractiveMode();

    static void handleStackTrace(const std::vector<std::string>& args);
    static void handleMemoryAsm(const std::vector<std::string>& args);
    static void handlePdbInfo(const std::vector<std::string>& args);
    static void handleScanFunc(const std::vector<std::string>& args);
    static void handleDemangle(const std::vector<std::string>& args);
    static void handleRunScript(const std::vector<std::string>& args);
};

#endif