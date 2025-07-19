/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_user_ShowCmd_HPP
#define ecflow_base_cts_user_ShowCmd_HPP

#include "ecflow/base/cts/user/UserCmd.hpp"

// Does Nothing in the server, however allows client code to display the
// returned Defs in different showStyles
// This class has no need for persistence, i.e client side only
class ShowCmd final : public UserCmd {
public:
    explicit ShowCmd(PrintStyle::Type_t s = PrintStyle::DEFS) : style_(s) {}

    // returns the showStyle
    bool show_cmd() const override { return true; }
    PrintStyle::Type_t show_style() const override { return style_; }

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

    // The Show Cmd is processed on the client side,
    // Likewise the doHandleRequest does nothing,
    STC_Cmd_ptr doHandleRequest(AbstractServer*) const override;

    PrintStyle::Type_t style_;

    // Persistence is still required since show command can be *USED* in a *GROUP* command
    // However its ONLY used on the client side, hence no need to serialise data members
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar(cereal::base_class<UserCmd>(this));
    }
};

std::ostream& operator<<(std::ostream& os, const ShowCmd&);

CEREAL_FORCE_DYNAMIC_INIT(ShowCmd)

#endif /* ecflow_base_cts_user_ShowCmd_HPP */
