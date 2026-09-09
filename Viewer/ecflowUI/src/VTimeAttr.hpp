/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
