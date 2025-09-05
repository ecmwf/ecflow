/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TextFilterHandlerDialog.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QTableWidgetItem>

#include "SessionHandler.hpp"
#include "TextFilterHandler.hpp"

//======================================
//
// TextFilterAddDialog
//
//======================================

TextFilterAddDialog::TextFilterAddDialog(QWidget* parent) : QDialog(parent) {
    setupUi(this);

    setWindowTitle(tr("Add item"));

    // match
    matchCb_->addItem(QIcon(QPixmap(":/viewer/filter_match.svg")), tr("match"), 0);
    matchCb_->addItem(QIcon(QPixmap(":/viewer/filter_no_match.svg")), tr("no match"), 1);
}

TextFilterItem TextFilterAddDialog::item() {
    return TextFilterItem(
        filterLe_->text().toStdString(), (matchCb_->currentIndex() == 0), caseCb_->isChecked(), menuCb_->isChecked());
}

void TextFilterAddDialog::init(const TextFilterItem& item) {
    filterLe_->setText(QString::fromStdString(item.filter()));
    matchCb_->setCurrentIndex(item.matched() ? 0 : 1);
    caseCb_->setChecked(item.caseSensitive());
    menuCb_->setChecked(item.contextMenu());
}

void TextFilterAddDialog::accept() {
    TextFilterItem it = item();
    if (TextFilterHandler::Instance()->contains(it.filter(), it.matched(), it.caseSensitive())) {
        QMessageBox::critical(nullptr,
                              tr("Save text filter"),
                              "Cannot save text filter! A text filter with the same regexp: <b>" +
                                  QString::fromStdString(it.filter()) + "</b> and settings already exists!");
        return;
    }

    TextFilterHandler::Instance()->add(it);
    QDialog::accept();
}

//======================================
//
// TextFilterEditDialog
//
//======================================

TextFilterEditDialog::TextFilterEditDialog(QWidget* parent) : TextFilterAddDialog(parent) {
    setWindowTitle(tr("Edit item"));
}

void TextFilterEditDialog::init(int itemIndex, const TextFilterItem& item) {
    itemIndex_ = itemIndex;
    TextFilterAddDialog::init(item);
}

void TextFilterEditDialog::accept() {
    if (itemIndex_ >= 0) {
        TextFilterItem it = item();

        if (TextFilterHandler::Instance()->items()[itemIndex_] == it &&
            TextFilterHandler::Instance()->items()[itemIndex_].contextMenu() == it.contextMenu()) {
            QDialog::reject();
            return;
        }

        if (TextFilterHandler::Instance()->containsExceptOne(
                itemIndex_, it.filter(), it.matched(), it.caseSensitive())) {
            QMessageBox::critical(nullptr,
                                  tr("Save text filter"),
                                  "Cannot save text filter! A text filter with the same regexp: <b>" +
                                      QString::fromStdString(it.filter()) + "</b> and settings already exists!");
            return;
        }

        TextFilterHandler::Instance()->update(itemIndex_, it);
    }

    QDialog::accept();
}

//======================================
//
// TextFilterHandlerDialog
//
//======================================

TextFilterHandlerDialog::TextFilterHandlerDialog(QWidget* parent) : QDialog(parent) {
    setupUi(this);

    auto* sep1 = new QAction(this);
    sep1->setSeparator(true);

    auto* sep2 = new QAction(this);
    sep2->setSeparator(true);

    auto* sep3 = new QAction(this);
    sep3->setSeparator(true);

    table_->addAction(actionAdd_);
    table_->addAction(sep1);
    table_->addAction(actionDuplicate_);
    table_->addAction(actionEdit_);
    table_->addAction(sep2);
    table_->addAction(actionApply_);
    table_->addAction(sep3);
    table_->addAction(actionRemove_);

    // Add actions for the toolbuttons
    addTb_->setDefaultAction(actionAdd_);
    editTb_->setDefaultAction(actionEdit_);
    removeTb_->setDefaultAction(actionRemove_);
    duplicateTb_->setDefaultAction(actionDuplicate_);
    applyTb_->setDefaultAction(actionApply_);

    // Init the table
    reloadTable();

    // init
    readSettings();
}

