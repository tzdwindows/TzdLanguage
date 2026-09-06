#include "../Res/TzdStrings.h"
#include "TzdOop.h"
#include "TzdInterpreter.h"
#include "TzdGC.h"
#include <mutex>

static std::unordered_map<std::string, TzdSelector> s_nameToSelector;
static std::vector<std::string> s_selectorToName = { "" }; // index 0 is reserved
static std::mutex s_selectorMutex;

TzdSelector tzdInternSelector(const std::string& name) {
    if (name.empty()) return 0;
    std::lock_guard<std::mutex> lock(s_selectorMutex);
    auto it = s_nameToSelector.find(name);
    if (it != s_nameToSelector.end()) return it->second;
    TzdSelector sel = static_cast<TzdSelector>(s_selectorToName.size());
    s_selectorToName.push_back(name);
    s_nameToSelector[name] = sel;
    return sel;
}

std::string tzdGetSelectorName(TzdSelector selector) {
    std::lock_guard<std::mutex> lock(s_selectorMutex);
    if (selector > 0 && selector < (TzdSelector)s_selectorToName.size()) {
        return s_selectorToName[selector];
    }
    return "";
}

std::unordered_map<std::string, TzdClassDef*> TzdOopManager::classMap;
std::unordered_map<std::string, std::vector<TzdClassDef*>> TzdOopManager::simpleNameCache;

// 【性能优化】：废弃基于 Map 的缓存，对 1-3 层的继承树直接迭代查表最快 (无需字符串哈希插入开销)
ClassMethod* TzdClassDef::findMethod(const std::string& methodName) {
    TzdClassDef* cur = this;
    while (cur) {
        auto it = cur->methods.find(methodName);
        if (it != cur->methods.end()) return &it->second;
        if (cur->parentName.empty()) break;
        cur = TzdOopManager::getClass(cur->parentName);
    }
    return nullptr;
}

ClassField* TzdClassDef::findField(const std::string& fieldName) {
    TzdClassDef* cur = this;
    while (cur) {
        auto it = cur->fields.find(fieldName);
        if (it != cur->fields.end()) return &it->second;
        if (cur->parentName.empty()) break;
        cur = TzdOopManager::getClass(cur->parentName);
    }
    return nullptr;
}

TzdValue* TzdClassDef::findStaticValue(const std::string& fieldName) {
    TzdClassDef* cur = this;
    while (cur) {
        auto it = cur->staticValues.find(fieldName);
        if (it != cur->staticValues.end()) return &it->second;
        if (cur->parentName.empty()) break;
        cur = TzdOopManager::getClass(cur->parentName);
    }
    return nullptr;
}

ClassConstructor* TzdClassDef::findConstructor(size_t argCount) {
    ClassConstructor* found = nullptr;
    for (auto& c : constructors) {
        if ((size_t)c.paramCount == argCount) {
            if (found) return nullptr;
            found = &c;
        }
    }
    return found;
}

const ClassConstructor* TzdClassDef::findConstructor(size_t argCount) const {
    const ClassConstructor* found = nullptr;
    for (const auto& c : constructors) {
        if ((size_t)c.paramCount == argCount) {
            if (found) return nullptr;
            found = &c;
        }
    }
    return found;
}

TzdClassDef::TzdClassDef(const std::string& fqn) : fullName(fqn) {
    size_t lastDot = fqn.rfind('.');
    if (lastDot != std::string::npos) {
        packageName = fqn.substr(0, lastDot);
        simpleName = fqn.substr(lastDot + 1);
    }
    else {
        packageName = "";
        simpleName = fqn;
    }
}

struct TzdInstancePool {
    static constexpr size_t POOL_CAP = 16384;
    void* slots[POOL_CAP];
    size_t count = 0;

    inline void* allocate(size_t size) {
        if (count > 0) return slots[--count];
        return std::malloc(size);
    }
    inline void deallocate(void* ptr) {
        if (count < POOL_CAP) slots[count++] = ptr;
        else std::free(ptr);
    }
};

static thread_local TzdInstancePool s_instancePool;

void* TzdInstance::operator new(size_t size) {
    return s_instancePool.allocate(size);
}

