/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/InLimit.hpp"

#include <stdexcept>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Limit.hpp"
#ifdef DEBUG
    #include "ecflow/core/Ecf.hpp"
#endif

using namespace ecf;

namespace ecf {

InlimitOptions parse_inlimit_value(std::string value) {
    if (value.empty()) {
        return InlimitOptions{/* tokens = */ 1, /* limited_submission = */ false, /* limited_node = */ false};
    }

    bool limited_submission               = false;
    constexpr const char* submission_flag = "-s";
    if (ecf::algorithm::contains(value, submission_flag)) {
        limited_submission = true;
        ecf::algorithm::remove_all(value, submission_flag);
    }

    bool limited_node               = false;
    constexpr const char* node_flag = "-n";
    if (ecf::algorithm::contains(value, node_flag)) {
        limited_node = true;
        ecf::algorithm::remove_all(value, node_flag);
    }

    if (limited_submission && limited_node) {
        throw std::runtime_error("AlterCmd: an inlimit cannot be limited for both submission and node");
    }

    ecf::algorithm::trim(value);

    if (value.empty()) {
        return InlimitOptions{/* tokens = */ 1, limited_submission, limited_node};
    }

    try {
        int tokens = ecf::convert_to<int>(value);

        if (tokens <= 0) {
            throw std::runtime_error(
                MESSAGE("AlterCmd: the inlimit value must be > 0, but value was: '" << tokens << "'"));
        }

        return InlimitOptions{tokens, limited_submission, limited_node};
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error(
            MESSAGE("AlterCmd: the inlimit value, '" << value << "', cannot be converted to an integer"));
    }
}

} // namespace ecf

InLimit::InLimit(const std::string& name,
                 const std::string& pathToNode,
                 int tokens,
                 bool limit_this_node_only,
                 bool limit_submission,
                 bool check)
    : n_(name),
      path_(pathToNode),
      tokens_(tokens),
      limit_this_node_only_(limit_this_node_only),
      limit_submission_(limit_submission) {
    if (check && !ecf::algorithm::is_valid_name(name)) {
        throw std::runtime_error("InLimit::InLimit: Invalid InLimit name: " + name);
    }
    if (limit_this_node_only_ && limit_submission_) {
        throw std::runtime_error(
            "InLimit::InLimit: can't limit family only(-n) and limit submission(-s) at the same time");
    }
}

InLimit InLimit::make_from_name_and_value(const std::string& name, const std::string& value) {
    // Parse the inlimit 'name', separating it into path and actual name
    std::string l_path; // This can be empty
    std::string l_name;
    if (!Extract::pathAndName(name, l_path, l_name)) {
        throw std::runtime_error("Invalid inlimit reference to '[</path/to/node:]<limit-name>': " + name);
    }

    // Parse the inlimit 'value', separating tokens from the flags for submission and node only
    auto [l_tokens, l_submission, l_node] = parse_inlimit_value(value);

    // Create the actual limit, performing additional validation of the parameters (throwing if not valid)
    return InLimit(l_name, l_path, l_tokens, l_node, l_submission);
}

bool InLimit::operator==(const InLimit& rhs) const {
    if (path_ != rhs.path_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==   path_ != rhs.path_\n";
        }
#endif
        return false;
    }
    if (n_ != rhs.n_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==     n_ != rhs.n_\n";
        }
#endif
        return false;
    }
    if (tokens_ != rhs.tokens_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==    tokens_(" << tokens_ << ") != rhs.tokens_(" << rhs.tokens_ << ")\n";
        }
#endif
        return false;
    }

    if (limit_this_node_only_ != rhs.limit_this_node_only_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==    limit_this_node_only_(" << limit_this_node_only_
                      << ") != rhs.limit_this_node_only_(" << rhs.limit_this_node_only_ << ")\n";
        }
#endif
        return false;
    }
    if (limit_submission_ != rhs.limit_submission_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==     limit_submission_(" << limit_submission_
                      << ") != rhs.limit_submission_ (" << rhs.limit_submission_ << ")\n";
        }
#endif
        return false;
    }
    if (incremented_ != rhs.incremented_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "InLimit::operator==    incremented_(" << incremented_ << ") != rhs.incremented_("
                      << rhs.incremented_ << ")\n";
        }
#endif
        return false;
    }

    // Note: comparison does not look at Limit pointers
    return true;
}

std::string InLimit::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void InLimit::write(std::string& ret) const {
    ret += "inlimit ";
    if (limit_this_node_only_) {
        ret += "-n ";
    }
    if (limit_submission_) {
        ret += "-s ";
    }
    if (path_.empty()) {
        ret += n_;
    }
    else {
        ret += path_;
        ret += ecf::string_constants::colon;
        ret += n_;
    }
    if (tokens_ != 1) {
        ret += " ";
        ret += ecf::convert_to<std::string>(tokens_);
    }
}

template <class Archive>
void InLimit::serialize(Archive& ar) {
    ar(CEREAL_NVP(n_));
    CEREAL_OPTIONAL_NVP(ar, path_, [this]() { return !path_.empty(); }); // conditionally save
    CEREAL_OPTIONAL_NVP(ar, tokens_, [this]() { return tokens_ != 1; }); // conditionally save
    CEREAL_OPTIONAL_NVP(
        ar, limit_this_node_only_, [this]() { return limit_this_node_only_; }); // conditionally save new to 5.0.0
    CEREAL_OPTIONAL_NVP(
        ar, limit_submission_, [this]() { return limit_submission_; });       // conditionally save new to 5.0.0
    CEREAL_OPTIONAL_NVP(ar, incremented_, [this]() { return incremented_; }); // conditionally save new to 5.0.0
}
CEREAL_TEMPLATE_SPECIALIZE(InLimit);
