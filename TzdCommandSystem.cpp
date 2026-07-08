#ifndef NOMINMAX

#define NOMINMAX

#endif

#include "Res/TzdStrings.h"

#include "TzdDebugger.h"

#include "TzdCommandSystem.h"

// 包含标准库

#include <algorithm>

#include <sstream>

#include <iomanip> 

#include <fstream>

#include <filesystem>

#include <any>

#include <string>

#include <vector>

#include <iostream>

TzdInterpreter* TzdCommandSystem::interpreter = nullptr;

TzdCommandSystem::TzdCommandSystem() : inputBuffer("") {

    if (interpreter == nullptr) {

        interpreter = new TzdInterpreter();

    }

}

void TzdCommandSystem::registerCmd(std::string name, std::string cn, std::string fmt, std::string desc, std::string opts, std::function<void(const std::vector<std::string>&)> func) {
    registry[name] = { name, cn, fmt, desc, opts, func };
}

void TzdCommandSystem::init() {
    // 1. 定义一个辅助 Lambda，把脚本传入的 TzdValue 参数转回 string 参数

    // 这样就能直接复用现有的 handleXxx 函数

    auto argsToStrings = [](const std::vector<TzdValue>& tzdArgs) -> std::vector<std::string> {

        std::vector<std::string> strArgs;

        for (const auto& arg : tzdArgs) {

            if (arg.type == TzdValue::STRING) {

                strArgs.push_back(arg.sVal);

            }

            else if (arg.type == TzdValue::FLOAT) {

                std::string s = std::to_string(arg.dVal);

                s.erase(s.find_last_not_of('0') + 1, std::string::npos);

                if (s.back() == '.') s.pop_back();

                strArgs.push_back(s);

            }

            else if (arg.type == TzdValue::BOOL) {

                strArgs.push_back(arg.bVal ? "true" : "false");

            }

        }

        return strArgs;

        };

    // ============================================================

    //  StackTrace

    // ============================================================

    // [指令模式] StackTrace pid;

    registerCmd("StackTrace",

        "进程堆栈跟踪",

        "StackTrace <PID|进程名>",

        "输出调用栈。支持不区分大小写的进程名查找。",

        "示例: StackTrace qq.exe;",

        TzdCommandSystem::handleStackTrace);

    // [函数模式] StackTrace("pid");

    interpreter->registerNativeFunction("StackTrace", [argsToStrings](const std::vector<TzdValue>& args) -> TzdValue {

        // 直接复用 handleStackTrace 逻辑

        TzdCommandSystem::handleStackTrace(argsToStrings(args));

        return TzdValue();

        });

    // ============================================================

    //  MemoryAsm

    // ============================================================

    // [指令模式]

    registerCmd("MemoryAsm",

        "内存反汇编分析",

        "MemoryAsm <PID|ProcessName>",

        "扫描目标进程的所有可执行代码段并翻译为汇编指令。",

        "危险操作：可能导致目标进程短暂卡顿。",

        TzdCommandSystem::handleMemoryAsm);

    // [函数模式]

    interpreter->registerNativeFunction("MemoryAsm", [argsToStrings](const std::vector<TzdValue>& args) -> TzdValue {

        TzdCommandSystem::handleMemoryAsm(argsToStrings(args));

        return TzdValue();

        });

    // ============================================================

    //  ScanFunc

    // ============================================================

    // [指令模式]

    registerCmd("ScanFunc",

        "扫描进程函数(支持PDB与GUI)",

        "ScanFunc <PID|Name> [-m Module] [-p PdbPath] [-g]",

        "通过特征码定位函数并进行符号还原。\n"

        "  -m: 指定目标模块名 (如: UnityPlayer.dll)\n"

        "  -p: 指定外部 PDB 符号路径\n"

        "  -g: 开启 DirectX 11 GUI 交互界面 (支持搜索/过滤)",

        "示例: ScanFunc notepad.exe -m notepad.exe -g",

        TzdCommandSystem::handleScanFunc);

    // [函数模式] ScanFunc("notepad.exe", "-g");

    interpreter->registerNativeFunction("ScanFunc", [argsToStrings](const std::vector<TzdValue>& args) -> TzdValue {

        TzdCommandSystem::handleScanFunc(argsToStrings(args));

        return TzdValue();

        });

    // ============================================================

    //  Demangle

    // ============================================================

    // [指令模式]

    registerCmd("Demangle",

        "C++符号去修饰",

        "Demangle <MangledName>",

        "将 MSVC 编译器的修饰名转换为可读的函数签名。",

        "示例: Demangle ?init@System@@QAEXXZ",

        TzdCommandSystem::handleDemangle);

    // [函数模式]

    interpreter->registerNativeFunction("Demangle", [argsToStrings](const std::vector<TzdValue>& args) -> TzdValue {

        TzdCommandSystem::handleDemangle(argsToStrings(args));

        return TzdValue();

        });

    // ============================================================

    //  PdbInfo

    // ============================================================

    // [指令模式]

    registerCmd("PdbInfo",

        "PDB文件分析工具",

        "PdbInfo <PdbPath>",

        "读取 PDB 文件头信息并尝试提取前 10 个公开符号。",

        "示例: PdbInfo C:\\Symbols\\game.pdb",

        TzdCommandSystem::handlePdbInfo);

    // [函数模式] PdbInfo("C:/path.pdb");

    interpreter->registerNativeFunction("PdbInfo", [argsToStrings](const std::vector<TzdValue>& args) -> TzdValue {

        TzdCommandSystem::handlePdbInfo(argsToStrings(args));

        return TzdValue();

        });

    // ============================================================

    //  Run (仅保留为系统指令，无需函数化)

    // ============================================================

    registerCmd("Run",

        "运行脚本",

        "Run <Code|FilePath>",

        "解析并运行 TzdLang 脚本代码。",

        "示例: Run \"a = gxxx 100; b = a ^ 2;\"",

        TzdCommandSystem::handleRunScript);

}

