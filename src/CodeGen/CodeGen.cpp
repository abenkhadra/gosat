//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "CodeGen.h"

namespace gosat {

const std::string CodeGenStr::kFunName = "gofunc";
const std::string CodeGenStr::kFunInput = "x";
const std::string CodeGenStr::kFunDis = "fp64_dis";

Symbol::Symbol(SymbolKind kind, const z3::expr expr) :
        m_kind{kind},
        m_expr{expr},
        m_name{"expr_" + std::to_string(expr.hash())
               + (kind == SymbolKind::kNegatedExpr ? "n"
                                                   : "")}
{}

SymbolKind Symbol::kind() const noexcept
{
    return m_kind;
}

const z3::expr* Symbol::expr() const noexcept
{
    return &m_expr;
}

const char* Symbol::name() const noexcept
{
    return m_name.c_str();
}

bool Symbol::isNegated() const noexcept
{
    return m_kind == SymbolKind::kNegatedExpr;
}
}
