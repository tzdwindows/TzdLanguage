// ============================================================================
// TzdNativeCodegen.cpp
// AST-to-Native C++ Code Generator Implementation for TzdLang AOT Compiler
// ============================================================================

#include "TzdNativeCodegen.h"
#include <regex>
#include <iostream>

namespace tzd {

TzdNativeCodegen::TzdNativeCodegen() {
}

std::string TzdNativeCodegen::escapeString(const std::string& raw) {
    std::string s;
    s.reserve(raw.size() + 2);
    for (char c : raw) {
        switch (c) {
            case '\\': s += "\\\\"; break;
            case '"':  s += "\\\""; break;
            case '\n': s += "\\n"; break;
            case '\r': s += "\\r"; break;
            case '\t': s += "\\t"; break;
            default:   s += c; break;
        }
    }
    return s;
}

std::string TzdNativeCodegen::sanitizeId(const std::string& id) {
    // Reserved C++ keywords that may appear as Tzd identifiers
    static const std::unordered_set<std::string> k_reserved = {
        "class", "struct", "int", "double", "float", "bool", "char",
        "void", "auto", "default", "case", "switch", "union",
        "register", "volatile", "signed", "unsigned", "template", "typename",
        "namespace", "using", "friend", "virtual", "override", "final",
        "inline", "constexpr", "static_cast", "dynamic_cast", "const_cast"
    };
    if (k_reserved.count(id)) {
        return "tzd_" + id;
    }
    return id;
}

std::string TzdNativeCodegen::exprToStr(antlr4::tree::ParseTree* tree) {
    if (!tree) return "TzdVal()";
    try {
        std::any val = visit(tree);
        if (val.has_value()) {
            if (val.type() == typeid(std::string)) {
                return std::any_cast<std::string>(val);
            }
        }
    } catch (...) {}
    return "TzdVal()";
}

std::string TzdNativeCodegen::generate(TzdLangParser::ProgramContext* tree, const std::string& scriptName, bool cpuOnly) {
    m_cpuOnly = cpuOnly;
    m_forwardDecls.str("");
    m_classDecls.str("");
    m_funcDecls.str("");
    m_topLevelStmts.str("");
    m_declaredClasses.clear();
    m_declaredFuncs.clear();
    m_currentClass.clear();
    m_inClass = false;
    m_inMethod = false;

    visitProgram(tree);

    std::ostringstream out;
    out << "// ============================================================================\n";
    out << "// Auto-generated Native C++ code by TzdLang AOT Machine Code Compiler\n";
    out << "// Source: " << scriptName << "\n";
    out << "// Target: Standalone x86_64 PE Executable (Zero-DLL Native Machine Code)\n";
    out << "// ============================================================================\n\n";

    if (cpuOnly) {
        out << "#define TZD_BUILD_CPU 1\n";
        out << "#define NO_LIBTORCH 1\n";
    }

    out << "#include \"TzdNativeRuntime.hpp\"\n\n";
    out << "using namespace tzd_rt;\n\n";

    // Forward declarations
    out << "// ── Forward Declarations ──\n";
    out << m_forwardDecls.str() << "\n";

    // Class definitions
    out << "// ── Class Definitions ──\n";
    out << m_classDecls.str() << "\n";

    // Global function definitions
    out << "// ── Function Definitions ──\n";
    out << m_funcDecls.str() << "\n";

    // Top-level statements
    out << "// ── Top-Level Program Logic ──\n";
    out << "void tzd_top_level() {\n";
    out << m_topLevelStmts.str();
    out << "}\n\n";

    // Main entry
    out << "// ── Application Entry Point ──\n";
    out << "int main(int argc, char* argv[]) {\n";
    out << "    init_console();\n";
    out << "    std::vector<TzdVal> argv_vec;\n";
    out << "    argv_vec.reserve(argc);\n";
    out << "    for (int i = 0; i < argc; ++i) {\n";
    out << "        argv_vec.push_back(TzdVal(argv[i]));\n";
    out << "    }\n";
    out << "    TzdVal ARGV = tzd_make_array({});\n";
    out << "    ARGV.arrVal = std::make_shared<std::vector<TzdVal>>(argv_vec);\n";
    out << "    TzdVal args = ARGV;\n\n";
    out << "    try {\n";
    out << "        tzd_top_level();\n";
    if (m_declaredFuncs.count("main") || m_declaredFuncs.count("main_func")) {
        out << "        main_func({ARGV});\n";
    }
    out << "    } catch (const TzdVal& ex) {\n";
    out << "        std::cerr << \"[Tzd Runtime Error] \" << ex.to_string() << std::endl;\n";
    out << "        return 1;\n";
    out << "    } catch (const std::exception& ex) {\n";
    out << "        std::cerr << \"[System Exception] \" << ex.what() << std::endl;\n";
    out << "        return 1;\n";
    out << "    } catch (...) {\n";
    out << "        std::cerr << \"[Fatal Error] Unknown uncaught exception.\" << std::endl;\n";
    out << "        return 1;\n";
    out << "    }\n";
    out << "    return 0;\n";
    out << "}\n";

    return out.str();
}

// ---- Program Visitor ----
std::any TzdNativeCodegen::visitProgram(TzdLangParser::ProgramContext* ctx) {
    if (!ctx) return std::string();

    // 1. First pass: Collect forward declarations for classes and functions
    for (auto stmt : ctx->statement()) {
        if (auto classStmt = dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt)) {
            std::string cName = classStmt->classDeclaration()->qualifiedName(0)->getText();
            m_declaredClasses.insert(cName);
            m_forwardDecls << "struct " << cName << "_Instance;\n";
            m_forwardDecls << "TzdVal new_" << cName << "(std::vector<TzdVal> args = {});\n";
        }
        else if (auto funStmt = dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) {
            std::string fName = sanitizeId(funStmt->functionDeclaration()->IDENTIFIER()->getText());
            if (fName == "main") fName = "main_func";
            m_declaredFuncs.insert(fName);
            m_forwardDecls << "TzdVal " << fName << "(std::vector<TzdVal> func_args = {});\n";
        }
    }

