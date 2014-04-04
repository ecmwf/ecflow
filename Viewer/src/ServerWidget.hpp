//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERWIDGET_HPP_
#define SERVERWIDGET_HPP_

#include <QDialog>
#include <QWidget>

#include "ui_ServerWidget.h"

class ServerFilter;
class ServerListModel;

class ServerWidget : public QWidget, private Ui::ServerWidget
{
    Q_OBJECT

public:
    ServerWidget(ServerFilter *,QWidget *parent=0);
    void saveSelection();

protected slots:
	void on_editTb_clicked();
	void on_addTb_clicked();
	void on_deleteTb_clicked();
	void on_rescanTb_clicked();

private:
	ServerFilter* filter_;
	ServerListModel* model_;

};

class ServerDialog : public QDialog
{
   Q_OBJECT

public:
	ServerDialog(ServerFilter *,QWidget* parent=0);
	~ServerDialog();
	void setServerFilter(ServerFilter *filter);

public slots:
   void accept();
   void reject();

protected:
	void readSettings();
	void writeSettings();

	ServerWidget* serverWidget_;
};

#endif
