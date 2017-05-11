//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include "z3++.h"
#include <vector>
#include <unordered_map>

namespace gosat {
class ModelValidator {
    ModelValidator(z3::context *ctx, z3::expr *expr);
    virtual ~ModelValidator() = default;
    ModelValidator(const ModelValidator &) = default;
    ModelValidator &operator=(const ModelValidator &) = default;
    ModelValidator &operator=(ModelValidator &&) = default;
    bool isValid(const double *model, size_t model_size);

private:
    void replaceVarsWithModel(z3::expr &expr, const double *model);

private:
    z3::context *m_ctx;
    z3::expr *m_expr;
    size_t m_model_size;
    size_t m_substitute_var_idx;
    std::unordered_map<unsigned, bool> m_visited_expr;
};
}
