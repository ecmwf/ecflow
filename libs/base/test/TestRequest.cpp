/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "MyDefsFixture.hpp"
#include "TestHelper.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/task/AbortCmd.hpp"
#include "ecflow/base/cts/task/CompleteCmd.hpp"
#include "ecflow/base/cts/task/CtsWaitCmd.hpp"
#include "ecflow/base/cts/task/EventCmd.hpp"
#include "ecflow/base/cts/task/InitCmd.hpp"
#include "ecflow/base/cts/task/LabelCmd.hpp"
#include "ecflow/base/cts/task/MeterCmd.hpp"
#include "ecflow/base/cts/task/QueueCmd.hpp"
#include "ecflow/base/cts/user/AlterCmd.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/base/cts/user/CSyncCmd.hpp"
#include "ecflow/base/cts/user/CheckPtCmd.hpp"
#include "ecflow/base/cts/user/ClientHandleCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/cts/user/CtsNodeCmd.hpp"
#include "ecflow/base/cts/user/EditScriptCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/FreeDepCmd.hpp"
#include "ecflow/base/cts/user/GroupCTSCmd.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/LogCmd.hpp"
#include "ecflow/base/cts/user/LogMessageCmd.hpp"
#include "ecflow/base/cts/user/OrderNodeCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/base/cts/user/PlugCmd.hpp"
#include "ecflow/base/cts/user/QueryCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"
#include "ecflow/base/cts/user/RunNodeCmd.hpp"
#include "ecflow/base/cts/user/ServerVersionCmd.hpp"
#include "ecflow/base/cts/user/ShowCmd.hpp"
#include "ecflow/base/cts/user/ZombieCmd.hpp"
#include "ecflow/base/stc/BlockClientZombieCmd.hpp"
#include "ecflow/base/stc/DefsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/SNewsCmd.hpp"
#include "ecflow/base/stc/SNodeCmd.hpp"
#include "ecflow/base/stc/SServerLoadCmd.hpp"
#include "ecflow/base/stc/SStringCmd.hpp"
#include "ecflow/base/stc/SSyncCmd.hpp"
#include "ecflow/base/stc/StcCmd.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_FIXTURE_TEST_SUITE(U_Base, MyDefsFixture)

BOOST_AUTO_TEST_SUITE(T_Request)

// Can't delete the fixture defs, hence use a NULL deleter to avoid freeing memory twice.
// Required since DefsCmd and LoadDefsCmd requires a shared_ptr.
struct null_deleter
{
    void operator()(void const*) const {}
};

