/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TreeNodeView.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>
#include <QShortcut>
#include <QTime>
#include <QtAlgorithms>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    #include <QGuiApplication>
#endif

#include "AbstractNodeView.hpp"
#include "ActionHandler.hpp"
#include "Animation.hpp"
#include "AttributeEditor.hpp"
#include "ExpandState.hpp"
#include "PropertyMapper.hpp"
#include "ServerHandler.hpp"
#include "StandardView.hpp"
#include "TableNodeSortModel.hpp"
#include "TreeNodeModel.hpp"
#include "TreeNodeViewDelegate.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VItemPathParser.hpp"
#include "VModelData.hpp"
#include "VNode.hpp"
#include "VTree.hpp"

#define _UI_TREENODEVIEW_DEBUG

TreeNodeView::TreeNodeView(AbstractNodeView* view, TreeNodeModel* model, NodeFilterDef* filterDef, QWidget* parent)
    : QObject(parent),
      NodeViewBase(filterDef),
      view_(view),
      model_(model),
      needItemsLayout_(false),
      prop_(nullptr),
      setCurrentIsRunning_(false),
      setCurrentFromExpandIsRunning_(false),
      canRegainCurrentFromExpand_(true),
      inStartUp_(true) {
    Q_ASSERT(view_);

    setObjectName("view");
    setProperty("style", "nodeView");
    setProperty("view", "tree");

    // Context menu
    connect(view_, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotContextMenu(const QPoint&)));

    // Selection
    connect(view_, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(slotDoubleClickItem(const QModelIndex)));

    // Selection
    connect(view_,
            SIGNAL(selectionChangedInView(const QItemSelection&, const QItemSelection&)),
            this,
            SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

    // expandState_=new ExpandState(this,model_);
    actionHandler_ = new ActionHandler(this, view_);

    connect(view_->delegate(), SIGNAL(sizeHintChangedGlobal()), this, SLOT(slotSizeHintChangedGlobal()));

    // Properties
    std::vector<std::string> propVec;
    propVec.emplace_back("view.tree.indentation");
    propVec.emplace_back("view.tree.background");
    propVec.emplace_back("view.tree.autoExpandLeafNode");
    propVec.emplace_back("view.tree.drawBranchLine");
    propVec.emplace_back("view.tree.branchLineColour");
    propVec.emplace_back("view.tree.serverToolTip");
    propVec.emplace_back("view.tree.nodeToolTip");
    propVec.emplace_back("view.tree.attributeToolTip");

    prop_ = new PropertyMapper(propVec, this);

    VProperty* prop = nullptr;
    std::string propName;

    // Initialise indentation
    prop = prop_->find("view.tree.indentation", true);
    adjustIndentation(prop->value().toInt());

    // Init bg colour
    prop = prop_->find("view.tree.background", true);
    adjustBackground(prop->value().value<QColor>());

    // Init branch line status (on/off)
    prop = prop_->find("view.tree.autoExpandLeafNode", true);
    adjustAutoExpandLeafNode(prop->value().toBool());

    // Init branch line status (on/off)
    prop = prop_->find("view.tree.drawBranchLine", true);
    adjustDrawBranchLine(prop->value().toBool());

    // Init branch line/connector colour
    prop = prop_->find("view.tree.branchLineColour", true);
    adjustBranchLineColour(prop->value().value<QColor>());

    // Adjust tooltip
    prop = prop_->find("view.tree.serverToolTip", true);
    adjustServerToolTip(prop->value().toBool());

    prop = prop_->find("view.tree.nodeToolTip", true);
    adjustNodeToolTip(prop->value().toBool());

    prop = prop_->find("view.tree.attributeToolTip", true);
    adjustAttributeToolTip(prop->value().toBool());

    inStartUp_ = false;
}

TreeNodeView::~TreeNodeView() {
    delete actionHandler_;
    delete prop_;
}

QWidget* TreeNodeView::realWidget() {
    return view_;
}

QObject* TreeNodeView::realObject() {
    return this;
}

// Collect the selected list of indexes
QModelIndexList TreeNodeView::selectedList() {
    QModelIndexList lst;
    Q_FOREACH (QModelIndex idx, view_->selectedIndexes()) {
        if (idx.column() == 0) {
            lst << idx;
        }
    }
    return lst;
}

