/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NodeQueryEditor.hpp"

#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QPalette>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QtGlobal>

#include "ComboMulti.hpp"
#include "CustomListWidget.hpp"
#include "Highlighter.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryHandler.hpp"
#include "NodeQueryOption.hpp"
#include "NodeQueryOptionEdit.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "ViewerUtil.hpp"

//======================================================
//
// NodeQuerySaveDialog
//
//======================================================

NodeQuerySaveDialog::NodeQuerySaveDialog(QWidget* parent) : QDialog(parent) {
    setupUi(this);
}

QString NodeQuerySaveDialog::name() const {
    return nameLe_->text();
}

void NodeQuerySaveDialog::accept() {
    QString name = nameLe_->text();

    if (!name.contains(QRegularExpression("[\\w|\\s]+"))) {
        QMessageBox::critical(nullptr,
                              tr("Invalid character"),
                              "Query names can only contain alphanumeric characters, whitespaces and \"_\". Please "
                              "choose a different name.");
        return;
    }

    if (NodeQueryHandler::instance()->find(name.toStdString())) {
        QMessageBox::critical(nullptr,
                              tr("Duplicated"),
                              "The specified name is already used by another query. Please choose a different name.");
        return;
    }

    QDialog::accept();
}

//======================================================
//
// NodeQueryEditor
//
//======================================================

NodeQueryEditor::NodeQueryEditor(QWidget* parent) : QWidget(parent) {
    setupUi(this);

    query_ = new NodeQuery("tmp");
    // attrPanel_->setQuery(query_);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    rootLe_->setClearButtonEnabled(true);
#endif

    Q_ASSERT(tab_->count() == 2);
    nodeTabText_ = tab_->tabText(0);
    attrTabText_ = tab_->tabText(1);

    //-------------------------
    // Query display
    //-------------------------

    // QFont f("Courier");
    /*QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    f.setFixedPitch(true);
    f.setPointSize(10);
    f.setStyleStrategy(QFont::PreferAntialias);
    queryTe_->setFont(f);*/

    QFont f;
    QFontMetrics fm(f);

    queryTe_->setFixedHeight((fm.height() + 2) * 3 + 6);
    queryTe_->setReadOnly(true);
    queryTe_->setWordWrapMode(QTextOption::WordWrap);

    // The document becomes the owner of the highlighter
    new Highlighter(queryTe_->document(), "query");

    //------------------
    // Options
    //------------------

    // Max item num
    numSpin_->setRange(10, 250000);
    numSpin_->setValue(query_->maxNum());
    numSpin_->setToolTip(tr("The maximum possible value is: ") + QString::number(numSpin_->maximum()));

    connect(numSpin_, SIGNAL(valueChanged(int)), this, SLOT(slotMaxNum(int)));

    caseCb_->setChecked(query_->caseSensitive());

    connect(caseCb_, SIGNAL(clicked(bool)), this, SLOT(slotCase(bool)));

    //-------------------------
    // Scope
    //-------------------------

    // Servers
    serverCb_->setMode(ComboMulti::FilterMode);

    serverResetTb_->setEnabled(serverCb_->hasSelection());

    connect(serverCb_, SIGNAL(selectionChanged()), this, SLOT(slotServerCbChanged()));

    connect(serverResetTb_, SIGNAL(clicked()), serverCb_, SLOT(clearSelection()));

    // Root
    connect(rootLe_, SIGNAL(textChanged(QString)), this, SLOT(slotRootNodeEdited(QString)));

    // nodePathGrid_

    // Name + path
    nameEdit_ = new NodeQueryStringOptionEdit(query_->option("node_name"), nodeGrid_, this, false);
    pathEdit_ = new NodeQueryStringOptionEdit(query_->option("node_path"), nodeGrid_, this, false);

    // Status change time
    periodEdit_ = new NodeQueryPeriodOptionEdit(query_->option("status_change_time"), nodeGrid_, this);

    //-------------------------
    // Filter
    //-------------------------

    // Node type
    typeEdit_  = new NodeQueryListOptionEdit(query_->option("type"), typeList_, typeResetTb_, this);
    stateEdit_ = new NodeQueryListOptionEdit(query_->option("state"), stateList_, stateResetTb_, this);
    flagEdit_  = new NodeQueryListOptionEdit(query_->option("flag"), flagList_, flagResetTb_, this);

    attrEdit_ = new NodeQueryListOptionEdit(query_->option("attribute"), attrList_, attrResetTb_, this);

    int listHeight = (fm.height() + 2) * 6 + 6;
    typeList_->setFixedHeight(listHeight);
    stateList_->setFixedHeight(listHeight);
    flagList_->setFixedHeight(listHeight);

    listHeight    = (fm.height() + 2) * 10 + 6;
    int listWidth = ViewerUtil::textWidth(fm, "variable") + 60;
    attrList_->setFixedHeight(listHeight);
    attrList_->setFixedWidth(listWidth);

    // Attr panel
    // connect(attrPanel_,SIGNAL(queryChanged()),
    //         this,SLOT(slotAttrPanelChanged()));

    // Attributes
    attrPanel_->setProperty("attrArea", "1");
    Q_FOREACH (NodeQueryAttrGroup* aGrp, query_->attrGroup().values()) {
        Q_ASSERT(aGrp);
        QString grName = aGrp->name();

        Q_FOREACH (NodeQueryOption* op, aGrp->options()) {
            NodeQueryOptionEdit* e = nullptr;
            // TODO: use factory here
            if (op->type() == "string") {
                e = new NodeQueryStringOptionEdit(op, attrGrid_, this, false);
            }
            else if (op->type() == "combo") {
                e = new NodeQueryComboOptionEdit(op, attrGrid_, this);
            }

            Q_ASSERT(e);
            attr_[grName] << e;
            e->setVisible(false);
        }
    }

    attrPanelLayout_->addStretch(1);
    // attrPanel_->hide();

    //--------------------------------
    // Select tab
    //--------------------------------
    tab_->setCurrentIndex(0);

    //--------------------------------
    // Query management
    //--------------------------------

    connect(saveAsTb_, SIGNAL(clicked()), this, SLOT(slotSaveQueryAs()));

    connect(advModeTb_, SIGNAL(clicked(bool)), this, SLOT(slotAdvMode(bool)));

    queryManageW_->hide();

    /*advModeTb_->hide();
    queryCbLabel_->hide();
    queryCb_->hide();
    saveAsTb_->hide();
    saveTb_->hide();*/

    // attrList_->hide();
    // attrLabel_->hide();
    // attrResetTb_->hide();

    init();

    checkGuiState();
}

