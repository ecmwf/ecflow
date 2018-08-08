#ifndef SUITEMODEL_H
#define SUITEMODEL_H

#include <QAbstractItemModel>
#include <QColor>

#include "SuiteFilterObserver.hpp"

class ServerHandler;
class SuiteFilter;

class SuiteModel : public QAbstractItemModel, public SuiteFilterObserver
{
    Q_OBJECT
public:
   	explicit SuiteModel(QObject *parent=nullptr);
   	~SuiteModel();

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	Qt::ItemFlags flags ( const QModelIndex & index) const;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	bool setData(const QModelIndex & index, const QVariant & value, int role );
   	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

    void setData(SuiteFilter* filter,ServerHandler* server);
    void reloadData();
    void resetData();

   	SuiteFilter* filter() const {return data_;}   
    SuiteFilter* realFilter() const {return realData_;}
    void setEdited(bool);
    bool isEdited() const {return edited_;}

    void notifyChange(SuiteFilter*);
   	void notifyDelete(SuiteFilter*);

Q_SIGNALS:
    void dataUpdated();

protected:
	bool hasData() const;
	void clearData();
	void updateData();

    ServerHandler* server_;
	SuiteFilter* data_;
	SuiteFilter* realData_;
	QColor presentCol_;
	QColor notPresentCol_;
    bool edited_;
};

#endif
