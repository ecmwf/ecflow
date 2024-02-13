/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Child.hpp"

#include <cassert>

#include "ecflow/core/Enumerate.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

namespace detail {

template <>
struct EnumTraits<Child::ZombieType>
{
    using underlying_t = std::underlying_type_t<Child::ZombieType>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(Child::ZombieType::USER, "user"),
        std::make_pair(Child::ZombieType::ECF, "ecf"),
        std::make_pair(Child::ZombieType::ECF_PID, "ecf_pid"),
        std::make_pair(Child::ZombieType::ECF_PASSWD, "ecf_passwd"),
        std::make_pair(Child::ZombieType::ECF_PID_PASSWD, "ecf_pid_passwd"),
        std::make_pair(Child::ZombieType::PATH, "path"),
        std::make_pair(Child::ZombieType::NOT_SET, "not_set"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<Child::ZombieType>::size == map.back().first + 1);
};

template <>
struct EnumTraits<Child::CmdType>
{
    using underlying_t = std::underlying_type_t<Child::CmdType>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(Child::CmdType::INIT, "init"),
        std::make_pair(Child::CmdType::EVENT, "event"),
        std::make_pair(Child::CmdType::METER, "meter"),
        std::make_pair(Child::CmdType::LABEL, "label"),
        std::make_pair(Child::CmdType::WAIT, "wait"),
        std::make_pair(Child::CmdType::QUEUE, "queue"),
        std::make_pair(Child::CmdType::ABORT, "abort"),
        std::make_pair(Child::CmdType::COMPLETE, "complete"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<Child::CmdType>::size == map.back().first + 1);
};

} // namespace detail

std::string Child::to_string(Child::ZombieType zt) {
    if (auto found = Enumerate<Child::ZombieType>::to_string(zt); found) {
        return std::string{found.value()};
    }
    return std::string{};
}

Child::ZombieType Child::zombie_type(const std::string& s) {
    return Enumerate<Child::ZombieType>::to_enum(s).value_or(Child::NOT_SET);
}

bool Child::valid_zombie_type(const std::string& s) {
    return Enumerate<Child::ZombieType>::is_valid(s);
}

std::string Child::to_string(const std::vector<Child::CmdType>& vec) {
    std::string ret;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            ret += ",";
        }
        ret += to_string(vec[i]);
    }
    return ret;
}

std::string Child::to_string(Child::CmdType ct) {
    if (auto found = Enumerate<Child::CmdType>::to_string(ct); found) {
        return std::string{found.value()};
    }
    assert(false);
    return std::string{Enumerate<Child::CmdType>::to_string(Child::INIT).value()};
}

std::vector<Child::CmdType> Child::child_cmds(const std::string& s) {
    // expect single or , separated tokens
    std::vector<std::string> tokens;
    Str::split(s, tokens, ",");
    std::vector<Child::CmdType> ret;
    ret.reserve(tokens.size());
    for (const auto& token : tokens) {
        ret.push_back(child_cmd(token));
    }
    return ret;
}

Child::CmdType Child::child_cmd(const std::string& s) {
    if (auto found = Enumerate<Child::CmdType>::to_enum(s); found) {
        return found.value();
    }
    assert(false);
    return Child::INIT;
}

bool Child::valid_child_cmds(const std::string& s) {
    // empty means all children
    if (s.empty()) {
        return true;
    }

    // expect single or , separated tokens
    std::vector<std::string> tokens;
    Str::split(s, tokens, ",");
    for (const auto& token : tokens) {
        if (!valid_child_cmd(token)) {
            return false;
        }
    }
    return true;
}

bool Child::valid_child_cmd(const std::string& s) {
    return Enumerate<Child::CmdType>::is_valid(s);
}

std::vector<Child::CmdType> Child::list() {
    return Enumerate<Child::CmdType>::enums();
}

} // namespace ecf
