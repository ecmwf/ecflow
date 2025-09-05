/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "AttributeEditor.hpp"

#include <QPushButton>

#include "AttributeEditorFactory.hpp"
#include "ConnectState.hpp"
#include "ServerHandler.hpp"
#include "UiLogS.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VConfig.hpp"
#include "VRepeatAttr.hpp"

#define _USE_MODELESS_ATTRIBUTEDITOR
#define _UI_ATTRIBUTEDITOR_DEBUG

#ifdef _USE_MODELESS_ATTRIBUTEDITOR
static QList<AttributeEditor*> editors;
#endif

AttributeEditor::AttributeEditor(VInfo_ptr info, QString type, QWidget* parent)
    : QDialog(parent),
      info_(info),
      form_(nullptr),
      type_(type) {
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

#ifdef _USE_MODELESS_ATTRIBUTEDITOR
    setModal(false);
#endif

    Q_ASSERT(info_);
    auto attr = info_->attribute();
    Q_ASSERT(attr);
    messageLabel_->hide();

    attrData_ = attr->data();

    QString wt = "Edit " + type;
    wt += "  -  " + QString::fromStdString(VConfig::instance()->appLongName());
    setWindowTitle(wt);

    attachInfo();

    connect(buttonBox_, SIGNAL(clicked(QAbstractButton*)), this, SLOT(slotButton(QAbstractButton*)));
}

AttributeEditor::~AttributeEditor() {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
#endif
    detachInfo();
#ifdef _USE_MODELESS_ATTRIBUTEDITOR
    editors.removeOne(this);
#endif
}

void AttributeEditor::edit(VInfo_ptr info, QWidget* /*parent*/) {
    Q_ASSERT(info);
    VAttribute* a = info->attribute();
    Q_ASSERT(a);
    auto aType = a->type();
    Q_ASSERT(aType);

#ifdef _USE_MODELESS_ATTRIBUTEDITOR
    Q_FOREACH (AttributeEditor* e, editors) {
        if ((e->info_ && info) && *(e->info_.get()) == *(info.get())) {
            e->raise();
            return;
        }
    }
#endif

    // For repeats we create an editor for each type
    std::string typeStr = a->type()->strName();
    if (a->type() == VAttributeType::find("repeat")) {
        typeStr += "_" + a->subType();
    }

    // The edtior will be automatically deleted on close (Qt::WA_DeleteOnClose is set)
    if (AttributeEditor* e = AttributeEditorFactory::create(typeStr, info, nullptr)) {
#ifdef _USE_MODELESS_ATTRIBUTEDITOR
        editors << e;
        e->show();
#else
        e->exec();
#endif
    }
}

void AttributeEditor::addForm(QWidget* w) {
    form_ = w;
    mainLayout_->insertWidget(3, w, 1);
}

void AttributeEditor::accept() {
    apply();
    QDialog::accept();
}

void AttributeEditor::attachInfo() {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
#endif

    if (info_) {
        if (info_->server()) {
            info_->server()->addNodeObserver(this);
            info_->server()->addServerObserver(this);
        }

        info_->addObserver(this);
    }
}

void AttributeEditor::detachInfo() {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
#endif
    if (info_) {
        if (info_->server()) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
            UiLog().dbg() << " remove NodeObserver " << this;
#endif
            info_->server()->removeNodeObserver(this);
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
            UiLog().dbg() << " remove ServerObserver " << this;
#endif
            info_->server()->removeServerObserver(this);
        }
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
        UiLog().dbg() << " remove InfoObserver";
#endif
        info_->removeObserver(this);
    }

    messageLabel_->stopLoadLabel();
}

void AttributeEditor::slotButton(QAbstractButton* b) {
    if (b && buttonBox_->buttonRole(b) == QDialogButtonBox::ResetRole) {
        resetValue();
    }
}

void AttributeEditor::checkButtonStatus() {
    setResetStatus(isValueChanged());
    setSaveStatus(isValueChanged());
}

void AttributeEditor::setResetStatus(bool st) {
    QPushButton* resetpb = buttonBox_->button(QDialogButtonBox::Reset);
    Q_ASSERT(resetpb);
    resetpb->setEnabled(st);
}

void AttributeEditor::setSaveStatus(bool st) {
    QPushButton* savepb = buttonBox_->button(QDialogButtonBox::Save);
    Q_ASSERT(savepb);
    savepb->setEnabled(st);
}

void AttributeEditor::setSuspended(bool st) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
    UiLog().dbg() << " status=" << st;
#endif

    Q_ASSERT(form_);
    form_->setEnabled(!st);

    QPushButton* savePb = buttonBox_->button(QDialogButtonBox::Save);
    Q_ASSERT(savePb);
    savePb->setEnabled(!st);

    QPushButton* resetPb = buttonBox_->button(QDialogButtonBox::Reset);
    Q_ASSERT(resetPb);
    resetPb->setEnabled(!st);
}

