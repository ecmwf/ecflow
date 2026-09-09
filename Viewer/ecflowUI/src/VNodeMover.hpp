/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VNodeMover_HPP
#define ecflow_viewer_VNodeMover_HPP

#include "VInfo.hpp"

class VNodeMover {
public:
    static bool hasMarkedForMove();
    static void markNodeForMove(VInfo_ptr markedNode);
    static void moveMarkedNode(VInfo_ptr destNode);

protected:
    static VInfo_ptr nodeMarkedForMove_;
};

#endif /* ecflow_viewer_VNodeMover_HPP */
