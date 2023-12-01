/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    VAutoRestoreAttr(VNode* parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VAutoRestoreAttr_HPP */
