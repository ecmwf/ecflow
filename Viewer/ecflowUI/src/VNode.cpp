/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VNode.hpp"

#include "AstCollateVNodesVisitor.hpp"
#include "ConnectState.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerHandler.hpp"
#include "TriggerCollector.hpp"
#include "TriggeredScanner.hpp"
#include "UiLog.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VDateAttr.hpp"
#include "VEventAttr.hpp"
#include "VFileInfo.hpp"
#include "VFilter.hpp"
#include "VGenVarAttr.hpp"
#include "VLabelAttr.hpp"
#include "VLateAttr.hpp"
#include "VLimitAttr.hpp"
#include "VLimiterAttr.hpp"
#include "VMeterAttr.hpp"
#include "VNState.hpp"
#include "VRepeatAttr.hpp"
#include "VSState.hpp"
#include "VTaskNode.hpp"
#include "VTimeAttr.hpp"
#include "VTriggerAttr.hpp"
#include "VUserVarAttr.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"

#define _UI_VNODE_DEBUG

// For a given node this class stores all the nodes that this node itself triggers.
// For memory efficiency we only store the AttributeFilterindex of the nodes not the pointers themselves.
class VNodeTriggerData {
public:
    std::vector<int> data_;
    std::map<std::string, std::vector<int>> eventData_;

    // std::vector<int> attr_;

    void get(VNode* node, TriggerCollector* tc) {
        VServer* s      = node->root();
        std::size_t num = data_.size();
        for (std::size_t i = 0; i < num; i++) {
            VItem* triggered = s->nodeAt(data_[i]);
            tc->add(triggered, nullptr, TriggerCollector::Normal);
        }
    }

    void getEvent(VNode* node, const std::string& eventName, std::vector<std::string>& res) {
        auto it = eventData_.find(eventName);
        if (it != eventData_.end()) {
            VServer* s = node->root();
            for (int i : it->second) {
                if (VNode* triggeredNode = s->nodeAt(i))
                    res.push_back(triggeredNode->absNodePath());
            }
        }
    }

    void add(VItem* triggered) {
        assert(triggered);
        VNode* triggeredNode = triggered->isNode();
        assert(triggeredNode);
        data_.push_back(triggeredNode->index());
    }

    void add(VItem* triggered, VAttribute* trigger) {
        static VAttributeType* eventType = nullptr;
        if (!eventType)
            eventType = VAttributeType::find("event");

        assert(trigger);
        assert(triggered);
        VNode* triggeredNode = triggered->isNode();
        assert(triggeredNode);

        // We only store the events
        if (trigger->type() == eventType) {
            eventData_[trigger->strName()].push_back(triggeredNode->index());
        }
    }
};

#if 0
class VNodeTriggerData
{
public:
    std::vector<std::pair<int,int> > data_;

