/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VLimiterAttr_HPP
#define ecflow_viewer_VLimiterAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class InLimit;

class VLimiterAttrType : public VAttributeType {
public:
    explicit VLimiterAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const InLimit&, QStringList&) const;
    void scan(VNode* vnode, std::vector<VAttribute*>& vec);
    int totalNum(VNode* vnode);

private:
    enum DataIndex {
        TypeIndex       = 0,
        NameIndex       = 1,
        PathIndex       = 2,
        TokenIndex      = 3,
        SubmissionIndex = 4,
        FamiliesIndex   = 5
    };
};

class VLimiterAttr : public VAttribute {

public:
    VLimiterAttr(VNode* parent, const InLimit&, int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VLimiterAttr_HPP */
