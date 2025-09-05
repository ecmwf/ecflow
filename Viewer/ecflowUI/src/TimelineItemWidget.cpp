/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TimelineItemWidget.hpp"

#include <QVBoxLayout>

#include "InfoProvider.hpp"
#include "MessageLabel.hpp"
#include "ServerHandler.hpp"
#include "SuiteFilter.hpp"
#include "TimelineData.hpp"
#include "TimelineWidget.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"
#include "ecflow/node/Node.hpp"

TimelineItemWidget::TimelineItemWidget(QWidget* /*parent*/) : delayedLoad_(false) {
    auto* vb = new QVBoxLayout(this);
    vb->setContentsMargins(0, 0, 0, 0);

    w_ = new TimelineWidget(this);
    vb->addWidget(w_);

    // This tab is always visible whatever node is selected!!!
    // We keep the data unchanged unless a new server is selected
    keepServerDataOnLoad_ = true;

    // We will KEEP the contents when the item (aka tab) becomes unselected
    // unselectedFlags_.clear();
}

TimelineItemWidget::~TimelineItemWidget() = default;

QWidget* TimelineItemWidget::realWidget() {
    return this;
}

void TimelineItemWidget::reload(VInfo_ptr info) {
    assert(active_);

    if (suspended_) {
        return;
    }

    clearContents();

    if (info && info->server() && info->server()->isDisabled()) {
        setEnabled(false);
        return;
    }
    else {
        setEnabled(true);
    }

    bool same = hasSameContents(info);

    // set the info. We do not need to observe the node!!!
    info_ = info;

    if (!same) {
        load();
    }
    else if (info_) {
        w_->selectPathInView(info_->nodePath());
    }
}

void TimelineItemWidget::load() {
    if (info_ && info_->server()) {
        ServerHandler* sh = info_->server();
        Q_ASSERT(sh);

        if (sh->activity() == ServerHandler::LoadActivity) {
            delayedLoad_ = true;
        }
        else {
            if (VServer* vs = sh->vRoot()) {
                QString logFile = QString::fromStdString(vs->findVariable("ECF_LOG", false));
                if (!logFile.isEmpty()) {
                    std::vector<std::string> suites;
                    if (SuiteFilter* sf = sh->suiteFilter()) {
                        if (sf->isEnabled()) {
                            suites = sh->suiteFilter()->filter();
                        }
                    }

                    // last 100 MB are read
                    w_->initLoad(QString::fromStdString(sh->name()),
                                 QString::fromStdString(sh->host()),
                                 QString::fromStdString(sh->port()),
                                 logFile,
                                 suites,
                                 sh->uidForServerLogTransfer(),
                                 sh->maxSizeForTimelineData(),
                                 info_->nodePath(),
                                 detached_); // last 100 MB are read

                    delayedLoad_ = false;
                }
            }
        }
    }

    if (info_) {
        w_->selectPathInView(info_->nodePath());
    }
}

void TimelineItemWidget::clearContents() {
    w_->clear();
    delayedLoad_ = false;
    InfoPanelItem::clear();
}

bool TimelineItemWidget::hasSameContents(VInfo_ptr info) {
    if (info && info_ && info->server()) {
        return info->server() == info_->server();
    }
    return false;
}

void TimelineItemWidget::notifyInfoChanged(const std::string& path) {
    if (info_) {
        w_->selectPathInView(path);
    }
}

void TimelineItemWidget::serverSyncFinished() {
    if (delayedLoad_) {
        load();
    }
}

void TimelineItemWidget::connectStateChanged() {
    if (frozen_) {
        return;
    }

    if (delayedLoad_) {
        load();
    }
}

void TimelineItemWidget::updateState(const FlagSet<ChangeFlag>& flags) {
    if (flags.isSet(SuspendedChanged)) {
        // Suspend
        if (suspended_) {
            // reloadTb_->setEnabled(false);
        }
        // Resume
        else {
            if (info_ && info_->node()) {
                // reloadTb_->setEnabled(true);
                if (delayedLoad_) {
                    load();
                }
            }
            else {
                clearContents();
            }
        }
    }

    if (flags.isSet(DetachedChanged)) {
        w_->setDetached(detached_);
        if (!detached_ && info_) {
            w_->selectPathInView(info_->nodePath());
        }
#if 0
        // we just returned from detached state
        //if the server in the widget does not match the current one we need to reload
        if(!detached_ && info_ && info_->server() &&
           !w_->isSameServer(info_->server()->host(),info_->server()->port()))
        {
            load();
        }
        else
        {
            w_->setDetached(detached_);
            if(!detached_ && info_)
                w_->selectPathInView(info_->nodePath());
        }
#endif
    }
}

void TimelineItemWidget::writeSettings(VComboSettings* vs) {
    vs->beginGroup("timeline");
    w_->writeSettings(vs);
    vs->endGroup();
}

void TimelineItemWidget::readSettings(VComboSettings* vs) {
    vs->beginGroup("timeline");
    w_->readSettings(vs);
    vs->endGroup();
}

static InfoPanelItemMaker<TimelineItemWidget> maker1("timeline");
