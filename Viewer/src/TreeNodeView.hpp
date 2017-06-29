//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TreeNodeView_HPP_
#define TreeNodeView_HPP_

#include <QTreeView>

#include "NodeViewBase.hpp"

//#include "ExpandState.hpp"
#include "VInfo.hpp"
#include "VProperty.hpp"

class AbstractNodeView;
class ActionHandler;
class Animation;
class ExpandNode;
class TableNodeSortModel;
class PropertyMapper;
class TreeNodeModel;
class StandardNodeViewDelegategate;
//class TreeViewExpandState;
class VTreeNode;

//class TreeNodeView;
//typedef ExpandState<TreeNodeView> TreeViewExpandState;


class TreeNodeView : public QObject, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
    TreeNodeView(AbstractNodeView* view,TreeNodeModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
    ~TreeNodeView();

    void reload();
    void rerender();
    QWidget* realWidget();
    QObject* realObject();
    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
    void selectFirstServer();
    bool isNodeExpanded(const QModelIndex&) const;

    void notifyChange(VProperty* p);

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
    void slotContextMenu(const QPoint &position);
    void slotViewCommand(VInfo_ptr,QString);
    void slotSaveExpand();
    void slotRestoreExpand();
    void slotSaveExpand(const VTreeNode* node);
    void slotRestoreExpand(const VTreeNode* node);
    void slotRepaint(Animation*);
    void slotRerender();
    void slotSizeHintChangedGlobal();

protected Q_SLOTS:
    void slotDoubleClickItem(const QModelIndex&);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

Q_SIGNALS:
    void selectionChanged(VInfo_ptr);
    void infoPanelCommand(VInfo_ptr,QString);
    void dashboardCommand(VInfo_ptr,QString);

protected:
    QModelIndexList selectedList();
    void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

    void saveExpand(ExpandNode *parentExpand,const QModelIndex& idx);
    void restoreExpand(ExpandNode *expand,const VNode* node);
    void expandTo(const QModelIndex& idxTo);
    void setCurrentSelectionFromExpand(VInfo_ptr info);
    void regainSelectionFromExpand();

    void adjustBackground(QColor col);
    void adjustBranchLineColour(QColor col);
    void adjustServerToolTip(bool);
    void adjustNodeToolTip(bool);
    void adjustAttributeToolTip(bool);

    AbstractNodeView *view_;
    TreeNodeModel* model_;
    ActionHandler* actionHandler_;
    bool needItemsLayout_;
    int defaultIndentation_;
    PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpand_;
    VInfo_ptr lastSelection_;

    //typedef ExpandState<CompactNodeView> CompactViewExpandState;
    //QVector<CompactViewExpandState*> expandStates_;
    bool inStartUp_;
};





#if 0


class TreeNodeView : public QTreeView, public NodeViewBase, public VPropertyObserver
{
Q_OBJECT

public:
    explicit TreeNodeView(TreeNodeModel* model,NodeFilterDef* filterDef,QWidget *parent=0);
	~TreeNodeView();

	void reload();
	void rerender();
	QWidget* realWidget();
    VInfo_ptr currentSelection();
    void setCurrentSelection(VInfo_ptr n);
	void selectFirstServer();
    bool isNodeExpanded(const QModelIndex& idx) const;

#if 0
    void setModel(NodeFilterModel* model);
#endif
	void notifyChange(VProperty* p);

    void readSettings(VSettings* vs) {}
    void writeSettings(VSettings* vs) {}

public Q_SLOTS:
	void slotDoubleClickItem(const QModelIndex&);
	void slotContextMenu(const QPoint &position);
	void slotViewCommand(VInfo_ptr,QString);
    void slotSaveExpand();
    void slotRestoreExpand();
    void slotSaveExpand(const VTreeNode* node);
    void slotRestoreExpand(const VTreeNode* node);
	void slotRepaint(Animation*);
	void slotRerender();
	void slotSizeHintChangedGlobal();

protected Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

Q_SIGNALS:
	void selectionChanged(VInfo_ptr);
	void infoPanelCommand(VInfo_ptr,QString);
	void dashboardCommand(VInfo_ptr,QString);

protected:
    void resizeEvent(QResizeEvent*);
    QModelIndexList selectedList();
	void handleContextMenu(QModelIndex indexClicked,QModelIndexList indexLst,QPoint globalPos,QPoint widgetPos,QWidget *widget);

	void adjustIndentation(int);
    void adjustBackground(QColor col,bool asjustStyleSheet=true);
    void adjustBranchLines(bool,bool asjustStyleSheet=true);
    void adjustStyleSheet();
    void adjustServerToolTip(bool);
    void adjustNodeToolTip(bool);
    void adjustAttributeToolTip(bool);

    void expandAll(const QModelIndex& idx);
	void collapseAll(const QModelIndex& idx);
    void expandTo(const QModelIndex& idxTo);
    void setCurrentSelectionFromExpand(VInfo_ptr info);
    void regainSelectionFromExpand();

    AbstractNodeView* view_;
    TreeNodeModel* model_;
	ActionHandler* actionHandler_;
    bool needItemsLayout_;
	int defaultIndentation_;
	TreeNodeViewDelegate* delegate_;
	PropertyMapper* prop_;
    QMap<QString,QString> styleSheet_;
    bool setCurrentIsRunning_;
    bool setCurrentFromExpand_;
    VInfo_ptr lastSelection_;

    //ypedef ExpandState<TreeNodeView> TreeViewExpandState;
    //QVector<TreeViewExpandState*> expandStates_;
};
#endif

#endif



