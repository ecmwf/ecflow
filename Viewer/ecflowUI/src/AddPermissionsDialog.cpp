// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "AddPermissionsDialog.hpp"

#include <string>
#include <utility>
#include <vector>

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "CommandHandler.hpp"
#include "EditorInfoLabel.hpp"
#include "PermissionsEditor.hpp"
#include "SessionHandler.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"

AddPermissionsDialog::AddPermissionsDialog(VInfo_ptr info, QWidget* parent)
    : QDialog(parent),
      info_(std::move(info)) {
    if (!info_ || !info_->isNode() || !info_->node()) {
        QDialog::reject();
        return;
    }

    setWindowTitle("Add permissions");

    auto* layout = new QVBoxLayout(this);

    header_ = new EditorInfoLabel(this);
    header_->setInfo(QString::fromStdString(info_->path()), "Permissions");
    layout->addWidget(header_);

    form_ = new PermissionsEditorWidget(this);
    layout->addWidget(form_, 1);

    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox_);

    connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
    connect(form_->valueTe_, SIGNAL(textChanged()), this, SLOT(slotValueChanged()));

    form_->valueTe_->setFocus();
    slotValueChanged();

    readSettings();
}

void AddPermissionsDialog::accept() {
    const std::string value = PermissionsEditor::value_of(form_->valueTe_->toPlainText());
    if (value.empty()) {
        return;
    }

    // The node can be gone since the dialog opened (deleted, or no longer readable after a refresh): nothing is sent
    if (!info_ || !info_->hasData() || !info_->node() || !info_->node()->node()) {
        UserMessage::message(
            UserMessage::WARN, true, "The node is no longer available: no Permissions attribute added");
        writeSettings();
        QDialog::reject();
        return;
    }

    const std::vector<std::string> cmd{"ecflow_client", "--permissions", "create", "<full_name>", value};
    CommandHandler::run(info_, cmd);

    writeSettings();
    QDialog::accept();
}

void AddPermissionsDialog::reject() {
    writeSettings();
    QDialog::reject();
}

void AddPermissionsDialog::slotValueChanged() {
    const bool hasEntries = !PermissionsEditor::value_of(form_->valueTe_->toPlainText()).empty();
    buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(hasEntries);
}

void AddPermissionsDialog::closeEvent(QCloseEvent* event) {
    event->accept();
    writeSettings();
}

void AddPermissionsDialog::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("AddPermissionsDialog")), QSettings::NativeFormat);

    // Only the size is remembered, never a value
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    settings.endGroup();
}

void AddPermissionsDialog::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("AddPermissionsDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(360, 260));
    }

    settings.endGroup();
}
