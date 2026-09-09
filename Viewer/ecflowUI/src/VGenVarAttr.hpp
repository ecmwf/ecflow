/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VGenVarAttr_HPP
#define ecflow_viewer_VGenVarAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;
class Variable;

class VGenVarAttrType : public VAttributeType {
public:
    explicit VGenVarAttrType();
    QString toolTip(QStringList d) const override;
    void encode(const Variable&, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1, ValueIndex = 2 };
};

class VGenVarAttr : public VAttribute {
public:
    VGenVarAttr(VNode* parent, const Variable&, int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;
    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
    static bool isReadOnly(const std::string&);
};

#endif /* ecflow_viewer_VGenVarAttr_HPP */
