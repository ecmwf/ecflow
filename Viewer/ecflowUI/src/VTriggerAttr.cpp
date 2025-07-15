/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VTriggerAttr.hpp"

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

//================================
// VTriggerAttrType
//================================

VTriggerAttrType::VTriggerAttrType() : VAttributeType("trigger") {
    dataCount_                             = 3;
    searchKeyToData_["trigger_type"]       = CompleteIndex;
    searchKeyToData_["trigger_expression"] = ExprIndex;
    searchKeyToData_["name"]               = TypeIndex;
    scanProc_                              = VTriggerAttr::scan;
}

QString VTriggerAttrType::toolTip(QStringList d) const {
    QString t;
    if (d.count() == dataCount_) {
        if (d[CompleteIndex] == "0")
            t += "<b>Type:</b> Trigger<br>";
        else if (d[CompleteIndex] == "1")
            t += "<b>Type:</b> Complete<br>";
        else
            return t;

        t += "<b>Expression:</b> " + d[ExprIndex];
    }
    return t;
}

QString VTriggerAttrType::definition(QStringList d) const {
    QString t;
    if (d.count() == dataCount_) {
        if (d[CompleteIndex] == "0")
            t += "trigger";
        else if (d[CompleteIndex] == "1")
            t += "complete";

        t += " " + d[ExprIndex];
    }
    return t;
}

void VTriggerAttrType::encodeTrigger(Expression* e, QStringList& data) const {
    if (e)
        data << qName_ << "0" << QString::fromStdString(e->expression());
    else
        encode_empty(data);
}

void VTriggerAttrType::encodeComplete(Expression* e, QStringList& data) const {
    if (e)
        data << qName_ << "1" << QString::fromStdString(e->expression());
    else
        encode_empty(data);
}

void VTriggerAttrType::encode_empty(QStringList& data) const {
    data << qName_ << ""
         << "";
}

//=====================================================
//
// VTriggerAttr
//
//=====================================================

VTriggerAttr::VTriggerAttr(VNode* parent, Expression*, int index) : VAttribute(parent, index) {
}

VAttributeType* VTriggerAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("trigger");
    return atype;
}

QStringList VTriggerAttr::data(bool /*firstLine*/) const {
    static auto* atype = dynamic_cast<VTriggerAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        if (index_ == 0) {
            atype->encodeTrigger(node->get_trigger(), s);
        }
        else {
            atype->encodeComplete(node->get_complete(), s);
        }
    }
    return s;
}

std::string VTriggerAttr::strName() const {
    return {"trigger"};
}

void VTriggerAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        Expression* eT = node->get_trigger();
        Expression* eC = node->get_complete();

        if (eT)
            vec.push_back(new VTriggerAttr(vnode, eT, 0));

        if (eC)
            vec.push_back(new VTriggerAttr(vnode, eC, 1));
    }
}

void VTriggerAttr::expressions(const VNode* vnode, std::string& trigger, std::string& complete) {
    if (node_ptr node = vnode->node()) {
        Expression* eT = node->get_trigger();
        Expression* eC = node->get_complete();

        if (eT)
            trigger = eT->expression();

        if (eC)
            complete = eC->expression();
    }
}

std::string VTriggerAttr::ast_str() const {
    std::string buffer;
    ecf::stringstreambuf ss{buffer};
    if (node_ptr node = parent_->node()) {
        if (index_ == 0) {
            if (Expression* e = node->get_trigger()) {
                auto ctx = ecf::Context::make_for(PrintStyle::DEFS);

                ecf::Indent l1(ctx);

                if (e->isFree()) {
                    ss << l1 << "# (free)\n";
                }

                // show the Abstract Syntax Tree for the expression. This also uses Indentor
                if (Ast* ast = node->triggerAst()) {
                    ecf::write_t(ss, *ast, ctx);
                }
            }
        }
        else {
            if (Expression* e = node->get_complete()) {
                auto ctx = ecf::Context::make_for(PrintStyle::DEFS);

                ecf::Indent l1(ctx);

                if (e->isFree()) {
                    ss << l1 << "# (free)\n";
                }

                // show the Abstract Syntax Tree for the expression. This also uses Indentor
                if (Ast* ast = node->completeAst()) {
                    ecf::write_t(ss, *ast, ctx);
                }
            }
        }
    }

    return buffer;
}