void printRuntimeError(const std::string& msg, antlr4::Token* token) {

    size_t line = token->getLine();

    size_t charPositionInLine = token->getCharPositionInLine();

    std::cerr << "==================================================" << std::endl;

    std::cerr << "[Tzd 运行时错误] 行 " << line << ":" << charPositionInLine << std::endl;

    std::cerr << "[错误详情] " << msg << std::endl;

    antlr4::CharStream* stream = token->getInputStream();

    if (stream) {

        std::string fullText = stream->toString();

        std::istringstream iss(fullText);

        std::string codeLine;

        size_t currentLine = 1;

        while (std::getline(iss, codeLine)) {

            if (!codeLine.empty() && codeLine.back() == '\r') codeLine.pop_back();

            if (currentLine == line) {

                std::cerr << "--------------------------------------------------" << std::endl;

                std::cerr << "    " << codeLine << std::endl;

                std::cerr << "    ";

                for (size_t i = 0; i < charPositionInLine; ++i) {

                    if (i < codeLine.size() && codeLine[i] == '\t') std::cerr << '\t';

                    else std::cerr << ' ';

                }

                std::cerr << "^--- 这里" << std::endl;

                break;

            }

            currentLine++;

        }

    }

    std::cerr << "==================================================" << std::endl;

}

void TzdCommandSystem::handleRunScript(const std::vector<std::string>& args) {

    if (args.empty()) return;

    std::string code = args[0];

    try {

        interpreter->loadScript(code);

    }

    catch (const std::exception& e) {

        std::cerr << "[Tzd 系统错误] " << e.what() << std::endl;

    }

}

void TzdCommandSystem::handleDemangle(const std::vector<std::string>& args) {

    if (args.empty()) {

        std::cout << "[Tzd] 用法: Demangle <修饰名字符串>" << std::endl;

        return;

    }

    std::string rawName = args[0];

    for (size_t i = 1; i < args.size(); ++i) rawName += args[i];

    std::string readable = SymbolDemangler::demangle(rawName);

    std::cout << "[原始] " << rawName << std::endl;

    std::cout << "[结果] " << readable << std::endl;

}

