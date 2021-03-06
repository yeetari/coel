#pragma once

#include <coel/ir/Instructions.hh>
#include <coel/ir/Value.hh>
#include <coel/support/List.hh>
#include <coel/support/ListNode.hh>

#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace coel::ir {

class BasicBlock final : public Value, public ListNode {
    List<Instruction> m_instructions;

public:
    BasicBlock() : Value(ValueKind::BasicBlock, nullptr) {}
    BasicBlock(const BasicBlock &) = delete;
    BasicBlock(BasicBlock &&) = delete;
    ~BasicBlock() override;

    BasicBlock &operator=(const BasicBlock &) = delete;
    BasicBlock &operator=(BasicBlock &&) = delete;

    auto begin() const { return m_instructions.begin(); }
    auto end() const { return m_instructions.end(); }
    auto iterator(Instruction *position) const;

    template <std::derived_from<Instruction> Inst, typename... Args>
    Inst *insert(ListIterator<Instruction> position, Args &&...args);

    template <std::derived_from<Instruction> Inst, typename... Args>
    Inst *insert(Instruction *position, Args &&...args);

    template <std::derived_from<Instruction> Inst, typename... Args>
    Inst *prepend(Args &&...args);

    template <std::derived_from<Instruction> Inst, typename... Args>
    Inst *append(Args &&...args);

    bool empty() const;
    bool has_terminator() const;
};

inline auto BasicBlock::iterator(Instruction *position) const {
    std::size_t index = 0;
    for (auto *inst : *this) {
        if (inst == position) {
            break;
        }
        index++;
    }
    return ListIterator<Instruction>(position);
}

template <std::derived_from<Instruction> Inst, typename... Args>
Inst *BasicBlock::insert(ListIterator<Instruction> position, Args &&...args) {
    return m_instructions.emplace<Inst>(position, std::forward<Args>(args)...);
}

template <std::derived_from<Instruction> Inst, typename... Args>
Inst *BasicBlock::insert(Instruction *position, Args &&...args) {
    return insert<Inst>(iterator(position), std::forward<Args>(args)...);
}

template <std::derived_from<Instruction> Inst, typename... Args>
Inst *BasicBlock::prepend(Args &&...args) {
    return insert<Inst>(begin(), std::forward<Args>(args)...);
}

template <std::derived_from<Instruction> Inst, typename... Args>
Inst *BasicBlock::append(Args &&...args) {
    return insert<Inst>(end(), std::forward<Args>(args)...);
}

} // namespace coel::ir
