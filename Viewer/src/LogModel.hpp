#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>



#include <vector>

class LogModelLine
{
public:
	LogModelLine(QString);

	enum Type {NoType,MessageType,ErrorType,LogType};

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
	bool hasData() const;
    void clearData();

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
