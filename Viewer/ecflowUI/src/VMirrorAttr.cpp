/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VMirrorAttr.hpp"

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/node/MirrorAttr.hpp"

//================================
// VMirrorAttrType
//================================

VMirrorAttrType::VMirrorAttrType() : VAttributeType("mirror") {
    dataCount_                             = 9;
    searchKeyToData_["mirror_name"]        = NameIndex;
    searchKeyToData_["mirror_remote_path"] = RemotePathIndex;
    searchKeyToData_["mirror_remote_host"] = RemoteHostIndex;
    searchKeyToData_["mirror_remote_port"] = RemotePortIndex;
    searchKeyToData_["mirror_polling"]     = PollingIndex;
    scanProc_                              = VMirrorAttr::scan;
}

QString VMirrorAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Mirror<br>";
    if (d.count() == dataCount_) {
        t += "<b>Name:</b> " + d[NameIndex] + "<br>";
        t += "<b>Remote Path:</b> " + d[RemotePathIndex] + "<br>";
        t += "<b>Remote Host:</b> " + d[RemoteHostIndex] + "<br>";
        t += "<b>Remote Port:</b> " + d[RemotePortIndex] + "<br>";
        t += "<b>Polling:</b> " + d[PollingIndex] + "<br>";
        t += "<b>SSL:</b> " + d[SslIndex] + "<br>";
        t += "<b>Auth:</b> " + d[AuthIndex];
        if (const auto& reason = d[ReasonIndex]; !reason.isEmpty()) {
            t += "<br><b>Reason:</b> <span style=\"color:red\">" + d[ReasonIndex] + "</span>";
        }
    }
    return t;
}

QString VMirrorAttrType::definition(QStringList d) const {
    QString t = "mirror";
    if (d.count() == dataCount_) {
        t += " " + d[NameIndex] + " '" + d[RemoteHostIndex] + ":" + d[RemotePortIndex] + "' at '" + d[RemotePathIndex] +
             "'";
    }
    return t;
}

void VMirrorAttrType::encode(const ecf::MirrorAttr& mirror, QStringList& data, bool firstLine) const {

    data << qName_                                                  // TypeIndex
         << QString::fromStdString(mirror.name())                   // NameIndex
         << QString::fromStdString(mirror.remote_path())            // RemotePathIndex
         << QString::fromStdString(mirror.remote_host())            // RemoteHostIndex
         << QString::fromStdString(mirror.remote_port())            // RemotePortIndex
         << QString::fromStdString(mirror.polling())                // PollingIndex
         << QString::fromStdString(mirror.ssl() ? "true" : "false") // SslIndex
         << QString::fromStdString(mirror.auth())                   // AuthIndex
         << QString::fromStdString(mirror.reason());                // ReasonIndex
}

void VMirrorAttrType::encode_empty(QStringList& data) const {
    data << qName_;
}

//=====================================================
//
// VMirrorAttr
//
//=====================================================

VMirrorAttr::VMirrorAttr(VNode* parent, [[maybe_unused]] const ecf::MirrorAttr& mirror, int index)
    : VAttribute(parent, index) {
}

int VMirrorAttr::lineNum() const {
    return 1;
}

VAttributeType* VMirrorAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("mirror");
    return atype;
}

QStringList VMirrorAttr::data(bool firstLine) const {
    static auto* atype = static_cast<VMirrorAttrType*>(type());
    QStringList s;
    if (parent_->node_) {
        const std::vector<ecf::MirrorAttr>& v = parent_->node_->mirrors();
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

std::string VMirrorAttr::strName() const {
    if (parent_->node_) {
        const std::vector<ecf::MirrorAttr>& v = parent_->node_->mirrors();
        return v[index_].name();
    }
    return {};
}

void VMirrorAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (vnode->node_) {
        const std::vector<ecf::MirrorAttr>& v = vnode->node_->mirrors();

        auto n = static_cast<int>(v.size());
        for (int i = 0; i < n; i++) {
            vec.push_back(new VMirrorAttr(vnode, v[i], i));
        }
    }
}
