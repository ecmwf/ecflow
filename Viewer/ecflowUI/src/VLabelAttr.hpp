/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VLabelAttr_HPP
#define ecflow_viewer_VLabelAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class Label;

class VLabelAttrType : public VAttributeType {
public:
    explicit VLabelAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const Label& label, QStringList& data, bool firstLine) const;
    void encode_empty(QStringList& data) const;

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1, ValueIndex = 2 };
};

class VLabelAttr : public VAttribute {
public:
    VLabelAttr(VNode* parent, const Label&, int index);

    int lineNum() const override;
    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VLabelAttr_HPP */
