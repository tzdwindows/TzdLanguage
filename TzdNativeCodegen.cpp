// ============================================================================
// TzdNativeCodegen.cpp
// AST-to-Native C++ Code Generator Implementation for TzdLang AOT Compiler
// ============================================================================

#include "TzdNativeCodegen.h"
#include "Generated/TzdLangLexer.h"
#include "Generated/TzdLangParser.h"
#include <fstream>
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

std::string TzdNativeCodegen::generate(
    TzdLangParser::ProgramContext* tree,
    const std::string& scriptName,
    bool cpuOnly,
    const std::vector<std::string>& importedFiles)
{
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

    // Parse and process all imported .tzd library files first
    std::vector<std::unique_ptr<antlr4::ANTLRInputStream>> inputStreams;
    std::vector<std::unique_ptr<TzdLangLexer>> lexers;
    std::vector<std::unique_ptr<antlr4::CommonTokenStream>> tokenStreams;
    std::vector<std::unique_ptr<TzdLangParser>> parsers;

    for (const auto& impPath : importedFiles) {
        std::ifstream f(impPath);
        if (f.is_open()) {
            std::stringstream ss;
            ss << f.rdbuf();
            std::string impCode = ss.str();
            auto input = std::make_unique<antlr4::ANTLRInputStream>(impCode);
            auto lexer = std::make_unique<TzdLangLexer>(input.get());
            auto tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
            tokens->fill();
            auto parser = std::make_unique<TzdLangParser>(tokens.get());
            auto* impTree = parser->program();
            if (parser->getNumberOfSyntaxErrors() == 0 && impTree) {
                visitProgram(impTree);
            }
            inputStreams.push_back(std::move(input));
            lexers.push_back(std::move(lexer));
            tokenStreams.push_back(std::move(tokens));
            parsers.push_back(std::move(parser));
        }
    }

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
    std::vector<std::string> constFieldNames;

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
                constFieldNames.push_back(fName);
                std::string init = "TzdVal()";
                if (fConst->expression()) init = exprToStr(fConst->expression());
                fields << "    inline static const TzdVal " << fName << " = " << init << ";\n";
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
            // Static Methods (static fun)
            else if (auto smMethod = dynamic_cast<TzdLangParser::MethodStaticDeclContext*>(mDecl)) {
                std::string mName = smMethod->IDENTIFIER()->getText();
                std::vector<std::string> mParams;
                if (smMethod->paramList()) {
                    for (auto p : smMethod->paramList()->param()) {
                        if (p->IDENTIFIER()) mParams.push_back(p->IDENTIFIER()->getText());
                        else mParams.push_back(p->getText());
                    }
                }

                m_inMethod = true;
                std::string mBody = exprToStr(smMethod->block());
                m_inMethod = false;

                methods << "    static TzdVal " << mName << "(std::vector<TzdVal> m_args = {}) {\n";
                for (size_t i = 0; i < mParams.size(); ++i) {
                    methods << "        TzdVal " << mParams[i] << " = m_args.size() > " << i << " ? m_args[" << i << "] : TzdVal();\n";
                }
                methods << "        " << mBody << "\n";
                methods << "        return TzdVal();\n";
                methods << "    }\n\n";
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
    for (const auto& fn : constFieldNames) {
        m_classDecls << "        if (name == \"" << fn << "\") return " << fn << ";\n";
    }
    m_classDecls << "        return " << baseClass << "::get_field(name);\n";
    m_classDecls << "    }\n\n";

    // Field setter
    m_classDecls << "    void set_field(const std::string& name, const TzdVal& val) override {\n";
    for (const auto& fn : fieldNames) {
        m_classDecls << "        if (name == \"" << fn << "\") { " << fn << " = val; return; }\n";
    }
    for (const auto& fn : constFieldNames) {
        m_classDecls << "        if (name == \"" << fn << "\") { return; }\n";
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
    if (ctx->expression()) {
        if (auto assignCtx = dynamic_cast<TzdLangParser::AssignmentExprContext*>(ctx->expression())) {
            if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(assignCtx->expression(0))) {
                if (auto idExpr = dynamic_cast<TzdLangParser::IdExprContext*>(atomExpr->atom())) {
                    std::string varName = idExpr->IDENTIFIER()->getText();
                    std::string rhs = exprToStr(assignCtx->expression(1));
                    return "TzdVal " + varName + " = " + rhs;
                }
            }
        }
        return exprToStr(ctx->expression());
    }
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
    if (m_declaredClasses.count(obj)) {
        return obj + "_Instance::" + member;
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
        if (m_declaredClasses.count(obj)) {
            return obj + "_Instance::" + method + "({" + argsStr.str() + "})";
        }
        return obj + ".call_method(\"" + method + "\", {" + argsStr.str() + "})";
    }

    std::string callee = exprToStr(ctx->atom());

    // Class instantiation without new: Point(x, y)
    if (m_declaredClasses.count(callee)) {
        return "new_" + callee + "({" + argsStr.str() + "})";
    }

    // Local user function
    if (m_declaredFuncs.count(callee) || (callee == "main" && m_declaredFuncs.count("main_func"))) {
        if (callee == "main") callee = "main_func";
        return callee + "({" + argsStr.str() + "})";
    }

    // Comprehensive built-in and standard library function registry (552 functions)
    static const std::unordered_set<std::string> builtins = {
        "E", "EPSILON", "GOLDEN_RATIO", "INF", "NAN", "PI", "Runtime", "SQRT2",
        "TAU", "abs", "acos", "addIncludePath", "appendFile", "argmax", "argmin", "asin",
        "assert", "assert_t", "atan", "atan2", "avg", "base64Decode", "base64Encode", "bigint",
        "bigintFactorial", "bigintGcd", "bit", "bool", "cbrt", "ceil", "changeDir", "charAt",
        "charCode", "clamp", "clear", "clock", "comb", "concat", "contains", "copyFile",
        "cos", "cosh", "countSubstr", "crc32", "cumsum", "currentDir", "dateDiff", "dateParts",
        "deepCopy", "degrees", "derivative", "det", "diff", "dirExists", "dot", "endsWith",
        "erf", "escape", "exit", "exp", "expm1", "factorial", "fib", "fileExists",
        "fileSize", "fill", "filter", "find", "flatten", "float", "floor", "format",
        "formatTime", "fromBinary", "fromCharCode", "fromHex", "fromJSON", "gcd", "getArraysInfo", "getBigIntMaxDigits",
        "getClassInfo", "getEnv", "getFunctions", "getNativeFunctions", "getOsInfo", "getScriptDir", "getScriptPath", "getSymbols",
        "has", "hasKey", "hash", "hexDump", "hypot", "identity", "includes", "indexOf",
        "indexOfArr", "input", "insert", "int", "inverse", "isBigint", "isFinite", "isNaN",
        "isNone", "isNull", "isPowerOf2", "isPrime", "isfinite_t", "isinf_t", "isnan_t", "join",
        "jsonParse", "jsonStringify", "keys", "lcm", "len", "lerp", "levenshtein", "linspace_arr",
        "listDir", "log", "log10", "log1p", "log2", "logBase", "makeDir", "map",
        "mapEntries", "mapFilter", "mapFromEntries", "mapGet", "mapHas", "mapKeys", "mapMap", "mapMerge",
        "mapValues", "match", "matrixMul", "max", "maxArr", "measure", "min", "minArr",
        "moveFile", "nextPowerOf2", "norm", "now", "ones", "padLeft", "padRight", "parseDouble",
        "parseFloat", "parseInt", "perm", "plot", "pop", "pow", "powmod", "print",
        "println", "push", "queuePop", "queuePopAll", "queuePush", "radians", "randInt", "random",
        "randomInt", "randomSeed", "range", "rank", "rational", "rationalAdd", "rationalMul", "readFile",
        "readLines", "reduce", "remove", "removeDir", "removeFile", "repeat", "replace", "replaceRegex",
        "reshape", "reverse", "reverseStr", "round", "sample", "setAdd", "setBigIntMaxDigits", "setContains",
        "setCreate", "setDifference", "setEnv", "setIntersect", "setRemove", "setSize", "setUnion", "shift",
        "shuffle", "sign", "simplifySym", "sin", "sinh", "sleep", "slice", "solve",
        "solveEq", "solveIneq", "solveSym", "sort", "split", "splitRegex", "sqrt", "stackPop",
        "stackPush", "startsWith", "str", "substr", "substring", "sum", "sys_thread_detach", "sys_thread_join",
        "sys_thread_start", "tan", "tanh", "tgamma", "time", "timestamp", "toBinary", "toBool",
        "toCamelCase", "toFixed", "toFloat", "toFraction", "toHex", "toInt", "toJSON", "toLower",
        "toPrecision", "toSnakeCase", "toString", "toTitleCase", "toUpper", "torch_abs", "torch_adagrad", "torch_adam",
        "torch_adamax", "torch_adamw", "torch_adaptive_avg_pool1d", "torch_adaptive_avg_pool2d", "torch_add", "torch_add_", "torch_all", "torch_allclose",
        "torch_any", "torch_arange", "torch_argmax", "torch_argmin", "torch_argsort", "torch_atan2_t", "torch_auto_cleanup", "torch_avg_pool2d",
        "torch_backward", "torch_batch_norm", "torch_batch_norm1d", "torch_batch_norm2d", "torch_bce_loss", "torch_bernoulli", "torch_bincount", "torch_bmm",
        "torch_broadcast_shapes", "torch_broadcast_tensors", "torch_broadcast_to", "torch_cat", "torch_chain_matmul", "torch_cholesky", "torch_chunk", "torch_clamp",
        "torch_clamp_", "torch_clip_grad_norm", "torch_clip_grad_value", "torch_clone", "torch_contiguous", "torch_conv1d", "torch_conv2d", "torch_conv_transpose2d",
        "torch_copy_", "torch_corrcoef", "torch_cosine_similarity", "torch_count_nonzero", "torch_count_params", "torch_cov", "torch_create_param", "torch_cross_entropy",
        "torch_cuda_is_available", "torch_cuda_max_memory_allocated", "torch_cuda_memory_allocated", "torch_cuda_memory_reserved", "torch_cuda_reset_peak_memory", "torch_cuda_synchronize", "torch_cumprod", "torch_cumsum",
        "torch_current_device", "torch_dequantize", "torch_det", "torch_det_t", "torch_detach", "torch_device_count", "torch_device_str", "torch_diag",
        "torch_diagflat", "torch_digamma", "torch_dim", "torch_div", "torch_div_", "torch_dropout", "torch_dtype", "torch_dtype_str",
        "torch_eig", "torch_element_size", "torch_elu", "torch_embedding", "torch_empty", "torch_empty_cache", "torch_empty_like", "torch_eq",
        "torch_equal", "torch_erf", "torch_erfc", "torch_exp", "torch_expand", "torch_eye", "torch_fill_", "torch_flatten",
        "torch_flatten_t", "torch_fmod", "torch_from_array", "torch_full", "torch_full_like", "torch_fused_linear_bias_gelu", "torch_fused_residual_layernorm", "torch_fused_silu_mul",
        "torch_fused_softmax_mask", "torch_gather", "torch_gc", "torch_ge", "torch_gelu", "torch_get_num_threads", "torch_glu", "torch_grad",
        "torch_grad_fn", "torch_gt", "torch_hardswish", "torch_hardtanh", "torch_histc", "torch_identity", "torch_index_copy_", "torch_index_put",
        "torch_index_select", "torch_init_kaiming", "torch_init_normal", "torch_init_ones", "torch_init_uniform", "torch_init_xavier", "torch_init_zeros", "torch_interpolate",
        "torch_inv", "torch_inverse", "torch_inverse_t", "torch_is_contiguous", "torch_is_floating_point", "torch_is_grad_enabled", "torch_is_integer", "torch_is_leaf",
        "torch_is_pinned", "torch_is_requires_grad", "torch_is_tensor", "torch_isfinite", "torch_isinf", "torch_isnan", "torch_item", "torch_jit_eval",
        "torch_jit_load", "torch_jit_save", "torch_jit_train", "torch_kl_div", "torch_l1_loss", "torch_layer_norm", "torch_le", "torch_leaky_relu",
        "torch_lerp", "torch_lgamma", "torch_linear", "torch_linspace", "torch_load", "torch_load_state_dict", "torch_log", "torch_log_softmax",
        "torch_logcumsumexp", "torch_logical_and", "torch_logical_not", "torch_logical_or", "torch_logspace", "torch_logsumexp", "torch_lstsq", "torch_lt",
        "torch_make_contiguous", "torch_manual_seed", "torch_masked_fill", "torch_masked_fill_", "torch_masked_select", "torch_matmul", "torch_matrix_exp", "torch_max_pool2d",
        "torch_max_t", "torch_mean", "torch_median", "torch_memory_allocated", "torch_memory_allocated_str", "torch_min_t", "torch_mish", "torch_mm",
        "torch_mse_loss", "torch_mul", "torch_mul_", "torch_multinomial", "torch_nadam", "torch_nbytes", "torch_ne", "torch_neg",
        "torch_nll_loss", "torch_no_grad", "torch_no_grad_scope", "torch_nonzero", "torch_norm_t", "torch_num_tensors", "torch_numel", "torch_one_hot",
        "torch_ones", "torch_ones_like", "torch_optim_delete", "torch_optim_step", "torch_optim_zero_grad", "torch_optimizer_create", "torch_orth", "torch_pad",
        "torch_pairwise_distance", "torch_pca", "torch_permute", "torch_pow", "torch_prelu", "torch_print", "torch_prod", "torch_q_scale",
        "torch_q_zero_point", "torch_quantize_per_channel", "torch_quantize_per_tensor", "torch_rand", "torch_randint", "torch_randint_like", "torch_randn", "torch_randperm",
        "torch_release_all", "torch_release_tensor", "torch_relu", "torch_remainder", "torch_repeat", "torch_requires_grad", "torch_requires_grad_params", "torch_reshape",
        "torch_rmsprop", "torch_save", "torch_save_state_dict", "torch_scalar_value", "torch_scatter", "torch_scatter_", "torch_selu", "torch_set_device",
        "torch_set_grad_enabled", "torch_set_num_threads", "torch_sgd", "torch_shape", "torch_sigmoid", "torch_sigmoid_fn", "torch_silu", "torch_smooth_l1_loss",
        "torch_softmax", "torch_softmin", "torch_softplus", "torch_solve", "torch_solve_t", "torch_sort_t", "torch_split_t", "torch_sqrt",
        "torch_squeeze", "torch_stack", "torch_std", "torch_std_mean", "torch_sub", "torch_sub_", "torch_sum", "torch_svd",
        "torch_tensor", "torch_threshold", "torch_to_array", "torch_to_bool", "torch_to_cpu", "torch_to_cuda", "torch_to_device", "torch_to_double",
        "torch_to_dtype", "torch_to_float", "torch_to_int", "torch_to_long", "torch_to_string", "torch_topk", "torch_trace_t", "torch_transpose",
        "torch_tril", "torch_triple_margin_loss", "torch_triu", "torch_unique", "torch_unsqueeze", "torch_upsample_bilinear2d", "torch_upsample_nearest2d", "torch_var",
        "torch_var_mean", "torch_version", "torch_view", "torch_where", "torch_zero_", "torch_zero_grad_params", "torch_zeros", "torch_zeros_like",
        "trace", "transpose", "trim", "trunc", "type", "unescape", "unique", "unshift",
        "uuid", "values", "warn", "wordCount", "writeFile", "writeLines", "zeros", "zip"
    };

    if (builtins.count(callee) || callee.rfind("torch_", 0) == 0 || callee.rfind("bigint", 0) == 0) {
        return "tzd_builtin_" + callee + "({" + argsStr.str() + "})";
    }

    // Otherwise, callable variable or function object
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