    void get(VNode* node ,TriggerCollector* tc)
    {
        VServer* s=node->root();

        for(size_t i=0; i < data_.size(); i++)
        {
            VItemTmp_ptr triggered(VItemTmp::create(s->nodeAt(data_[i].first)));
            VItemTmp_ptr trigger;
            if(data_[i].second == -2)
            {
                trigger=VItemTmp::creaate(node);
            else
            {
                trigger=VItemTmp::create(VAttribute::makeFromId(node,data_[i].second));
            }

            tc->add(triggered,0,TriggerCollector::Normal,trigger);
            VItemTmp_ptr nullItem;
            tc->add(triggered,nullItem,TriggerCollector::Normal);
        }
    }
AttributeFilter
    void add(VItem* n)
    {
        assert(n);
        VNode* node=n->isNode();
        assert(node);
        data_.push_back(std::make_pair(node->index(),-2));
    }

    void add(VItem* triggered,VItem* trigger)
    {
        assert(triggered);
        assert(trigger);
        VNode* node=triggered->isNode();
        assert(node);
        VAttribute* attr=trigger->isAttribute();
        assert(attr);
        if(attr->id() >=0)
            data_.push_back(std::make_pair(node->index(),attr->id()));
    }
};
#endif

//=================================================
// VNode
//=================================================

VNode::VNode(VNode* parent, node_ptr node)
    : VItem(parent),
      node_(node),
// parent_(parent),
#if 0
    attrNum_(-1),
    cachedAttrNum_(-1),
#endif
      index_(-1),
      data_(nullptr) {
    if (parent_)
        parent_->addChild(this);

    if (node_)
        node_->set_graphic_ptr(this);

    // do not scan for attributes in a server
    if (parent_)
        scanAttr();
}

VNode::~VNode() {
    if (data_)
        delete data_;

    for (auto& i : attr_)
        delete i;
}

VServer* VNode::root() const {
    return server()->vRoot();
}

ServerHandler* VNode::server() const {
    return (parent_) ? (parent_->server()) : nullptr;
}

VNode* VNode::suite() const {
    if (isTopLevel())
        return const_cast<VNode*>(this);

    VNode* p = parent();
    while (p) {
        if (p->isTopLevel())
            return p;
        p = p->parent();
    }

    assert(0);

    return nullptr;
}

const ecf::Calendar& VNode::calendar() const {
    if (VNode* sn = suite()) {
        if (Suite* s = sn->node()->isSuite()) {
            return s->calendar();
        }
    }

    static ecf::Calendar emptyCal;
    return emptyCal;
}

bool VNode::isTopLevel() const {
    return isSuite();
    // return (parent_ && parent_->isServer());
    // return (node_)?(node_->isSuite() != NULL):false;
}

void VNode::clear() {
    children_.clear();
#if 0
    attrNum_=-1,
	cachedAttrNum_=-1;
#endif
}

bool VNode::hasAccessed() const {
    return true; //! name_.empty();
}

//------------------------
// Attributes
//------------------------

void VNode::scanAttr() {
    std::vector<VAttribute*> v;
    VAttributeType::scan(this, v);
    std::size_t n = v.size();
    attr_.reserve(n);
    for (std::size_t i = 0; i < n; i++)
        attr_.push_back(v[i]);
}

void VNode::rescanAttr() {
    for (auto& i : attr_)
        delete i;

    attr_ = std::vector<VAttribute*>();
    scanAttr();
}

int VNode::attrNum(AttributeFilter* filter) const {
    if (filter) {
        int n = 0;
        for (auto i : attr_) {
            if (filter->isSet(i->type()) || filter->forceShowAttr() == i)
                n++;
        }
        return n;
    }

    return attr_.size();
}

VAttribute* VNode::attribute(int row, AttributeFilter* filter) const {
    assert(row >= 0);

    if (filter) {
        int n    = 0;
        auto cnt = static_cast<int>(attr_.size());
        if (row >= cnt)
            return nullptr;

        for (int i = 0; i < cnt; i++) {
            if (filter->isSet(attr_[i]->type()) || filter->forceShowAttr() == attr_[i]) {
                if (n == row) {
                    return attr_[i];
                }
                n++;
            }
        }
    }
    else if (row < static_cast<int>(attr_.size())) {
        return attr_[row];
    }

    return nullptr;
}

VAttribute* VNode::attributeForType(int row, VAttributeType* t) const {
    assert(row >= 0);

    auto cnt = static_cast<int>(attr_.size());
    if (row >= cnt)
        return nullptr;

    bool hasIt = false;
    int rowCnt = 0;
    for (int i = 0; i < cnt; i++) {
        if (attr_[i]->type() == t) {
            if (rowCnt == row) {
                return attr_[i];
            }
            rowCnt++;
            hasIt = true;
        }
        else if (hasIt) {
            return nullptr;
        }
    }

    return nullptr;
}

int VNode::indexOfAttribute(const VAttribute* a, AttributeFilter* filter) const {
    if (filter) {
        int n    = 0;
        auto cnt = static_cast<int>(attr_.size());
        for (int i = 0; i < cnt; i++) {
            if (filter->isSet(attr_[i]->type()) || filter->forceShowAttr() == attr_[i]) {
                if (a == attr_[i])
                    return n;

                n++;
            }
        }
    }
    else {
        auto cnt = static_cast<int>(attr_.size());
        for (int i = 0; i < cnt; i++) {
            if (a == attr_[i])
                return i;
        }
    }

    return -1;
}

VAttribute* VNode::findAttribute(const std::string& typeName, const std::string& name) {
    VAttributeType* t = VAttributeType::find(typeName);
    Q_ASSERT(t);
    return (t) ? findAttribute(t, name) : nullptr;
}

VAttribute* VNode::findAttribute(VAttributeType* t, const std::string& name) {
    Q_ASSERT(t);

    auto cnt     = static_cast<int>(attr_.size());
    bool hasType = false;
    for (int i = 0; i < cnt; i++) {
        if (attr_[i]->type() == t) {
            if (attr_[i]->strName() == name)
                return attr_[i];
            hasType = true;
        }
        else if (hasType)
            return nullptr;
    }
    return nullptr;
}

VAttribute* VNode::findAttribute(QStringList aData) {
    std::size_t cnt = attr_.size();
    for (std::size_t i = 0; i < cnt; i++) {
        if (attr_[i]->sameAs(aData))
            return attr_[i];
    }
    return nullptr;
}

void VNode::findAttributes(VAttributeType* t, std::vector<VAttribute*>& v) {
    std::size_t cnt = attr_.size();
    bool hasType    = false;
    for (std::size_t i = 0; i < cnt; i++) {
        if (attr_[i]->type() == t) {
            v.push_back(attr_[i]);
            hasType = true;
        }
        else if (hasType)
            return;
    }
}

void VNode::addChild(VNode* vn) {
    children_.push_back(vn);
}

void VNode::removeChild(VNode* vn) {
    auto it = std::find(children_.begin(), children_.end(), vn);
    if (it != children_.end()) {
        children_.erase(it);
    }
}

VNode* VNode::childAt(int index) const {
    assert(index >= 0 && index < static_cast<int>(children_.size()));
    return children_[index];
}

int VNode::indexOfChild(const VNode* vn) const {
    for (unsigned int i = 0; i < children_.size(); i++) {
        if (children_[i] == vn)
            return i;
    }

    return -1;
}

int VNode::indexOfChild(node_ptr n) const {
    for (unsigned int i = 0; i < children_.size(); i++) {
        if (children_[i]->node() == n)
            return i;
    }

    return -1;
}

VNode* VNode::findChild(const std::string& name) const {
    for (auto i : children_) {
        if (i->sameName(name))
            return i;
    }
    return nullptr;
}

void VNode::collect(std::vector<VNode*>& vec) const {
    for (int i = 0; i < numOfChildren(); i++) {
        vec.push_back(children_.at(i));
        children_[i]->collect(vec);
    }
}

void VNode::collectAbortedTasks(std::vector<VNode*>& vec) const {
    for (int i = 0; i < numOfChildren(); i++) {
        if (children_[i]->isTask() && children_[i]->isAborted()) {
            vec.push_back(children_[i]);
        }
        children_[i]->collectAbortedTasks(vec);
    }
}

int VNode::tryNo() const {
    std::string v = genVariable("ECF_TRYNO");
    if (v.empty())
        return 0;

    return ecf::convert_to<int>(v);
}

VNode* VNode::find(const std::vector<std::string>& pathVec) {
    if (pathVec.size() == 0)
        return this;

    if (pathVec.size() == 1) {
        return findChild(pathVec.at(0));
    }

    std::vector<std::string> rest(pathVec.begin() + 1, pathVec.end());
    VNode* n = findChild(pathVec.at(0));

    return n ? n->find(rest) : nullptr;
}

std::string VNode::genVariable(const std::string& key) const {
    std::string val;
    if (node_)
        node_->findGenVariableValue(key, val);
    return val;
}

std::string VNode::findVariable(const std::string& key, bool substitute) const {
    std::string val;
    if (!node_)
        return val;

    // should set the def mutex because variableSubsitution
    // might need information from the defs

    const Variable& var = node_->findVariable(key);
    if (!var.empty()) {
        val = var.theValue();
        if (substitute) {
            node_->variableSubsitution(val);
        }
        return val;
    }
    const Variable& gvar = node_->findGenVariable(key);
    if (!gvar.empty()) {
        val = gvar.theValue();
        if (substitute) {
            node_->variableSubsitution(val);
        }
        return val;
    }

    return val;
}

std::string VNode::findInheritedVariable(const std::string& key, bool substitute) const {
    std::string val;
    if (!node_)
        return val;

    // should set the def mutex because it might need information from the defs
    // but it would be hang the GUI e.g. in the table view

    if (node_->findParentVariableValue(key, val)) {
        if (substitute) {
            // this must resolve ECF_MICRO all the time
            node_->variableSubsitution(val);
        }
        return val;
    }

    return val;
}

void VNode::collectInheritedVariableNames(std::set<std::string>& vars) const {
    if (!node_)
        return;

    std::vector<Variable> v, gv;
    variables(v);
    genVariables(gv);

    for (auto& i : v) {
        vars.insert(i.name());
    }

    for (auto& i : gv) {
        vars.insert(i.name());
    }

    // Try to find it in the parent
    if (parent()) {
        parent()->collectInheritedVariableNames(vars);
    }
}

bool VNode::substituteVariableValue(std::string& val) const {
    if (!node_)
        return false;

    // should set the def mutex because variableSubsitution
    // might need information from the defs
    return node_->variableSubsitution(val);
}

int VNode::variablesNum() const {
    if (node_.get())
        return static_cast<int>(node_->variables().size());

    return 0;
}

int VNode::genVariablesNum() const {
    std::vector<Variable> gv;

    if (node_) {
        node_->gen_variables(gv);
        return static_cast<int>(gv.size());
    }

    return 0;
}

void VNode::variables(std::vector<Variable>& vars) const {
    vars.clear();
    if (node_)
        vars = node_->variables();
}

void VNode::genVariables(std::vector<Variable>& genVars) const {
    genVars.clear();
    if (node_)
        node_->gen_variables(genVars);
}

std::string VNode::fullPath() const {
    return absNodePath();
}

std::string VNode::absNodePath() const {
    return (node_) ? node_->absNodePath() : "";
}

bool VNode::pathEndMatch(const std::string& relPath) const {
    std::string pPath = absNodePath();
    if (relPath.empty() || pPath.size() < relPath.size()) {
        return false;
    }
    else {
        return (pPath.substr(pPath.size() - relPath.size(), std::string::npos) == relPath);
    }
    return false;
}

bool VNode::sameContents(VItem* item) const {
    return item == this;
}

bool VNode::sameName(const std::string& name) const {
    return (strName() == name) ? true : false;
    // return (node_)?(node_->name() == name):false;
}

std::string VNode::strName() const {
    if (node_ && node_.get())
        return node_->name();

    return {};
    /*
    if(name_.empty())
    {
            if(node_)
                    name_=node_->name();
    }
    return name_;*/
}

QString VNode::name() const {
    return QString::fromStdString(strName());
    // return (node_)?QString::fromStdString(node_->name()):QString();
}

std::string VNode::serverName() const {
    if (ServerHandler* s = server()) {
        return s->name();
    }
    return {};
}

QString VNode::stateName() {
    return VNState::toName(this);
}

bool VNode::isDefaultStateComplete() {
    if (node_)
        return (node_->defStatus() == DState::COMPLETE);

    return false;
}

QString VNode::defaultStateName() {
    return VNState::toDefaultStateName(this);
}

QString VNode::serverStateName() {
    return {""};
}

bool VNode::isSuspended() const {
    return (node_ && node_->isSuspended());
}

bool VNode::isAborted() const {
    return (node_ && node_->state() == NState::ABORTED && !node_->isSuspended());
}

bool VNode::isSubmitted() const {
    return (node_ && node_->state() == NState::SUBMITTED);
}

bool VNode::isActive() const {
    return (node_ && node_->state() == NState::ACTIVE);
}

QColor VNode::stateColour() const {
    return VNState::toColour(this);
}

QColor VNode::realStateColour() const {
    return VNState::toRealColour(this);
}

QColor VNode::stateFontColour() const {
    return VNState::toFontColour(this);
}

QColor VNode::typeFontColour() const {
    return VNState::toTypeColour(this);
}

bool VNode::userLogServer(std::string& host, std::string& port) {
    if (ServerHandler* sh = server()) {
        host = sh->conf()->stringValue(VServerSettings::UserLogServerHost).toStdString();
        if (!host.empty()) {
            port = sh->conf()->stringValue(VServerSettings::UserLogServerPort).toStdString();
            return !port.empty();
        }
    }
    return false;
}

bool VNode::logServer(std::string& host, std::string& port) {
    if (!node_)
        return false;

    host              = findInheritedVariable("ECF_LOGHOST", true);
    port              = findInheritedVariable("ECF_LOGPORT");
    std::string micro = findInheritedVariable("ECF_MICRO");
    if (!host.empty() && !port.empty() && (micro.empty() || host.find(micro) == std::string::npos)) {
        return true;
    }

    return false;
}

std::vector<VNode*> VNode::ancestors(SortMode sortMode) {
    std::vector<VNode*> nodes;

    VNode* n = this;

    nodes.push_back(n);
    n = n->parent();

    if (sortMode == ChildToParentSort) {
        while (n) {
            nodes.push_back(n);
            n = n->parent();
        }
    }

    else if (sortMode == ParentToChildSort) {
        while (n) {
            nodes.insert(nodes.begin(), n);
            n = n->parent();
        }
    }

    return nodes;
}

VNode* VNode::ancestorAt(int idx, SortMode sortMode) {
    if (sortMode == ChildToParentSort && idx == 0)
        return this;

    std::vector<VNode*> nodes = ancestors(sortMode);
    if (static_cast<int>(nodes.size()) > idx) {
        return nodes[idx];
    }

    return nullptr;
}

const std::string& VNode::nodeType() {
    static std::string suiteStr("suite");
    static std::string familyStr("family");
    static std::string taskStr("task");
    static std::string defaultStr("node");
    static std::string serverStr("server");

    if (isServer())
        return serverStr;

    node_ptr np = node();

    if (!np || !np.get())
        return defaultStr;

    if (np->isSuite())
        return suiteStr;
    else if (np->isFamily())
        return familyStr;
    else if (np->isTask())
        return taskStr;

    return defaultStr;
}

#if 0
bool VNode::isFamily() const
{
    node_ptr np=node();

    if(!np || !np.get())
        return false;

    return (np->isFamily())?true:false;
}

bool VNode::isAlias() const
{
    node_ptr np=node();

    if(!np || !np.get())
        return false;

    return (np->isAlias())?true:false;
}
#endif

std::string VNode::flagsAsStr() const {
    return (node_) ? node_->flag().to_string() : std::string();
}

bool VNode::isFlagSet(ecf::Flag::Type f) const {
    if (node_) {
        return node_->flag().is_set(f);
    }
    return false;
}

void VNode::why(std::vector<std::string>& bottomUp, std::vector<std::string>& topDown) const {
    if (node_) {
        node_->bottom_up_why(bottomUp, true);
        if (isFamily() || isSuite()) {
            node_->top_down_why(topDown, true);
        }
    }
}

const std::string& VNode::abortedReason() const {
    if (node_) {
        return node_->abortedReason();
    }

    static std::string emptyStr;
    return emptyStr;
}

void VNode::statusChangeTime(QString& sct) const {
    if (node_) {
        boost::posix_time::ptime t = node_->state_change_time();
        std::string s              = boost::posix_time::to_simple_string(t);
        sct                        = QString::fromStdString(s);
    }
}

unsigned int VNode::statusChangeTime() const {
    if (node_) {
        static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::ptime t = node_->state_change_time();
        boost::posix_time::time_duration diff(t - epoch);
        return diff.ticks() / diff.ticks_per_second();
    }
    return 0;
}

QString VNode::toolTip() {
    QString txt = "<b>Name</b>: " + name() + "<br>";
    txt += "<b>Path</b>: " + QString::fromStdString(absNodePath()) + "<br>";
    txt += "<b>Type</b>: " + QString::fromStdString(nodeType()) + "<br>";

    txt += "<b>Status</b>: " + stateName();
    if (isSuspended())
        txt += " (" + VNState::toRealStateName(this) + ")";

    txt += "<br>";
    txt += "<b>Default status</b>: " + defaultStateName() + "<br>";

    txt += "<b>Server:</b> " + QString::fromStdString(server()->name()) + "<br>";
    txt += "<b>Host</b>: " + QString::fromStdString(server()->host());
    txt += " <b>Port</b>: " + QString::fromStdString(server()->port());

    QString rs = QString::fromStdString(abortedReason());
    if (!rs.isEmpty())
        txt += "<br><b>Aborted reason</b>:" + rs;

    return txt;
}

//===========================================================
// Triggers
//===========================================================

void VNode::triggerExpr(std::string& trigger, std::string& complete) const {
    VTriggerAttr::expressions(this, trigger, complete);
}

// Collect the information about all the triggers triggering this node
void VNode::triggers(TriggerCollector* tlc) {
    VItem* nullItem = nullptr;
    if (!node_) {
        return;
    }

    // Check the node itself
    // if(tlr.self())
    {
        // find nodes, event, meters and variables triggering this node
        if (node_ && !node_->isSuite()) {
            std::vector<VItem*> theVec;
            AstCollateVNodesVisitor astVisitor(theVec);

            // Collect the nodes from ast
            if (node_->completeAst())
                node_->completeAst()->accept(astVisitor);

            if (node_->triggerAst())
                node_->triggerAst()->accept(astVisitor);

            // Add the found items to the collector
            for (auto& it : theVec) {
                tlc->add(it, nullItem, TriggerCollector::Normal);
            }
        }

        // Check other attributes

        // Limiters
        const std::vector<InLimit>& limiterVec = node_->inlimits();
        for (auto& limiter : limiterVec) {
            if (Limit* lim = node_->findLimitViaInLimit(limiter)) {
                if (Node* limParent = lim->node()) {
                    if (VNode* p = root()->toVNode(limParent)) {
                        if (VAttribute* n = p->getLimit(lim->name())) {
                            tlc->add(n, nullItem, TriggerCollector::Normal);
                        }
                    }
                }
            }
        }

        // Date
        std::vector<VAttribute*> dateVec;
        findAttributes(VAttributeType::find("date"), dateVec);
        size_t n = dateVec.size();
        for (std::size_t i = 0; i < n; i++) {
            tlc->add(dateVec[i], nullItem, TriggerCollector::Normal);
        }

        // Time
        std::vector<VAttribute*> timeVec;
        findAttributes(VAttributeType::find("time"), timeVec);
        n = timeVec.size();
        for (std::size_t i = 0; i < n; i++) {
            tlc->add(timeVec[i], nullItem, TriggerCollector::Normal);
        }
    }

    if (tlc->scanParents()) {
        VNode* p = parent();
        while (p) {
            TriggerParentCollector tpc(p, tlc);
            p->triggers(&tpc);
            p = p->parent();
        }
    }

    if (tlc->scanKids()) {
        triggersInChildren(this, this, tlc);
    }
}

// Collect the triggers triggering node n in the children of parent
void VNode::triggersInChildren(VNode* n, VNode* p, TriggerCollector* tlc) {
    for (auto& i : p->children_) {
        TriggerChildCollector tcc(n, i, tlc);
        i->triggers(&tcc);
        triggersInChildren(n, i, tlc);
    }
}

void VNode::clearTriggerData() {
    if (data_)
        delete data_;
    data_ = nullptr;
}

// These are called during the scan for triggered nodes
void VNode::addTriggeredData(VItem* n) {
    if (!data_)
        data_ = new VNodeTriggerData;

    data_->add(n);
}

void VNode::addTriggeredData(VItem* triggered, VAttribute* trigger) {
    if (!data_)
        data_ = new VNodeTriggerData;

    assert(trigger->parent() == this);
    data_->add(triggered, trigger);
}

// Collect the information about all the nodes this node or its attributes trigger
void VNode::triggered(TriggerCollector* tlc, TriggeredScanner* scanner) {
    if (scanner && !root()->triggeredScanned()) {
        // unsigned int aNum=VAttribute::totalNum();
        scanner->start(root());
        root()->setTriggeredScanned(true);
        // assert(aNum == VAttribute::totalNum());
    }

    // Get the nodes directly triggered by this node
    if (data_)
        data_->get(this, tlc);

    if (tlc->scanParents()) {
        VNode* p = parent();
        while (p) {
            TriggerParentCollector tpc(p, tlc);
            p->triggered(&tpc);
            p = p->parent();
        }
    }

    if (tlc->scanKids()) {
        triggeredByChildren(this, this, tlc);
    }
}

void VNode::triggeredByChildren(VNode* n, VNode* p, TriggerCollector* tlc) {
    for (auto& i : p->children_) {
        TriggerChildCollector tcc(n, i, tlc);
        i->triggered(&tcc);
        triggeredByChildren(n, i, tlc);
    }
}

void VNode::triggeredByEvent(const std::string& name,
                             std::vector<std::string>& triggeredVec,
                             TriggeredScanner* scanner) {
    if (scanner && !root()->triggeredScanned()) {
        // unsigned int aNum=VAttribute::totalNum();
        scanner->start(root());
        root()->setTriggeredScanned(true);
        // assert(aNum == VAttribute::totalNum());
    }

    // Get the nodes directly triggered by this event
    if (data_)
        data_->getEvent(this, name, triggeredVec);
}

VAttribute* VNode::getLimit(const std::string& name) {
    VAttribute* nullItem = nullptr;
    std::vector<VAttribute*> limit;
    findAttributes(VAttributeType::find("limit"), limit);
    std::size_t limitNum = limit.size();
    for (std::size_t i = 0; i < limitNum; i++) {
        if (limit[i]->strName() == name) {
            return limit[i];
        }
    }

    return nullItem;
}

QString VNode::nodeMenuMode() const {
    ServerHandler* s = server();
    Q_ASSERT(s);
    return s->nodeMenuMode();
}

QString VNode::defStatusNodeMenuMode() const {
    ServerHandler* s = server();
    Q_ASSERT(s);
    return s->defStatusNodeMenuMode();
}

const std::string& VSuiteNode::typeName() const {
    static std::string t("suite");
    return t;
}

const std::string& VFamilyNode::typeName() const {
    static std::string t("family");
    return t;
}

const std::string& VAliasNode::typeName() const {
    static std::string t("alias");
    return t;
}

void VNode::print() {
    UiLog().dbg() << name() << " " << children_.size();
    for (auto& i : children_)
        i->print();
}

//=================================================
//
// VNodeRoot - this represents the server
//
//=================================================

VServer::VServer(ServerHandler* server)
    : VNode(nullptr, node_ptr()),
      server_(server),
      totalNum_(0),
      triggeredScanned_(false) {
    // Attributes are not scannedfor servers
    Q_ASSERT(attr_.empty());
}

VServer::~VServer() {
    clear();
}

int VServer::totalNumOfTopLevel(VNode* n) const {
    if (!n->isTopLevel())
        return -1;

    int idx = indexOfChild(n);
    if (idx != -1)
        return totalNumOfTopLevel(idx);

    return -1;
}

int VServer::totalNumOfTopLevel(int idx) const {
    assert(totalNumInChild_.size() == children_.size());

    if (idx >= 0 && idx < static_cast<int>(totalNumInChild_.size())) {
        return totalNumInChild_[idx];
    }

    return -1;
}

int VServer::totalNumOfTopLevel(const std::string& name) const {
    for (size_t i = 0; i < children_.size(); i++) {
        if (name == children_[i]->strName())
            return totalNumOfTopLevel(i);
    }
    return -1;
}

//--------------------------------
// Clear
//--------------------------------

// Clear the whole contents
void VServer::clear() {
    if (totalNum_ == 0)
        return;

    cache_.clear();
    prevNodeState_.clear();

    // clear attrubutes
    Q_ASSERT(attr_.empty());
    for (auto& i : attrForSearch_)
        delete i;

    attrForSearch_.clear();

    bool hasNotifications = server_->conf()->notificationsEnabled();

    // Delete the children nodes. It will recursively delete all the nodes. It also saves the prevNodeState!!
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        deleteNode(*it, hasNotifications);
    }

