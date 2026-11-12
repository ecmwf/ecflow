// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ecf {

///
/// @brief Defines stable numeric classifications for failed server operations.
///
/// @invariant Published values are never renumbered or reused; successful operations have no error code.
///
enum class ErrorCode : int { UNCLASSIFIED = 0, FILE_UNAVAILABLE = 1, EMPTY_CONTENT = 2 };

///
/// @brief Retains a raw server code, including values unknown to this client.
///
class CodedError : public std::runtime_error {
public:
    ///
    /// @brief Constructs a coded failure with formatted and raw diagnostic text.
    ///
    /// @param[in] description Existing client-facing diagnostic text.
    /// @param[in] code Raw numeric value received from the server, including zero or an unknown value.
    /// @param[in] message Unformatted server diagnostic text; empty for a locally constructed failure.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    CodedError(const std::string& description, int code, std::string message = {})
        : std::runtime_error(description),
          code_(code),
          message_(std::move(message)) {}

    ///
    /// @brief Returns the raw numeric code, even if not recognised by this client.
    ///
    /// @return Received numeric code.
    ///
    int code() const noexcept { return code_; }

    ///
    /// @brief Returns the unformatted server diagnostic.
    ///
    /// @return Server diagnostic text, valid for the lifetime of this error.
    ///
    const std::string& message() const noexcept { return message_; }

private:
    int code_;
    std::string message_;
};

///
/// @brief Identifies failures with a recognised file-specific error code.
///
class FileHandlingError : public CodedError {
public:
    ///
    /// @brief Constructs a file failure with a recognised numeric code.
    ///
    /// @param[in] description Existing client-facing diagnostic.
    /// @param[in] code Recognised file classification; not UNCLASSIFIED.
    /// @param[in] message Raw server diagnostic.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    FileHandlingError(const std::string& description, ErrorCode code, std::string message = {})
        : CodedError(description, static_cast<int>(code), std::move(message)) {}
};

///
/// @brief Identifies a failed operation caused by an existing but empty file.
///
class EmptyContentError : public FileHandlingError {
public:
    ///
    /// @brief Constructs a file-content failure without changing the diagnostic.
    ///
    /// @param[in] description Existing client-facing diagnostic.
    /// @param[in] message Raw server diagnostic.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    EmptyContentError(const std::string& description, std::string message = {})
        : FileHandlingError(description, ErrorCode::EMPTY_CONTENT, std::move(message)) {}
};

///
/// @brief Retains a group failure with ordered, independent child diagnostics.
///
class GroupError : public std::runtime_error {
public:
    ///
    /// @brief Associates one failed child's raw numeric code with its server message.
    ///
    using Entry = std::pair<int, std::string>;

    ///
    /// @brief Constructs an aggregate error without an aggregate numeric code.
    ///
    /// @param[in] description Existing concatenated client diagnostic.
    /// @param[in] entries Failed child codes and unformatted messages in reply order.
    /// @throws std::bad_alloc if storing the diagnostic or entries fails.
    ///
    GroupError(const std::string& description, std::vector<Entry> entries)
        : std::runtime_error(description),
          entries_(std::move(entries)) {}

    ///
    /// @brief Returns each failed child in reply order.
    ///
    /// @return Ordered child code and raw message pairs, valid for the lifetime of this error.
    ///
    const std::vector<Entry>& entries() const noexcept { return entries_; }

private:
    std::vector<Entry> entries_;
};

} // namespace ecf
