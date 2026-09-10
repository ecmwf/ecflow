/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/WhyCmd.hpp"

#include <stdexcept>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

WhyCmd::WhyCmd(defs_ptr defs, const std::string& absNodePath)
    : defs_(defs) {
    if (!defs_.get()) {
        throw std::runtime_error("WhyCmd: The definition parameter is empty");
    }

    if (!absNodePath.empty()) {
        node_ = defs_->findAbsNode(absNodePath);
        if (!node_.get()) {
            std::string errorMsg = "WhyCmd: The node path parameter '";
            errorMsg += absNodePath;
            errorMsg += "' cannot be found.";
            throw std::runtime_error(errorMsg);
        }
    }
}

std::string WhyCmd::why() const {
    std::vector<std::string> theReasonWhy;
    if (node_.get()) {
        node_->bottom_up_why(theReasonWhy);
    }
    else {
        defs_->top_down_why(theReasonWhy);
    }

    // Do not add /n on very last item
    std::string reason;
    for (size_t i = 0; i < theReasonWhy.size(); ++i) {
        reason += theReasonWhy[i];
        if (i != theReasonWhy.size() - 1) {
            reason += "\n";
        }
    }
    return reason;
}