void TzdCommandSystem::handlePdbInfo(const std::vector<std::string>& args) {

    if (args.empty()) {

        std::cout << "[Tzd] 用法: PdbInfo <Pdb路径>" << std::endl;

        return;

    }

    std::string path = args[0];

    std::cout << "[Tzd] 正在读取 PDB: " << path << " ..." << std::endl;

    PdbReader reader(path);

    if (!reader.IsValid()) {

        std::cout << "[Tzd] 错误: PDB 文件无效或无法打开。" << std::endl;

        return;

    }

    std::cout << "[Tzd] PDB 文件校验通过 (MSF 格式有效)。" << std::endl;

    std::vector<uint32_t> mockSections = { 0x1000, 0x2000, 0x3000, 0x4000, 0x5000 };

    std::vector<PdbSymbol> symbols;

    if (reader.ParsePublicSymbols(mockSections, symbols)) {

        std::cout << "[Tzd] 成功解析符号表，共找到 " << symbols.size() << " 个公开符号。" << std::endl;

        for (const auto& sym : symbols) {

            std::cout << "  [0x" << std::hex << std::setw(8) << std::setfill('0') << sym.rva << "] "

                << SymbolDemangler::demangle(sym.name) << std::dec << std::endl;

        }

    }

    else {

        std::cout << "[Tzd] 警告: 符号流解析失败。" << std::endl;

    }

}

void TzdCommandSystem::handleScanFunc(const std::vector<std::string>& args) {

    if (args.empty()) {

        std::cout << "[Tzd] 用法: ScanFunc <PID|进程名> [-m 模块名] [-p PDB路径] [-g (开启GUI)]" << std::endl;

        return;

    }

    DWORD pid = 0;

    std::string target = args[0];

    std::string moduleName = "";

    std::string pdbPath = "";

    bool useGui = false;

    for (size_t i = 1; i < args.size(); ++i) {

        if (args[i] == "-m" && i + 1 < args.size()) {

            moduleName = args[++i];

        }

        else if (args[i] == "-p" && i + 1 < args.size()) {

            pdbPath = args[++i];

        }

        else if (args[i] == "-g") {

            useGui = true;

        }

    }

    try {

        if (std::all_of(target.begin(), target.end(), ::isdigit)) {

            pid = std::stoul(target);

        }

        else {

            pid = TzdStackTrace::getPidByName(target);

        }

    }

    catch (...) {

        pid = TzdStackTrace::getPidByName(target);

    }

    if (pid == 0) {

        std::cout << "[Tzd] 错误: 找不到目标进程: " << target << std::endl;

        return;

    }

    std::cout << "[Tzd] 正在扫描进程: " << target << " (" << pid << ")" << std::endl;

    if (!moduleName.empty()) std::cout << "[Tzd] 过滤模块: " << moduleName << std::endl;

    if (!pdbPath.empty())   std::cout << "[Tzd] 符号文件: " << pdbPath << std::endl;

    if (useGui)             std::cout << "[Tzd] 模式: GUI 视图" << std::endl;

    TzdFuncScanner scanner;

    if (useGui) {

        std::vector<ScanResult> results;

        scanner.scanProcess(pid, moduleName, pdbPath, &results);

        if (results.empty()) {

            std::cout << "[Tzd] 警告: 未扫描到任何符合特征的函数，GUI 未启动。" << std::endl;

        }

    }

    else {

        scanner.scanProcess(pid, moduleName, pdbPath, nullptr);

    }

    std::cout << "\n[Tzd] 任务已提交，扫描完成。" << std::endl;

}

void TzdCommandSystem::handleMemoryAsm(const std::vector<std::string>& args) {

    if (args.empty()) {

        std::cout << "[Tzd] 用法: MemoryAsm <PID|Name>;" << std::endl;

        return;

    }

    DWORD pid = (isdigit(args[0][0])) ? std::stoul(args[0]) : TzdStackTrace::getPidByName(args[0]);

    if (pid != 0) {

        TzdMemoryAsm::analyzeAndDumpAsm(pid);

    }

    else {

        std::cout << "[Tzd] 错误: 找不到目标进程。" << std::endl;

    }

}

