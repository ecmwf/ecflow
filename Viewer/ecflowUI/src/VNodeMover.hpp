// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VInfo.hpp"

class VNodeMover {
public:
    static bool hasMarkedForMove();
    static void markNodeForMove(VInfo_ptr markedNode);
    static void moveMarkedNode(VInfo_ptr destNode);

protected:
    static VInfo_ptr nodeMarkedForMove_;
};
