/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Environment_HPP
#define ecflow_core_Environment_HPP

#include <optional>

#include "ecflow/core/Message.hpp"

namespace ecf {

namespace environment {

constexpr const char* ECF_PORT            = "ECF_PORT";
constexpr const char* ECF_RID             = "ECF_RID";
constexpr const char* ECF_TRYNO           = "ECF_TRYNO";
constexpr const char* ECF_TRIES           = "ECF_TRIES";
constexpr const char* ECF_NAME            = "ECF_NAME";
constexpr const char* ECF_HOST            = "ECF_HOST";
constexpr const char* ECF_HOST_PROTOCOL   = "ECF_HOST_PROTOCOL";
constexpr const char* ECF_HOSTFILE        = "ECF_HOSTFILE";
constexpr const char* ECF_HOSTFILE_POLICY = "ECF_HOSTFILE_POLICY";
constexpr const char* ECF_TIMEOUT         = "ECF_TIMEOUT";
constexpr const char* ECF_ZOMBIE_TIMEOUT  = "ECF_ZOMBIE_TIMEOUT";
constexpr const char* ECF_CONNECT_TIMEOUT = "ECF_CONNECT_TIMEOUT";
constexpr const char* ECF_DENIED          = "ECF_DENIED";
constexpr const char* ECF_PASS            = "ECF_PASS";
constexpr const char* NO_ECF              = "NO_ECF";
constexpr const char* ECF_JOB             = "ECF_JOB";
constexpr const char* ECF_JOBOUT          = "ECF_JOBOUT";
constexpr const char* ECF_SCRIPT          = "ECF_SCRIPT";
constexpr const char* ECF_DUMMY_TASK      = "ECF_DUMMY_TASK";
constexpr const char* ECF_NO_SCRIPT       = "ECF_NO_SCRIPT";
constexpr const char* ECF_MICRO           = "ECF_MICRO";
constexpr const char* ECF_FILES           = "ECF_FILES";
constexpr const char* ECF_FETCH           = "ECF_FETCH";
constexpr const char* ECF_KILL_CMD        = "ECF_KILL_CMD";
constexpr const char* ECF_STATUS_CMD      = "ECF_STATUS_CMD";
constexpr const char* ECF_HOME            = "ECF_HOME";
constexpr const char* ECF_INCLUDE         = "ECF_INCLUDE";
constexpr const char* ECF_JOB_CMD         = "ECF_JOB_CMD";
constexpr const char* ECF_OUT             = "ECF_OUT";
constexpr const char* ECF_EXTN            = "ECF_EXTN";
constexpr const char* ECF_LOG             = "ECF_LOG";
constexpr const char* ECF_PASSWD          = "ECF_PASSWD";
constexpr const char* ECF_CUSTOM_PASSWD   = "ECF_CUSTOM_PASSWD";

constexpr const char* ECF_SSL  = "ECF_SSL";
constexpr const char* ECF_USER = "ECF_USER";

constexpr const char* ECF_DEBUG_CLIENT = "ECF_DEBUG_CLIENT";
constexpr const char* ECF_DEBUG_LEVEL  = "ECF_DEBUG_LEVEL";

namespace /* anonymous */ {

template <typename T>
struct Wrapper
{
    static std::optional<T> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return T{var};
        }
        return std::nullopt;
    }
};

template <>
struct Wrapper<const char*>
{
    static std::optional<const char*> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return std::make_optional(var);
        }
        return std::nullopt;
    }
};

template <>
struct Wrapper<int>
{
    static std::optional<int> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return atoi(var);
        }
        return std::nullopt;
    }
};

template <>
struct Wrapper<unsigned int>
{
    static std::optional<int> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return atoi(var);
        }
        return std::nullopt;
    }
};

template <>
struct Wrapper<long>
{
    static std::optional<long> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return atol(var);
        }
        return std::nullopt;
    }
};

template <>
struct Wrapper<bool>
{
    static std::optional<bool> get(const char* name) {
        if (auto var = std::getenv(name); var) {
            return true;
        }
        return std::nullopt;
    }
};

} // namespace

struct EnvVarNotFound : public std::runtime_error
{
    explicit EnvVarNotFound(std::basic_string<char> what) : std::runtime_error(what) {}
};

/**
 * @brief Retrieves the environment variable value and stores it in the given variable.
 *        If the environment variable is not found, the variable is left unchanged.
 *
 *        In case of integral types, the environment variable is converted to the corresponding type.
 *        In case of bool type, if the environment variable is set, the variable is set to true.
 *
 * @tparam T
 * @param name
 * @param value
 */
template <typename T>
void get(const char* name, T& value) {
    if (auto found = Wrapper<T>::get(name); found) {
        value = found.value();
    }
}

/**
 * @brief Retrieves the environment variable value and returns it.
 *        If the environment variable is not found, an exception is thrown.
 *
 *        In case of integral types, the environment variable is converted to the corresponding type.
 *        In case of bool type, if the environment variable is set, the result is true.
 *
 * @tparam T
 * @param name
 */
template <typename T = std::string>
T get(const char* name) {
    if (auto found = Wrapper<T>::get(name); found) {
        return found.value();
    }

    throw EnvVarNotFound(Message(name).str());
}

/**
 * @brief Retrieves the environment variable value and returns it (wrapped in a std::optional<>).
 *        If the environment variable is not found, std::nullopt is returned.
 *
 *        In case of integral types, the environment variable is converted to the corresponding type.
 *        In case of bool type, if the environment variable is set, the result is true.
 *
 * @tparam T
 * @param name
 */
template <typename T = std::string>
std::optional<T> fetch(const char* name) {
    return Wrapper<T>::get(name);
}

/**
 * @brief Checks if an environment variable is set.
 *
 * @tparam T
 * @param name
 */
inline bool has(const char* name) {
    return std::getenv(name) != nullptr;
}

} // namespace environment

} // namespace ecf

#endif /* ecflow_core_Environment_HPP */
