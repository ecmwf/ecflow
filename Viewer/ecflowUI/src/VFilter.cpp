/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VFilter.hpp"

#include <algorithm>

#include <QDebug>
#include <QElapsedTimer>
#include <QTime>

#include "NodeQuery.hpp"
#include "NodeQueryEngine.hpp"
#include "ServerFilter.hpp"
#include "ServerHandler.hpp"
#include "UIDebug.hpp"
#include "UiLogS.hpp"
#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "VIcon.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "VParam.hpp"
#include "VSettings.hpp"
#include "VTree.hpp"

#define _UI_VFILTER_DEBUG

//==============================================
//
// VFilter
//
//==============================================

VParamSet::VParamSet() = default;

void VParamSet::init(const std::vector<VParam*>& items) {
    all_ = items;

    int maxId = 0;
    for (std::vector<VParam*>::const_iterator it = all_.begin(); it != all_.end(); ++it) {
        if (static_cast<int>((*it)->id()) > maxId) {
            maxId = (*it)->id();
        }
    }

    if (maxId > 0) {
        currentCache_.resize(maxId + 1, 0);
        for (auto it = all_.begin(); it != all_.end(); ++it) {
            currentCache_[(*it)->id()] = 1;
        }
    }

    setCurrent(all_, false);
}

// This has to be very fast. Called a millions of times!
bool VParamSet::isSet(VParam* p) const {
    // assert(p);
    // return (current_.find(p) != current_.end());
    // return std::find(current_.begin(),current_.end(),p) != current_.end();

    // for(size_t i=0; i < current_.size(); i++)
    //     if(current_[i] == p)
    //         return true;

    // return true;
    return (currentCache_[p->id()] == 0) ? false : true;

    // return false;
}

bool VParamSet::isSet(const std::string& name) const {
    for (auto it : current_) {
        if (it->strName() == name) {
            return true;
        }
    }
    return false;
}

QStringList VParamSet::currentAsList() const {
    QStringList lst;
    for (auto it : current_) {
        lst << QString::fromStdString(it->strName());
    }
    return lst;
}

void VParamSet::clearCurrent() {
    current_.clear();
    std::fill(currentCache_.begin(), currentCache_.end(), 0);
    empty_    = true;
    complete_ = false;
}

void VParamSet::addToCurrent(VParam* p) {
    current_.push_back(p);
    uint id = p->id();
    UI_ASSERT(id < currentCache_.size(), "id=" << id << " currentCache_.size()=" << currentCache_.size());
    currentCache_[id] = 1;
    empty_            = false;
    complete_         = (current_.size() == all_.size());
}

void VParamSet::setCurrent(const std::vector<VParam*>& items, bool broadcast) {
    clearCurrent();

    for (auto it = all_.begin(); it != all_.end(); ++it) {
        if (std::find(items.begin(), items.end(), *it) != items.end()) {
            addToCurrent(*it);
        }
    }

    if (broadcast) {
        Q_EMIT changed();
    }
}

void VParamSet::setCurrent(const std::vector<std::string>& names, bool broadcast) {
    clearCurrent();

    for (auto it = all_.begin(); it != all_.end(); ++it) {
        if (std::find(names.begin(), names.end(), (*it)->strName()) != names.end()) {
            addToCurrent(*it);
        }
    }

    if (broadcast) {
        Q_EMIT changed();
    }
}

void VParamSet::setCurrent(QStringList names, bool broadcast) {
    clearCurrent();

    for (auto it = all_.begin(); it != all_.end(); ++it) {
        if (names.contains(QString::fromStdString((*it)->strName()))) {
            addToCurrent(*it);
        }
    }

    if (broadcast) {
        Q_EMIT changed();
    }
}

void VParamSet::writeSettings(VSettings* vs) {
    std::vector<std::string> array;

    if (isComplete()) {
        array.emplace_back("_ALL_");
    }
    else {
        for (auto it = current_.begin(); it != current_.end(); ++it) {
            array.push_back((*it)->strName());
        }
    }

    vs->put(settingsId_, array);
}

