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

double fp64_dis(double a, double b);
double fp64_isnan(double a, double flag);

namespace gosat {
namespace fpa_util {

bool inline
isFloat32(const unsigned exponent, const unsigned significand) noexcept
{
    return (exponent == 8 && significand == 24);
}

bool inline
isFloat64(const unsigned exponent, const unsigned significand) noexcept
{
    return (exponent == 11 && significand == 53);
}


bool inline isFPVar(const z3::expr& expr) noexcept
{
    return (expr.num_args() == 0
            && expr.decl().decl_kind() == Z3_OP_UNINTERPRETED
            && expr.get_sort().sort_kind() == Z3_FLOATING_POINT_SORT);
}

bool isNonLinearFPExpr(const z3::expr& expr) noexcept;

bool isFloat32VarDecl(const z3::expr& expr) noexcept;

bool isFloat64VarDecl(const z3::expr& expr) noexcept;

bool isBoolExpr(const z3::expr& expr) noexcept;

/**
 *
 * @param expr
 * @return float value of expression
 * @pre isFloat32
 */
float toFloat32(const z3::expr& expr) noexcept;

/**
 *
 * @param exp
 * @return double value of expression
 * @pre isFloat64
 */
double toFloat64(const z3::expr& expr) noexcept;

}
}
