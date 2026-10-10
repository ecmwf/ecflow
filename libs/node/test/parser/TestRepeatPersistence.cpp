// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Pins how Repeats are written to, and read from, definitions and checkpoints
///        (DEFS, STATE, MIGRATE and NET styles).
///

namespace {

///
/// @brief The outcome of loading a definition from text.
///
struct Loaded
{
    bool ok{false};
    std::string error;
    defs_ptr defs = Defs::create();

    const Repeat& repeat(const std::string& path = "/s") const { return defs->findAbsNode(path)->repeat(); }
};

Loaded load(const std::string& text) {
    Loaded loaded;
    std::string warning;
    loaded.ok = loaded.defs->restore_from_string(text, loaded.error, warning);
    return loaded;
}

///
/// @brief Builds the text of a definition holding a single suite `s` with the given Repeat line.
///
std::string suite_with(const std::string& repeat_line, const std::string& style = "") {
    std::string text;
    if (!style.empty()) {
        text += "defs_state " + style + "\n";
    }
    text += "suite s\n  " + repeat_line + "\nendsuite\n";
    return text;
}

///
/// @brief Retrieves the line holding the Repeat of the given node, as written in the given style.
///
std::string repeat_line_of(const Defs& defs, PrintStyle::Type_t style, const std::string& node) {
    std::istringstream text(ecf::as_string(defs, style));
    std::string line;
    bool in_node = false;
    while (std::getline(text, line)) {
        auto first = line.find_first_not_of(' ');
        if (first == std::string::npos) {
            continue;
        }
        line = line.substr(first);
        if (line.rfind("family ", 0) == 0) {
            in_node = (line.substr(7, node.size()) == node &&
                       (line.size() == 7 + node.size() || line[7 + node.size()] == ' '));
        }
        if (in_node && line.rfind("repeat ", 0) == 0) {
            return line;
        }
    }
    return std::string{};
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_RepeatPersistence)

BOOST_AUTO_TEST_CASE(written_lines_carry_the_value_after_the_attributes) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->add_family("integer")->addRepeat(RepeatInteger("N", 0, 10, 1));
    s->add_family("date")->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));
    s->add_family("datetime")->addRepeat(RepeatDateTime("T", "20260101T000000", "20260102T000000", "06:00:00"));
    s->add_family("datelist")->addRepeat(RepeatDateList("D", {20260101, 20260201}));
    s->add_family("enumerated")->addRepeat(RepeatEnumerated("E", {"a", "b"}));
    s->add_family("string")->addRepeat(RepeatString("S", {"a", "b"}));
    s->add_family("day")->addRepeat(RepeatDay(2));
    s->add_family("unchanged")->addRepeat(RepeatInteger("N", 0, 10, 2));

    s->find_by_name("integer")->changeRepeat("4");
    s->find_by_name("date")->changeRepeat("20260315");
    s->find_by_name("datetime")->changeRepeat("20260101T120000");
    s->find_by_name("datelist")->changeRepeat("20260201");
    s->find_by_name("enumerated")->changeRepeat("b");
    s->find_by_name("string")->changeRepeat("b");

    struct Expected
    {
        std::string node;
        std::string migrate;
        std::string defs;
    };
    const std::vector<Expected> expected{
        {"integer", "repeat integer N 0 10 # 4", "repeat integer N 0 10"},
        {"date", "repeat date YMD 20260101 20261231 1 # 20260315", "repeat date YMD 20260101 20261231 1"},
        {"datetime",
         "repeat datetime T 20260101T000000 20260102T000000 06:00:00 # 20260101T120000",
         "repeat datetime T 20260101T000000 20260102T000000 06:00:00"},
        {"datelist", R"(repeat datelist D "20260101" "20260201" # 1)", R"(repeat datelist D "20260101" "20260201")"},
        {"enumerated", R"(repeat enumerated E "a" "b" # 1)", R"(repeat enumerated E "a" "b")"},
        {"string", R"(repeat string S "a" "b" # 1)", R"(repeat string S "a" "b")"},
        {"day", "repeat day 2", "repeat day 2"},
        {"unchanged", "repeat integer N 0 10 2", "repeat integer N 0 10 2"},
    };
    for (const auto& e : expected) {
        BOOST_CHECK_EQUAL(repeat_line_of(defs, PrintStyle::MIGRATE, e.node), e.migrate);
        BOOST_CHECK_EQUAL(repeat_line_of(defs, PrintStyle::DEFS, e.node), e.defs);
    }
}