    // Clear the children vector
    children_.clear();

    totalNumInChild_.clear();

    // A sanity check
    assert(totalNum_ == 0);

    // Deallocate the nodes vector
    nodes_ = std::vector<VNode*>();

    triggeredScanned_ = false;
}

// Delete a particular node
void VServer::deleteNode(VNode* node, bool hasNotifications) {
    for (int i = 0; i < node->numOfChildren(); i++) {
        deleteNode(node->childAt(i), hasNotifications);
    }

    // If there are notifications we need to save previous state
    if (hasNotifications) {
        if (node->node_->isTask()) {
            VNodeInternalState st;
            node->internalState(st);
            prevNodeState_[node->absNodePath()] = st;
        }
    }

    delete node;
    totalNum_--;
}

//------------------------------------------
// Variables
//------------------------------------------

int VServer::variablesNum() const {
    return cache_.vars_.size();
}

int VServer::genVariablesNum() const {
    return cache_.genVars_.size();
}

void VServer::variables(std::vector<Variable>& vars) const {
    vars.clear();
    vars = cache_.vars_;
}

void VServer::genVariables(std::vector<Variable>& vars) const {
    vars.clear();
    vars = cache_.genVars_;
}

std::string VServer::genVariable(const std::string& key) const {
    std::string val;

    for (const auto& genVar : cache_.genVars_) {
        if (genVar.name() == key)
            val = genVar.theValue();
    }

    return val;
}

