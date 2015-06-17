//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef PROPERTYDIALOG_INC_
#define PROPERTYDIALOG_INC_

#include "ui_PropertyDialog.h"

#include <QDialog>

class PropertyEditor;
class VProperty;

class PropertyDialog : public QDialog, private Ui::PropertyDialog
{

Q_OBJECT    
    
public:
    PropertyDialog(QWidget *parent=0);
    ~PropertyDialog() {};

    bool configChanged() const {return configChanged_;}

    //Called from VConfigLoader
    static void load(VProperty*);

public Q_SLOTS:
    void accept();
    void slotChangePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void build();
    void addPage(QWidget *w,QIcon icon,QString txt);
    void manageChange();

    QList<PropertyEditor*> editors_;
    bool configChanged_;

    static VProperty* prop_;

};

#endif

