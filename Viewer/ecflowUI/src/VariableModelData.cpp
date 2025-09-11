/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VariableModelData.hpp"

#include <QString>

#include "CommandHandler.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"
#include "VAttribute.hpp"
#include "VGenVarAttr.hpp"
#include "VItemPathParser.hpp"
#include "VNState.hpp"
#include "VNode.hpp"
#include "VariableModelDataObserver.hpp"

static std::string defaultStr("");

#define UI_VARIABLEMODELDATA_DEBUG

//==========================================
//
// VariableModelData
//
//==========================================

VariableModelData::VariableModelData(VInfo_ptr info) : info_(info) {
    reload();
}

VariableModelData::~VariableModelData() = default;

void VariableModelData::clear() {
    vars_.clear();
    genVars_.clear();
}

void VariableModelData::reload() {
    clear();

    if (info_ && info_->node()) {
        info_->node()->variables(vars_);
        info_->node()->genVariables(genVars_);
        removeDuplicates(vars_, genVars_);
    }
}

// When this function called duplicates must have already been removed!!
void VariableModelData::reset(const std::vector<Variable>& vars, const std::vector<Variable>& genVars) {
    clear();

    if (info_ && info_->node()) {
        vars_    = vars;
        genVars_ = genVars;
    }
}

// Remove the generated variables that have the same name as a user variable. User
// variables take precedence over generated variables.
void VariableModelData::removeDuplicates(const std::vector<Variable>& vars, std::vector<Variable>& genVars) {
    std::vector<Variable> gvOri = genVars;
    genVars.clear();

    for (auto& it : gvOri) {
        bool hasIt = false;
        for (const auto& var : vars) {
            if (it.name() == var.name()) {
                hasIt = true;
                break;
            }
        }
        if (!hasIt) {
            genVars.push_back(it);
        }
    }
}

std::string VariableModelData::fullPath() {
    if (info_ && info_->node()) {
        return info_->path();
    }

    return {};
}

std::string VariableModelData::name() {
    return info_->name();
}

std::string VariableModelData::type() {
    if (info_) {
        if (info_->isServer()) {
            return "server";
        }
        else if (info_->node()) {
            return info_->node()->nodeType();
        }
    }

    return {};
}

VNode* VariableModelData::node() const {
    if (info_ && info_->isNode()) {
        return info_->node();
    }

    return nullptr;
}

VInfo_ptr VariableModelData::info(int index) const {
    if (info_) {
        if (index < 0 || index >= varNum()) {
            return {};
        }

        std::string p = info_->storedPath();
        if (!isGenVar(index)) {
            p = VItemPathParser::encodeAttribute(p, vars_[index].name(), "var");
        }
        else {
            p = VItemPathParser::encodeAttribute(p, genVars_[index - vars_.size()].name(), "genvar");
        }

        return VInfo::createFromPath(p);
    }

    return {};
}

const std::string& VariableModelData::name(int index) const {
    if (index < 0 || index >= varNum()) {
        return defaultStr;
    }

    if (!isGenVar(index)) {
        return vars_.at(index).name();
    }
    else {
        return genVars_.at(index - vars_.size()).name();
    }

    return defaultStr;
}

const std::string& VariableModelData::value(int index) const {
    if (index < 0 || index >= varNum()) {
        return defaultStr;
    }

    if (!isGenVar(index)) {
        return vars_.at(index).theValue();
    }
    else {
        return genVars_.at(index - vars_.size()).theValue();
    }

    return defaultStr;
}

const std::string& VariableModelData::value(const std::string& n, bool& hasIt) const {
    hasIt = false;
    if (n.empty()) {
        return defaultStr;
    }

    for (const auto& var : vars_) {
        if (var.name() == n) {
            hasIt = true;
            return var.theValue();
        }
    }
    for (const auto& genVar : genVars_) {
        if (genVar.name() == n) {
            hasIt = true;
            return genVar.theValue();
        }
    }

    return defaultStr;
}

bool VariableModelData::hasName(const std::string& n) const {
    for (const auto& var : vars_) {
        if (var.name() == n) {
            return true;
        }
    }

    for (const auto& genVar : genVars_) {
        if (genVar.name() == n) {
            return true;
        }
    }

    return false;
}

