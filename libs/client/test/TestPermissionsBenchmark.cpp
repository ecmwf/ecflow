// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "PermissionsBenchmark.hpp"
#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// The benchmark of the Permissions attributes (R12.7): a reference tree, served to several identities, each with
/// its readable set. It measures, for every identity, the latency of full and incremental synchronisations and of
/// news requests, and, for the server, its CPU time, its memory and the size and save time of its checkpoint.
///
/// The parameters are read from the environment, with defaults giving the reference configuration:
///
/// - ECF_TEST_PERMISSIONS_SUITES (50), ECF_TEST_PERMISSIONS_FAMILIES (20), ECF_TEST_PERMISSIONS_TASKS (10): the tree;
/// - ECF_TEST_PERMISSIONS_IDENTITIES (20): the identities, one client each;
/// - ECF_TEST_PERMISSIONS_READABLE_SETS (5): the distinct readable sets the identities are spread over;
/// - ECF_TEST_PERMISSIONS_ITERATIONS (10): the rounds of changes followed by incremental synchronisations;
/// - ECF_TEST_PERMISSIONS_REPORT (permissions_benchmark.json): the file the results are written to.
///
/// Every metric is printed, and written to the report with the parameters and the hardware.
///

namespace {

using ecf::test::Credentials;
using ecf::test::PermissionsServer;
using Clock  = std::chrono::steady_clock;
namespace bm = ecf::test::benchmark;

double milliseconds(Clock::duration d) {
    return std::chrono::duration<double, std::milli>(d).count();
}

double cpu_seconds(int pid) {
    return bm::parse_cpu_time(bm::run("ps -o time= -p " + std::to_string(pid)));
}

double memory_mib(int pid) {
    return bm::parse_memory_kib(bm::run("ps -o rss= -p " + std::to_string(pid)));
}

} // namespace

BOOST_AUTO_TEST_SUITE(P_Permissions)