void VParamSet::readSettings(VSettings* vs) {
    clearCurrent();

    std::vector<std::string> array;

    // Try to read the old version (aka V0) of the settings
    // In this case an empty list means all is selected!
    if (vs->contains(settingsIdV0_)) {
        vs->get(settingsIdV0_, array);
        if (array.empty()) {
            setCurrent(all_, false);
            return;
        }
    }
    // otherwise read the standard version
    else {
        vs->get(settingsId_, array);
    }

    for (auto it = array.begin(); it != array.end(); ++it) {
        std::string name = *it;
        if (name == "_ALL_") {
            setCurrent(all_, false);
            return;
        }

        for (auto itA = all_.begin(); itA != all_.end(); ++itA) {
            if ((*itA)->strName() == name) {
                addToCurrent(*itA);
            }
        }
    }
}

//==============================================
//
// StateFilter
//
//==============================================

NodeStateFilter::NodeStateFilter() : VParamSet() {
    settingsId_            = "states";
    settingsIdV0_          = "state";
    std::vector<VParam*> v = VNState::filterItems();
    init(v);
}

//==============================================
//
// AttributeFilter
//
//==============================================

AttributeFilter::AttributeFilter() : VParamSet() {
    settingsId_            = "attributes";
    settingsIdV0_          = "attribute";
    std::vector<VParam*> v = VAttributeType::filterItems();
    init(v);
}

bool AttributeFilter::matchForceShowAttr(const VNode* n, VAttributeType* t) const {
    if (forceShowAttr_) {
        if (VAttribute* a = forceShowAttr_->attribute()) {
            return (a->parent() == n && a->type() == t);
        }
    }
    return false;
}

void AttributeFilter::setForceShowAttr(VAttribute* a) {
    forceShowAttr_ = VInfoAttribute::create(a);
}

VAttribute* AttributeFilter::forceShowAttr() const {
    return (forceShowAttr_) ? (forceShowAttr_->attribute()) : nullptr;
}

void AttributeFilter::clearForceShowAttr() {
    forceShowAttr_.reset();
}

void AttributeFilter::updateForceShowAttr() {
    if (forceShowAttr_) {
        forceShowAttr_->regainData();
        if (forceShowAttr_->hasData()) {
            forceShowAttr_.reset();
        }
    }
}

// By default all but the uservar and genvar attributes should be visible
std::vector<VParam*> AttributeFilter::defaults() const {
    std::vector<VParam*> def;
    for (auto it : all_) {
        if (it->strName() != "var" && it->strName() != "genvar") {
            def.emplace_back(it);
        }
    }
    return def;
}

void AttributeFilter::readSettings(VSettings* vs) {
    VParamSet::readSettings(vs);

    // If the filter list is not complete we need to be sure that a newly added attribute type
    // is automatically enabled in the filter. This is based on the contents of the lastNames attribute
    // file. This file is updated on exit and stores the full list of attribute names (existing at exit).
    // So we can figure out if a new attribute type were introduced since the last startup and we can
    // guarantee that is is always enabled for the first time.
    if (!isComplete()) {
        const std::vector<std::string>& lastNames = VAttributeType::lastNames();

        // The lastNames are not found. This must be the first startup after the lastNames concept were introduced
        // or it is a fresh startup after cleaning the config (or the very first one). We enable all the .
        // It could be only a one-time problem for users who already set their attribute filter.
        if (lastNames.empty()) {
            setCurrent(defaults(), false);
        }
        else {
            // Check which default attribute types are not in lastNames
            for (auto it : defaults()) {
                // The item is not in lastNames so it must be a newly added type. We add it to the filter list
                if (std::find(lastNames.begin(), lastNames.end(), it->strName()) == lastNames.end()) {
                    addToCurrent(it);
                }
            }
        }
    }
}

//==============================================
//
// IconFilter
//
//==============================================

IconFilter::IconFilter() : VParamSet() {
    settingsId_            = "icons";
    settingsIdV0_          = "icon";
    std::vector<VParam*> v = VIcon::filterItems();
    init(v);
}

