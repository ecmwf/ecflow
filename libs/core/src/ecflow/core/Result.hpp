/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Result_HPP
#define ecflow_core_Result_HPP

#include <string>
#include <variant>

namespace ecf {

template <typename V, typename E = std::string>
class Result {

    struct Error
    {
        E reason_;
    };

public:
    using value_t = V;
    using error_t = E;

    Result()              = delete;
    Result(const Result&) = default;
    Result(Result&&)      = default;

private:
    explicit Result(const V& value) : success_{true}, data_{value} {}
    explicit Result(V&& value) : success_{true}, data_{std::move(value)} {}
    explicit Result(const Error& error) : success_{false}, data_{error} {}

public:
    ~Result() = default;

    Result& operator=(const Result&) = default;
    Result& operator=(Result&&)      = default;

    static Result success(const V& value) { return Result(value); }
    static Result success(V&& value) { return Result(std::move(value)); }
    static Result failure(const E& error) { return Result{Error{error}}; }

    [[nodiscard]] bool ok() const { return success_; }
    [[nodiscard]] value_t value() { return std::move(std::get<value_t>(data_)); }
    [[nodiscard]] const value_t& value() const { return std::get<value_t>(data_); }
    [[nodiscard]] const error_t& reason() const { return std::get<Error>(data_).reason_; }

private:
    bool success_;
    std::variant<V, Error> data_;
};

} // namespace ecf

#endif /* ecflow_core_Result_HPP */
