/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VLimitAttr_HPP
#define ecflow_viewer_VLimitAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ecflow/node/LimitFwd.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

class VLimitAttrType : public VAttributeType {
public:
    explicit VLimitAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(limit_ptr, QStringList&) const;

private:
    enum DataIndex { TypeIndex = 0, NameIndex = 1, ValueIndex = 2, MaxIndex = 3 };
};

class VLimitAttr : public VAttribute {

public:
    VLimitAttr(VNode* parent, limit_ptr, int index);

    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;
    QStringList paths() const;
    void removePaths(const std::vector<std::string>& paths);
    void resetPaths();

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VLimitAttr_HPP */
