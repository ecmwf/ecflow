//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLOADITEMWIDGET_HPP
#define SERVERLOADITEMWIDGET_HPP

#include <QPlainTextEdit>

#include "InfoPanelItem.hpp"
#include "VInfo.hpp"

class VNode;
class LogLoadData;
class LogLoadWidget;

class ServerLoadItemWidget : public QWidget, public InfoPanelItem
{
public:
    explicit ServerLoadItemWidget(QWidget *parent=0);
    ~ServerLoadItemWidget();

    void reload(VInfo_ptr);
    QWidget* realWidget();
    void clearContents();

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) {}
    void defsChanged(const std::vector<ecf::Aspect::Type>&) {}

protected:
    void updateState(const ChangeFlags&);
    void serverSyncFinished();

private:
    void load();

    LogLoadWidget* w_;

    //ServerLoadView* view_;
    //Ui::ServerLoadItemWidget* ui_;
    //LogLoadData* data_;
    //LogLoadSuiteModel* suiteModel_;
    //QSortFilterProxyModel* suiteSortModel_;
};


#if 0
#include <QAbstractItemModel>
class LogLoadSuiteModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LogLoadSuiteModel(QObject *parent=0);
    ~LogLoadSuiteModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    bool setData(const QModelIndex& idx, const QVariant & value, int role );
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(LogLoadData* data);
    bool hasData() const;
    void clearData();

Q_SIGNALS:
    void checkStateChanged(int,bool);

protected:
    QString formatPrecentage(float perc) const;

    LogLoadData* data_;
};

#endif

#endif // SERVERLOADITEMWIDGET_HPP


