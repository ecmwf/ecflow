/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "CommandOutputWidget.hpp"

#include <QDebug>
#include <QItemSelectionModel>

#include "CommandOutput.hpp"
#include "ModelColumn.hpp"
#include "TextFormat.hpp"
#include "ViewerUtil.hpp"

CommandOutputModel::CommandOutputModel(QObject* parent) : QAbstractItemModel(parent) {
    columns_ = ModelColumn::def("output_columns");

    assert(columns_);
}

CommandOutputModel::~CommandOutputModel() = default;

bool CommandOutputModel::hasData() const {
    return CommandOutputHandler::instance()->itemCount() > 0;
}

void CommandOutputModel::dataIsAboutToChange() {
    beginResetModel();
}

void CommandOutputModel::dataChanged() {
    endResetModel();
}

int CommandOutputModel::columnCount(const QModelIndex& /*parent */) const {
    return columns_->count();
}

int CommandOutputModel::rowCount(const QModelIndex& parent) const {
    if (!hasData()) {
        return 0;
    }

    // Parent is the root:
    if (!parent.isValid()) {
        return CommandOutputHandler::instance()->itemCount();
    }

    return 0;
}

Qt::ItemFlags CommandOutputModel::flags(const QModelIndex& /*index*/) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant CommandOutputModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !hasData()) {
        return {};
    }

    int pos = CommandOutputHandler::instance()->itemCount() - index.row() - 1;
    if (pos < 0 || pos >= CommandOutputHandler::instance()->itemCount()) {
        return {};
    }

    QString id = columns_->id(index.column());

    CommandOutput_ptr item = CommandOutputHandler::instance()->items()[pos];
    if (!item) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        if (id == "command") {
            return item->command();
        }
        else if (id == "status") {
            return item->statusStr();
        }
        else if (id == "runtime") {
            return item->runTime().toString("yyyy-MM-dd hh:mm:ss");
        }
        else {
            return {};
        }
    }
    else if (role == Qt::ForegroundRole) {
        if (id == "status") {
            return item->statusColour();
        }
        return {};
    }
    return {};
}

QVariant CommandOutputModel::headerData(const int section, const Qt::Orientation orient, const int role) const {
    if (orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole)) {
        return QAbstractItemModel::headerData(section, orient, role);
    }

    if (role == Qt::DisplayRole) {
        return columns_->label(section);
    }
    else if (role == Qt::UserRole) {
        return columns_->id(section);
    }

    return {};
}

QModelIndex CommandOutputModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasData() || row < 0 || column < 0) {
        return {};
    }

    // When parent is the root this index refers to a node or server
    if (!parent.isValid()) {
        return createIndex(row, column);
    }

    return {};
}

QModelIndex CommandOutputModel::parent(const QModelIndex& /*child*/) const {
    return {};
}

CommandOutput_ptr CommandOutputModel::indexToItem(const QModelIndex& idx) const {
    if (idx.isValid() && hasData()) {
        int pos = CommandOutputHandler::instance()->itemCount() - idx.row() - 1;
        if (pos >= 0 || pos < CommandOutputHandler::instance()->itemCount()) {
            return CommandOutputHandler::instance()->items()[pos];
        }
    }

    CommandOutput_ptr r;
    return r;
}

QModelIndex CommandOutputModel::itemToStatusIndex(CommandOutput_ptr item) const {
    if (item) {
        int pos = CommandOutputHandler::instance()->indexOfItem(item);
        if (pos != -1) {
            int row = CommandOutputHandler::instance()->itemCount() - pos - 1;
            if (row >= 0 && row < rowCount()) {
                return index(row, columns_->indexOf("status"));
            }
        }
    }

    return {};
}

//==============================================
//
// CommandOutputWidget
//
//==============================================

CommandOutputWidget::CommandOutputWidget(QWidget* parent) : QWidget(parent), errCol_(QColor(204, 0, 0)) {
    setupUi(this);

    infoLabel_->setProperty("fileInfo", "1");

    errWidget_->hide();

    model_ = new CommandOutputModel(this);

    tree_->setModel(model_);
    tree_->setRootIsDecorated(false);

    // Adjust the tree columns
    QFont f;
    QFontMetrics fm(f);
    tree_->setColumnWidth(
        0, ViewerUtil::textWidth(fm, " sh ecflow_client --port %ECF_PORT% --host %ECF_HOST% --stats--port %ECF_PORT%"));
    tree_->setColumnWidth(1, ViewerUtil::textWidth(fm, " running "));

    errTe_->setShowLineNumbers(false);
    outTe_->setShowLineNumbers(false);

    searchLine_->setEditor(outTe_);
    searchLine_->setVisible(false);

    CommandOutputHandler* handler = CommandOutputHandler::instance();
    Q_ASSERT(handler);

    connect(handler, SIGNAL(itemAddBegin()), this, SLOT(slotItemAddBegin()));

    connect(handler, SIGNAL(itemAddEnd()), this, SLOT(slotItemAddEnd()));

    connect(handler,
            SIGNAL(itemOutputAppend(CommandOutput_ptr, QString)),
            this,
            SLOT(slotItemOutputAppend(CommandOutput_ptr, QString)));

    connect(handler,
            SIGNAL(itemErrorAppend(CommandOutput_ptr, QString)),
            this,
            SLOT(slotItemErrorAppend(CommandOutput_ptr, QString)));

    connect(handler, SIGNAL(itemOutputReload(CommandOutput_ptr)), this, SLOT(slotItemOutputReload(CommandOutput_ptr)));

    connect(handler, SIGNAL(itemErrorReload(CommandOutput_ptr)), this, SLOT(slotItemErrorReload(CommandOutput_ptr)));

    connect(
        handler, SIGNAL(itemStatusChanged(CommandOutput_ptr)), this, SLOT(slotItemStatusChanged(CommandOutput_ptr)));

    splitter_->setCollapsible(0, false);
    splitter_->setCollapsible(1, false);
    splitter_->setCollapsible(2, false);

    // The selection changes in the view
    connect(tree_->selectionModel(),
            SIGNAL(currentChanged(QModelIndex, QModelIndex)),
            this,
            SLOT(slotItemSelected(QModelIndex, QModelIndex)));

    if (model_->rowCount() > 0) {
        tree_->setCurrentIndex(model_->index(0, 0));
    }
}

