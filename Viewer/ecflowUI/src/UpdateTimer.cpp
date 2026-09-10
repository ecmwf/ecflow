/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "UpdateTimer.hpp"

void UpdateTimer::drift(int dValSec, int maxValMin) {
    double v = interval() + dValSec * 1000;
    if (v > maxValMin * 1000 * 60) {
        v = maxValMin * 1000 * 60;
    }

    setInterval(v);
}
