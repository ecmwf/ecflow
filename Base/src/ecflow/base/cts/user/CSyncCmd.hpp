/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_CSyncCmd_HPP
#define ecflow_base_cts_user_CSyncCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Client---(CSyncCmd::SYNC_FULL)---->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::SYNC)--------->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::SYNC_CLOCK)--->Server-----(SSyncCmd)--->client:
// Client---(CSyncCmd::NEWS)--------->Server-----(SNewsCmd)--->client:
class CSyncCmd final : public UserCmd {
public:
    enum Api { NEWS, SYNC, SYNC_FULL, SYNC_CLOCK };

    CSyncCmd(Api a,
             unsigned int client_handle,
             unsigned int client_state_change_no,
             unsigned int client_modify_change_no)
        : api_(a),
          client_handle_(client_handle),
          client_state_change_no_(client_state_change_no),
          client_modify_change_no_(client_modify_change_no) {}
    explicit CSyncCmd(unsigned int client_handle) : api_(SYNC_FULL), client_handle_(client_handle) {}
    CSyncCmd() = default;

    Api api() const { return api_; }
    int client_state_change_no() const { return client_state_change_no_; }
    int client_modify_change_no() const { return client_modify_change_no_; }
    int client_handle() const { return client_handle_; }

    void set_client_handle(int client_handle) override { client_handle_ = client_handle; } // used by group_cmd
    void print(std::string&) const override;
    std::string print_short() const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;
    int timeout() const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    /// Custom handling of command logging to add additional debug on same line
    /// makes it easier to debug errors in syncing.
    void do_log(AbstractServer*) const override;

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    Api api_{SYNC};
    int client_handle_{0};
    int client_state_change_no_{0};
    int client_modify_change_no_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(api_),
           CEREAL_NVP(client_handle_),
           CEREAL_NVP(client_state_change_no_),
           CEREAL_NVP(client_modify_change_no_));
    }
};

std::ostream& operator<<(std::ostream& os, const CSyncCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CSyncCmd)

#endif /* ecflow_base_cts_user_CSyncCmd_HPP */
