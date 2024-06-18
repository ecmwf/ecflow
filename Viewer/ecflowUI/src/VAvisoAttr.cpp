/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VAvisoAttr.hpp"

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/node/AvisoAttr.hpp"

//================================
// VAvisoAttrType
//================================

VAvisoAttrType::VAvisoAttrType() : VAttributeType("aviso") {
    dataCount_                         = 9;
    searchKeyToData_["aviso_name"]     = NameIndex;
    searchKeyToData_["aviso_listener"] = ListenerIndex;
    searchKeyToData_["aviso_url"]      = UrlIndex;
    searchKeyToData_["aviso_schema"]   = SchemaIndex;
    searchKeyToData_["aviso_polling"]  = PollingIndex;
    searchKeyToData_["aviso_revision"] = RevisionIndex;
    searchKeyToData_["aviso_auth"]     = AuthIndex;
    searchKeyToData_["aviso_reason"]   = ReasonIndex;
    scanProc_                          = VAvisoAttr::scan;
}

QString VAvisoAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Aviso<br>";
    if (d.count() == dataCount_) {
        t += "<b>Name:</b> " + d[NameIndex] + "<br>";
        t += "<b>Listener:</b> " + d[ListenerIndex] + "<br>";
        t += "<b>URL:</b> " + d[UrlIndex] + "<br>";
        t += "<b>Schema:</b> " + d[SchemaIndex] + "<br>";
        t += "<b>Polling:</b> " + d[PollingIndex] + " s<br>";
        t += "<b>Revision:</b> " + d[RevisionIndex] + "<br>";
        t += "<b>Auth:</b> " + d[AuthIndex];
        if (auto& reason = d[ReasonIndex]; !reason.isEmpty()) {
            t += "<br><b>Reason:</b> <span style=\"color:red\">" + d[ReasonIndex] + "</span>";
        }
    }
    return t;
}

QString VAvisoAttrType::definition(QStringList d) const {
    QString t = "aviso";
    if (d.count() == dataCount_) {
        t += " " + d[NameIndex] + " '" + d[ListenerIndex] + "'";
    }
    return t;
}

void VAvisoAttrType::encode(const ecf::AvisoAttr& aviso, QStringList& data, bool firstLine) const {
    std::string val = aviso.listener();

    if (firstLine) {
        std::size_t pos = val.find("\n");
        if (pos != std::string::npos) {
            val.resize(pos);
        }
    }

    data << qName_                                  // TypeIndex
         << QString::fromStdString(aviso.name())    // NameIndex
         << QString::fromStdString(val)             // ListenerIndex
         << QString::fromStdString(aviso.url())     // UrlIndex
         << QString::fromStdString(aviso.schema())  // SchemaIndex
         << QString::fromStdString(aviso.polling()) // PollingIndex
         << QString::number(aviso.revision())       // Revision Index
         << QString::fromStdString(aviso.auth())    // AuthIndex
         << QString::fromStdString(aviso.reason()); // ReasonIndex
}

void VAvisoAttrType::encode_empty(QStringList& data) const {
    data << qName_;
}

//=====================================================
//
// VAvisoAttr
//
//=====================================================

VAvisoAttr::VAvisoAttr(VNode* parent, [[maybe_unused]] const ecf::AvisoAttr& aviso, int index)
    : VAttribute(parent, index) {
}

int VAvisoAttr::lineNum() const {
    if (parent_->node_) {
        const std::vector<ecf::AvisoAttr>& v = parent_->node_->avisos();
        std::string val                      = v[index_].listener();
        return std::count(val.begin(), val.end(), '\n') + 1;
    }

    return 1;
}

VAttributeType* VAvisoAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("aviso");
    return atype;
}

QStringList VAvisoAttr::data(bool firstLine) const {
    static auto* atype = static_cast<VAvisoAttrType*>(type());
    QStringList s;
    if (parent_->node_) {
        const std::vector<ecf::AvisoAttr>& v = parent_->node_->avisos();
        if (index_ < static_cast<int>(v.size()))
            atype->encode(v[index_], s, firstLine);

        // this can happen temporarily during update when:
        //
        // - an attribute was already deleted
        // - the notification was emitted from the update thread, but
        //   has not yet reached the main thread
        // - here, in the main thread, we still have the old (incorrect) attribute number
        //
        // * In this case, as safety measure, we encode an empty attribute.
        //   When the notification arrives all the attributes of the given node will be rescanned
        //   and will be set with the correct state.
        else
            atype->encode_empty(s);
    }
    return s;
}

std::string VAvisoAttr::strName() const {
    if (parent_->node_) {
        const std::vector<ecf::AvisoAttr>& v = parent_->node_->avisos();
        return v[index_].name();
    }
    return {};
}

void VAvisoAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (vnode->node_) {
        const std::vector<ecf::AvisoAttr>& v = vnode->node_->avisos();

        auto n = static_cast<int>(v.size());
        for (int i = 0; i < n; i++) {
            vec.push_back(new VAvisoAttr(vnode, v[i], i));
        }
    }
}
