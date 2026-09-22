// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

// ecFlow C++ bridge for Rust FFI — umbrella header pulled in by the
// cxx-generated bridge (`include!("EcflowBridge.h")` in lib.rs) and by
// downstream `-sys` crates. Real declarations live in the per-topic headers
// below.
#pragma once

#include "ClientWrapper.h"
