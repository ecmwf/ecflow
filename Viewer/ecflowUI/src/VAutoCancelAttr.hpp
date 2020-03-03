//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VAUTOCANCELATTR_HPP
#define VAUTOCANCELATTR_HPP

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "AutoCancelAttr.hpp"

#include <QStringList>
#include <string>
#include <vector>

class AttributeFilter;
class VAttributeType;
class VNode;

class VAutoCancelAttrType : public VAttributeType
{
public:
    explicit VAutoCancelAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(ecf::AutoCancelAttr*,QStringList&) const;

private:
    enum DataIndex {TypeIndex=0,ValueIndex=1};
};

class VAutoCancelAttr : public VAttribute
{

public:
    VAutoCancelAttr(VNode *parent);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;

    static void scan(VNode* vnode,std::vector<VAttribute*>& vec);
};

#endif // VAUTOCANCELATTR_HPP