NodeQueryEditor::~NodeQueryEditor() {
    delete query_;

    if (serverFilter_) {
        serverFilter_->removeObserver(this);
    }
}

void NodeQueryEditor::setFilterMode(bool b) {
    if (filterMode_ != b) {
        filterMode_ = b;
        Q_ASSERT(tab_->count() == 2);
        tab_->setTabEnabled(1, !filterMode_);
    }
}

void NodeQueryEditor::init() {
    initIsOn_ = true;

    numSpin_->setValue(query_->maxNum());
    caseCb_->setChecked(query_->caseSensitive());

    numSpin_->setEnabled(!query_->ignoreMaxNum());
    numLabel_->setEnabled(!query_->ignoreMaxNum());

    // Servers
    QStringList servers = query_->servers();
    if (servers == serverCb_->all()) {
        serverCb_->clearSelection();
    }
    else {
        serverCb_->setSelection(servers);
    }

    rootLe_->setText(QString::fromStdString(query_->rootNode()));

    // Node name
    nameEdit_->setQuery(query_);
    pathEdit_->setQuery(query_);

    // Lists
    typeEdit_->setQuery(query_);
    stateEdit_->setQuery(query_);
    flagEdit_->setQuery(query_);
    attrEdit_->setQuery(query_);

    // period
    periodEdit_->setQuery(query_);

    initIsOn_ = false;

    updateQueryTe();
    checkGuiState();

    // switch to the first
    tab_->setCurrentIndex(0);
}

