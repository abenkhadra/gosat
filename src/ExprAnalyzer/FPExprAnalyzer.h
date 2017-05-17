//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include "z3++.h"
#include <unordered_map>

namespace gosat {

/**
 * /brief An analyzer to get relevant properties of FP expressions
 */
class FPExprAnalyzer {
public:
    FPExprAnalyzer();

    virtual ~FPExprAnalyzer() = default;

    FPExprAnalyzer(const FPExprAnalyzer&) = default;

    FPExprAnalyzer& operator=(const FPExprAnalyzer&) = default;

    FPExprAnalyzer& operator=(FPExprAnalyzer&&) = default;

    bool hasRNERoundingMode(const z3::expr& exp) const noexcept;

    void analyze(const z3::expr& expr) noexcept;

    void prettyPrintSummary(const std::string& formula_name) const noexcept;

public:
    uint m_float_var_count;
    uint m_double_var_count;
    uint m_const_count;
    bool m_is_linear;
    bool m_has_double_const;
    bool m_has_float_const;
    bool m_has_non_fp_const;
    bool m_has_non_rne_round_mode;
    bool m_has_unsupported_expr;
    std::unordered_map<unsigned int, std::string> m_var_sym_map;
    std::unordered_map<unsigned int, float> m_fp32_const_sym_map;
    std::unordered_map<unsigned int, double> m_fp64_const_sym_map;
};
}
