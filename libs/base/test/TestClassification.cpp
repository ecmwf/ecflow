// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include <boost/core/demangle.hpp>
#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/RequestContext.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/cts/CtsCmdRegistry.hpp"
#include "ecflow/base/stc/BlockClientZombieCmd.hpp"
#include "ecflow/base/stc/DefsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/SClientHandleCmd.hpp"
#include "ecflow/base/stc/SClientHandleSuitesCmd.hpp"
#include "ecflow/base/stc/SNewsCmd.hpp"
#include "ecflow/base/stc/SNodeCmd.hpp"
#include "ecflow/base/stc/SServerLoadCmd.hpp"
#include "ecflow/base/stc/SStatsCmd.hpp"
#include "ecflow/base/stc/SStringCmd.hpp"
#include "ecflow/base/stc/SStringVecCmd.hpp"
#include "ecflow/base/stc/SSuitesCmd.hpp"
#include "ecflow/base/stc/SSyncCmd.hpp"
#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/base/stc/ZombieGetCmd.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::ReplyFiltering;

///
/// The declared classification of commands and replies (R5.11, R7.7): every reply type declares how it is filtered,
/// and every user command has an authorisation of its own, which denies a requester without roles.
///

namespace {

struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_classification.log"};
};

template <typename T>
std::pair<std::string, ReplyFiltering> declared() {
    const T reply;
    return {boost::core::demangle(typeid(T).name()), reply.filtering()};
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_Classification, LogFixture)

///
/// @brief Verifies the filtering declared by every reply type: none (no node data, or data accepted unfiltered by
///        D5.20, D5.21, D5.26, D5.28), node content (definitions or nodes), or listing (names of nodes).
///
/// A reply type without a declaration does not build, since ServerToClientCmd::filtering() is pure.
///
BOOST_AUTO_TEST_CASE(test_reply_types_declare_their_filtering) {
    ECF_NAME_THIS_TEST();

    const std::map<std::string, ReplyFiltering> expected{
        {"BlockClientZombieCmd", ReplyFiltering::None},
        {"DefsCmd", ReplyFiltering::NodeContent},
        {"ErrorCmd", ReplyFiltering::None},
        {"GroupSTCCmd", ReplyFiltering::None},
        {"SClientHandleCmd", ReplyFiltering::None},
        {"SClientHandleSuitesCmd", ReplyFiltering::Listing},
        {"SNewsCmd", ReplyFiltering::None},
        {"SNodeCmd", ReplyFiltering::NodeContent},
        {"SServerLoadCmd", ReplyFiltering::None},
        {"SStatsCmd", ReplyFiltering::Listing},
        {"SStringCmd", ReplyFiltering::None},
        {"SStringVecCmd", ReplyFiltering::None},
        {"SSuitesCmd", ReplyFiltering::Listing},
        {"SSyncCmd", ReplyFiltering::NodeContent},
        {"StcCmd", ReplyFiltering::None},
        {"ZombieGetCmd", ReplyFiltering::Listing},
    };
    const std::vector<std::pair<std::string, ReplyFiltering>> actual{
        declared<BlockClientZombieCmd>(),
        declared<DefsCmd>(),
        declared<ErrorCmd>(),
        declared<GroupSTCCmd>(),
        declared<SClientHandleCmd>(),
        declared<SClientHandleSuitesCmd>(),
        declared<SNewsCmd>(),
        declared<SNodeCmd>(),
        declared<SServerLoadCmd>(),
        declared<SStatsCmd>(),
        declared<SStringCmd>(),
        declared<SStringVecCmd>(),
        declared<SSuitesCmd>(),
        declared<SSyncCmd>(),
        declared<StcCmd>(),
        declared<ZombieGetCmd>(),
    };
    BOOST_CHECK_EQUAL(actual.size(), expected.size());
    for (const auto& [name, filtering] : actual) {
        BOOST_TEST_CONTEXT(name) {
            const auto found = expected.find(name);
            BOOST_REQUIRE(found != expected.end());
            BOOST_CHECK(filtering == found->second);
        }
    }
}

///
/// @brief Verifies that every user command known to the client has an authorisation of its own, which denies a
///        requester the server attribute does not admit; only the requests exempted by the decisions are accepted.
///
BOOST_AUTO_TEST_CASE(test_user_commands_have_an_authorisation) {
    ECF_NAME_THIS_TEST();

    auto defs = Defs::create();
    defs->add_suite("s");
    defs->server_state().set_permissions(ecf::Permissions::parse_cli("admin:rwxlops"));
    MockServer server(defs);

    CtsCmdRegistry registry;
    BOOST_REQUIRE(!registry.commands().empty());
    for (const auto& command : registry.commands()) {
        auto* cts = dynamic_cast<ClientToServerCmd*>(command.get());
        BOOST_REQUIRE(cts);
        std::string option = boost::core::demangle(typeid(*cts).name());
        try {
            option = cts->theArg();
        }
        catch (const std::exception&) {
            // the argument of some prototypes depends on their parameters; the type names them instead
        }
        if (cts->task_cmd()) {
            continue; // task requests are not authorised (D5.1)
        }
        BOOST_TEST_CONTEXT(option) {
            try {
                cts->set_identity(ecf::Identity::make_user("stranger", ""));
                const ecf::RequestContext context(cts->identity(), server);
                const auto result = cts->authorise(context);
                // Exempted: --ping (D10.1), and --group, whose sub-commands are authorised as they run (D5.27)
                const bool exempted = option == "ping" || option == "group";
                BOOST_CHECK_MESSAGE(result.ok() == exempted, option << (result.ok() ? " accepted" : " refused"));
                if (!result.ok()) {
                    BOOST_CHECK_MESSAGE(result.reason().find("unknown command") == std::string::npos, result.reason());
                }
            }
            catch (const std::exception& e) {
                BOOST_ERROR(option << ": " << e.what());
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
