//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include "nlopt.h"
#include <string>
#include <vector>

extern "C" {
#include "gofuncs.h"
}

namespace gosat {

class GOFuncsMap {
    using  FuncEntry = std::pair<nlopt_func, unsigned> ;
public:
    GOFuncsMap() {
        m_global_func_vec =
            {
#include "gofuncs.api"
                {"_dummy_func", {m_dummy_func, 1}}
            };
        m_global_func_vec.pop_back();
    }
    virtual ~GOFuncsMap() = default;

    const std::vector<std::pair<std::string, FuncEntry>> &
    getFuncMap() const noexcept { return m_global_func_vec;};

private:
    nlopt_func m_dummy_func;
    std::vector<std::pair<std::string, FuncEntry>> m_global_func_vec;
};
}
