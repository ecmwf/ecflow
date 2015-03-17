//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VARIABLEITEMWIDGET_HPP_
#define VARIABLEITEMWIDGET_HPP_

#include "ui_VariableEditDialog.h"
#include "ui_VariableAddDialog.h"
#include "ui_VariableItemWidget.h"

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VariableModel;
class VariableModelData;
class VariableModelDataHandler;
class VariableSortModel;


class VariableDialogChecker
{
protected:
	VariableDialogChecker(QString txt) : errorText_(txt) {};

	bool checkName(QString name);
	bool checkValue(QString value);
	void error(QString msg);

	QString errorText_;
};


class VariableEditDialog : public QDialog, private Ui::VariableEditDialog //, public VariableDialogChecker
{
Q_OBJECT

public:
	VariableEditDialog(QString name,QString value,bool genVar,QWidget* parent=0);

	QString name() const;
	QString value() const;

public Q_SLOTS:
	void accept();

protected:
	bool genVar_;

};

class VariableAddDialog : public QDialog, private Ui::VariableAddDialog //, public VariableDialogChecker
{
Q_OBJECT

public:
	VariableAddDialog(VariableModelData* data,QWidget* parent=0);

	QString name() const;
	QString value() const;

public Q_SLOTS:
	void accept();

protected:
	VariableModelData* data_;
};




class VariableItemWidget : public QWidget, public InfoPanelItem, protected Ui::VariableItemWidget
{
Q_OBJECT

public:
	VariableItemWidget(QWidget *parent=0);
	~VariableItemWidget();

	void reload(VInfo_ptr);
	QWidget* realWidget();
	void clearContents();

public Q_SLOTS:
	void on_actionEdit_triggered();
	void on_actionAdd_triggered();
	void on_actionDuplicate_triggered();
	void on_actionDelete_triggered();
	void on_varView_doubleClicked(const QModelIndex& index);
	void on_filterTb_toggled(bool);
	void on_filterLine_textChanged(QString text);
	void slotItemSelected(const QModelIndex& idx);

protected:
	void checkActionState();
	void editItem(const QModelIndex& index);
	void duplicateItem(const QModelIndex& index);
	void addItem(const QModelIndex& index);
	void removeItem(const QModelIndex& index);

	void nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&);

	VariableModelDataHandler* data_;
	VariableModel* model_;
	VariableSortModel* sortModel_;
};

#endif