    // 2. Second pass: Process statements
    for (auto stmt : ctx->statement()) {
        if (auto classStmt = dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt)) {
            visitClassDeclaration(classStmt->classDeclaration());
        }
        else if (auto funStmt = dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) {
            visitFunctionDeclaration(funStmt->functionDeclaration());
        }
        else {
            std::string code = exprToStr(stmt);
            if (!code.empty()) {
                m_topLevelStmts << "    " << code << "\n";
            }
        }
    }

    return std::string();
}

// ---- Class Declaration ----
std::any TzdNativeCodegen::visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) {
    std::string className = ctx->qualifiedName(0)->getText();
    std::string baseClass = "TzdInstance";
    if (ctx->qualifiedName().size() > 1) {
        baseClass = ctx->qualifiedName(1)->getText() + "_Instance";
    }

    m_currentClass = className;
    m_inClass = true;

    std::ostringstream fields;
    std::ostringstream ctorBody;
    std::vector<std::string> ctorParams;
    bool hasCtor = false;

    std::ostringstream methods;
    std::ostringstream methodDispatch;
    std::vector<std::string> fieldNames;

    if (ctx->classBody()) {
        for (auto member : ctx->classBody()->classMember()) {
            auto mDecl = member->memberDecl();
            if (!mDecl) continue;

            // Fields
            if (auto fVar = dynamic_cast<TzdLangParser::FieldVarDeclContext*>(mDecl)) {
                std::string fName = fVar->IDENTIFIER()->getText();
                fieldNames.push_back(fName);
                fields << "    TzdVal " << fName << ";\n";
            }
            else if (auto fLet = dynamic_cast<TzdLangParser::FieldLetDeclContext*>(mDecl)) {
                std::string fName = fLet->IDENTIFIER()->getText();
                fieldNames.push_back(fName);
                fields << "    TzdVal " << fName << ";\n";
            }
            else if (auto fConst = dynamic_cast<TzdLangParser::FieldConstDeclContext*>(mDecl)) {
                std::string fName = fConst->IDENTIFIER()->getText();
                fieldNames.push_back(fName);
                fields << "    TzdVal " << fName << ";\n";
            }
            // Constructor
            else if (auto ctorCtx = dynamic_cast<TzdLangParser::ConstructorDeclContext*>(mDecl)) {
                hasCtor = true;
                if (ctorCtx->paramList()) {
                    for (auto p : ctorCtx->paramList()->param()) {
                        if (p->IDENTIFIER()) ctorParams.push_back(p->IDENTIFIER()->getText());
                        else ctorParams.push_back(p->getText());
                    }
                }
                m_inMethod = true;
                ctorBody << exprToStr(ctorCtx->block());
                m_inMethod = false;
            }
            // Methods
            else if (auto mMethod = dynamic_cast<TzdLangParser::MethodDeclContext*>(mDecl)) {
                std::string mName = mMethod->IDENTIFIER()->getText();
                std::vector<std::string> mParams;
                if (mMethod->paramList()) {
                    for (auto p : mMethod->paramList()->param()) {
                        if (p->IDENTIFIER()) mParams.push_back(p->IDENTIFIER()->getText());
                        else mParams.push_back(p->getText());
                    }
                }

                m_inMethod = true;
                std::string mBody = exprToStr(mMethod->block());
                m_inMethod = false;

                // Native method signature
                methods << "    TzdVal " << mName << "(std::vector<TzdVal> m_args = {}) {\n";
                for (size_t i = 0; i < mParams.size(); ++i) {
                    methods << "        TzdVal " << mParams[i] << " = m_args.size() > " << i << " ? m_args[" << i << "] : TzdVal();\n";
                }
                methods << "        " << mBody << "\n";
                methods << "        return TzdVal();\n";
                methods << "    }\n\n";

                // Add to dispatch
                methodDispatch << "        if (name == \"" << mName << "\") return " << mName << "(args);\n";
            }
        }
    }

    // Build complete class struct
    m_classDecls << "struct " << className << "_Instance : public " << baseClass << " {\n";
    m_classDecls << fields.str() << "\n";

    // Default constructor
    m_classDecls << "    " << className << "_Instance() {\n";
    m_classDecls << "        className = \"" << className << "\";\n";
    m_classDecls << "    }\n\n";

    // Param constructor
    m_classDecls << "    " << className << "_Instance(std::vector<TzdVal> ctor_args) {\n";
    m_classDecls << "        className = \"" << className << "\";\n";
    for (size_t i = 0; i < ctorParams.size(); ++i) {
        m_classDecls << "        TzdVal " << ctorParams[i] << " = ctor_args.size() > " << i << " ? ctor_args[" << i << "] : TzdVal();\n";
    }
    if (hasCtor) {
        m_classDecls << "        " << ctorBody.str() << "\n";
    }
    m_classDecls << "    }\n\n";

    // Field getter
    m_classDecls << "    TzdVal get_field(const std::string& name) override {\n";
    for (const auto& fn : fieldNames) {
        m_classDecls << "        if (name == \"" << fn << "\") return " << fn << ";\n";
    }
    m_classDecls << "        return " << baseClass << "::get_field(name);\n";
    m_classDecls << "    }\n\n";

    // Field setter
    m_classDecls << "    void set_field(const std::string& name, const TzdVal& val) override {\n";
    for (const auto& fn : fieldNames) {
        m_classDecls << "        if (name == \"" << fn << "\") { " << fn << " = val; return; }\n";
    }
    m_classDecls << "        " << baseClass << "::set_field(name, val);\n";
    m_classDecls << "    }\n\n";

    // Method dispatch
    m_classDecls << "    TzdVal call_method(const std::string& name, std::vector<TzdVal> args) override {\n";
    m_classDecls << methodDispatch.str();
    m_classDecls << "        return " << baseClass << "::call_method(name, args);\n";
    m_classDecls << "    }\n\n";

    // Method bodies
    m_classDecls << methods.str();

    m_classDecls << "};\n\n";

    // Factory helper
    m_classDecls << "inline TzdVal new_" << className << "(std::vector<TzdVal> args) {\n";
    m_classDecls << "    return TzdVal(std::make_shared<" << className << "_Instance>(args));\n";
    m_classDecls << "}\n\n";

    m_inClass = false;
    m_currentClass.clear();
    return std::string();
}

