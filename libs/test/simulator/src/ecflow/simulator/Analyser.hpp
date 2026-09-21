// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

class Defs;

namespace ecf {

class Analyser {
public:
    Analyser();

    static void run(Defs& theDefs);
};

} // namespace ecf