BOOST_AUTO_TEST_CASE(every_checkpoint_style_reloads_the_same_repeat) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->add_family("descending")->addRepeat(RepeatInteger("N", 10, 0, -2));
    s->add_family("datetime")->addRepeat(RepeatDateTime("T", "20260102T000000", "20260101T000000", "00:-30:00"));
    s->add_family("completed")->addRepeat(RepeatDate("YMD", 20260101, 20260103, 1));
    s->add_family("list")->addRepeat(RepeatEnumerated("E", {"a", "b"}));

    s->find_by_name("descending")->changeRepeat("4");
    s->find_by_name("datetime")->changeRepeat("20260101T120000");
    for (int i = 0; i < 3; ++i) {
        s->find_by_name("completed")->increment_repeat(); // past the end
    }
    s->find_by_name("list")->increment_repeat();
    s->find_by_name("list")->increment_repeat(); // past the last member
    BOOST_REQUIRE(!s->find_by_name("completed")->repeat().valid());
    BOOST_REQUIRE(!s->find_by_name("list")->repeat().valid());

    for (auto style : {PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        auto loaded = load(ecf::as_string(defs, style));
        BOOST_REQUIRE_MESSAGE(loaded.ok, "style " << PrintStyle::to_string(style) << ": " << loaded.error);
        for (const std::string node : {"descending", "datetime", "completed", "list"}) {
            const auto& original = s->find_by_name(node)->repeat();
            const auto& restored = loaded.repeat("/s/" + node);
            BOOST_CHECK_MESSAGE(restored == original,
                                "style " << PrintStyle::to_string(style) << ": expected " << original.dump()
                                         << " but found " << restored.dump());
        }
    }
}

BOOST_AUTO_TEST_CASE(reload_keeps_any_value_without_validation) {
    ECF_NAME_THIS_TEST();

    {
        auto loaded = load(suite_with("repeat date D 20240101 20240131 7 # 20240102", "MIGRATE")); // off the grid
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 20240102);
    }
    {
        auto loaded = load(suite_with("repeat date D 20240101 20240131 1 # 20250101", "MIGRATE")); // out of range
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 20250101);
        BOOST_CHECK_EQUAL(loaded.repeat().last_valid_value(), 20240131);
    }
    {
        auto loaded = load(suite_with("repeat integer N 0 10 3 # 4", "MIGRATE")); // off the grid
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 4);
    }
    {
        auto loaded = load(suite_with("repeat enumerated E a b # 7", "MIGRATE")); // past the last member
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().index_or_value(), 7);
        BOOST_CHECK(!loaded.repeat().valid());
    }
    {
        auto loaded = load(suite_with("repeat datelist D 20240101 20240102 # -1", "MIGRATE")); // before the first
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().index_or_value(), -1);
    }
}

BOOST_AUTO_TEST_CASE(reload_refuses_what_construction_refuses) {
    ECF_NAME_THIS_TEST();

    // A checkpoint holding a Repeat that its constructor refuses cannot be loaded at all
    BOOST_CHECK(!load(suite_with("repeat date YMD 20261231 20260101 1 # 20261231", "MIGRATE")).ok);
    BOOST_CHECK(!load(suite_with("repeat date YMD 20260101 20261231 0", "MIGRATE")).ok);
    BOOST_CHECK(!load(suite_with("repeat datetime T 20260101T000000 20260102T000000 00:00:00", "MIGRATE")).ok);

    // An integer Repeat is not validated, so inconsistent ones load
    for (const std::string line :
         {"repeat integer N 10 0 1 # 10", "repeat integer N 0 10 0 # 0", "repeat integer N 10 0"}) {
        auto loaded = load(suite_with(line, "MIGRATE"));
        BOOST_CHECK_MESSAGE(loaded.ok, line << ": " << loaded.error);
    }
}

BOOST_AUTO_TEST_CASE(malformed_value_tokens) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(!load(suite_with("repeat integer N 0 10 # abc", "MIGRATE")).ok);
    BOOST_CHECK(!load(suite_with("repeat integer N 0 10 # 4 # x", "MIGRATE")).ok);
    {
        auto loaded = load(suite_with("repeat integer N 0 10 # 4 extra", "MIGRATE"));
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 4);
    }
    {
        // without a space after '#', the value is not recognised and the Repeat stays at its start
        auto loaded = load(suite_with("repeat integer N 0 10 #4", "MIGRATE"));
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 0);
    }
    {
        // a definition file ignores the value
        auto loaded = load(suite_with("repeat integer N 0 10 # abc"));
        BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
        BOOST_CHECK_EQUAL(loaded.repeat().value(), 0);
    }
}