// ---- Function Declaration ----
std::any TzdNativeCodegen::visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) {
    std::string funcName = sanitizeId(ctx->IDENTIFIER()->getText());
    if (funcName == "main") funcName = "main_func";

    std::vector<std::string> params;
    if (ctx->paramList()) {
        for (auto p : ctx->paramList()->param()) {
            if (p->IDENTIFIER()) params.push_back(p->IDENTIFIER()->getText());
            else params.push_back(p->getText());
        }
    }

    std::string body = exprToStr(ctx->block());

    m_funcDecls << "TzdVal " << funcName << "(std::vector<TzdVal> func_args) {\n";
    for (size_t i = 0; i < params.size(); ++i) {
        m_funcDecls << "    TzdVal " << params[i] << " = func_args.size() > " << i << " ? func_args[" << i << "] : TzdVal();\n";
    }
    m_funcDecls << "    " << body << "\n";
    m_funcDecls << "    return TzdVal();\n";
    m_funcDecls << "}\n\n";

    return std::string();
}

// ---- Statement Visitors ----
std::any TzdNativeCodegen::visitBlock(TzdLangParser::BlockContext* ctx) {
    if (!ctx) return std::string("{}");
    std::ostringstream ss;
    ss << "{\n";
    for (auto stmt : ctx->statement()) {
        std::string s = exprToStr(stmt);
        if (!s.empty()) {
            ss << "        " << s << "\n";
        }
    }
    ss << "    }";
    return ss.str();
}

