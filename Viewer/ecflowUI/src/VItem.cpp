/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VItem.hpp"

#include "VNode.hpp"

bool VItem::isAncestor(const VItem* n) const {
    if (n == this) {
        return true;
    }

    VNode* nd = parent();
    while (nd) {
        if (const_cast<VItem*>(n) == nd) {
            return true;
        }

        nd = nd->parent();
    }
    return false;
}