void TzdCommandSystem::handleStackTrace(const std::vector<std::string>& args) {

    if (args.empty()) {

        std::cout << "[Tzd] 错误: 请提供 PID 或进程名。用法: StackTrace <Target>;" << std::endl;

        return;

    }

    std::string target = args[0];

    DWORD pid = 0;

    if (!target.empty() && std::all_of(target.begin(), target.end(), ::isdigit)) {

        pid = std::stoul(target);

    }

    else {

        pid = TzdStackTrace::getPidByName(target);

        if (pid == 0) {

            std::cout << "[Tzd] 错误: 找不到名为 '" << target << "' 的进程。" << std::endl;

            return;

        }

        std::cout << "[Tzd] 已找到进程 " << target << "，对应 PID: " << pid << std::endl;

    }

    TzdStackTrace::dumpProcessStack(pid);

}

void TzdCommandSystem::printHelp(std::string cmdName) {

    if (cmdName.empty()) {

        std::cout << "\n--- " << APP_NAME << " 命令系统 (作者: " << AUTHOR << ") ---" << std::endl;

        std::cout << "直接输入命令并以分号(;)结束。常用命令如下:" << std::endl;

        for (auto const& [name, meta] : registry) {

            std::cout << "  > " << name << " \t [" << meta.chineseName << "]" << std::endl;

        }

        std::cout << "输入 'help <命令名>;' 查看具体详情。" << std::endl;

    }

    else {

        if (registry.count(cmdName)) {

            auto& m = registry[cmdName];

            std::cout << "\n【命令中文名】: " << m.chineseName << std::endl;

            std::cout << "【使用格式】  : " << m.format << std::endl;

            std::cout << "【功能介绍】  : " << m.description << std::endl;

            std::cout << "【子选项/扩展】: " << m.subOptions << std::endl;

        }

        else {

            std::cout << "[Tzd] 未找到命令 '" << cmdName << "' 的帮助信息。" << std::endl;

        }

    }

}

