#include "Liveness.hh"

#include <coel/ir/BasicBlock.hh>
#include <coel/ir/Function.hh>
#include <coel/ir/InstVisitor.hh>
#include <coel/ir/Instructions.hh>
#include <coel/support/Assert.hh>

namespace coel::codegen {

Liveness::Liveness(ir::Function &function, const Graph<ir::BasicBlock> &cfg) : m_cfg(&cfg) {
    for (const auto &argument : function.arguments()) {
        COEL_ASSERT(!m_def_map.contains(&argument));
        m_def_map.emplace(&argument, cfg.entry());
    }
    for (auto *block : function) {
        m_block = block;
        for (auto *inst : *block) {
            m_inst = inst;
            inst->accept(this);
        }
    }
}

void Liveness::visit_def(const ir::Value *value) {
    COEL_ASSERT(!m_def_map.contains(value));
    m_def_map.emplace(value, m_block);
}

void Liveness::visit_use(const ir::Value *value) {
    if (value->kind() == ir::ValueKind::Constant) {
        return;
    }
    if (const auto *reg = value->as<Register>()) {
        if (reg->physical()) {
            return;
        }
    }
    auto *def_block = m_def_map.at(value);
    std::vector<ir::BasicBlock *> work_queue;
    work_queue.push_back(m_block);
    while (!work_queue.empty()) {
        auto *block = work_queue.back();
        work_queue.pop_back();
        auto begin = block->begin();
        auto end = block->end();
        if (block == def_block) {
            // TODO: Properly.
        }
        if (block == m_block) {
            end = block->iterator(m_inst);
            ++end;
        }
        for (auto it = end;; --it) {
            if (it == block->end()) {
                continue;
            }
            m_live_map[*it][value] = true;
            if (it == begin) {
                break;
            }
        }
        if (block == def_block) {
            break;
        }
        for (auto *pred : m_cfg->preds(block)) {
            work_queue.push_back(pred);
        }
    }
}

bool Liveness::live_at(const ir::Value *value, const ir::Instruction *point) {
    return m_live_map[point][value];
}

void Liveness::visit(ir::BinaryInst *binary) {
    visit_def(binary);
    visit_use(binary->lhs());
    visit_use(binary->rhs());
}

void Liveness::visit(ir::CallInst *call) {
    visit_def(call);
    for (auto *arg : call->args()) {
        visit_use(arg);
    }
}

void Liveness::visit(ir::CompareInst *compare) {
    visit_def(compare);
    visit_use(compare->lhs());
    visit_use(compare->rhs());
}

void Liveness::visit(ir::CondBranchInst *cond_branch) {
    visit_use(cond_branch->cond());
}

void Liveness::visit(ir::CopyInst *copy) {
    visit_def(copy->dst());
    visit_use(copy->src());
}

void Liveness::visit(ir::LoadInst *load) {
    visit_def(load);
}

void Liveness::visit(ir::RetInst *ret) {
    visit_use(ret->value());
}

} // namespace coel::codegen
