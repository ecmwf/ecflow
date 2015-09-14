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

PropertyLine* PropertyLineFactory::create(VProperty* p,bool addLabel,QWidget* w)
{
	if(!p)
		return 0;

	VProperty::Type t=p->type();
	std::map<VProperty::Type,PropertyLineFactory*>::iterator j = makers->find(t);
	if(j != makers->end())
		return (*j).second->make(p,addLabel,w);

	return 0;
}

//=========================================================================
//
// PropertyLine
//
//=========================================================================

PropertyLine::PropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) :
	QObject(parent),
	prop_(vProp),
	label_(0),
	suffixLabel_(0)
{
	if(addLabel)
		label_=new QLabel(vProp->param("label"),parent);

	QString suffixText=vProp->param("suffix");
	if(!suffixText.isEmpty())
	{
		suffixLabel_=new QLabel(suffixText);
	}

	defaultTb_= new QToolButton(parent);
	defaultTb_->setText("Default");
	defaultTb_->setToolTip(tr("Reset to default value"));
    defaultTb_->setIcon(QPixmap(":/viewer/reset_to_default.svg"));

	connect(defaultTb_,SIGNAL(clicked(bool)),
			this,SLOT(slotResetToDefault(bool)));
}

void PropertyLine::slotResetToDefault(bool)
{
	reset(prop_->defaultValue());
	checkState();
}

void PropertyLine::checkState()
{
	if(prop_->defaultValue() != currentValue())
		defaultTb_->setEnabled(true);
	else
		defaultTb_->setEnabled(false);
}

//=========================================================================
//
// StringPropertyLine
//
//=========================================================================

StringPropertyLine::StringPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

	le_=new QLineEdit(parent);

	connect(le_,SIGNAL(textEdited(QString)),
			this,SLOT(slotEdited(QString)));
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
	PropertyLine::checkState();
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

QVariant StringPropertyLine::currentValue()
{
	return le_->text();
}

void StringPropertyLine::slotEdited(QString)
{
	PropertyLine::checkState();
}

//=========================================================================
//
// ColourPropertyLine
//
//=========================================================================

ColourPropertyLine::ColourPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

	QFont f;
	QFontMetrics fm(f);
	int height=fm.height();
	int width=fm.width("AAAAAAA");

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
	QColor c=v.value<QColor>();

	QString sh("QToolButton{background: rgb(" + QString::number(c.red()) + "," +
			QString::number(c.green()) + "," + QString::number(c.blue()) + ");}");
	cb_->setStyleSheet(sh);

	currentCol_=c;

	PropertyLine::checkState();
}

void ColourPropertyLine::slotEdit(bool)
{
	QColor currentCol=currentValue().value<QColor>();
	QColor col=QColorDialog::getColor(currentCol,cb_->parentWidget());

	if(col.isValid())
	{
		reset(col);
	}
}

bool ColourPropertyLine::applyChange()
{
	QColor v=prop_->value().value<QColor>();
	QColor c=currentValue().value<QColor>();

	if(v != c)
	{
		prop_->setValue(c);
		return true;
	}

	return false;
}

QVariant ColourPropertyLine::currentValue()
{
	return currentCol_;
}

//=========================================================================
//
// FontPropertyLine
//
//=========================================================================

FontPropertyLine::FontPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

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
	font_=v.value<QFont>();
	lName_->setText(font_.toString());
	PropertyLine::checkState();
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

QVariant FontPropertyLine::currentValue()
{
	return font_;
}

//=========================================================================
//
// IntPropertyLine
//
//=========================================================================

IntPropertyLine::IntPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

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

	connect(le_,SIGNAL(textEdited(QString)),
			this,SLOT(slotEdited(QString)));
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
	PropertyLine::checkState();
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

QVariant IntPropertyLine::currentValue()
{
	return le_->text().toInt();
}

void IntPropertyLine::slotEdited(QString)
{
	PropertyLine::checkState();
}

//=========================================================================
//
// BoolPropertyLine
//
//=========================================================================

BoolPropertyLine::BoolPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,false,parent)
{
	cb_=new QCheckBox(vProp->param("label"));

	connect(cb_,SIGNAL(stateChanged(int)),
			   this,SLOT(slotStateChanged(int)));
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
	PropertyLine::checkState();
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

QVariant BoolPropertyLine::currentValue()
{
	return cb_->isChecked();
}

void BoolPropertyLine::slotStateChanged(int)
{
	PropertyLine::checkState();
}

static PropertyLineMaker<StringPropertyLine> makerStr(VProperty::StringType);
static PropertyLineMaker<ColourPropertyLine> makerCol(VProperty::ColourType);
static PropertyLineMaker<FontPropertyLine> makerFont(VProperty::FontType);
static PropertyLineMaker<IntPropertyLine> makerInt(VProperty::IntType);
static PropertyLineMaker<BoolPropertyLine> makerBool(VProperty::BoolType);