void TzdCommandSystem::process(std::string input) {

    // 1. 去除首尾空白

    std::string trimmed = input;

    size_t firstNonSpace = trimmed.find_first_not_of(" \t\r\n");

    if (firstNonSpace == std::string::npos) return;

    trimmed.erase(0, firstNonSpace);

    size_t lastNonSpace = trimmed.find_last_not_of(" \t\r\n");

    if (lastNonSpace != std::string::npos) trimmed.erase(lastNonSpace + 1);

    if (trimmed.empty()) return;

    // 2. 提取首个单词

    size_t firstSpace = trimmed.find_first_of(" \t");

    std::string cmdHead = (firstSpace == std::string::npos) ? trimmed : trimmed.substr(0, firstSpace);

    // 3. 关键字保护

    static const std::set<std::string> keywords = {

        "fun", "class", "var", "if", "while", "for", "return", "ret",

        "print", "new", "sin", "cos", "tan", "log"

    };

    bool isKeyword = keywords.count(cmdHead);

    bool looksLikeFunctionCall = false;

    if (trimmed.find('(') != std::string::npos) {

        size_t openParen = trimmed.find('(');

        std::string potentialName = trimmed.substr(0, openParen);

        size_t lastChar = potentialName.find_last_not_of(" \t");

        if (lastChar != std::string::npos) potentialName = potentialName.substr(0, lastChar + 1);

        if (potentialName == cmdHead) {

            looksLikeFunctionCall = true;

        }

    }

    bool isSystemCmd = (cmdHead == "help") || (!isKeyword && !looksLikeFunctionCall && registry.count(cmdHead));

    if (!isSystemCmd) {

        // --- 脚本模式 ---

        std::string scriptCode = trimmed;

        if (scriptCode.size() >= 4 && scriptCode.substr(0, 4) == "Run ") {

            scriptCode = scriptCode.substr(4);

            size_t scriptStart = scriptCode.find_first_not_of(" \t");

            if (scriptStart != std::string::npos) scriptCode.erase(0, scriptStart);

        }

        if (scriptCode.empty()) return;

        std::vector<std::string> args = { scriptCode };

        try {

            handleRunScript(args);

        }

        catch (const std::exception& e) {

            std::cerr << "[Tzd 运行时错误] " << e.what() << std::endl;

        }

        return;

    }

    // --- 系统指令模式 ---

    std::vector<std::string> tokens;

    std::string currentToken;

    bool inQuotes = false;

    for (size_t i = 0; i < trimmed.length(); ++i) {

        char c = trimmed[i];

        if (c == '"') { inQuotes = !inQuotes; continue; }

        if (std::isspace(c) && !inQuotes) {

            if (!currentToken.empty()) { tokens.push_back(currentToken); currentToken.clear(); }

        }

        else currentToken += c;

    }

    if (!currentToken.empty()) tokens.push_back(currentToken);

    // 重定向处理

    std::string outputFile = "";

    auto it = std::find(tokens.begin(), tokens.end(), "=>");

    if (it != tokens.end()) {

        if (std::next(it) != tokens.end()) {

            outputFile = *std::next(it);

            tokens.erase(it, tokens.end());

        }

        else {

            std::cerr << "[Tzd 语法错误] 重定向符号 '=>' 后缺少文件名。" << std::endl;

            return;

        }

    }

    if (tokens.empty()) return;

    std::string head = tokens[0];

    std::vector<std::string> sysArgs(tokens.begin() + 1, tokens.end());

    std::ofstream outFile;

    std::streambuf* coutBuf = nullptr;

    if (!outputFile.empty()) {

        char buffer[MAX_PATH];

        GetModuleFileNameA(NULL, buffer, MAX_PATH);

        std::filesystem::path finalPath = std::filesystem::path(buffer).parent_path() / outputFile;

        outFile.open(finalPath, std::ios::out | std::ios::trunc);

        if (outFile.is_open()) {

            coutBuf = std::cout.rdbuf();

            std::cout.rdbuf(outFile.rdbuf());

        }

        else {

            std::cerr << "[Tzd 错误] 无法创建输出文件" << std::endl;

            return;

        }

    }

    if (head == "help") {

        if (!sysArgs.empty()) printHelp(sysArgs[0]);

        else printHelp();

    }

    else {

        registry[head].handler(sysArgs);

    }

    if (coutBuf) {

        std::cout.rdbuf(coutBuf);

        outFile.close();

        std::cout << "[Tzd] 输出已保存至: " << outputFile << std::endl;

    }

}

