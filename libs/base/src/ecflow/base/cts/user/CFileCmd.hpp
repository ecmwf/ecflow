/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_CFileCmd_HPP
#define ecflow_base_cts_user_CFileCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

//================================================================================
// Paired with SStringCmd
// Client---(CFileCmd)---->Server-----(SStringCmd)--->client:
//================================================================================
class CFileCmd final : public UserCmd {
public:
    enum File_t { ECF, JOB, JOBOUT, MANUAL, KILL, STAT };
    CFileCmd(const std::string& pathToNode, File_t file, size_t max_lines)
        : file_(file),
          pathToNode_(pathToNode),
          max_lines_(max_lines) {}
    CFileCmd(const std::string& pathToNode, const std::string& file_type, const std::string& max_lines);
    CFileCmd() = default;

    // Uses by equals only
    const std::string& pathToNode() const { return pathToNode_; }
    File_t fileType() const { return file_; }
    size_t max_lines() const { return max_lines_; }

    static std::vector<CFileCmd::File_t> fileTypesVec();
    static std::string toString(File_t);

    bool handleRequestIsTestable() const override { return false; }
    void print(std::string&) const override;
    void print_only(std::string&) const override;
    bool equals(ClientToServerCmd*) const override;

    [[nodiscard]] ecf::authentication_t authenticate(AbstractServer& server) const override;
    [[nodiscard]] ecf::authorisation_t authorise(AbstractServer& server) const override;

    const char* theArg() const override { return arg(); }
    void addOption(boost::program_options::options_description& desc) const override;
    void create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const override;

private:
    static const char* arg();  // used for argument parsing
    static const char* desc(); // The description of the argument as provided to user

    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;
    // bool authenticate(AbstractServer*, STC_Cmd_ptr&) const override;

    File_t file_{ECF};
    std::string pathToNode_;
    size_t max_lines_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this), CEREAL_NVP(file_), CEREAL_NVP(pathToNode_), CEREAL_NVP(max_lines_));
    }
};

std::ostream& operator<<(std::ostream& os, const CFileCmd&);

CEREAL_FORCE_DYNAMIC_INIT(CFileCmd)

#endif /* ecflow_base_cts_user_CFileCmd_HPP */
