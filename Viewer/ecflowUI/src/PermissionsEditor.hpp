// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include "AttributeEditor.hpp"
#include "VInfo.hpp"
#include "ui_PermissionsEditorWidget.h"

///
/// @brief Holds the form of the dialogs that add and edit a Permissions attribute.
///
class PermissionsEditorWidget : public QWidget, protected Ui::PermissionsEditorWidget {
    friend class PermissionsEditor;
    friend class AddPermissionsDialog;
    friend class ServerPermissionsDialog;

public:
    ///
    /// @brief Creates the form.
    ///
    /// @param[in] parent The parent widget, which owns the form
    ///
    explicit PermissionsEditorWidget(QWidget* parent = nullptr);
};

///
/// @brief Edits the Permissions attribute of a node in a dedicated dialog (D6.1).
///
/// The entries are shown one per line, in the command-line form <user>:<letters>. Applying the dialog sends
/// `--permissions update <path> <entries>`, which the server validates as every change of Permissions attributes;
/// a refused change is reported like any other failed command.
///
class PermissionsEditor : public AttributeEditor {
    Q_OBJECT
public:
    ///
    /// @brief Creates the dialog for the attribute selected, filled with its entries, one per line.
    ///
    /// @param[in] info The attribute selected, of type "permissions"
    /// @param[in] parent The parent widget
    ///
    explicit PermissionsEditor(VInfo_ptr info, QWidget* parent = nullptr);

    ///
    /// @brief Destroys the dialog, remembering its size.
    ///
    ~PermissionsEditor() override;

    ///
    /// @brief Returns the value of the attribute, in the command-line form, from the text of the dialog.
    ///
    /// @param[in] text One entry per line; blank lines, and spaces around entries, are ignored, and commas also
    ///            separate entries
    /// @return The entries, separated by commas, e.g. "alice:rwxp,bob:r".
    ///
    static std::string value_of(const QString& text);

protected Q_SLOTS:
    ///
    /// @brief Updates the state of the buttons when the text changes.
    ///
    void slotValueChanged();

protected:
    ///
    /// @brief Sends `--permissions update <path> <value>`, the value taken from the text by value_of().
    ///
    void apply() override;

    ///
    /// @brief Restores the entries shown when the dialog was opened.
    ///
    void resetValue() override;

    ///
    /// @brief Returns whether the value differs from the one shown when the dialog was opened.
    ///
    /// @return True when value_of() of the text differs from that of the original entries.
    ///
    bool isValueChanged() override;

    ///
    /// @brief Restores the size of the dialog from the session settings.
    ///
    void readSettings();

    ///
    /// @brief Saves the size of the dialog in the session settings; no value is saved.
    ///
    void writeSettings();

    PermissionsEditorWidget* w_;
    QString oriVal_;
};