void TzdCommandSystem::enterInteractiveMode() {

    std::cout << R"(

  ____________________________________________________________________

 |                                                                    |

 |   _______ ________   __   _____                                    |

 |  |__   __|___  /\ \ / /  / ____|                 _                 |

 |     | |     / /  \ V /  | |     ___  _ __  _ __ | |_ _ __ ___      |

 |     | |    / /    > <   | |    / _ \| '_ \| '_ \| __| '__/ _ \     |

 |     | |   / /__  / . \  | |___| (_) | | | | | | | |_| | | (_) |    |

 |     |_|  /_____|/_/ \_\  \_____\___/|_| |_|_| |_|\__|_|  \___/     |

 |                                                                    |

 |_____________________________________ Powered by TzdEngine _________|

)" << std::endl;

    std::cout << " [System Info]" << std::endl;

    std::cout << "   * 版本号  : 1.0.0 Alpha" << std::endl;

    std::cout << "   * 构建于  : " << __DATE__ << " " << __TIME__ << std::endl;

    std::cout << "   * 开发者  : " << AUTHOR << std::endl;

    std::cout << "   * 架构    : " << (sizeof(void*) == 8 ? "x64 (64-bit)" : "x86 (32-bit)") << std::endl;

    std::cout << "\n [Module Status]" << std::endl;

    std::cout << "   * StackTrace ... [OK]" << std::endl;

    std::cout << "   * MemoryAsm  ... [OK]" << std::endl;

    std::cout << "   * PdbReader  ... [OK]" << std::endl;

    std::cout << "   * GUI System ... [Standby]" << std::endl;

    std::cout << "\n [Interactive Shell]" << std::endl;

    std::cout << "   输入 'help' 查看完整命令列表。" << std::endl;

    std::cout << "   输入 'exit' 退出程序。" << std::endl;

    std::cout << "   支持多行输入 (直到大括号闭合或遇到分号)。" << std::endl;

    std::cout << " --------------------------------------------------------------------\n" << std::endl;

    std::string line;

    int braceCount = 0;

    while (true) {

        if (inputBuffer.empty()) std::cout << "Tzd> ";

        else std::cout << "   > ";

        if (!std::getline(std::cin, line)) break;

        std::string trimLine = line;

        trimLine.erase(0, trimLine.find_first_not_of(" \t\r\n"));

        trimLine.erase(trimLine.find_last_not_of(" \t\r\n") + 1);

        if (trimLine == "exit" || trimLine == "exit;") break;

        bool inQuotes = false;

        for (char c : line) {

            if (c == '"') inQuotes = !inQuotes;

            if (!inQuotes) {

                if (c == '{') braceCount++;

                if (c == '}') braceCount--;

            }

        }

        inputBuffer += line + "\n";

        std::string tempBuffer = inputBuffer;

        size_t lastCharIdx = tempBuffer.find_last_not_of(" \t\r\n");

        bool endsWithSemi = (lastCharIdx != std::string::npos && tempBuffer[lastCharIdx] == ';');

        if (braceCount <= 0 && endsWithSemi) {

            process(inputBuffer);

            inputBuffer = "";

            braceCount = 0;

        }

    }

}

