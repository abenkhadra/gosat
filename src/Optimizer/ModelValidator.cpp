//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "ModelValidator.h"
#include "Utils/fpa_util.h"

namespace gosat {

ModelValidator::ModelValidator(z3::context *ctx, z3::expr *expr) :
    m_ctx{ctx},
    m_expr{expr},
    m_model_size{0},
    m_substitute_var_idx{0} {}

bool
ModelValidator::isValid(const double *model,
                        size_t model_size) {
    m_model_size = model_size;
    m_substitute_var_idx = 0;
    replaceVarsWithModel(*m_expr, model);
    std::cout << *m_expr << std::endl;
    return false;
}

void
ModelValidator::replaceVarsWithModel(z3::expr &expr, const double *model) {
    if (m_substitute_var_idx >= m_model_size) {
        return;
    }
    auto result = m_visited_expr.insert({expr.hash(), true});
    if (!result.second) {
        // already visited
        return;
    }
    if (fpa_util::isFPVar(expr)) {
        // replace variable with FP numeral
        if (fpa_util::isFloat32VarDecl(expr)) {
            auto sort = Z3_mk_fpa_sort(*m_ctx, 8, 24);
            const float var_f = static_cast<float>(model[m_substitute_var_idx]);
            auto var = Z3_mk_fpa_numeral_float(*m_ctx,
                                                var_f,
                                                sort);
            z3::expr_vector vec (*m_ctx);
            vec.push_back(z3::expr(*m_ctx, var));
            expr.substitute(vec);
        } else {
            auto sort = Z3_mk_fpa_sort(*m_ctx, 11, 53);
            auto var = Z3_mk_fpa_numeral_double(*m_ctx,
                                                model[m_substitute_var_idx],
                                                sort);
            z3::expr_vector vec (*m_ctx);
            vec.push_back(z3::expr(*m_ctx, var));
            expr.substitute(vec);
        }
        m_substitute_var_idx++;
        return;
    }
    for (uint i = 0; i < expr.num_args(); ++i) {
        auto rval_expr = expr.arg(i);
        replaceVarsWithModel(rval_expr, model);
    }
}
}
