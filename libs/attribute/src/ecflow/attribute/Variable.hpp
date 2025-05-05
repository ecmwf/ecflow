/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_Variable_HPP
#define ecflow_attribute_Variable_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace cereal {
class access;
}

////////////////////////////////////////////////////////////////////////////////////////
// Class Variable:
// Use compiler , generated destructor, assignment,  copy constructor
class Variable {
public:
    // This constructor was added as an optimisation, to avoid checking variable names.
    // This constructor allows easily create generated variables, and is also useful on default constructors of
    // Suites, Families, and Tasks. This is also used during serialisation, to avoid checking generated names that are
    // guaranteed to be valid.
    // Notice that the bool argument is used as a "tag" argument and not actually used.
    Variable(const std::string& name, const std::string& value, [[maybe_unused]] bool check) : n_(name), v_(value) {}
    Variable(const std::string& name, const std::string& value);
    Variable() = default;

    const std::string& name() const { return n_; }
    void print(std::string&) const;
    void print_server_variable(std::string&) const;
    void print_generated(std::string&) const;
    bool empty() const { return n_.empty(); }

    void set_value(const std::string& v) { v_ = v; }
    const std::string& theValue() const { return v_; }
    int value() const;

    void set_name(const std::string& v);
    std::string& value_by_ref() { return v_; }

    bool operator==(const Variable& rhs) const;
    bool operator<(const Variable& rhs) const { return n_ < rhs.name(); }
    std::string toString() const;
    std::string dump() const;

    // Added to support return by reference
    static const Variable& EMPTY();

private:
    void write(std::string&) const;

private:
    std::string n_;
    std::string v_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

class VariableMap {
public:
    using storage_t = std::vector<Variable>;
    using index_t   = std::unordered_map<std::string, size_t>;

    template <typename... VARIABLES>
    explicit VariableMap(const VARIABLES... variables) : variables_{variables...} {
        // Fill index
        for (size_t i = 0; i < variables_.size(); ++i) {
            index_.insert(std::make_pair(variables_[i].name(), i));
        }
    }

    [[nodiscard]] storage_t::iterator begin() { return variables_.begin(); }
    [[nodiscard]] storage_t::const_iterator begin() const { return variables_.begin(); }
    [[nodiscard]] storage_t::iterator end() { return variables_.end(); }
    [[nodiscard]] storage_t::const_iterator end() const { return variables_.end(); }

    [[nodiscard]] bool empty() const { return variables_.empty(); }
    [[nodiscard]] size_t size() const { return variables_.size(); }

    void set_value(const std::string& value);

    [[nodiscard]] Variable& operator[](const std::string& name);

private:
    storage_t variables_;
    index_t index_;
};

#endif /* ecflow_attribute_Variable_HPP */
