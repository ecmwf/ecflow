/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_VTimeAttr_HPP
#define ecflow_viewer_VTimeAttr_HPP

#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/TimeAttr.hpp"
#include "ecflow/attribute/TodayAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VTimeAttrType : public VAttributeType {
public:
    explicit VTimeAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const ecf::Calendar& calendar, const ecf::TimeAttr& d, QStringList& data);
    void encode(const ecf::Calendar& calendar, const ecf::TodayAttr& d, QStringList& data);
    void encode(const ecf::Calendar& calendar, const ecf::CronAttr& d, QStringList& data);

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1, FreeIndex = 2 };
};

class VTimeAttr : public VAttribute {

public:
    enum DataType { TimeData, TodayData, CronData };

    VTimeAttr(VNode* parent, const ecf::TimeAttr&, int index);
    VTimeAttr(VNode* parent, const ecf::TodayAttr&, int index);
    VTimeAttr(VNode* parent, const ecf::CronAttr&, int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
    static int totalNum(VNode* vnode);

protected:
    DataType dataType_;
};

#endif /* ecflow_viewer_VTimeAttr_HPP */
