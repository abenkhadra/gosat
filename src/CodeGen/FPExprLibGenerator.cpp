//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "FPExprLibGenerator.h"

namespace gosat {

static std::string kHeaderFileName = kDumpFileName + ".h";
static std::string kCFileName = kDumpFileName + ".c";
static std::string kApiFileName = kDumpFileName + ".api";
static std::string kDistanceFunc =
        "double fp64_dis(const double a, const double b) {\n"
                "    if (a == b || isnan(a) || isnan(b)) {\n"
                "        return 0;\n"
                "    }\n"
                "    const double scale = pow(2, 54);\n"
                "    uint64_t a_uint = *(const uint64_t *)(&a);\n"
                "    uint64_t b_uint = *(const uint64_t *)(&b);\n"
                "    if ((a_uint & 0x8000000000000000) != (b_uint & 0x8000000000000000)) {\n"
                "        // signs are not equal return sum\n"
                "        return ((double)\n"
                "        ((a_uint & 0x7FFFFFFFFFFFFFFF) + (b_uint & 0x7FFFFFFFFFFFFFFF)))/scale;\n"
                "    }\n"
                "    b_uint &= 0x7FFFFFFFFFFFFFFF;\n"
                "    a_uint &= 0x7FFFFFFFFFFFFFFF;\n"
                "    if (a_uint < b_uint) {\n"
                "        return ((double)(b_uint - a_uint))/scale;\n"
                "    }\n"
                "    return ((double)(a_uint - b_uint))/scale;\n"
                "}\n\n";

FPExprLibGenerator::FPExprLibGenerator() :
        m_api_gen_mode{LibAPIGenMode::kPlainAPI}
{}

void
FPExprLibGenerator::init(LibAPIGenMode mode)
{
    m_api_gen_mode = mode;
    if (!dumpFilesExists()) {
        m_h_file.open(kHeaderFileName, std::ios::out | std::ios::trunc);
        m_h_file << "/* goSAT: automatically generated file*/\n\n"
                 << "#pragma once\n\n";
        m_h_file.close();

        m_c_file.open(kCFileName, std::ios::out | std::ios::trunc);
        m_c_file << "/* goSAT: automatically generated file */\n\n"
                 << "#include \"";
        m_c_file << kHeaderFileName << "\"\n"
                 << "#include <math.h>\n\n"
                 << "#include <stdint.h>\n\n"
                 << kDistanceFunc;
        m_c_file.close();

        m_api_file.open(kApiFileName, std::ios::out | std::ios::trunc);
        m_api_file.close();
    }
    m_h_file.open(kHeaderFileName, std::ios::app);
    m_c_file.open(kCFileName, std::ios::app);
    m_api_file.open(kApiFileName, std::ios::app);
}

void
FPExprLibGenerator::appendFunction
        (size_t args_count,
         const std::string& func_name,
         const std::string& func_sig,
         const std::string& func_def)
{

    m_h_file << func_sig << ";\n\n";

    m_c_file << func_def << "\n\n";

    if (m_api_gen_mode == LibAPIGenMode::kPlainAPI) {
        m_api_file << func_name << ","
                   << std::to_string(args_count) << "\n";
    } else {
        m_api_file << "{\"";
        m_api_file << func_name << "\", {"
                   << func_name << ", "
                   << std::to_string(args_count) << "}}, \n";
    }
}

bool
FPExprLibGenerator::dumpFilesExists()
{
    std::ifstream h_file(kHeaderFileName.c_str());
    if (!h_file.good())
        return false;
    std::ifstream c_file(kCFileName.c_str());
    if (!c_file.good())
        return false;
    std::ifstream api_file(kApiFileName.c_str());
    return api_file.good();
}

FPExprLibGenerator::~FPExprLibGenerator()
{
    m_h_file.close();
    m_c_file.close();
    m_api_file.close();
}
}
