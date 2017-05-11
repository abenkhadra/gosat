//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "GOFuncsMap.h"
#include "Optimizer/NLoptOptimizer.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <chrono>

typedef std::numeric_limits<double> dbl;

inline float elapsedTimeFrom(std::chrono::steady_clock::time_point &st_time) {
    const auto res = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now() - st_time).count();
    return static_cast<float>(res) / 1000;
}

int main() {
    /*
     * The following global optimization algorithms seem to give the best
     * results. Note that all of them are derivative-free algorithms.
     *
     * - NLOPT_GN_DIRECT_L (provide similar performance to its variants)
     * - NLOPT_GN_CRS2_LM
     * - NLOPT_GN_ISRES
     * - NLOPT_G_MLSL
     *
     */
    gosat::NLoptOptimizer opt(NLOPT_GN_CRS2_LM);
    gosat::GOFuncsMap func_map;
    int status = 0;
    for (const auto &entry: func_map.getFuncMap()) {
        std::chrono::steady_clock::time_point
            begin = std::chrono::steady_clock::now();
        const auto func = entry.second.first;
        const auto dim = entry.second.second;
        std::vector<double> x(dim, 0.0);
        double minima = 1.0; /* minimum value */
        status = opt.optimize(func, dim, x.data(), &minima);
        if (status < 0) {
            std::cout << std::setprecision(4);
            std::cout << entry.first << ",error,"
                      << elapsedTimeFrom(begin) << ",INF,"<< status << std::endl;
        } else {
            std::string result = (minima == 0) ? "sat" : "unsat";
            std::cout << std::setprecision(4);
            std::cout << entry.first << "," << result << "," ;
            std::cout << elapsedTimeFrom(begin) << ",";
            std::cout << std::setprecision(dbl::digits10) << minima
                      << "," << status << std::endl;
        }
    }
    return 0;
}