//------------------------------------------
// Find
//------------------------------------------

VNode* VServer::toVNode(const Node* nc) const {
    return static_cast<VNode*>(nc->graphic_ptr());
}

VNode* VServer::find(const std::string& fullPath) {
    if (fullPath.empty())
        return nullptr;

    if (fullPath == "/")
        return this;

    std::vector<std::string> pathVec;
    ecf::algorithm::split(pathVec, fullPath, "/");

    if (pathVec.size() > 0 && pathVec.at(0).empty()) {
        pathVec.erase(pathVec.begin());
    }

    return VNode::find(pathVec);
}

std::string VServer::findVariable(const std::string& key, bool substitute) const {
    std::string val;

    // Search user variables first
    for (const auto& var : cache_.vars_) {
        if (var.name() == key) {
            val = var.theValue();
            if (substitute)
                substituteVariableValue(val);

            return val;
        }
    }

    // Then search server variables
    for (const auto& genVar : cache_.genVars_) {
        if (genVar.name() == key) {
            val = genVar.theValue();
            if (substitute)
                substituteVariableValue(val);

            return val;
        }
    }

    return val;
}

std::string VServer::findInheritedVariable(const std::string& key, bool substitute) const {
    return findVariable(key, substitute);
}

bool VServer::substituteVariableValue(std::string& val) const {
    if (val.empty())
        return false;

    ServerDefsAccess defsAccess(server_); // will reliquish its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (!defs)
        return false;

    return defs->server().variableSubsitution(val);
}

