/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "NodeQueryOption.hpp"

#include <map>
#include <string>

#include <QDebug>

#include "NodeQuery.hpp"
#include "UiLog.hpp"
#include "VAttributeType.hpp"
#include "VConfig.hpp"
#include "VProperty.hpp"
#include "VSettings.hpp"

StringMatchMode::Mode NodeQueryStringOption::defaultMatchMode_ = StringMatchMode::ContainsMatch;
bool NodeQueryStringOption::defaultCaseSensitive_              = false;

#define _UI_NODEQUERY_DEBUG

class NodeQueryOptionFactory;
static std::map<std::string, NodeQueryOptionFactory*>* makers = nullptr;

//=========================================
// Factory
//=========================================

class NodeQueryOptionFactory {
public:
    explicit NodeQueryOptionFactory(const std::string& t);
    virtual ~NodeQueryOptionFactory()                                = default;
    explicit NodeQueryOptionFactory(const NodeQueryOptionFactory&)   = delete;
    NodeQueryOptionFactory& operator=(const NodeQueryOptionFactory&) = delete;

    virtual NodeQueryOption* make(VProperty* p)                      = 0;
    static NodeQueryOption* create(VProperty* p);
};

template <class T>
class NodeQueryOptionMaker : public NodeQueryOptionFactory {
    NodeQueryOption* make(VProperty* p) override { return new T(p); }

public:
    explicit NodeQueryOptionMaker(const std::string& t) : NodeQueryOptionFactory(t) {}
};

NodeQueryOptionFactory::NodeQueryOptionFactory(const std::string& type) {
    if (makers == nullptr)
        makers = new std::map<std::string, NodeQueryOptionFactory*>;

    (*makers)[type] = this;
}

NodeQueryOption* NodeQueryOptionFactory::create(VProperty* p) {
    if (!p)
        return nullptr;

    std::string type = p->param("type").toStdString();
    auto j           = makers->find(type);
    if (j != makers->end())
        return (*j).second->make(p);

    return nullptr;
}

//===============================================
//
// NodeQueryOption
//
//===============================================

NodeQueryOption::NodeQueryOption(VProperty* p)
    : type_(p->param("type")),
      name_(p->name()),
      label_(p->param("label")),
      ignoreIfAny_((p->param("ignoreIfAny") == "true") ? true : false) {
    if (label_.isEmpty())
        label_ = name_;
}

void NodeQueryOption::build(NodeQuery* query) {
#ifdef _UI_NODEQUERY_DEBUG
    UI_FUNCTION_LOG
#endif

    // Node query part
    VProperty* prop = VConfig::instance()->find("node_query");
    Q_ASSERT(prop);
#ifdef _UI_NODEQUERY_DEBUG
    UiLog().dbg() << " load node_query";
#endif
    for (int i = 0; i < prop->children().size(); i++) {
        VProperty* p = prop->children().at(i);
        QString type = p->param("type");
#ifdef _UI_NODEQUERY_DEBUG
        UiLog().dbg() << "  name=" << p->strName() << " type=" << type.toStdString();
#endif
        NodeQueryOption* op = NodeQueryOptionFactory::create(p);
        Q_ASSERT(op);
        query->options_[op->name()] = op;
    }

    // Attr query part
    prop = VConfig::instance()->find("attribute_query");
    Q_ASSERT(prop);
#ifdef _UI_NODEQUERY_DEBUG
    UiLog().dbg() << " load attribute_query";
#endif
    for (int i = 0; i < prop->children().size(); i++) {
        VProperty* p = prop->children().at(i);
        if (p->name() == "attribute") {
            query->options_[p->name()] = NodeQueryOptionFactory::create(p);
        }
        else {
#ifdef _UI_NODEQUERY_DEBUG
            UiLog().dbg() << "  name=" << p->strName();
#endif
            // Q_ASSERT(def_["attribute"]->values().contains(p->name()));

            QList<VAttributeType*> typeLst;
            QList<NodeQueryOption*> opLst;

            QString types = p->param("types");
            if (types.isEmpty()) {
                VAttributeType* t = VAttributeType::find(p->name().toStdString());
                Q_ASSERT(t);
                typeLst << t;
            }
            else {
                Q_FOREACH (QString s, types.split("|")) {
                    VAttributeType* t = VAttributeType::find(s.toStdString());
                    Q_ASSERT(t);
                    typeLst << t;
                }
            }

            Q_ASSERT(!typeLst.empty());

            Q_FOREACH (VProperty* ch, p->children()) {
#ifdef _UI_NODEQUERY_DEBUG
                UiLog().dbg() << "  option: name=" << ch->strName() << " type=" << ch->param("type").toStdString();
#endif
                NodeQueryOption* op         = NodeQueryOptionFactory::create(ch);
                query->options_[ch->name()] = op;
                opLst << op;
                Q_ASSERT(op);
            }
            if (p->name() == "variable")
                query->attrGroup_[p->name()] = new NodeQueryVarAttrGroup(p->name(), typeLst, opLst);
            else
                query->attrGroup_[p->name()] = new NodeQueryAttrGroup(p->name(), typeLst, opLst);
        }
    }
}