int VariableModelData::indexOf(const std::string& varName, bool genVar) const {
    int idx = -1;
    for (const auto& var : vars_) {
        idx++;
        if (!genVar && var.name() == varName) {
            return idx;
        }
    }

    if (!genVar) {
        return -1;
    }

    for (const auto& genVar : genVars_) {
        idx++;
        if (genVar.name() == varName) {
            return idx;
        }
    }

    return -1;
}

#if 0
void VariableModelData::buildAlterCommand(std::vector<std::string>& cmd,
		                            const std::string& action, const std::string& type,
		                            const std::string& name,const std::string& value)
{
	cmd.push_back("ecflow_client");
	cmd.push_back("--alter");
	cmd.push_back(action);
	cmd.push_back(type);

	if(!name.empty())
	{
		cmd.push_back(name);
		cmd.push_back(value);
	}

	cmd.push_back("<full_name>");

}
#endif

void VariableModelData::setValue(int index, const std::string& val) {
    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd, "change", "variable", name(index), val);

    CommandHandler::run(info_, cmd);
}

void VariableModelData::alter(const std::string& name, const std::string& val) {
    std::string mode = "add";

    // Existing name
    if (hasName(name)) {
        // Generated variables cannot be changed. We instead add user variable with the same
        // name. It will shadow the original gen variable.
        if (isGenVar(name)) {
            mode = "add";
        }
        else {
            mode = "change";
        }
    }

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd, mode, "variable", name, val);
    CommandHandler::run(info_, cmd);
}

void VariableModelData::add(const std::string& name, const std::string& val) {
    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd, (hasName(name)) ? "change" : "add", "variable", name, val);
    CommandHandler::run(info_, cmd);
}

void VariableModelData::remove(const std::string& varName) {
    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd, "delete", "variable", varName, "");
    CommandHandler::run(info_, cmd);
}

bool VariableModelData::isGenVar(int index) const {
    return (index >= static_cast<int>(vars_.size()));
}

bool VariableModelData::isMirrorVar(int index) const {
    return info_->isNode() && info_->node()->node()->isMirror();
}

bool VariableModelData::isGenVar(const std::string& n) const {
    for (const auto& genVar : genVars_) {
        if (genVar.name() == n) {
            return true;
        }
    }
    return false;
}

bool VariableModelData::isReadOnly(int index) const {
    return isReadOnly(name(index));
}

bool VariableModelData::isReadOnly(const std::string& varName) const {
    return VGenVarAttr::isReadOnly(varName);
}

bool VariableModelData::isShadowed(int index) const {
    return (shadowed_.find(name(index)) != shadowed_.end());
}

int VariableModelData::varNum() const {
    return vars_.size() + genVars_.size();
}

void VariableModelData::latestVars(std::vector<Variable>& v, std::vector<Variable>& vg) {
    if (info_ && info_->node()) {
        info_->node()->variables(v);
        info_->node()->genVariables(vg);
        removeDuplicates(v, vg);
    }
}

bool VariableModelData::updateShadowed(std::set<std::string>& names) {
    std::set<std::string> ori = shadowed_;
    shadowed_.clear();
    bool changed = false;

#ifdef UI_VARIABLEMODELDATA_DEBUG
    UiLog().dbg() << " ori shadowed:";
    for (const auto& it : ori) {
        UiLog().dbg() << "  " << it;
    }
#endif

    for (const auto& name : names) {
        if (hasName(name)) {
            shadowed_.insert(name);
            if (ori.find(name) == ori.end()) {
                changed = true;
            }
        }
    }

    for (auto& var : vars_) {
        names.insert(var.name());
    }
    for (auto& genVar : genVars_) {
        names.insert(genVar.name());
    }

#ifdef UI_VARIABLEMODELDATA_DEBUG
    UiLog().dbg() << " shadowed:";
    for (const auto& it : shadowed_) {
        UiLog().dbg() << "  " << it;
    }

    UiLog().dbg() << " changed: " << changed;
#endif

    return changed;
}

