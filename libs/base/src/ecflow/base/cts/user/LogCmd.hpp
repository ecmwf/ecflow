/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_cts_user_LogCmd_HPP
#define ecflow_base_cts_user_LogCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

/// The LogCmd is paired with SStringCmd
/// Client---(LogCmd)---->Server-----(SStringCmd)--->client:
/// When doHandleRequest is called in the server it will return SStringCmd
/// The SStringCmd is used to transport the log file contents to the client
class LogCmd final : public UserCmd {
public:
    enum LogApi { GET, CLEAR, FLUSH, NEW, PATH };
    explicit LogCmd(LogApi a,
                    int get_last_n_lines = 0); // for zero we take default from log. Avoid adding dependency on log.hpp
    explicit LogCmd(const std::string& path);  // NEW
    LogCmd();

    LogApi api() const { return api_; }
    int get_last_n_lines() const { return get_last_n_lines_; }
    const std::string& new_path() const { return new_path_; }

    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    bool isWrite() const override;
    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg(); // used for argument parsing

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    LogApi api_{LogCmd::GET};
    int get_last_n_lines_; // default to 100 -> ECFLOW-174
    std::string new_path_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(api_), CEREAL_NVP(get_last_n_lines_), CEREAL_NVP(new_path_));
    }
};

std::ostream& operator<<(std::ostream& os, const LogCmd&);

CEREAL_FORCE_DYNAMIC_INIT(LogCmd)

#endif /* ecflow_base_cts_user_LogCmd_HPP */
