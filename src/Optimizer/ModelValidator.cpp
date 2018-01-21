//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "ModelValidator.h"

namespace gosat {

ModelValidator::ModelValidator
        (const FPIRGenerator* ir_generator) :
        m_ir_gen{ir_generator}
{}

bool
ModelValidator::isValid(z3::expr smt_expr, const std::vector<double>& model)
{
    auto var_symbols = m_ir_gen->getVars();
    assert((var_symbols.size() == model.size()) && "Model size mismatch!");

    // now substituting fpa wrapped variables with consts from model
    z3::expr_vector src_1(smt_expr.ctx()), dst_1(smt_expr.ctx());
    for (const auto& symbol_pair:m_ir_gen->getVarsFPAWrapped()) {
        src_1.push_back(*symbol_pair.first->expr());
        // XXX: assuming TO_FPA should invert type casting
        auto kind = symbol_pair.second->kind() == SymbolKind::kFP32Var
                    ? SymbolKind::kFP64Var : SymbolKind::kFP32Var;
        dst_1.push_back(genFPConst(smt_expr, kind,
                                   symbol_pair.second->id(), model));
    }
    z3::expr expr_sub_1 = smt_expr.substitute(src_1, dst_1);

    // now substituting actual variables with consts from model
    z3::expr_vector src_2(smt_expr.ctx()), dst_2(smt_expr.ctx());
    for (const auto symbol:var_symbols) {
        src_2.push_back(*symbol->expr());
        dst_2.push_back(genFPConst(smt_expr, symbol->kind(),
                                   symbol->id(), model));
    }
    z3::expr expr_sub_2 = expr_sub_1.substitute(src_2, dst_2);
    z3::solver solver(smt_expr.ctx());
    solver.add(expr_sub_2);
    return solver.check() == z3::check_result::sat;
}

z3::expr
ModelValidator::genFPConst(z3::expr expr, SymbolKind kind, unsigned id,
                           const std::vector<double>& model) const noexcept
{
    if (kind == SymbolKind::kFP32Var) {
        auto var_ast =
                Z3_mk_fpa_numeral_float(expr.ctx(),
                                        static_cast<float>(model[id]),
                                        Z3_mk_fpa_sort_32(expr.ctx()));
        return z3::to_expr(expr.ctx(), var_ast);
    } else {
        assert(kind == SymbolKind::kFP64Var && "Bad variable kind!");
        //TODO: handle FP16 and FP128 constants
        auto var_ast =
                Z3_mk_fpa_numeral_double(expr.ctx(), model[id],
                                         Z3_mk_fpa_sort_64(expr.ctx()));
        return z3::to_expr(expr.ctx(), var_ast);
    }
}
}
