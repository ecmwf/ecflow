//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "AttributeSearchPanel.hpp"

#include <QtGlobal>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

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

	QString query() const;
	void hide();
	void show();
	void add(AttrLineDesc* line) {lines_ << line;}

protected:
	QString name_;
	QList<AttrLineDesc*> lines_;
	QGridLayout* grid_;
};

QString AttrGroupDesc::query() const
{
	QString q;
	Q_FOREACH(AttrLineDesc* line, lines_)
	{
		QString s=line->value();
		if(!s.isEmpty())
		{
			if(!q.isEmpty())
				q+=" or ";
			q+=line->name() + " =\'" + s + "\'";
		}
	}

	if(q.isEmpty())
		q=name_;

	return q;
}

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
	QWidget(parent)
{
	//setupUi(this);

	QVBoxLayout* vb=new QVBoxLayout;
	setLayout(vb);

	grid_=new QGridLayout();
	vb->addLayout(grid_);

	//Build groups
	QStringList attGroupNames;
	attGroupNames << "date" << "event" << "label" << "late" << "limit" << "limiter" << "meter"
	    	   << "repeat" << "time" << "trigger" << "variable";

	Q_FOREACH(QString grName,attGroupNames)
	{
		AttrGroupDesc* gr=new AttrGroupDesc(grName,grid_);
		groups_[grName]=gr;
	}

	//Populate groups
	addStringLine("Date name","date_name","date");
	addStringLine("Event name","event_name","event");
	//addStringLine("Date name","date_name","date");
	addStringLine("Label name","label_name","label");
	addStringLine("Label value","label_value","label");
	addStringLine("Late name","late_name","late");
	addStringLine("Limit name","limit_name","limit");
	addStringLine("Limit value","limit_value","limit");
	addStringLine("Limit max","limit_max","limit");
	addStringLine("Limiter name","limiter_name","limiter");
	addStringLine("Meter name","meter_name","meter");
	addStringLine("Repeat name","repeat_name","repeat");
	addStringLine("Repeat value","repeat_value","repeat");
	addStringLine("Time name","time_name","time");
	addStringLine("Trigger expression","trigger_expression","trigger");
	addStringLine("Variable name","variable_name","variable");
	addStringLine("Variable value","variable_value","variable");

	//Initialise lines
	QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		it.value()->hide();
	}
	hide();
}

AttributeSearchPanel::~AttributeSearchPanel()
{
	QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		delete it.value();
	}
}

void AttributeSearchPanel::addStringLine(QString labelTxt,QString text,QString group)
{
	QLabel *label=new QLabel(labelTxt + ":",this);
	QLineEdit* le=new QLineEdit(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le->setClearButtonEnabled(true);
#endif

	int row=grid_->rowCount();
	grid_->addWidget(label,row,0);
	grid_->addWidget(le,row,1);

	connect(le,SIGNAL(textEdited(QString)),
			this,SLOT(slotTextEdited(QString)));

	AttrLineStringDesc* line=new AttrLineStringDesc(text,row,le);

	QMap<QString,AttrGroupDesc*>::iterator it=groups_.find(group);
	assert(it != groups_.end());
	it.value()->add(line);
}

void AttributeSearchPanel::setSelection(QStringList lst)
{
	if(lst == selection_)
		return;

	QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		bool inNew=lst.contains(it.key());
		bool inCurrent=selection_.contains(it.key());

		if(inNew && !inCurrent)
		{
		   it.value()->show();
		}
		else if(!inNew && inCurrent)
		{
		   it.value()->hide();
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

void AttributeSearchPanel::slotTextEdited(QString)
{
	buildQuery();
}

void AttributeSearchPanel::buildQuery()
{
	query_.clear();

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

	Q_EMIT queryChanged();
}

QStringList AttributeSearchPanel::groupNames() const
{
	return groups_.keys();
}
