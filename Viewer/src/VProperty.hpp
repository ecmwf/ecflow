//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VPROPERTY_HPP_
#define VPROPERTY_HPP_

#include <QColor>
#include <QFont>
#include <QList>
#include <QVariant>

class VProperty;

class VPropertyObserver
{
public:
	VPropertyObserver() {}
	virtual ~VPropertyObserver() {}

	virtual void notifyChange(VProperty*)=0;
};


//This class defines a property storing its value as a QVariant.
//Properties are used to store the viewer's configuration. Editable properties
//store all the information needed to display them in an property editor.
//Properties can have children so trees can be built up from them.

class VProperty
{
public:
    VProperty(const std::string& name);
    ~VProperty();

    QString name() const {return name_;}
    const std::string& strName() const {return strName_;}
    QString labelText() const {return labelText_;}
    QString toolTip() const {return toolTip_;}
    bool editable() const {return editable_;}
    QVariant defaultValue() const {return defaultValue_;}
    QVariant value() const {return value_;}

    void setEditable(bool e) {editable_=e;}
    void setLabelText(const std::string&);
    void setToolTip(const std::string&);
    void setDefaultValue(const std::string&);
    void setValue(const std::string&);
    void setValue(QVariant);

    bool hasChildren() const {return children_.count() >0;}
    QList<VProperty*> children() const {return children_;}
    void addChild(VProperty*);
    VProperty* findChild(QString name);
    
    void addObserver(VPropertyObserver*);
    void removeObserver(VPropertyObserver*);

protected:
    std::string strName_; //The name of the property as as std::string
    QString name_; //The name of the property. Used as an id.
    QString toolTip_;  //The tooltip to display in the editor
    QString labelText_; //The name/label to display in the editor
    QVariant defaultValue_; //The default value
    QVariant value_; //The current value
    bool editable_;

private:
    void dispatchChange();

    static bool isColour(const std::string&);
    static bool isFont(const std::string&);
    static bool isNumber(const std::string&);

    static QColor toColour(const std::string&) ;
    static QFont  toFont(const std::string&);
    static int    toNumber(const std::string&);

    QList<VProperty*> children_;
    QList<VPropertyObserver*> observers_;
};

#endif
