//============================================================================
// Copyright 2009-2017 ECMWF.
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

class QAbstractButton;

class PropertyEditor;
class VProperty;

class PropertyDialog : public QDialog, private Ui::PropertyDialog
{

Q_OBJECT    
    
public:
    explicit PropertyDialog(QWidget *parent=nullptr);
    ~PropertyDialog() override = default;

    bool isConfigChanged() const {return configChanged_;}
    void showPage(QString);

    //Called from VConfigLoader
    static void load(VProperty*);

public Q_SLOTS:
    void accept() override;
    void reject() override;
    void slotChangePage(QListWidgetItem *current, QListWidgetItem *previous);
    void slotButton(QAbstractButton*);

Q_SIGNALS:
	void configChanged();

private:
    void build();
    void addPage(QWidget *w,QPixmap pix,QString txt);
    void manageChange(bool);
    void apply();

    void closeEvent(QCloseEvent * event) override;
    void readSettings();
    void writeSettings();

    QList<PropertyEditor*> editors_;
    bool configChanged_{false};

    static VProperty* prop_;

};

#endif

