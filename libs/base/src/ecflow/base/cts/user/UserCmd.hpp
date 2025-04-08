/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_UserCmd_HPP
#define ecflow_base_cts_user_UserCmd_HPP

#include "ecflow/base/Cmd.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"

//=================================================================================
// User Command
// ================================================================================
class UserCmd : public ClientToServerCmd {
public:
    UserCmd() = default;

    const std::string& user() const { return user_; }
    const std::string& passwd() const { return pswd_; }
    bool is_custom_user() const { return cu_; }

    void setup_user_authentification(const std::string& user, const std::string& passwd) override;
    bool setup_user_authentification(AbstractClientEnv&) override;
    void setup_user_authentification() override;

protected:
    bool equals(ClientToServerCmd*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    // bool do_authenticate(AbstractServer* as, STC_Cmd_ptr&, const std::string& path) const;
    // bool do_authenticate(AbstractServer* as, STC_Cmd_ptr&, const std::vector<std::string>& paths) const;

    /// Prompt the user for confirmation: If user responds with no, will exit client
    static void prompt_for_confirmation(const std::string& prompt);

    /// All user commands will be pre_fixed with "--" and post_fixed with :user@host
    void user_cmd(std::string& os, const std::string& the_cmd) const;

    static int time_out_for_load_sync_and_get();

    // The order is preserved during the split. Paths assumed to start with '/' char
    static void split_args_to_options_and_paths(const std::vector<std::string>& args,
                                                std::vector<std::string>& options,
                                                std::vector<std::string>& paths,
                                                bool treat_colon_in_path_as_path = false);

private:
    std::string user_;
    std::string pswd_;
    bool cu_ = false; // custom user, i.e used set_user_name() || ECF_USER || --user -> only check this password

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<ClientToServerCmd>(this), CEREAL_NVP(user_));
        CEREAL_OPTIONAL_NVP(ar, pswd_, [this]() { return !pswd_.empty(); }); // conditionally save
        CEREAL_OPTIONAL_NVP(ar, cu_, [this]() { return cu_; });              // conditionally save
    }
};

#endif /* ecflow_base_cts_user_UserCmd_HPP */
