// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDialog>
#include <QString>

#include "ServerObserver.hpp"
#include "VInfo.hpp"

class QCloseEvent;
class QDialogButtonBox;
class QLabel;
class EditorInfoLabel;
class PermissionsEditorWidget;

///
/// @brief Edits the Permissions attribute of the server (D6.1), which the tree does not show as a row.
///
/// The dialog holds the same form as PermissionsEditor, filled with the entries of the server attribute, one per
/// line, in the command-line form <user>:<letters>. Accepting it sends `--permissions update / <entries>`, which the
/// server validates as every change of Permissions attributes; a refused change (for example one leaving nobody
/// holding p) is reported like any other failed command. The server attribute is never added or deleted from
/// ecFlowUI: it is configured when the server starts.
///
/// The dialog observes the server: when the user loses read access to it, when the server stops using Permissions,
/// or when it is deleted, the entries are no longer shown and nothing can be sent (R6.4).
///
class ServerPermissionsDialog : public QDialog, public ServerObserver {
    Q_OBJECT

public:
    ///
    /// @brief Creates the dialog for the server selected, filled with the entries of its attribute.
    ///
    /// @param[in] info The server selected; anything else, or a server without a server attribute, rejects the dialog
    /// @param[in] parent The parent widget
    ///
    explicit ServerPermissionsDialog(VInfo_ptr info, QWidget* parent = nullptr);

    ///
    /// @brief Destroys the dialog, no longer observing the server.
    ///
    ~ServerPermissionsDialog() override;

    /// @copydoc ServerObserver::notifyDefsChanged
    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) override;

    ///
    /// @brief Withdraws the entries when the server is deleted from ecFlowUI.
    ///
    /// @param[in] server The server deleted
    ///
    void notifyServerDelete(ServerHandler* server) override;

    ///
    /// @brief Withdraws the entries when the user lost read access to the server.
    ///
    /// @param[in] server The server whose connection state changed
    ///
    void notifyServerConnectState(ServerHandler* server) override;

    ///
    /// @brief Withdraws the entries when the server no longer uses Permissions, or denies access, after a scan.
    ///
    /// @param[in] server The server scanned
    ///
    void notifyEndServerScan(ServerHandler* server) override;

    ///
    /// @brief Returns whether the dialog was set up, i.e. the server selected uses Permissions.
    ///
    /// @return True when the dialog holds the entries of a server attribute.
    ///
    bool isValid() const { return valid_; }

protected Q_SLOTS:
    ///
    /// @brief Sends `--permissions update / <value>`, the value taken from the text by
    ///        PermissionsEditor::value_of(), and closes the dialog.
    ///
    /// Nothing is sent when the value is unchanged, or when the server no longer uses Permissions or is no longer
    /// readable (a refresh while the dialog was open): in the latter case a warning is shown.
    ///
    void accept() override;

    ///
    /// @brief Closes the dialog without sending anything.
    ///
    void reject() override;

    ///
    /// @brief Enables the OK button only when the text holds at least one entry.
    ///
    void slotValueChanged();

protected:
    ///
    /// @brief Saves the size of the dialog when it is closed by the window manager.
    ///
    /// @param[in] event The close event, accepted
    ///
    void closeEvent(QCloseEvent* event) override;

    ///
    /// @brief Saves the size of the dialog in the session settings; no value is saved.
    ///
    void writeSettings();

    ///
    /// @brief Restores the size of the dialog from the session settings.
    ///
    void readSettings();

    ///
    /// @brief Hides the entries, disables OK, and shows the reason; the dialog can then only be closed.
    ///
    /// @param[in] reason The reason shown to the user
    ///
    void withdraw(const QString& reason);

    ///
    /// @brief Stops observing the server, if it is still observed.
    ///
    void detachServer();

    VInfo_ptr info_;
    ServerHandler* server_{nullptr};
    bool withdrawn_{false};
    QLabel* messageLabel_{nullptr};
    bool valid_{false};
    QString oriVal_;
    EditorInfoLabel* header_{nullptr};
    PermissionsEditorWidget* form_{nullptr};
    QDialogButtonBox* buttonBox_{nullptr};
};