void TzdCommandSystem::start(int argc, char* argv[]) {
    init();

    std::string runMainScript = "";
    bool hasCustomFlags = false;
    bool silentMode = false;
    
    std::string debugHost = "127.0.0.1";
    int debugPort = 0;
    bool enableDebug = false;

    // 清理尾部引号的 Lambda
    auto stripQuotes = [](const std::string& s) -> std::string {
        std::string res = s;
        if (res.size() >= 2 && res.front() == '"' && res.back() == '"') {
            res = res.substr(1, res.size() - 2);
        }
        else if (res.size() >= 2 && res.front() == '\'' && res.back() == '\'') {
            res = res.substr(1, res.size() - 2);
        }
        return res;
        };

    // 参数解析
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // 1. 工作目录: --setProjectDirectory="path" 或 --setpd="path"
        if (arg.rfind("--setProjectDirectory=", 0) == 0 || arg.rfind("--setpd=", 0) == 0) {
            hasCustomFlags = true;
            size_t eqPos = arg.find('=');
            std::string pathVal = stripQuotes(arg.substr(eqPos + 1));
            if (!pathVal.empty()) {
                try {
                    std::filesystem::current_path(pathVal);
                    interpreter->m_includePaths.push_back(std::filesystem::absolute(pathVal).string());
                }
                catch (const std::exception& e) {
                    std::fprintf(stderr, TzdCmd::WORK_DIR_WARN, e.what()); std::cerr << std::endl;
                }
            }
        }
        // 2. 库路径: --addLibraryDirectory="path1","path2"
        else if (arg.rfind("--addLibraryDirectory=", 0) == 0) {
            hasCustomFlags = true;
            size_t eqPos = arg.find('=');
            std::string valList = arg.substr(eqPos + 1);

            // 逗号分隔路径支持
            std::vector<std::string> paths;
            std::string current;
            bool insideQuotes = false;
            for (size_t charIdx = 0; charIdx < valList.size(); ++charIdx) {
                char c = valList[charIdx];
                if (c == '"' || c == '\'') {
                    insideQuotes = !insideQuotes;
                    continue;
                }
                if (c == ',' && !insideQuotes) {
                    if (!current.empty()) {
                        paths.push_back(current);
                        current.clear();
                    }
                }
                else {
                    current += c;
                }
            }
            if (!current.empty()) {
                paths.push_back(current);
            }

            for (const auto& p : paths) {
                std::string cleanPath = stripQuotes(p);
                if (!cleanPath.empty()) {
                    interpreter->m_includePaths.push_back(std::filesystem::absolute(cleanPath).string());
                }
            }
        }
        // 3. 执行单 tzd 文件: --runMainTzd="path"
        else if (arg.rfind("--runMainTzd=", 0) == 0) {
            hasCustomFlags = true;
            size_t eqPos = arg.find('=');
            runMainScript = stripQuotes(arg.substr(eqPos + 1));
        }
        // 4. 静默模式 (隐藏 >>> 10 的输出)
        else if (arg == "--silent" || arg == "-s") {
            silentMode = true;
        }
        // 5. 调试端口/主机
        else if (arg.rfind("--debug-port=", 0) == 0) {
            size_t eqPos = arg.find('=');
            debugPort = std::stoi(arg.substr(eqPos + 1));
            enableDebug = true;
        }
        else if (arg.rfind("--debug-host=", 0) == 0) {
            size_t eqPos = arg.find('=');
            debugHost = stripQuotes(arg.substr(eqPos + 1));
            enableDebug = true;
        }
        else if (arg.rfind("--debug-addr=", 0) == 0) {
            size_t eqPos = arg.find('=');
            std::string addr = stripQuotes(arg.substr(eqPos + 1));
            size_t colon = addr.find(':');
            if (colon != std::string::npos) {
                debugHost = addr.substr(0, colon);
                debugPort = std::stoi(addr.substr(colon + 1));
            } else {
                debugPort = std::stoi(addr);
            }
            enableDebug = true;
        }
    }

    if (silentMode || !runMainScript.empty()) {
        interpreter->m_silentMode = true;
    }

    // 启动调试服务器
    if (enableDebug && debugPort > 0) {
        TzdDebugger::startDebugServer(interpreter, debugHost, debugPort);
    }

    // 文件运行模式
    if (!runMainScript.empty()) {
        try {
            interpreter->loadScriptFromFile(runMainScript);
            
            // 检查是否有 main() 函数并执行
            if (!interpreter->scopes.empty() && interpreter->scopes[0].count("main")) {
                TzdValue mainFunc = interpreter->scopes[0]["main"];
                if (mainFunc.type == TzdValue::FUNCTION) {
                    interpreter->callFunction(mainFunc, {});
                }
            }
        }
        catch (const std::exception& e) {
            std::fprintf(stderr, TzdErr::EXECUTION, e.what()); std::cerr << std::endl;
        }
        return;
    }

    // 非交互模式或带标志的交互模式
    if (hasCustomFlags) {
        enterInteractiveMode();
        return;
    }

    // 原生命令行执行逻辑
    if (argc > 1) {
        std::string cmdLine = "";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            // 过滤配置标志
            if (arg == "-s" || arg == "--silent" ||
                arg.rfind("--debug-port=", 0) == 0 ||
                arg.rfind("--debug-host=", 0) == 0 ||
                arg.rfind("--debug-addr=", 0) == 0 ||
                arg.rfind("--setProjectDirectory=", 0) == 0 ||
                arg.rfind("--setpd=", 0) == 0 ||
                arg.rfind("--addLibraryDirectory=", 0) == 0 ||
                arg.rfind("--runMainTzd=", 0) == 0) 
            {
                continue;
            }
            cmdLine += arg + " ";
        }
        
        std::string trimCmd = cmdLine;
        trimCmd.erase(0, trimCmd.find_first_not_of(" \t\r\n"));
        trimCmd.erase(trimCmd.find_last_not_of(" \t\r\n") + 1);
        
        if (!trimCmd.empty()) {
            process(trimCmd);
        } else {
            enterInteractiveMode();
        }
    }
    else {
        enterInteractiveMode();
    }
}