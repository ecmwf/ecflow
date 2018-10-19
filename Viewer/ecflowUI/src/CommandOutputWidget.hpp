//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef COMMANDOUTPUTWIDGET_HPP
#define COMMANDOUTPUTWIDGET_HPP

#include <QAbstractItemModel>
#include <QSettings>
#include <QWidget>

#include "ui_CommandOutputWidget.h"

#include "CommandOutput.hpp"

class ModelColumn;

class CommandOutputModel : public QAbstractItemModel
{
public:
    explicit  CommandOutputModel(QObject *parent=0);
    ~CommandOutputModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void dataIsAboutToChange();
    void dataChanged();
    bool updateData();
    bool hasData() const;
    CommandOutput_ptr indexToItem(const QModelIndex& idx) const;
    QModelIndex itemToStatusIndex(CommandOutput_ptr item) const;

protected:
    ModelColumn* columns_;
};

class CommandOutputWidget : public QWidget, protected Ui::CommandOutputWidget
{
Q_OBJECT

public:
    explicit CommandOutputWidget(QWidget *parent=0);
    ~CommandOutputWidget();

    void readSettings(QSettings&);
    void writeSettings(QSettings&);

protected Q_SLOTS:
    void slotItemSelected(const QModelIndex&,const QModelIndex&);
    void slotItemAddBegin();
    void slotItemAddEnd();
    void slotItemOutputAppend(CommandOutput_ptr,QString);
    void slotItemErrorAppend(CommandOutput_ptr,QString);
    void slotItemOutputReload(CommandOutput_ptr);
    void slotItemErrorReload(CommandOutput_ptr);
    void slotItemStatusChanged(CommandOutput_ptr);
    void on_searchTb__clicked();
    void on_gotoLineTb__clicked();
    void on_fontSizeUpTb__clicked();
    void on_fontSizeDownTb__clicked();

Q_SIGNALS:
    void editorFontSizeChanged();

protected:
    bool isCurrent(CommandOutput_ptr item);
    void loadItem(CommandOutput_ptr);
    void updateInfoLabel(CommandOutput_ptr);
    void removeSpacer();
    QString formatErrorText(QString txt);

    CommandOutputModel* model_;
    QColor errCol_;
};

#endif // COMMANDOUTPUTWIDGET_HPP
