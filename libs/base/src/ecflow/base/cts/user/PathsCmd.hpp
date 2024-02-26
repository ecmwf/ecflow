/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_PathsCmd_HPP
#define ecflow_base_cts_user_PathsCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// DELETE If paths_ empty will delete all suites (beware) else will delete the chosen nodes.
class PathsCmd final : public UserCmd {
public:
    enum Api { NO_CMD, SUSPEND, RESUME, KILL, STATUS, CHECK, EDIT_HISTORY, ARCHIVE, RESTORE };

    PathsCmd(Api api, const std::vector<std::string>& paths, bool force = false)
        : api_(api),
          paths_(paths),
          force_(force) {}
    PathsCmd(Api api, const std::string& absNodePath, bool force = false);
    explicit PathsCmd(Api api) : api_(api) { assert(api != NO_CMD); }
    PathsCmd() = default;

    Api api() const { return api_; }
    const std::vector<std::string>& paths() const { return paths_; }
    bool force() const { return force_; }

    void print(std::string&) const override;
    std::string print_short() const override;
    void print_only(std::string&) const override;
    void print(std::string& os, const std::string& path) const override;

    bool equals(ClientToServerCmd*) const override;
    bool isWrite() const override;

    const char* theArg() const override;
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;
    void cleanup() override { std::vector<std::string>().swap(paths_); } /// run in the server, after handlerequest

    void my_print(std::string& os, const std::vector<std::string>& paths) const;
    void my_print_only(std::string& os, const std::vector<std::string>& paths) const;

private:
    Api api_{NO_CMD};
    std::vector<std::string> paths_;
    bool force_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(api_), CEREAL_NVP(paths_), CEREAL_NVP(force_));
    }
};

std::ostream& operator<<(std::ostream& os, const PathsCmd&);

CEREAL_FORCE_DYNAMIC_INIT(PathsCmd)

#endif /* ecflow_base_cts_user_PathsCmd_HPP */
