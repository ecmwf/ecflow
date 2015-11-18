//============================================================================
// Copyright 2015 ECMWF.
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
class NodeQueryListModel;

class NodeQuerySaveDialog : public QDialog, protected Ui::NodeQuerySaveDialog
{
Q_OBJECT

public:
    explicit NodeQuerySaveDialog(QWidget *parent = 0);
    ~NodeQuerySaveDialog() {};
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
    void toggleDefPanelVisible();
    bool isDefPanelVisible() const;
    int maxNum() const;

    void notifyServerFilterAdded(ServerItem*);
    void notifyServerFilterRemoved(ServerItem*);
    void notifyServerFilterChanged(ServerItem*);
    void notifyServerFilterDelete();

protected Q_SLOTS:
	void slotServerCbChanged();
	void slotRootNodeEdited(QString);
	void slotNameEdited(QString);
	void slotNameMatchChanged(int);
	void slotNameCaseChanged(bool);
	void slotPathEdited(QString);
	void slotPathMatchChanged(int);
	void slotPathCaseChanged(bool);
	void slotTypeListChanged();
	void slotStateListChanged();
	void slotFlagListChanged();
	void slotAttrListChanged();
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
	void adjustQueryTe(int rowNum=0);
	void setQueryTe(QString);
	void checkGuiState();

	NodeQuery* query_;
	ServerFilter* serverFilter_;
	bool queryTeCanExpand_;
	bool initIsOn_;
	bool canBeRun_;
};

#endif
