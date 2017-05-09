//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VTIME_HPP
#define VTIME_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

#include <QStringList>
#include <vector>

#include "CronAttr.hpp"
#include "TimeAttr.hpp"
#include "TodayAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VTimeAttrType : public VAttributeType
{
public:
    explicit VTimeAttrType();
    QString toolTip(QStringList d) const;
    QString definition(QStringList d) const;
    void encode(const ecf::TimeAttr& d,QStringList& data);
    void encode(const ecf::TodayAttr& d,QStringList& data);
    void encode(const ecf::CronAttr& d,QStringList& data);

private:
    enum DataIndex {TypeIndex=0,NameIndex=1};
};

class VTimeAttr : public VAttribute
{

public:
    enum DataType {TimeData,TodayData,CronData};

    VTimeAttr(VNode *parent,const ecf::TimeAttr&,int index);
    VTimeAttr(VNode *parent,const ecf::TodayAttr&,int index);
    VTimeAttr(VNode *parent,const ecf::CronAttr&,int index);

    VAttributeType* type() const;
    QStringList data() const;
    std::string strName() const;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
    static int totalNum(VNode* vnode);

protected:
    DataType dataType_;
};

#endif // VTIME_HPP
