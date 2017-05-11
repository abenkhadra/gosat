//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#pragma once

#include <nlopt.h>

namespace gosat {

class OptConfig {
public:
    OptConfig();
    OptConfig(nlopt_algorithm global_alg, nlopt_algorithm local_alg);
    virtual ~OptConfig() = default;
    int MaxEvalCount;
    int MaxLocalEvalCount;
    double RelTolerance;
    double Bound;
    double StepSize;
    unsigned InitialPopulation;
};

class NLoptOptimizer {
public:
    NLoptOptimizer();
    NLoptOptimizer(nlopt_algorithm global_alg,
                   nlopt_algorithm local_alg = NLOPT_LN_BOBYQA);
    virtual ~NLoptOptimizer() = default;
    int optimize
        (nlopt_func func, unsigned dim, double *x, double *min) const noexcept;
    double eval
        (nlopt_func func, unsigned dim, const double *x) const noexcept;
    bool existsRoundingError
        (nlopt_func func,
         unsigned int dim,
         const double *x,
         const double *min) const noexcept;
    void fixRoundingErrorNearZero
        (nlopt_func const func,
         unsigned dim,
         double *x,
         double *min) const noexcept;
    int refineResult(nlopt_func func, unsigned dim, double *x, double *min);
    static bool isSupportedGlobalOptAlg(nlopt_algorithm opt_alg) noexcept;
    static bool isSupportedLocalOptAlg(nlopt_algorithm local_opt_alg) noexcept;
    static bool isRequireLocalOptAlg(nlopt_algorithm opt_alg) noexcept;
    static bool isRequirePopulation(nlopt_algorithm opt_alg) noexcept;
private:
    const nlopt_algorithm m_global_opt_alg;
    const nlopt_algorithm m_local_opt_alg;
public:
    OptConfig Config;
};
}
