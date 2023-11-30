/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    VAutoArchiveAttr(VNode* parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VAutoArchiveAttr_HPP */