std::any TzdNativeCodegen::visitBlockStmt(TzdLangParser::BlockStmtContext* ctx) {
    return visitBlock(ctx->block());
}

std::any TzdNativeCodegen::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) {
    return visitVariableDeclaration(ctx->variableDeclaration());
}

std::any TzdNativeCodegen::visitVariableDeclaration(TzdLangParser::VariableDeclarationContext* ctx) {
    std::string varName = ctx->IDENTIFIER()->getText();
    std::string init = "TzdVal()";
    if (ctx->expression()) {
        init = exprToStr(ctx->expression());
    }
    return "TzdVal " + varName + " = " + init + ";";
}

std::any TzdNativeCodegen::visitIfStmt(TzdLangParser::IfStmtContext* ctx) {
    std::string cond = exprToStr(ctx->expression());
    std::string thenStmt = exprToStr(ctx->statement(0));
    std::string res = "if (" + cond + ".as_bool()) " + thenStmt;
    if (ctx->statement().size() > 1) {
        res += " else " + exprToStr(ctx->statement(1));
    }
    return res;
}

std::any TzdNativeCodegen::visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) {
    std::string cond = exprToStr(ctx->expression());
    std::string body = exprToStr(ctx->statement());
    return "while (" + cond + ".as_bool()) " + body;
}

std::any TzdNativeCodegen::visitForStmt(TzdLangParser::ForStmtContext* ctx) {
    std::string init = ctx->forInit() ? exprToStr(ctx->forInit()) : "";
    std::string cond = ctx->cond ? (exprToStr(ctx->cond) + ".as_bool()") : "true";
    std::string step = ctx->step ? exprToStr(ctx->step) : "";
    std::string body = exprToStr(ctx->statement());

    // Clean trailing semicolon from init
    if (!init.empty() && init.back() == ';') init.pop_back();

    return "for (" + init + "; " + cond + "; " + step + ") " + body;
}

std::any TzdNativeCodegen::visitForInit(TzdLangParser::ForInitContext* ctx) {
    if (ctx->variableDeclaration()) return visitVariableDeclaration(ctx->variableDeclaration());
    if (ctx->expression()) return exprToStr(ctx->expression());
    return std::string();
}

std::any TzdNativeCodegen::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    if (ctx->expression()) {
        return "return " + exprToStr(ctx->expression()) + ";";
    }
    return std::string("return TzdVal();");
}

std::any TzdNativeCodegen::visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) {
    return std::string("break;");
}

std::any TzdNativeCodegen::visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) {
    return std::string("continue;");
}

std::any TzdNativeCodegen::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) {
    std::string e = exprToStr(ctx->expression());
    return e + ";";
}

std::any TzdNativeCodegen::visitEmptyStmt(TzdLangParser::EmptyStmtContext* ctx) {
    return std::string(";");
}

