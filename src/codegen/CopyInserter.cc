#include <codegen/CopyInserter.hh>

#include <codegen/Context.hh>
#include <ir/BasicBlock.hh>
#include <ir/Function.hh>
#include <ir/InstVisitor.hh>
#include <ir/Instructions.hh>
#include <ir/Unit.hh>
#include <support/Assert.hh>

namespace codegen {
namespace {

class CopyInserter final : public ir::InstVisitor {
    Context &m_context;
    ir::BasicBlock *m_block{nullptr};

public:
    explicit CopyInserter(Context &context) : m_context(context) {}

    void run(ir::Function *function);
    void visit(ir::AddInst *) override;
    void visit(ir::BranchInst *) override;
    void visit(ir::CallInst *) override;
    void visit(ir::CondBranchInst *) override;
    void visit(ir::CopyInst *) override;
    void visit(ir::RetInst *) override;
};

void CopyInserter::run(ir::Function *function) {
    for (auto *block : *function) {
        m_block = block;
        for (auto it = block->begin(); it != block->end(); ++it) {
            auto *call = (*it)->as<ir::CallInst>();
            (*it)->accept(this);
            if (call != nullptr) {
                for (std::size_t i = 0; i < call->args().size(); i++) {
                    ++it;
                }
            }
            ++it;
        }
    }
}

void CopyInserter::visit(ir::AddInst *add) {
    auto *copy = m_context.create_virtual();
    m_block->insert<ir::CopyInst>(add, copy, add->lhs());
    add->set_lhs(copy);
}

void CopyInserter::visit(ir::BranchInst *) {
    ENSURE_NOT_REACHED();
}

void CopyInserter::visit(ir::CallInst *call) {
    // TODO: Assuming target/ABI registers.
    std::array argument_registers{7, 6, 2, 1, 8, 9};
    for (std::size_t i = 0; i < call->args().size(); i++) {
        auto *phys = m_context.create_physical(argument_registers[i]);
        m_block->insert<ir::CopyInst>(call, phys, call->args()[i]);
    }
    auto *copy = m_context.create_virtual();
    m_block->insert<ir::CopyInst>(++m_block->iterator(call), copy, m_context.create_physical(0));
    call->replace_all_uses_with(copy);
}

void CopyInserter::visit(ir::CondBranchInst *cond_branch) {
    auto *copy = m_context.create_virtual();
    m_block->insert<ir::CopyInst>(cond_branch, copy, cond_branch->cond());
    cond_branch->set_cond(copy);
}

void CopyInserter::visit(ir::CopyInst *) {
    ENSURE_NOT_REACHED();
}

void CopyInserter::visit(ir::RetInst *ret) {
    // TODO: Assuming target register 0 is return register.
    auto *copy = m_context.create_physical(0);
    m_block->insert<ir::CopyInst>(ret, copy, ret->value());
    ret->set_value(copy);
}

} // namespace

void insert_copies(Context &context) {
    CopyInserter inserter(context);
    for (auto *function : context.unit()) {
        inserter.run(function);
    }
}

} // namespace codegen
