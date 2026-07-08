#include "../Res/TzdStrings.h"
#include "TzdOop.h"

#include "TzdInterpreter.h"

std::unordered_map<std::string, TzdClassDef*> TzdOopManager::classMap;
std::unordered_map<std::string, std::vector<TzdClassDef*>> TzdOopManager::simpleNameCache;
static std::unordered_map<TzdClassDef*, std::unordered_map<std::string, TzdValue>> s_fieldTemplateCache;

static const std::unordered_map<std::string, TzdValue>& getFieldTemplate(TzdClassDef* def) {
    auto it = s_fieldTemplateCache.find(def);
    if (it != s_fieldTemplateCache.end()) return it->second;

    std::unordered_map<std::string, TzdValue> templ;
    TzdClassDef* cur = def;
    std::vector<TzdClassDef*> hierarchy;
    while (cur) {
        hierarchy.insert(hierarchy.begin(), cur);
        if (cur->parentName.empty()) break;
        cur = TzdOopManager::getClass(cur->parentName);
    }

    for (auto* cls : hierarchy) {
        for (auto const& [name, field] : cls->fields) {
            if (field.isStatic) continue;
            const std::string& t = field.type;
            if (t == "int" || t == "i32") templ[name] = TzdValue((int)0);
            else if (t == "byte" || t == "u8") templ[name] = TzdValue((unsigned char)0);
            else if (t == "ptr" || t == "pointer" || t == "hwnd") templ[name] = TzdValue((void*)nullptr);
            else if (t == "ulong" || t == "u64") templ[name] = TzdValue(0ULL);
            else if (t == "float" || t == "double") templ[name] = TzdValue(0.0);
            else if (t == "bool") templ[name] = TzdValue(false);
            else if (t == "string") templ[name] = TzdValue("");
            else if (t.find("[]") != std::string::npos) templ[name] = TzdValue(std::vector<TzdValue>{});
            else if (t == "map") templ[name] = TzdValue(std::unordered_map<std::string, TzdValue>{});
            else templ[name] = TzdValue();
        }
    }

    auto [insertedIt, _] = s_fieldTemplateCache.emplace(def, std::move(templ));
    return insertedIt->second;
}

ClassMethod* TzdClassDef::findMethod(const std::string& methodName) {
    auto cacheIt = methodCache.find(methodName);
    if (cacheIt != methodCache.end()) return cacheIt->second;

    auto it = methods.find(methodName);
    if (it != methods.end()) {
        methodCache[methodName] = &it->second;
        return &it->second;
    }

    ClassMethod* result = nullptr;
    if (!parentName.empty()) {
        TzdClassDef* parent = TzdOopManager::getClass(parentName);
        if (parent) result = parent->findMethod(methodName);
    }
    methodCache[methodName] = result;
    return result;
}

ClassField* TzdClassDef::findField(const std::string& fieldName) {
    auto cacheIt = fieldCache.find(fieldName);
    if (cacheIt != fieldCache.end()) return cacheIt->second;

    auto it = fields.find(fieldName);
    if (it != fields.end()) {
        fieldCache[fieldName] = &it->second;
        return &it->second;
    }

    ClassField* result = nullptr;
    if (!parentName.empty()) {
        TzdClassDef* parent = TzdOopManager::getClass(parentName);
        if (parent) result = parent->findField(fieldName);
    }
    fieldCache[fieldName] = result;
    return result;
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

TzdInstance::TzdInstance(TzdClassDef* def) : definition(def) {
    fields = getFieldTemplate(def);
}

TzdClassDef::TzdClassDef(const std::string& fqn) : fullName(fqn) {
    size_t lastDot = fqn.rfind('.');
    if (lastDot != std::string::npos) {
        // �а��� (org.tzd.Test)
        packageName = fqn.substr(0, lastDot);
        simpleName = fqn.substr(lastDot + 1);
    }
    else {
        // �ް��� (Test)
        packageName = "";
        simpleName = fqn;
    }
}

struct TzdValue TzdInstance::getMember(const std::string& name) {
    if (TzdValue* direct = getMemberPtr(name)) {
        return *direct;
    }

    if (definition) {
        ClassMethod* method = definition->findMethod(name);
        if (method) {
            TzdValue funcVal;
            if (method->isNative) {
                funcVal.type = TzdValue::NATIVE_FUNCTION;
                funcVal.nativeFunc = method->nativeWrapper;
            }
            else {
                funcVal.type = TzdValue::FUNCTION;
                funcVal.funcBody = method->body;
                funcVal.params = method->params;
            }
            funcVal.name = method->name;
            funcVal.jittedPtr = method->jittedPtr;
            funcVal.instanceVal = this;
            return funcVal;
        }

        if (definition->findField(name)) {
            return TzdValue();
        }
    }

    throw std::runtime_error(TzdRte::OOP_MEMBER_NOT_FOUND_PREFIX + name + TzdRte::OOP_MEMBER_NOT_FOUND_MID + definition->fullName + TzdRte::OOP_MEMBER_NOT_FOUND_SUFFIX);
}

TzdValue* TzdInstance::getMemberPtr(const std::string& name) {
    auto it = fields.find(name);
    if (it != fields.end()) return &it->second;
    if (!definition) return nullptr;
    return definition->findStaticValue(name);
}

// ���ó�Ա����
void TzdInstance::setMember(const std::string& name, const struct TzdValue& val) {
    if (TzdValue* direct = getMemberPtr(name)) {
        *direct = val;
        return;
    }
    if (definition && definition->findField(name)) {
        fields[name] = val;
        return;
    }
    throw std::runtime_error(TzdRte::OOP_ASSIGN_NOT_FOUND_PREFIX + name + TzdRte::OOP_ASSIGN_NOT_FOUND_MID + definition->fullName + ")");
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

    classMap[cls->fullName] = cls;
    simpleNameCache[cls->simpleName].push_back(cls);

    if (!cls->parentName.empty()) {
        TzdClassDef* parent = getClass(cls->parentName);
        if (parent) parent->derivedClasses.push_back(cls);
    }
}
bool TzdClassDef::isSubclassOf(const std::string& targetParentName) {
    if (this->fullName == targetParentName || this->simpleName == targetParentName) return true;
    if (parentName.empty()) return false;

    TzdClassDef* parent = TzdOopManager::getClass(parentName);
    if (parent) {
        return parent->isSubclassOf(targetParentName); // �ݹ����ϲ���
    }
    return false;
}

// �ݹ��ȡ��������
void TzdClassDef::getAllSubclasses(std::vector<TzdClassDef*>& outSubclasses) {
    for (auto* child : derivedClasses) {
        outSubclasses.push_back(child);
        child->getAllSubclasses(outSubclasses); // �ݹ����²���
    }
}

// Manager ��װ�ӿ�
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

    TzdClassDef* targetDef = getClass(typeName);
    if (!targetDef) return false;

    TzdClassDef* current = obj->definition;
    while (current) {
        if (current == targetDef || current->fullName == targetDef->fullName) {
            return true;
        }
        if (current->parentName.empty()) break;
        current = getClass(current->parentName);
    }
    return false;
}


