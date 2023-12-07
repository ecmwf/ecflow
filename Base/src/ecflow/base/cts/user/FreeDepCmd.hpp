/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_FreeDepCmd_HPP
#define ecflow_base_cts_user_FreeDepCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Free Dependencies
class FreeDepCmd final : public UserCmd {
public:
    explicit FreeDepCmd(const std::vector<std::string>& paths,
                        bool trigger = true,
                        bool all     = false, // day, date, time, today, trigger, cron
                        bool date    = false,
                        bool time    = false // includes time, day, date, today, cron
                        )
        : paths_(paths),
          trigger_(trigger),
          all_(all),
          date_(date),
          time_(time) {}

    explicit FreeDepCmd(const std::string& path,
                        bool trigger = true,
                        bool all     = false, // day, date, time, today, trigger, cron
                        bool date    = false,
                        bool time    = false // includes time, day, date, today, cron
                        )
        : paths_(std::vector<std::string>(1, path)),
          trigger_(trigger),
          all_(all),
          date_(date),
          time_(time) {}

    FreeDepCmd() = default;

    // Uses by equals only
    const std::vector<std::string>& paths() const { return paths_; }
    bool trigger() const { return trigger_; }
    bool all() const { return all_; }
    bool date() const { return date_; }
    bool time() const { return time_; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after doHandleRequest

private:
    std::vector<std::string> paths_;
    bool trigger_{true};
    bool all_{false};
    bool date_{false};
    bool time_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(paths_),
           CEREAL_NVP(trigger_),
           CEREAL_NVP(all_),
           CEREAL_NVP(date_),
           CEREAL_NVP(time_));
    }
};

std::ostream& operator<<(std::ostream& os, const FreeDepCmd&);

CEREAL_FORCE_DYNAMIC_INIT(FreeDepCmd)

#endif /* ecflow_base_cts_user_FreeDepCmd_HPP */
