// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/VariableLookup.hpp"

#include <algorithm>
#include <string_view>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/MaskCredentials.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

namespace {

bool search_user_edit_variables(const std::string& name, std::string& value, const NameValueMap& user_edit_variables) {
    auto i = user_edit_variables.find(name);
    if (i != user_edit_variables.end()) {
        if (((*i).second).empty()) {
            // when we call --edit_script submit file, before a job is submitted the values
            // of generated variables like  ECF_RID, ECF_TRYNO, ECF_NAME, ECF_PASS, ECF_JOB, ECF_JOBOUT, ECF_SCRIPT
            // will be empty. In this case return false, so that we pick up the values from the node.
            return false;
        }
        value = (*i).second;
        return true;
    }
    return false;
}

} // namespace

MapVariableLookup::MapVariableLookup(NameValueMap variables,
                                     NameValueMap generated,
                                     NameValueMap user,
                                     bool colon_names)
    : variables_(std::move(variables)),
      generated_(std::move(generated)),
      user_(std::move(user)),
      colon_names_(colon_names) {
}

MapVariableLookup MapVariableLookup::from_node(const Node& node) {
    NameValueMap variables;
    NameValueMap generated;
    NameValueMap user;

    // Nearest first: the order of Node::findParentVariableValue and Node::find_parent_gen_variable_value. The names
    // of the generated variables come from gen_variables, but their values from findGenVariableValue, since a node
    // may list them in another order than its lookup takes them (a Suite lists its repeat's before its own)
    std::vector<Variable> gen;
    std::string value;
    for (const Node* n = &node; n != nullptr; n = n->parent()) {
        for (const auto& v : n->variables()) {
            variables.emplace(v.name(), v.value());
            user.emplace(v.name(), v.value());
        }
        if (!n->repeat().empty()) {
            variables.emplace(n->repeat().name(), n->repeat().valueAsString());
        }
        gen.clear();
        n->gen_variables(gen);
        for (const auto& v : gen) {
            if (n->findGenVariableValue(v.name(), value)) {
                variables.emplace(v.name(), value);
                generated.emplace(v.name(), value);
            }
        }
    }

    // Then the server: its user variables take precedence over its own variables of the same name
    if (const Defs* defs = node.defs(); defs != nullptr) {
        const auto& server = defs->server_state();
        for (const auto* list : {&server.user_variables(), &server.server_variables()}) {
            for (const auto& v : *list) {
                variables.emplace(v.name(), v.value());
                generated.emplace(v.name(), v.value());
                user.emplace(v.name(), v.value());
            }
        }
    }

    return {std::move(variables), std::move(generated), std::move(user), node.isAlias() != nullptr};
}

bool MapVariableLookup::find(const std::string& name, std::string& value) const {
    auto i = variables_.find(name);
    if (i == variables_.end()) {
        return false;
    }
    value = i->second;
    return true;
}

bool MapVariableLookup::find_generated(const std::string& name, std::string& value) const {
    auto i = generated_.find(name);
    if (i == generated_.end()) {
        return false;
    }
    value = i->second;
    return true;
}

bool MapVariableLookup::find_user(const std::string& name, std::string& value) const {
    auto i = user_.find(name);
    if (i == user_.end()) {
        return false;
    }
    value = i->second;
    return true;
}

bool MapVariableLookup::allows_colon_names() const {
    return colon_names_;
}

bool NodeVariableLookup::find(const std::string& name, std::string& value) const {
    return node_.findParentVariableValue(name, value);
}

bool NodeVariableLookup::find_generated(const std::string& name, std::string& value) const {
    return node_.find_parent_gen_variable_value(name, value);
}

bool NodeVariableLookup::find_user(const std::string& name, std::string& value) const {
    return node_.findParentUserVariableValue(name, value);
}

bool NodeVariableLookup::allows_colon_names() const {
    return node_.isAlias() != nullptr;
}

