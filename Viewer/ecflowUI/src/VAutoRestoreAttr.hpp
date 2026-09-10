/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VAutoRestoreAttr_HPP
#define ecflow_viewer_VAutoRestoreAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VAutoRestoreAttrType : public VAttributeType {
public:
    explicit VAutoRestoreAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(ecf::AutoRestoreAttr*, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, ValueIndex = 1 };
};

class VAutoRestoreAttr : public VAttribute {

public:
    explicit VAutoRestoreAttr(VNode* parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VAutoRestoreAttr_HPP */
