/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Spline.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>

Spline::Spline(const std::vector<double>& x, const std::vector<double>& y) : xp_(x), yp_(y) {
    assert(xp_.size() == yp_.size());
    assert(xp_.size() >= 2); // we need at least 3 points for a cubic spline
    size_t n = x.size();

    // points must be sorted by xp!!!

    // creates the tridiagonal matrix to be solved
    std::vector<double> lower(n, 0.);
    std::vector<double> diagonal(n, 2.);
    std::vector<double> upper(n, 0.);
    std::vector<double> rhs(n, 0.); // right hand side - vector

    std::vector<double> h(n);
    h[0] = 0.;
    for (size_t i = 1; i < n; i++) {
        h[i] = xp_[i] - xp_[i - 1];
    }

    lower[0]        = 0.;
    lower[n - 1]    = 0.;
    upper[0]        = 0.;
    upper[n - 1]    = 0.;
    diagonal[0]     = 2.;
    diagonal[n - 1] = 2.;

    for (size_t i = 1; i < n - 1; i++) {
        lower[i]    = h[i] / 3.;
        upper[i]    = h[i + 1] / 3.;
        diagonal[i] = 2.0 * (h[i + 1] + h[i]) / 3.0;
        rhs[i]      = ((yp_[i + 1] - yp_[i]) / h[i + 1] - (yp_[i] - yp_[i - 1]) / h[i]);
    }

    // compute the spline coefficients
    coeffA_ = std::vector<double>(n);
    coeffB_ = std::vector<double>(n);
    coeffC_ = std::vector<double>(n);
    if (tdma(lower, diagonal, upper, rhs, coeffB_)) {
        status_ = true;
        for (size_t i = 0; i < n - 1; i++) {
            coeffA_[i] = (coeffB_[i + 1] - coeffB_[i]) / (h[i + 1] * 3.);
            coeffC_[i] = (yp_[i + 1] - yp_[i]) / h[i + 1] - (2.0 * coeffB_[i] + coeffB_[i + 1]) * h[i + 1] / 3.;
        }

        double vh      = h[n - 1];
        coeffA_[n - 1] = 0.0;
        coeffC_[n - 1] = 3.0 * coeffA_[n - 2] * vh * vh + 2.0 * coeffB_[n - 2] * vh + coeffC_[n - 2];
    }
}

double Spline::eval(double x) const {
    if (!status_) {
        return 0.;
    }

    int idx            = -1;
    const double delta = 1E-5;
    if (fabs(x - xp_[0]) < delta) {
        idx = 0;
    }
    else if (fabs(x - xp_.back()) < delta) {
        idx = xp_.size() - 1;
    }
    else {
        for (size_t i = 0; i < xp_.size() - 1; i++) {
            if (x >= xp_[i] && x < xp_[i + 1]) {
                idx = i;
                break;
            }
        }
    }

    // we use the Horner method to evaluate the polynom
    if (idx >= 0) {
        double t = x - xp_[idx];
        return ((coeffA_[idx] * t + coeffB_[idx]) * t + coeffC_[idx]) * t + yp_[idx];
    }

    return 0.;
}

// Code taken from: http://www.cplusplus.com/forum/beginner/247330/
//*********************************************************************************
// Solve, using the Thomas Algorithm (TDMA), the tri-diagonal system              *
//     a_i X_i-1 + b_i X_i + c_i X_i+1 = d_i,     i = 0, n - 1                    *
//                                                                                *
// Effectively, this is the n x n matrix equation.                                *
// a[i], b[i], c[i] are the non-zero diagonals of the matrix and d[i] is the rhs. *
// a[0] and c[n-1] aren't used.                                                   *
//*********************************************************************************
bool Spline::tdma(const std::vector<double>& a,
                  const std::vector<double>& b,
                  const std::vector<double>& c,
                  const std::vector<double>& d,
                  std::vector<double>& X) {
    const double SMALL = 1.0E-30; // used to stop divide-by-zero

    int n = d.size();
    std::vector<double> P(n, 0);
    std::vector<double> Q(n, 0);
    X = P;

    // Forward pass
    int i              = 0;
    double denominator = b[i];
    P[i]               = -c[i] / denominator;
    Q[i]               = d[i] / denominator;
    for (i = 1; i < n; i++) {
        denominator = b[i] + a[i] * P[i - 1];
        if (fabs(denominator) < SMALL)
            return false;
        P[i] = -c[i] / denominator;
        Q[i] = (d[i] - a[i] * Q[i - 1]) / denominator;
    }

    // Backward pass
    X[n - 1] = Q[n - 1];
    for (i = n - 2; i >= 0; i--)
        X[i] = P[i] * X[i + 1] + Q[i];

    return true;
}
