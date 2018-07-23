//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AttributeSearchPanel.hpp"

#include "CaseSensitiveButton.hpp"
#include "NodeQuery.hpp"
#include "NodeQueryOptionEdit.hpp"
#include "StringMatchCombo.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVariant>

#include <assert.h>

//======================================================
//
// AttributeLineDesc
//
//======================================================

class AttrLineDesc
{
public:
	AttrLineDesc(QString name,int row) : name_(name), row_(row) {}
	virtual ~AttrLineDesc() {}

	virtual QString value()=0;
	QString name()  const {return name_;}
	int row() const {return row_;}

protected:
	QString name_;
	int row_;
};

class AttrLineStringDesc : public  AttrLineDesc
{
public:
	AttrLineStringDesc(QString name,int row,QLineEdit *le) : AttrLineDesc(name,row), le_(le) {}

	QString value() {return le_->text();}

protected:
	QLineEdit* le_;
};


//======================================================
//
// AttributeGroupDesc
//
//======================================================

class AttrGroupDesc
{
public:
	AttrGroupDesc(QString name,QGridLayout* grid) : name_(name), grid_(grid) {}

#if 0
    QString query() const;
#endif
	void hide();
	void show();
	void add(AttrLineDesc* line) {lines_ << line;}

protected:
	QString name_;
	QList<AttrLineDesc*> lines_;
	QGridLayout* grid_;
};

void AttrGroupDesc::hide()
{
	Q_FOREACH(AttrLineDesc* item,lines_)
	{
		int row=item->row();
		for(int i=0; i < grid_->columnCount(); i++)
		{
			if(QLayoutItem *li=grid_->itemAtPosition(row,i))
			{
				if(li->widget())
				{
					li->widget()->hide();
				}
			}
		}
    }
}

void AttrGroupDesc::show()
{
	Q_FOREACH(AttrLineDesc* item,lines_)
	{
		int row=item->row();
		for(int i=0; i < grid_->columnCount(); i++)
		{
			if(QLayoutItem *li=grid_->itemAtPosition(row,i))
			{
				if(li->widget())
				{
					li->widget()->show();
				}
			}
		}
	}
}

//======================================================
//
// AttributeSearchPanel
//
//======================================================

AttributeSearchPanel::AttributeSearchPanel(QWidget* parent) :
	QWidget(parent),
	query_(NULL)
{
	//setupUi(this);
}

AttributeSearchPanel::~AttributeSearchPanel()
{
}

void AttributeSearchPanel::setQuery(NodeQuery* query)
{
	query_=query;
}

void AttributeSearchPanel::setSelection(QStringList lst)
{
	if(lst == selection_)
		return;

    QMapIterator<QString,QList<NodeQueryOptionEdit*> > it(groups_);
    while (it.hasNext())
    {
        it.next();
        bool inNew=lst.contains(it.key());
        bool inCurrent=selection_.contains(it.key());

        bool st=false;
        if(inNew && !inCurrent)
        {
           st=true;
        }
        else if(!inNew && inCurrent)
        {
           st=false;
        }

        Q_FOREACH(NodeQueryOptionEdit* e,it.value())
        {
            e->setVisible(st);
        }

    }

    if(lst.isEmpty())
        hide();
    else
        show();

    selection_=lst;

    buildQuery();
}

void AttributeSearchPanel::clearSelection()
{
	setSelection(QStringList());
}

void AttributeSearchPanel::slotOptionEditChanged()
{

}

void AttributeSearchPanel::buildQuery()
{
	/*query_.clear();

	QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		QString name=it.key();
		if(selection_.contains(name))
		{
			if(!query_.isEmpty())
				query_+=" or ";
			query_+=it.value()->query();
		}
	}

	Q_EMIT queryChanged();*/
}

QStringList AttributeSearchPanel::groupNames() const
{
	return groups_.keys();
}

void AttributeSearchPanel::init()
{
}