//===============================================
//
// NodeQueryStringOption
//
//===============================================

NodeQueryStringOption::NodeQueryStringOption(VProperty* p)
    : NodeQueryOption(p),
      matchMode_(defaultMatchMode_),
      caseSensitive_(defaultCaseSensitive_) {
}

QString NodeQueryStringOption::query() const {
    QString s;
    if (!value_.isEmpty()) {
        s = name_ + " " + matchOperator() + " \'" + value_ + "\'";
    }
    return s;
}

void NodeQueryStringOption::swap(const NodeQueryOption* option) {
    const auto* op = static_cast<const NodeQueryStringOption*>(option);
    Q_ASSERT(op);

    value_         = op->value();
    matchMode_     = op->matchMode();
    caseSensitive_ = op->caseSensitive();
}

void NodeQueryStringOption::save(VSettings* vs) {
    if (value_.isEmpty() && matchMode_.mode() == defaultMatchMode_ && caseSensitive_ == defaultCaseSensitive_)
        return;

    vs->beginGroup(name_.toStdString());
    vs->put("value", value_.toStdString());
    vs->put("matchMode", matchMode_.toInt());
    vs->putAsBool("caseSensistive", caseSensitive_);
    vs->endGroup();
}

void NodeQueryStringOption::load(VSettings* vs) {
    if (!vs->contains(name().toStdString()))
        return;

    vs->beginGroup(name_.toStdString());
    value_ = QString::fromStdString(vs->get("value", value_.toStdString()));
    matchMode_.setMode(static_cast<StringMatchMode::Mode>(vs->get<int>("matchMode", matchMode_.toInt())));
    caseSensitive_ = vs->getAsBool("caseSensistive", caseSensitive_);
    vs->endGroup();
}

//===============================================
//
// NodeQueryListOption
//
//===============================================

NodeQueryListOption::NodeQueryListOption(VProperty* p)
    : NodeQueryOption(p),
      values_(p->param("values").split("|")),
      valueLabels_(p->param("labels").split("|")) {
    if (valueLabels_.count() == 1 && valueLabels_[0].isEmpty())
        valueLabels_ = values_;

    Q_ASSERT(valueLabels_.count() == values_.count());
}

QString NodeQueryListOption::query(QString op) const {
    QString s;
    if (!selection_.isEmpty()) {
        s = selection_.join(op);
    }
    return s;
}

void NodeQueryListOption::swap(const NodeQueryOption* option) {
    const auto* op = static_cast<const NodeQueryListOption*>(option);
    Q_ASSERT(op);
    selection_ = op->selection();
}

void NodeQueryListOption::save(VSettings* vs) {
    if (selection_.isEmpty())
        return;

    std::vector<std::string> v;
    Q_FOREACH (QString s, selection_)
        v.push_back(s.toStdString());

    vs->put(name_.toStdString(), v);
}

void NodeQueryListOption::load(VSettings* vs) {
    if (!vs->contains(name_.toStdString()))
        return;

    std::vector<std::string> v;
    vs->get(name_.toStdString(), v);

    selection_.clear();
    for (auto it = v.begin(); it != v.end(); ++it)
        selection_ << QString::fromStdString(*it);
}

