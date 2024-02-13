/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_MoveCmd_HPP
#define ecflow_base_cts_user_MoveCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

class MoveCmd final : public UserCmd {
public:
    MoveCmd(const std::pair<std::string, std::string>& host_port, Node* src, const std::string& dest);
    MoveCmd();
    ~MoveCmd() override;

    Node* source() const;
    const std::string& src_node() const { return src_node_; }
    const std::string& dest() const { return dest_; }

    bool handleRequestIsTestable() const override { return false; }
    bool isWrite() const override { return true; }

    void print(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    bool check_source() const;

private:
    std::string src_node_;
    std::string src_host_;
    std::string src_port_;
    std::string src_path_;
    std::string dest_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this),
           CEREAL_NVP(src_node_),
           CEREAL_NVP(src_host_),
           CEREAL_NVP(src_port_),
           CEREAL_NVP(src_path_),
           CEREAL_NVP(dest_));
    }
};

std::ostream& operator<<(std::ostream& os, const MoveCmd&);

CEREAL_FORCE_DYNAMIC_INIT(MoveCmd)

#endif /* ecflow_base_cts_user_MoveCmd_HPP */
