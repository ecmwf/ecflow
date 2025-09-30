/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Extract_HPP
#define ecflow_core_Extract_HPP

#include <string>
#include <vector>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Result.hpp"

class Extract {
public:
    // Disable default construction
    Extract() = delete;
    // Disable copy (and move) semantics
    Extract(const Extract&)                  = delete;
    const Extract& operator=(const Extract&) = delete;

    ///
    /// Extract path and name from given token.
    ///
    /// Given
    ///   "/suite/family:obj", extracts path = "/suite/family", and name = "obj"
    ///   "/suite/family", extracts path = "/suite/family", and name = ""
    ///   "obj", extracts path = "", and name = "obj"
    /// @returns true if extraction succeeded; false, otherwise
    ///
    static bool pathAndName(const std::string& token, std::string& path, std::string& name);

    ///
    /// Extract 2nd token from the given string (considering provided separator)
    ///
    /// Given str = "HH:MM", extracts ret = "MM"
    ///
    /// @returns true if extraction succeeded; false, otherwise
    ///
    static bool split_get_second(const std::string& str, std::string& ret, char separator = ':');

    ///
    /// Extract a value of the given type from the provided token
    ///
    /// @tparam TO the type of the value to extract
    /// @param token the input token
    /// @return a Result object, containing either the extracted value, or an error message
    ///
    template <typename TO, typename = std::enable_if_t<std::is_arithmetic_v<TO>>>
    static ecf::Result<TO> value(const std::string& token) {
        try {
            TO v = ecf::convert_to<TO>(token);
            return ecf::Result<TO>::success(v);
        }
        catch (const ecf::bad_conversion&) {
            return ecf::Result<TO>::failure("Unable to convert '" + token + "' as " + typeid(TO).name() + "");
        }
    }

    ///
    /// Extract a value of the given type from the provided token
    ///
    /// @tparam TO the type of the value to extract
    /// @param token the input token
    /// @param error the error message to the included in the thrown exception
    /// @return the extracted value, or throws std::runtime_error exception with the specified error message
    ///
    template <typename TO>
    static TO value(const std::string& token, const std::string& error) {
        if (auto result = Extract::value<TO>(token); result.ok()) {
            return result.value();
        }

        throw std::runtime_error(error);
    }

    ///
    /// Extract YMD integer, of the form yyyymmdd, from the given token
    ///
    /// @throws std::runtime_error if extractions fails, with the provided error message included in exception message
    ///
    static int ymd(const std::string& ymdToken, std::string& errorMsg);

    ///
    /// Extract (optional) integer, from token in index pos
    ///
    /// Notice: when first character of selected token is '#', the return is the provided default value
    ///
    /// Example of tokens:
    ///   ["repeat", "integer", "variable", "1", "2", "#a", "comment"]
    ///    - extracting from "repeat", "integer", "variable", or "comment" throws std::runtime_error
    ///    - extracting from "1", returns 1
    ///    - extracting from "2", returns 3
    ///    - extracting from "#a", returns the given default value
    ///   ["repeat", "integer", "variable", "1", "2", "#", "a", "comment"]
    ///    - extracting from "repeat", "integer", "variable", "a", or "comment" throws std::runtime_error
    ///    - extracting from "1", returns 1
    ///    - extracting from "2", returns 3
    ///    - extracting from "#", returns the given default value
    ///
    /// @returns the extracted integer, if extraction succeeded; default value if selected token starts with '#'
    /// @throws std::runtime_error if extractions fails, with the provided error message included in exception message
    ///
    ///
    template <typename TO>
    static int
    optional_value(const std::vector<std::string>& tokens, size_t pos, int defaultValue, const std::string& errorMsg) {
        if (pos < tokens.size() && tokens[pos][0] != '#') {
            return value<TO>(tokens[pos], errorMsg);
        }
        return defaultValue;
    }
};

#endif /* ecflow_core_Extract_HPP */
