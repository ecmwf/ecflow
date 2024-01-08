/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_VRepeatAttr_HPP
#define ecflow_viewer_VRepeatAttr_HPP

#include <string>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class VAttributeType;
class VRepeatAttr;
class VNode;
class Repeat;

class VRepeatAttrType : public VAttributeType {
public:
    explicit VRepeatAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Repeat&, const VRepeatAttr*, QStringList&, const std::string&, QString) const;

private:
    enum DataIndex {
        TypeIndex      = 0,
        SubtypeIndex   = 1,
        NameIndex      = 2,
        ValueIndex     = 3,
        StartIndex     = 4,
        EndIndex       = 5,
        StepIndex      = 6,
        AllValuesIndex = 7,
        CurrentPosIdex = 8
    };
};

class VRepeatAttr : public VAttribute {
public:
    explicit VRepeatAttr(VNode* parent);

    int startIndex() const { return 0; }
    virtual int endIndex() const       = 0;
    virtual int currentIndex() const   = 0;
    virtual QString startValue() const = 0;
    virtual QString endValue() const   = 0;
    int step() const;
    virtual std::string value(int index) const = 0;
    virtual int currentPosition() const        = 0;

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);

    virtual QString allValues() const;
};

class VRepeatDateAttr : public VRepeatAttr {
public:
    explicit VRepeatDateAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    int currentPosition() const override;
    const std::string& subType() const override { return subType_; }

protected:
    static std::string subType_;
};

class VRepeatDateListAttr : public VRepeatAttr {
public:
    explicit VRepeatDateListAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    const std::string& subType() const override { return subType_; }
    QString allValues() const override;
    int currentPosition() const override;

protected:
    static std::string subType_;
};

class VRepeatDayAttr : public VRepeatAttr {
public:
    explicit VRepeatDayAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override { return 0; }
    int currentIndex() const override { return 0; }
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    const std::string& subType() const override { return subType_; }
    int currentPosition() const override { return -1; }

protected:
    static std::string subType_;
};

class VRepeatIntAttr : public VRepeatAttr {
public:
    explicit VRepeatIntAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    int currentPosition() const override;
    const std::string& subType() const override { return subType_; }

protected:
    static std::string subType_;
};

class VRepeatEnumAttr : public VRepeatAttr {
public:
    explicit VRepeatEnumAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    const std::string& subType() const override { return subType_; }
    QString allValues() const override;
    int currentPosition() const override;

protected:
    static std::string subType_;
};

class VRepeatStringAttr : public VRepeatAttr {
public:
    explicit VRepeatStringAttr(VNode* n) : VRepeatAttr(n) {}
    int endIndex() const override;
    int currentIndex() const override;
    QString startValue() const override;
    QString endValue() const override;
    std::string value(int index) const override;
    const std::string& subType() const override { return subType_; }
    QString allValues() const override;
    int currentPosition() const override;

protected:
    static std::string subType_;
};

#endif /* ecflow_viewer_VRepeatAttr_HPP */
