// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VAutoCancelAttrType : public VAttributeType {
public:
    explicit VAutoCancelAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(ecf::AutoCancelAttr*, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, ValueIndex = 1 };
};

class VAutoCancelAttr : public VAttribute {

public:
    explicit VAutoCancelAttr(VNode* parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};