static void
populateCmdVec(std::vector<Cmd_ptr>& cmd_vec, std::vector<STC_Cmd_ptr>& stc_cmd_vec, MockServer* mock_server) {
    std::vector<std::string> suite_names;
    suite_names.emplace_back("suiteName");
    MyDefsFixture fixture;
    defs_ptr client_defs = fixture.create_defs();
    client_defs->clear_externs(); // server defs should not have externs

    // Client --> Server commands
    // make sure begin cmd is first. As this will init calendar. + other commands rely on it
    // i.e RequeueNodeCmd assumes calendar has been initialized
    cmd_vec.push_back(Cmd_ptr(new ShowCmd()));
    cmd_vec.push_back(Cmd_ptr(new ServerVersionCmd()));
    cmd_vec.push_back(Cmd_ptr(new ReplaceNodeCmd("suiteName", false, client_defs, true)));
    cmd_vec.push_back(Cmd_ptr(new LoadDefsCmd(client_defs, true /*force*/)));
    cmd_vec.push_back(
        Cmd_ptr(new BeginCmd("suiteName"))); // after loading new defs, must call begin, for downstream cmds
    cmd_vec.push_back(
        Cmd_ptr(new BeginCmd("EmptySuite"))); // after loading new defs, must call begin for downstream cmds
    cmd_vec.push_back(Cmd_ptr(new LogMessageCmd("LogMessageCmd")));
    cmd_vec.push_back(Cmd_ptr(new LogCmd(LogCmd::CLEAR))); // server replies back OK/Error Cmd
    cmd_vec.push_back(Cmd_ptr(new LogCmd(LogCmd::GET)));   // server replies back OK/Error | SStringCmd
    cmd_vec.push_back(Cmd_ptr(new LogCmd(LogCmd::PATH)));  // server replies back Error |SStringCmd
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::RESTORE_DEFS_FROM_CHECKPT)));
    cmd_vec.push_back(Cmd_ptr(new CheckPtCmd()));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::PING)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::DEBUG_SERVER_ON)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::DEBUG_SERVER_OFF)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::SHUTDOWN_SERVER)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::HALT_SERVER)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::TERMINATE_SERVER)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_WHITE_LIST_FILE)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_PASSWD_FILE)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_CUSTOM_PASSWD_FILE)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::FORCE_DEP_EVAL)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::STATS)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::STATS_SERVER)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::STATS_RESET)));
    cmd_vec.push_back(Cmd_ptr(new CtsCmd(CtsCmd::SUITES)));
    cmd_vec.push_back(Cmd_ptr(new CSyncCmd(CSyncCmd::NEWS, 0, 0, 0)));
    cmd_vec.push_back(Cmd_ptr(new CSyncCmd(CSyncCmd::SYNC, 0, 0, 0)));
    cmd_vec.push_back(Cmd_ptr(new CSyncCmd(CSyncCmd::SYNC_CLOCK, 0, 0, 0)));
    cmd_vec.push_back(Cmd_ptr(new CSyncCmd(0))); // SYNC_FULL
    cmd_vec.push_back(Cmd_ptr(new RequeueNodeCmd("/suiteName", RequeueNodeCmd::NO_OPTION)));
    cmd_vec.push_back(Cmd_ptr(new OrderNodeCmd("/suiteName", NOrder::ALPHA)));
    cmd_vec.push_back(Cmd_ptr(new RunNodeCmd("/suiteName", true /* force for test */, true /* for test */)));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::SUSPEND, "EmptySuite")));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::RESUME, "EmptySuite")));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::KILL, "EmptySuite")));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::STATUS, "EmptySuite")));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::CHECK, "/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::CHECK, "")));                  // check the full defs
    cmd_vec.push_back(Cmd_ptr(new PathsCmd(PathsCmd::EDIT_HISTORY, "/suiteName"))); // check the full defs
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::WHY, "/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET, "/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET_STATE, "/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::MIGRATE, "/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::CHECK_JOB_GEN_ONLY, "EmptySuite"))); // will *reset* begin
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET, "")));                          // return the full defs
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET_STATE, "")));
    cmd_vec.push_back(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::MIGRATE, "")));
    cmd_vec.push_back(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::FOB,
                                            std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                            Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                            Submittable::DUMMY_JOBS_PASSWORD())));
    cmd_vec.push_back(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::FAIL,
                                            std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                            Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                            Submittable::DUMMY_JOBS_PASSWORD())));
    cmd_vec.push_back(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::ADOPT,
                                            std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                            Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                            Submittable::DUMMY_JOBS_PASSWORD())));
    cmd_vec.push_back(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::REMOVE,
                                            std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                            Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                            Submittable::DUMMY_JOBS_PASSWORD())));
    cmd_vec.push_back(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::KILL,
                                            std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                            Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                            Submittable::DUMMY_JOBS_PASSWORD())));
    cmd_vec.push_back(Cmd_ptr(new ClientHandleCmd(0, suite_names, true)));                    // register
    cmd_vec.push_back(Cmd_ptr(new ClientHandleCmd(1, suite_names, ClientHandleCmd::ADD)));    // add
    cmd_vec.push_back(Cmd_ptr(new ClientHandleCmd(1, suite_names, ClientHandleCmd::REMOVE))); // remove
    cmd_vec.push_back(Cmd_ptr(new ClientHandleCmd(1, true)));                                 // auto_add new suites
    cmd_vec.push_back(Cmd_ptr(new ClientHandleCmd(1)));                                       // de-register/drop

    cmd_vec.push_back(Cmd_ptr(new CtsCmd(
        CtsCmd::RESTART_SERVER))); // make sure server is restarted or child cmds fail, as server will not allow comms

    cmd_vec.push_back(Cmd_ptr(new InitCmd("suiteName/familyName/taskName",
                                          Submittable::DUMMY_JOBS_PASSWORD(),
                                          Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                          1)));
    cmd_vec.push_back(Cmd_ptr(new EventCmd("suiteName/familyName/taskName",
                                           Submittable::DUMMY_JOBS_PASSWORD(),
                                           Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                           1,
                                           "eventName")));
    cmd_vec.push_back(Cmd_ptr(new MeterCmd("suiteName/familyName/heir_familyName/taskName",
                                           Submittable::DUMMY_JOBS_PASSWORD(),
                                           Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                           1,
                                           "myMeter",
                                           100)));
    cmd_vec.push_back(Cmd_ptr(new CompleteCmd("suiteName/familyName/taskName",
                                              Submittable::DUMMY_JOBS_PASSWORD(),
                                              Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                              1)));
    cmd_vec.push_back(Cmd_ptr(new AbortCmd("suiteName/familyName/taskName",
                                           Submittable::DUMMY_JOBS_PASSWORD(),
                                           Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                           1)));
    cmd_vec.push_back(Cmd_ptr(new CtsWaitCmd("suiteName/familyName/taskName",
                                             Submittable::DUMMY_JOBS_PASSWORD(),
                                             Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                             1,
                                             "1 eq 1")));
    cmd_vec.push_back(Cmd_ptr(new LabelCmd("suiteName/familyName/taskName",
                                           Submittable::DUMMY_JOBS_PASSWORD(),
                                           Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                           1,
                                           "labelName",
                                           "label value")));
    cmd_vec.push_back(Cmd_ptr(new QueueCmd("/suiteName/familyName/taskName",
                                           Submittable::DUMMY_JOBS_PASSWORD(),
                                           Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                           1,
                                           "queue1",
                                           "active",
                                           "",
                                           "/suiteName")));

    std::vector<Variable> to_add{Variable("name", "value"), Variable("name2", "value")};
    std::vector<std::string> to_del{"name", "name2"};
    cmd_vec.push_back(Cmd_ptr(new InitCmd("suiteName/familyName/taskName",
                                          Submittable::DUMMY_JOBS_PASSWORD(),
                                          Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                          1,
                                          to_add)));
    cmd_vec.push_back(Cmd_ptr(new CompleteCmd("suiteName/familyName/taskName",
                                              Submittable::DUMMY_JOBS_PASSWORD(),
                                              Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                              1,
                                              to_del)));

    cmd_vec.push_back(Cmd_ptr(new ForceCmd("/suiteName", "complete", true, true)));
    cmd_vec.push_back(Cmd_ptr(new FreeDepCmd("/suiteName")));
    cmd_vec.push_back(Cmd_ptr(new CFileCmd("/suiteName", CFileCmd::ECF, 10)));
    cmd_vec.push_back(Cmd_ptr(new CFileCmd("/suiteName", CFileCmd::JOB, 100)));
    cmd_vec.push_back(Cmd_ptr(new CFileCmd("/suiteName", CFileCmd::JOBOUT, 100)));
    cmd_vec.push_back(Cmd_ptr(new CFileCmd("/suiteName", CFileCmd::MANUAL, 100)));
    cmd_vec.push_back(Cmd_ptr(new EditScriptCmd()));
    cmd_vec.push_back(Cmd_ptr(new AlterCmd("/suiteName/t1", AlterCmd::ADD_DATE, "12.*.*")));
    cmd_vec.push_back(Cmd_ptr(new AlterCmd("/suiteName/t1", AlterCmd::ADD_DAY, "sunday")));
    cmd_vec.push_back(Cmd_ptr(new AlterCmd("/suiteName/t1", AlterCmd::ADD_TIME, "+12:00")));
    cmd_vec.push_back(Cmd_ptr(new AlterCmd("/suiteName/t1", AlterCmd::ADD_TIME, "+10:00 20:00 00:30")));
    cmd_vec.push_back(Cmd_ptr(new AlterCmd("/suiteName/t1", AlterCmd::ADD_TODAY, "10:00 20:00 00:30")));
    cmd_vec.push_back(Cmd_ptr(new PlugCmd()));
    cmd_vec.push_back(Cmd_ptr(new QueryCmd("event", "/suiteName/t1", "event", "")));

    std::shared_ptr<GroupCTSCmd> theGroupCmd = std::make_shared<GroupCTSCmd>();
    theGroupCmd->addChild(Cmd_ptr(new ServerVersionCmd()));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::PING)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::SHUTDOWN_SERVER)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::HALT_SERVER)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::TERMINATE_SERVER)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_WHITE_LIST_FILE)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_PASSWD_FILE)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::RELOAD_CUSTOM_PASSWD_FILE)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::SERVER_LOAD)));
    theGroupCmd->addChild(Cmd_ptr(new PathsCmd(PathsCmd::SUSPEND, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new PathsCmd(PathsCmd::RESUME, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new PathsCmd(PathsCmd::KILL, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET_STATE, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::MIGRATE, "EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new ShowCmd()));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::FOB,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::FAIL,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::ADOPT,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::REMOVE,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::KILL,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new ZombieCmd(ecf::ZombieCtrlAction::BLOCK,
                                                std::vector<std::string>(1, "/suiteName/familyName/taskName"),
                                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                Submittable::DUMMY_JOBS_PASSWORD())));
    theGroupCmd->addChild(Cmd_ptr(new RequeueNodeCmd("/suiteName", RequeueNodeCmd::NO_OPTION)));
    theGroupCmd->addChild(Cmd_ptr(new OrderNodeCmd("/suiteName", NOrder::ALPHA)));
    theGroupCmd->addChild(Cmd_ptr(new RunNodeCmd("/suiteName", true /*force for test*/, true /* for test */)));
    theGroupCmd->addChild(Cmd_ptr(new CtsCmd(CtsCmd::RESTART_SERVER)));
    theGroupCmd->addChild(Cmd_ptr(new InitCmd("suiteName/familyName/taskName",
                                              Submittable::DUMMY_JOBS_PASSWORD(),
                                              Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                              1)));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::CHECK_JOB_GEN_ONLY, "EmptySuite"))); // will *reset* begin
    theGroupCmd->addChild(Cmd_ptr(new EventCmd("suiteName/familyName/taskName",
                                               Submittable::DUMMY_JOBS_PASSWORD(),
                                               Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                               1,
                                               "eventName")));
    theGroupCmd->addChild(Cmd_ptr(new MeterCmd("suiteName/familyName/heir_familyName/taskName",
                                               Submittable::DUMMY_JOBS_PASSWORD(),
                                               Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                               1,
                                               "myMeter",
                                               100)));
    theGroupCmd->addChild(Cmd_ptr(new CompleteCmd("suiteName/familyName/taskName",
                                                  Submittable::DUMMY_JOBS_PASSWORD(),
                                                  Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                  1)));
    theGroupCmd->addChild(Cmd_ptr(new AbortCmd("suiteName/familyName/taskName",
                                               Submittable::DUMMY_JOBS_PASSWORD(),
                                               Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                               1)));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET)));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::GET_STATE)));
    theGroupCmd->addChild(Cmd_ptr(new CtsNodeCmd(CtsNodeCmd::MIGRATE)));
    theGroupCmd->addChild(
        Cmd_ptr(new CtsCmd(CtsCmd::FORCE_DEP_EVAL))); // will force deletion of any Node which has autocomplete
    theGroupCmd->addChild(Cmd_ptr(new LoadDefsCmd(client_defs, true /*force*/)));
    theGroupCmd->addChild(Cmd_ptr(new BeginCmd("suiteName")));
    theGroupCmd->addChild(Cmd_ptr(new BeginCmd("EmptySuite")));
    theGroupCmd->addChild(Cmd_ptr(new LogCmd(LogCmd::GET)));
    theGroupCmd->addChild(Cmd_ptr(new LogCmd(LogCmd::CLEAR)));
    theGroupCmd->addChild(Cmd_ptr(new LogMessageCmd("LogMessageCmd")));
    theGroupCmd->addChild(Cmd_ptr(new ForceCmd("/suiteName", "complete", true, true)));
    theGroupCmd->addChild(Cmd_ptr(new FreeDepCmd("/suiteName", false, true)));

    BOOST_CHECK_MESSAGE(theGroupCmd->isWrite(), "Expected isWrite() to return true");
    BOOST_CHECK_MESSAGE(theGroupCmd->get_cmd(), "Expected get_cmd() to return true");
    BOOST_CHECK_MESSAGE(theGroupCmd->task_cmd(), "Expected task_cmd() to return true");
    BOOST_CHECK_MESSAGE(theGroupCmd->terminate_cmd(), "Expected terminate_cmd() to return true");
    BOOST_CHECK_MESSAGE(theGroupCmd->group_cmd(), "Expected group_cmd() to return true");
    cmd_vec.push_back(theGroupCmd);

    // Server --> Client commands
    stc_cmd_vec.push_back(STC_Cmd_ptr(new ErrorCmd("The error")));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new StcCmd(StcCmd::OK)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new StcCmd(StcCmd::BLOCK_CLIENT_SERVER_HALTED)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new StcCmd(StcCmd::BLOCK_CLIENT_ON_HOME_SERVER)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new BlockClientZombieCmd(ecf::Child::ECF)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new SStringCmd("Dummy contents")));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new SServerLoadCmd("/path/to/log_file")));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new SSyncCmd(0, 0, 0, mock_server)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new SNewsCmd(0, 0, 0, mock_server)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new DefsCmd(mock_server)));
    stc_cmd_vec.push_back(STC_Cmd_ptr(new SNodeCmd(mock_server, node_ptr())));

    std::shared_ptr<GroupSTCCmd> theSTCGroupCmd = std::make_shared<GroupSTCCmd>();
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new ErrorCmd()));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new StcCmd(StcCmd::OK)));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new StcCmd(StcCmd::BLOCK_CLIENT_SERVER_HALTED)));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new StcCmd(StcCmd::BLOCK_CLIENT_ON_HOME_SERVER)));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new BlockClientZombieCmd(ecf::Child::ECF)));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new SStringCmd()));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new SServerLoadCmd()));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new DefsCmd(mock_server)));
    theSTCGroupCmd->addChild(STC_Cmd_ptr(new SNodeCmd(mock_server, node_ptr())));
    stc_cmd_vec.push_back(theSTCGroupCmd);
}