void TzdInstance::operator delete(void* ptr) {
    s_instancePool.deallocate(ptr);
}

// 【性能优化】：实例化对象时，直接拷贝已在类注册时展平计算好的默认字段模板，无需任何遍历
TzdInstance::TzdInstance(TzdClassDef* def) : definition(def) {
    if (def) {
        fieldValues = def->defaultFieldValues;
    }
    // Register with GC for mark-and-sweep tracking only if enabled
    if (TzdGarbageCollector::getInstance().isEnabled()) {
        TzdGarbageCollector::getInstance().registerInstance(this);
    }
}

TzdInstance::~TzdInstance() {
    // Unregister from GC only if enabled
    if (TzdGarbageCollector::getInstance().isEnabled()) {
        TzdGarbageCollector::getInstance().unregisterInstance(this);
    }
}

TzdClassDef::~TzdClassDef() {
    for (auto& [name, m] : methods) {
        if (m.templateVal) {
            delete m.templateVal;
            m.templateVal = nullptr;
        }
    }
}


struct TzdValue TzdInstance::getMember(TzdSelector selector, const std::string& name) {
    if (selector == 0) {
        if (!name.empty()) selector = tzdInternSelector(name);
        else return TzdValue();
    }

    if (definition) {
        const TzdMemberSlot* slot = definition->tzdDispatch.find(selector);
        if (slot) {
            if (slot->fieldIndex >= 0) {
                if (slot->fieldIndex < (int)fieldValues.size()) {
                    return fieldValues[slot->fieldIndex];
                }
                return TzdValue();
            }
            if (slot->method) {
                if (slot->method->templateVal) {
                    TzdValue funcVal = *(slot->method->templateVal);
                    funcVal.jittedPtr = slot->method->jittedPtr;
                    funcVal.setInstance(this);
                    return funcVal;
                }
                TzdValue funcVal;
                if (slot->method->isNative) {
                    funcVal.type = TzdValue::NATIVE_FUNCTION;
                    funcVal.nativeFunc = slot->method->nativeWrapper;
                } else {
                    funcVal.type = TzdValue::FUNCTION;
                    funcVal.funcBody = slot->method->body;
                    funcVal.params = slot->method->params;
                    funcVal.paramTypes = slot->method->paramTypes;
                }
                funcVal.name = slot->method->name;
                funcVal.jittedPtr = slot->method->jittedPtr;
                funcVal.setInstance(this);
                return funcVal;
            }
            if (slot->staticValue) {
                return *slot->staticValue;
            }
        }

        // Fallback: search static value or method by name
        std::string memberName = name.empty() ? tzdGetSelectorName(selector) : name;
        if (!memberName.empty()) {
            if (TzdValue* staticVal = definition->findStaticValue(memberName)) {
                return *staticVal;
            }
            if (ClassMethod* method = definition->findMethod(memberName)) {
                if (method->templateVal) {
                    TzdValue funcVal = *(method->templateVal);
                    funcVal.jittedPtr = method->jittedPtr;
                    funcVal.setInstance(this);
                    return funcVal;
                }
            }
            if (definition->findField(memberName)) {
                return TzdValue();
            }
        }
    }

    std::string errName = name.empty() ? tzdGetSelectorName(selector) : name;
    throw std::runtime_error(TzdRte::OOP_MEMBER_NOT_FOUND_PREFIX + errName + TzdRte::OOP_MEMBER_NOT_FOUND_MID + (definition ? definition->fullName : "<null>") + TzdRte::OOP_MEMBER_NOT_FOUND_SUFFIX);
}

struct TzdValue TzdInstance::getMember(const std::string& name) {
    TzdSelector sel = tzdInternSelector(name);
    return getMember(sel, name);
}

TzdValue* TzdInstance::getMemberPtr(TzdSelector selector) {
    if (!definition || selector == 0) return nullptr;
    const TzdMemberSlot* slot = definition->tzdDispatch.find(selector);
    if (slot) {
        if (slot->fieldIndex >= 0 && slot->fieldIndex < (int)fieldValues.size()) {
            return &fieldValues[slot->fieldIndex];
        }
        if (slot->staticValue) {
            return slot->staticValue;
        }
    }
    return nullptr;
}

