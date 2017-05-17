//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "FPExprCodeGenerator.h"
#include "Utils/FPAUtils.h"
#include <algorithm>

namespace gosat {

FPExprCodeGenerator::FPExprCodeGenerator() :
        m_has_invalid_fp_const{false},
        m_has_unsupported_expr{false}
{
}

bool
FPExprCodeGenerator::hasRNERoundingMode(const z3::expr& expr) const noexcept
{
    return expr.arg(0).decl().decl_kind() == Z3_OP_FPA_RM_NEAREST_TIES_TO_EVEN;
}

void
FPExprCodeGenerator::genFuncCode
        (const std::string& func_name, const z3::expr& expr) noexcept
{
    m_func_code = "";
    m_func_code.reserve(16 * 1024);
    includeFuncHeader(func_name);
    genFuncCodeRecursive(expr, false);
    m_func_code.append("return expr_" + std::to_string(expr.hash()) + ";\n");
    includeFuncFooter();
}

void
FPExprCodeGenerator::includeFuncHeader(const std::string& func_name) noexcept
{
    m_func_code += FPExprCodeGenerator::genFuncSignature(func_name) + "{ \n";
}

void
FPExprCodeGenerator::includeFuncFooter() noexcept
{
    m_func_code += "}\n";
}

const Symbol*
FPExprCodeGenerator::genNumeralCode(const z3::expr& expr)
{
    if (expr.get_sort().sort_kind() == Z3_FLOATING_POINT_SORT) {
        unsigned signd = Z3_fpa_get_sbits(expr.ctx(), expr.get_sort());
        unsigned expo = Z3_fpa_get_ebits(expr.ctx(), expr.get_sort());
        if (fpa_util::isFloat32(expo, signd)) {
            auto result_pair = insertSymbol(SymbolKind::kFP32Const, expr);
            if (result_pair.second) {
                // TODO: handling FP32 should be configurable
                float result = fpa_util::toFloat32(expr);
                m_func_code += "const float ";
                m_func_code += result_pair.first->name();
                m_func_code += " = " + std::to_string(result) + "f ;\n";
            }
            return result_pair.first;
        } else {
            assert(fpa_util::isFloat64(expo, signd)
                   && "Invalid float format!");
            auto result_pair = insertSymbol(SymbolKind::kFP64Const, expr);
            if (result_pair.second) {
                double result = fpa_util::toFloat64(expr);
                m_func_code += "const double ";
                m_func_code += result_pair.first->name();
                m_func_code += " = " + std::to_string(result) + " ;\n";
            }
            return result_pair.first;
        }
    }
    if (expr.decl().decl_kind() == Z3_OP_BNUM) {
        auto result_pair = insertSymbol(SymbolKind::kFP64Const, expr);
        if (result_pair.second) {
            std::string result =
                    Z3_ast_to_string(expr.ctx(), static_cast<z3::ast>(expr));
            result.replace(0, 1, 1, '0');
            m_func_code += "const double ";
            m_func_code += result_pair.first->name();
            m_func_code += " = " + result + " ;\n";
        }
        return result_pair.first;
    }
    return nullptr;
}

const Symbol*
FPExprCodeGenerator::genFuncCodeRecursive
        (const z3::expr& expr, bool is_negated) noexcept
{
    if (!expr.is_app()) {
        // is_app <==> Z3_NUMERAL_AST || Z3_APP_AST
        m_has_unsupported_expr = true;
        return nullptr;
    }
    if (expr.is_numeral()) {
        return genNumeralCode(expr);
    }
    if (fpa_util::isFPVar(expr)) {
        // TODO: handling FP32 might be configurable here
        auto result_pair = insertSymbol(SymbolKind::kFP64Var, expr);
        if (result_pair.second) {
            m_func_code += "const double ";
            m_func_code += result_pair.first->name();
            m_func_code += " = " + CodeGenStr::kFunInput;
            m_func_code += "[" + std::to_string(m_var_sym_vec.size()) + "] ;\n";
            m_var_sym_vec.push_back(result_pair.first);
        }
        return result_pair.first;
    }
    if (!fpa_util::isBoolExpr(expr)) {
        is_negated = false;
    }
    SymbolKind kind =
            (is_negated) ? SymbolKind::kNegatedExpr : SymbolKind::kExpr;
    if (expr.decl().decl_kind() == Z3_OP_NOT && !is_negated) {
        is_negated = true;
    }
    auto result_pair = insertSymbol(kind, expr);
    if (!result_pair.second) {
        return result_pair.first;
    }
    // Expr not visited before
    std::vector<const Symbol*> arg_syms;
    arg_syms.reserve(expr.num_args());
    for (uint i = 0; i < expr.num_args(); ++i) {
        arg_syms.push_back(genFuncCodeRecursive(expr.arg(i), is_negated));
    }
    genExprCode(result_pair.first, arg_syms);
    return result_pair.first;
}

const std::string&
FPExprCodeGenerator::getFuncCode() const noexcept
{
    return m_func_code;
}

static inline std::string
genBinArgExpr(const char* op, const Symbol* arg_1, const Symbol* arg_2)
{
    return std::string()
           + arg_1->name()
           + op
           + arg_2->name()
           + ";\n";
}

static std::string
genMultiArgExpr(const char* op, std::vector<const Symbol*>& args_syms)
{
    assert(args_syms.size() >= 2 && "Invalid multi-argument expression");
    if (args_syms.size() == 2) {
        return genBinArgExpr(op, args_syms[0], args_syms[1]);
    }
    std::string result;
    for (unsigned i = 0; i < args_syms.size() - 1; ++i) {
        result += args_syms[i]->name();
        result += op;
    }
    return result + args_syms[args_syms.size() - 1]->name() + ";\n";
}

static inline
std::string genBinArgCompExpr
        (const char* op, const Symbol* arg_1, const Symbol* arg_2)
{
    return std::string("(")
           + arg_1->name()
           + op
           + arg_2->name()
           + ")"
           + "? 0: "
           + CodeGenStr::kFunDis
           + "("
           + arg_1->name()
           + ","
           + arg_2->name()
           + ")"
           + ";\n";
}

static inline
std::string genBinArgCompExpr2
        (const char* op, const Symbol* arg_1, const Symbol* arg_2)
{
    return std::string("(")
           + arg_1->name()
           + op
           + arg_2->name()
           + ")"
           + "? 0: "
           + CodeGenStr::kFunDis
           + "("
           + arg_1->name()
           + ", "
           + arg_2->name()
           + ")"
           + " + 1;\n";
}

static inline
std::string genEqCompExpr(const Symbol* arg_1, const Symbol* arg_2)
{
    return CodeGenStr::kFunDis
           + "("
           + arg_1->name()
           + ","
           + arg_2->name()
           + ")"
           + ";\n";
}

static inline
std::string genNotEqCompExpr(const Symbol* arg_1, const Symbol* arg_2)
{
    return std::string("(")
           + arg_1->name()
           + " != "
           + arg_2->name()
           + ")"
           + "? 0: 1 ;\n";
}

void
FPExprCodeGenerator::genExprCode(const Symbol* expr_sym,
                                 std::vector<const Symbol*>& args_syms) noexcept
{
    m_func_code.append("const double ");
    m_func_code.append(expr_sym->name());
    m_func_code.append(" = ");
    switch (expr_sym->expr()->decl().decl_kind()) {
        // Boolean operations
        case Z3_OP_TRUE:
            if (expr_sym->isNegated())
                m_func_code.append("1.0;\n");
            else
                m_func_code.append("0.0;\n");
            break;
        case Z3_OP_FALSE:
            if (expr_sym->isNegated())
                m_func_code.append("0.0;\n");
            else
                m_func_code.append("1.0;\n");
            break;
        case Z3_OP_EQ:
        case Z3_OP_FPA_EQ:
            if (expr_sym->isNegated())
                m_func_code.append
                        (genNotEqCompExpr(args_syms[0], args_syms[1]));
            else
                m_func_code.append
                        (genEqCompExpr(args_syms[0], args_syms[1]));
            break;
        case Z3_OP_NOT:
            m_func_code.append
                    (std::string(args_syms[0]->name()) + ";\n");
            break;
        case Z3_OP_AND:
            if (expr_sym->isNegated())
                m_func_code.append(genMultiArgExpr(" * ", args_syms));
            else
                m_func_code.append(genMultiArgExpr(" + ", args_syms));
            break;
        case Z3_OP_OR:
            if (expr_sym->isNegated())
                m_func_code.append(genMultiArgExpr(" + ", args_syms));
            else
                m_func_code.append(genMultiArgExpr(" * ", args_syms));
            break;
            // Floating point operations
        case Z3_OP_FPA_PLUS_INF:
            m_func_code.append
                    ("INFINITY;\n");
            break;
        case Z3_OP_FPA_MINUS_INF:
            m_func_code.append
                    ("-INFINITY;\n");
            break;
        case Z3_OP_FPA_NAN:
            m_func_code.append
                    ("NAN;\n");
            break;
        case Z3_OP_FPA_PLUS_ZERO:
            m_func_code.append
                    ("0;\n");
            break;
        case Z3_OP_FPA_MINUS_ZERO:
            m_func_code.append
                    ("-0;\n");
            break;
        case Z3_OP_FPA_ADD:
            m_func_code.append(
                    genBinArgExpr(" + ", args_syms[1], args_syms[2]));
            break;
        case Z3_OP_FPA_SUB:
            m_func_code.append(
                    genBinArgExpr(" - ", args_syms[1], args_syms[2]));
            break;
        case Z3_OP_FPA_NEG:
            m_func_code.append
                    ("-" + std::string(args_syms[0]->name()) + ";\n");
            break;
        case Z3_OP_FPA_MUL:
            m_func_code.append(
                    genBinArgExpr(" * ", args_syms[1], args_syms[2]));
            break;
        case Z3_OP_FPA_DIV:
            m_func_code.append(
                    genBinArgExpr(" / ", args_syms[1], args_syms[2]));
            break;
        case Z3_OP_FPA_REM:
            m_func_code.append(
                    genBinArgExpr(" % ", args_syms[1], args_syms[2]));
            break;
        case Z3_OP_FPA_ABS:
            m_func_code.append(
                    "abs(" + std::string(args_syms[0]->name()) + ");\n");
            break;
        case Z3_OP_FPA_LT:
            if (expr_sym->isNegated())
                m_func_code.append
                        (genBinArgCompExpr(" >= ", args_syms[0], args_syms[1]));
            else
                m_func_code.append
                        (genBinArgCompExpr2(" < ", args_syms[0], args_syms[1]));
            break;
        case Z3_OP_FPA_GT:
            if (expr_sym->isNegated())
                m_func_code.append
                        (genBinArgCompExpr(" <= ", args_syms[0], args_syms[1]));
            else
                m_func_code.append
                        (genBinArgCompExpr2(" > ", args_syms[0], args_syms[1]));
            break;
        case Z3_OP_FPA_LE:
            if (expr_sym->isNegated())
                m_func_code.append
                        (genBinArgCompExpr2(" > ", args_syms[0], args_syms[1]));
            else
                m_func_code.append
                        (genBinArgCompExpr(" <= ", args_syms[0], args_syms[1]));
            break;
        case Z3_OP_FPA_GE:
            if (expr_sym->isNegated())
                m_func_code.append
                        (genBinArgCompExpr2(" < ", args_syms[0], args_syms[1]));
            else
                m_func_code.append
                        (genBinArgCompExpr(" >= ", args_syms[0], args_syms[1]));
            break;
        case Z3_OP_FPA_TO_FP:
            m_func_code.append(
                    std::string(args_syms[args_syms.size() - 1]->name()) +
                    ";\n");
            break;
        default:
            m_func_code.append(
                    "Unsupported expr:" + expr_sym->expr()->decl().name().str()
                    + "\n");
    }
}

unsigned long
FPExprCodeGenerator::getVarCount() const noexcept
{
    return m_var_sym_vec.size();
}

std::pair<const Symbol*, bool>
FPExprCodeGenerator::insertSymbol(const SymbolKind kind,
                                  const z3::expr expr) noexcept
{
    if (kind != SymbolKind::kNegatedExpr) {
        auto result = m_expr_sym_map.insert
                ({expr.hash(), Symbol(kind, expr)});
        return {&(*result.first).second, result.second};
    } else {
        // XXX: would this weaken hashing?
        auto result = m_expr_sym_map.insert
                ({expr.hash() + static_cast<unsigned>(kind),
                  Symbol(kind, expr)});
        return {&(*result.first).second, result.second};
    }
}

const char*
FPExprCodeGenerator::getSymbolName(const SymbolKind kind,
                                   const z3::expr& expr) const noexcept
{
    return (*m_expr_sym_map.find(
            expr.hash() + static_cast<unsigned>(kind))).second.name();
}

const Symbol*
FPExprCodeGenerator::findSymbol(const SymbolKind kind,
                                const z3::expr& expr) const noexcept
{
    auto result = m_expr_sym_map.find(
            expr.hash() + static_cast<unsigned>(kind));
    if (result != m_expr_sym_map.cend()) {
        return &(*result).second;
    }
    return nullptr;
}

std::string
FPExprCodeGenerator::getFuncNameFrom(const std::string& file_path)
{
    size_t start_idx = file_path.rfind('/');
    size_t end_idx = file_path.rfind('.');
    if (start_idx != std::string::npos) {
        auto res = file_path.substr(start_idx + 1, end_idx - start_idx - 1);
        std::replace(res.begin(), res.end(), '.', '_');
        return res;
    }
    return ("");
}

std::string
FPExprCodeGenerator::genFuncSignature(const std::string& func_name)
{
    std::string res;
    res = "double " + func_name + "(unsigned n, const double * "
          + CodeGenStr::kFunInput
          + ", double * grad, void * data)";
    return res;
}
}
