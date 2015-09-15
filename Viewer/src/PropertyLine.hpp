//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef PROPERTYLINE_INC_
#define PROPERTYLINE_INC_

#include <string>

#include <QFont>
#include <QObject>
#include <QVariant>

class  QCheckBox;
class  QLabel;
class  QLineEdit;
class  QSpinBox;
class  QToolButton;
class  QWidget;

#include "VProperty.hpp"

class PropertyLine;

//-------------------------------------
// Factory
//------------------------------------

class PropertyLineFactory
{
public:
	explicit PropertyLineFactory(VProperty::Type);
	virtual ~PropertyLineFactory();

	virtual PropertyLine* make(VProperty* p,bool,QWidget* w) = 0;
	static PropertyLine* create(VProperty* p,bool,QWidget* w);

private:
	explicit PropertyLineFactory(const PropertyLineFactory&);
	PropertyLineFactory& operator=(const PropertyLineFactory&);

};

template<class T>
class PropertyLineMaker : public PropertyLineFactory
{
	PropertyLine* make(VProperty* p,bool addLabel,QWidget* w) { return new T(p,addLabel,w); }
public:
	explicit PropertyLineMaker(VProperty::Type t) : PropertyLineFactory(t) {}
};


//-------------------------------------
// Abstract property line editor
//------------------------------------

class PropertyLine: public QObject
{
 Q_OBJECT

public:
	PropertyLine(VProperty*,bool addLabel,QWidget* parent=0);
	virtual ~PropertyLine() {}

	QLabel* label() {return label_;};
	QLabel* suffixLabel() {return suffixLabel_;};
	virtual QWidget* item()=0;
	virtual QWidget* button()=0;
	QToolButton* defaultTb() {return defaultTb_;};
	QToolButton* masterTb() {return masterTb_;};

	void init();
	virtual void reset(QVariant)=0;
	virtual bool applyChange()=0;
	virtual QVariant currentValue()=0;

protected Q_SLOTS:
	void slotResetToDefault(bool);
	void slotMaster(bool b);
	void checkState();

protected:
	virtual void setEnabledEditable(bool)=0;

	VProperty* prop_;
	QVariant val_;
	QLabel* label_;
	QLabel* suffixLabel_;
	QToolButton* defaultTb_;
	QToolButton* masterTb_;
};

//-------------------------------------
// String editor
//------------------------------------

class StringPropertyLine : public PropertyLine
{
	Q_OBJECT

public:
	StringPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent=0);
	QWidget* item();
	QWidget* button();
	void reset(QVariant);
	bool applyChange();
	QVariant currentValue();

public Q_SLOTS:
	void slotEdited(QString);

protected:
	void setEnabledEditable(bool);

private:
	QLineEdit* le_;
};

//-------------------------------------
// Colour editor
//------------------------------------

class ColourPropertyLine : public PropertyLine
{
Q_OBJECT

public:
	ColourPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent=0);
	QWidget* item();
	QWidget* button();
	void reset(QVariant);
	bool applyChange();
	QVariant currentValue();

private Q_SLOTS:
	void slotEdit(bool);

protected:
	void setEnabledEditable(bool);

private:
	QToolButton* cb_;
	QColor currentCol_;
};

//-------------------------------------
// Font editor
//------------------------------------

class FontPropertyLine : public PropertyLine
{
Q_OBJECT

public:
	FontPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent=0);
	QWidget* item();
	QWidget* button();
	void reset(QVariant);
	bool applyChange();
	QVariant currentValue();

private Q_SLOTS:
	void slotEdit(bool);

protected:
	void setEnabledEditable(bool);

private:
	QLabel* lName_;
	QToolButton *tbEdit_;
	QFont font_;
};

//-------------------------------------
// Int editor
//------------------------------------

class IntPropertyLine : public PropertyLine
{
	Q_OBJECT

public:
	IntPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent=0);
	QWidget* item();
	QWidget* button();
	void reset(QVariant);
	bool applyChange();
	QVariant currentValue();

public Q_SLOTS:
	void slotEdited(QString);

protected:
	void setEnabledEditable(bool);

private:
	QLineEdit* le_;
};

//-------------------------------------
// Boolean editor
//------------------------------------

class BoolPropertyLine : public PropertyLine
{
	Q_OBJECT

public:
	BoolPropertyLine(VProperty* vProp,bool addLabel,QWidget * parent=0);
	QWidget* item();
	QWidget* button();
	void reset(QVariant);
	bool applyChange();
	QVariant currentValue();

public Q_SLOTS:
	void slotStateChanged(int);

protected:
	void setEnabledEditable(bool);

private:
	QCheckBox* cb_;
};


#endif