TzdValue* TzdInstance::getMemberPtr(const std::string& name) {
    TzdSelector sel = tzdInternSelector(name);
    if (TzdValue* ptr = getMemberPtr(sel)) return ptr;
    return definition ? definition->findStaticValue(name) : nullptr;
}

void TzdInstance::setMember(TzdSelector selector, const struct TzdValue& val) {
    setMember(selector, "", val);
}

void TzdInstance::setMember(TzdSelector selector, const std::string& name, const struct TzdValue& val) {
    if (!definition) throw std::runtime_error("Instance definition is null");
    if (selector == 0) {
        if (!name.empty()) selector = tzdInternSelector(name);
    }
    if (selector != 0) {
        const TzdMemberSlot* slot = definition->tzdDispatch.find(selector);
        if (slot) {
            if (slot->fieldIndex >= 0 && slot->fieldIndex < (int)fieldValues.size()) {
                TzdGarbageCollector::getInstance().writeBarrier(this, &fieldValues[slot->fieldIndex], fieldValues[slot->fieldIndex]);
                fieldValues[slot->fieldIndex] = val;
                return;
            }
            if (slot->staticValue) {
                *slot->staticValue = val;
                return;
            }
        }
    }

    std::string memberName = name.empty() ? tzdGetSelectorName(selector) : name;
    if (!memberName.empty()) {
        if (TzdValue* staticVal = definition->findStaticValue(memberName)) {
            *staticVal = val;
            return;
        }
    }
    throw std::runtime_error(TzdRte::OOP_ASSIGN_NOT_FOUND_PREFIX + memberName + TzdRte::OOP_ASSIGN_NOT_FOUND_MID + definition->fullName + ")");
}

void TzdInstance::setMember(const std::string& name, const struct TzdValue& val) {
    TzdSelector sel = tzdInternSelector(name);
    setMember(sel, name, val);
}


void injectNativeMethod(TzdClassDef* cls, const std::string& name, TzdValue::NativeFuncType func) {
    ClassMethod m;
    m.name = name;
    m.isNative = true;
    m.nativeWrapper = func;
    cls->methods[name] = m;
}

