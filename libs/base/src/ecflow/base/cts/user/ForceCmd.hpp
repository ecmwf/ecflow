/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ForceCmd_HPP
#define ecflow_base_cts_user_ForceCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Set the state on the affected node ONLY.
// If recursive is used set the state, on node and  _ALL_ nodes _beneath
// setRepeatToLastValue set, only make sense when used with recursive.
// stateOrEvent, string is one of:
// < unknown | suspended | complete | queued | submitted | active | aborted | clear | set >
class ForceCmd final : public UserCmd {
public:
    ForceCmd(const std::vector<std::string>& paths,
             const std::string& stateOrEvent,
             bool recursive,
             bool setRepeatToLastValue)
        : paths_(paths),
          stateOrEvent_(stateOrEvent),
          recursive_(recursive),
          setRepeatToLastValue_(setRepeatToLastValue) {}
    ForceCmd(const std::string& path, const std::string& stateOrEvent, bool recursive, bool setRepeatToLastValue)
        : paths_(std::vector<std::string>(1, path)),
          stateOrEvent_(stateOrEvent),
          recursive_(recursive),
          setRepeatToLastValue_(setRepeatToLastValue) {}
    ForceCmd() = default;

    // Uses by equals only
    const std::vector<std::string> paths() const { return paths_; }
    const std::string& stateOrEvent() const { return stateOrEvent_; }
    bool recursive() const { return recursive_; }
    bool setRepeatToLastValue() const { return setRepeatToLastValue_; }

    bool isWrite() const override { return true; }
    void print(std::string&) const override;
    std::string print_short() const override;
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

    void my_print(std::string& os, const std::vector<std::string>& paths) const;
    void my_print_only(std::string& os, const std::vector<std::string>& paths) const;

private:
    std::vector<std::string> paths_;
    std::string stateOrEvent_;
    bool recursive_{false};
    bool setRepeatToLastValue_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(paths_),
           CEREAL_NVP(stateOrEvent_),
           CEREAL_NVP(recursive_),
           CEREAL_NVP(setRepeatToLastValue_));
    }
};

std::ostream& operator<<(std::ostream& os, const ForceCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ForceCmd)

#endif /* ecflow_base_cts_user_ForceCmd_HPP */
