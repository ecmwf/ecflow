// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ServerPermissionsDialog.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "CommandHandler.hpp"
#include "EditorInfoLabel.hpp"
#include "PermissionsEditor.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "SessionHandler.hpp"
#include "UserMessage.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/Defs.hpp"

namespace {

// The server attribute in the command-line form, or nothing when the server does not use Permissions
std::optional<std::string> server_permissions(ServerHandler* server) {
    if (!server || server->isAccessDenied()) {
        return std::nullopt;
    }
    ServerDefsAccess defsAccess(server); // relinquishes its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (!defs) {
        return std::nullopt;
    }
    if (const auto* permissions = defs->server_state().permissions(); permissions) {
        return permissions->to_cli();
    }
    return std::nullopt;
}

} // namespace

ServerPermissionsDialog::ServerPermissionsDialog(VInfo_ptr info, QWidget* parent)
    : QDialog(parent),
      info_(std::move(info)) {
    if (!info_ || !info_->isServer()) {
        return;
    }
    const auto current = server_permissions(info_->server());
    if (!current) {
        return;
    }
    valid_  = true;
    server_ = info_->server();
    server_->addServerObserver(this);

    setWindowTitle("Edit permissions");

    auto* layout = new QVBoxLayout(this);

    header_ = new EditorInfoLabel(this);
    header_->setInfo(QString::fromStdString(info_->server()->name()) + " (server)", "Permissions");
    layout->addWidget(header_);

    messageLabel_ = new QLabel(this);
    messageLabel_->setWordWrap(true);
    messageLabel_->hide();
    layout->addWidget(messageLabel_);

    form_ = new PermissionsEditorWidget(this);
    layout->addWidget(form_, 1);

    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox_);

    // The value is held in the command-line form: one entry per line in the dialog
    oriVal_ = QString::fromStdString(*current).split(",", Qt::SkipEmptyParts).join("\n");
    form_->valueTe_->setPlainText(oriVal_);

    connect(buttonBox_, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox_, SIGNAL(rejected()), this, SLOT(reject()));
    connect(form_->valueTe_, SIGNAL(textChanged()), this, SLOT(slotValueChanged()));

    form_->valueTe_->setFocus();
    slotValueChanged();

    readSettings();
}

ServerPermissionsDialog::~ServerPermissionsDialog() {
    detachServer();
}

void ServerPermissionsDialog::detachServer() {
    if (server_) {
        server_->removeServerObserver(this);
        server_ = nullptr;
    }
}

void ServerPermissionsDialog::withdraw(const QString& reason) {
    withdrawn_ = true;
    if (form_) {
        form_->hide();
    }
    if (buttonBox_) {
        buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    messageLabel_->setText(reason);
    messageLabel_->show();
}

void ServerPermissionsDialog::notifyDefsChanged(ServerHandler* /*server*/,
                                                const std::vector<ecf::Aspect::Type>& /*a*/) {
}

void ServerPermissionsDialog::notifyServerDelete(ServerHandler* server) {
    if (server == server_) {
        detachServer();
        withdraw("The server is no longer available: please close the dialog.");
    }
}

void ServerPermissionsDialog::notifyServerConnectState(ServerHandler* server) {
    if (server == server_ && server->isAccessDenied()) {
        withdraw("Read access to the server was denied: its Permissions are no longer shown.");
    }
}

void ServerPermissionsDialog::notifyEndServerScan(ServerHandler* server) {
    if (server == server_ && !server_permissions(server)) {
        withdraw("The server no longer uses Permissions, or denied access: please close the dialog.");
    }
}

void ServerPermissionsDialog::accept() {
    if (withdrawn_) {
        return;
    }
    const std::string value = PermissionsEditor::value_of(form_->valueTe_->toPlainText());
    if (value.empty()) {
        return;
    }

    if (value == PermissionsEditor::value_of(oriVal_)) {
        writeSettings();
        QDialog::accept();
        return;
    }

    // The server can have stopped using Permissions, or denied access, since the dialog opened: nothing is sent
    if (!info_ || !info_->server() || !server_permissions(info_->server())) {
        UserMessage::message(
            UserMessage::WARN, true, "The server Permissions are no longer available: nothing changed");
        writeSettings();
        QDialog::reject();
        return;
    }

    const std::vector<std::string> cmd{"ecflow_client", "--permissions", "update", "/", value};
    CommandHandler::run(info_, cmd);

    writeSettings();
    QDialog::accept();
}

void ServerPermissionsDialog::reject() {
    writeSettings();
    QDialog::reject();
}

void ServerPermissionsDialog::slotValueChanged() {
    if (withdrawn_) {
        return;
    }
    const bool hasEntries = !PermissionsEditor::value_of(form_->valueTe_->toPlainText()).empty();
    buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(hasEntries);
}

void ServerPermissionsDialog::closeEvent(QCloseEvent* event) {
    event->accept();
    writeSettings();
}

void ServerPermissionsDialog::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ServerPermissionsDialog")), QSettings::NativeFormat);

    // Only the size is remembered, never a value
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    settings.endGroup();
}

void ServerPermissionsDialog::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("ServerPermissionsDialog")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(360, 260));
    }

    settings.endGroup();
}