namespace detail {

bool substitute_variables(std::string& cmd,
                          const VariableLookup& lookup,
                          const NameValueMap& user_edit_variables,
                          char micro,
                          SubstitutionRecord* record,
                          bool mask) {
    // scan the command for variables, and substitute
    // edit cmd "/home/ma/map/sms/smsfectch -F %ECF_FILES% -I %ECF_INCLUDE%"
    // We can also have
    //
    // "%<VAR>:<substitute>% i.e. if VAR exist use it, else use substitute
    //
    // ************************************************************************************************************
    // Special case handling for user variables, and generated variables, which take precedence over node variables
    // ************************************************************************************************************
    //
    // i.e. VAR is defined as BILL
    //  %VAR:fred --f%  will either be "BILL" or if VAR is not defined "fred --f"
    //
    // Infinite recursion. It is possible to end up with infinite recursion:
    //   	edit hello '%hello%'  # string like %hello% will cause infinite recursion
    //   	edit fred '%bill%'
    //	 	edit bill '%fred%'   # should be 10
    // To prevent this we will use a simple count
    bool double_micro_found    = false;
    std::string::size_type pos = 0;
    int count                  = 0;
    const bool is_a_alias      = lookup.allows_colon_names();
    while (true) {
        // A while loop here is used to:
        //   a/ Allow for multiple substitution on a single line. i.e. `%ECF_FILES% -I %ECF_INCLUDE%`
        //   b/ Allow for recursive substitution. %fred% -> %bill% -> 10

        size_t firstPercentPos = cmd.find(micro, pos);
        if (firstPercentPos == std::string::npos) {
            break;
        }

        size_t secondPercentPos = cmd.find(micro, firstPercentPos + 1);
        if (secondPercentPos == std::string::npos) {
            break;
        }

        pos = 0;
        if (secondPercentPos - firstPercentPos <= 1) {
            // handle %% with no characters in between, skip over
            // i.e. to handle "printf %%02d %HOUR:00%" --> "printf %02d 00" i.e. if HOUR not defined
            pos                = secondPercentPos + 1;
            double_micro_found = true;
            continue;
        }

        std::string percentVar(cmd.begin() + firstPercentPos + 1, cmd.begin() + secondPercentPos);

        // Replace the reference by the value used, recording the variable once, and masking it when requested
        auto replace_reference = [&](const std::string& name, const std::string& value) {
            constexpr std::string_view masked_value = "***";
            const bool credential                   = (record || mask) && ecf::is_credential_name(name);
            if (record) {
                auto& variables = record->variables;
                if (std::none_of(variables.begin(), variables.end(), [&](const auto& v) { return v.first == name; })) {
                    variables.emplace_back(name, credential ? std::string(masked_value) : value);
                }
            }
            cmd.replace(
                firstPercentPos, secondPercentPos - firstPercentPos + 1, mask && credential ? masked_value : value);
        };
        // cout << percentVar << "\n";

        // ****************************************************************************************
        // Look for generated variables that should NOT be overridden first:
        //    Variable like ECF_PASS can be overridden, i.e. with FREE_JOBS_PASSWORD
        //    However for job file generation we should use the generated variables first.
        //    if the user removes ECF_PASS then we are stuck with the wrong value in the script file
        //    FREE_JOBS_PASSWORD is left for the server to deal with
        // Leave ECF_JOB and ECF_JOBOUT out of this list: As user may legitimately override these. ECFLOW-999
        bool generated_variable = false;
        if (percentVar.find("ECF_") == 0) {
            if (percentVar.find(ecf::environment::ECF_HOST) != std::string::npos) {
                generated_variable = true;
            }
            else if (percentVar.find(ecf::environment::ECF_PORT) != std::string::npos) {
                generated_variable = true;
            }
            else if (percentVar.find(ecf::environment::ECF_TRYNO) != std::string::npos) {
                generated_variable = true;
            }
            else if (percentVar.find(ecf::environment::ECF_NAME) != std::string::npos) {
                generated_variable = true;
            }
            else if (percentVar.find(ecf::environment::ECF_PASS) != std::string::npos) {
                generated_variable = true;
            }
        }

        size_t firstColon = percentVar.find(':');

        // First search user variable (*ONLY* set user edit's the script)
        // Handle case: cmd = "%fred:bill% and where we have user variable "fred:bill"
        // Handle case: cmd = "%fred%      and where we have user variable "fred"
        // If we fail to find the variable we return false.
        // Note: When a variable is found, it can have an empty value  which is still valid
        std::string varValue;
        if (!user_edit_variables.empty() && search_user_edit_variables(percentVar, varValue, user_edit_variables)) {
            replace_reference(percentVar, varValue);
        }
        else if (generated_variable && firstColon == std::string::npos && lookup.find_generated(percentVar, varValue)) {
            replace_reference(percentVar, varValue);
        }
        else {
            if (firstColon != std::string::npos) {

                if (is_a_alias && lookup.find(percentVar, varValue)) {
                    // For alias, we could have added variables with %A:0%, %A:1%.
                    // Aliases allow variables with ':' in the name.
                    replace_reference(percentVar, varValue);
                }
                else {
                    // ':' is not a valid in variables, hence split, and search, if search fails use replacement
                    std::string var(percentVar.begin(), percentVar.begin() + firstColon);
                    if (!user_edit_variables.empty() &&
                        search_user_edit_variables(var, varValue, user_edit_variables)) {
                        replace_reference(var, varValue);
                    }
                    else if (generated_variable && lookup.find_generated(var, varValue)) {
                        replace_reference(var, varValue);
                    }
                    else if (lookup.find(var, varValue)) {
                        // Note: variable can exist, but have an empty value
                        // replace the "%VAR:fred --f%" with var
                        replace_reference(var, varValue);
                    }
                    else {
                        std::string substitute(percentVar.begin() + firstColon + 1, percentVar.end());
                        replace_reference(var, substitute);
                    }
                }
            }
            else if (lookup.find(percentVar, varValue)) {
                // No ':' search user variables, repeat, and then generated variables.
                replace_reference(percentVar, varValue);
            }
            else {
                // Cannot find in user variables, or node variable, hence cannot go any further
                return false;
            }
        }

        // Simple Check for infinite recursion
        if (count > 1000) {
            return false;
        }
        count++;
    }

    if (double_micro_found) {
        // replace all double micro with a single micro, this must be a single parse
        // date +%%Y%%m%%d" ==> date +%Y%m%d
        // %%%%             ==> %%            // i.e. single parse
        std::string doubleEcfMicro;
        doubleEcfMicro += micro;
        doubleEcfMicro += micro;
        size_t last_pos = 0;
        while (true) {
            auto ecf_double_micro_pos = cmd.find(doubleEcfMicro, last_pos);
            if (ecf_double_micro_pos != std::string::npos) {
                cmd.erase(cmd.begin() + ecf_double_micro_pos);
                last_pos = ecf_double_micro_pos + 1;
            }
            else {
                break;
            }
        }
    }

    return true;
}

} // namespace detail

