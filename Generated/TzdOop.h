#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>

#include "TzdLangParser.h"
#include "TzdFFIAdapter.h"
#include "TzdDispatch.h"
// 必须前置声明 TzdValue（如果当前文件没有完整引入的话）
#include "TzdInterpreter.h" 

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

    struct TzdValue* templateVal = nullptr;
};

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

    std::vector<std::pair<std::string, int>> fieldIndices;
    std::vector<TzdValue> defaultFieldValues;

    // 【性能优化核心】：类注册时预先展平构建的默认字段模板，实例化时 O(1) 拷贝
    std::unordered_map<std::string, TzdValue> defaultFieldsTemplate;

    // 【vtable 优化】：方法索引表，消除字符串查找
    std::vector<ClassMethod*> methodTable;           // methodIndex → ClassMethod*
    std::unordered_map<std::string, int> methodIndexMap; // methodName → methodIndex
    int nextMethodIndex = 0;

    // 【vtable 优化】：字段名 → 索引的快速查找
    std::unordered_map<std::string, int> fieldNameToIndex; // fieldName → fieldValues index

    // 【类型ID】：用于 inline cache 的类型检查
    uint32_t typeId = 0;
    TzdDispatchTable tzdDispatch;

    ClassConstructor* findConstructor(size_t argCount);
    const ClassConstructor* findConstructor(size_t argCount) const;

    TzdClassDef(const std::string& fqn);
    virtual ~TzdClassDef();

    bool isSubclassOf(const std::string& targetParentName);
    void getAllSubclasses(std::vector<TzdClassDef*>& outSubclasses);

    struct ClassMethod* findMethod(const std::string& methodName);
    struct ClassField* findField(const std::string& fieldName);
    struct TzdValue* findStaticValue(const std::string& fieldName);

    // 【vtable 优化】：注册方法到索引表
    int registerMethodIndex(const std::string& methodName);
    int getMethodIndex(const std::string& methodName) const;
    ClassMethod* getMethodByIndex(int index) const;

    // 【flat field 优化】：注册字段到索引表
    void rebuildFieldIndexMap();
    int getFieldIndex(const std::string& fieldName) const;
};

class TzdInstance {
public:
    TzdClassDef* definition;

    std::vector<struct TzdValue> fieldValues;

    // 侵入式引用计数：TzdValue 拷贝时 retain，析构/覆盖时 release；归 0 自删。
    mutable std::atomic<int> refCount{ 0 };
    inline void retain() const { refCount.fetch_add(1, std::memory_order_relaxed); }
    inline void release() const {
        int prev = refCount.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1) delete this;
    }

    void* operator new(size_t size);
    void* operator new(size_t, void* ptr) noexcept { return ptr; }
    void operator delete(void* ptr);
    void operator delete(void*, void*) noexcept {}

    // 【GC 三色标记】：White=0 (未标记), Gray=1 (待扫描), Black=2 (已完成)
    enum class GCColor : uint8_t { White = 0, Gray = 1, Black = 2 };
    mutable std::atomic<uint8_t> gcColor{ 0 };
    inline void gcMarkGray() const { gcColor.store((uint8_t)GCColor::Gray, std::memory_order_relaxed); }
    inline void gcMarkBlack() const { gcColor.store((uint8_t)GCColor::Black, std::memory_order_relaxed); }
    inline void gcClearMark() const { gcColor.store((uint8_t)GCColor::White, std::memory_order_relaxed); }
    inline bool gcIsMarked() const { return gcColor.load(std::memory_order_relaxed) != (uint8_t)GCColor::White; }
    inline bool gcIsGray() const { return gcColor.load(std::memory_order_relaxed) == (uint8_t)GCColor::Gray; }
    inline GCColor gcGetColor() const { return (GCColor)gcColor.load(std::memory_order_relaxed); }

    // 【GC 分代】：Young=0, Survivor=1, Tenured=2
    enum class Gen : uint8_t { Young = 0, Survivor = 1, Tenured = 2 };
    mutable std::atomic<uint8_t> gcGeneration{ 0 };
    inline Gen gcGetGeneration() const { return (Gen)gcGeneration.load(std::memory_order_relaxed); }
    inline void gcSetGeneration(Gen g) const { gcGeneration.store((uint8_t)g, std::memory_order_relaxed); }

    TzdInstance(TzdClassDef* def);
    ~TzdInstance();

    struct TzdValue getMember(const std::string& name);
    struct TzdValue getMember(TzdSelector selector, const std::string& name = "");
    struct TzdValue* getMemberPtr(TzdSelector selector);
    struct TzdValue* getMemberPtr(const std::string& name);
    void setMember(TzdSelector selector, const struct TzdValue& val);
    void setMember(TzdSelector selector, const std::string& name, const struct TzdValue& val);
    void setMember(const std::string& name, const struct TzdValue& val);

    // 【flat field 优化】：直接索引访问，O(1) 无字符串查找 (定义在 .cpp 中)
    struct TzdValue* getMemberPtrByIndex(int index);
    struct TzdValue getMemberByIndex(int index);
    void setMemberByIndex(int index, const struct TzdValue& val);
};

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