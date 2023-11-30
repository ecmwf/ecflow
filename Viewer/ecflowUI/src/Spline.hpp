/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_Spline_HPP
#define ecflow_viewer_Spline_HPP

#include <vector>

class Spline {
public:
    Spline(const std::vector<double>& x, const std::vector<double>& y);
    double eval(double x) const;
    bool status() const { return status_; }

private:
    bool tdma(const std::vector<double>& a,
              const std::vector<double>& b,
              const std::vector<double>& c,
              const std::vector<double>& d,
              std::vector<double>& X);

    std::vector<double> xp_;
    std::vector<double> yp_;
    // cubic polynomial coefficients
    std::vector<double> coeffA_, coeffB_, coeffC_;
    bool status_{false};
};

#endif /* ecflow_viewer_Spline_HPP */
