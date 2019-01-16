//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VItem.hpp"
#include "VNode.hpp"

bool VItem::isAncestor(const VItem* n) const
{
    if(n == this)
        return true;

    VNode* nd=parent();
    while(nd)
    {
        if(const_cast<VItem*>(n) == nd)
            return true;

        nd=nd->parent();
    }
    return false;
}
