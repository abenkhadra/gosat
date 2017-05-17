//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include "IRGen/FPIRGenerator.h"
#include "z3++.h"

namespace gosat {
class IRSymbol;

class FPIRGenerator;

class ModelValidator {
public:
    ModelValidator(const FPIRGenerator* ir_generator);

    virtual ~ModelValidator() = default;

    ModelValidator(const ModelValidator&) = default;

    ModelValidator& operator=(const ModelValidator&) = default;

    ModelValidator& operator=(ModelValidator&&) = default;

    bool isValid(z3::expr smt_expr, const std::vector<double>& model);

private:
    z3::expr genFPConst(z3::expr expr, SymbolKind kind, unsigned id,
                        const std::vector<double>& model) const noexcept;

private:
    const FPIRGenerator* m_ir_gen;
};
}
