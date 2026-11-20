// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/base/cts/user/UserCmd.hpp"

class PlugCmd final : public UserCmd {
public:
    PlugCmd(const std::string& source, const std::string& dest)
        : source_(source),
          dest_(dest) {}
    PlugCmd() = default;

    // Uses by equals only
    const std::string& source() const { return source_; }
    const std::string& dest() const { return dest_; }

    ///
    /// @brief Returns the destination path on this server, when the destination is on this server.
    ///
    /// The destination is on this server when a node has the destination path, or when the path names this server
    /// (`host:port/path`, the host being this host or localhost) and a node has the rest of the path.
    ///
    /// @param[in] server This server
    /// @return The path of the destination node, or std::nullopt for a destination on another server (or none).
    ///
    std::optional<std::string> local_destination(const AbstractServer& server) const;

private:
    ///
    /// @brief Returns the denial of a plug that the receiving server denied, recording no edit.
    ///
    /// @param[in] server This server
    /// @param[in] host The receiving server's host
    /// @param[in] port The receiving server's port
    /// @return The denial, naming the request and the receiving server.
    ///
    STC_Cmd_ptr denied_by_receiver(AbstractServer& server, const std::string& host, const std::string& port) const;

public:
    time_duration_t timeout() const override { return std::chrono::seconds{120}; }
    bool handleRequestIsTestable() const override { return false; }
    bool isWrite() const override { return true; }
    using UserCmd::print;
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(const ecf::RequestContext& context) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg(); // used for argument parsing

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

private:
    std::string source_;
    std::string dest_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(source_), CEREAL_NVP(dest_));
    }
};

std::ostream& operator<<(std::ostream& os, const PlugCmd&);

CEREAL_FORCE_DYNAMIC_INIT(PlugCmd)
