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
#include <QComboBox>
#include <QColorDialog>
#include <QDebug>
#include <QFontDialog>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QToolButton>

#include <assert.h>

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
	suffixLabel_(0),
	defaultTb_(0),
	masterTb_(0),
	enabled_(true)
{
	oriVal_=prop_->value();

	if(addLabel)
		label_=new QLabel(prop_->param("label"),parent);

	QString suffixText=prop_->param("suffix");
	if(!suffixText.isEmpty())
	{
		suffixLabel_=new QLabel(suffixText);
	}

	defaultTb_= new QToolButton(parent);
	defaultTb_->setToolTip(tr("Reset to default value"));
    defaultTb_->setIcon(QPixmap(":/viewer/reset_to_default.svg"));
    defaultTb_->setAutoRaise(true);

    connect(defaultTb_,SIGNAL(clicked(bool)),
    	    this,SLOT(slotResetToDefault(bool)));

    if(prop_->master())
    {
    	masterTb_=new QToolButton(parent);
    	masterTb_->setCheckable(true);
    	masterTb_->setText("Use global");
    	masterTb_->setToolTip(tr("Use global server settings"));
    	masterTb_->setIcon(QPixmap(":/viewer/chain.svg"));
    	masterTb_->setAutoRaise(true);
    	masterTb_->setChecked(prop_->useMaster());

    	connect(masterTb_,SIGNAL(toggled(bool)),
    			this,SLOT(slotMaster(bool)));
    }

}

PropertyLine::~PropertyLine()
{
}

void PropertyLine::init()
{
	if(prop_->master())
	{
		if(masterTb_->isChecked() != prop_->useMaster())
			masterTb_->setChecked(prop_->useMaster());
		else
			slotMaster(prop_->useMaster());
	}
	else
	{
		slotReset(prop_->value());
	}
}

void PropertyLine::slotResetToDefault(bool)
{
	slotReset(prop_->defaultValue());
	checkState();
}

void PropertyLine::slotEnabled(VProperty*,QVariant v)
{
	if(enabled_ != v.toBool())
	{
		if(!masterTb_->isChecked())
		{
			enabled_=v.toBool();
			checkState();
		}
	}
}

void PropertyLine::checkState()
{
	if(label_)
	{
		label_->setEnabled(enabled_);
	}
	if(masterTb_)
	{
		masterTb_->setEnabled(enabled_);
	}
	if(suffixLabel_)
	{
		suffixLabel_->setEnabled(enabled_);
	}

	defaultTb_->setEnabled(enabled_);

	setEnabledEditable(enabled_);

	if(masterTb_->isChecked())
		return;

	if(enabled_)
	{
		if(prop_->defaultValue() != currentValue())
			defaultTb_->setEnabled(true);
		else
			defaultTb_->setEnabled(false);
	}
}

bool PropertyLine::applyMaster()
{
	if(masterTb_ && prop_->useMaster() != masterTb_->isChecked())
	{
		prop_->setUseMaster(masterTb_->isChecked());
		return true;
	}
	return false;
}


void PropertyLine::slotMaster(bool b)
{
	//prop_->setUseMaster(b);
	slotReset(prop_->master()->value());
	if(b)
	{
		defaultTb_->setEnabled(false);
		setEnabledEditable(false);
	}
	else
	{
		defaultTb_->setEnabled(true);
		checkState();
		setEnabledEditable(true);
	}
}

