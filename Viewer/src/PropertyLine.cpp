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

#include "Sound.hpp"

#include <assert.h>

static std::map<VProperty::GuiType,PropertyLineFactory*>* makers = 0;

//=========================================================================
//
// PropertyLineFactory
//
//=========================================================================

PropertyLineFactory::PropertyLineFactory(VProperty::GuiType type)
{
	if(makers == 0)
		makers = new std::map<VProperty::GuiType,PropertyLineFactory*>;

	(*makers)[type] = this;
}

PropertyLineFactory::~PropertyLineFactory()
{
	// Not called
}

PropertyLine* PropertyLineFactory::create(VProperty* p,bool addLabel,QWidget* w)
{
	if(!p || !p->link())
		return 0;

	VProperty::GuiType t=p->link()->guiType();
	std::map<VProperty::GuiType,PropertyLineFactory*>::iterator j = makers->find(t);
	if(j != makers->end())
		return (*j).second->make(p,addLabel,w);

	return 0;
}

//=========================================================================
//
// PropertyLine
//
//=========================================================================

PropertyLine::PropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) :
	QObject(parent),
	prop_(NULL),
	guiProp_(guiProp),
	label_(0),
	suffixLabel_(0),
	defaultTb_(0),
	masterTb_(0),
	enabled_(true),
	doNotEmitChange_(false)
{
	prop_=guiProp_->link();
	assert(prop_);

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
	doNotEmitChange_=true;
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
	doNotEmitChange_=false;
}

void PropertyLine::slotResetToDefault(bool)
{
	slotReset(prop_->defaultValue());
	checkState();
}

void PropertyLine::slotEnabled(QVariant v)
{
	if(enabled_ != v.toBool())
	{
		if(!masterTb_ || !masterTb_->isChecked())
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

	if(masterTb_ && masterTb_->isChecked())
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

	Q_EMIT masterChanged(b);

	valueChanged();
}

void PropertyLine::slotReset(VProperty* prop,QVariant v)
{
	if(prop == prop_)
		slotReset(v);
}

void PropertyLine::valueChanged()
{
	if(!doNotEmitChange_)
		Q_EMIT changed();
}

void PropertyLine::addHelper(PropertyLine* line)
{
	if(line)
		helpers_[line->property()->name()]=line;
}

//=========================================================================
//
// StringPropertyLine
//
//=========================================================================

StringPropertyLine::StringPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,addLabel,parent)
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
	valueChanged();
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
	valueChanged();
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

ColourPropertyLine::ColourPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

	QFont f;
	QFontMetrics fm(f);
	int height=fm.height();
	int width=fm.width("AAAAAAA");

	cb_=new QToolButton(parent);
    cb_->setFixedWidth(width);
    cb_->setFixedHeight(height+2);
    cb_->setToolTip(tr("Click to select a colour"));

    styleSheet_="QToolButton { background: BG;  border: 1px solid rgb(120,120,120); border-radius: 2px;}";

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

	QString st=styleSheet_;
	st.replace("BG","rgb(" + QString::number(c.red()) + "," +
			QString::number(c.green()) + "," + QString::number(c.blue()) + ")");

	cb_->setStyleSheet(st);

	currentCol_=c;

	PropertyLine::checkState();
	valueChanged();
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

FontPropertyLine::FontPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,addLabel,parent)
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
	valueChanged();
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
	valueChanged();
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

IntPropertyLine::IntPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,addLabel,parent)
{
	if(label_)
        label_->setText(label_->text() + ":");

	le_=new QLineEdit(parent);
	QIntValidator* validator=new QIntValidator(le_);

	QString s=guiProp->param("max");
	if(!s.isEmpty())
	{
		validator->setTop(s.toInt());
	}

	s=guiProp->param("min");
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
	valueChanged();
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
	valueChanged();
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

BoolPropertyLine::BoolPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,false,parent)
{
	cb_=new QCheckBox(prop_->param("label"));

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
	valueChanged();

	//BoolLines emit this signal because they might control
	//other lines' status
	Q_EMIT changed(currentValue());
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
	valueChanged();

	//BoolLines emit this signal because they might control
	//other lines' status
	Q_EMIT changed(currentValue());
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

ComboPropertyLine::ComboPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) : PropertyLine(guiProp,addLabel,parent)
{
	if(label_)
	    label_->setText(label_->text() + ":");

	cb_=new QComboBox(parent);//(vProp->param("label"));

	connect(cb_,SIGNAL(currentIndexChanged(int)),
			   this,SLOT(slotCurrentChanged(int)));

	QStringList lst=prop_->param("values_label").split("/");
    QStringList lstData=prop_->param("values").split("/");
    if(prop_->param("values_label").simplified().isEmpty())
        lst=lstData;
    
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
	valueChanged();
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
    valueChanged();
}

void ComboPropertyLine::setEnabledEditable(bool b)
{
	cb_->setEnabled(b);
}

//=========================================================================
//
// SoundComboPropertyLine
//
//=========================================================================

SoundComboPropertyLine::SoundComboPropertyLine(VProperty* guiProp,bool addLabel,QWidget * parent) :
	ComboPropertyLine(guiProp,addLabel,parent),
	playTb_(NULL)
{
	playTb_=new QToolButton(parent);
	playTb_->setText("play");
	playTb_->setToolTip(tr("Play sound"));

	connect(playTb_,SIGNAL(clicked(bool)),
			this,SLOT(slotPlay(bool)));
}

QWidget* SoundComboPropertyLine::item()
{
	return cb_;
}

QWidget* SoundComboPropertyLine::button()
{
	return playTb_;
}

void SoundComboPropertyLine::setEnabledEditable(bool b)
{
	cb_->setEnabled(b);
	playTb_->setEnabled(b);
}

void SoundComboPropertyLine::slotPlay(bool)
{
	int repeat=0;
	if(PropertyLine* line=helpers_.value("sound_repeat",NULL))
		repeat=line->currentValue().toInt();

	Sound::instance()->playSystem(currentValue().toString().toStdString(),repeat);
}


static PropertyLineMaker<StringPropertyLine> makerStr(VProperty::StringGui);
static PropertyLineMaker<ColourPropertyLine> makerCol(VProperty::ColourGui);
static PropertyLineMaker<FontPropertyLine> makerFont(VProperty::FontGui);
static PropertyLineMaker<IntPropertyLine> makerInt(VProperty::IntGui);
static PropertyLineMaker<BoolPropertyLine> makerBool(VProperty::BoolGui);
static PropertyLineMaker<ComboPropertyLine> makerCombo(VProperty::StringComboGui);
static PropertyLineMaker<SoundComboPropertyLine> makerSoundCombo(VProperty::SoundComboGui);