static void test_persistence(const Defs& theFixtureDefs) {
    auto* fixtureDefs = const_cast<Defs*>(&theFixtureDefs);
    fixtureDefs->clear_externs();       // server does not have externs
    MockServer mockServer(fixtureDefs); // creates shared ptr with a NULL deleter

    std::vector<Cmd_ptr> cmd_vec;
    std::vector<STC_Cmd_ptr> stc_cmd_vec;
    populateCmdVec(cmd_vec, stc_cmd_vec, &mockServer);

    int getRequest       = 0;
    int terminateRequest = 0;
    int groupRequest     = 0;
    int i                = 0;
    for (const Cmd_ptr& theCmd : cmd_vec) {

        // std::cout << "TheCmd "; theCmd->print(std::cout); std::cout << "\n";
        if (theCmd->connect_to_different_servers()) {
            BOOST_CHECK_MESSAGE(theCmd->task_cmd(),
                                "Currently only tasks commands, are allowed to connect to different servers");
        }

        ClientToServerRequest cmd_request;
        cmd_request.set_cmd(theCmd);
        {
            if (theCmd.get()->handleRequestIsTestable()) {
                // test handleRequest while were at it.
                // Avoid TERMINATE_SERVER cmd as this will prematurely cause an exit, wont appear as an error
                // cerr << "cmd_request = " << cmd_request << "\n";
                try {
                    ++i;
                    STC_Cmd_ptr ok_or_error_cmd = cmd_request.handleRequest(&mockServer);
                    if (ok_or_error_cmd) {
                        // Commands like ErrorCmd, OkCmd don't return a cmd_ptr from handleRequest
                        // those that do, check OkCmd returned, else if ErrorCmd show the error
                        BOOST_CHECK_MESSAGE(ok_or_error_cmd->ok(),
                                            "Request '" << cmd_request << "' returned " << ok_or_error_cmd->error());
                    }
                }
                catch (std::exception& e) {
                    BOOST_CHECK_MESSAGE(false, "Unexpected exception : " << e.what() << " : " << cmd_request);
                }
            }
        }

        // cout << " Saving = " << cmd_request << "\n";
        std::string saved_request;
        {
            BOOST_REQUIRE_NO_THROW(ecf::save("request.txt", cmd_request)); // save as filename
            ecf::save_as_string(saved_request, cmd_request);               // save as string
            // cout << saved_request << "\n";                                // Take a peek at json output
        }

        // cout << " Restoring = " << cmd_request << "\n";
        {
            ClientToServerRequest restoredRequest;
            BOOST_REQUIRE_NO_THROW(ecf::restore("request.txt", restoredRequest));
            BOOST_REQUIRE_MESSAGE(restoredRequest == cmd_request,
                                  "restoredRequest " << restoredRequest << " cmd_request " << cmd_request);

            if (restoredRequest.getRequest()) {
                getRequest++;
            }
            if (restoredRequest.terminateRequest()) {
                terminateRequest++;
            }
            if (restoredRequest.groupRequest()) {
                groupRequest++;
            }
        }
        {
            ClientToServerRequest restoredRequest;
            ecf::restore_from_string(saved_request, restoredRequest);
            BOOST_REQUIRE_MESSAGE(restoredRequest == cmd_request,
                                  "restoredRequest " << restoredRequest << " cmd_request " << cmd_request);
        }

        fs::remove("request.txt");
    }

    BOOST_CHECK_MESSAGE(getRequest == 3, " expected 3 get Request  but found " << getRequest);
    BOOST_CHECK_MESSAGE(terminateRequest == 2, " expected 2 terminate Request but found " << terminateRequest);
    BOOST_CHECK_MESSAGE(groupRequest == 1, " expected 1 group Request but found " << groupRequest);

    for (const STC_Cmd_ptr& theCmd : stc_cmd_vec) {
        //		std::cout << "TheCmd "; theCmd->print(std::cout); std::cout << "\n";

        const ServerToClientResponse cmd_request(theCmd); // MUST be const to avoid AIX compiler warning
        BOOST_REQUIRE_NO_THROW(ecf::save("request.txt", cmd_request));

        ServerToClientResponse restoredRequest;
        BOOST_REQUIRE_NO_THROW(ecf::restore("request.txt", restoredRequest));
        BOOST_REQUIRE_MESSAGE(restoredRequest == cmd_request,
                              "restoredRequest " << restoredRequest << " cmd_request " << cmd_request);
        fs::remove("request.txt");
    }
}

BOOST_AUTO_TEST_CASE(test_all_request_persistence_text) {
    ECF_NAME_THIS_TEST();

    TestLog test_log("test_all_request_persistence_text.log"); // will create log file, and destroy log and remove file
                                                               // at end of scope

    test_persistence(fixtureDefsFile());
}

BOOST_AUTO_TEST_CASE(test_request_authenticate) {
    ECF_NAME_THIS_TEST();

    TestLog test_log(
        "test_request_authenticate.log"); // will create log file, and destroy log and remove file at end of scope

    // the path "suiteName0/familyName0/taskName0" must exist in the defsfile_ fixture
    Cmd_ptr cmd_ptr(new InitCmd("suiteName/familyName/taskName",
                                Submittable::DUMMY_JOBS_PASSWORD(),
                                Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                1));

    // The invokeRequest will return check if the path and password exist in the Node tree
    TestHelper::invokeRequest(&defsfile_, cmd_ptr);

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