void IconFilter::readSettings(VSettings* vs) {
    VParamSet::readSettings(vs);

    // If the filter list is not complete we need to be sure that a newly added icon type
    // is automatically enabled in the filter. This is based on the contents of the lastNames icon
    // file. This file is updated on exit and stores the full list of icon names (existing at exit).
    // So we can figure out if a new icon type were introduced since the last startup and we can
    // guarantee that is is always enabled for the first time.
    if (!isComplete()) {
        const std::vector<std::string>& lastNames = VIcon::lastNames();

        // The lastNames are not found. This must be the first startup after lastNames concept were introduced
        // or it is a fresh startup after cleaning the config (or the very first one). We enable all the icons.
        // It could be only a one-time problem for users who already set their icon filter.
        if (lastNames.empty()) {
            setCurrent(all_, false);
        }
        else {
            // Check which icons are not in lastNames
            for (auto itA = all_.begin(); itA != all_.end(); ++itA) {
                // The item is not in lastNames so it must be a newly added icon type. We add it to the filter list
                if (std::find(lastNames.begin(), lastNames.end(), (*itA)->strName()) == lastNames.end()) {
                    addToCurrent(*itA);
                }
            }
        }
    }
}

//==============================================
//
// NodeFilterDef
//==============================================

NodeFilterDef::NodeFilterDef(ServerFilter* serverFilter, Scope /*scope*/) : serverFilter_(serverFilter) {
    nodeState_ = new NodeStateFilter;
    connect(nodeState_, SIGNAL(changed()), this, SIGNAL(changed()));
    query_ = new NodeQuery("tmp", true);
}

NodeFilterDef::~NodeFilterDef() {
    delete query_;
    delete nodeState_;
}

NodeQuery* NodeFilterDef::query() const {
    return query_;
}

void NodeFilterDef::setQuery(NodeQuery* q) {
    query_->swap(q);
    Q_EMIT changed();
}

void NodeFilterDef::writeSettings(VSettings* vs) {
    vs->beginGroup("query");
    query_->save(vs);
    vs->endGroup();
}

void NodeFilterDef::readSettings(VSettings* vs) {
    vs->beginGroup("query");
    query_->load(vs);
    vs->endGroup();

    Q_EMIT changed();
}

void NodeFilterDef::serverRemoved(const std::string& serverName) {
    if (query_->removeServer(serverName)) {
        Q_EMIT changed();
    }
}

void NodeFilterDef::serverRenamed(const std::string& newName, const std::string& oldName) {
    if (query_->renameServer(newName, oldName)) {
        Q_EMIT changed();
    }
}

//==============================================
//
// NodeFilter
//
//==============================================

NodeFilter::NodeFilter(NodeFilterDef* def, ServerHandler* server) : def_(def), server_(server) {
    assert(server_);
    queryEngine_ = new NodeFilterEngine(this);
}

NodeFilter::~NodeFilter() {
    delete queryEngine_;
}

void NodeFilter::clear() {
    clearForceShowNode();
}

void NodeFilter::setForceShowNode(VNode* n) {
    forceShowNode_ = n;
#ifdef _UI_VFILTER_DEBUG
    if (forceShowNode_) {
        UiLogS(server_).dbg() << "NodeFilter::setForceShowNode --> " << forceShowNode_->absNodePath();
    }
#endif
}

void NodeFilter::clearForceShowNode() {
    forceShowNode_ = nullptr;
}

//============================================
//
// TreeNodeFilter
//
//============================================

TreeNodeFilter::TreeNodeFilter(NodeFilterDef* def, ServerHandler* server, VTree* tree)
    : NodeFilter(def, server),
      tree_(tree) {
}

void TreeNodeFilter::clear() {
    NodeFilter::clear();
    match_ = std::vector<VNode*>();
}

bool TreeNodeFilter::isNull() {
    // return def_->nodeState_->isComplete() || def_->nodeState_->isEmpty();
    return def_->nodeState_->isEmpty();
}