BOOST_AUTO_TEST_CASE(test_permissions_benchmark) {
    ECF_NAME_THIS_TEST();

    const int suites     = bm::parameter("ECF_TEST_PERMISSIONS_SUITES", 50);
    const int families   = bm::parameter("ECF_TEST_PERMISSIONS_FAMILIES", 20);
    const int tasks      = bm::parameter("ECF_TEST_PERMISSIONS_TASKS", 10);
    const int identities = bm::parameter("ECF_TEST_PERMISSIONS_IDENTITIES", 20);
    const int sets       = bm::parameter("ECF_TEST_PERMISSIONS_READABLE_SETS", 5);
    const int iterations = bm::parameter("ECF_TEST_PERMISSIONS_ITERATIONS", 10);
    const char* report   = std::getenv("ECF_TEST_PERMISSIONS_REPORT");
    BOOST_REQUIRE_MESSAGE(sets <= suites && sets <= identities,
                          "ECF_TEST_PERMISSIONS_READABLE_SETS must not exceed the suites nor the identities");

    // Identity i belongs to the readable set i % sets; suite s is readable by the set s % sets
    std::vector<Credentials> users;
    for (int i = 0; i < identities; ++i) {
        users.push_back(Credentials{"user" + std::to_string(i), "secret" + std::to_string(i)});
    }
    std::string server_entries = R"([{"user": "admin", "permissions": "rwxlops"})";
    for (const auto& user : users) {
        server_entries += R"(, {"user": ")" + user.user + R"(", "permissions": "r"})";
    }
    server_entries += "]";

    auto defs = Defs::create();
    for (int s = 0; s < suites; ++s) {
        auto suite = defs->add_suite("s" + std::to_string(s));
        std::vector<ecf::Permissions::Entry> entries{{"admin", ecf::Roles{ecf::Role::Read, ecf::Role::Write}, false}};
        for (int i = s % sets; i < identities; i += sets) {
            entries.push_back({users[i].user, ecf::Roles{ecf::Role::Read}, false});
        }
        suite->add_permissions(ecf::Permissions(entries));
        for (int f = 0; f < families; ++f) {
            auto family = suite->add_family("f" + std::to_string(f));
            for (int t = 0; t < tasks; ++t) {
                auto task = family->add_task("t" + std::to_string(t));
                task->addLabel(Label("progress", "0"));
            }
        }
    }

    PermissionsServer server(Credentials{"admin", "admin-secret"}, users, server_entries, std::chrono::seconds(60));
    auto admin = server.administrator();
    BOOST_REQUIRE_MESSAGE(admin->load(defs) == 0, admin->errorMsg());

    std::vector<std::unique_ptr<ClientInvoker>> clients;
    for (const auto& user : users) {
        clients.push_back(server.client(user));
    }

    const double cpu_before = cpu_seconds(server.pid());

    // Full synchronisations: the first synchronisation of each client
    std::vector<double> full;
    for (auto& client : clients) {
        const auto start = Clock::now();
        BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
        full.push_back(milliseconds(Clock::now() - start));
        BOOST_REQUIRE_MESSAGE(client->server_reply().full_sync(), "the first synchronisation was not full");
    }

    // Rounds of changes, each followed by news and incremental synchronisations of every client
    std::vector<double> news;
    std::vector<double> incremental;
    for (int round = 0; round < iterations; ++round) {
        for (int s = 0; s < suites; ++s) {
            const auto path = "/s" + std::to_string(s) + "/f" + std::to_string(round % families) + "/t0";
            BOOST_REQUIRE_MESSAGE(admin->alter(path, "change", "label", "progress", std::to_string(round + 1)) == 0,
                                  admin->errorMsg());
        }
        for (auto& client : clients) {
            auto start = Clock::now();
            BOOST_REQUIRE_MESSAGE(client->news_local() == 0, client->errorMsg());
            news.push_back(milliseconds(Clock::now() - start));
            BOOST_REQUIRE_MESSAGE(client->get_news(), "no news reported after a change");
            start = Clock::now();
            BOOST_REQUIRE_MESSAGE(client->sync_local() == 0, client->errorMsg());
            incremental.push_back(milliseconds(Clock::now() - start));
            BOOST_REQUIRE_MESSAGE(!client->server_reply().full_sync(), "a synchronisation after a change was full");
        }
    }

    // Every client holds the last change of a suite of its readable set (identity i reads the suites s with
    // s % sets == i % sets)
    const auto last = std::to_string(iterations);
    for (std::size_t i = 0; i < clients.size(); ++i) {
        const auto path = "/s" + std::to_string(static_cast<int>(i) % sets) + "/f" +
                          std::to_string((iterations - 1) % families) + "/t0";
        const auto node = clients[i]->defs()->findAbsNode(path);
        BOOST_REQUIRE_MESSAGE(node, path << " missing for " << users[i].user);
        BOOST_CHECK_EQUAL(node->find_label("progress").new_value(), last);
    }

    const double cpu    = cpu_seconds(server.pid()) - cpu_before;
    const double memory = memory_mib(server.pid());

    // Checkpoint: save time and size
    const auto start = Clock::now();
    BOOST_REQUIRE_MESSAGE(admin->checkPtDefs() == 0, admin->errorMsg());
    const double save_ms     = milliseconds(Clock::now() - start);
    const auto checkpoint    = server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.check");
    const double size_mib    = static_cast<double>(fs::file_size(checkpoint)) / (1024.0 * 1024.0);
    const auto full_s        = bm::summarise(full);
    const auto incremental_s = bm::summarise(incremental);
    const auto news_s        = bm::summarise(news);

    nlohmann::ordered_json results;
    results["parameters"] = {{"suites", suites},
                             {"families", families},
                             {"tasks", tasks},
                             {"nodes", suites * (1 + families * (1 + tasks))},
                             {"identities", identities},
                             {"clients", identities},
                             {"readable_sets", sets},
                             {"iterations", iterations}};
    results["hardware"]   = {{"machine", bm::run("uname -m")},
                             {"cpu",
                              bm::run("sysctl -n machdep.cpu.brand_string 2>/dev/null || grep -m1 'model name' "
                                      "/proc/cpuinfo")},
                             {"cores", bm::run("getconf _NPROCESSORS_ONLN")},
                             {"system", bm::run("uname -sr")}};
    results["metrics"]    = {{"full_sync_ms_median", full_s.median},
                             {"full_sync_ms_max", full_s.max},
                             {"incremental_sync_ms_median", incremental_s.median},
                             {"incremental_sync_ms_max", incremental_s.max},
                             {"news_ms_median", news_s.median},
                             {"news_ms_max", news_s.max},
                             {"server_cpu_s", cpu},
                             {"server_memory_mib", memory},
                             {"checkpoint_size_mib", size_mib},
                             {"checkpoint_save_ms", save_ms}};

    std::cout << results.dump(2) << std::endl;
    bm::write_report(report ? report : "permissions_benchmark.json", results.dump(2) + "\n");

    // Every metric was measured: positive, except the CPU time, which rounds to zero for small trees
    for (const auto& [name, value] : results["metrics"].items()) {
        const bool measured = name == "server_cpu_s" ? value.get<double>() >= 0 : value.get<double>() > 0;
        BOOST_CHECK_MESSAGE(measured, "metric not measured: " << name);
    }
}

BOOST_AUTO_TEST_SUITE_END()
