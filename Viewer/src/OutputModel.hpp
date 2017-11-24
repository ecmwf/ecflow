#ifndef OUTPUTMODEL_H
#define OUTPUTMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include <vector>

#include "NodeObserver.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"

class OutputModel : public QAbstractItemModel
{
public:
	explicit OutputModel(QObject *parent=0);

    void setData(const std::vector<VDir_ptr>&,const std::string& jobout);
   	void clearData();
    bool isEmpty() const {return (!hasData());}
   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	//Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

   	std::string fullName(const QModelIndex& index) const;

protected:
    VDirItem* itemAt(int row,VDir_ptr& dir) const;
    bool hasData() const;
   	QString formatSize(unsigned int size) const;
   	QString formatDate(QDateTime) const;
   	QString formatAgo(QDateTime) const;

    std::vector<VDir_ptr> dirs_;
    int joboutRow_;
    static QColor joboutCol_;
};

//Filters and sorts the output
class OutputSortModel : public QSortFilterProxyModel
{
public:
	explicit OutputSortModel(QObject *parent=0);
    ~OutputSortModel() {}

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
	bool filterAcceptsRow(int,const QModelIndex &) const;

	QModelIndex fullNameToIndex(const std::string& fullName);
};

class OutputDirLitsDelegate : public QStyledItemDelegate
{
public:
    explicit OutputDirLitsDelegate(QWidget *parent=0);
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    //QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;

};

#endif