//----------------------------------------------
// Scan
//----------------------------------------------

// Clear the contents and get the number of children (suites)
// the server contain. At this point we do not build the tree.
void VServer::beginScan(VServerChange& /*change*/) {
    // Clear the contents
    clear();

    // Get the Defs.
    {
        ServerDefsAccess defsAccess(server_); // will reliquish its resources on destruction
        defs_ptr defs = defsAccess.defs();
        if (!defs)
            return;

        // We need to update the cached server variables
        updateCache(defs);
    }
}

// Build the whole tree.
void VServer::endScan() {
    totalNum_ = 0;

    // Get the Defs
    {
        ServerDefsAccess defsAccess(server_); // will reliquish its resources on destruction
        defs_ptr defs = defsAccess.defs();
        if (!defs)
            return;

        bool hasNotifications = server_->conf()->notificationsEnabled();

        // Scan the suits.This will recursively scan all nodes in the tree.
        const std::vector<suite_ptr>& suites = defs->suiteVec();

        for (const auto& suite : suites) {
            VNode* vn = new VSuiteNode(this, suite);
            totalNum_++;
            scan(vn, hasNotifications);
        }
    }

    if (totalNum_ > 0) {
        nodes_.reserve(totalNum_);
        collect(nodes_);
        for (size_t i = 0; i < nodes_.size(); i++)
            nodes_[i]->setIndex(i);
    }
}

