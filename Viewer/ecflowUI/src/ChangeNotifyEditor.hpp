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
	explicit ChangeNotifyEditor(QWidget* parent=nullptr);
	void addRow(QString,QList<PropertyLine*>,QWidget*);

protected Q_SLOTS:
	void slotRowSelected(const QModelIndex& idx);

private:
	ChangeNotifyEditorModel* model_;
};


class ChangeNotifyEditorModelData
{
public:
	ChangeNotifyEditorModelData() = default;

	QString label_;
	QString desc_;
	VProperty* enabled_{nullptr};
	bool enabledMaster_{false};
	bool enabledVal_{false};
};


class ChangeNotifyEditorModel : public QAbstractItemModel
{
	Q_OBJECT

public:
   	explicit ChangeNotifyEditorModel(QObject *parent=nullptr);
   	~ChangeNotifyEditorModel() override;

   	void add(QString,QList<VProperty*>);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
   	bool setData(const QModelIndex & index, const QVariant& value, int role = Qt::EditRole) override;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;
   	Qt::ItemFlags flags ( const QModelIndex & index) const override;

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
