//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "FPAUtils.h"
#include <cmath>
#include <cstring>
#include <limits>

/**
 * /brief Provides a scaled distance value between representations of two double variables
 */
double fp64_dis(const double a, const double b)
{
    /*
     * Helpful. Ordered layout of FP32
     * 1 11111111 ----------------------- -nan
     * 1 11111111 00000000000000000000000 -oo
     * 1 -------- ----------------------- -norm
     * 1 00000000 ----------------------- -denorm
     * 1 00000000 00000000000000000000000 -o
     * 0 11111111 ----------------------- +nan
     * 0 11111111 00000000000000000000000 +oo
     * 0 -------- ----------------------- +norm
     * 0 00000000 ----------------------- +denorm
     * 0 00000000 00000000000000000000000 +o
     */

    if (a == b || std::isnan(a) || std::isnan(b)) {
        return 0;
    }
    const double scale = pow(2, 54);
    uint64_t a_uint = *(const uint64_t*) (&a);
    uint64_t b_uint = *(const uint64_t*) (&b);
    if ((a_uint & 0x8000000000000000) != (b_uint & 0x8000000000000000)) {
        // signs are not equal return sum
        return ((double)
                ((a_uint & 0x7FFFFFFFFFFFFFFF) +
                 (b_uint & 0x7FFFFFFFFFFFFFFF))) / scale;
    }
    b_uint &= 0x7FFFFFFFFFFFFFFF;
    a_uint &= 0x7FFFFFFFFFFFFFFF;
    if (a_uint < b_uint) {
        return ((double) (b_uint - a_uint)) / scale;
    }
    return ((double) (a_uint - b_uint)) / scale;
}

double fp64_isnan(double a, double flag)
{
    if (flag != 0) {
        // flag set, invert result
        return std::isnan(a)? 1.0: 0.0;
    } else {
        return std::isnan(a)? 0.0: 1.0;
    }
}