void VServer::scan(VNode* node, bool hasNotifications) {
    int prevTotalNum = totalNum_;

    std::vector<node_ptr> nodes;
    node->node()->immediateChildren(nodes);

    // totalNum_+=nodes.size();

    // Preallocates the children vector to the reqiuired size to save memory.
    if (nodes.size() > 0) {
        node->children_.reserve(nodes.size());
    }

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        VNode* vn = nullptr;
        if ((*it)->isTask()) {
            vn = new VTaskNode(node, *it);

            // If there are notifications we need to check them using the previous state
            if (hasNotifications) {
                std::string path = (*it)->absNodePath();
                auto itP         = prevNodeState_.find(path);
                if (itP != prevNodeState_.end())
                    vn->check(server_->conf(), itP->second);
            }
        }
        else if ((*it)->isFamily()) {
            vn = new VFamilyNode(node, *it);
        }
        else if ((*it)->isAlias()) {
            vn = new VAliasNode(node, *it);
        }
        else {
            assert(0);
        }
        totalNum_++;
        scan(vn, hasNotifications);
    }

    if (node->parent() == this) {
        totalNumInChild_.push_back(totalNum_ - prevTotalNum);
    }
}

VNode* VServer::nodeAt(int idx) const {
    assert(idx >= 0 && idx < static_cast<int>(nodes_.size()));
    return nodes_.at(idx);
}