// This slot is called when the selection changed in the view
void TreeNodeView::selectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) {
#ifdef _UI_TREENODEVIEW_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndexList lst = view_->selectedIndexes();

    // When the selection was triggered from restoring the expand state of the tree
    // we do not want to broadcast it
    if (lst.count() > 0 && !setCurrentFromExpandIsRunning_) {
        VInfo_ptr info = model_->nodeInfo(lst.back());
        if (info && !info->isEmpty()) {
#ifdef _UI_TREENODEVIEW_DEBUG
            UiLog().dbg() << " emit=" << info->path();
#endif
            Q_EMIT selectionChanged(info);
        }
        // Remembers the current selection
        lastSelection_ = info;
    }

    // The model has to know about the selection in order to manage the
    // nodes that are forced to be shown. We only do it when we are not in the the middle of
    //  "setCurrentSelection()" call because that handles the forceShow independently.
    if (!setCurrentIsRunning_) {
        model_->selectionChanged(lst);
    }
}

// Returns the current selection (the last one!)
VInfo_ptr TreeNodeView::currentSelection() {
    QModelIndexList lst = view_->selectedIndexes();
    if (lst.count() > 0) {
        return model_->nodeInfo(lst.back());
    }
    return VInfo_ptr();
}

// Sets the current selection to the given VInfo item.
//  called:
//   -from outside of the view when the selection is broadcast from another view
//   -from within the view when the tree expand state was restored
void TreeNodeView::setCurrentSelection(VInfo_ptr info) {
#ifdef _UI_TREENODEVIEW_DEBUG
    UI_FUNCTION_LOG
#endif
    // We cannot call it recursively.
    // While the current item  is being selected we do not allow
    // another setCurrent call to go through
    if (!info || setCurrentIsRunning_) {
        return;
    }

    // Indicate that setCurrent started
    setCurrentIsRunning_ = true;

#ifdef _UI_TREENODEVIEW_DEBUG
    UiLog().dbg() << " info=" << info->path();
#endif

    // Forcing an object to be shown can result in altering and relayouting the tree. We
    // have to block the regaining of the selection at the end of the layout
    // process when the tree expand state is restored.
    canRegainCurrentFromExpand_ = false;

    // Force the object to be shown in the tree
    model_->setForceShow(info);

    // Lookup the object in the model
    QModelIndex idx = model_->infoToIndex(info);

    // Get the index again if it is needed
    // if(!idx.isValid())
    //{
    //     idx=model_->infoToIndex(info);
    // }

    // The re-layouting finished. We do not need to block the regaining of selection when
    // the tree expand state is restored.
    canRegainCurrentFromExpand_ = true;

    // If the item is in the model we set it as current
    if (idx.isValid()) {
        view_->setCurrentIndex(idx); // this will call selectionChanged
        view_->scrollTo(idx);
    }

    // Indicate that the set current process finished
    setCurrentIsRunning_ = false;
}

// Sets the current selection to the given VInfo item
// when the tree expand state is restored
void TreeNodeView::setCurrentSelectionFromExpand(VInfo_ptr info) {
#ifdef _UI_TREENODEVIEW_DEBUG
    UI_FUNCTION_LOG
#endif
    if (!info || setCurrentFromExpandIsRunning_) {
        return;
    }

#ifdef _UI_TREENODEVIEW_DEBUG
    UiLog().dbg() << " info=" << info->path();
#endif

    setCurrentFromExpandIsRunning_ = true;
    setCurrentSelection(info);
    setCurrentFromExpandIsRunning_ = false;
}

// Selects the first server in the view
void TreeNodeView::selectFirstServer() {
    QModelIndex idx = model_->index(0, 0);
    if (idx.isValid()) {
        view_->setCurrentIndex(idx);
        VInfo_ptr info = model_->nodeInfo(idx);
        Q_EMIT selectionChanged(info);
    }
}

