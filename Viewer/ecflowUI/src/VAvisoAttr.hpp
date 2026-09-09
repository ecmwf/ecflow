/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VAvisoAttr_HPP
#define ecflow_viewer_VAvisoAttr_HPP

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class AttributeFilter;
class VAttributeType;
class VNode;

namespace ecf {
class AvisoAttr;
}

class VAvisoAttrType : public VAttributeType {
public:
    explicit VAvisoAttrType();
    QString toolTip(QStringList d) const override;
    QString definition(QStringList d) const override;
    void encode(const ecf::AvisoAttr& aviso, QStringList& data, bool firstLine) const;
    void encode_empty(QStringList& data) const;

private:
    enum DataIndex {
        TypeIndex     = 0,
        NameIndex     = 1,
        ListenerIndex = 2,
        UrlIndex      = 3,
        SchemaIndex   = 4,
        PollingIndex  = 5,
        RevisionIndex = 6,
        AuthIndex     = 7,
        ReasonIndex   = 8,
        ActiveIndex   = 9,
    };
};

class VAvisoAttr : public VAttribute {
public:
    VAvisoAttr(VNode* parent, const ecf::AvisoAttr&, int index);

    int lineNum() const override;
    VAttributeType* type() const override;
    QStringList data(bool firstLine) const override;
    std::string strName() const override;

    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};

#endif /* ecflow_viewer_VAvisoAttr_HPP */
