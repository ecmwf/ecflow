/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Family.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/NodeTreeVisitor.hpp"

using namespace ecf;
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////
// The false below is used as a dummy argument to call the Variable constructor that does not
// Check the variable names. i.e we know they are valid

Family& Family::operator=(const Family& rhs) {
    if (this != &rhs) {
        NodeContainer::operator=(rhs);
        delete fam_gen_variables_;
        fam_gen_variables_ = nullptr;
    }
    return *this;
}

node_ptr Family::clone() const {
    return std::make_shared<Family>(*this);
}

Family::~Family() {
    if (!Ecf::server()) {
        notify_delete();
    }

    delete fam_gen_variables_;
}

family_ptr Family::create(const std::string& name, bool check) {
    return std::make_shared<Family>(name, check);
}

family_ptr Family::create_me(const std::string& name) {
    return std::make_shared<Family>(name, true);
}

bool Family::check_defaults() const {
    if (fam_gen_variables_ != nullptr) {
        throw std::runtime_error("Family ::check_defaults():  fam_gen_variables_ != nullptr");
    }
    return NodeContainer::check_defaults();
}

void Family::accept(ecf::NodeTreeVisitor& v) {
    v.visitFamily(this);
    NodeContainer::accept(v);
}

void Family::acceptVisitTraversor(ecf::NodeTreeVisitor& v) {
    v.visitFamily(this);
}

void Family::begin() {
    NodeContainer::begin();
    update_generated_variables();
}

bool Family::resolveDependencies(JobsParam& jobsParam) {
    if (jobsParam.check_for_job_generation_timeout()) {
        return false;
    }

    return NodeContainer::resolveDependencies(jobsParam);
}

void Family::requeue(Requeue_args& args, std::function<bool(Node*)> authorisation) {
    NodeContainer::requeue(args, authorisation);
    update_generated_variables();
}

bool Family::operator==(const Family& rhs) const {
    return NodeContainer::operator==(rhs);
}

void Family::write_state(std::string& ret, bool& added_comment_char) const {
    NodeContainer::write_state(ret, added_comment_char);
}
void Family::read_state(const std::string& line, const std::vector<std::string>& lineTokens) {
    NodeContainer::read_state(line, lineTokens);
}

const std::string& Family::debugType() const {
    return ecf::Str::FAMILY();
}

void Family::collateChanges(DefsDelta& changes) const {
    /// All changes to family should be on ONE compound_memento_ptr
    compound_memento_ptr compound;
    NodeContainer::incremental_changes(changes, compound);

    // Traversal
    NodeContainer::collateChanges(changes);
}

// generated variables --------------------------------------------------------------------------

void Family::update_generated_variables() const {
    if (!fam_gen_variables_) {
        fam_gen_variables_ = new FamGenVariables(this);
    }
    fam_gen_variables_->update_generated_variables();
    update_repeat_genvar();
}

const Variable& Family::findGenVariable(const std::string& name) const {
    // Generally it should be never the case that the values are empty
    // Since the user is assumed to have called begin(), which force
    // the generation of generated variables

    // AST can reference generated variables. Currently integer based values
    // The family names can be integers

    if (!fam_gen_variables_) {
        update_generated_variables();
    }
    const Variable& gen_var = fam_gen_variables_->findGenVariable(name);
    if (!gen_var.empty()) {
        return gen_var;
    }

    return NodeContainer::findGenVariable(name);
}

void Family::gen_variables(std::vector<Variable>& vec) const {
    if (!fam_gen_variables_) {
        update_generated_variables();
    }

    vec.reserve(vec.size() + 3);
    fam_gen_variables_->gen_variables(vec);
    NodeContainer::gen_variables(vec);
}

std::string Family::find_node_path(const std::string& type, const std::string& node_name) const {
    if (Str::caseInsCompare(type, "family")) {
        if (node_name == name()) {
            return absNodePath();
        }
    }
    return NodeContainer::find_node_path(type, node_name);
}

// ================================================================

FamGenVariables::FamGenVariables(const Family* f)
    : family_(f),
      genvar_family_("FAMILY", "", false),
      genvar_family1_("FAMILY1", "", false) {
}

void FamGenVariables::update_generated_variables() const {
    // This function is called during:
    //   o begin()
    //   o requeue()
    // Since family generated not persisted, allow for demand creation by client
    genvar_family1_.set_value(family_->name());

    // FAMILY is the full path excluding the suite, there is *NO* leading slash
    std::string path              = family_->absNodePath();
    string::size_type secondSlash = path.find('/', 1);
    path.erase(0, secondSlash + 1);
    genvar_family_.set_value(path);
}

const Variable& FamGenVariables::findGenVariable(const std::string& name) const {
    if (genvar_family_.name() == name) {
        return genvar_family_;
    }
    if (genvar_family1_.name() == name) {
        return genvar_family1_;
    }
    return Variable::EMPTY();
}

void FamGenVariables::gen_variables(std::vector<Variable>& vec) const {
    vec.push_back(genvar_family_);
    vec.push_back(genvar_family1_);
}

template <class Archive>
void Family::serialize(Archive& ar, std::uint32_t const version) {
    ar(cereal::base_class<NodeContainer>(this));
}

CEREAL_TEMPLATE_SPECIALIZE_V(Family);
CEREAL_REGISTER_TYPE(Family)
