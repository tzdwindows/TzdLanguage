#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct TzdValue;
struct ClassMethod;

using TzdSelector = uint32_t;

// Zero is reserved for an unresolved selector. IDs are process-local, never serialized.
TzdSelector tzdInternSelector(const std::string& name);
std::string tzdGetSelectorName(TzdSelector selector);

struct TzdMemberSlot {
    int fieldIndex = -1;
    TzdValue* staticValue = nullptr;
    ClassMethod* method = nullptr;

    inline bool isValid() const {
        return fieldIndex >= 0 || staticValue != nullptr || method != nullptr;
    }
    inline bool isField() const { return fieldIndex >= 0; }
    inline bool isMethod() const { return method != nullptr; }
    inline bool isStatic() const { return staticValue != nullptr; }
};

class TzdDispatchTable {
public:
    static constexpr unsigned pageBits = 5;
    static constexpr unsigned pageSize = 1u << pageBits;
    using Page = std::array<TzdMemberSlot, pageSize>;

    inline const TzdMemberSlot* find(TzdSelector selector) const {
        if (selector == 0) return nullptr;
        const size_t page = selector >> pageBits;
        if (page >= pages.size() || !pages[page]) return nullptr;
        const TzdMemberSlot& slot = (*pages[page])[selector & (pageSize - 1)];
        return slot.isValid() ? &slot : nullptr;
    }

    // Build before publishing the class; readers never mutate the table.
    inline TzdMemberSlot& add(TzdSelector selector) {
        const size_t page = selector >> pageBits;
        if (page >= pages.size()) pages.resize(page + 1);
        if (!pages[page]) pages[page] = std::make_unique<Page>();
        return (*pages[page])[selector & (pageSize - 1)];
    }

    void clear() {
        pages.clear();
    }

private:
    std::vector<std::unique_ptr<Page>> pages;
};
