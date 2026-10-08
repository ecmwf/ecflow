// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <set>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Operations.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace {

///
/// @brief Records every item a traversal visits.
///
struct Recorder
{
    void operator()(Defs&) { visited.insert("defs"); }
    void operator()(Suite& s) { visited.insert("suite:" + s.absNodePath()); }
    void operator()(Family& f) { visited.insert("family:" + f.absNodePath()); }
    void operator()(Task& t) { visited.insert("task:" + t.absNodePath()); }
    void operator()(Alias& a) { visited.insert("alias:" + a.absNodePath()); }
    void operator()(ecf::AvisoAttr& a) { visited.insert("aviso:" + a.name()); }
    void operator()(ecf::MirrorAttr& m) { visited.insert("mirror:" + m.name()); }

    template <typename T>
    void operator()(T&&) {
        visited.insert("other");
    }

    std::set<std::string> visited;
};

defs_ptr make_defs() {
    std::string definition = R"(
        suite s1
          family f1
            task t1
              aviso --name A --listener '{ "event": "mars" }'
            family f2
              task t2
                mirror --name M --remote_path /s/t --remote_host h --remote_port 3141
            endfamily
          endfamily
          task t3
        endsuite
        suite s2
          task t4
        endsuite
    )";

    defs_ptr defs = Defs::create();
    DefsStructureParser parser(defs.get(), definition, true);
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(parser.doParse(errorMsg, warningMsg), "Failed to parse definition: " << errorMsg);
    return defs;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Operations)

BOOST_AUTO_TEST_CASE(visit_all_from_defs_reaches_every_node_and_attribute) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs();

    Recorder recorder;
    ecf::visit_all(*defs, recorder);

    std::set<std::string> expected = {"defs",
                                      "suite:/s1",
                                      "suite:/s2",
                                      "family:/s1/f1",
                                      "family:/s1/f1/f2",
                                      "task:/s1/f1/t1",
                                      "task:/s1/f1/f2/t2",
                                      "task:/s1/t3",
                                      "task:/s2/t4",
                                      "aviso:A",
                                      "mirror:M"};
    BOOST_CHECK_EQUAL_COLLECTIONS(recorder.visited.begin(), recorder.visited.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(visit_all_reaches_the_aliases_and_their_attributes) {
    ECF_NAME_THIS_TEST();

    auto defs  = make_defs();
    auto t1    = defs->findAbsNode("/s1/f1/t1")->isTask();
    auto alias = t1->add_alias_only();
    alias->addAviso(ecf::AvisoAttr{
        alias.get(), "B", R"({ "event": "mars" })", ecf::AvisoAttr::default_url, 0, ecf::AvisoAttr::default_auth, ""});

    Recorder recorder;
    ecf::visit_all(*defs, recorder);

    BOOST_CHECK(recorder.visited.count("alias:" + alias->absNodePath()) == 1);
    BOOST_CHECK(recorder.visited.count("aviso:B") == 1);
    BOOST_CHECK(recorder.visited.count("aviso:A") == 1);
}

BOOST_AUTO_TEST_CASE(visit_all_from_suite_reaches_every_node_and_attribute_of_the_suite) {
    ECF_NAME_THIS_TEST();

    auto defs = make_defs();

    Recorder recorder;
    ecf::visit_all(*defs->suites()[0], recorder);

    std::set<std::string> expected = {"suite:/s1",
                                      "family:/s1/f1",
                                      "family:/s1/f1/f2",
                                      "task:/s1/f1/t1",
                                      "task:/s1/f1/f2/t2",
                                      "task:/s1/t3",
                                      "aviso:A",
                                      "mirror:M"};
    BOOST_CHECK_EQUAL_COLLECTIONS(recorder.visited.begin(), recorder.visited.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
