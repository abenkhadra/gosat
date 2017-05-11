//===------------------------------------------------------------*- C++ -*-===//
//
// This file is distributed under MIT License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright (c) 2017 University of Kaiserslautern.
//

#include "NLoptOptimizer.h"
#include <assert.h>
#include <cmath>
#include <vector>

namespace gosat {
OptConfig::OptConfig() :
    MaxEvalCount{500000},
    MaxLocalEvalCount{50000},
    RelTolerance{1e-10},
    Bound{1e9},
    StepSize{0.5},
    InitialPopulation{0} {}

OptConfig::OptConfig(nlopt_algorithm global_alg, nlopt_algorithm local_alg) :
    MaxEvalCount{500000},
    MaxLocalEvalCount{1000},
    RelTolerance{1e-10},
    Bound{1e9},
    StepSize{0.5},
    InitialPopulation{0} {
    assert(local_alg == NLOPT_LN_BOBYQA && "Invalid local optimization algorithms!");
    if (global_alg == NLOPT_G_MLSL_LDS || global_alg == NLOPT_G_MLSL) {
        MaxEvalCount = 50000;
        RelTolerance = 1e-8;
    }
}

NLoptOptimizer::NLoptOptimizer() :
    m_global_opt_alg{NLOPT_GN_DIRECT},
    m_local_opt_alg{NLOPT_LN_BOBYQA} {}

NLoptOptimizer::NLoptOptimizer(nlopt_algorithm global_alg,
                               nlopt_algorithm local_alg) :
    m_global_opt_alg{global_alg},
    m_local_opt_alg{local_alg},
    Config{global_alg, local_alg} {}

int
NLoptOptimizer::optimize
    (nlopt_func func, unsigned dim, double *x, double *min)
const noexcept {
    if (func(dim, x, nullptr, nullptr) == 0) {
        // trivially satisfiable algorithm
        *min = 0;
        return 0;
    }
    assert(NLoptOptimizer::isSupportedGlobalOptAlg(m_global_opt_alg)
               && "Unsupported global optimization algorithm");
    nlopt_opt opt;
    opt = nlopt_create(m_global_opt_alg, dim);
    nlopt_set_min_objective(opt, func, NULL);
    nlopt_set_upper_bounds1(opt, Config.Bound);
    nlopt_set_lower_bounds1(opt, -Config.Bound);
    std::vector<double> step_size_arr(dim, Config.StepSize);
    nlopt_set_initial_step(opt, step_size_arr.data());
    nlopt_set_stopval(opt, 0);
    nlopt_set_xtol_rel(opt, Config.RelTolerance);
    nlopt_set_maxeval(opt, Config.MaxEvalCount);
    if (NLoptOptimizer::isRequirePopulation(m_global_opt_alg)) {
        nlopt_set_population(opt, Config.InitialPopulation);
    }
    if (!NLoptOptimizer::isRequireLocalOptAlg(m_global_opt_alg)) {
        auto status = nlopt_optimize(opt, x, min);
        nlopt_destroy(opt);
        return status;
    }
    assert(NLoptOptimizer::isSupportedLocalOptAlg(m_local_opt_alg)
               && "Unsupported local optimization algorithm!");
    nlopt_opt local_opt;
    local_opt = nlopt_create(m_local_opt_alg, dim);
    nlopt_set_min_objective(local_opt, func, NULL);
    nlopt_set_initial_step(local_opt, step_size_arr.data());
    nlopt_set_stopval(local_opt, 0);
    nlopt_set_maxeval(local_opt, Config.MaxLocalEvalCount);
    nlopt_set_local_optimizer(opt, local_opt);
    auto status = nlopt_optimize(opt, x, min);
    nlopt_destroy(local_opt);
    nlopt_destroy(opt);
    return status;
}

bool
NLoptOptimizer::isSupportedLocalOptAlg(nlopt_algorithm local_opt_alg) noexcept {
    return (local_opt_alg == NLOPT_LN_BOBYQA
        || local_opt_alg == NLOPT_LN_SBPLX);
}

bool
NLoptOptimizer::isRequireLocalOptAlg(nlopt_algorithm opt_alg) noexcept {
    return opt_alg == NLOPT_G_MLSL || opt_alg == NLOPT_G_MLSL_LDS;
}

bool
NLoptOptimizer::isSupportedGlobalOptAlg(nlopt_algorithm opt_alg) noexcept {
    switch (opt_alg) {
        case NLOPT_GN_DIRECT:
        case NLOPT_GN_DIRECT_L:
        case NLOPT_GN_DIRECT_L_RAND:
        case NLOPT_GN_ORIG_DIRECT:
        case NLOPT_GN_ORIG_DIRECT_L:
        case NLOPT_GN_MLSL_LDS:
        case NLOPT_G_MLSL:
        case NLOPT_G_MLSL_LDS:
        case NLOPT_GN_CRS2_LM:
        case NLOPT_GN_ISRES:
        case NLOPT_GN_ESCH:
            return true;
        default:
            return false;
    }
}

bool
NLoptOptimizer::isRequirePopulation(nlopt_algorithm opt_alg) noexcept {
    switch (opt_alg) {
        case NLOPT_GN_MLSL:
        case NLOPT_GN_CRS2_LM:
        case NLOPT_GN_ISRES:
        case NLOPT_GN_ESCH:
            return true;
        default:
            return false;
    }
}

int
NLoptOptimizer::refineResult
    (nlopt_func func, unsigned dim, double *x, double *min) {
    nlopt_opt opt;
    opt = nlopt_create(NLOPT_LN_BOBYQA, dim);
    nlopt_set_min_objective(opt, func, NULL);
    nlopt_set_initial_step(opt, &Config.StepSize);
    nlopt_set_xtol_rel(opt, Config.RelTolerance);
    nlopt_set_maxeval(opt, Config.MaxLocalEvalCount);
    auto status = nlopt_optimize(opt, x, min);
    nlopt_destroy(opt);
    return status;
}

bool
NLoptOptimizer::existsRoundingError
    (nlopt_func func,
     unsigned int dim,
     const double *x,
     const double *min) const noexcept {
    return func(dim, x, nullptr, nullptr) != *min;
}

void
NLoptOptimizer::fixRoundingErrorNearZero(nlopt_func const func,
                                         unsigned dim,
                                         double *x,
                                         double *min) const noexcept {
    if (*min == 0 || std::fabs(*min) > 1e-6) {
        return;
    }
    for (int i = 0; i < dim; ++i) {
        double int_part;
        const double frac_part = std::modf(x[i], &int_part);
        if (std::fabs(x[i] - int_part) < 1e-6) {
            double temp = x[i];
            x[i] = int_part;
            const auto min_x = func(dim, x, nullptr, nullptr);
            if (*min < min_x || std::fpclassify(min_x) == FP_NAN) {
                x[i] = temp;
            }
        }
    }
    *min = func(dim, x, nullptr, nullptr);
}

double NLoptOptimizer::eval
    (nlopt_func func, unsigned dim, const double *x) const noexcept {
    return func(dim, x, nullptr, nullptr);
}
}
