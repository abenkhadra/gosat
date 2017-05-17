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

namespace gosat {
/**
 * /brief String constants used for code generation
 */
class CodeGenStr {
public:
    static const std::string kFunName;
    static const std::string kFunInput;
    static const std::string kFunDis;
};

enum class SymbolKind : unsigned {
    kExpr = 0,
    kNegatedExpr = 1,
    kFP32Const = 2,
    kFP64Const = 4,
    kFP32Var = 8,
    kFP64Var = 16,
    kUnknown = 32
};

enum class FPVarKind : unsigned {
    kUnknown,
    kFP32,
    kFP64,
};

class Symbol {
public:
    Symbol() = delete;

    virtual ~Symbol() = default;

    Symbol(const Symbol&) = default;

    Symbol& operator=(const Symbol&) = default;

    Symbol& operator=(Symbol&&) = default;

    explicit Symbol(SymbolKind kind, const z3::expr expr);

    SymbolKind kind() const noexcept;

    const z3::expr* expr() const noexcept;

    const char* name() const noexcept;

    bool isNegated() const noexcept;

private:
    const SymbolKind m_kind;
    const z3::expr m_expr;
    const std::string m_name;
};
}