void NodeQueryEditor::showDefPanel(bool b) {
    scopeBox_->setVisible(b);
    optionBox_->setVisible(b);
    tab_->setVisible(b);
}

bool NodeQueryEditor::isDefPanelVisible() const {
    return scopeBox_->isVisible();
}

void NodeQueryEditor::showQueryPanel(bool b) {
    queryLabel_->setVisible(b);
    queryTe_->setVisible(b);
}

bool NodeQueryEditor::isQueryPanelVisible() const {
    return queryTe_->isVisible();
}

void NodeQueryEditor::slotAdvMode(bool /*b*/) {
#if 0
    //	filterBox_->setVisible(!b);
	queryTe_->setReadOnly(!b);
#endif
}

void NodeQueryEditor::slotMaxNum(int v) {
    if (!initIsOn_) {
        query_->setMaxNum(v);
        updateQueryTe();
    }
}

void NodeQueryEditor::slotCase(bool b) {
    if (!initIsOn_) {
        query_->setCaseSensitive(b);
        updateQueryTe();
    }
}

void NodeQueryEditor::slotServerCbChanged() {
    serverResetTb_->setEnabled(serverCb_->hasSelection());

    if (!initIsOn_) {
        query_->setServers(serverCb_->selection());
        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::slotRootNodeEdited(QString) {
    if (!initIsOn_) {
        query_->setRootNode(rootLe_->text().simplified().toStdString());
        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::slotOptionEditChanged() {
    if (!initIsOn_) {
        auto* e = static_cast<NodeQueryOptionEdit*>(sender());
        Q_ASSERT(e);
        if (e->optionId() == "attribute") {
            setAttributePanel(attrList_->selection());
        }

        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::setAttributePanel(QStringList lst) {
    QMapIterator<QString, QList<NodeQueryOptionEdit*>> it(attr_);
    while (it.hasNext()) {
        it.next();
        bool st = lst.contains(it.key());
        Q_FOREACH (NodeQueryOptionEdit* e, it.value()) {
            e->setVisible(st);
        }
    }
#if 0
    if(lst.isEmpty())
        attrPanel_->hide();
    else
        attrPanel_->show();
#endif
}

#if 0
void NodeQueryEditor::slotAttrListChanged()
{
	if(!initIsOn_)
	{
		attrResetTb_->setEnabled(attrList_->hasSelection());
		attrPanel_->setSelection(attrList_->selection());
		query_->setAttrGroupSelection(attrList_->selection());
		updateQueryTe();
		checkGuiState();
	}
}
#endif

void NodeQueryEditor::slotAttrPanelChanged() {
    if (!initIsOn_) {
        updateQueryTe();
        checkGuiState();
    }
}

void NodeQueryEditor::checkGuiState() {
    serverResetTb_->setEnabled(serverCb_->hasSelection());

    bool oneServer = (serverCb_->count() == 1 || serverCb_->selection().count() == 1);

    bool oriRootLeStatus = rootLe_->isEnabled();
    rootLabel_->setEnabled(oneServer);
    rootLe_->setEnabled(oneServer);

    if (oriRootLeStatus != rootLe_->isEnabled()) {
        if (rootLe_->isEnabled()) {
            query_->setRootNode(rootLe_->text().simplified().toStdString());
        }
        else {
            query_->setRootNode("");
        }
        updateQueryTe();
    }

    QString t = nodeTabText_;
    if (query_->hasBasicNodeQueryPart()) {
        t += "*";
    }
    tab_->setTabText(0, t);

    if (!filterMode_) {
        t = attrTabText_;
        if (!query_->attrQueryPart().isEmpty()) {
            t += "*";
        }
        tab_->setTabText(1, t);
    }
}

void NodeQueryEditor::updateQueryTe() {
    query_->buildQueryString();
    queryTe_->setHtml(query_->sqlQuery());
}

void NodeQueryEditor::slotClear() {
    nameEdit_->clear();
    pathEdit_->clear();
    typeEdit_->clear();
    stateEdit_->clear();
    flagEdit_->clear();
    periodEdit_->clear();
    attrEdit_->clear();
}

//------------------------------------------
// Servers
//------------------------------------------

QStringList NodeQueryEditor::allServers() const {
    return serverCb_->all();
}

void NodeQueryEditor::updateServers() {
    //    serverCb_->clearSelection();
    serverCb_->clear();

    if (serverFilter_) {
        for (auto it : serverFilter_->items()) {
            serverCb_->addItem(QString::fromStdString(it->name()));
        }
    }

    slotServerCbChanged();
}

void NodeQueryEditor::setServerFilter(ServerFilter* sf) {
    if (serverFilter_ == sf) {
        return;
    }

    if (serverFilter_) {
        serverFilter_->removeObserver(this);
    }

    serverFilter_ = sf;

    if (serverFilter_) {
        serverFilter_->addObserver(this);
    }

    updateServers();
}

void NodeQueryEditor::notifyServerFilterAdded(ServerItem*) {
    updateServers();
    /*ServerHandler* s=item->serverHandler();
        s->addServerObserver(this);
        updateTitle();*/
}

void NodeQueryEditor::notifyServerFilterRemoved(ServerItem* item) {
    if (filterMode_) {
        return;
    }

    auto sel = serverCb_->selection();
    auto all = serverCb_->all();
    serverCb_->clear();
    all.removeAll(QString::fromStdString(item->name()));
    sel.removeAll(QString::fromStdString(item->name()));
    for (auto s : all) {
        serverCb_->addItem(s);
    }
    serverCb_->setSelection(sel);
    slotServerCbChanged();
    Q_EMIT rerunRequested();
}

void NodeQueryEditor::notifyServerFilterChanged(ServerItem* item) {
    if (filterMode_) {
        return;
    }

    // try to detect server name change
    auto sel = serverCb_->selection();
    auto all = serverCb_->all();
    QString changedFrom, changedTo;
    for (int idx = 0; idx < all.count(); idx++) {
        if (!serverFilter_->isFiltered(all[idx].toStdString())) {
            changedFrom = all[idx];
            changedTo   = QString::fromStdString(item->name());
            all[idx]    = changedTo;
            auto idx1   = sel.indexOf(changedFrom);
            if (idx != -1) {
                sel[idx1] = changedTo;
            }
            break;
        }
    }

    if (!changedFrom.isEmpty()) {
        serverCb_->clear();
        for (auto s : all) {
            serverCb_->addItem(s);
        }
        serverCb_->setSelection(sel);
        slotServerCbChanged();
    }
}

void NodeQueryEditor::notifyServerFilterDelete() {
    serverFilter_->removeObserver(this);
    serverFilter_ = nullptr;
}

//------------------------------------------
// Root node
//------------------------------------------

void NodeQueryEditor::setRootNode(VInfo_ptr info) {
    if (info && info.get()) {
        if (ServerHandler* server = info->server()) {
            QStringList sel(QString::fromStdString(server->name()));
            serverCb_->setSelection(sel);

            if (info->isNode()) {
                if (VNode* node = info->node()) {
                    rootLe_->setText(QString::fromStdString(node->absNodePath()));
                }
            }
            else {
                rootLe_->clear();
            }
        }
    }
}

//------------------------------------------
// Query
//------------------------------------------

int NodeQueryEditor::maxNum() const {
    return numSpin_->value();
}

NodeQuery* NodeQueryEditor::query() const {
    return query_;
}

//------------------------------------------
// Query management
//------------------------------------------

void NodeQueryEditor::setQuery(NodeQuery* q) {
    query_->swap(q);
    init();
}

void NodeQueryEditor::slotSaveQueryAs() {
    NodeQuerySaveDialog d(this);
    if (d.exec() == QDialog::Accepted) {
        std::string name = d.name().toStdString();
        NodeQuery* q     = query_->clone(name);
        NodeQueryHandler::instance()->add(q, true);
    }
}