void PropertyLine::slotReset(VProperty* prop,QVariant v)
{
	if(prop == prop_)
		slotReset(v);
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

void StringPropertyLine::slotReset(QVariant v)
{
	le_->setText(v.toString());
	PropertyLine::checkState();
}

bool StringPropertyLine::applyChange()
{
	PropertyLine::applyMaster();

	QString v=oriVal_.toString();
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


void StringPropertyLine::setEnabledEditable(bool b)
{
	le_->setEnabled(b);
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
	//cb_->setAutoFillBackground(true);
    cb_->setFixedWidth(width);
    cb_->setFixedHeight(height+8);
    cb_->setIconSize(QSize(cb_->width()-8,cb_->height()-8));

    cb_->setToolTip(tr("Click to select a colour"));

    cb_->setProperty("colourTb","1");

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

void ColourPropertyLine::slotReset(QVariant v)
{
	QColor c=v.value<QColor>();

	QPixmap pix(cb_->iconSize());
	pix.fill(c);
	QPainter painter(&pix);
	painter.setPen(QColor(60,60,60));
	painter.drawLine(0,0,pix.width(),0);
	painter.drawLine(0,0,0,pix.height());
	painter.setPen(QColor(240,240,240));
	painter.drawLine(pix.width(),1,pix.width(),pix.height()-1);
	painter.drawLine(0,pix.height()-1,pix.width(),pix.height()-1);

	cb_->setIcon(pix);


	currentCol_=c;

	PropertyLine::checkState();
}

void ColourPropertyLine::slotEdit(bool)
{
	QColor currentCol=currentValue().value<QColor>();
	QColor col=QColorDialog::getColor(currentCol,cb_->parentWidget());

	if(col.isValid())
	{
		slotReset(col);
	}
}

bool ColourPropertyLine::applyChange()
{
	PropertyLine::applyMaster();

	QColor v=oriVal_.value<QColor>();
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

void ColourPropertyLine::setEnabledEditable(bool b)
{
	cb_->setEnabled(b);
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

void FontPropertyLine::slotReset(QVariant v)
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
	PropertyLine::applyMaster();

	if(oriVal_.value<QFont>() != font_)
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

void FontPropertyLine::setEnabledEditable(bool b)
{
	tbEdit_->setEnabled(b);
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

void IntPropertyLine::slotReset(QVariant v)
{
	le_->setText(QString::number(v.toInt()));
	PropertyLine::checkState();
}

bool IntPropertyLine::applyChange()
{
	PropertyLine::applyMaster();

	int cv=le_->text().toInt();
	if(oriVal_.toInt() != cv)
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

void IntPropertyLine::setEnabledEditable(bool b)
{
	le_->setEnabled(b);
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

void BoolPropertyLine::slotReset(QVariant v)
{
	cb_->setChecked(v.toBool());
	PropertyLine::checkState();
}

bool BoolPropertyLine::applyChange()
{
	PropertyLine::applyMaster();

	if(oriVal_.toBool() != cb_->isChecked())
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
	Q_EMIT changed(prop_,cb_->isChecked());
}

void BoolPropertyLine::setEnabledEditable(bool b)
{
	cb_->setEnabled(b);
}




//=========================================================================
//
// ComboPropertyLine
//
//=========================================================================

ComboPropertyLine::ComboPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent) : PropertyLine(vProp,addLabel,parent)
{
	cb_=new QComboBox(parent);//(vProp->param("label"));

	connect(cb_,SIGNAL(currentIndexChanged(int)),
			   this,SLOT(slotCurrentChanged(int)));

	QStringList lst=prop_->param("values_label").split("/");
    QStringList lstData=prop_->param("values").split("/");
    assert(lst.count() == lstData.count());
	for(int i=0; i < lst.count(); i++)
		cb_->addItem(lst[i],lstData[i]);
}

QWidget* ComboPropertyLine::item()
{
	return cb_;
}

QWidget* ComboPropertyLine::button()
{
	return NULL;
}

void ComboPropertyLine::slotReset(QVariant v)
{
	QStringList lst=prop_->param("values").split("/");
	int idx=lst.indexOf(v.toString());
	if(idx != -1)
		cb_->setCurrentIndex(idx);

	PropertyLine::checkState();
}

bool ComboPropertyLine::applyChange()
{
    PropertyLine::applyMaster();

	int idx=cb_->currentIndex();
    
    if(idx != -1)
    {
        QString currentDataVal=cb_->itemData(idx).toString();
        if(oriVal_.toString() != currentDataVal)
        {
		    prop_->setValue(currentDataVal);
		    return true;
        }    
	}

	return false;
}

QVariant ComboPropertyLine::currentValue()
{
	int idx=cb_->currentIndex();

	if(idx != -1)
	{
	    return cb_->itemData(idx).toString();
	}

	return QString();
}

void ComboPropertyLine::slotCurrentChanged(int)
{
    PropertyLine::checkState();
}

void ComboPropertyLine::setEnabledEditable(bool b)
{
	cb_->setEnabled(b);
}

static PropertyLineMaker<StringPropertyLine> makerStr(VProperty::StringType);
static PropertyLineMaker<ColourPropertyLine> makerCol(VProperty::ColourType);
static PropertyLineMaker<FontPropertyLine> makerFont(VProperty::FontType);
static PropertyLineMaker<IntPropertyLine> makerInt(VProperty::IntType);
static PropertyLineMaker<BoolPropertyLine> makerBool(VProperty::BoolType);
static PropertyLineMaker<ComboPropertyLine> makerCombo(VProperty::StringComboType);
