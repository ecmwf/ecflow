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

#include "ecflow/core/Message.hpp"
#include "ecflow/core/Str.hpp"

namespace cereal {
class access;
}

///
/// @brief Represents an ecFlow variable as a name/value pair.
///
/// Variables are lightweight string attributes used throughout the node tree.
/// The name is subject to the same validation rules as ecFlow identifiers.
///
class Variable {
public:
    ///
    /// @brief Constructs an empty variable with an empty name and value.
    ///
    Variable() = default;

    ///
    /// @brief Constructs a variable with the given name and value, without validation.
    ///
    /// @param[in] name  The variable name.
    /// @param[in] value The variable value.
    ///
    Variable(const std::string& name, const std::string& value)
        : n_(name),
          v_(value) {}

    ///
    /// @brief Constructs a variable after validating the given name.
    ///
    /// @param[in] name  The variable name; must be a valid ecFlow identifier.
    /// @param[in] value The variable value.
    /// @return A new Variable with the given name and value.
    /// @throws std::runtime_error if @p name is not a valid variable name.
    /// @see is_valid_variable_name
    ///
    static Variable new_variable(const std::string& name, const std::string& value) {
        // Perform name validation
        std::string msg;
        if (!is_valid_variable_name(name, msg)) {
            throw std::runtime_error(MESSAGE("Variable::new_variable: " << msg));
        }

        // Create the variable
        return Variable(name, value);
    }

    ///
    /// @brief Checks whether the variable name is empty.
    ///
    /// @return `true` if the variable name is empty, otherwise `false`.
    ///
    inline bool empty() const { return n_.empty(); }

    ///
    /// @brief Returns the variable name.
    ///
    /// @return A reference to the variable name.
    ///
    inline const std::string& name() const { return n_; }

    ///
    /// @brief Sets the variable name after validating it.
    ///
    /// @param[in] name The new variable name; must be a valid ecFlow identifier.
    /// @throws std::runtime_error if @p name is not a valid variable name.
    /// @see is_valid_variable_name
    ///
    inline void set_name(const std::string& name) {
        // Perform name validation
        std::string msg;
        if (!ecf::algorithm::is_valid_name(name, msg)) {
            throw std::runtime_error(MESSAGE("Variable::set_name: " << msg));
        }

        // Set the variable name
        n_ = name;
    }

    ///
    /// @brief Returns the current 'string' variable value.
    ///
    /// @return The variable value.
    ///
    inline const std::string& value() const { return v_; }

    ///
    /// @brief Returns the current variable value, converted to an R value.
    ///
    /// @tparam R The target conversion type.
    /// @return The variable value, or a default value if the value fails the conversion to R.
    ///
    /// @note This is defined to establish a generic contract for specific conversions to R. No implementation provided.
    /// @note The default value, returned when the value cannot be converted to R, is specific to R itself.
    ///
    template <class R>
    R value() const;

    ///
    /// @brief Sets the variable value.
    ///
    /// @param[in] value The new variable value.
    ///
    inline void set_value(const std::string& value) { v_ = value; }

    ///
    /// @brief Returns a mutable reference to the variable value.
    ///
    /// @return A reference to the variable value.
    ///
    inline std::string& value_by_ref() { return v_; }

    ///
    /// @brief Compares two variables for equality.
    ///
    /// @param[in] rhs The variable to compare with.
    /// @return `true` if both name and value are equal, otherwise `false`.
    ///
    bool operator==(const Variable& rhs) const;

    ///
    /// @brief Compares two variables by name.
    ///
    /// @param[in] rhs The variable to compare with.
    /// @return `true` if this variable's name is lexicographically less than @p rhs's name.
    ///
    inline bool operator<(const Variable& rhs) const { return n_ < rhs.name(); }

    ///
    /// @brief Returns an ecFlow-defs representation of this variable.
    ///
    /// @return A string in the form `edit <name> '<value>'`.
    ///
    std::string toString() const;

    ///
    /// @brief Returns a debug string including the integer value.
    ///
    /// @return A string combining toString() and value().
    ///
    std::string dump() const;

