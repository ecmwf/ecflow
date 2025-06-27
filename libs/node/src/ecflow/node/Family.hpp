/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Family_HPP
#define ecflow_node_Family_HPP

#include "ecflow/node/NodeContainer.hpp"

class FamGenVariables;

class Family final : public NodeContainer {
public:
    explicit Family(const std::string& name, bool check = true) : NodeContainer(name, check) {}
    Family() = default;
    Family(const Family& rhs) : NodeContainer(rhs) {}
    Family& operator=(const Family&);
    node_ptr clone() const override;

    ~Family() override;

    static family_ptr create(const std::string& name, bool check = true);
    static family_ptr create_me(const std::string& name); // python api, to pick correct init function

    bool check_defaults() const override;

    Suite* suite() const override { return parent()->suite(); }
    Defs* defs() const override {
        return (parent()) ? parent()->defs() : nullptr;
    } // exposed to python hence check for NULL first
    Family* isFamily() const override { return const_cast<Family*>(this); }
    NodeContainer* isNodeContainer() const override { return const_cast<Family*>(this); }

    void begin() override;
    bool resolveDependencies(JobsParam&) override; // overriden to speicy family for job profiler
    void requeue(Requeue_args&) override;
    void accept(ecf::NodeTreeVisitor&) override;
    void acceptVisitTraversor(ecf::NodeTreeVisitor& v) override;
    void update_generated_variables() const override;
    const Variable& findGenVariable(const std::string& name) const override;
    void gen_variables(std::vector<Variable>&) const override;
    std::string find_node_path(const std::string& type, const std::string& name) const override;

    const std::string& debugType() const override;

    bool operator==(const Family& rhs) const;

    void collateChanges(DefsDelta&) const override;
    void set_memento(const OrderMemento* m, std::vector<ecf::Aspect::Type>& aspects, bool f) {
        NodeContainer::set_memento(m, aspects, f);
    }
    void set_memento(const ChildrenMemento* m, std::vector<ecf::Aspect::Type>& aspects, bool f) {
        NodeContainer::set_memento(m, aspects, f);
    }

    void read_state(const std::string& line, const std::vector<std::string>& lineTokens) override;

public:
    void write_state(std::string&, bool&) const override;

private:
    mutable FamGenVariables* fam_gen_variables_{nullptr};

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

// We can have several thousands Families. This class helps in avoiding
// the creation of generated variables until required.
// This improves client->server download times by avoiding thousands of string constructions
class FamGenVariables {
public:
    explicit FamGenVariables(const Family*);
    // Disable copy (and move) semantics
    FamGenVariables(const FamGenVariables&)                  = delete;
    const FamGenVariables& operator=(const FamGenVariables&) = delete;

    void update_generated_variables() const;
    const Variable& findGenVariable(const std::string& name) const;
    void gen_variables(std::vector<Variable>& vec) const;

private:
    const Family* family_;
    mutable Variable genvar_family_;
    mutable Variable genvar_family1_;
};

#endif /* ecflow_node_Family_HPP */
