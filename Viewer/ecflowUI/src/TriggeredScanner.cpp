/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TriggeredScanner.hpp"

#include <QDebug>

#include "TriggerCollector.hpp"
#include "VNode.hpp"

void TriggeredScanner::clear() {
    total_   = 0;
    current_ = 0;
}

void TriggeredScanner::start(VServer* s) {
    clear();
    assert(s);
    total_   = s->totalNum();
    current_ = 0;
    Q_EMIT scanStarted();
    scan(s);
    Q_EMIT scanFinished();
    clear();
}

// Scan the the whole tree to find for each node all the nodes that it or its
// attributes trigger.
void TriggeredScanner::scan(VNode* n) {
    TriggeredCollector tc(n);
    n->triggers(&tc);

    updateProgress();

    for (int i = 0; i < n->numOfChildren(); i++) {
        scan(n->childAt(i));
    }
}

void TriggeredScanner::updateProgress() {
    current_++;
    if (current_ > 0 && current_ % batchSize_ == 0) {
        Q_EMIT scanProgressed(progress());
    }
}

int TriggeredScanner::progress() const {
    if (total_ > 0) {
        return static_cast<int>(100. * static_cast<float>(current_) / static_cast<float>(total_));
    }
    return 0;
}