//----------------------------------------------
// Update
//----------------------------------------------

void VServer::beginUpdate(VNode* node, const std::vector<ecf::Aspect::Type>& aspect, VNodeChange& /*change*/) {
#if 0
    //If the number of nodes changed we need to rescan the whole server-tree
    if(std::find(aspect.begin(),aspect.end(),ecf::Aspect::ADD_REMOVE_NODE) != aspect.end())
    {
        change.rescan_=true;
    }
#endif
    // NOTE: when this function is called the real node (Node) has already been updated. However the
    // views do not know about this change. So at this point (this is the begin step of the update)
    // all VNode functions have to return the values valid before the update happened!!!!!!!
    // The main goal of this function is to cleverly provide the views with some information about the nature of the
    // update.

    // Update the generated variables. There is no notification about their change so we have to do it!!!
    if (node->node()) {
        Suite* s = nullptr;
        s        = node->node()->isSuite();
        if (!s) {
            s = node->node()->suite();
        }

        if (s && s->begun()) {
            node->node()->update_generated_variables();
            s->update_generated_variables();
        }

        if (node->nodeType() == "task") {
            bool stateCh = (std::find(aspect.begin(), aspect.end(), ecf::Aspect::STATE) != aspect.end());
            node->check(server_->conf(), stateCh);
        }
    }

    //-------------------------------------------------------------------------
    // The trigger relations might be changed. We need to clear the mapped
    // trigger relations globally if:
    //    -a trigger expression changed (the aspect is EXPR_TRIGGER)
    //    -a trigger expression was added or removed (the aspect is ADD_REMOVE_ATTR)
    //--------------------------------------------------------------------------

    for (auto it : aspect) {
        if (it == ecf::Aspect::ADD_REMOVE_ATTR) {
            // we need to rescan the attributes belong to the node
            node->rescanAttr();
            clearNodeTriggerData();
            return;
        }
        else if (it == ecf::Aspect::EXPR_TRIGGER) {
            clearNodeTriggerData();
        }
    }

    // In any other cases it is just a simple update (value or status changed)
}

//-------------------------------------------------------------------------------------------
// Finishes the update. It has to be consistent with the changes registered in VNodeChange.
// If anything does not match we return false that will call reset!!!
//-------------------------------------------------------------------------------------------

void VServer::endUpdate(VNode* /*node*/,
                        const std::vector<ecf::Aspect::Type>& /*aspect*/,
                        const VNodeChange& /*change*/) {
}

void VServer::beginUpdate(const std::vector<ecf::Aspect::Type>& aspect) {
    // We need to update the cached server variables
    if (std::find(aspect.begin(), aspect.end(), ecf::Aspect::SERVER_VARIABLE) != aspect.end() ||
        std::find(aspect.begin(), aspect.end(), ecf::Aspect::FLAG) != aspect.end()) {
        // This will use the defs!!!
        updateCache();
    }
}

// NOTE: server attributes are only used for search, because the tree is not yet
// able to manage them. So we always keep the attr_ vector empty and use attrForSearch_
// when server attribute data is needed for the search.
const std::vector<VAttribute*>& VServer::attrForSearch() {
    Q_ASSERT(attr_.empty());
    for (auto& i : attrForSearch_)
        delete i;

    attrForSearch_.clear();

    // this call will update the attr_ vector !!
    rescanAttr();

    attrForSearch_ = attr_;
    attr_.clear();
    return attrForSearch_;
}

void VServer::suites(std::vector<std::string>& sv) {
    for (int i = 0; i < numOfChildren(); i++) {
        sv.push_back(children_.at(i)->strName());
    }
}

std::string VServer::strName() const {
    if (server_)
        return server_->name();

    return {};

    /*if(name_.empty())
    {
            if(server_)
                    name_=server_->name();
    }
    return name_;*/
}

QString VServer::stateName() {
    if (VSState::isRunningState(server_)) {
        return VNState::toName(server_);
    }

    return VSState::toName(server_);
}

QString VServer::defaultStateName() {
    return stateName();
}

QString VServer::serverStateName() {
    return VSState::toName(server_);
}

bool VServer::isSuspended() const {
    return false;
}

QColor VServer::stateColour() const {
    if (VSState::isRunningState(server_)) {
        return VNState::toColour(server_);
    }

    return VSState::toColour(server_);
}

QColor VServer::stateFontColour() const {
    if (VSState::isRunningState(server_)) {
        return VNState::toFontColour(server_);
    }

    return VSState::toFontColour(server_);
}

