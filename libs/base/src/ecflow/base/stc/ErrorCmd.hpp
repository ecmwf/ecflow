// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

class ErrorCmd final : public ServerToClientCmd {
public:
    ///
    /// @brief Identifies the kind of an error reply.
    ///
    enum class Kind {
        Error,           ///< Any error
        PermissionDenied ///< The request was refused by the authorisation of the server
    };

    explicit ErrorCmd(const std::string& errorMsg, Kind kind = Kind::Error);
    ErrorCmd()
        : ServerToClientCmd() {}

    ///
    /// @brief Sets the message and the kind of the reply.
    ///
    /// An error is logged; a denial is not, since the server records denials itself, at a bounded rate.
    ///
    /// @param errorMsg The message, which must not be empty
    /// @param kind The kind
    ///
    void init(const std::string& errorMsg, Kind kind = Kind::Error);

    ///
    /// @brief Returns the kind of the reply.
    ///
    /// @return The kind.
    ///
    [[nodiscard]] Kind kind() const { return kind_; }
    std::string print() const override;
    bool equals(ServerToClientCmd*) const override;
    bool handle_server_response(ServerReply&, Cmd_ptr cts_cmd, bool debug) const override;

    std::string error() const override { return error_msg_; }   /// Used by test
    bool ok() const override { return false; }                  /// Used by group command
    void cleanup() override { std::string{}.swap(error_msg_); } /// run in the server, after command send to client

private:
    std::string error_msg_;
    Kind kind_{Kind::Error};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<ServerToClientCmd>(this), CEREAL_NVP(error_msg_));
        // The kind is written only for a denial, and last, so that a reader that does not know it reads the other
        // members unchanged
        if constexpr (Archive::is_saving::value) {
            if (kind_ != Kind::Error) {
                ar(cereal::make_nvp("kind_", static_cast<int>(kind_)));
            }
        }
        else {
            int kind = static_cast<int>(Kind::Error);
            (void)cereal::make_optional_nvp(ar, "kind_", kind);
            kind_ = kind == static_cast<int>(Kind::PermissionDenied) ? Kind::PermissionDenied : Kind::Error;
        }
    }
};

std::ostream& operator<<(std::ostream& os, const ErrorCmd&);
