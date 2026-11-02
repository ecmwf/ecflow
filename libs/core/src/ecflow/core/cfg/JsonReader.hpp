// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/core/cfg/Reader.hpp"

namespace ecf::cfg {

///
/// @brief Reads a configuration document in strict JSON (RFC 8259).
///
/// The reader refuses the following, naming the origin and, where known, the position:
///
///   - a syntax error, including comments and an empty document (with line and column)
///   - content after the document
///   - a key repeated in an object (with the path of the key)
///   - an integer outside the range of `std::int64_t` (with the path of the number)
///
/// A number with a fraction or an exponent, e.g. `1.5` or `1e3`, is read as a floating-point number.
///
class JsonReader final : public Reader {
public:
    [[nodiscard]] CTree read(std::string_view text, std::string_view origin) const override;
};

} // namespace ecf::cfg