bool TreeNodeFilter::isComplete() {
    return def_->nodeState_->isComplete();
}

//
bool TreeNodeFilter::update(const std::vector<VNode*>& topChange, std::vector<VNode*>& topFilterChange) {
#ifdef _UI_VFILTER_DEBUG
    UI_FUNCTION_LOG_S(server_);
#endif

    // nodes_.clear();

    // If all states are hidden or visible
    if (def_->nodeState_->isComplete() || def_->nodeState_->isEmpty()) {
        // deallocate the match vector
        match_ = std::vector<VNode*>();
        // assert(match_.capacity() == 0);
#ifdef _UI_VFILTER_DEBUG
        UiLogS(server_).dbg() << " no filter is defined!";
#endif
        return false;
    }

#ifdef _UI_VFILTER_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    VServer* root = server_->vRoot();
    if (root->totalNum() > 0) {
        bool fullRun = false;

        // The number of nodes changed: we need to rerun everything
        if (static_cast<int>(match_.size()) != root->totalNum() || match_.size() != tree_->nodeVec().size()) {
            // Deallocates the match vector
            match_ = std::vector<VNode*>();
            // match_.reserve(root->totalNum());
            VNode* n = nullptr;
            match_.resize(root->totalNum(), n);
            // td::fill(match_.begin(), match_.end(), n);
            fullRun = true;
        }

        // The topchange vector is empty: it can only happen when we need to rerun everything
        else if (topChange.empty()) {
            VNode* n = nullptr;
            // match_.clear();
            std::fill(match_.begin(), match_.end(), n);
            fullRun = true;
        }

        // We rerun everything
        if (fullRun) {
            for (int i = 0; i < root->numOfChildren(); i++) {
                filterState(root->childAt(i), def_->nodeState_);
            }
        }

        // We only check the branches defined by the nodes in topChange
        else {
            // At this point the tree_->nodeVec() and match must have the same content
            assert(tree_->nodeVec().size() == match_.size());

            // Update the filter results
            for (auto i : topChange) {
                filterState(i, def_->nodeState_);
            }

#ifdef _UI_VFILTER_DEBUG
            int diffCnt = 0;
            for (size_t i = 0; i < match_.size(); i++) {
                if (tree_->vnodeAt(i) != match_[i]) {
                    diffCnt++;
                }
            }
            UiLogS(server_).dbg() << " number of differences in filter: " << diffCnt;
#endif

            // We collect the topmost nodes with changes. It could be different to
            // topChange so we need this step!
            for (auto i : topChange) {
                assert(i->isSuite());
                collectTopFilterChange(i, topFilterChange);
            }

#ifdef _UI_VFILTER_DEBUG
            assert(static_cast<int>(topFilterChange.size()) <= diffCnt);
            if (diffCnt > 0) {
                assert(topFilterChange.size() > 0);
            }
#endif
        }

#ifdef _UI_VFILTER_DEBUG
        UiLogS(server_).dbg() << " top level nodes that changed in filter:";
        for (auto& i : topFilterChange) {
            UiLogS(server_).dbg() << "  " << i->strName();
        }
#endif
    }
    else {
        match_.clear();
    }

#ifdef _UI_VFILTER_DEBUG
    UiLogS(server_).dbg() << " elapsed time: " << timer.elapsed() << " ms";
    UiLogS(server_).dbg() << " filter size: " << match_.size();
    UiLogS(server_).dbg() << " capacity:" << match_.capacity();
#endif

    return true;
}

bool TreeNodeFilter::update() {
    std::vector<VNode*> topChange;
    std::vector<VNode*> topFilterChange;
    return update(topChange, topFilterChange);
}

// Finds the top level nodes whose filter status changed
bool TreeNodeFilter::collectTopFilterChange(VNode* node, std::vector<VNode*>& topFilterChange) {
    int idx = node->index();
    if (tree_->vnodeAt(idx) != match_[idx]) {
        topFilterChange.push_back(node);
        return true;
    }

    for (int i = 0; i < node->numOfChildren(); i++) {
        if (collectTopFilterChange(node->childAt(i), topFilterChange)) {
            break;
        }
    }

    return false;
}

