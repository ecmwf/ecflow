/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ServerSettingsItemWidget.hpp"

#include <cassert>

#include <QPushButton>

#include "ServerHandler.hpp"
#include "VNode.hpp"
#include "ecflow/node/Node.hpp"

//========================================================
//
// ServerSettingsItemWidget
//
//========================================================

ServerSettingsItemWidget::ServerSettingsItemWidget(QWidget* parent)
    : QWidget(parent) {
    setupUi(this);

    connect(buttonBox_, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotClicked(QAbstractButton*)));

    QPushButton* applyPb = buttonBox_->button(QDialogButtonBox::Apply);
    assert(applyPb);
    applyPb->setEnabled(false);

    connect(editor_, SIGNAL(changed()), this, SLOT(slotEditorChanged()));

    // This tab is always visible whatever node is selected!!!
    // We keep the data unchanged unless a new server is selected
    keepServerDataOnLoad_ = true;
}

QWidget* ServerSettingsItemWidget::realWidget() {
    return this;
}

void ServerSettingsItemWidget::reload(VInfo_ptr info) {
    assert(active_);

    clearContents();

    if (info && info->server() && info->server()->isDisabled()) {
        editor_->empty();
        setEnabled(false);
        return;
    }
    else {
        setEnabled(true);
    }

    info_ = info;

    if (info_ && info_->server()) {
        editor_->edit(info_->server()->conf()->guiProp(), QString::fromStdString(info_->server()->name()));
    }
    else {}
}

void ServerSettingsItemWidget::clearContents() {
    InfoPanelItem::clear();
    // TODO: properly set gui state
}

void ServerSettingsItemWidget::updateState(const FlagSet<ChangeFlag>& flags) {
    if (flags.isSet(ActiveChanged)) {
        if (active_) {
            editor_->setEnabled(true);
            buttonBox_->setEnabled(true);
        }
    }

    if (flags.isSet(SuspendedChanged)) {
        if (active_) {
            if (suspended_) {
                editor_->setEnabled(false);
                buttonBox_->setEnabled(false);
            }
            else {
                editor_->setEnabled(true);
                buttonBox_->setEnabled(true);
            }
        }
    }
}

void ServerSettingsItemWidget::slotEditorChanged() {
    QPushButton* applyPb = buttonBox_->button(QDialogButtonBox::Apply);
    assert(applyPb);
    applyPb->setEnabled(true);
}

void ServerSettingsItemWidget::slotClicked(QAbstractButton* button) {
    if (!active_) {
        return;
    }

    switch (buttonBox_->standardButton(button)) {
        case QDialogButtonBox::Apply: {
            if (editor_->applyChange()) {
                if (info_ && info_->server()) {
                    info_->server()->conf()->saveSettings();
                }
            }
        } break;
        default:
            break;
    }
}

static InfoPanelItemMaker<ServerSettingsItemWidget> maker1("server_settings");
