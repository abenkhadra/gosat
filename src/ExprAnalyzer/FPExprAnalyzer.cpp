//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "FPExprAnalyzer.h"
#include "Utils/fpa_util.h"

namespace gosat {

FPExprAnalyzer::FPExprAnalyzer() :
    m_float_var_count{0},
    m_double_var_count{0},
    m_const_count{0},
    m_is_linear{true},
    m_has_double_const{false},
    m_has_float_const{false},
    m_has_non_fp_const{false},
    m_has_non_rne_round_mode{false},
    m_has_unsupported_expr{false} {
}

bool FPExprAnalyzer::hasRNERoundingMode(const z3::expr &exp) const noexcept {
    return exp.arg(0).decl().decl_kind() == Z3_OP_FPA_RM_NEAREST_TIES_TO_EVEN;
}

void FPExprAnalyzer::analyze(const z3::expr &expr) noexcept {
    //std::cout << expr << "\n";
    // FP expression can be
    //   - Unary (uninterpreted) function
    //   - Binary (uninterpreted) function
    //   - Binary function with rounding mode
    if (!expr.is_app()) {
        // is_app <==> Z3_NUMERAL_AST || Z3_APP_AST
        m_has_unsupported_expr = true;
        return;
    }
    if (expr.is_numeral()) {
        if (expr.get_sort().sort_kind() == Z3_FLOATING_POINT_SORT) {
            unsigned signd = Z3_fpa_get_sbits(expr.ctx(), expr.get_sort());
            unsigned expo = Z3_fpa_get_ebits(expr.ctx(), expr.get_sort());
            if (fpa_util::isFloat32(expo, signd)) {
                if (m_fp32_const_sym_map.find(expr.hash())
                    == m_fp32_const_sym_map.cend()) {
                    float result = fpa_util::toFloat32(expr);
                    m_fp32_const_sym_map[expr.hash()] = result;
                    m_const_count++;
                }
                return;
            }
            if (fpa_util::isFloat64(expo, signd)) {
                if (m_fp64_const_sym_map.find(expr.hash())
                    == m_fp64_const_sym_map.cend()) {
                    double result = fpa_util::toFloat64(expr);
                    m_fp64_const_sym_map[expr.hash()] = result;
                    m_const_count++;
                }
            } else
                m_has_non_fp_const = true;
            return;
        }
        if (expr.get_sort().sort_kind() == Z3_ROUNDING_MODE_SORT) {
            if (expr.decl().decl_kind() != Z3_OP_FPA_RM_NEAREST_TIES_TO_EVEN) {
                m_has_non_rne_round_mode = true;
            }
            return;
        }
        m_has_non_fp_const = true;
        return;
    }
    if (fpa_util::isFPVar(expr)) {
        if (m_var_sym_map.find(expr.hash()) == m_var_sym_map.cend()) {
            m_var_sym_map[expr.hash()] = expr.decl().name().str();
            if (fpa_util::isFloat32VarDecl(expr))
                m_float_var_count++;
            else
                m_double_var_count++;
        }
        return;
    }
    if (fpa_util::isNonLinearFPExpr(expr)) {
        m_is_linear = false;
    }
    for (uint i = 0; i < expr.num_args(); ++i) {
        analyze(expr.arg(i));
    }
    return;
}

void FPExprAnalyzer::prettyPrintSummary(const std::string &formula_name) const noexcept {
    std::cout << "Formula: " << formula_name
              <<"\nIs linear ("
              << std::string((m_is_linear)? "yes": "no")
              << ")\nHas float variables (" << m_float_var_count
              << ")\nHas double variables (" << m_double_var_count
              << ")\nHas const values (" << m_const_count
              << ")\nHas unsupported expr ("
              << std::string((m_has_unsupported_expr)? "yes": "no")
              << ")\n";
}
}