TextFilterHandlerDialog::~TextFilterHandlerDialog() {
    writeSettings();
}

void TextFilterHandlerDialog::reloadTable() {
    // The itemlist is fairly small. For simplicity we do not use a model/view solution here just
    // a tablewidget. We reload the whole table widget whenerver there is a change in the list.

    table_->clear(); // it removes the headers as well

    QStringList headers;
    headers << "Match mode"
            << "Case sensitive"
            << "Context menu"
            << "Regexp (grep)";
    table_->setHorizontalHeaderLabels(headers);

    const std::vector<TextFilterItem>& items = TextFilterHandler::Instance()->items();
    table_->setRowCount(items.size());
    for (size_t i = 0; i < items.size(); i++) {
        QString filterTxt = QString::fromStdString(items[i].filter());
        // Replace whitespace with Open Box U+2423 just for better interpretation
        filterTxt.replace(QChar(' '), QChar(9251));

        auto* filterItem  = new QTableWidgetItem(filterTxt);
        auto* matchedItem = new QTableWidgetItem((items[i].matched()) ? "match" : "no match");
        auto* caseItem    = new QTableWidgetItem((items[i].caseSensitive()) ? "yes" : "no");
        auto* contextItem = new QTableWidgetItem((items[i].contextMenu()) ? "yes" : "no");

        table_->setItem(i, 0, matchedItem);
        table_->setItem(i, 1, caseItem);
        table_->setItem(i, 2, contextItem);
        table_->setItem(i, 3, filterItem);
    }

    if (table_->rowCount() > 0) {
        table_->setCurrentCell(0, 0);
    }

    updateStatus();
}

void TextFilterHandlerDialog::editItem() {
    int r = table_->currentRow();
    if (r >= 0) {
        TextFilterEditDialog diag(this);
        diag.init(r, TextFilterHandler::Instance()->items()[r]);
        if (diag.exec() == QDialog::Accepted) {
            reloadTable();
        }
    }
}

void TextFilterHandlerDialog::on_actionEdit__triggered() {
    editItem();
}

void TextFilterHandlerDialog::on_actionDuplicate__triggered() {
    int r = table_->currentRow();
    if (r >= 0) {
        TextFilterAddDialog diag(this);
        diag.init(TextFilterHandler::Instance()->items()[r]);
        if (diag.exec() == QDialog::Accepted) {
            reloadTable();
        }
    }
}

void TextFilterHandlerDialog::on_actionRemove__triggered() {
    int r = table_->currentRow();
    if (r >= 0) {
        TextFilterHandler::Instance()->remove(r);
        reloadTable();
    }
}

void TextFilterHandlerDialog::on_actionAdd__triggered() {
    TextFilterAddDialog diag(this);
    if (diag.exec() == QDialog::Accepted) {
        reloadTable();
    }
}

void TextFilterHandlerDialog::on_table__doubleClicked(const QModelIndex& /*index*/) {
    editItem();
}

void TextFilterHandlerDialog::on_actionApply__triggered() {
    applyIndex_ = table_->currentRow();
    close();
}

void TextFilterHandlerDialog::updateStatus() {
    bool hasSelected = table_->currentRow() >= 0;

    actionEdit_->setEnabled(hasSelected);
    actionDuplicate_->setEnabled(hasSelected);
    actionRemove_->setEnabled(hasSelected);
    actionApply_->setEnabled(hasSelected);
}

QString TextFilterHandlerDialog::settingsFile() {
    SessionItem* cs = SessionHandler::instance()->current();
    Q_ASSERT(cs);
    return QString::fromStdString(cs->qtSettingsFile("TextFilterHandlerDialog"));
}

void TextFilterHandlerDialog::writeSettings() {
    QSettings settings(settingsFile(), QSettings::NativeFormat);

    // We have to clear it not to remember all the previous windows
    settings.clear();

    settings.beginGroup("main");
    settings.setValue("size", size());
    settings.endGroup();
}

void TextFilterHandlerDialog::readSettings() {
    QSettings settings(settingsFile(), QSettings::NativeFormat);

    settings.beginGroup("main");
    if (settings.contains("size")) {
        resize(settings.value("size").toSize());
    }
    else {
        resize(QSize(520, 330));
    }

    settings.endGroup();
}
