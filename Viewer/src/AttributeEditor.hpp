//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef ATTRIBUTEEDITOR_HPP
#define ATTRIBUTEEDITOR_HPP

#include <QDialog>

#include "NodeObserver.hpp"
#include "ServerObserver.hpp"
#include "VInfo.hpp"

#include "ui_AttributeEditorDialog.h"

class AttributeEditor : public QDialog, public ServerObserver,  public NodeObserver, public VInfoObserver, protected Ui::AttributeEditorDialog
{
Q_OBJECT

public:
    AttributeEditor(VInfo_ptr info,QString type,QWidget* parent);
    virtual ~AttributeEditor();

    //From VInfoObserver
    void notifyDelete(VInfo*) {}
    void notifyDataLost(VInfo*);

    //From NodeObserver
    void notifyBeginNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a,const VNodeChange&);
    void notifyEndNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a,const VNodeChange&) {}

    //From ServerObserver
    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a);
    void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server);
    void notifyEndServerClear(ServerHandler* server) {}
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
    void notifyEndServerScan(ServerHandler* server);
    void notifyServerConnectState(ServerHandler* server);
    void notifyServerSuiteFilterChanged(ServerHandler* server) {}
    void notifyServerSyncFinished(ServerHandler* server) {}

    static void edit(VInfo_ptr info,QWidget *parent);

public Q_SLOTS:
    void accept();
    void slotButton(QAbstractButton*);

protected:
    void attachInfo();
    void detachInfo();
    void checkButtonStatus();
    void setResetStatus(bool st);
    void setSuspended(bool);
    void addForm(QWidget* w);
    void hideForm();
    void doNotUseReset();
    void disableCancel();
    virtual void apply()=0;
    virtual void resetValue()=0;
    virtual bool isValueChanged()=0;

    VInfo_ptr info_;
    QStringList attrData_;
    QWidget* form_;
    QString type_;
};

#endif // ATTRIBUTEEDITOR_HPP
