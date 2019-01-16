//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_CHANGENOTIFYEDITOR_HPP_
#define VIEWER_SRC_CHANGENOTIFYEDITOR_HPP_

#include <QWidget>
#include <QAbstractItemModel>

#include <vector>
#include "VProperty.hpp"

#include "ui_ChangeNotifyEditor.h"

class QGridlayout;

class ChangeNotifyEditorModel;
class PropertyLine;

class ChangeNotifyEditor : public QWidget, protected Ui::ChangeNotifyEditor
{
	Q_OBJECT

public:
	explicit ChangeNotifyEditor(QWidget* parent=0);
	void addRow(QString,QList<PropertyLine*>,QWidget*);

protected Q_SLOTS:
	void slotRowSelected(const QModelIndex& idx);

private:
	ChangeNotifyEditorModel* model_;
};


class ChangeNotifyEditorModelData
{
public:
	ChangeNotifyEditorModelData() : enabled_(NULL), enabledMaster_(false), enabledVal_(false) {}

	QString label_;
	QString desc_;
	VProperty* enabled_;
	bool enabledMaster_;
	bool enabledVal_;
};


class ChangeNotifyEditorModel : public QAbstractItemModel
{
	Q_OBJECT

public:
   	explicit ChangeNotifyEditorModel(QObject *parent=0);
   	~ChangeNotifyEditorModel();

   	void add(QString,QList<VProperty*>);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	bool setData(const QModelIndex & index, const QVariant& value, int role = Qt::EditRole);
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;
   	Qt::ItemFlags flags ( const QModelIndex & index) const;

public Q_SLOTS:
   	void slotEnabledChanged(QVariant v);
   	void slotEnabledMasterChanged(bool b);

Q_SIGNALS:
	void enabledChanged(VProperty*,QVariant);

protected:
	int lineToRow(PropertyLine* line) const;

	QList<ChangeNotifyEditorModelData> data_;
};


#endif /* VIEWER_SRC_CHANGENOTIFYEDITOR_HPP_ */
