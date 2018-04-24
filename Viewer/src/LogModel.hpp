//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include <vector>

class LogModelLine
{
public:
	explicit LogModelLine(QString);

	enum Type {NoType,MessageType,ErrorType,LogType,WarningType,DebugType};

	QString date_;
	QString entry_;
	Type type_;
};


class LogModel : public QAbstractItemModel
{
public:
   	explicit LogModel(QObject *parent=0);
   	~LogModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void setData(const std::string&);
	void setData(const std::vector<std::string>&);
	void appendData(const std::vector<std::string>&);
	bool hasData() const;
    void clearData();
    QModelIndex lastIndex() const;

    QString entryText(const QModelIndex&) const;
    QString fullText(const QModelIndex&) const;

protected:
	QList<LogModelLine> data_;
};

class LogDelegate : public QStyledItemDelegate
{
public:
    explicit LogDelegate(QWidget *parent=0);
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};



#endif
