//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODEQUERYEDITOR_HPP_
#define NODEQUERYEDITOR_HPP_

#include "ui_NodeQueryEditor.h"
#include "ui_NodeQuerySaveDialog.h"

#include <QAbstractItemModel>
#include <QDialog>
#include <QWidget>

#include "ServerFilter.hpp"
#include "VInfo.hpp"

class NodeQuery;
class NodeQueryDef;
class NodeQueryListModel;
class NodeQueryOptionEdit;

class NodeQuerySaveDialog : public QDialog, protected Ui::NodeQuerySaveDialog
{
Q_OBJECT

public:
    explicit NodeQuerySaveDialog(QWidget *parent = 0);
    ~NodeQuerySaveDialog() {}
    QString name() const;

public Q_SLOTS:
	void accept();
};

class NodeQueryEditor : public QWidget, protected Ui::NodeQueryEditor, public ServerFilterObserver
{
    Q_OBJECT

public:
    explicit NodeQueryEditor(QWidget *parent = 0);
    ~NodeQueryEditor();

    void setServerFilter(ServerFilter*);
    void setRootNode(VInfo_ptr);
    void setQuery(NodeQuery*);
    NodeQuery* query() const;
    void setQueryTeCanExpand(bool);
    bool isDefPanelVisible() const;
    void showDefPanel(bool);
    bool isQueryPanelVisible() const;
    void showQueryPanel(bool);
    int maxNum() const;
    QStringList allServers() const;
    void setFilterMode(bool);

    void notifyServerFilterAdded(ServerItem*);
    void notifyServerFilterRemoved(ServerItem*);
    void notifyServerFilterChanged(ServerItem*);
    void notifyServerFilterDelete();

public Q_SLOTS:
    void slotClear();

protected Q_SLOTS:
    void slotOptionEditChanged();
    void slotServerCbChanged();
	void slotRootNodeEdited(QString);
    void slotAttrPanelChanged();
	void slotSaveQueryAs();
	void slotAdvMode(bool b);
	void slotMaxNum(int);
    void slotCase(bool);

Q_SIGNALS:
	void queryEnabledChanged(bool);

private:
	void updateServers();
	void init();
	void initAttr();
	void updateQueryTe();
	void checkGuiState();
    void setAttributePanel(QStringList lst);

	NodeQuery* query_;
	ServerFilter* serverFilter_;
	bool queryTeCanExpand_;
	bool initIsOn_;
	bool canBeRun_;
    bool filterMode_;

    NodeQueryOptionEdit* nameEdit_;
    NodeQueryOptionEdit* pathEdit_;
    NodeQueryOptionEdit* typeEdit_;
    NodeQueryOptionEdit* stateEdit_;
    NodeQueryOptionEdit* flagEdit_;
    NodeQueryOptionEdit* periodEdit_;
    NodeQueryOptionEdit* attrEdit_;
    QMap<QString,QList<NodeQueryOptionEdit*> > attr_;
    QString nodeTabText_;
    QString attrTabText_;
    QString periodTabText_;
};

#endif
