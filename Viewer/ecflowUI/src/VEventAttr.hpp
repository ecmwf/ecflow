/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VEventAttr_HPP
#define ecflow_viewer_VEventAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class Event;

class VEventAttrType : public VAttributeType {
public:
    explicit VEventAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Event&, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1, ValueIndex = 2 };
};

class VEventAttr : public VAttribute {
public:
    VEventAttr(VNode* parent, const Event&, int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VEventAttr_HPP */
