//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VNODEMOVER_HPP
#define VNODEMOVER_HPP

#include "VInfo.hpp"

class VNodeMover
{
public:
    static bool hasMarkedForMove();
    static void markNodeForMove(VInfo_ptr markedNode);
    static void moveMarkedNode(VInfo_ptr destNode);

protected:
    static VInfo_ptr nodeMarkedForMove_;
};

#endif // VNODEMOVER_HPP
