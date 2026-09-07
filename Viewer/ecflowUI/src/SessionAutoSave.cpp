/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "SessionAutoSave.hpp"

#include "MainWindow.hpp"
#include "UiLog.hpp"

//
// ScopedRestore
//

SessionAutoSave::ScopedRestore::ScopedRestore() {
    SessionAutoSave::instance()->beginRestore();
}

SessionAutoSave::ScopedRestore::~ScopedRestore() {
    SessionAutoSave::instance()->endRestore();
}

//
// SessionAutoSave
//

SessionAutoSave::SessionAutoSave()
    : QObject(nullptr) {
    idleTimer_.setSingleShot(true);
    idleTimer_.setInterval(idleIntervalMs);
    connect(&idleTimer_, &QTimer::timeout, this, &SessionAutoSave::flush);

    maxTimer_.setSingleShot(true);
    maxTimer_.setInterval(maxIntervalMs);
    connect(&maxTimer_, &QTimer::timeout, this, &SessionAutoSave::flush);
}

SessionAutoSave* SessionAutoSave::instance() {
    static SessionAutoSave inst;
    return &inst;
}

void SessionAutoSave::request() {
    if (isRestoring()) {
        return;
    }

    dirty_ = true;

    // The idle timer is restarted on every request.
    idleTimer_.start(idleIntervalMs);
    // The maximum-delay timer is armed by the first request only, so that continuous activity cannot
    // postpone the write indefinitely.
    if (!maxTimer_.isActive()) {
        maxTimer_.start();
    }
}

void SessionAutoSave::flush() {
    if (!dirty_ || isRestoring()) {
        return;
    }

    stopTimers();

    if (MainWindow::autoSave()) {
        dirty_ = false;
    }
    else {
        UiLog().warn() << "SessionAutoSave: session could not be saved, will retry";
        idleTimer_.start(retryIntervalMs);
    }
}

void SessionAutoSave::abandon() {
    stopTimers();
    dirty_ = false;
}

void SessionAutoSave::beginRestore() {
    restoreDepth_++;
}

void SessionAutoSave::endRestore() {
    Q_ASSERT(restoreDepth_ > 0);
    restoreDepth_--;
}

void SessionAutoSave::stopTimers() {
    idleTimer_.stop();
    maxTimer_.stop();
}
