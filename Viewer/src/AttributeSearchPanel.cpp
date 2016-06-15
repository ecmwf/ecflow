//============================================================================
// Copyright 2016 ECMWF.
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


#if 0
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
#endif

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

	QVBoxLayout* vb=new QVBoxLayout;
	setLayout(vb);
    vb->setContentsMargins(0,0,0,0);

	grid_=new QGridLayout();
	vb->addLayout(grid_);
    vb->addStretch(1);

            //Build groups
	//QStringList attGroupNames;
	//attGroupNames << "date" << "event" << "label" << "late" << "limit" << "limiter" << "meter"
	//    	   << "repeat" << "time" << "trigger" << "variable";

//TODO

    Q_FOREACH(NodeQueryAttrDef* aDef,NodeQuery::attrDef().values())
    {
        Q_ASSERT(aDef);
        QString grName=aDef->name();
        //AttrGroupDesc* gr=new AttrGroupDesc(grName,grid_);
        //groups_[grName]=gr;

        Q_FOREACH(NodeQueryDef* d,aDef->defs())
        {
            NodeQueryOptionEdit *e=new NodeQueryStringOptionEdit(d->name(),grid_,this);
            groups_[grName] << e;

#if 0
            AttrLineStringDesc* line=new AttrLineStringDesc(text,row,le);

            QMap<QString,AttrGroupDesc*>::iterator it=groups_.find(group);
            assert(it != groups_.end());
            it.value()->add(line);
#endif
            e->setVisible(false);

            //addStringLine(d->label(),d->name(),grName);
        }
    }

	/*

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
*/
	//Initialise lines

#if 0
    QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		it.value()->hide();
	}
	hide();
#endif
}

AttributeSearchPanel::~AttributeSearchPanel()
{
#if 0
    QMapIterator<QString,AttrGroupDesc*> it(groups_);
	while (it.hasNext())
	{
		it.next();
		delete it.value();
	}
#endif
}

void AttributeSearchPanel::setQuery(NodeQuery* query)
{
	query_=query;
}

#if 0
void AttributeSearchPanel::addStringLine(QString labelTxt,QString text,QString group)
{
	QLabel *label=new QLabel(labelTxt + ":",this);
	QLineEdit* le=new QLineEdit(this);
	StringMatchCombo *matchCb=new StringMatchCombo(this);
#if 0
    CaseSensitiveButton* caseTb=new CaseSensitiveButton(this);
#endif
	le->setProperty("id",text);
	matchCb->setProperty("id",text);

#if 0
    caseTb->setProperty("id",text);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    le->setClearButtonEnabled(true);
#endif

	int row=grid_->rowCount();
	grid_->addWidget(label,row,0);
    grid_->addWidget(matchCb,row,1);
    grid_->addWidget(le,row,2);
#if 0
    grid_->addWidget(caseTb,row,3);
#endif
	connect(le,SIGNAL(textEdited(QString)),
			this,SLOT(slotTextEdited(QString)));

	connect(matchCb,SIGNAL(currentIndexChanged(int)),
			this,SLOT(slotMatchChanged(int)));

#if 0
	connect(caseTb,SIGNAL(changed(bool)),
			this,SLOT(slotCaseChanged(bool)));
#endif

	AttrLineStringDesc* line=new AttrLineStringDesc(text,row,le);

	QMap<QString,AttrGroupDesc*>::iterator it=groups_.find(group);
	assert(it != groups_.end());
	it.value()->add(line);
}
#endif

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

#if 0
    QMapIterator<QString,QList<NodeQueryOptionEdit*> >> it(groups_);
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
#endif

}

void AttributeSearchPanel::clearSelection()
{
	setSelection(QStringList());
}

void AttributeSearchPanel::slotOptionEditChanged()
{

}


#if 0
void AttributeSearchPanel::slotTextEdited(QString val)
{
	if(QLineEdit *le=static_cast<QLineEdit*>(sender()))
	{
#if 0
        QString id=le->property("id").toString();
		if(NodeQueryStringOption *op=query_->stringOption(id))
		{
			op->setValue(val);
		}
#endif
         Q_EMIT queryChanged();
	}

	buildQuery();
}

void AttributeSearchPanel::slotMatchChanged(int val)
{
	if(StringMatchCombo *cb=static_cast<StringMatchCombo*>(sender()))
	{
#if 0
        QString id=cb->property("id").toString();
		if(NodeQueryStringOption *op=query_->stringOption(id))
		{
			op->setMatchMode(cb->currentMatchMode());
        }
#endif
        Q_EMIT queryChanged();
	}

	buildQuery();
}
#endif



#if 0
void AttributeSearchPanel::slotCaseChanged(bool val)
{
	if(CaseSensitiveButton *tb=static_cast<CaseSensitiveButton*>(sender()))
	{
		QString id=tb->property("id").toString();
		if(NodeQueryStringOption *op=query_->stringOption(id))
		{
			op->setCaseSensitive(val);
		}
	}

	buildQuery();
}
#endif

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