void TzdOopManager::registerClass(TzdClassDef* cls) {
    if (classMap.count(cls->fullName)) {
        throw std::runtime_error(TzdRte::CLASS_REDEFINED_PREFIX + cls->fullName + TzdRte::CLASS_REDEFINED_SUFFIX);
    }

    injectNativeMethod(cls, "getClassName", [cls](auto args) { return TzdValue(cls->fullName); });
    injectNativeMethod(cls, "getParentName", [cls](auto args) {
        return TzdValue(cls->parentName.empty() ? "" : cls->parentName);
        });
    injectNativeMethod(cls, "getFields", [cls](auto args) {
        std::vector<TzdValue> fieldNames;
        for (auto const& [name, field] : cls->fields) {
            fieldNames.push_back(TzdValue(name));
        }
        return TzdValue(fieldNames);
        });
    injectNativeMethod(cls, "getMethods", [cls](auto args) {
        std::vector<TzdValue> methodNames;
        for (auto const& [name, method] : cls->methods) {
            methodNames.push_back(TzdValue(name));
        }
        return TzdValue(methodNames);
        });
    injectNativeMethod(cls, "isSubclassOf", [cls](auto args) {
        if (args.empty() || args[0].type != TzdValue::STRING) return TzdValue(false);
        return TzdValue(cls->isSubclassOf(args[0].sVal));
        });

    // =========================================================================
    // 【核心性能优化步骤 1】在注册阶段就把类的默认字段模板预先构建好，省去实例化时的遍历
    // =========================================================================
    std::vector<TzdClassDef*> hierarchy;
    TzdClassDef* cur = cls;
    while (cur) {
        hierarchy.insert(hierarchy.begin(), cur);
        if (cur->parentName.empty()) break;
        cur = TzdOopManager::getClass(cur->parentName);
    }

    int currentFieldIndex = 0; // 字段在数组中的物理位置
    for (auto* parentCls : hierarchy) {
        for (auto const& [name, field] : parentCls->fields) {
            if (field.isStatic) continue;

            // 记录索引
            cls->fieldIndices.push_back({ name, currentFieldIndex++ });

            // 推入默认值
            const std::string& t = field.type;
            TzdValue defVal;
            if (t == "int" || t == "i32") defVal = TzdValue((int)0);
            else if (t == "byte" || t == "u8") defVal = TzdValue((unsigned char)0);
            else if (t == "ptr" || t == "pointer" || t == "hwnd") defVal = TzdValue((void*)nullptr);
            else if (t == "ulong" || t == "u64") defVal = TzdValue(0ULL);
            else if (t == "float" || t == "double") defVal = TzdValue(0.0);
            else if (t == "bool") defVal = TzdValue(false);
            else if (t == "string") defVal = TzdValue("");
            else if (t.find("[]") != std::string::npos) defVal = TzdValue(std::vector<TzdValue>{});
            else if (t == "map") defVal = TzdValue(std::unordered_map<std::string, TzdValue>{});
            else defVal = TzdValue();

            cls->defaultFieldValues.push_back(defVal);
        }
    }

    // =========================================================================
    // 【核心性能优化步骤 2】将 Method 初始化为 Template 以避免运行期拷贝 Vector
    // =========================================================================
    for (auto& [mName, m] : cls->methods) {
        m.templateVal = new TzdValue(); // 动态分配指针，避开头文件限制

        if (m.isNative) {
            m.templateVal->type = TzdValue::NATIVE_FUNCTION;
            m.templateVal->nativeFunc = m.nativeWrapper;
        }
        else {
            m.templateVal->type = TzdValue::FUNCTION;
            m.templateVal->funcBody = m.body;
            m.templateVal->params = m.params; // 这里仅发生一次 Vector 拷贝
        }
        m.templateVal->name = m.name;
    }

    // =========================================================================
    // 【vtable 优化步骤 3】：构建方法索引表 (vtable) 和字段名→索引映射
    // =========================================================================
    // Build field name → index map for O(1) lookup
    cls->fieldNameToIndex.clear();
    for (const auto& [fname, fidx] : cls->fieldIndices) {
        cls->fieldNameToIndex[fname] = fidx;
    }

    // Build method vtable: assign each method an index
    cls->methodTable.clear();
    cls->methodIndexMap.clear();
    cls->nextMethodIndex = 0;
    for (auto& [mName, m] : cls->methods) {
        int idx = cls->nextMethodIndex++;
        cls->methodIndexMap[mName] = idx;
        cls->methodTable.push_back(&cls->methods[mName]);
    }

    // =========================================================================
    // 【TzdDispatch 全域超平坦矩阵优化步骤 4】：构建全平坦选择子表 cls->tzdDispatch
    // =========================================================================
    cls->tzdDispatch.clear();

    // 1. 继承链静态变量展平
    for (auto* parentCls : hierarchy) {
        for (auto& [sName, sVal] : parentCls->staticValues) {
            TzdSelector sel = tzdInternSelector(sName);
            cls->tzdDispatch.add(sel).staticValue = &sVal;
        }
    }

    // 2. 继承链方法全量展平（子类覆盖父类方法）
    for (auto* parentCls : hierarchy) {
        for (auto& [mName, m] : parentCls->methods) {
            TzdSelector sel = tzdInternSelector(mName);
            ClassMethod* targetMethod = cls->methods.count(mName) ? &cls->methods[mName] : &m;
            cls->tzdDispatch.add(sel).method = targetMethod;
        }
    }

    // 3. 实例字段索引全量展平
    for (const auto& [fname, fidx] : cls->fieldIndices) {
        TzdSelector sel = tzdInternSelector(fname);
        cls->tzdDispatch.add(sel).fieldIndex = fidx;
    }

    // 4. 当前类静态变量最终绑定
    for (auto& [sName, sVal] : cls->staticValues) {
        TzdSelector sel = tzdInternSelector(sName);
        cls->tzdDispatch.add(sel).staticValue = &sVal;
    }

    // 注册到系统全局变量
    classMap[cls->fullName] = cls;
    simpleNameCache[cls->simpleName].push_back(cls);

    if (!cls->parentName.empty()) {
        TzdClassDef* parent = getClass(cls->parentName);
        if (parent) parent->derivedClasses.push_back(cls);
    }
}