//===============================================
//
// NodeQueryComboOption
//
//===============================================

NodeQueryComboOption::NodeQueryComboOption(VProperty* p)
    : NodeQueryOption(p),
      values_(p->param("values").split("|")),
      valueLabels_(p->param("labels").split("|")) {
    if (valueLabels_.count() == 1 && valueLabels_[0].isEmpty())
        valueLabels_ = values_;

    Q_ASSERT(values_.count() > 0);
    Q_ASSERT(valueLabels_.count() == values_.count());

    value_ = p->defaultValue().toString();
    if (!values_.contains(value_))
        value_ = values_[0];
}

QString NodeQueryComboOption::query() const {
    QString s;
    if (ignoreIfAny_ && value_ == "any")
        return s;

    if (!value_.isEmpty()) {
        s = name_ + " = " + " \'" + value_ + "\'";
    }
    return s;
}

void NodeQueryComboOption::setValue(QString val) {
    value_ = val;
}

void NodeQueryComboOption::swap(const NodeQueryOption* option) {
    const auto* op = static_cast<const NodeQueryComboOption*>(option);
    Q_ASSERT(op);
    value_ = op->value();
}

void NodeQueryComboOption::save(VSettings* vs) {
    vs->put(name_.toStdString(), value().toStdString());
}

void NodeQueryComboOption::load(VSettings*) {
}

//===============================================
//
// NodeQueryPeriodOption
//
//===============================================

NodeQueryPeriodOption::NodeQueryPeriodOption(VProperty* p) : NodeQueryOption(p) {
}

void NodeQueryPeriodOption::clear() {
    mode_   = NoMode;
    period_ = -1;
    periodUnits_.clear();
    fromDate_ = QDateTime();
    toDate_   = QDateTime();
    fromDate_.setTimeSpec(Qt::UTC);
    toDate_.setTimeSpec(Qt::UTC);
}

void NodeQueryPeriodOption::setLastPeriod(int period, QString periodUnits) {
    setPeriod(LastPeriodMode, period, periodUnits);
}

void NodeQueryPeriodOption::setOlderPeriod(int period, QString periodUnits) {
    setPeriod(OlderPeriodMode, period, periodUnits);
}

void NodeQueryPeriodOption::setPeriod(Mode mode, int interval, QString intervalUnits) {
    if (mode == LastPeriodMode || mode == OlderPeriodMode) {
        mode_        = mode;
        period_      = interval;
        periodUnits_ = intervalUnits;
        fromDate_    = QDateTime();
        toDate_      = QDateTime();
    }
}

void NodeQueryPeriodOption::setPeriod(QDateTime fromDate, QDateTime toDate) {
    mode_     = FixedPeriodMode;
    fromDate_ = fromDate;
    toDate_   = toDate;
    period_   = -1;
    periodUnits_.clear();
}

QString NodeQueryPeriodOption::query() const {
    QString s;
    if (mode_ == LastPeriodMode || mode_ == OlderPeriodMode) {
        if (period_ >= 0 && periodUnits_ >= nullptr) {
            QDateTime prev = QDateTime::currentDateTime();
            if (periodUnits_ == "minute")
                prev = prev.addSecs(-60 * period_);
            else if (periodUnits_ == "hour")
                prev = prev.addSecs(-3600 * period_);
            else if (periodUnits_ == "day")
                prev = prev.addDays(-period_);
            else if (periodUnits_ == "week")
                prev = prev.addDays(-7 * period_);
            else if (periodUnits_ == "month")
                prev = prev.addMonths(-period_);
            else if (periodUnits_ == "year")
                prev = prev.addYears(-period_);
            else
                return {};

            if (mode_ == LastPeriodMode) {
                s = name_ + " date::>= " + prev.toString(Qt::ISODate);
            }
            else {
                s = name_ + " date::< " + prev.toString(Qt::ISODate);
            }
        }
    }
    else if (mode_ == FixedPeriodMode) {
        if (fromDate_.isValid() && toDate_.isValid()) {
            s = name_ + " date::>= " + fromDate_.toString(Qt::ISODate) + " and " + name_ +
                " date::<= " + toDate_.toString(Qt::ISODate);
        }
    }
    return s;
}

