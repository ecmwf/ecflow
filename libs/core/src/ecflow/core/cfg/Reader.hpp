// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <string_view>

#include "ecflow/core/cfg/CTree.hpp"

namespace ecf::cfg {

///
/// @brief Reads a configuration document of a given file format into a CTree.
///
/// A reader only reads: it refuses a document that is malformed or ambiguous, but does not judge its content.
///
class Reader {
public:
    virtual ~Reader() = default;

    ///
    /// @brief Reads a document from the given text.
    ///
    /// @param[in] text The document
    /// @param[in] origin Where the document comes from, e.g. the file name, used in error messages
    /// @return The tree of the document.
    /// @throws ConfigurationError if the document is malformed or ambiguous; the message names the origin.
    ///
    [[nodiscard]] virtual CTree read(std::string_view text, std::string_view origin) const = 0;

    ///
    /// @brief Reads a document from the given file.
    ///
    /// @param[in] file The file
    /// @return The tree of the document.
    /// @throws ConfigurationError if the file is not a readable regular file, or the document is malformed or
    ///   ambiguous; the message names the file.
    ///
    [[nodiscard]] CTree read_file(const std::filesystem::path& file) const;
};

} // namespace ecf::cfg