bool TreeNodeFilter::filterState(VNode* node, VParamSet* stateFilter) {
    bool ok = false;

    if (stateFilter->isSet(VNState::toState(node)) || forceShowNode_ == node) {
        ok = true;
    }

    for (int i = 0; i < node->numOfChildren(); i++) {
        if (filterState(node->childAt(i), stateFilter) == true && ok == false) {
            ok = true;
        }
    }

    if (ok) {
        match_[node->index()] = node;
    }
    else {
        match_[node->index()] = nullptr;
    }

    return ok;
}

//===========================================================
//
// TableNodeFilter
//
//===========================================================

TableNodeFilter::TableNodeFilter(NodeFilterDef* def, ServerHandler* server) : NodeFilter(def, server), matchCount_(0) {
}

// When nothing should be shown
bool TableNodeFilter::isNull() {
    return false;
    // return def_->nodeState_->isComplete();
    // return def_->nodeState_->isNull();
}

// When everything should be shown
bool TableNodeFilter::isComplete() {
    return false;
    // return def_->nodeState_->isComplete();
}

void TableNodeFilter::clear() {
    NodeFilter::clear();
    match_.clear();
    index_.clear();
    matchCount_ = 0;
}

int TableNodeFilter::indexOf(const VNode* node) const {
    switch (matchMode_) {
        case VectorMatch:
            return index_[node->index()];
        case AllMatch:
            return node->index();
        case NoneMatch:
            return -1;
        default:
            assert(0);
            return -1;
    }

    return -1;
}

VNode* TableNodeFilter::nodeAt(int index) const {
    switch (matchMode_) {
        case VectorMatch:
            return match_[index];
        case AllMatch:
            return server_->vRoot()->nodeAt(index);
        case NoneMatch:
            return nullptr;
        default:
            assert(0);
            return nullptr;
    }

    return nullptr;
}

bool TableNodeFilter::update() {
#ifdef _UI_VFILTER_DEBUG
    UiLogS(server_).dbg() << "TableNodeFilter::update -->";
#endif

    NodeQuery* q = def_->query_;
    Q_ASSERT(q);
    if (!q->hasServer(server_->name()) || server_->vRoot()->totalNum() == 0) {
        matchMode_ = NoneMatch;
        // Deallocates
        match_      = std::vector<VNode*>();
        index_      = std::vector<int>();
        matchCount_ = 0;
#ifdef _UI_VFILTER_DEBUG
        UiLogS(server_).dbg() << " no nodes are filtered!";
#endif
        return true;
    }

    if (q->query().isEmpty() && q->rootNode().empty()) {
        matchMode_ = AllMatch;
        // Deallocates
        match_      = std::vector<VNode*>();
        index_      = std::vector<int>();
        matchCount_ = server_->vRoot()->totalNum();
#ifdef _UI_VFILTER_DEBUG
        UiLogS(server_).dbg() << " all the nodes are filtered!";
#endif
        return true;
    }

#ifdef _UI_VFILTER_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    matchMode_ = VectorMatch;
    match_.clear();
    int num = server_->vRoot()->totalNum();
    if (num != static_cast<int>(index_.size())) {
        // Reallocates
        index_ = std::vector<int>();
        index_.resize(num, -1);
    }
    else {
        std::fill(index_.begin(), index_.end(), -1);
    }

    queryEngine_->setQuery(def_->query_);
    if (queryEngine_->runQuery(server_)) {
        matchCount_ = match_.size();
        for (size_t i = 0; i < match_.size(); i++) {
            index_[match_[i]->index()] = i;
        }
    }
    else {
        std::fill(index_.begin(), index_.end(), -1);
    }

#ifdef _UI_VFILTER_DEBUG
    UiLogS(server_).dbg() << " elapsed time: " << timer.elapsed() << " ms";
    UiLogS(server_).dbg() << " filter size: " << match_.size();
    UiLogS(server_).dbg() << " capacity: " << match_.capacity();
#endif

    return true;
}
