//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTFILTERWIDGET_HPP
#define TEXTFILTERWIDGET_HPP

#include "ui_TextFilterWidget.h"

#include <QWidget>
#include <QAbstractItemModel>
#include <QCompleter>

#include "TextFilterHandler.hpp"

class TextFilterCompleterModel;

class TextFilterWidget : public QWidget, private Ui::TextFilterWidget
{
Q_OBJECT

public:
    explicit TextFilterWidget(QWidget *parent=0);
    ~TextFilterWidget() {}

public Q_SLOTS:
    void slotFilterEditor();
    void on_runTb__clicked();
    void on_confTb__clicked();
    void on_closeTb__clicked();

Q_SIGNALS:
    void runRequested(QString);


private:
    QCompleter* completer_;
    TextFilterCompleterModel* completerModel_;
};

class TextFilterCompleterModel : public QAbstractItemModel
{
public:
    explicit TextFilterCompleterModel(QObject *parent=0);
    ~TextFilterCompleterModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(const std::vector<TextFilterItem>&);
    //bool updateData(const std::vector<Zombie>&);
    //void clearData();
    bool hasData() const;
    //Zombie indexToZombie(const QModelIndex&) const;

protected:
    std::vector<TextFilterItem> data_;
};

#endif // TEXTFILTERWIDGET_HPP