BOOST_AUTO_TEST_CASE(completed_integer_beyond_int_cannot_be_reloaded) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addRepeat(RepeatInteger("N", 1, 10, 2147483647));
    s->increment_repeat();
    BOOST_REQUIRE_EQUAL(s->repeat().value(), 2147483648L);

    auto text = ecf::as_string(defs, PrintStyle::MIGRATE);
    BOOST_CHECK_MESSAGE(text.find("repeat integer N 1 10 2147483647 # 2147483648") != std::string::npos, text);

    // the value is written as a long, but read back as an int
    auto loaded = load(text);
    BOOST_CHECK(!loaded.ok);
    BOOST_CHECK_MESSAGE(loaded.error.find("could not extract repeat value") != std::string::npos, loaded.error);
}

BOOST_AUTO_TEST_CASE(datetime_without_step_does_not_accept_a_comment) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(load(suite_with("repeat datetime T 20240101T000000 20240102T000000")).ok);
    BOOST_CHECK(load(suite_with("repeat datetime T 20240101T000000 20240102T000000 06:00:00 # c")).ok);

    // the token after the end is taken as the step, even when it starts a comment
    auto loaded = load(suite_with("repeat datetime T 20240101T000000 20240102T000000 # c"));
    BOOST_CHECK(!loaded.ok);
}

BOOST_AUTO_TEST_CASE(integer_tokens_in_a_definition) {
    ECF_NAME_THIS_TEST();

    struct Case
    {
        std::string line;
        bool ok;
        std::string written;
    };
    const std::vector<Case> cases{
        {"repeat integer N +0 +10 +2", true, "repeat integer N 0 10 2"},
        {"repeat integer N 007 10", true, "repeat integer N 7 10"},
        {"repeat integer N -2147483648 0", true, "repeat integer N -2147483648 0"},
        {"repeat integer N 0 10 2 99", true, "repeat integer N 0 10 2"}, // the extra token is ignored
        {"repeat integer N 0x0 10", false, ""},
        {"repeat integer N 0 1e1", false, ""},
        {"repeat integer N 0 2147483648", false, ""},
    };
    for (const auto& c : cases) {
        auto loaded = load(suite_with(c.line));
        BOOST_CHECK_MESSAGE(loaded.ok == c.ok, c.line << ": " << loaded.error);
        if (loaded.ok) {
            auto text = ecf::as_string(*loaded.defs, PrintStyle::DEFS);
            BOOST_CHECK_MESSAGE(text.find(c.written + "\n") != std::string::npos, c.line << " written as\n" << text);
        }
    }
}

BOOST_AUTO_TEST_CASE(members_with_spaces_do_not_round_trip) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addRepeat(RepeatString("S", {"a b", "c"}));

    auto text = ecf::as_string(defs, PrintStyle::DEFS);
    BOOST_REQUIRE_MESSAGE(text.find(R"(repeat string S "a b" "c")") != std::string::npos, text);

    // the members are split at the space when read back
    auto loaded = load(text);
    BOOST_REQUIRE_MESSAGE(loaded.ok, loaded.error);
    const auto& values = loaded.repeat().as<RepeatString>().values();
    const std::vector<std::string> expected{"\"a", "b\"", "c"};
    BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(datelist_data_files) {
    ECF_NAME_THIS_TEST();

    // These files are not collected by the data-file runner, which only considers .def files
    auto parse = [](const std::string& directory, const std::string& file) {
        std::string path =
            ecf::File::test_data("libs/node/test/parser/data/" + directory + "/repeat/" + file, "parser");
        Defs defs;
        DefsStructureParser parser(&defs, path);
        std::string error, warning;
        return parser.doParse(error, warning);
    };

    BOOST_CHECK(parse("good_defs", "repeat_date_list.ecf"));
    BOOST_CHECK(!parse("bad_defs", "repeat_date_list.ecf"));  // invalid months
    BOOST_CHECK(!parse("bad_defs", "repeat_date_list1.ecf")); // empty list
}

BOOST_AUTO_TEST_SUITE_END() // T_RepeatPersistence

BOOST_AUTO_TEST_SUITE_END() // U_Parser
