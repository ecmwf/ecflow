// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "PermissionsEditor.hpp"

#include <vector>

#include <QRegularExpression>
#include <QSettings>

#include "AttributeEditorFactory.hpp"
#include "CommandHandler.hpp"
#include "SessionHandler.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"

PermissionsEditorWidget::PermissionsEditorWidget(QWidget* parent)
    : QWidget(parent) {
    setupUi(this);

    QLayoutItem* item = grid_->itemAtPosition(1, 0);
    Q_ASSERT(item);
    item->setAlignment(Qt::AlignLeft | Qt::AlignTop);
}

PermissionsEditor::PermissionsEditor(VInfo_ptr info, QWidget* parent)
    : AttributeEditor(info, "permissions", parent) {
    w_ = new PermissionsEditorWidget(this);
    addForm(w_);

    VAttribute* a = info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "permissions");

    if (a->data().count() < 2) {
        return;
    }

    // The value is held in the command-line form: one entry per line in the dialog
    oriVal_ = a->data().at(1).split(",", Qt::SkipEmptyParts).join("\n");

    w_->valueTe_->setPlainText(oriVal_);
    w_->valueTe_->setFocus();

    header_->setInfo(QString::fromStdString(info_->path()), "Permissions");

    connect(w_->valueTe_, SIGNAL(textChanged()), this, SLOT(slotValueChanged()));

    checkButtonStatus();

    readSettings();
}

PermissionsEditor::~PermissionsEditor() {
    writeSettings();
}

std::string PermissionsEditor::value_of(const QString& text) {
    QStringList entries;
    for (const QString& line : text.split(QRegularExpression("[\\n,]"), Qt::SkipEmptyParts)) {
        if (const QString entry = line.trimmed(); !entry.isEmpty()) {
            entries << entry;
        }
    }
    return entries.join(",").toStdString();
}

void PermissionsEditor::apply() {
    // An empty value is refused by the client, before anything is sent; the attribute is removed with
    // ecflow_client --permissions delete
    const std::vector<std::string> cmd{
        "ecflow_client", "--permissions", "update", "<full_name>", value_of(w_->valueTe_->toPlainText())};
    CommandHandler::run(info_, cmd);
}

void PermissionsEditor::resetValue() {
    w_->valueTe_->setPlainText(oriVal_);
    checkButtonStatus();
}

void PermissionsEditor::slotValueChanged() {
    checkButtonStatus();
}

bool PermissionsEditor::isValueChanged() {
    return value_of(oriVal_) != value_of(w_->valueTe_->toPlainText());
}

void PermissionsEditor::writeSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("PermissionsEditor")), QSettings::NativeFormat);

    // Only the size is remembered, never a value
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    settings.endGroup();
}

void PermissionsEditor::readSettings() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    QSettings settings(QString::fromStdString(cs->qtSettingsFile("PermissionsEditor")), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(360, 260));
    }

    settings.endGroup();
}

static AttributeEditorMaker<PermissionsEditor> makerStr("permissions");
