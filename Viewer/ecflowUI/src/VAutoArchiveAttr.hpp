/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VAutoArchiveAttr_HPP
#define ecflow_viewer_VAutoArchiveAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/attribute/AutoArchiveAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VAutoArchiveAttrType : public VAttributeType {
public:
    explicit VAutoArchiveAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(ecf::AutoArchiveAttr*, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, ValueIndex = 1 };
};

class VAutoArchiveAttr : public VAttribute {

public:
    explicit VAutoArchiveAttr(VNode* parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VAutoArchiveAttr_HPP */
