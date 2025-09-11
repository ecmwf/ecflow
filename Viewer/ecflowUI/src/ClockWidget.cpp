/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ClockWidget.hpp"

#include <string>
#include <vector>

#include <QTime>
#include <QTimer>

#include "PropertyMapper.hpp"
#include "VConfig.hpp"

ClockWidget::ClockWidget(QWidget* parent) : QLabel(parent) {
    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(slotTimeOut()));

    QString sh = "QLabel {color: rgb(34,107,138); margin-left: 5px; }";
    setStyleSheet(sh);

    // has to be hidden initially
    hide();

    std::vector<std::string> propVec;
    propVec.emplace_back("view.clock.showClock");
    propVec.emplace_back("view.clock.clockFormat");
    prop_ = new PropertyMapper(propVec, this);
    Q_ASSERT(prop_);
    prop_->initObserver(this);

    adjustTimer();
}

ClockWidget::~ClockWidget() {
    delete prop_;
}

void ClockWidget::renderTime() {
    if (isHidden()) {
        return;
    }

    QTime t = QTime::currentTime();
    if (showSec_) {
        setText("<b>" + t.toString(" HH:mm:ss ") + "</b>");
    }
    else {
        setText("<b>" + t.toString(" HH:mm ") + "</b>");
    }
}

void ClockWidget::slotTimeOut() {
    renderTime();
    if (!showSec_) {
        adjustTimer();
    }
}

void ClockWidget::adjustTimer() {
    if (isHidden()) {
        timer_->stop();
    }
    else {
        if (showSec_) {
            timer_->start(timeoutInMs_);
        }
        else {
            const int msInSec  = 1000;
            const int secInMin = 60;
            int sec            = QTime::currentTime().second();
            int interval       = (sec <= 1) ? (secInMin) : (secInMin - sec + 1);
            interval *= msInSec;
            if (timer_->interval() != interval || !timer_->isActive()) {
                timer_->start(interval);
            }
        }
    }
}

void ClockWidget::notifyChange(VProperty* p) {
    if (p->path() == "view.clock.showClock") {
        bool v = p->value().toBool();
        if (v != isVisible()) {
            setVisible(v);
            renderTime();
            adjustTimer();
        }
    }
    else if (p->path() == "view.clock.clockFormat") {
        QString v    = p->valueAsString();
        bool showSec = false;
        if (v == "hhmmss") {
            showSec = true;
        }

        if (showSec != showSec_) {
            showSec_ = showSec;
            renderTime();
            adjustTimer();
        }
    }
}