namespace gosat {
namespace fpa_util {

bool isBoolExpr(const z3::expr& expr) noexcept
{
    switch (expr.decl().decl_kind()) {
        case Z3_OP_TRUE:
        case Z3_OP_FALSE:
        case Z3_OP_EQ:
        case Z3_OP_FPA_EQ:
        case Z3_OP_NOT:
        case Z3_OP_AND:
        case Z3_OP_OR:
            // Floating point operations
        case Z3_OP_FPA_LT:
        case Z3_OP_FPA_GT:
        case Z3_OP_FPA_LE:
        case Z3_OP_FPA_GE:
        case Z3_OP_FPA_IS_NAN:
        case Z3_OP_FPA_IS_INF:
        case Z3_OP_FPA_IS_ZERO:
        case Z3_OP_FPA_IS_NORMAL:
        case Z3_OP_FPA_IS_SUBNORMAL:
        case Z3_OP_FPA_IS_NEGATIVE:
        case Z3_OP_FPA_IS_POSITIVE:
            return true;
        default:
            return false;
    }
}

bool
isFloat32VarDecl(const z3::expr& expr) noexcept
{
    // XXX: this is ugly! no API direct way to get exponent and significand sizes
    Z3_string decl_str =
            Z3_ast_to_string(expr.ctx(),
                             static_cast<z3::ast>(expr.decl().range()));
    if (strstr(decl_str + 17, "8") != NULL) {
        assert((strstr(decl_str + 19, "24") != NULL)
               && "Invalid Float32 variable declaration");
        return true;
    }
    return false;
}

bool
isFloat64VarDecl(const z3::expr& expr) noexcept
{
    // XXX: this is ugly! no direct way to get exponent and significand sizes
    Z3_string decl_str = Z3_ast_to_string(expr.ctx(),
                                          static_cast<z3::ast>(expr.decl().range()));
    if (strstr(decl_str + 17, "11") != NULL) {
        assert((strstr(decl_str + 19, "53") != NULL)
               && "Invalid Float32 variable declaration");
        return true;
    }
    return false;
}

bool
isNonLinearFPExpr(const z3::expr& expr) noexcept
{
    if (expr.get_sort().sort_kind() != Z3_FLOATING_POINT_SORT) {
        return false;
    }
    switch (expr.decl().decl_kind()) {
        case Z3_OP_FPA_MUL:
        case Z3_OP_FPA_DIV:
        case Z3_OP_FPA_REM:
        case Z3_OP_FPA_ABS:
        case Z3_OP_FPA_MIN:
        case Z3_OP_FPA_MAX:
        case Z3_OP_FPA_FMA:
        case Z3_OP_FPA_SQRT:
        case Z3_OP_FPA_ROUND_TO_INTEGRAL:
            return true;
        default:
            return false;
    }
}

inline unsigned short
getBaseofNumStr(const char* numerical) noexcept
{
    if (numerical[1] == 'b') { return 2; }
    if (numerical[1] == 'x') { return 16; }
    if (numerical[1] == 'o') { return 8; }
    return 0;
}

/**
 *
 * @param expr
 * @return float value of expression
 * @pre isFloat32
 */
float
toFloat32(const z3::expr& expr) noexcept
{
    switch (expr.decl().decl_kind()) {
        case Z3_OP_FPA_PLUS_INF:
            return std::numeric_limits<float>::infinity();
        case Z3_OP_FPA_MINUS_INF:
            return -(std::numeric_limits<float>::infinity());
        case Z3_OP_FPA_NAN:
            return std::numeric_limits<float>::quiet_NaN();
        case Z3_OP_FPA_PLUS_ZERO:
            return 0;
        case Z3_OP_FPA_MINUS_ZERO:
            return -0;
        default:
            break;
    }
    // XXX: proper functions are not working as of Z3 v4.5.0
    //  - Z3_fpa_get_numeral_sign(expr.ctx(), static_cast<z3::ast>(expr), &sign);
    //  - Z3_fpa_get_numeral_exponent_int64(expr.ctx(), static_cast<z3::ast>(expr), &exponent);
    //  - Z3_fpa_get_numeral_significand_uint64(expr.ctx(), static_cast<z3::ast>(expr), &significand);
    assert(expr.num_args() == 3 && "<toFloat32> Invalid FP constant");
    Z3_string bv_str = Z3_ast_to_string(expr.ctx(),
                                        static_cast<z3::ast >(expr.arg(0)));
    int sign = std::stoi(bv_str + 2, NULL, getBaseofNumStr(bv_str));
    bv_str = Z3_ast_to_string(expr.ctx(),
                              static_cast<Z3_ast >(expr.arg(1)));
    uint64_t exponent = std::stoul(bv_str + 2, NULL, getBaseofNumStr(bv_str));
    bv_str = Z3_ast_to_string(expr.ctx(),
                              static_cast<z3::ast >(expr.arg(2)));
    uint64_t significand = std::stoull(bv_str + 2, NULL,
                                       getBaseofNumStr(bv_str));
    uint32_t result = static_cast<uint32_t >(exponent);
    result <<= 23;
    result |= static_cast<uint32_t >(significand);
    if (sign) result |= 0x80000000;
    return *(reinterpret_cast<float*>(&result));
}

/**
 *
 * @param exp
 * @return double value of expression
 * @pre isFloat64
 */
double
toFloat64(const z3::expr& expr) noexcept
{
    switch (expr.decl().decl_kind()) {
        case Z3_OP_FPA_PLUS_INF:
            return std::numeric_limits<double>::infinity();
        case Z3_OP_FPA_MINUS_INF:
            return -(std::numeric_limits<double>::infinity());
        case Z3_OP_FPA_NAN:
            return std::numeric_limits<double>::quiet_NaN();
        case Z3_OP_FPA_PLUS_ZERO:
            return 0;
        case Z3_OP_FPA_MINUS_ZERO:
            return -0;
        default:
            break;
    }
    assert(expr.num_args() == 3 && "<toFloat64> Invalid FP constant");
    Z3_string bv_str = Z3_ast_to_string(expr.ctx(),
                                        static_cast<z3::ast >(expr.arg(0)));
    int sign = std::stoi(bv_str + 2, NULL, getBaseofNumStr(bv_str));
    bv_str = Z3_ast_to_string(expr.ctx(),
                              static_cast<Z3_ast >(expr.arg(1)));
    uint64_t exponent = std::stoul(bv_str + 2, NULL, getBaseofNumStr(bv_str));
    bv_str = Z3_ast_to_string(expr.ctx(),
                              static_cast<z3::ast >(expr.arg(2)));
    uint64_t significand = std::stoull(bv_str + 2, NULL,
                                       getBaseofNumStr(bv_str));
    // Hidden bit must not be represented see:
    // http://smtlib.cs.uiowa.edu/theories-FloatingPoint.shtml
    uint64_t result = exponent;
    result <<= 52;
    result |= significand;
    if (sign) result |= 0x8000000000000000;
    return *(reinterpret_cast<double*>(&result));
}
}
}
