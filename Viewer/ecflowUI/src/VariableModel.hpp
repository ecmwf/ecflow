#ifndef VARIABLEMODEL_H
#define VARIABLEMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <vector>

#include "NodeObserver.hpp"
#include "VInfo.hpp"

class Node;
class VariableModelData;
class VariableModelDataHandler;
class VariableSortModel;

class VariableModel : public QAbstractItemModel
{
Q_OBJECT

friend class VariableSortModel;

public:
   	VariableModel(VariableModelDataHandler* data,QObject *parent=nullptr);

    enum CustomItemRole {ReadOnlyRole = Qt::UserRole+1,GenVarRole = Qt::UserRole+2,ShadowRole = Qt::UserRole+3};

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	Qt::ItemFlags flags ( const QModelIndex & index) const override;
   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;

	bool variable(const QModelIndex& index, QString& name,QString& value,bool& genVar) const;

    VariableModelData* indexToData(const QModelIndex& index) const;
    VariableModelData* indexToData(const QModelIndex& index,int& block) const;
    VInfo_ptr indexToInfo(const QModelIndex& index) const;
    QModelIndex infoToIndex(VInfo_ptr info) const;

	bool isVariable(const QModelIndex & index) const;

public Q_SLOTS:
	void slotReloadBegin();
	void slotReloadEnd();
    void slotClearBegin(int block,int num);
    void slotClearEnd(int block,int num);
    void slotLoadBegin(int block,int num);
    void slotLoadEnd(int block,int num);
    void slotDataChanged(int);

Q_SIGNALS:
    void filterChanged();
    void rerunFilter();

protected:
	bool hasData() const;

	int indexToLevel(const QModelIndex&) const;
    void identify(const QModelIndex& index,int& parent,int& row) const;

	VariableModelDataHandler* data_;
	static QColor varCol_;
    static QColor genVarCol_;
    static QColor shadowCol_;
	static QColor blockBgCol_;
	static QColor blockFgCol_;
};


//Filter and sorts the variables

class VariableSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
	enum MatchMode {FilterMode,SearchMode};

	VariableSortModel(VariableModel*,QObject *parent=nullptr);
    ~VariableSortModel() override = default;

	MatchMode matchMode() const {return matchMode_;}
	void setMatchMode(MatchMode mode);
	void setMatchText(QString text);

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
	bool filterAcceptsRow(int,const QModelIndex &) const override;

	//From QSortFilterProxyModel:
	//we set the source model in the constructor. So this function should not do anything.
    void setSourceModel(QAbstractItemModel*) override {}
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
	
    QModelIndexList match(const QModelIndex& start,int role,const QVariant& value,int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap )) const override;

#if 0
    void test();
    void test(const QModelIndex& p);
    void testSource(const QModelIndex& p);
#endif

public Q_SLOTS:
    void slotShowShadowed(bool);

protected Q_SLOTS:
    void slotFilterChanged();
    void slotRerunFilter();

protected:
    void match(QString text);
    void print(const QModelIndex idx);

    VariableModel* varModel_;
    bool showShadowed_;

	MatchMode matchMode_;
	mutable QString matchText_;
    mutable QModelIndexList matchLst_;

    QMap<QString,int> nameCnt_;
    bool ignoreDuplicateNames_; //Ignore duplicate names across ancestors
};


#endif