void TreeNodeView::slotContextMenu(const QPoint& position) {
#ifdef _UI_TREENODEVIEW_DEBUG
    UI_FUNCTION_LOG
#endif
    QModelIndex indexClicked = view_->indexAt(position);
    QModelIndexList indexSel = selectedList();
    if (!indexSel.contains(indexClicked)) {
        indexSel.clear();
        indexSel << indexClicked;
    }

    QPoint scrollOffset(view_->horizontalScrollBar()->value(), view_->verticalScrollBar()->value());
    handleContextMenu(indexClicked, indexSel, view_->mapToGlobal(position), position + scrollOffset, view_);
}

void TreeNodeView::handleContextMenu(QModelIndex indexClicked,
                                     QModelIndexList indexLst,
                                     QPoint globalPos,
                                     QPoint /*widgetPos*/,
                                     QWidget* /*widget*/) {
    // Node actions
    if (indexClicked.isValid() && indexClicked.column() == 0) // indexLst[0].isValid() && indexLst[0].column() == 0)
    {
        std::vector<VInfo_ptr> nodeLst;
        for (int i = 0; i < indexLst.count(); i++) {
            VInfo_ptr info = model_->nodeInfo(indexLst[i]);
            if (info && !info->isEmpty()) {
                nodeLst.push_back(info);
            }
        }

        actionHandler_->contextMenu(nodeLst, globalPos);
    }

    // Desktop actions
    else {}
}

void TreeNodeView::slotDoubleClickItem(const QModelIndex& idx) {
    VInfo_ptr info = model_->nodeInfo(idx);
    if (info && info->isAttribute()) {
        slotViewCommand(info, "edit");
    }
}

void TreeNodeView::slotCommandShortcut() {
    if (auto* sc = static_cast<QShortcut*>(QObject::sender())) {
        QModelIndexList indexLst = selectedList();
        std::vector<VInfo_ptr> nodeLst;
        for (int i = 0; i < indexLst.count(); i++) {
            VInfo_ptr info = model_->nodeInfo(indexLst[i]);
            if (info && !info->isEmpty()) {
                nodeLst.push_back(info);
            }
        }
        actionHandler_->runCommand(nodeLst, sc->property("id").toInt());
    }
}

void TreeNodeView::slotViewCommand(VInfo_ptr info, QString cmd) {
    // Expand all the children of the given node
    if (cmd == "expand") {
        QModelIndex idx = model_->infoToIndex(info);
        if (idx.isValid()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif
#ifdef _UI_TREENODEVIEW_DEBUG
            QElapsedTimer t;
            t.start();
#endif
            // apply expand in the view
            view_->expandAll(idx);
#ifdef _UI_TREENODEVIEW_DEBUG
            UiLog().dbg() << "expandAll time=" << t.elapsed() / 1000. << "s";
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QGuiApplication::restoreOverrideCursor();
#endif
            // save/update the expand state object
            saveExpandAll(idx);
        }
    }
    else if (cmd == "collapse") {
        QModelIndex idx = model_->infoToIndex(info);
        if (idx.isValid()) {
            // apply expand in the view
            view_->collapseAll(idx);

            // save/update the expand state object
            saveCollapseAll(idx);
        }
    }

    else if (cmd == "edit") {
        if (info && info->isAttribute()) {
            AttributeEditor::edit(info, view_);
        }
    }

    // Only filter the given suite (of the node stored in info).
    // We achive this by setting the suitefilter to filter only this
    // suite.
    else if (cmd == "filterOne") {
        if (info) {
            if (ServerHandler* server = info->server()) {
                VNode* n = info->node();
                if (n && info->isNode()) {
                    server->setSuiteFilterWithOne(n);
                }
            }
        }
    }

    /*if(cmd == "set_as_root")
    {
        model_->setRootNode(nodeLst.at(0)->node());
        expandAll();
    }*/
}

void TreeNodeView::reload() {
    // model_->reload();
    // expandAll();
}

void TreeNodeView::rerender() {
    if (needItemsLayout_) {
        view_->doItemsLayout();
        needItemsLayout_ = false;
    }
    else {
        view_->viewport()->update();
    }
}

void TreeNodeView::slotRerender() {
    rerender();
}

void TreeNodeView::slotRepaint(Animation* an) {
    if (!an) {
        return;
    }

    Q_FOREACH (VNode* n, an->targets()) {
        view_->update(model_->nodeToIndex(n));
    }
}

void TreeNodeView::slotSizeHintChangedGlobal() {
    needItemsLayout_ = true;
}

