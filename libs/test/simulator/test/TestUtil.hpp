// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

/// Returns the location of the defs file, such that it is in the test data area
std::string findTestDataLocation(const std::string& defsFile);
