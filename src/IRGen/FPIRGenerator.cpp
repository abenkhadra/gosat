//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "FPIRGenerator.h"
#include "Utils/FPAUtils.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/ToolOutputFile.h>


namespace gosat {

FPIRGenerator::FPIRGenerator
        (llvm::LLVMContext* context, llvm::Module* module) :
        m_has_invalid_fp_const(false),
        m_found_unsupported_smt_expr(false),
        m_gofunc(nullptr),
        m_ctx(context),
        m_mod(module)
{}

const IRSymbol* FPIRGenerator::genNumeralIR
        (llvm::IRBuilder<>& builder, const z3::expr& expr) noexcept
{
    using namespace llvm;
    if (expr.get_sort().sort_kind() == Z3_FLOATING_POINT_SORT) {
        unsigned sigd = Z3_fpa_get_sbits(expr.ctx(), expr.get_sort());
        unsigned expo = Z3_fpa_get_ebits(expr.ctx(), expr.get_sort());
        if (fpa_util::isFloat32(expo, sigd)) {
            auto result_iter = findSymbol(SymbolKind::kFP32Const, &expr);
            if (result_iter != m_expr_sym_map.cend()) {
                return &(*result_iter).second;
            }
            // TODO: handling FP32 should be configurable
            float numeral = fpa_util::toFloat32(expr);
            Value* value = ConstantFP::get(builder.getDoubleTy(), numeral);
            auto res_pair = insertSymbol(SymbolKind::kFP32Const, expr, value, 0);
            return res_pair.first;
        } else {
            if (!fpa_util::isFloat64(expo, sigd)) {
                std::cerr << "unsupported\n";
                std::exit(5);
            }
            auto result_iter = findSymbol(SymbolKind::kFP64Const, &expr);
            if (result_iter != m_expr_sym_map.cend()) {
                return &(*result_iter).second;
            }
            double numeral = fpa_util::toFloat64(expr);
            Value* value = ConstantFP::get(builder.getDoubleTy(), numeral);
            auto res_pair = insertSymbol(SymbolKind::kFP64Const, expr, value, 0);
            return res_pair.first;
        }
    }
    if (expr.decl().decl_kind() == Z3_OP_BNUM) {
        auto result_iter = findSymbol(SymbolKind::kFP64Const, &expr);
        if (result_iter != m_expr_sym_map.cend()) {
            return &(*result_iter).second;
        }
        std::string numeral_str = Z3_ast_to_string(expr.ctx(),
                                                   static_cast<z3::ast>(expr));
        numeral_str.replace(0, 1, 1, '0');
        Value* value = ConstantFP::get(builder.getDoubleTy(),
                                       std::stod(numeral_str));
        auto res_pair = insertSymbol(SymbolKind::kFP64Const, expr, value, 0);
        return res_pair.first;
    }
    return nullptr;
}

llvm::Function* FPIRGenerator::getDistanceFunction() const noexcept
{
    return m_func_fp64_dis;
}

llvm::Function* FPIRGenerator::genFunction
        (const z3::expr& expr)  noexcept
{
    using namespace llvm;
    if (m_gofunc != nullptr) {
        return m_gofunc;
    }
    m_gofunc = cast<Function>(
            m_mod->getOrInsertFunction(StringRef(CodeGenStr::kFunName),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getInt32Ty(*m_ctx),
                                       Type::getDoublePtrTy(*m_ctx),
                                       Type::getDoublePtrTy((*m_ctx)),
                                       Type::getInt8PtrTy(*m_ctx),
                                       nullptr));
    Function::arg_iterator cur_arg = m_gofunc->arg_begin();
    (*cur_arg).setName("n");
    cur_arg++;
    (*cur_arg).setName("x");
    (*cur_arg).addAttr(Attribute::NoCapture);
    (*cur_arg).addAttr(Attribute::ReadOnly);
    cur_arg++;
    (*cur_arg).setName("grad");
    (*cur_arg).addAttr(Attribute::NoCapture);
    (*cur_arg).addAttr(Attribute::ReadNone);
    cur_arg++;
    (*cur_arg).setName("data");
    (*cur_arg).addAttr(Attribute::NoCapture);
    (*cur_arg).addAttr(Attribute::ReadNone);

    BasicBlock* BB = BasicBlock::Create(*m_ctx, "EntryBlock", m_gofunc);
    IRBuilder<> builder(BB);

    // TBAA Metadata
    MDBuilder md_builder(*m_ctx);
    auto md_scalar = md_builder.createConstant(builder.getInt64(0));
    auto md_node_1 = MDNode::get(*m_ctx,
                                 MDString::get(*m_ctx, "Simple C/C++ TBAA"));
    auto md_node_2 = MDNode::get(*m_ctx,
                                 {MDString::get(*m_ctx, "omnipotent char"),
                                  md_node_1, md_scalar});
    auto md_node_3 = MDNode::get(*m_ctx,
                                 {MDString::get(*m_ctx, "double"), md_node_2,
                                  md_scalar});
    m_tbaa_node = MDNode::get(*m_ctx, {md_node_3, md_node_3, md_scalar});

    // Initialize external functions to be linked to JIT
    m_func_fp64_dis = cast<Function>(
            m_mod->getOrInsertFunction(StringRef(CodeGenStr::kFunDis),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy((*m_ctx)),
                                       nullptr));
    m_func_fp64_eq_dis = cast<Function>(
            m_mod->getOrInsertFunction(StringRef(CodeGenStr::kFunEqDis),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy((*m_ctx)),
                                       nullptr));
    m_func_fp64_neq_dis = cast<Function>(
            m_mod->getOrInsertFunction(StringRef(CodeGenStr::kFunNEqDis),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy((*m_ctx)),
                                       nullptr));
    m_func_isnan = cast<Function>(
            m_mod->getOrInsertFunction(StringRef(CodeGenStr::kFunIsNan),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy(*m_ctx),
                                       Type::getDoubleTy((*m_ctx)),
                                       nullptr));

    m_func_fp64_dis->setLinkage(Function::ExternalLinkage);
    m_func_fp64_eq_dis->setLinkage(Function::ExternalLinkage);
    m_func_fp64_neq_dis->setLinkage(Function::ExternalLinkage);
    m_func_isnan->setLinkage(Function::ExternalLinkage);
    m_const_zero = ConstantFP::get(builder.getDoubleTy(), 0.0);
    m_const_one = ConstantFP::get(builder.getDoubleTy(), 1.0);
    auto return_val_sym = genFuncRecursive(builder, expr, false);
    builder.CreateRet(return_val_sym->getValue());
    return m_gofunc;
}

const IRSymbol* FPIRGenerator::genFuncRecursive
        (llvm::IRBuilder<>& builder, const z3::expr expr,
         bool is_negated) noexcept
{
    if (!expr.is_app()) {
        // is_app <==> Z3_NUMERAL_AST || Z3_APP_AST
        std::cerr << "unsupported\n";
        std::exit(4);
    }
    if (fpa_util::isRoundingModeApp(expr) &&
        expr.decl().decl_kind() != Z3_OP_FPA_RM_NEAREST_TIES_TO_EVEN) {
        m_found_unsupported_smt_expr = true;
    }
    if (expr.is_numeral()) {
        return genNumeralIR(builder, expr);
    }
    if (fpa_util::isFPVar(expr)) {
        // TODO: handle FP16 and FP128 variables
        SymbolKind kind;
        if (fpa_util::isFloat32VarDecl(expr)) {
            kind = SymbolKind::kFP32Var;
        } else if (fpa_util::isFloat64VarDecl(expr)) {
            kind = SymbolKind::kFP64Var;
        } else {
            // XXX: instead of failing directly we give it a try.
            // The result might still be useful
            m_found_unsupported_smt_expr = true;
        }
        auto result_iter = findSymbol(kind, &expr);
        if (result_iter != m_expr_sym_map.cend()) {
            return &(*result_iter).second;
        }
        using namespace llvm;
        Argument* arg2 = &(*(++m_gofunc->arg_begin()));
        auto idx_ptr = builder.CreateInBoundsGEP
                (llvm::cast<Value>(arg2),
                 builder.getInt64(getVarCount()));
        auto loaded_val = builder.CreateAlignedLoad(idx_ptr, 8);
        loaded_val->setMetadata(llvm::LLVMContext::MD_tbaa, m_tbaa_node);
        auto result_pair =
                insertSymbol(kind, expr, loaded_val, getVarCount());
        m_var_sym_vec.emplace_back(result_pair.first);
        return result_pair.first;
    }
    if (!fpa_util::isBoolExpr(expr)) {
        is_negated = false;
    } else if (expr.decl().decl_kind() == Z3_OP_NOT) {
        is_negated = !is_negated;
    }
    SymbolKind kind = (is_negated) ? SymbolKind::kNegatedExpr : SymbolKind::kExpr;
    if (is_negated &&
        expr.decl().decl_kind() != Z3_OP_NOT &&
        expr.decl().decl_kind() != Z3_OP_AND &&
        expr.decl().decl_kind() != Z3_OP_OR) {
        // propagate negation according to de-morgan's
        is_negated = false;
    }
    auto result_iter = findSymbol(kind, &expr);
    if (result_iter != m_expr_sym_map.cend()) {
        return &(*result_iter).second;
    }
    // Expr not visited before
    std::vector<const IRSymbol*> arg_syms;
    arg_syms.reserve(expr.num_args());
    for (uint i = 0; i < expr.num_args(); ++i) {
        arg_syms.push_back(genFuncRecursive(builder, expr.arg(i), is_negated));
    }
    auto res_pair = insertSymbol(kind, expr, nullptr);
    res_pair.first->setValue(genExprIR(builder, res_pair.first, arg_syms));
    if (expr.decl().decl_kind() == Z3_OP_FPA_TO_FP &&
        fpa_util::isFPVar(expr.arg(1))) {
        m_var_sym_fpa_vec.emplace_back(
                std::make_pair(res_pair.first, arg_syms[1]));
    }
    return res_pair.first;
}

llvm::Value* FPIRGenerator::genExprIR
        (llvm::IRBuilder<>& builder, const IRSymbol* expr_sym,
         std::vector<const IRSymbol*>& arg_syms) noexcept
{
    using namespace llvm;
    switch (expr_sym->expr()->decl().decl_kind()) {
        // Boolean operations
        case Z3_OP_TRUE:
            if (expr_sym->isNegated())
                return m_const_one;
            else
                return m_const_zero;
        case Z3_OP_FALSE:
            if (expr_sym->isNegated())
                return m_const_zero;
            else
                return m_const_one;
        case Z3_OP_EQ:
            if (expr_sym->isNegated()) {
                return builder.CreateCall(m_func_fp64_neq_dis,
                                          {arg_syms[0]->getValue(),
                                           arg_syms[1]->getValue()});
            } else {
                return builder.CreateCall(m_func_fp64_eq_dis,
                                          {arg_syms[0]->getValue(),
                                           arg_syms[1]->getValue()});
            }
        case Z3_OP_FPA_EQ:
            if (expr_sym->isNegated()) {
                auto result = builder.CreateFCmpONE(arg_syms[0]->getValue(),
                                                    arg_syms[1]->getValue());
                return builder.CreateSelect(result, m_const_zero, m_const_one);
            } else {
                return builder.CreateCall(m_func_fp64_dis, {arg_syms[0]->getValue(),
                                                            arg_syms[1]->getValue()});
            }
        case Z3_OP_NOT:
            // Do nothing, negation is handled with de-morgans
            return arg_syms[0]->getValue();
        case Z3_OP_AND:
            if (expr_sym->isNegated())
                return genMultiArgMulIR(builder, arg_syms);
            else
                return genMultiArgAddIR(builder, arg_syms);
        case Z3_OP_OR:
            if (expr_sym->isNegated())
                return genMultiArgAddIR(builder, arg_syms);
            else
                return genMultiArgMulIR(builder, arg_syms);
            // Floating point operations
        case Z3_OP_FPA_PLUS_INF:
            return ConstantFP::get(builder.getDoubleTy(), INFINITY);
        case Z3_OP_FPA_MINUS_INF:
            return ConstantFP::get(builder.getDoubleTy(), -INFINITY);
        case Z3_OP_FPA_NAN:
            return ConstantFP::get(builder.getDoubleTy(), NAN);
        case Z3_OP_FPA_PLUS_ZERO:
            return ConstantFP::get(builder.getDoubleTy(), 0.0);
        case Z3_OP_FPA_MINUS_ZERO:
            return ConstantFP::get(builder.getDoubleTy(), -0.0);
        case Z3_OP_FPA_ADD:
            return builder.CreateFAdd(arg_syms[1]->getValue(),
                                      arg_syms[2]->getValue());
        case Z3_OP_FPA_SUB:
            return builder.CreateFSub(arg_syms[1]->getValue(),
                                      arg_syms[2]->getValue());
        case Z3_OP_FPA_NEG:
            return builder.CreateFSub(
                    ConstantFP::get(builder.getDoubleTy(), -0.0),
                    arg_syms[0]->getValue());
        case Z3_OP_FPA_MUL:
            return builder.CreateFMul(arg_syms[1]->getValue(),
                                      arg_syms[2]->getValue());
        case Z3_OP_FPA_DIV:
            return builder.CreateFDiv(arg_syms[1]->getValue(),
                                      arg_syms[2]->getValue());
        case Z3_OP_FPA_REM:
            return builder.CreateFRem(arg_syms[1]->getValue(),
                                      arg_syms[2]->getValue());
        case Z3_OP_FPA_ABS: {
            auto fabs_func = Intrinsic::getDeclaration(m_mod, Intrinsic::fabs);
            return builder.CreateCall(fabs_func, arg_syms[0]->getValue());
        }
        case Z3_OP_FPA_LT:
            if (expr_sym->isNegated()) {
                auto comp_res = builder.CreateFCmpOGE(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR(builder, arg_syms, comp_res);
            } else {
                auto comp_res = builder.CreateFCmpOLT(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR2(builder, arg_syms, comp_res);
            }
        case Z3_OP_FPA_GT:
            if (expr_sym->isNegated()) {
                auto comp_res = builder.CreateFCmpOLE(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR(builder, arg_syms, comp_res);
            } else {
                auto comp_res = builder.CreateFCmpOGT(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR2(builder, arg_syms, comp_res);
            }
        case Z3_OP_FPA_LE:
            if (expr_sym->isNegated()) {
                auto comp_res = builder.CreateFCmpOGT(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR2(builder, arg_syms, comp_res);
            } else {
                auto comp_res = builder.CreateFCmpOLE(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR(builder, arg_syms, comp_res);
            }
        case Z3_OP_FPA_GE:
            if (expr_sym->isNegated()) {
                auto comp_res = builder.CreateFCmpOLT(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR2(builder, arg_syms, comp_res);
            } else {
                auto comp_res = builder.CreateFCmpOGE(arg_syms[0]->getValue(),
                                                      arg_syms[1]->getValue());
                return genBinArgCmpIR(builder, arg_syms, comp_res);
            }
        case Z3_OP_FPA_TO_FP:
            return (arg_syms[arg_syms.size() - 1])->getValue();
        case Z3_OP_FPA_IS_NAN:
            if (expr_sym->isNegated()) {
                auto call_res = builder.CreateCall(m_func_isnan, {arg_syms[0]->getValue(), m_const_one});
                call_res->setTailCall(false);
                return call_res;
            } else {
                auto call_res = builder.CreateCall(m_func_isnan, {arg_syms[0]->getValue(), m_const_zero});
                call_res->setTailCall(false);
                return call_res;
            }
        default:
            std::cerr << "unsupported: " +
                         expr_sym->expr()->decl().name().str() + "\n";
            std::exit(3);
    }
}

llvm::Value* FPIRGenerator::genBinArgCmpIR
        (llvm::IRBuilder<>& builder, std::vector<const IRSymbol*>& arg_syms,
         llvm::Value* comp_result) noexcept
{
    using namespace llvm;
    BasicBlock* bb_first = BasicBlock::Create(*m_ctx, "", m_gofunc);
    BasicBlock* bb_second = BasicBlock::Create(*m_ctx, "", m_gofunc);
    BasicBlock* bb_cur = builder.GetInsertBlock();
    builder.CreateCondBr(comp_result, bb_second, bb_first);
    builder.SetInsertPoint(bb_first);
    auto call_res = builder.CreateCall(m_func_fp64_dis, {arg_syms[0]->getValue(),
                                                    arg_syms[1]->getValue()});
    call_res->setTailCall(false);
    builder.CreateBr(bb_second);
    builder.SetInsertPoint(bb_second);
    auto phi_inst = builder.CreatePHI(builder.getDoubleTy(), 2);
    phi_inst->addIncoming(call_res, bb_first);
    phi_inst->addIncoming(m_const_zero, bb_cur);
    return phi_inst;
}

llvm::Value* FPIRGenerator::genBinArgCmpIR2
        (llvm::IRBuilder<>& builder, std::vector<const IRSymbol*>& arg_syms,
         llvm::Value* comp_result) noexcept
{
    using namespace llvm;
    BasicBlock* bb_first = BasicBlock::Create(*m_ctx, "", m_gofunc);
    BasicBlock* bb_second = BasicBlock::Create(*m_ctx, "", m_gofunc);
    BasicBlock* bb_cur = builder.GetInsertBlock();
    builder.CreateCondBr(comp_result, bb_second, bb_first);
    builder.SetInsertPoint(bb_first);
    auto call_res = builder.CreateCall(m_func_fp64_dis, {arg_syms[0]->getValue(),
                                                    arg_syms[1]->getValue()});
    call_res->setTailCall(false);
    auto dis_res = builder.CreateFAdd(call_res, m_const_one);
    builder.CreateBr(bb_second);
    builder.SetInsertPoint(bb_second);
    auto phi_inst = builder.CreatePHI(builder.getDoubleTy(), 2);
    phi_inst->addIncoming(dis_res, bb_first);
    phi_inst->addIncoming(m_const_zero, bb_cur);
    return phi_inst;
}

llvm::Value* FPIRGenerator::genMultiArgAddIR
        (llvm::IRBuilder<>& builder,
         std::vector<const IRSymbol*>& arg_syms) noexcept
{
    auto result = builder.CreateFAdd(arg_syms[0]->getValue(),
                                     arg_syms[1]->getValue());
    for (unsigned i = 2; i < arg_syms.size(); ++i) {
        result = builder.CreateFAdd(result, arg_syms[i]->getValue());
    }
    return result;
}

llvm::Value* FPIRGenerator::genMultiArgMulIR
        (llvm::IRBuilder<>& builder,
         std::vector<const IRSymbol*>& arg_syms) noexcept
{
    auto result = builder.CreateFMul(arg_syms[0]->getValue(),
                                     arg_syms[1]->getValue());
    for (unsigned i = 2; i < arg_syms.size(); ++i) {
        result = builder.CreateFMul(result, arg_syms[i]->getValue());
    }
    return result;
}

unsigned FPIRGenerator::getVarCount() const noexcept
{
    return static_cast<unsigned>(m_var_sym_vec.size());
}

const std::vector<IRSymbol*>&
FPIRGenerator::getVars() const noexcept
{
    return m_var_sym_vec;
}

const std::vector<std::pair<IRSymbol*, const IRSymbol*>>&
FPIRGenerator::getVarsFPAWrapped() const noexcept
{
    return m_var_sym_fpa_vec;
}

std::pair<IRSymbol*, bool> FPIRGenerator::insertSymbol
        (const SymbolKind kind, const z3::expr expr, llvm::Value* value,
         unsigned id) noexcept
{
    if (kind != SymbolKind::kNegatedExpr) {
        auto result = m_expr_sym_map.insert
                ({expr.hash(), IRSymbol(kind, expr, value, id)});
        return {&(*result.first).second, result.second};
    } else {
        // XXX: would this weaken hashing?
        auto result = m_expr_sym_map.insert
                ({expr.hash() + static_cast<unsigned>(kind),
                  IRSymbol(kind, expr, value, id)});
        return {&(*result.first).second, result.second};
    }
}

SymMapType::const_iterator FPIRGenerator::findSymbol
        (const SymbolKind kind, const z3::expr* expr) const noexcept
{
    if (kind != SymbolKind::kNegatedExpr) {
        return m_expr_sym_map.find(expr->hash());
    } else {
        // XXX: would this weaken hashing?
        return m_expr_sym_map.find(expr->hash() + static_cast<unsigned>(kind));
    }
}

void FPIRGenerator::addGlobalFunctionMappings(llvm::ExecutionEngine *engine)
{
    double (*func_ptr_dis)(double, double) = fp64_dis;
    double (*func_ptr_eq_dis)(double, double) = fp64_eq_dis;
    double (*func_ptr_neq_dis)(double, double) = fp64_neq_dis;
    double (*func_ptr_isnan)(double, double) = fp64_isnan;
    engine->addGlobalMapping(this->m_func_fp64_dis, (void *)func_ptr_dis);
    engine->addGlobalMapping(this->m_func_fp64_eq_dis, (void *)func_ptr_eq_dis);
    engine->addGlobalMapping(this->m_func_fp64_neq_dis, (void *)func_ptr_neq_dis);
    engine->addGlobalMapping(this->m_func_isnan, (void *)func_ptr_isnan);
}

bool FPIRGenerator::isFoundUnsupportedSMTExpr() noexcept
{
    return m_found_unsupported_smt_expr;
}
}