    ///
    /// @brief Appends an ecFlow-defs representation of this variable to the given buffer.
    ///
    /// @param[out] ret The buffer to append to.
    ///
    void write(std::string& ret) const;

    ///
    /// @brief Checks whether the given string is a valid ecFlow variable name.
    ///
    /// @param[in]  name The name to validate.
    /// @param[out] msg  Diagnostic message populated when validation fails.
    /// @return `true` if @p name is a valid variable name, otherwise `false`.
    ///
    inline static bool is_valid_variable_name(const std::string& name, std::string& msg) {
        return ecf::algorithm::is_valid_name(name, msg);
    }

    ///
    /// @brief Returns a shared empty variable, suitable for return-by-reference fallback.
    ///
    /// @return A reference to a process-wide empty Variable instance.
    ///
    static const Variable& EMPTY();

private:
    std::string n_;
    std::string v_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

///
/// @brief Parses the variable value as an integer.
///
/// @return The integer value, or `0` if the value cannot be converted.
///
template <>
inline int Variable::value<int>() const {
    // check if the value is convertible to an integer
    return ecf::algorithm::to_int(v_, 0 /* value to return if conversion fails*/);
}

///
/// @brief Stores a collection of Variables and provides indexed lookup by name.
///
/// Internally keeps a vector of variables together with a name-to-index hash map.
/// Duplicate names are not handled explicitly by the constructor; later duplicates
/// overwrite earlier entries in the index.
///
class VariableMap {
public:
    ///
    /// @brief Vector-based storage type for the variables.
    ///
    using storage_t = std::vector<Variable>;

    ///
    /// @brief Hash map from variable name to its position in @c variables_.
    ///
    using index_t = std::unordered_map<std::string, size_t>;

    ///
    /// @brief Constructs a map from the given variables.
    ///
    /// @tparam VARIABLES A parameter pack of Variable types.
    /// @param[in] variables The variables to store in the map.
    ///
    template <typename... VARIABLES>
    explicit VariableMap(const VARIABLES... variables)
        : variables_{variables...} {
        // Fill index
        for (size_t i = 0; i < variables_.size(); ++i) {
            index_.insert(std::make_pair(variables_[i].name(), i));
        }
    }

    ///
    /// @brief Returns an iterator to the first variable.
    ///
    /// @return A mutable iterator to the beginning of the storage.
    ///
    [[nodiscard]] inline storage_t::iterator begin() { return variables_.begin(); }

    ///
    /// @brief Returns a const iterator to the first variable.
    ///
    /// @return A const iterator to the beginning of the storage.
    ///
    [[nodiscard]] inline storage_t::const_iterator begin() const { return variables_.begin(); }

    ///
    /// @brief Returns an iterator past the last variable.
    ///
    /// @return A mutable iterator to the end of the storage.
    ///
    [[nodiscard]] inline storage_t::iterator end() { return variables_.end(); }

    ///
    /// @brief Returns a const iterator past the last variable.
    ///
    /// @return A const iterator to the end of the storage.
    ///
    [[nodiscard]] inline storage_t::const_iterator end() const { return variables_.end(); }

    ///
    /// @brief Checks whether the map contains no variables.
    ///
    /// @return `true` if the map is empty, otherwise `false`.
    ///
    [[nodiscard]] inline bool empty() const { return variables_.empty(); }

    ///
    /// @brief Returns the number of variables in the map.
    ///
    /// @return The number of stored variables.
    ///
    [[nodiscard]] inline size_t size() const { return variables_.size(); }

    ///
    /// @brief Sets the same value on every variable in the map.
    ///
    /// @param[in] value The value to assign to all variables.
    ///
    void set_value(const std::string& value);

    ///
    /// @brief Looks up a variable by name.
    ///
    /// @param[in] name The name of the variable to retrieve.
    /// @return A reference to the matching Variable.
    /// @throws std::runtime_error if no variable with @p name exists in the map.
    ///
    [[nodiscard]] Variable& operator[](const std::string& name);

private:
    storage_t variables_;
    index_t index_;
};

#endif /* ecflow_attribute_Variable_HPP */
