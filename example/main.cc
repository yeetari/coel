#include <codegen/codegen/Context.hh>
#include <codegen/codegen/CopyInserter.hh>
#include <codegen/codegen/RegisterAllocator.hh>
#include <codegen/graph/DepthFirstSearch.hh>
#include <codegen/graph/DotGraph.hh>
#include <codegen/ir/BasicBlock.hh>
#include <codegen/ir/Constant.hh>
#include <codegen/ir/Dumper.hh>
#include <codegen/ir/Function.hh>
#include <codegen/ir/Instructions.hh>
#include <codegen/ir/Unit.hh>
#include <codegen/support/Assert.hh>
#include <codegen/x86/Backend.hh>

#include <fmt/core.h>

#include <fstream>
#include <sstream>
#include <sys/mman.h>

int main() {
    ir::Unit unit;

    auto *main = unit.append_function("main", 0);
    auto *callee = unit.append_function("foo", 2);

    auto *main_entry = main->append_block();
    auto *call = main_entry->append<ir::CallInst>(callee, std::vector<ir::Value *>{ir::Constant::get(10), ir::Constant::get(20)});
    main_entry->append<ir::RetInst>(call);

    auto *callee_entry = callee->append_block();
    auto *true_dst = callee->append_block();
    auto *false_dst = callee->append_block();
    auto *add1 = callee_entry->append<ir::AddInst>(ir::Constant::get(5), callee->argument(0));
    callee_entry->append<ir::CondBranchInst>(ir::Constant::get(1), true_dst, false_dst);
    auto *true_add = true_dst->append<ir::AddInst>(add1, callee->argument(1));
    true_dst->append<ir::RetInst>(true_add);
    auto *false_add = false_dst->append<ir::AddInst>(add1, ir::Constant::get(40));
    false_dst->append<ir::RetInst>(false_add);

    fmt::print("=====\n");
    fmt::print("INPUT\n");
    fmt::print("=====\n");
    ir::dump(unit);

    codegen::Context context(unit);
    codegen::insert_copies(context);

    fmt::print("===============\n");
    fmt::print("INSERTED COPIES\n");
    fmt::print("===============\n");
    ir::dump(unit);

    codegen::register_allocate(context);

    fmt::print("===================\n");
    fmt::print("ALLOCATED REGISTERS\n");
    fmt::print("===================\n");
    ir::dump(unit);

    auto compiled = x86::compile(unit);
    auto encoded = x86::encode(compiled);
    std::ofstream output_file("foo.bin", std::ios::binary | std::ios::trunc);
    output_file.write(reinterpret_cast<const char *>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
    auto *code_region =
        mmap(nullptr, encoded.size(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(code_region, encoded.data(), encoded.size());
    return reinterpret_cast<int (*)()>(code_region)();
}
