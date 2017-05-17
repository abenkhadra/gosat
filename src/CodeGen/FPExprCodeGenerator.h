//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include "CodeGen.h"
#include <unordered_map>
#include <vector>

namespace gosat {

/**
 * /brief C++ code generator based on GO transformation
 */
class FPExprCodeGenerator {
public:
    FPExprCodeGenerator();

    virtual ~FPExprCodeGenerator() = default;

    FPExprCodeGenerator(const FPExprCodeGenerator&) = default;

    FPExprCodeGenerator& operator=(const FPExprCodeGenerator&) = default;

    FPExprCodeGenerator& operator=(FPExprCodeGenerator&&) = default;

    bool hasRNERoundingMode(const z3::expr& expr) const noexcept;

    void
    genFuncCode(const std::string& func_name, const z3::expr& expr) noexcept;

    const std::string& getFuncCode() const noexcept;

    unsigned long getVarCount() const noexcept;

    static std::string getFuncNameFrom(const std::string& file_path);

    static std::string genFuncSignature(const std::string& func_name);

private:
    const Symbol*
    genFuncCodeRecursive(const z3::expr& expr, bool is_negated) noexcept;

    void genExprCode(const Symbol* expr_sym,
                     std::vector<const Symbol*>& args_syms) noexcept;

    void includeFuncHeader(const std::string& func_name) noexcept;

    void includeFuncFooter() noexcept;

    std::pair<const Symbol*, bool>
    insertSymbol(const SymbolKind kind, const z3::expr expr) noexcept;

    const char*
    getSymbolName(const SymbolKind kind, const z3::expr& expr) const noexcept;

    const Symbol*
    findSymbol(const SymbolKind kind, const z3::expr& expr) const noexcept;

    const Symbol* genNumeralCode(const z3::expr& expr);

private:
    bool m_has_invalid_fp_const;
    bool m_has_unsupported_expr;
    std::vector<const Symbol*> m_var_sym_vec;
    std::unordered_map<unsigned int, Symbol> m_expr_sym_map;
    std::string m_func_code;
};
}
