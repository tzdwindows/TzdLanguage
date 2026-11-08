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
        if (!c.isVararg && (size_t)c.paramCount == argCount) {
            if (found) return nullptr;
            found = &c;
        }
    }
    if (found) return found;
    for (auto& c : constructors) {
        if (c.isVararg && argCount >= (size_t)(c.paramCount > 0 ? c.paramCount - 1 : 0)) {
            if (found) return nullptr;
            found = &c;
        }
    }
    return found;
}

const ClassConstructor* TzdClassDef::findConstructor(size_t argCount) const {
    const ClassConstructor* found = nullptr;
    for (const auto& c : constructors) {
        if (!c.isVararg && (size_t)c.paramCount == argCount) {
            if (found) return nullptr;
            found = &c;
        }
    }
    if (found) return found;
    for (const auto& c : constructors) {
        if (c.isVararg && argCount >= (size_t)(c.paramCount > 0 ? c.paramCount - 1 : 0)) {
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
    TzdInstance* slots[POOL_CAP];
    size_t count = 0;

    inline TzdInstance* get(TzdClassDef* def) {
        if (count > 0) {
            TzdInstance* inst = slots[--count];
            inst->definition = def;
            inst->refCount.store(0, std::memory_order_relaxed);
            inst->clearCompression();
            if (def) {
                inst->fieldValues = def->defaultFieldValues;
            } else {
                inst->fieldValues.clear();
            }
            return inst;
        }
        return new TzdInstance(def);
    }

    inline void put(TzdInstance* inst) {
        if (!inst) return;
        inst->fieldValues.clear();
        inst->clearCompression();
        inst->definition = nullptr;
        if (count < POOL_CAP) {
            slots[count++] = inst;
        } else {
            delete inst;
        }
    }
};

static thread_local TzdInstancePool s_instancePool;

TzdInstance* TzdInstance::create(TzdClassDef* def) {
    return s_instancePool.get(def);
}

void TzdInstance::recycle(TzdInstance* inst) {
    if (!inst) return;
    s_instancePool.put(inst);
}

void* TzdInstance::operator new(size_t size) {
    return std::malloc(size);
}

void TzdInstance::operator delete(void* ptr) {
    std::free(ptr);
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

struct TzdCompressedField {
    uint16_t index;
    TzdValue value;
};

struct TzdInstance::CompressedData {
    std::vector<TzdCompressedField> fields;
};

TzdInstance::~TzdInstance() {
    clearCompression();
    // Unregister from GC only if enabled
    if (TzdGarbageCollector::getInstance().isEnabled()) {
        TzdGarbageCollector::getInstance().unregisterInstance(this);
    }
}

void TzdInstance::clearCompression() {
    if (m_compressed) {
        delete m_compressed;
        m_compressed = nullptr;
    }
    m_isCompressed = false;
}

size_t TzdInstance::getCompressedMemoryBytes() const {
    if (!m_isCompressed || !m_compressed) return 0;
    size_t sz = sizeof(CompressedData) + m_compressed->fields.capacity() * sizeof(TzdCompressedField);
    for (const auto& entry : m_compressed->fields) {
        if (!entry.value.sVal.empty()) sz += entry.value.sVal.capacity();
        if (entry.value.fastBuf && entry.value.fastLen > 0) sz += (size_t)entry.value.fastLen * sizeof(double);
    }
    return sz;
}

size_t TzdInstance::compress() {
    if (m_isCompressed) return 0;
    if (fieldValues.empty()) {
        m_isCompressed = true;
        return 0;
    }
    size_t beforeBytes = fieldValues.capacity() * sizeof(TzdValue);
    if (!m_compressed) {
        m_compressed = new CompressedData();
    }
    m_compressed->fields.clear();
    for (size_t i = 0; i < fieldValues.size(); ++i) {
        bool isDefault = false;
        if (definition && i < definition->defaultFieldValues.size()) {
            const auto& defVal = definition->defaultFieldValues[i];
            if (fieldValues[i].type == defVal.type &&
                fieldValues[i].dVal == defVal.dVal &&
                fieldValues[i].lVal == defVal.lVal &&
                fieldValues[i].bVal == defVal.bVal &&
                fieldValues[i].ptrVal == defVal.ptrVal &&
                fieldValues[i].sVal == defVal.sVal) {
                isDefault = true;
            }
        } else if (fieldValues[i].type == TzdValue::NONE) {
            isDefault = true;
        }
        if (!isDefault) {
            m_compressed->fields.push_back({ (uint16_t)i, std::move(fieldValues[i]) });
        }
    }
    fieldValues.clear();
    fieldValues.shrink_to_fit();
    m_isCompressed = true;
    m_compressed->fields.shrink_to_fit();
    size_t afterBytes = sizeof(CompressedData) + m_compressed->fields.capacity() * sizeof(TzdCompressedField);
    return (beforeBytes > afterBytes) ? (beforeBytes - afterBytes) : 0;
}

void TzdInstance::decompress() {
    if (!m_isCompressed) return;
    if (definition) {
        fieldValues = definition->defaultFieldValues;
    }
    if (m_compressed) {
        for (auto& entry : m_compressed->fields) {
            if (entry.index >= fieldValues.size()) {
                fieldValues.resize(entry.index + 1);
            }
            fieldValues[entry.index] = std::move(entry.value);
        }
        delete m_compressed;
        m_compressed = nullptr;
    }
    m_isCompressed = false;
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
                if (m_isCompressed) {
                    if (m_compressed) {
                        for (const auto& entry : m_compressed->fields) {
                            if (entry.index == (uint16_t)slot->fieldIndex) {
                                return entry.value;
                            }
                        }
                    }
                    if (definition && slot->fieldIndex < (int)definition->defaultFieldValues.size()) {
                        return definition->defaultFieldValues[slot->fieldIndex];
                    }
                    return TzdValue();
                }
                if (slot->fieldIndex < (int)fieldValues.size()) {
                    return fieldValues[slot->fieldIndex];
                }
                return TzdValue();
            }
            if (slot->method) {
                if (slot->method->templateVal) {
                    TzdValue funcVal = *(slot->method->templateVal);
                    funcVal.ensureExtra().jittedPtr = slot->method->jittedPtr;
                    funcVal.setInstance(this);
                    return funcVal;
                }
                TzdValue funcVal;
                if (slot->method->isNative) {
                    funcVal.type = TzdValue::NATIVE_FUNCTION;
                    funcVal.ensureExtra().nativeFunc = slot->method->nativeWrapper;
                } else {
                    funcVal.type = TzdValue::FUNCTION;
                    funcVal.ensureExtra().funcBody = slot->method->body;
                    funcVal.ensureExtra().params = slot->method->params;
                    funcVal.ensureExtra().paramTypes = slot->method->paramTypes;
                }
                funcVal.name = slot->method->name;
                funcVal.ensureExtra().isVararg = slot->method->isVararg;
                funcVal.ensureExtra().jittedPtr = slot->method->jittedPtr;
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
                    funcVal.ensureExtra().jittedPtr = method->jittedPtr;
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
    if (m_isCompressed) ensureDecompressed();
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

std::string getTzdValueTypeName(const TzdValue& val) {
    switch (val.type) {
    case TzdValue::NONE: return "null";
    case TzdValue::INT: return "int";
    case TzdValue::LONG: return "long";
    case TzdValue::SHORT: return "short";
    case TzdValue::SBYTE: return "sbyte";
    case TzdValue::BYTE: return "byte";
    case TzdValue::UINT: return "uint";
    case TzdValue::ULONG: return "ulong";
    case TzdValue::USHORT: return "ushort";
    case TzdValue::FLOAT: return "float";
    case TzdValue::DOUBLE: return "double";
    case TzdValue::BOOL: return "bool";
    case TzdValue::STRING: return "string";
    case TzdValue::POINTER: return "ptr";
    case TzdValue::ARRAY: return "array";
    case TzdValue::MAP: return "map";
    case TzdValue::FUNCTION: case TzdValue::NATIVE_FUNCTION: return "function";
    case TzdValue::INSTANCE:
        return (val.instanceVal && val.instanceVal->definition) ? val.instanceVal->definition->fullName : "object";
    case TzdValue::CLASS_DEF:
        return val.classDefVal ? val.classDefVal->fullName : "class";
    case TzdValue::BIGINT: return "BigInt";
    case TzdValue::RATIONAL: return "Rational";
    case TzdValue::BIGFLOAT: return "BigFloat";
    case TzdValue::TENSOR: return "Tensor";
    case TzdValue::FUTURE: return "future";
    case TzdValue::ANY_REF: return "any";
    case TzdValue::ERROR_VAL: return "error";
    case TzdValue::ENUM_VAL: return "enum";
    default: return "unknown";
    }
}

bool isTzdValueCompatibleWithType(const std::string& expectedType, const TzdValue& val, TzdValue* outConverted) {
    if (outConverted) *outConverted = val;
    if (expectedType.empty() || expectedType == "var" || expectedType == "auto" ||
        expectedType == "any" || expectedType == "object") {
        return true;
    }

    // 1. Integer types
    if (expectedType == "int" || expectedType == "i32" || expectedType == "long" || expectedType == "i64" ||
        expectedType == "short" || expectedType == "byte" || expectedType == "u8" || expectedType == "sbyte" ||
        expectedType == "uint" || expectedType == "ulong" || expectedType == "ushort") {
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                outConverted->dVal = (double)val.lVal;
                outConverted->lVal = val.lVal;
            }
            return true;
        }
        if (val.type == TzdValue::DOUBLE || val.type == TzdValue::FLOAT) {
            if (outConverted) {
                outConverted->type = (expectedType == "long" || expectedType == "i64" || expectedType == "ulong") ? TzdValue::LONG : TzdValue::INT;
                outConverted->lVal = (long long)val.dVal;
                outConverted->dVal = (double)outConverted->lVal;
            }
            return true;
        }
        if (val.type == TzdValue::BIGINT) {
            if (outConverted) {
                try {
                    long long parsed = std::stoll(val.sVal);
                    if (expectedType == "int" || expectedType == "i32") {
                        if (parsed >= INT32_MIN && parsed <= INT32_MAX) {
                            outConverted->type = TzdValue::INT;
                            outConverted->lVal = parsed;
                            outConverted->dVal = (double)parsed;
                        } else {
                            *outConverted = val;
                        }
                    } else {
                        outConverted->type = TzdValue::LONG;
                        outConverted->lVal = parsed;
                        outConverted->dVal = (double)parsed;
                    }
                } catch (...) {
                    *outConverted = val;
                }
            }
            return true;
        }
        return false;
    }

    // 2. Floating-point types
    if (expectedType == "float" || expectedType == "double") {
        if (val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE) {
            if (outConverted) {
                outConverted->lVal = (long long)val.dVal;
            }
            return true;
        }
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                outConverted->type = (expectedType == "float") ? TzdValue::FLOAT : TzdValue::DOUBLE;
                outConverted->dVal = (double)val.lVal;
                outConverted->lVal = val.lVal;
            }
            return true;
        }
        if (val.type == TzdValue::BIGINT || val.type == TzdValue::BIGFLOAT) {
            if (outConverted) {
                try {
                    outConverted->dVal = std::stod(val.sVal);
                    outConverted->lVal = (long long)outConverted->dVal;
                } catch (...) {
                    outConverted->dVal = std::numeric_limits<double>::infinity();
                }
                outConverted->type = (expectedType == "float") ? TzdValue::FLOAT : TzdValue::DOUBLE;
            }
            return true;
        }
        return false;
    }

    // 2.1 BigInt
    if (expectedType == "BigInt" || expectedType == "bigint") {
        if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition &&
            (val.instanceVal->definition->fullName == "BigInt" || val.instanceVal->definition->simpleName == "BigInt" ||
             val.instanceVal->definition->fullName == "math.BigInt")) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type == TzdValue::BIGINT) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGINT;
                outConverted->sVal = std::to_string(val.lVal);
            }
            return true;
        }
        if (val.type == TzdValue::STRING) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGINT;
            }
            return true;
        }
        if (val.type == TzdValue::DOUBLE || val.type == TzdValue::FLOAT) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGINT;
                outConverted->sVal = std::to_string((long long)val.dVal);
            }
            return true;
        }
        return false;
    }

    // 2.2 Rational
    if (expectedType == "Rational" || expectedType == "rational") {
        if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition &&
            (val.instanceVal->definition->fullName == "Rational" || val.instanceVal->definition->simpleName == "Rational" ||
             val.instanceVal->definition->fullName == "math.Rational")) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type == TzdValue::RATIONAL) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type == TzdValue::BIGINT) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::RATIONAL;
                outConverted->sVal = val.sVal + "/1";
            }
            return true;
        }
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::RATIONAL;
                outConverted->sVal = std::to_string(val.lVal) + "/1";
            }
            return true;
        }
        return false;
    }

    // 2.3 BigFloat
    if (expectedType == "BigFloat" || expectedType == "bigfloat") {
        if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition &&
            (val.instanceVal->definition->fullName == "BigFloat" || val.instanceVal->definition->simpleName == "BigFloat" ||
             val.instanceVal->definition->fullName == "math.BigFloat")) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type == TzdValue::BIGFLOAT) {
            if (outConverted) *outConverted = val;
            return true;
        }
        if (val.type == TzdValue::BIGINT || val.type == TzdValue::STRING) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGFLOAT;
            }
            return true;
        }
        if (val.type == TzdValue::DOUBLE || val.type == TzdValue::FLOAT) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGFLOAT;
                outConverted->sVal = std::to_string(val.dVal);
            }
            return true;
        }
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                *outConverted = val;
                outConverted->type = TzdValue::BIGFLOAT;
                outConverted->sVal = std::to_string(val.lVal);
            }
            return true;
        }
        return false;
    }

    // 3. String
    if (expectedType == "string") {
        return val.type == TzdValue::STRING || val.type == TzdValue::NONE;
    }

    // 4. Bool
    if (expectedType == "bool") {
        return val.type == TzdValue::BOOL;
    }

    // 5. Pointer / Reference
    if (expectedType == "ptr" || expectedType == "pointer" || expectedType == "hwnd" || expectedType == "void") {
        if (val.type == TzdValue::POINTER || val.type == TzdValue::NONE ||
            val.type == TzdValue::INSTANCE || val.type == TzdValue::MAP ||
            val.type == TzdValue::ARRAY || val.type == TzdValue::TENSOR ||
            val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION) {
            return true;
        }
        if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::ULONG) {
            if (outConverted) {
                outConverted->type = TzdValue::POINTER;
                outConverted->ptrVal = (void*)(uintptr_t)val.ulVal;
            }
            return true;
        }
        return false;
    }

    // 6. Function
    if (expectedType == "function" || expectedType == "fn") {
        return val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION || val.type == TzdValue::NONE;
    }

    // 7. Array
    if (expectedType == "array" || expectedType.find("[]") != std::string::npos) {
        return val.type == TzdValue::ARRAY || val.type == TzdValue::NONE;
    }

    // 8. Map
    if (expectedType == "map") {
        return val.type == TzdValue::MAP || val.type == TzdValue::NONE;
    }

    // 9. Class / OOP instance
    if (val.type == TzdValue::NONE) {
        return true;
    }
    if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition) {
        return val.instanceVal->definition->fullName == expectedType ||
               val.instanceVal->definition->simpleName == expectedType ||
               val.instanceVal->definition->isSubclassOf(expectedType);
    }
    if (val.type == TzdValue::CLASS_DEF && val.classDefVal) {
        return val.classDefVal->fullName == expectedType || val.classDefVal->simpleName == expectedType;
    }

    return false;
}