QString NodeQueryPeriodOption::sqlQuery() const {
    QString s;
    if (mode_ == LastPeriodMode) {
        if (period_ >= 0 && periodUnits_ >= nullptr) {
            s = name_ + " >= now() -interval " + QString::number(period_) + " " + periodUnits_;
        }
    }
    else if (mode_ == OlderPeriodMode) {
        if (period_ >= 0 && periodUnits_ >= nullptr) {
            s = name_ + " < now() -interval " + QString::number(period_) + " " + periodUnits_;
        }
    }
    else if (mode_ == FixedPeriodMode) {
        if (fromDate_.isValid() && toDate_.isValid()) {
            s = name_ + " between " + fromDate_.toString(Qt::ISODate) + " and " + toDate_.toString(Qt::ISODate);
        }
    }
    return s;
}

void NodeQueryPeriodOption::swap(const NodeQueryOption* option) {
    const auto* op = static_cast<const NodeQueryPeriodOption*>(option);
    Q_ASSERT(op);

    if (op->mode_ == LastPeriodMode || op->mode_ == OlderPeriodMode) {
        setPeriod(op->mode_, op->period_, op->periodUnits_);
    }
    else if (op->mode_ == FixedPeriodMode) {
        setPeriod(op->fromDate(), op->toDate());
    }
    else
        mode_ = NoMode;
}

void NodeQueryPeriodOption::save(VSettings* vs) {
    if (mode_ == NoMode)
        return;

    vs->beginGroup(name_.toStdString());
    if (mode_ == LastPeriodMode) {
        vs->put("mode", "last");
        vs->put("value", period_);
        vs->put("units", periodUnits_.toStdString());
    }
    else if (mode_ == OlderPeriodMode) {
        vs->put("mode", "older");
        vs->put("value", period_);
        vs->put("units", periodUnits_.toStdString());
    }
    else if (mode_ == FixedPeriodMode) {
        vs->put("mode", "fixed");
        vs->put("from", fromDate_.toString(Qt::ISODate).toStdString());
        vs->put("to", toDate_.toString(Qt::ISODate).toStdString());
    }
    vs->endGroup();
}

void NodeQueryPeriodOption::load(VSettings* vs) {
    if (!vs->contains(name().toStdString()))
        return;

    vs->beginGroup(name_.toStdString());
    QString mode = QString::fromStdString(vs->get("mode", std::string()));

    if (mode == "last" || mode == "older") {
        mode_        = (mode == "last") ? LastPeriodMode : OlderPeriodMode;
        period_      = vs->get<int>("value", period_);
        periodUnits_ = QString::fromStdString(vs->get("units", periodUnits_.toStdString()));

        // Check period length
        if (period_ <= 0)
            clear();

        // Check period units
        if (periodUnits_ != "minute" && periodUnits_ != "hour" && periodUnits_ != "day" && periodUnits_ != "week" &&
            periodUnits_ != "month" && periodUnits_ != "year")
            clear();
    }
    else if (mode == "fixed") {
        mode_        = FixedPeriodMode;
        QString from = QString::fromStdString(vs->get("from", fromDate_.toString(Qt::ISODate).toStdString()));
        QString to   = QString::fromStdString(vs->get("to", fromDate_.toString(Qt::ISODate).toStdString()));

        fromDate_    = QDateTime::fromString(from, Qt::ISODate);
        fromDate_.setTimeSpec(Qt::UTC);
        toDate_ = QDateTime::fromString(to, Qt::ISODate);
        toDate_.setTimeSpec(Qt::UTC);

        // Check if dates are valis
        if (!fromDate_.isValid() || !fromDate_.isValid())
            clear();
    }
    else {
        clear();
    }

    vs->endGroup();
}

static NodeQueryOptionMaker<NodeQueryStringOption> maker1("string");
static NodeQueryOptionMaker<NodeQueryListOption> maker2("list");
static NodeQueryOptionMaker<NodeQueryComboOption> maker3("combo");
static NodeQueryOptionMaker<NodeQueryPeriodOption> maker4("period");
