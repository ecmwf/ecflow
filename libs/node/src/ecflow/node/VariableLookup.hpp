// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/node/SubstitutionRecord.hpp"

namespace ecf {

///
/// @brief Gives the values of the variables that a substitution may refer to.
///
/// The interface separates the substitution of variable references from the definition, so that a command can be
/// substituted from the variables of a node, or from a plain set of variables obtained elsewhere.
///
class VariableLookup {
public:
    virtual ~VariableLookup() = default;

    ///
    /// @brief Finds a variable as seen from a node: its user variables, repeat and generated variables, then those of
    /// its parents, then the server's.
    ///
    /// @param[in] name The name of the variable.
    /// @param[out] value Receives the value when the variable is found; an empty value is a value.
    /// @return True when the variable is found.
    ///
    virtual bool find(const std::string& name, std::string& value) const = 0;

    ///
    /// @brief Finds a generated variable as seen from a node: its generated variables, then those of its parents,
    /// then the server's.
    ///
    /// @param[in] name The name of the variable.
    /// @param[out] value Receives the value when the variable is found.
    /// @return True when the variable is found.
    ///
    virtual bool find_generated(const std::string& name, std::string& value) const = 0;

    ///
    /// @brief Tells whether a variable name may contain a colon, as for an alias.
    ///
    /// @return True when `%NAME:default%` is first looked up as a whole name.
    ///
    virtual bool allows_colon_names() const = 0;
};

///
/// @brief Gives the values of variables from plain maps, for a caller without a node.
///
/// The maps hold the result of the lookups of VariableLookup for each name, as a node would give them; from_node
/// builds them from a node.
///
class MapVariableLookup : public VariableLookup {
public:
    ///
    /// @brief Creates a lookup over the given maps.
    ///
    /// @param[in] variables The variables as seen from the node, as VariableLookup::find gives them.
    /// @param[in] generated The generated variables as seen from the node, as VariableLookup::find_generated gives
    ///                      them.
    /// @param[in] colon_names Whether names may contain a colon, as for an alias.
    ///
    MapVariableLookup(NameValueMap variables, NameValueMap generated, bool colon_names = false);

    ///
    /// @brief Builds the maps from the variables visible from a node.
    ///
    /// The generated variables of the node and its parents are taken as they are, created when missing, as the node
    /// lookups do; the caller updates them first, for example with update_generated_variables(), when current values
    /// are needed. For each name, the value is the one that the node lookups give.
    ///
    /// @param[in] node The node.
    /// @return The lookup, which gives the same values as the node.
    ///
    static MapVariableLookup from_node(const Node& node);

    bool find(const std::string& name, std::string& value) const override;
    bool find_generated(const std::string& name, std::string& value) const override;
    bool allows_colon_names() const override;

    ///
    /// @brief Returns the variables as seen from the node.
    ///
    /// @return The map of names to values.
    ///
    [[nodiscard]] const NameValueMap& variables() const { return variables_; }

    ///
    /// @brief Returns the generated variables as seen from the node.
    ///
    /// @return The map of names to values.
    ///
    [[nodiscard]] const NameValueMap& generated() const { return generated_; }

private:
    NameValueMap variables_;
    NameValueMap generated_;
    bool colon_names_;
};

namespace detail {

///
/// @brief Replaces every variable reference of a command, e.g. `%VAR%`, by the value of the variable.
///
/// The value of a variable is searched first in @p user_edit_variables, then through @p lookup. Generated variables
/// that identify the job (ECF_HOST, ECF_PORT, ECF_TRYNO, ECF_NAME, ECF_PASS) take precedence over user variables of
/// the same name. A reference may be nested in the value of another variable, and `%NAME:default%` gives a default for
/// an undefined `NAME`; a doubled micro character is replaced by a single one.
///
/// @param[in,out] cmd The command; on return, the command as substituted, or as partially substituted on failure
/// @param[in] lookup Gives the values of the variables
/// @param[in] user_edit_variables Variables given by the user, which take precedence over the others
/// @param[in] micro The character delimiting the references
/// @param[out] record When given, receives the variables used, appended to those already there; this lower-level
///                    function neither clears the record nor renders its masked command, which variable_substitution
///                    does
/// @param[in] mask Whether the value of a variable with a credential name is replaced by `***` in @p cmd
/// @return true when every reference was substituted; false at the first variable that cannot be found, or when the
///         references are nested too deeply, e.g. a variable referring to itself.
///
bool substitute_variables(std::string& cmd,
                          const VariableLookup& lookup,
                          const NameValueMap& user_edit_variables,
                          char micro,
                          SubstitutionRecord* record,
                          bool mask);

} // namespace detail

///
/// @brief Replaces every variable reference of a command by the value of the variable, recording the substitution.
///
/// This is detail::substitute_variables without masking; when a record is given, it also receives the command
/// substituted with its credentials masked, rendered separately, so that the command itself is the same with or without
/// a record.
///
/// @param[in,out] cmd The command; on return, the command as substituted, or as partially substituted on failure
/// @param[in] lookup Gives the values of the variables
/// @param[in] user_edit_variables Variables given by the user, which take precedence over the others
/// @param[in] micro The character delimiting the references
/// @param[out] record When given, receives the variables used and the masked command
/// @return true when every reference was substituted; false otherwise, as for detail::substitute_variables.
///
bool variable_substitution(std::string& cmd,
                           const VariableLookup& lookup,
                           const NameValueMap& user_edit_variables,
                           char micro,
                           SubstitutionRecord* record = nullptr);

} // namespace ecf