void VServer::why(std::vector<std::string>& theReasonWhy) const {
    ServerDefsAccess defsAccess(server_); // will reliquish its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (!defs)
        return;

    defs->why(theReasonWhy, true);
}

std::string VServer::flagsAsStr() const {
    return cache_.flag_.to_string();
}

bool VServer::isFlagSet(ecf::Flag::Type f) const {
    return cache_.flag_.is_set(f);
}

QString VServer::logOrCheckpointError() const {
    std::string s;
    if (isFlagSet(ecf::Flag::LOG_ERROR)) {
        s = findVariable("ECF_LOG_ERROR", true);
    }
    if (isFlagSet(ecf::Flag::CHECKPT_ERROR)) {
        if (!s.empty())
            s += " ";
        s += findVariable("ECF_CHECKPT_ERROR", true);
    }
    return QString::fromStdString(s);
}

void VServer::updateCache() {
    cache_.clear();

    ServerDefsAccess defsAccess(server_); // will reliquish its resources on destruction
    defs_ptr defs = defsAccess.defs();
    if (!defs)
        return;

    updateCache(defs);
}

void VServer::updateCache(defs_ptr defs) {
    cache_.vars_    = defs->server().user_variables();
    cache_.genVars_ = defs->server().server_variables();
    cache_.flag_    = defs->flag();
}

const std::string& VServer::typeName() const {
    static std::string t("server");
    return t;
}

QString VServer::toolTip() {
    QString txt = "<b>Server</b>: " + QString::fromStdString(server_->name()) + "<br>";
    txt += "<b>Host</b>: " + QString::fromStdString(server_->host());
    txt += " <b>Port</b>: " + QString::fromStdString(server_->port()) + "<br>";

    if (server_->isSsl()) {
        txt += "<b>SSL</b>: enabled<br>";
    }

    if (!server_->user().empty()) {
        txt += "<b>Custom user</b>: " + QString::fromStdString(server_->user()) + "<br>";
    }

    ConnectState* st = server_->connectState();
    QColor colErr(255, 95, 95);

    if (server_->activity() == ServerHandler::LoadActivity) {
        txt += "<b>Server is being loaded!</b><br>";
        // txt+="<b>Started</b>: " + VFileInfo::formatDateAgo(st->lastConnectTime()) + "<br>";
    }
    else {
        if (st->state() == ConnectState::Normal) {
            txt += "<b>Server status</b>: " + VSState::toName(server_) + "<br>";
            txt += "<b>Status</b>: " + VNState::toName(server_) + "<br>";
            txt += "<b>Total number of nodes</b>: " + QString::number(totalNum_);
        }
        else if (st->state() == ConnectState::Lost) {
            txt += "<b><font color=" + colErr.name() + ">Failed to connect to server!</b><br>";
            txt += "<b>Last connection</b>: " + VFileInfo::formatDateAgo(st->lastConnectTime()) + "<br>";
            txt += "<b>Last failed attempt</b>: " + VFileInfo::formatDateAgo(st->lastLostTime()) + "<br>";
            if (!st->errorMessage().empty())
                txt += "<b>Error message</b>:<br>" + QString::fromStdString(st->shortErrorMessage());
        }
        else if (st->state() == ConnectState::Disconnected) {
            txt += "<b><font color=" + colErr.name() + ">Server is disconnected!</b><br>";
            txt += "<b>Disconnected</b>: " + VFileInfo::formatDateAgo(st->lastDisconnectTime()) + "<br>";
        }
        else if (st->state() == ConnectState::VersionIncompatible) {
            txt += "<b><font color=" + colErr.name() + ">Server version is incompatible with client!</b><br>";
            if (!st->errorMessage().empty())
                txt += "<b>Error message</b>:<br>" + QString::fromStdString(st->errorMessage());
        }
        else if (st->state() == ConnectState::SslIncompatible) {
            txt +=
                "<b><font color=" + colErr.name() + ">Possible SSL incompatibility between server and client!</b><br>";
            if (!st->errorMessage().empty())
                txt += "<b>Error message</b>:<br>" + QString::fromStdString(st->errorMessage()).replace("\n", "<br>");
        }
        else if (st->state() == ConnectState::SslCertificateError) {
            txt += "<b><font color=" + colErr.name() + ">SSL certificate error!</b><br>";
            if (!st->errorMessage().empty())
                txt += "<b>Error message</b>:<br>" + QString::fromStdString(st->errorMessage()).replace("\n", "<br>");
        }
        else if (st->state() == ConnectState::FailedClient) {
            txt += "<b><font color=" + colErr.name() + ">Could not create client object!</b><br>";
            if (!st->errorMessage().empty())
                txt += "<b>Error message</b>:<br>" + QString::fromStdString(st->errorMessage()).replace("\n", "<br>");
        }
    }

    auto userLogHost = server_->conf()->stringValue(VServerSettings::UserLogServerHost);
    auto userLogPort = server_->conf()->stringValue(VServerSettings::UserLogServerPort);
    if (!userLogHost.isEmpty() && !userLogPort.isEmpty()) {
        txt += "<br><b>Custom logserver</b>: " + userLogHost + "@" + userLogPort;
    }

    return txt;
}

void VServer::clearNodeTriggerData() {
    triggeredScanned_ = false;
    std::size_t num   = nodes_.size();
    for (std::size_t i = 0; i < num; i++)
        nodes_[i]->clearTriggerData();
}

void VServer::print() {
    for (auto& i : children_)
        i->print();
}