void AttributeEditor::notifyDataLost(VInfo* info) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
#endif
    if (info_ && info_.get() == info) {
        detachInfo();
        messageLabel_->showWarning("The parent node or the edited " + type_ +
                                   " <b>is not available</b> anymore! Please close the dialog!");
        setSuspended(true);
    }
}

void AttributeEditor::notifyBeginNodeChange(const VNode* vn,
                                            const std::vector<ecf::Aspect::Type>& aspect,
                                            const VNodeChange&) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG
#endif
    if (info_ && info_->node() && info_->node() == vn) {
        bool attrNumCh = (std::find(aspect.begin(), aspect.end(), ecf::Aspect::ADD_REMOVE_ATTR) != aspect.end());
        if (attrNumCh) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
            UiLog().dbg() << " ADD_REMOVE_ATTR";
#endif
            // try to regain the data
            info_->regainData();

            // If the attribute is not available dataLost() was already called.
            if (!info_ || !info_->hasData()) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
                UiLog().dbg() << " attribute does not exist";
#endif
                return;
            }

            Q_ASSERT(info_->server() && info_->node());
            setSuspended(false);

#if 0
            if(1)
            {
    #ifdef _UI_ATTRIBUTEDITOR_DEBUG
                UiLog().dbg() << " attribute does not exist";
    #endif
                detachInfo();
                messageLabel_->showWarning("The edited " + type_ +
                                           " <b>is not available</b> any more! Please close the dialog!");
                setSuspended(true);
            }
#endif
        }
        else {
            nodeChanged(aspect);
        }
    }
}

void AttributeEditor::notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& /*a*/) {
    if (info_ && info_->server() && info_->server() == server) {
        // TODO: implement
    }
}

void AttributeEditor::notifyServerDelete(ServerHandler* server) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG_S(server)
#endif
    if (info_ && info_->server() == server) {
        detachInfo();
        messageLabel_->showWarning("Server <b>" + QString::fromStdString(server->name()) +
                                   "</b> was removed from ecFlowUI! The edited " + type_ +
                                   " <b>is not available</b> anymore! Please close the dialog!");
        setSuspended(true);
    }
}

// This must be called at the beginning of a reset
void AttributeEditor::notifyBeginServerClear(ServerHandler* server) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG_S(server)
#endif

    if (info_) {
        if (info_->server() && info_->server() == server) {
            messageLabel_->showWarning("Server <b>" + QString::fromStdString(server->name()) +
                                       "</b> is being reloaded. \
                   Until it is finished this " +
                                       type_ + " <b>cannot be modified</b>!");

            messageLabel_->startLoadLabel();

            setSuspended(true);
            checkButtonStatus();
        }
    }
}

// This must be called at the end of a reset
void AttributeEditor::notifyEndServerScan(ServerHandler* server) {
#ifdef _UI_ATTRIBUTEDITOR_DEBUG
    UI_FUNCTION_LOG_S(server)
#endif

    if (info_) {
        if (info_->server() && info_->server() == server) {
            messageLabel_->hide();
            messageLabel_->clear();

            // We try to ressurect the info. We have to do it explicitly because it is not guaranteed
            // that notifyEndServerScan() will be first called on other objects than AttributeEditor. So it
            // is possible that the node exists but is still set to NULL in VInfo.
            info_->regainData();

            // If the node is not available dataLost() was already called.
            if (!info_ || !info_->hasData()) {
                return;
            }

            Q_ASSERT(info_->server() && info_->node());

            setSuspended(false);
        }
    }
}

void AttributeEditor::notifyServerConnectState(ServerHandler* server) {
    Q_ASSERT(server);
    if (info_->server() && info_->server() == server) {
        switch (server->connectState()->state()) {
            case ConnectState::Lost:
                messageLabel_->showWarning("Connection lost to server <b>" + QString::fromStdString(server->name()) +
                                           "</b>. \
                   Until it the connection regained this " +
                                           type_ + " <b>cannot be modified</b>!");
                messageLabel_->stopLoadLabel();
                setSuspended(true);
                break;
            case ConnectState::Normal:
                messageLabel_->hide();
                messageLabel_->clear();
                messageLabel_->stopLoadLabel();
                setSuspended(false);
                break;
            default:
                break;
        }
    }
}

void AttributeEditor::doNotUseReset() {
    if (QPushButton* resetPb = buttonBox_->button(QDialogButtonBox::Reset)) {
        resetPb->setEnabled(false);
        resetPb->hide();
    }
}

void AttributeEditor::disableCancel() {
    if (QPushButton* cancelPb = buttonBox_->button(QDialogButtonBox::Cancel)) {
        cancelPb->hide();
    }
}
