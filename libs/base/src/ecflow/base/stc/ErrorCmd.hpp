// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/ErrorCode.hpp"

///
/// @brief Carries a server failure's diagnostic and stable numeric classification to the client.
///
/// @invariant A code of zero denotes an unclassified failure, never success.
///
class ErrorCmd final : public ServerToClientCmd {
public:
    ///
    /// @brief Constructs an empty reply for deserialisation or later initialisation.
    ///
    /// The diagnostic is initially empty and the numeric code is zero; deserialisation or init() populates the reply.
    ///
    ErrorCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Constructs an unclassified server failure with numeric code zero.
    ///
    /// @param[in] errorMsg Server diagnostic to transmit; must be non-empty.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    explicit ErrorCmd(const std::string& errorMsg);

    ///
    /// @brief Constructs a classified server failure without changing its diagnostic text.
    ///
    /// @param[in] errorMsg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Stable numeric classification of the failure.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    ErrorCmd(const std::string& errorMsg, ecf::ErrorCode code);

    ///
    /// @brief Constructs a server failure with a raw code, including unknown future values.
    ///
    /// @param[in] errorMsg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Raw numeric classification of the failure.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    ErrorCmd(const std::string& errorMsg, int code);

    ///
    /// @brief Replaces a reusable reply's diagnostic and numeric classification.
    ///
    /// @param[in] errorMsg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Raw numeric classification; zero resets an earlier code.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    void init(const std::string& errorMsg, int code = 0);

    ///
    /// @brief Replaces a reusable reply using a recognised classification.
    ///
    /// @param[in] errorMsg Server diagnostic to transmit; must be non-empty.
    /// @param[in] code Stable numeric classification of the failure.
    /// @throws std::bad_alloc if storing the diagnostic fails.
    ///
    void init(const std::string& errorMsg, ecf::ErrorCode code) { init(errorMsg, static_cast<int>(code)); }

    ///
    /// @brief Returns the raw numeric classification of this failure.
    ///
    /// @return Received code, including zero or an unknown future value.
    /// Never throws.
    ///
    int code() const noexcept { return code_; }

    ///
    /// @brief Formats the reply for diagnostics without its numeric classification.
    ///
    /// @return Text in the form "cmd:Error [ <diagnostic> ]".
    /// @throws std::bad_alloc if constructing the text fails.
    ///
    std::string print() const override;

    ///
    /// @brief Checks whether another reply has the ErrorCmd type.
    ///
    /// @param[in] rhs Reply to compare; a null or different reply type does not match.
    /// @return True for another ErrorCmd according to the base reply comparison.
    ///
    bool equals(ServerToClientCmd* rhs) const override;

    ///
    /// @brief Appends the failure diagnostic and records a genuine server code in the reply state.
    ///
    /// Transport-generated failures retain their diagnosis without inventing a server code.
    /// Within a group, the raw code and message are appended as one ordered child entry.
    ///
    /// @param[in,out] server_reply Client reply state to update.
    /// @param[in] cts_cmd Request used to prefix the formatted diagnostic.
    /// @param[in] debug Whether to print the raw diagnostic to standard output.
    /// @return False because an ErrorCmd always represents failure.
    /// @throws std::bad_alloc if constructing or storing the diagnostic fails.
    ///
    bool handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const override;

    ///
    /// @brief Returns the unformatted diagnostic, including for grouped reply inspection.
    ///
    /// @return A copy of the server diagnostic.
    /// @throws std::bad_alloc if copying the diagnostic fails.
    ///
    std::string error() const override { return error_msg_; }

    ///
    /// @brief Reports that this reply represents a failed command.
    ///
    /// @return False, regardless of the numeric code (including zero).
    ///
    bool ok() const noexcept override { return false; }

    ///
    /// @copydoc ServerToClientCmd::cleanup
    ///
    /// Resets the numeric classification as well as the diagnostic before reply reuse.
    ///
    void cleanup() override {
        std::string{}.swap(error_msg_);
        code_ = 0;
    }

private:
    std::string error_msg_;
    int code_{0};

    friend class cereal::access;
    ///
    /// @brief Archives the diagnostic and, for version 1, its numeric classification.
    ///
    /// Loading a version-0 reply defaults the code to zero for compatibility with old servers.
    ///
    /// @tparam Archive Cereal input or output archive type.
    /// @param[in,out] ar Archive to read or write.
    /// @param[in] version Serialized ErrorCmd class version.
    /// @throws cereal::Exception if the archive cannot be read or written.
    ///
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(error_msg_));
        if (version >= 1) {
            ar(CEREAL_NVP(code_));
        }
        else if constexpr (Archive::is_loading::value) {
            code_ = 0;
        }
    }
};

CEREAL_CLASS_VERSION(ErrorCmd, 1)

///
/// @brief Writes the formatted failure diagnostic without its numeric classification.
///
/// @param[in,out] os Destination stream.
/// @param[in] error Reply to render.
/// @return The destination stream.
///
std::ostream& operator<<(std::ostream& os, const ErrorCmd& error);