CommandOutputWidget::~CommandOutputWidget() = default;

bool CommandOutputWidget::isCurrent(CommandOutput_ptr item) {
    return (item && item.get() == model_->indexToItem(tree_->currentIndex()).get());
}

void CommandOutputWidget::slotItemSelected(const QModelIndex&, const QModelIndex&) {
    CommandOutput_ptr current = model_->indexToItem(tree_->currentIndex());
    loadItem(current);
}

void CommandOutputWidget::slotItemAddBegin() {
    Q_ASSERT(model_);
    model_->dataIsAboutToChange();
}

void CommandOutputWidget::slotItemAddEnd() {
    Q_ASSERT(model_);
    model_->dataChanged();
    if (model_->rowCount() > 0) {
        tree_->setCurrentIndex(model_->index(0, 0));
    }
}

void CommandOutputWidget::slotItemOutputAppend(CommandOutput_ptr item, QString txt) {
    if (isCurrent(item)) {
        outTe_->appendPlainText(txt);
    }
}

void CommandOutputWidget::slotItemErrorAppend(CommandOutput_ptr item, QString txt) {
    if (isCurrent(item)) {
        errWidget_->show();
        errTe_->appendHtml(formatErrorText(txt));
    }
}

void CommandOutputWidget::slotItemOutputReload(CommandOutput_ptr item) {
    if (isCurrent(item)) {
        outTe_->setPlainText(item->output());
    }
}

void CommandOutputWidget::slotItemErrorReload(CommandOutput_ptr item) {
    if (isCurrent(item)) {
        errTe_->clear();
        errWidget_->show();
        errTe_->appendHtml(formatErrorText(item->error()));
    }
}

void CommandOutputWidget::slotItemStatusChanged(CommandOutput_ptr item) {
    if (item) {
        QModelIndex idx = model_->itemToStatusIndex(item);
        if (idx.isValid()) {
            tree_->update(idx);
        }
        if (item == model_->indexToItem(tree_->currentIndex())) {
            updateInfoLabel(item);
        }
    }
}

void CommandOutputWidget::loadItem(CommandOutput_ptr item) {
    if (item) {
        outTe_->clear();
        errTe_->clear();
        errWidget_->hide();
        updateInfoLabel(item);

        // Set output text
        outTe_->setPlainText(item->output());

        // Set error text
        QString err = item->error();
        if (!err.isEmpty()) {
            errWidget_->show();
            errTe_->appendHtml(formatErrorText(err));
        }
        // return true;
    }
}

void CommandOutputWidget::updateInfoLabel(CommandOutput_ptr item) {
    if (!item) {
        infoLabel_->clear();
        return;
    }

    QColor boldCol(39, 49, 101);
    QColor defCol(90, 90, 90);
    QString s = Viewer::formatBoldText("Command: ", boldCol) + item->command() + "<br>";

    if (!item->commandDefinition().isEmpty()) {
        s += Viewer::formatBoldText("Definition: ", boldCol) + Viewer::formatText(item->commandDefinition(), defCol) +
             "<br>";
    }

    s += Viewer::formatBoldText("Started at: ", boldCol) + item->runTime().toString("yyyy-MM-dd hh:mm:ss") +
         Viewer::formatBoldText(" &nbsp;Status: ", boldCol) +
         Viewer::formatText(item->statusStr(), item->statusColour());

    infoLabel_->setText(s);
}

void CommandOutputWidget::removeSpacer() {
    // Remove the first spacer item!!
    for (int i = 0; horizontalLayout->count(); i++) {
        if (QSpacerItem* sp = horizontalLayout->itemAt(i)->spacerItem()) {
            horizontalLayout->takeAt(i);
            delete sp;
            break;
        }
    }
}

void CommandOutputWidget::on_searchTb__clicked() {
    searchLine_->setVisible(true);
    searchLine_->setFocus();
    searchLine_->selectAll();
}

void CommandOutputWidget::on_gotoLineTb__clicked() {
    outTe_->gotoLine();
}

void CommandOutputWidget::on_fontSizeUpTb__clicked() {
    // We need to call a custom slot here instead of "zoomIn"!!!
    outTe_->slotZoomIn();
}

void CommandOutputWidget::on_fontSizeDownTb__clicked() {
    // We need to call a custom slot here instead of "zoomOut"!!!
    outTe_->slotZoomOut();
}

QString CommandOutputWidget::formatErrorText(QString txt) {
    return Viewer::formatText(txt.replace('\n', "<br>"), errCol_);
}

void CommandOutputWidget::writeSettings(QSettings& settings) {
    settings.beginGroup("widget");
    settings.setValue("splitter_v1", splitter_->saveState());
    ViewerUtil::saveTreeColumnWidth(settings, "treeColumnWidth", tree_);
    settings.endGroup();
}

void CommandOutputWidget::readSettings(QSettings& settings) {
    settings.beginGroup("widget");

    ViewerUtil::initTreeColumnWidth(settings, "treeColumnWidth", tree_);

    if (settings.contains("splitter_v1")) {
        splitter_->restoreState(settings.value("splitter_v1").toByteArray());
    }

    settings.endGroup();
}
