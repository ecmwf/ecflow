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

    ///
    /// @brief Extract path and name from given token.
    ///
    /// Given:
    ///   "/suite/family:obj", extracts path = "/suite/family", and name = "obj"
    ///   "/suite/family", extracts path = "/suite/family", and name = ""
    ///   "obj", extracts path = "", and name = "obj"
    ///
    /// @param token the input token to extract from
    /// @param path  the extracted path component
    /// @param name  the extracted name component
    /// @return true if extraction succeeded; false, otherwise
    ///
    static bool pathAndName(const std::string& token, std::string& path, std::string& name);

    ///
    /// @brief Extract the 2nd token from the given string, using the provided separator.
    ///
    /// Given str = "HH:MM", extracts ret = "MM".
    ///
    /// @param str       the input string to split
    /// @param ret       the extracted second token
    /// @param separator the character used to split the string
    /// @return true if extraction succeeded; false, otherwise
    ///
    static bool split_get_second(const std::string& str, std::string& ret, char separator = ':');

    ///
    /// @brief Extract a value of the given arithmetic type from the provided token.
    ///
    /// @tparam TO the arithmetic type of the value to extract
    /// @param token the input token to convert
    /// @return a Result object containing either the extracted value, or an error message
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
    /// @brief Extract a value of the given type from the provided token, throwing on failure.
    ///
    /// @tparam TO the type of the value to extract
    /// @param token the input token to convert
    /// @param error the error message to include in the thrown exception
    /// @return the extracted value
    /// @throws std::runtime_error if the token cannot be converted to the requested type
    ///
    template <typename TO>
    static TO value(const std::string& token, const std::string& error) {
        if (auto result = Extract::value<TO>(token); result.ok()) {
            return result.value();
        }

        throw std::runtime_error(error);
    }

    ///
    /// @brief Extract a YMD integer of the form yyyymmdd from the given token.
    ///
    /// @param ymdToken the input token containing the date in yyyymmdd format
    /// @param errorMsg the error message to include in the thrown exception on failure
    /// @return the extracted date as an integer of the form yyyymmdd
    /// @throws std::runtime_error if the token is not a valid 8-character date
    ///
    static int ymd(const std::string& ymdToken, std::string& errorMsg);

    ///
    /// @brief Extract an optional integer from the token at the given index.
    ///
    /// When the first character of the selected token is '#', the default value is returned.
    ///
    /// Example:
    ///   ["repeat", "integer", "variable", "1", "2", "#a", "comment"]
    ///    - extracting from "repeat", "integer", "variable", or "comment" throws std::runtime_error
    ///    - extracting from "1", returns 1
    ///    - extracting from "2", returns 2
    ///    - extracting from "#a", returns the given default value
    ///   ["repeat", "integer", "variable", "1", "2", "#", "a", "comment"]
    ///    - extracting from "repeat", "integer", "variable", "a", or "comment" throws std::runtime_error
    ///    - extracting from "1", returns 1
    ///    - extracting from "2", returns 2
    ///    - extracting from "#", returns the given default value
    ///
    /// @tparam TO       the type to use for the conversion
    /// @param tokens    the sequence of tokens to extract from
    /// @param pos       the index of the token to extract
    /// @param defaultValue the value to return when the selected token starts with '#'
    /// @param errorMsg  the error message to include in the thrown exception on failure
    /// @return the extracted integer, or @p defaultValue if the selected token starts with '#'
    /// @throws std::runtime_error if extraction fails
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