void TzdInstance::setMember(TzdSelector selector, const std::string& name, const struct TzdValue& val) {
    if (!definition) throw std::runtime_error("Instance definition is null");
    if (m_isCompressed) ensureDecompressed();
    if (selector == 0) {
        if (!name.empty()) selector = tzdInternSelector(name);
    }

    std::string memberName = name.empty() ? tzdGetSelectorName(selector) : name;
    ClassField* field = !memberName.empty() ? definition->findField(memberName) : nullptr;

    TzdValue finalVal = val;
    if (field && !field->type.empty() && field->type != "var" && field->type != "any" && field->type != "auto") {
        if (!isTzdValueCompatibleWithType(field->type, val, &finalVal)) {
            throw std::runtime_error("类型不匹配: 无法将 " + getTzdValueTypeName(val) + " 类型的值赋给 " + field->type + " 类型的成员 '" + memberName + "' (位于类 " + definition->fullName + " 中)");
        }
    }

    if (selector != 0) {
        const TzdMemberSlot* slot = definition->tzdDispatch.find(selector);
        if (slot) {
            if (slot->fieldIndex >= 0 && slot->fieldIndex < (int)fieldValues.size()) {
                TzdGarbageCollector::getInstance().writeBarrier(this, &fieldValues[slot->fieldIndex], fieldValues[slot->fieldIndex]);
                fieldValues[slot->fieldIndex] = finalVal;
                return;
            }
            if (slot->staticValue) {
                *slot->staticValue = finalVal;
                return;
            }
        }
    }

    if (!memberName.empty()) {
        if (TzdValue* staticVal = definition->findStaticValue(memberName)) {
            *staticVal = finalVal;
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
        std::string pName = cur->parentName;
        cur = TzdOopManager::getClass(pName);
        if (!cur) {
            throw std::runtime_error("找不到父类/基类定义: '" + pName + "'");
        }
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
        if (m.declaringClass.empty()) m.declaringClass = cls->fullName;
        m.templateVal = new TzdValue(); // 动态分配指针，避开头文件限制

        if (m.isNative) {
            m.templateVal->type = TzdValue::NATIVE_FUNCTION;
            m.templateVal->ensureExtra().nativeFunc = m.nativeWrapper;
        }
        else {
            m.templateVal->type = TzdValue::FUNCTION;
            m.templateVal->ensureExtra().funcBody = m.body;
            m.templateVal->ensureExtra().params = m.params; // 这里仅发生一次 Vector 拷贝
            m.templateVal->ensureExtra().paramTypes = m.paramTypes;
            m.templateVal->ensureExtra().isVararg = m.isVararg;
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
    cls->rebuildFieldIndexMap();

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

void TzdOopManager::unregisterClass(const std::string& name) {
    auto it = classMap.find(name);
    if (it != classMap.end()) {
        std::string simple = it->second->simpleName;
        classMap.erase(it);
        auto sit = simpleNameCache.find(simple);
        if (sit != simpleNameCache.end()) {
            auto& vec = sit->second;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [&](TzdClassDef* d){ return d->fullName == name; }), vec.end());
            if (vec.empty()) simpleNameCache.erase(sit);
        }
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
    if (m_isCompressed) ensureDecompressed();
    if (index >= 0 && index < (int)fieldValues.size()) return &fieldValues[index];
    return nullptr;
}

TzdValue TzdInstance::getMemberByIndex(int index) {
    if (m_isCompressed) {
        if (m_compressed) {
            for (const auto& entry : m_compressed->fields) {
                if (entry.index == (uint16_t)index) return entry.value;
            }
        }
        if (definition && index < (int)definition->defaultFieldValues.size()) {
            return definition->defaultFieldValues[index];
        }
        return TzdValue();
    }
    if (index >= 0 && index < (int)fieldValues.size()) return fieldValues[index];
    return TzdValue();
}

void TzdInstance::setMemberByIndex(int index, const TzdValue& val) {
    if (m_isCompressed) ensureDecompressed();
    if (index >= 0 && index < (int)fieldValues.size() && definition) {
        TzdValue finalVal = val;
        if (index < (int)definition->fieldIndices.size()) {
            const std::string& fieldName = definition->fieldIndices[index].first;
            ClassField* field = definition->findField(fieldName);
            if (field && !field->type.empty() && field->type != "var" && field->type != "any" && field->type != "auto") {
                if (!isTzdValueCompatibleWithType(field->type, val, &finalVal)) {
                    throw std::runtime_error("类型不匹配: 无法将 " + getTzdValueTypeName(val) + " 类型的值赋给 " + field->type + " 类型的成员 '" + fieldName + "' (位于类 " + definition->fullName + " 中)");
                }
            }
        }
        // SATB write barrier: snapshot old value before overwrite
        TzdGarbageCollector::getInstance().writeBarrier(this, &fieldValues[index], fieldValues[index]);
        fieldValues[index] = finalVal;
    }
}