//====================================================
// Expand state management
//====================================================

void TreeNodeView::expandTo(const QModelIndex& idxTo) {
    QModelIndex idx = model_->parent(idxTo);
    QModelIndexList lst;

    while (idx.isValid()) {
        lst.push_front(idx);
        idx = idx.parent();
    }

    Q_FOREACH (QModelIndex d, lst) {
        view_->expand(d);
    }
}

#if 0
//Save all
void TreeNodeView::slotSaveExpand()
{
    //For each server we save the expand state
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        //The expand state is stored on the VTreeServer and must survive updates and refreshes!
        ExpandState* es=ts->expandState();
        if(!es)
        {
            es=new ExpandState(view_,model_);
            ts->setExpandState(es); //the treeserver takes ownership of the expandstate
        }

        //Save the current state
        Q_ASSERT(ts->tree());
        VNode* vnode=ts->tree()->vnode();
        Q_ASSERT(vnode);
        es->save(vnode);
    }
}

void TreeNodeView::slotRestoreExpand()
{    
    //For each server we restore the expand state
    for(int i=0; i < model_->rowCount(); i++)
    {
        QModelIndex serverIdx=model_->index(i, 0);
        VTreeServer* ts=model_->indexToServer(serverIdx);
        Q_ASSERT(ts);

        //The expand state is stored on the VTreeServer
        ExpandState* es=ts->expandState();
        if(es)
        {
            Q_ASSERT(ts->tree());
            VNode* vnode=ts->tree()->vnode();
            Q_ASSERT(vnode);

            bool expanded=view_->isExpanded(serverIdx);
            view_->collapse(serverIdx);
            es->collectExpanded(vnode,view_->expandedIndexes);
            if(expanded)
                view_->expand(serverIdx);
        }
    }
    regainSelectionFromExpand();

}
#endif

// Save the expand state for the given node (it can be a server as well)
void TreeNodeView::slotSaveExpand(const VTreeNode* node) {
    UI_FUNCTION_LOG
    Q_ASSERT(node);
    VTreeServer* ts = node->server();
    Q_ASSERT(ts);

#ifdef _UI_TREENODEVIEW_DEBUG
    UiLog().dbg() << " node=" << node->vnode()->fullPath();
#endif

    ExpandState* es = ts->expandState();
    if (!es) {
        es = new ExpandState(view_, model_);
        ts->setExpandState(es); // the treeserver takes ownership of the expandstate
    }

    Q_ASSERT(es);

    // Save the current state
    es->save(node->vnode());

    // es->print();
}

// Restore the expand state for the given node (it can be a server as well)
void TreeNodeView::slotRestoreExpand(const VTreeNode* node) {
    UI_FUNCTION_LOG
    Q_ASSERT(node);
    VTreeServer* ts = node->server();
    Q_ASSERT(ts);

#ifdef _UI_TREENODEVIEW_DEBUG
    UiLog().dbg() << " node=" << node->vnode()->fullPath();
#endif

    if (ExpandState* es = ts->expandState()) {
        QModelIndex idx = model_->nodeToIndex(node);
        if (idx.isValid()) {
            bool expandedOri = view_->isExpanded(idx);
            view_->collapse(idx);
            es->collectExpanded(node->vnode(), view_->expandedIndexes);
#ifdef _UI_TREENODEVIEW_DEBUG
            UiLog().dbg() << " expanded=" << view_->isExpanded(idx);
#endif
            if (expandedOri || view_->isExpanded(idx)) {
                view_->expand(idx);
            }
        }

        // es->print();
    }

    if (canRegainCurrentFromExpand_) {
        regainSelectionFromExpand();
    }
}

void TreeNodeView::saveExpandAll(const QModelIndex& idx) {
    if (!idx.isValid()) {
        return;
    }

    VTreeNode* tnode = model_->indexToServerOrNode(idx);
    Q_ASSERT(tnode);
    VTreeServer* ts = tnode->server();
    Q_ASSERT(ts);
    Q_ASSERT(ts->tree());
    VNode* vnode = ts->tree()->vnode();
    Q_ASSERT(vnode);

    // The expand state is stored on the VTreeServer
    ExpandState* es = ts->expandState();
    if (!es) {
        es = new ExpandState(view_, model_);
        ts->setExpandState(es); // the treeserver takes ownership of the expandstate
    }
    if (es->isEmpty()) {
        es->save(vnode);
    }
    es->saveExpandAll(vnode);
}

