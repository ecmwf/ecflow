//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "PropertyLine.hpp"

#include <QCheckBox>
#include <QColorDialog>
#include <QDebug>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QToolButton>


static std::map<VProperty::Type,PropertyLineFactory*>* makers = 0;

//=========================================================================
//
// PropertyLineFactory
//
//=========================================================================

PropertyLineFactory::PropertyLineFactory(VProperty::Type type)
{
	if(makers == 0)
		makers = new std::map<VProperty::Type,PropertyLineFactory*>;

	(*makers)[type] = this;
}

PropertyLineFactory::~PropertyLineFactory()
{
	// Not called
}

PropertyLine* PropertyLineFactory::create(VProperty* p,QWidget* w)
{
	if(!p)
		return 0;

	VProperty::Type t=p->type();
	std::map<VProperty::Type,PropertyLineFactory*>::iterator j = makers->find(t);
	if(j != makers->end())
		return (*j).second->make(p,w);

	return 0;
}

//=========================================================================
//
// PropertyLine
//
//=========================================================================

PropertyLine::PropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) :
	prop_(vProp),
	label_(0)
{
	if(addLabel)
		label_=new QLabel(vProp->param("label"),parent);
}

//=========================================================================
//
// StringPropertyLine
//
//=========================================================================

StringPropertyLine::StringPropertyLine(VProperty* vProp,QWidget * parent) : PropertyLine(vProp,parent)
{
	le_=new QLineEdit(parent);
}

QWidget* StringPropertyLine::item()
{
	return le_;
}

QWidget* StringPropertyLine::button()
{
	return NULL;
}

void StringPropertyLine::reset(QVariant v)
{
	le_->setText(v.toString());
}

bool StringPropertyLine::applyChange()
{
	QString v=prop_->value().toString();
	if(v != le_->text())
	{
		prop_->setValue(le_->text());
		return true;
	}
	return false;
}

//=========================================================================
//
// ColourPropertyLine
//
//=========================================================================

ColourPropertyLine::ColourPropertyLine(VProperty* vProp,QWidget * parent) : PropertyLine(vProp,true,parent)
{
	QFont f;
	QFontMetrics fm(f);
	int height=fm.height();
	int width=fm.width("AAAAAAAAAA");

	cb_=new QToolButton(parent);
	cb_->setAutoFillBackground(true);
    cb_->setFixedWidth(width);
    cb_->setFixedHeight(height+2);
    cb_->setToolTip(tr("Click to select a colour"));

	connect(cb_,SIGNAL(clicked(bool)),
			this,SLOT(slotEdit(bool)));
}

QWidget* ColourPropertyLine::item()
{
	return cb_;
}

QWidget* ColourPropertyLine::button()
{
	return NULL;
}

void ColourPropertyLine::reset(QVariant v)
{
	QPalette pal = cb_->palette();
	pal.setColor(QPalette::Window, v.value<QColor>());
	cb_->setPalette(pal);
}

void ColourPropertyLine::slotEdit(bool)
{
	QColor col=QColorDialog::getColor(cb_->palette().color(QPalette::Window),cb_->parentWidget());

	if(col.isValid())
	{
		reset(col);
	}
}

bool ColourPropertyLine::applyChange()
{
	QColor v=prop_->value().value<QColor>();
	QColor c=cb_->palette().color(QPalette::Window);

	if(v != c)
	{
		prop_->setValue(c);
		return true;
	}

	return false;
}

//=========================================================================
//
// FontPropertyLine
//
//=========================================================================

FontPropertyLine::FontPropertyLine(VProperty* vProp,QWidget * parent) : PropertyLine(vProp,true,parent)
{
	lName_=new QLabel(parent);

	tbEdit_=new QToolButton(parent);
	tbEdit_->setToolTip(tr("Edit"));

	connect(tbEdit_,SIGNAL(clicked(bool)),
			this,SLOT(slotEdit(bool)));
}

QWidget* FontPropertyLine::item()
{
	return lName_;
}

QWidget* FontPropertyLine::button()
{
	return tbEdit_;
}

void FontPropertyLine::reset(QVariant v)
{
	lName_->setText(font_.toString());
}

void FontPropertyLine::slotEdit(bool)
{
	QFont c;

	bool ok;
	QFont f = QFontDialog::getFont(&ok,c,lName_->parentWidget());

	if(ok)
	{
		lName_->setText(f.toString());
		font_=f;
	}
}

bool FontPropertyLine::applyChange()
{
	QFont v=prop_->value().value<QFont>();
	if(v != font_)
	{
		prop_->setValue(font_);
		return true;
	}
	return false;
}

//=========================================================================
//
// IntPropertyLine
//
//=========================================================================

IntPropertyLine::IntPropertyLine(VProperty* vProp,QWidget * parent) : PropertyLine(vProp,true,parent)
{
	le_=new QLineEdit(parent);
	QIntValidator* validator=new QIntValidator(le_);

	QString s=vProp->param("max");
	if(!s.isEmpty())
	{
		validator->setTop(s.toInt());
	}

	s=vProp->param("min");
	if(!s.isEmpty())
	{
			validator->setBottom(s.toInt());
	}

	le_->setValidator(validator);
}

QWidget* IntPropertyLine::item()
{
	return le_;
}

QWidget* IntPropertyLine::button()
{
	return NULL;
}

void IntPropertyLine::reset(QVariant v)
{
	le_->setText(QString::number(v.toInt()));
}

bool IntPropertyLine::applyChange()
{
	int v=prop_->value().toInt();
	int cv=le_->text().toInt();
	if(v != cv)
	{
		prop_->setValue(cv);
		return true;
	}
	return false;
}


//=========================================================================
//
// BoolPropertyLine
//
//=========================================================================

BoolPropertyLine::BoolPropertyLine(VProperty* vProp,QWidget * parent) : PropertyLine(vProp,false,parent)
{
	cb_=new QCheckBox(vProp->param("label"));
}

QWidget* BoolPropertyLine::item()
{
	return cb_;
}

QWidget* BoolPropertyLine::button()
{
	return NULL;
}

void BoolPropertyLine::reset(QVariant v)
{
	cb_->setChecked(v.toBool());
}

bool BoolPropertyLine::applyChange()
{
	int v=prop_->value().toBool();

	if(v != cb_->isChecked())
	{
		prop_->setValue(cb_->isChecked());
		return true;
	}
	return false;
}


static PropertyLineMaker<StringPropertyLine> makerStr(VProperty::StringType);
static PropertyLineMaker<ColourPropertyLine> makerCol(VProperty::ColourType);
static PropertyLineMaker<FontPropertyLine> makerFont(VProperty::FontType);
static PropertyLineMaker<IntPropertyLine> makerInt(VProperty::IntType);
static PropertyLineMaker<BoolPropertyLine> makerBool(VProperty::BoolType);

