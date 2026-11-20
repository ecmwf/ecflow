// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/cts/user/UserCmd.hpp"

class ReplaceNodeCmd final : public UserCmd {
public:
    ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const defs_ptr& client_defs, bool force);
    ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const std::string& path_to_defs, bool force);
    ReplaceNodeCmd() = default;

    const std::string& the_client_defs() const { return clientDefs_; }
    const std::string& pathToNode() const { return pathToNode_; }
    const std::string& path_to_defs() const { return path_to_defs_; }
    bool createNodesAsNeeded() const { return createNodesAsNeeded_; }
    bool force() const { return force_; }

    ///
    /// @brief Returns the definitions the command carries, parsed on first use and kept for the request.
    ///
    /// The authorisation reads them before the command runs, which then uses them instead of parsing again.
    ///
    /// @return The definitions; nullptr when they cannot be parsed, the command then reporting the error.
    ///
    defs_ptr client_definitions() const;

    ///
    /// @brief Identifies the nodes concerned by a replace, as Defs::replaceChild determines them.
    ///
    struct Target
    {
        node_ptr existing; ///< The node of the server at the path, if any
        const Node*
            imported{};  ///< The subtree imported: the client node, or its topmost ancestor missing on the server
        node_ptr parent; ///< The node of the server under which the subtree goes; null for a suite
    };

    ///
    /// @brief Returns the nodes concerned by the replace of a node.
    ///
    /// @param[in] server The definitions of the server
    /// @param[in] client_node The node at the path, in the client definitions
    /// @param[in] create Whether missing parents are created
    /// @return The nodes; when the node exists, imported is the client node and parent that of the existing node.
    ///
    static Target target(const Defs& server, const node_ptr& client_node, bool create);

    bool isWrite() const override { return true; }
    time_duration_t timeout() const override { return std::chrono::seconds{300}; }
    using UserCmd::print;
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(const ecf::RequestContext& context) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

    // void set_client_env(const std::vector<std::pair<std::string,std::string> >& env ) { client_env_ = env;} // only
    // used in test

private:
    static const char* arg(); // used for argument parsing

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { /// run in the server, after command send to client
        std::string{}.swap(clientDefs_);
        parsed_.reset();
    }

    bool createNodesAsNeeded_{false};
    bool force_{false};
    std::string pathToNode_;
    std::string path_to_defs_; // Can be empty if defs loaded in memory via python api
    std::string clientDefs_;
    mutable defs_ptr parsed_; // the client definitions, once parsed; not serialised

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(createNodesAsNeeded_),
           CEREAL_NVP(force_),
           CEREAL_NVP(pathToNode_),
           CEREAL_NVP(path_to_defs_),
           CEREAL_NVP(clientDefs_));
    }
};

std::ostream& operator<<(std::ostream& os, const ReplaceNodeCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ReplaceNodeCmd)