std::any TzdNativeCodegen::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) {
    std::string tryBlock = exprToStr(ctx->block(0));
    std::string catchVar = ctx->IDENTIFIER()->getText();
    std::string catchBlock = exprToStr(ctx->block(1));
    return "try " + tryBlock + " catch (const TzdVal& " + catchVar + ") " + catchBlock;
}

std::any TzdNativeCodegen::visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) {
    return "throw " + exprToStr(ctx->expression()) + ";";
}

std::any TzdNativeCodegen::visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) {
    std::string val = exprToStr(ctx->expression());
    std::ostringstream ss;
    ss << "{\n";
    ss << "    TzdVal _switch_val = " << val << ";\n";
    bool first = true;
    for (auto c : ctx->switchCase()) {
        std::string caseVal = exprToStr(c->expression());
        if (!first) ss << " else ";
        first = false;
        ss << "if (_switch_val == " << caseVal << ") {\n";
        for (auto s : c->statement()) ss << "        " << exprToStr(s) << "\n";
        ss << "    }";
    }
    if (ctx->switchDefault()) {
        if (!first) ss << " else ";
        ss << "{\n";
        for (auto s : ctx->switchDefault()->statement()) ss << "        " << exprToStr(s) << "\n";
        ss << "    }\n";
    }
    ss << "}";
    return ss.str();
}

std::any TzdNativeCodegen::visitSwitchCase(TzdLangParser::SwitchCaseContext* ctx) {
    return std::string();
}

std::any TzdNativeCodegen::visitSwitchDefault(TzdLangParser::SwitchDefaultContext* ctx) {
    return std::string();
}

std::any TzdNativeCodegen::visitImportStmt(TzdLangParser::ImportStmtContext* ctx) {
    std::string path = ctx->importStatement() && ctx->importStatement()->STRING() ? ctx->importStatement()->STRING()->getText() : "";
    return std::string("// import: " + path);
}

// ---- Expression Visitors ----

std::any TzdNativeCodegen::visitIntExpr(TzdLangParser::IntExprContext* ctx) {
    return "TzdVal(" + ctx->INTEGER()->getText() + "LL)";
}

std::any TzdNativeCodegen::visitFloatExpr(TzdLangParser::FloatExprContext* ctx) {
    return "TzdVal(" + ctx->FLOAT()->getText() + ")";
}

std::any TzdNativeCodegen::visitStringExpr(TzdLangParser::StringExprContext* ctx) {
    std::string text = ctx->STRING()->getText();
    if (text.size() >= 2 && (text.front() == '"' || text.front() == '\'') && text.front() == text.back()) {
        text = text.substr(1, text.size() - 2);
    }
    return "TzdVal(\"" + escapeString(text) + "\")";
}

std::any TzdNativeCodegen::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) {
    return std::string("TzdVal(true)");
}

std::any TzdNativeCodegen::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) {
    return std::string("TzdVal(false)");
}

std::any TzdNativeCodegen::visitNullExpr(TzdLangParser::NullExprContext* ctx) {
    return std::string("TzdVal()");
}

std::any TzdNativeCodegen::visitIdExpr(TzdLangParser::IdExprContext* ctx) {
    std::string id = ctx->IDENTIFIER()->getText();
    if (id == "this") {
        return std::string("this");
    }
    return sanitizeId(id);
}

std::any TzdNativeCodegen::visitParenExpr(TzdLangParser::ParenExprContext* ctx) {
    return "(" + exprToStr(ctx->expression()) + ")";
}

std::any TzdNativeCodegen::visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) {
    std::ostringstream ss;
    ss << "tzd_make_array({";
    if (ctx->exprList()) {
        auto exprs = ctx->exprList()->expression();
        for (size_t i = 0; i < exprs.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << exprToStr(exprs[i]);
        }
    }
    ss << "})";
    return ss.str();
}

std::any TzdNativeCodegen::visitSuperExpr(TzdLangParser::SuperExprContext* ctx) {
    return std::string("/* super */");
}

std::any TzdNativeCodegen::visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) {
    return visitPrintFunction(ctx->printFunction());
}

std::any TzdNativeCodegen::visitPrintFunction(TzdLangParser::PrintFunctionContext* ctx) {
    std::ostringstream ss;
    ss << "tzd_print_vec({";
    if (ctx->exprList()) {
        auto exprs = ctx->exprList()->expression();
        for (size_t i = 0; i < exprs.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << exprToStr(exprs[i]);
        }
    }
    ss << "})";
    return ss.str();
}

