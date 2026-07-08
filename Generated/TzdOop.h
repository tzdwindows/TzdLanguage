#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

#include "TzdLangParser.h"
#include "TzdFFIAdapter.h"

struct TzdValue;
class TzdInterpreter;

using JittedFunc = void (*)(void*, void*);

struct ClassField {
    std::string name;
    std::string type; // int, float, etc.
    bool isStatic = false;
    bool isConst = false;
    TzdLangParser::ExpressionContext* initExpr = nullptr;
    struct TzdValue* constValue = nullptr;
};

// ??????????
struct ClassConstructor {
    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    int paramCount = 0;
    TzdLangParser::BlockContext* body = nullptr;
    TzdLangParser::ConstructorDeclContext* declCtx = nullptr;
    void (*jittedPtr)(void*, void*) = nullptr;
    std::string jitSymbolName;

    std::string sourceFile = "";
    int line = 0;
    int column = 0;
};

// ????????
struct ClassMethod {
    
    std::string name;
    bool isStatic = false;
    bool isNative = false;
    TzdFFIAdapter::WrapperFunc nativeWrapper;
    bool isAbstract = false;
    bool isAnnotation = false;
    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    void (*jittedPtr)(void*, void*) = nullptr;
    TzdLangParser::BlockContext* body = nullptr;

    std::vector<std::string> annotations;

    std::string sourceFile = "";
    int line = 0;
    int column = 0;
};

// ???? (Class Definition)
class TzdClassDef {
public:
    std::string name;
    std::string fullName;
    std::string packageName;
    std::string simpleName;

    std::string parentName;
    std::vector<TzdClassDef*> derivedClasses;
    bool isAnnotation = false;
    bool isEnum = false;

    std::unordered_map<std::string, struct ClassField> fields;
    std::unordered_map<std::string, struct ClassMethod> methods;
    std::unordered_map<std::string, struct TzdValue> staticValues;
    std::vector<ClassConstructor> constructors;
    std::vector<std::string> annotations;

    ClassConstructor* findConstructor(size_t argCount);
    const ClassConstructor* findConstructor(size_t argCount) const;

    // ?????????????
    TzdClassDef(const std::string& fqn);
    virtual ~TzdClassDef() {}

    bool isSubclassOf(const std::string& targetParentName);
    void getAllSubclasses(std::vector<TzdClassDef*>& outSubclasses);

    struct ClassMethod* findMethod(const std::string& methodName);
    struct ClassField* findField(const std::string& fieldName);
    struct TzdValue* findStaticValue(const std::string& fieldName);

private:
    std::unordered_map<std::string, struct ClassMethod*> methodCache;
    std::unordered_map<std::string, struct ClassField*> fieldCache;
};

// ????? (Object Instance)
class TzdInstance {
public:
    TzdClassDef* definition;
    std::unordered_map<std::string, struct TzdValue> fields; // ???????

    TzdInstance(TzdClassDef* def);

    struct TzdValue getMember(const std::string& name);
    struct TzdValue* getMemberPtr(const std::string& name);
    void setMember(const std::string& name, const struct TzdValue& val);
};

// OOP ??????
class TzdOopManager {
private:
    static std::unordered_map<std::string, TzdClassDef*> classMap;


    static std::unordered_map<std::string, std::vector<TzdClassDef*>> simpleNameCache;

public:
    static void registerClass(TzdClassDef* cls);

    static TzdClassDef* getClass(const std::string& name);

    static std::vector<TzdClassDef*> getSubclassesOf(const std::string& parentName);

    static bool isInstanceOf(TzdInstance* obj, const std::string& typeName);
};

