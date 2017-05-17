//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include <string>
#include <fstream>

namespace gosat {
static std::string kDumpFileName = "gofuncs";

enum LibAPIGenMode {
    kUnsetAPI = 0,
    kPlainAPI,
    kCppAPI
};

class FPExprLibGenerator {
public:
    FPExprLibGenerator();

    virtual ~FPExprLibGenerator();

    FPExprLibGenerator(const FPExprLibGenerator&) = default;

    FPExprLibGenerator& operator=(const FPExprLibGenerator&) = default;

    FPExprLibGenerator& operator=(FPExprLibGenerator&&) = default;

    void init(LibAPIGenMode mode = LibAPIGenMode::kPlainAPI);

    void appendFunction
            (size_t args_count,
             const std::string& func_name,
             const std::string& func_sig,
             const std::string& func_def);

    bool dumpFilesExists();

private:
    LibAPIGenMode m_api_gen_mode;
    std::ofstream m_h_file;
    std::ofstream m_c_file;
    std::ofstream m_api_file;
};
}