std::any TzdNativeCodegen::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) {
    std::string obj = exprToStr(ctx->atom());
    std::string member = ctx->IDENTIFIER()->getText();
    if (obj == "this") {
        return "this->get_field(\"" + member + "\")";
    }
    return obj + ".get_member(\"" + member + "\")";
}

std::any TzdNativeCodegen::visitIndexExpr(TzdLangParser::IndexExprContext* ctx) {
    std::string obj = exprToStr(ctx->expression(0));
    std::string idx = exprToStr(ctx->expression(1));
    return obj + ".get_index(" + idx + ")";
}

std::any TzdNativeCodegen::visitCallExpr(TzdLangParser::CallExprContext* ctx) {
    std::vector<std::string> args;
    if (ctx->exprList()) {
        for (auto e : ctx->exprList()->expression()) {
            args.push_back(exprToStr(e));
        }
    }
    std::ostringstream argsStr;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) argsStr << ", ";
        argsStr << args[i];
    }

    // Check if callee is member access: obj.method(args)
    if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(ctx->atom())) {
        std::string obj = exprToStr(memCtx->atom());
        std::string method = memCtx->IDENTIFIER()->getText();
        if (obj == "this") {
            return "this->call_method(\"" + method + "\", {" + argsStr.str() + "})";
        }
        return obj + ".call_method(\"" + method + "\", {" + argsStr.str() + "})";
    }

    std::string callee = exprToStr(ctx->atom());

    // Standard builtins mapping
    static const std::unordered_set<std::string> builtins = {
        "print", "println", "len", "str", "toString", "int", "toInt", "parseInt",
        "float", "toFloat", "parseDouble", "bool", "toBool", "type", "isNone", "isNull",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
        "sqrt", "cbrt", "pow", "exp", "log", "log10", "log2", "abs", "floor", "ceil", "round", "trunc",
        "min", "max", "clamp", "random", "randInt", "factorial",
        "time", "clock", "sleep", "exit", "assert", "input",
        "push", "pop", "insert", "remove", "clear", "contains", "indexOf", "slice", "join", "reverse", "sort",
        "readFile", "writeFile", "appendFile", "fileExists", "removeFile",
        "keys", "values", "hasKey",
        "torch_tensor", "torch_zeros", "torch_ones", "torch_randn", "torch_empty",
        "torch_matmul", "torch_add", "torch_sub", "torch_mul", "torch_div",
        "torch_sigmoid", "torch_relu", "torch_softmax", "torch_mean", "torch_sum",
        "torch_scalar_value", "torch_shape", "torch_is_tensor", "torch_cuda_is_available"
    };

    if (builtins.count(callee)) {
        return "tzd_builtin_" + callee + "({" + argsStr.str() + "})";
    }

    // Class instantiation without new: Point(x, y)
    if (m_declaredClasses.count(callee)) {
        return "new_" + callee + "({" + argsStr.str() + "})";
    }

    // Regular function call
    if (callee == "main") callee = "main_func";
    return callee + "({" + argsStr.str() + "})";
}

std::any TzdNativeCodegen::visitNewExpr(TzdLangParser::NewExprContext* ctx) {
    std::string className = ctx->qualifiedName()->getText();
    std::ostringstream argsStr;
    if (ctx->exprList()) {
        auto exprs = ctx->exprList()->expression();
        for (size_t i = 0; i < exprs.size(); ++i) {
            if (i > 0) argsStr << ", ";
            argsStr << exprToStr(exprs[i]);
        }
    }
    return "new_" + className + "({" + argsStr.str() + "})";
}

std::any TzdNativeCodegen::visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) {
    std::vector<std::string> params;
    if (ctx->paramList()) {
        for (auto p : ctx->paramList()->param()) {
            if (p->IDENTIFIER()) params.push_back(p->IDENTIFIER()->getText());
            else params.push_back(p->getText());
        }
    }
    std::string body = exprToStr(ctx->block());

    std::ostringstream ss;
    ss << "TzdVal([=](std::vector<TzdVal> l_args) -> TzdVal {\n";
    for (size_t i = 0; i < params.size(); ++i) {
        ss << "        TzdVal " << params[i] << " = l_args.size() > " << i << " ? l_args[" << i << "] : TzdVal();\n";
    }
    ss << "        " << body << "\n";
    ss << "        return TzdVal();\n";
    ss << "    })";
    return ss.str();
}

