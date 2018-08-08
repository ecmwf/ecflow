//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VREPEATATTR_HPP
#define VREPEATATTR_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <string>
#include <QStringList>

class VAttributeType;
class VNode;
class Repeat;

class VRepeatAttrType : public VAttributeType
{
public:
    explicit VRepeatAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Repeat&,QStringList&,const std::string&,QString) const;

private:
    enum DataIndex {TypeIndex=0,SubtypeIndex=1,NameIndex=2,ValueIndex=3,StartIndex=4,EndIndex=5,StepIndex=6,AllValuesIndex=7};
};

class VRepeatAttr : public VAttribute
{
public:
    VRepeatAttr(VNode *parent);

    int startIndex() const {return 0;}
    virtual int endIndex() const=0;
    virtual int currentIndex() const=0;
    int step() const;
    virtual std::string value(int index) const=0;

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);

    virtual QString allValues() const;

};

class VRepeatDateAttr : public VRepeatAttr
{
public:
    VRepeatDateAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    std::string value(int index) const override;
    const std::string& subType() const override {return subType_;}

protected:
    static std::string subType_;
};

class VRepeatDayAttr : public VRepeatAttr
{
public:
    VRepeatDayAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override {return 0;}
    int currentIndex() const override {return 0;}
    std::string value(int index) const override;
    const std::string& subType() const override {return subType_;}

protected:
    static std::string subType_;
};

class VRepeatIntAttr : public VRepeatAttr
{
public:
    VRepeatIntAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    std::string value(int index) const override;
    const std::string& subType() const override {return subType_;}

protected:
    static std::string subType_;
};

class VRepeatEnumAttr : public VRepeatAttr
{
public:
    VRepeatEnumAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    std::string value(int index) const override;
    const std::string& subType() const override {return subType_;}
    QString allValues() const override;

protected:
    static std::string subType_;
};

class VRepeatStringAttr : public VRepeatAttr
{
public:
    VRepeatStringAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    std::string value(int index) const override;
    const std::string& subType() const override {return subType_;}
    QString allValues() const override;

protected:
    static std::string subType_;
};


#endif // VREPEATATTR_HPP