bool VariableModelData::checkUpdateNames(const std::vector<Variable>& v, const std::vector<Variable>& vg) {
    for (unsigned int i = 0; i < vars_.size(); i++) {
        if (vars_[i].name() != v[i].name()) {
            return true;
        }
    }

    for (unsigned int i = 0; i < genVars_.size(); i++) {
        if (genVars_[i].name() != vg[i].name()) {
            return true;
        }
    }

    return false;
}

// Check if the total number of variables will change. It does not update the local data!
int VariableModelData::checkUpdateDiff(std::vector<Variable>& v, std::vector<Variable>& vg) {
    if (info_ && info_->node() && v.empty() && vg.empty()) {
        // get the current set of variables from the node/server. This might be different
        // to the ones we store.
        latestVars(v, vg);
    }

    // Return the change in the total size of variables
    return v.size() + vg.size() - (vars_.size() + genVars_.size());
}

// Check if any of the names or values has changed. We suppose that the number of current and new
// variables are the same but some of their names or values have been changed.
bool VariableModelData::update(const std::vector<Variable>& v, const std::vector<Variable>& vg) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

#if 0
    if(info_ && info_->node() && v.empty() && vg.empty())
	{
    #ifdef UI_VARIABLEMODELDATA_DEBUG
        UiLog().dbg() << " call latestVars";
    #endif
        latestVars(v,vg);
	}
#endif

#ifdef UI_VARIABLEMODELDATA_DEBUG
    UiLog().dbg() << " new list of variables:";
    for (const auto& i : v) {
        UiLog().dbg() << "  " << i.name() << "=" << i.theValue();
    }
    UiLog().dbg() << "   new list of generated variables:";
    for (const auto& i : vg) {
        UiLog().dbg() << "  " << i.name() << "=" << i.theValue();
    }
#endif

    // We must have the same number of variables
    UI_ASSERT(v.size() + vg.size() == vars_.size() + genVars_.size(),
              "v.size()=" << v.size() << " vg.size()=" << vg.size() << " vars_.size()=" << vars_.size()
                          << " genVars_.size()" << genVars_.size());

    bool changed = false;
    if (v.size() != vars_.size() || vg.size() != genVars_.size()) {
        changed = true;
#ifdef UI_VARIABLEMODELDATA_DEBUG
        UiLog().dbg() << " variables size changed! var: " << vars_.size() << " -> " << v.size()
                      << "gen var: " << genVars_.size() << " -> " << vg.size();
#endif
    }
    else {
        UI_ASSERT(v.size() == vars_.size(), "v.size()=" << v.size() << " vars_.size()=" << vars_.size());

        UI_ASSERT(vg.size() == genVars_.size(), "vg.size()=" << vg.size() << " genVars_.size()=" << genVars_.size());

        for (std::size_t i = 0; i < vars_.size(); i++) {
            if (vars_[i].name() != v[i].name() || vars_[i].theValue() != v[i].theValue()) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
                UiLog().dbg() << " variable changed! name: " << vars_[i].name() << " -> " << v[i].name()
                              << " value: " << vars_[i].theValue() << " -> " << v[i].theValue();
#endif
                changed = true;
                break;
            }
        }

        if (changed == false) {
            for (std::size_t i = 0; i < genVars_.size(); i++) {
                if (genVars_[i].name() != vg[i].name() || genVars_[i].theValue() != vg[i].theValue()) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
                    UiLog().dbg() << " generated variable changed! name: " << genVars_[i].name() << " -> "
                                  << vg[i].name() << " value: " << genVars_[i].theValue() << " -> " << vg[i].theValue();
#endif
                    changed = true;
                    break;
                }
            }
        }
    }

    if (changed) {
        vars_    = v;
        genVars_ = vg;
#ifdef UI_VARIABLEMODELDATA_DEBUG
        UiLog().dbg() << " updated vars and genvars";
#endif
    }

    return changed;
}

//==========================================
//
// VariableModelDataHandler
//
//==========================================

VariableModelDataHandler::VariableModelDataHandler() = default;

VariableModelDataHandler::~VariableModelDataHandler() {
    clear();
}

