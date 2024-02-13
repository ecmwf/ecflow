/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
