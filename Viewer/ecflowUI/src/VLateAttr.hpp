/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VLateAttr_HPP
#define ecflow_viewer_VLateAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/attribute/LateAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class Label;

class VLateAttrType : public VAttributeType {
public:
    explicit VLateAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(ecf::LateAttr* late, QStringList& data) const;

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1 };
};

class VLateAttr : public VAttribute {
public:
    VLateAttr(VNode* parent, const std::string&);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VLateAttr_HPP */