void VariableModelDataHandler::reload(VInfo_ptr info) {
    // Notifies the model that a change will happen
    Q_EMIT reloadBegin();

    clear(false);

    if (info && info->node()) {
        server_ = info->server();

        std::vector<VNode*> nodes = info->node()->ancestors(VNode::ChildToParentSort);

        for (auto n : nodes) {
            if (n->isServer()) {
                VInfo_ptr ptr = VInfoServer::create(n->server());
                data_.push_back(new VariableModelData(ptr));
            }
            else {
                VInfo_ptr ptr = VInfoNode::create(n);
                data_.push_back(new VariableModelData(ptr));
            }
        }

        updateShadowed();
    }

    Q_EMIT reloadEnd();
}

#if 0
void VariableModelDataHandler::reload()
{
	//Notifies the model that a change will happen
	Q_EMIT reloadBegin();

	for(std::vector<VariableModelData*>::iterator it=data_.begin(); it != data_.end(); ++it)
	{
		(*it)->reload();
	}

    updateShadowed();

    Q_EMIT reloadEnd();
}
#endif

bool VariableModelDataHandler::updateShadowed() {
#ifdef UI_VARIABLEMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    bool shadowChanged = false;

    names_.clear();

    if (data_.size() == 0) {
        return shadowChanged;
    }

    // There are no shadowed vars in the first node
    std::size_t num = data_[0]->varNum();
    for (std::size_t i = 0; i < num; i++) {
        names_.insert(data_[0]->name(i));
    }

    std::size_t dataNum = data_.size();
    for (std::size_t i = 1; i < dataNum; i++) {
        if (data_[i]->updateShadowed(names_)) {
            shadowChanged = true;
        }
    }

// #ifdef UI_VARIABLEMODELDATA_DEBUG
#if 0
    UiLog().dbg() << " names:";
    for(std::set<std::string>::const_iterator it=names_.begin(); it != names_.end(); ++it)
    {
        UiLog().dbg() << "      " + *it;
    }
#endif

    return shadowChanged;
}

void VariableModelDataHandler::clear(bool emitSignal) {
    if (emitSignal) {
        Q_EMIT reloadBegin();
    }

    for (auto& it : data_) {
        delete it;
    }

    server_ = nullptr;
    data_.clear();
    names_.clear();

    broadcastClear();

    if (emitSignal) {
        Q_EMIT reloadEnd();
    }
}

int VariableModelDataHandler::varNum(int index) const {
    if (index >= 0 && index < static_cast<int>(data_.size())) {
        return data_.at(index)->varNum();
    }

    return -1;
}

VariableModelData* VariableModelDataHandler::data(int index) const {
    if (index >= 0 && index < static_cast<int>(data_.size())) {
        return data_.at(index);
    }

    return nullptr;
}

// It is called when a node changed.
bool VariableModelDataHandler::nodeChanged(const VNode* node, const std::vector<ecf::Aspect::Type>& /*aspect*/) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif
    int dataIndex = -1;
    for (std::size_t i = 0; i < data_.size(); i++) {
        if (data_[i]->node() == node) {
            dataIndex = i;
            break;
        }
    }

#ifdef UI_VARIABLEMODELDATA_DEBUG
    UiLog().dbg() << " dataIndex=" << dataIndex;
#endif
    Q_ASSERT(dataIndex != -1);

    // Update the given block
    bool retVal = updateVariables(dataIndex);

    Q_ASSERT(data_[dataIndex]->node());

    // Always update the suite as well!!
    if (!data_[dataIndex]->node()->isSuite()) {
        for (std::size_t i = dataIndex + 1; i < data_.size(); i++) {
            if (data_[i]->node()->isSuite()) {
                if (updateVariables(i)) {
                    retVal = true;
                }

                break;
            }
        }
    }

    if (retVal) {
        broadcastUpdate();
    }

    return retVal;
}

// It is called when the server defs was changed
bool VariableModelDataHandler::defsChanged(const std::vector<ecf::Aspect::Type>& /*aspect*/) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
    UI_FUNCTION_LOG
#endif

    if (data_.size() == 0) {
        return false;
    }

    int dataIndex = data_.size() - 1;
    Q_ASSERT(dataIndex >= 0 && dataIndex < static_cast<int>(data_.size()));
    VariableModelData* d = data_.at(data_.size() - 1);
    Q_ASSERT(d);
    Q_ASSERT(d->type() == "server");

    bool retVal = updateVariables(dataIndex);

    if (retVal) {
        broadcastUpdate();
    }

    return retVal;
}