// 【性能优化】：将递归的 isSubclassOf 修改为 Iteration，避免函数调用栈开销
bool TzdClassDef::isSubclassOf(const std::string& targetParentName) {
    if (this->fullName == targetParentName || this->simpleName == targetParentName) return true;

    TzdClassDef* cur = this;
    while (!cur->parentName.empty()) {
        cur = TzdOopManager::getClass(cur->parentName);
        if (!cur) break;
        if (cur->fullName == targetParentName || cur->simpleName == targetParentName) return true;
    }
    return false;
}

void TzdClassDef::getAllSubclasses(std::vector<TzdClassDef*>& outSubclasses) {
    for (auto* child : derivedClasses) {
        outSubclasses.push_back(child);
        child->getAllSubclasses(outSubclasses);
    }
}

std::vector<TzdClassDef*> TzdOopManager::getSubclassesOf(const std::string& parentName) {
    TzdClassDef* parent = getClass(parentName);
    std::vector<TzdClassDef*> result;
    if (parent) {
        parent->getAllSubclasses(result);
    }
    return result;
}

TzdClassDef* TzdOopManager::getClass(const std::string& name) {
    if (classMap.count(name)) {
        return classMap[name];
    }
    if (name.find('.') != std::string::npos) {
        return nullptr;
    }
    if (simpleNameCache.count(name)) {
        const auto& candidates = simpleNameCache[name];

        if (candidates.empty()) return nullptr;
        if (candidates.size() == 1) {
            return candidates[0];
        }
        std::string msg = TzdRte::OOP_DUPLICATE_CLASS_PREFIX + name + TzdRte::OOP_DUPLICATE_CLASS_SUFFIX;
        for (auto* c : candidates) {
            msg += "[" + c->fullName + "] ";
        }
        throw std::runtime_error(msg);
    }
    return nullptr;
}

bool TzdOopManager::isInstanceOf(TzdInstance* obj, const std::string& typeName) {
    if (!obj || !obj->definition) return false;
    return obj->definition->isSubclassOf(typeName);
}

// ============================================================================
// vtable + flat field access implementation
// ============================================================================

int TzdClassDef::registerMethodIndex(const std::string& methodName) {
    auto it = methodIndexMap.find(methodName);
    if (it != methodIndexMap.end()) return it->second;
    int idx = nextMethodIndex++;
    methodIndexMap[methodName] = idx;
    auto* m = findMethod(methodName);
    if (m) methodTable.push_back(m);
    return idx;
}

int TzdClassDef::getMethodIndex(const std::string& methodName) const {
    auto it = methodIndexMap.find(methodName);
    if (it != methodIndexMap.end()) return it->second;
    return -1;
}

ClassMethod* TzdClassDef::getMethodByIndex(int index) const {
    if (index >= 0 && index < (int)methodTable.size()) return methodTable[index];
    return nullptr;
}

void TzdClassDef::rebuildFieldIndexMap() {
    fieldNameToIndex.clear();
    for (const auto& [fname, fidx] : fieldIndices) {
        fieldNameToIndex[fname] = fidx;
    }
}

int TzdClassDef::getFieldIndex(const std::string& fieldName) const {
    auto it = fieldNameToIndex.find(fieldName);
    if (it != fieldNameToIndex.end()) return it->second;
    return -1;
}

// ============================================================================
// TzdInstance flat field access implementation
// ============================================================================

TzdValue* TzdInstance::getMemberPtrByIndex(int index) {
    if (index >= 0 && index < (int)fieldValues.size()) return &fieldValues[index];
    return nullptr;
}

TzdValue TzdInstance::getMemberByIndex(int index) {
    if (index >= 0 && index < (int)fieldValues.size()) return fieldValues[index];
    return TzdValue();
}

void TzdInstance::setMemberByIndex(int index, const TzdValue& val) {
    if (index >= 0 && index < (int)fieldValues.size()) {
        // SATB write barrier: snapshot old value before overwrite
        TzdGarbageCollector::getInstance().writeBarrier(this, &fieldValues[index], fieldValues[index]);
        fieldValues[index] = val;
    }
}