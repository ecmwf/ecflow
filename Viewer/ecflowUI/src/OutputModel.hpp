/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_OutputModel_HPP
#define ecflow_viewer_OutputModel_HPP

#include <set>
#include <vector>

#include <QAbstractItemModel>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include "NodeObserver.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"

class OutputModel : public QAbstractItemModel {
public:
    explicit OutputModel(QObject* parent = nullptr);

    void resetData(const std::vector<VDir_ptr>&, const std::string& jobout);
    void clearData();
    bool isEmpty() const { return (!hasData()); }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    // Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex&) const override;

    std::string fullName(const QModelIndex& index) const;
    void itemDesc(const QModelIndex& index, std::string& itemFullName, VDir::FetchMode& mode) const;
    QModelIndex itemToIndex(const std::string& itemFullName, VDir::FetchMode fetchMode) const;
    QModelIndex itemToIndex(const std::string& itemFullName) const;

protected:
    VDirItem* itemAt(int row, VDir_ptr& dir) const;
    bool hasData() const;
    QString formatSize(unsigned int size) const;
    QString formatDate(QDateTime) const;
    QString formatAgo(QDateTime) const;
    qint64 secsToNow(QDateTime dt) const;

    std::vector<VDir_ptr> dirs_;
    std::set<int> joboutRows_;
    static QColor joboutCol_;
};

// Filters and sorts the output
class OutputSortModel : public QSortFilterProxyModel {
public:
    explicit OutputSortModel(QObject* parent = nullptr);
    ~OutputSortModel() override = default;

    QModelIndex fullNameToIndex(const std::string& fullName);
};

class OutputDirListDelegate : public QStyledItemDelegate {
public:
    explicit OutputDirListDelegate(QWidget* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

#endif /* ecflow_viewer_OutputModel_HPP */
