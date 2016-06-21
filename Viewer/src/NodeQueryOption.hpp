//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef NODEQUERYOPTION_HPP
#define NODEQUERYOPTION_HPP

#include <QStringList>

#include "StringMatchMode.hpp"

class NodeQuery;
class NodeQueryAttributeTerm;
class VAttributeType;
class VProperty;
class VSettings;
class NodeQueryStringOption;
class NodeQueryListOption;
class NodeQueryComboOption;

class NodeQueryOption
{
public:
    NodeQueryOption(VProperty*);

    QString type() const {return type_;}
    QString name() const {return name_;}
    QString label() const {return label_;}
    virtual QString valueAsString() const=0;

    virtual void swap(const NodeQueryOption*)=0;
    virtual QString query() const {return QString();}
    virtual QString query(QString op) const {return QString();}
    virtual void load(VSettings*)=0;
    virtual void save(VSettings*)=0;

    virtual NodeQueryStringOption* isString() const {return NULL;}
    virtual NodeQueryListOption* isList() const {return NULL;}
    virtual NodeQueryComboOption* isCombo() const {return NULL;}

    static void build(NodeQuery*);

protected:
    QString type_;
    QString name_;
    QString label_;
    bool ignoreIfAny_;

    //QStringList values_;
    //QStringList valueLabels_;
};

class NodeQueryStringOption : public NodeQueryOption
{
public:
    NodeQueryStringOption(VProperty*);

    void swap(const NodeQueryOption*);

    QString value() const {return value_;}
    QString valueAsString() const {return value();}
    const StringMatchMode&  matchMode() const {return matchMode_;}
    QString matchOperator() const {return QString::fromStdString(matchMode_.matchOperator());}
    bool caseSensitive() const {return caseSensitive_;}

    void setValue(QString s) {value_=s;}
    void setMatchMode(StringMatchMode::Mode m) {matchMode_.setMode(m);}
    void setMatchMode(const StringMatchMode& m) {matchMode_=m;}
    void setCaseSensitive(bool b) {caseSensitive_=b;}

    QString query() const;
    void load(VSettings*);
    void save(VSettings*);

    NodeQueryStringOption* isString() const {return const_cast<NodeQueryStringOption*>(this);}

protected:
    QString value_;
    StringMatchMode matchMode_;
    bool caseSensitive_;

    static StringMatchMode::Mode defaultMatchMode_;
    static bool defaultCaseSensitive_;
};

class NodeQueryListOption : public NodeQueryOption
{
public:
    NodeQueryListOption(VProperty*);

    void swap(const NodeQueryOption*);

    QString query(QString op) const;
    void load(VSettings*);
    void save(VSettings*);

    QString valueAsString() const {return QString();}
    QStringList values() const {return values_;}
    QStringList valueLabels() const {return valueLabels_;}
    void setSelection(QStringList lst) {selection_=lst;}
    QStringList selection() const {return selection_;}

    NodeQueryListOption* isList() const {return const_cast<NodeQueryListOption*>(this);}

protected:
    QStringList selection_;
    QStringList values_;
    QStringList valueLabels_;
};

class NodeQueryComboOption : public NodeQueryOption
{
public:
    NodeQueryComboOption(VProperty*);

    void swap(const NodeQueryOption*);

    QString query() const;
    void load(VSettings*);
    void save(VSettings*);

    QStringList values() const {return values_;}
    QStringList valueLabels() const {return valueLabels_;}
    void setValue(QString);
    QString value() const {return value_;}
    QString valueAsString() const {return value();}
    
    NodeQueryComboOption* isCombo() const {return const_cast<NodeQueryComboOption*>(this);}

protected:
    QString value_;
    QStringList values_;
    QStringList valueLabels_;
};

#endif // NODEQUERYOPTION_HPP