std::any TzdNativeCodegen::visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string op = ctx->PLUS() ? "+" : "-";
    std::string right = exprToStr(ctx->expression(1));
    return "(" + left + " " + op + " " + right + ")";
}

std::any TzdNativeCodegen::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string op = "*";
    if (ctx->DIV()) op = "/";
    else if (ctx->MOD()) op = "%";
    std::string right = exprToStr(ctx->expression(1));
    return "(" + left + " " + op + " " + right + ")";
}

std::any TzdNativeCodegen::visitPowerExpr(TzdLangParser::PowerExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string right = exprToStr(ctx->expression(1));
    return left + ".pow(" + right + ")";
}

std::any TzdNativeCodegen::visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string right = exprToStr(ctx->expression(1));
    std::string op = "<";
    if (ctx->GT()) op = ">";
    else if (ctx->LE()) op = "<=";
    else if (ctx->GE()) op = ">=";
    return "TzdVal(" + left + " " + op + " " + right + ")";
}

std::any TzdNativeCodegen::visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string right = exprToStr(ctx->expression(1));
    std::string op = ctx->EEQ() ? "==" : "!=";
    return "TzdVal(" + left + " " + op + " " + right + ")";
}

std::any TzdNativeCodegen::visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string right = exprToStr(ctx->expression(1));
    return "TzdVal(" + left + ".as_bool() && " + right + ".as_bool())";
}

std::any TzdNativeCodegen::visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) {
    std::string left = exprToStr(ctx->expression(0));
    std::string right = exprToStr(ctx->expression(1));
    return "TzdVal(" + left + ".as_bool() || " + right + ".as_bool())";
}

std::any TzdNativeCodegen::visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) {
    std::string expr = exprToStr(ctx->expression());
    if (ctx->MINUS()) return "(-" + expr + ")";
    if (ctx->NOT()) return "(!" + expr + ")";
    return expr;
}

std::any TzdNativeCodegen::visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) {
    std::string expr = exprToStr(ctx->expression());
    std::string op = ctx->INC() ? "++" : "--";
    return "(" + op + expr + ")";
}

std::any TzdNativeCodegen::visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) {
    std::string expr = exprToStr(ctx->expression());
    std::string op = ctx->INC() ? "++" : "--";
    return "(" + expr + op + ")";
}

std::any TzdNativeCodegen::visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) {
    auto lhsCtx = ctx->expression(0);
    auto rhsCtx = ctx->expression(1);
    std::string rhs = exprToStr(rhsCtx);

    // Member assignment: obj.member = rhs
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomExpr->atom())) {
            std::string obj = exprToStr(memCtx->atom());
            std::string member = memCtx->IDENTIFIER()->getText();
            if (obj == "this") {
                return "this->set_field(\"" + member + "\", " + rhs + ")";
            }
            return obj + ".set_member(\"" + member + "\", " + rhs + ")";
        }
    }

    // Index assignment: obj[idx] = rhs
    if (auto idxCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhsCtx)) {
        std::string obj = exprToStr(idxCtx->expression(0));
        std::string idx = exprToStr(idxCtx->expression(1));
        return obj + ".set_index(" + idx + ", " + rhs + ")";
    }

    std::string lhs = exprToStr(lhsCtx);
    std::string op = "=";
    if (ctx->PLUS_ASSIGN()) op = "+=";
    else if (ctx->MIN_ASSIGN()) op = "-=";
    else if (ctx->MUL_ASSIGN()) op = "*=";
    else if (ctx->DIV_ASSIGN()) op = "/=";

    return "(" + lhs + " " + op + " " + rhs + ")";
}

std::any TzdNativeCodegen::visitCastExpr(TzdLangParser::CastExprContext* ctx) {
    return exprToStr(ctx->expression());
}

std::any TzdNativeCodegen::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) {
    return std::string("TzdVal(true)");
}

} // namespace tzd
