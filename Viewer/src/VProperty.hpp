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

#include <vector>

class VProperty;

class VPropertyObserver
{
public:
	VPropertyObserver() {}
	virtual ~VPropertyObserver() {}

	virtual void notifyChange(VProperty*)=0;
};

class VPropertyVisitor
{
public:
	VPropertyVisitor() {};
	virtual ~VPropertyVisitor() {};
	virtual void visit(VProperty*)=0;
};



//This class defines a property storing its value as a QVariant.
//Properties are used to store the viewer's configuration. Editable properties
//store all the information needed to display them in a property editor.
//Properties can have children so trees can be built up from them.

class VProperty : public VPropertyObserver
{
public:
    explicit VProperty(const std::string& name);
    ~VProperty();

    enum Type {StringType,IntType,BoolType,ColourType,FontType};

    QString name() const {return name_;}
    const std::string& strName() const {return strName_;}
    QVariant defaultValue() const {return defaultValue_;}
    QVariant value() const {return value_;}
    std::string valueAsString() const;
    Type type() const {return type_;}
    QString param(QString name);

    void setDefaultValue(const std::string&);
    void setValue(const std::string&);
    void setValue(QVariant);
    void setParam(QString,QString);

    std::string path();
    void setParent(VProperty* p) {parent_=p;}
    bool hasChildren() const {return children_.count() >0;}
    QList<VProperty*> children() const {return children_;}
    void addChild(VProperty*);
    VProperty* findChild(QString name);
    VProperty* find(const std::string& fullPath);
    void collectChildren(std::vector<VProperty*>&) const;

    bool changed() const;
    void collectLinks(std::vector<VProperty*>&);

    void setLink(VProperty* p) {link_=p;}
    VProperty* link() const {return link_;}

    void setMaster(VProperty*);
    VProperty *clone(bool addLink,bool setMaster);

    void addObserver(VPropertyObserver*);
    void removeObserver(VPropertyObserver*);

    void notifyChange(VProperty*) {}

    static bool isColour(const std::string&);
    static bool isFont(const std::string&);
    static bool isNumber(const std::string&);
    static bool isBool(const std::string&);

    static QColor toColour(const std::string&) ;
    static QFont  toFont(const std::string&);
    static int    toNumber(const std::string&);
    static bool   toBool(const std::string&);

    static QString toString(QColor col);
    static QString toString(QFont f);

private:
    void dispatchChange();
    VProperty* find(const std::vector<std::string>& pathVec);

    std::string strName_; //The name of the property as an std::string
    QString name_; //The name of the property. Used as an id.
    QVariant defaultValue_; //The default value
    QVariant value_; //The current value

    VProperty* parent_;
    QList<VProperty*> children_;
    QList<VPropertyObserver*> observers_;
    VProperty* master_;
    bool useMaster_;
    Type type_;
    QMap<QString,QString> params_;
    VProperty* link_;
};


#endif
