//============================================================================
// Copyright 2016 ECMWF.
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

#include "ServerObserver.hpp"
#include "VInfo.hpp"

#include "ui_AttributeEditDialog.h"

class AttributeEditor : public QDialog, public ServerObserver, public VInfoObserver, protected Ui::AttributeEditDialog
{
Q_OBJECT

public:
    AttributeEditor(VInfo_ptr info,QWidget* parent);
    virtual ~AttributeEditor();
    //From VInfoObserver
    void notifyDelete(VInfo*) {}
    void notifyDataLost(VInfo*) {};

    //From ServerObserver
    void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) {};
    void notifyServerDelete(ServerHandler* server);
    void notifyBeginServerClear(ServerHandler* server);
    void notifyEndServerClear(ServerHandler* server) {}
    void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
    void notifyEndServerScan(ServerHandler* server);
    void notifyServerConnectState(ServerHandler* server) {};
    void notifyServerSuiteFilterChanged(ServerHandler* server) {};
    void notifyServerSyncFinished(ServerHandler* server) {};

    static void edit(VInfo_ptr info,QWidget *parent);

public Q_SLOTS:
    void accept();

protected:
    void attachInfo();
    void detachInfo();
    void addForm(QWidget* w);
    virtual void apply()=0;

    VInfo_ptr info_;
};

#endif // ATTRIBUTEEDITOR_HPP
