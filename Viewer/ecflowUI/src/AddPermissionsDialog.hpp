// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDialog>

#include "VInfo.hpp"

class QCloseEvent;
class QDialogButtonBox;
class EditorInfoLabel;
class PermissionsEditorWidget;

///
/// @brief Adds a Permissions attribute to a node that has none (D6.1).
///
/// The dialog holds the same form as PermissionsEditor: one entry per line, in the command-line form
/// <user>:<letters>. Accepting it sends `--permissions create <path> <entries>`, which the server validates as every
/// change of Permissions attributes; a refused change is reported like any other failed command.
///
class AddPermissionsDialog : public QDialog {
    Q_OBJECT

public:
    ///
    /// @brief Creates the dialog, with no entries, for the node selected.
    ///
    /// @param[in] info The node selected; anything else rejects the dialog
    /// @param[in] parent The parent widget
    ///
    explicit AddPermissionsDialog(VInfo_ptr info, QWidget* parent = nullptr);

protected Q_SLOTS:
    ///
    /// @brief Sends `--permissions create <path> <value>`, the value taken from the text by
    ///        PermissionsEditor::value_of(), and closes the dialog.
    ///
    /// Nothing is sent when the node is no longer available (deleted, or no longer readable after a refresh while
    /// the dialog was open): a warning is shown and the dialog is closed.
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

    VInfo_ptr info_;
    EditorInfoLabel* header_{nullptr};
    PermissionsEditorWidget* form_{nullptr};
    QDialogButtonBox* buttonBox_{nullptr};
};