void TreeNodeView::saveCollapseAll(const QModelIndex& idx) {
    if (!idx.isValid()) {
        return;
    }

    VTreeNode* tnode = model_->indexToServerOrNode(idx);
    Q_ASSERT(tnode);
    VTreeServer* ts = tnode->server();
    Q_ASSERT(ts);
    Q_ASSERT(ts->tree());
    VNode* vnode = ts->tree()->vnode();
    Q_ASSERT(vnode);

    // The expand state is stored on the VTreeServer
    ExpandState* es = ts->expandState();
    if (!es) {
        es = new ExpandState(view_, model_);
        ts->setExpandState(es); // the treeserver takes ownership of the expandstate
    }
    if (es->isEmpty()) {
        es->save(vnode);
    }
    es->saveCollapseAll(vnode);
}

void TreeNodeView::regainSelectionFromExpand() {
    Q_ASSERT(canRegainCurrentFromExpand_ == true);

    VInfo_ptr s = currentSelection();
    if (!s) {
        if (lastSelection_) {
            std::string lastPath = lastSelection_->storedPath();
            lastSelection_->regainData();
            if (!lastSelection_->hasData()) {
                lastSelection_.reset();

                // We might have lost the selection because the
                // selected node was removed. We try to find the parent
                // and select it when exists.
                VItemPathParser p(lastPath);
                VInfo_ptr ptr = VInfo::createFromPath(p.parent());
                if (ptr) {
                    setCurrentSelectionFromExpand(ptr);
                }
            }
            else {
                setCurrentSelectionFromExpand(lastSelection_);
            }
        }
    }
}

//==============================================
// Property handling
//==============================================

void TreeNodeView::adjustIndentation(int indent) {
    if (indent >= 0) {
        view_->setIndentation(indent);
        needItemsLayout_ = true;
    }
}

void TreeNodeView::adjustBackground(QColor col) {
    if (col.isValid()) {
        QPalette p = view_->viewport()->palette();
        p.setColor(QPalette::Window, col);
        view_->viewport()->setPalette(p);

        // When we set the palette on startup something resets the palette
        // before the first paint event happens. So we set the expected bg colour
        // so that the view should know what bg colour it should use.
        if (inStartUp_) {
            view_->setExpectedBg(col);
        }
    }
}

void TreeNodeView::adjustAutoExpandLeafNode(bool b) {
    view_->setAutoExpandLeafNode(b);
}

void TreeNodeView::adjustDrawBranchLine(bool b) {
    view_->setDrawConnector(b);
}

void TreeNodeView::adjustBranchLineColour(QColor col) {
    view_->setConnectorColour(col);
}

void TreeNodeView::adjustServerToolTip(bool st) {
    model_->setEnableServerToolTip(st);
}

void TreeNodeView::adjustNodeToolTip(bool st) {
    model_->setEnableNodeToolTip(st);
}

void TreeNodeView::adjustAttributeToolTip(bool st) {
    model_->setEnableAttributeToolTip(st);
}

void TreeNodeView::notifyChange(VProperty* p) {
    if (p->path() == "view.tree.background") {
        adjustBackground(p->value().value<QColor>());
    }
    else if (p->path() == "view.tree.indentation") {
        adjustIndentation(p->value().toInt());
    }
    else if (p->path() == "view.tree.autoExpandLeafNode") {
        adjustAutoExpandLeafNode(p->value().toBool());
    }
    else if (p->path() == "view.tree.drawBranchLine") {
        adjustDrawBranchLine(p->value().toBool());
    }
    else if (p->path() == "view.tree.branchLineColour") {
        adjustBranchLineColour(p->value().value<QColor>());
    }
    else if (p->path() == "view.tree.serverToolTip") {
        adjustServerToolTip(p->value().toBool());
    }
    else if (p->path() == "view.tree.nodeToolTip") {
        adjustNodeToolTip(p->value().toBool());
    }
    else if (p->path() == "view.tree.attributeToolTip") {
        adjustAttributeToolTip(p->value().toBool());
    }
}
