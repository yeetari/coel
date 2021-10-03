#pragma once

#include <coel/ir/Function.hh>
#include <coel/support/List.hh>

#include <cstddef>
#include <memory>
#include <string>

namespace coel::ir {

class Unit {
    List<Function> m_functions;

public:
    auto begin() const { return m_functions.begin(); }
    auto end() const { return m_functions.end(); }

    Function *append_function(std::string name, std::size_t argument_count);
    Function *find_function(std::string_view name);
};

} // namespace coel::ir