bool VariableModelDataHandler::updateVariables(int dataIndex) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
    UI_FUNCTION_LOG
    UiLog().dbg() << " dataIndex=" << dataIndex;
#endif

    bool retVal = false;

    // There is no notification about generated variables. Basically they can
    // change at any update!! So we have to check all the variables at every update!!
    std::vector<Variable> v;
    std::vector<Variable> vg;

    // Get the current set of variables and check if the total number of variables
    // has changed. At this point v and vg do not contain any duplicates (within the
    // same block the user variables take precedence over the generated variables)
    int cntDiff = data_[dataIndex]->checkUpdateDiff(v, vg);

    const int numOld = data_[dataIndex]->varNum(); // the current num in the model
    const int numNew = v.size() + vg.size();       // the new num

#ifdef UI_VARIABLEMODELDATA_DEBUG
    UiLog().dbg() << " cntDiff=" << cntDiff << " numOld=" << numOld << " numNew=" << numNew;
#endif

    // Clear the block's contents in the model
    Q_EMIT clearBegin(dataIndex, numOld);
    data_[dataIndex]->clear();
    Q_EMIT clearEnd(dataIndex, numOld);

    // Load the new data for the block in the model
    Q_EMIT loadBegin(dataIndex, numNew);
    data_[dataIndex]->reset(v, vg);
    Q_EMIT loadEnd(dataIndex, numNew);

    Q_ASSERT(data_[dataIndex]->varNum() == numNew);

    // At this point the block is filtered and sorted!!!!

    // Check if the shadowed list of variables changed
    if (updateShadowed()) {
#ifdef UI_VARIABLEMODELDATA_DEBUG
        UiLog().dbg() << " emit rerunFilter";
#endif
        // We need to rerun the filter!!
        Q_EMIT rerunFilter();
    }

    retVal = true;

    return retVal;
}

const std::string&
VariableModelDataHandler::value(const std::string& node, const std::string& name, bool& hasIt) const {
    hasIt = false;
    for (auto i : data_) {
        if (i->name() == node) {
            return i->value(name, hasIt);
        }
    }

    return defaultStr;
}

void VariableModelDataHandler::findVariable(const std::string& name,
                                            const std::string& nodePath,
                                            bool genVar,
                                            int& block,
                                            int& row) const {
    block = -1;
    row   = -1;
    for (size_t i = 0; i < data_.size(); i++) {
        if (data_[i]->fullPath() == nodePath) {
            block = i;
            row   = data_[i]->indexOf(name, genVar);
            return;
        }
    }
}

void VariableModelDataHandler::findVariable(VInfo_ptr info, int& block, int& row) const {
    block = -1;
    row   = -1;

    findBlock(info, block);
    if (block != -1 && info && info->isAttribute()) {
        if (VAttribute* a = info->attribute()) {
            std::string name  = a->strName();
            std::string tName = a->typeName();
            if (!name.empty() && (tName == "var" || tName == "genvar")) {
                row = data_[block]->indexOf(name, (tName == "genvar"));
            }
        }
    }
}

void VariableModelDataHandler::findBlock(VInfo_ptr info, int& block) const {
    block = -1;

    if (!info) {
        return;
    }

    std::string p = info->nodePath();
    if (!p.empty()) {
        int n = count();
        for (int i = 0; i < n; i++) {
            if (data_[i]->info_ && data_[i]->info_->nodePath() == p) {
                block = i;
                return;
            }
        }
    }
}

void VariableModelDataHandler::addObserver(VariableModelDataObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it == observers_.end()) {
        observers_.push_back(o);
    }
}

void VariableModelDataHandler::removeObserver(VariableModelDataObserver* o) {
    auto it = std::find(observers_.begin(), observers_.end(), o);
    if (it != observers_.end()) {
        observers_.erase(it);
    }
}

void VariableModelDataHandler::broadcastClear() {
    std::vector<VariableModelDataObserver*> obsCopy = observers_;
    for (auto& it : obsCopy) {
        it->notifyCleared(this);
    }
}

void VariableModelDataHandler::broadcastUpdate() {
    for (auto& observer : observers_) {
        observer->notifyUpdated(this);
    }
}