bool variable_substitution(std::string& cmd,
                           const VariableLookup& lookup,
                           const NameValueMap& user_edit_variables,
                           char micro,
                           SubstitutionRecord* record) {
    if (record) {
        // The masked command is rendered separately, so that the command itself is substituted as without a record
        record->variables.clear();
        record->masked_command = cmd;
        detail::substitute_variables(record->masked_command, lookup, user_edit_variables, micro, nullptr, true);
    }
    return detail::substitute_variables(cmd, lookup, user_edit_variables, micro, record, false);
}

bool variable_substitution_with_user_micro(std::string& cmd, const VariableLookup& lookup, SubstitutionRecord* record) {
    char micro = '%';
    std::string micro_char;
    lookup.find_user(ecf::environment::ECF_MICRO, micro_char);
    if (!micro_char.empty() && micro_char.size() == 1) {
        micro = micro_char.front();
    }

    NameValueMap user_edit_variables;
    return variable_substitution(cmd, lookup, user_edit_variables, micro, record);
}

bool variable_dollar_substitution(std::string& cmd, const VariableLookup& lookup) {
    // scan command for environment variables, and substitute
    // edit ECF_INCLUDE $ECF_HOME/include

    while (true) {
        size_t firstPos = cmd.find('$');
        if (firstPos == std::string::npos) {
            break;
        }

        size_t secondPos = cmd.find_first_not_of(ecf::string_constants::alphanumeric_underscore_chars, firstPos + 1);
        if (secondPos == std::string::npos) {
            secondPos = cmd.size();
        }
        if (secondPos - firstPos <= 1) {
            break; // handle $/ with no characters in between
        }

        std::string env(cmd.begin() + firstPos + 1, cmd.begin() + secondPos);

        std::string envValue;
        if (!lookup.find(env, envValue)) {
            return false;
        }

        cmd.replace(firstPos, secondPos - firstPos, envValue);

        if (envValue.find(env) != std::string::npos) {
            // infinite loop
            break;
        }
    }
    return true;
}

} // namespace ecf
