/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm>
#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

namespace std {

std::ostream& operator<<(std::ostream& o, const std::vector<std::string_view>& vs) {
    o << '[';
    if (!vs.empty()) {
        o << '"' << vs[0] << '"';
        for (size_t i = 1; i != vs.size(); ++i) {
            o << ',' << '"' << vs[i] << '"';
        }
    }
    o << ']';
    return o;
}

} // namespace std

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Label)

BOOST_AUTO_TEST_CASE(test_label_parsing) {
    ECF_NAME_THIS_TEST();

    {
        std::string line = "label name \"value\"";
        std::vector<string> linetokens;
        Str::split(line, linetokens);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\nvalue")";
        std::vector<string> linetokens;
        Str::split(line, linetokens);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\nvalue");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = "label name \"value that is multiple token !!!! 23445 !^ & * ( )\"";
        std::vector<string> linetokens;
        Str::split(line, linetokens);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value that is multiple token !!!! 23445 !^ & * ( )");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\n that\n is\n multiple\n token\n and\n new\n \nlines")";
        std::vector<string> linetokens;
        Str::split(line, linetokens);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\n that\n is\n multiple\n token\n and\n new\n \nlines");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
}

using ecf::algorithm::tokenize_quotation;
using namespace std::string_literals;

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_1) {
    auto tokens = tokenize_quotation("");
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_2) {
    auto tokens = tokenize_quotation("    ");
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_3) {
    auto line   = "a b c"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"a"s, "b"s, "c"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_3a) {
    auto line   = " a b c "s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"a"s, "b"s, "c"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_3b) {
    auto line   = "   a  b  c   "s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"a"s, "b"s, "c"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_4) {
    auto line   = "   aa  bbb  cccc   "s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"aa"s, "bbb"s, "cccc"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_5) {
    auto line   = "  aa   bbb   cccc  "s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"aa"s, "bbb"s, "cccc"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_6) {
    auto line   = R"(  aa   "bbb"   cccc  )"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"aa"s, R"("bbb")"s, "cccc"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_7) {
    auto line   = R"(  aa   "b'b'b"   cccc  )"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"aa"s, R"("b'b'b")"s, "cccc"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_8) {
    auto line   = R"(  aa   "b'b\nb'b"   cccc  )"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"aa"s, R"("b'b\nb'b")"s, "cccc"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_9) {
    auto line   = R"(label name " default value " # " current # value ")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(
        tokens,
        (std::vector<std::string_view>{"label"s, "name"s, R"(" default value ")"s, "#"s, R"(" current # value ")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_10) {
    auto line   = R"(label name " default\nvalue " # " current\n#\nvalue ")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens,
                      (std::vector<std::string_view>{
                          "label"s, "name"s, R"(" default\nvalue ")"s, "#"s, R"(" current\n#\nvalue ")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_11) {
    auto line   = R"(label name default\nvalue # current\n#\nvalue)"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(
        tokens,
        (std::vector<std::string_view>{"label"s, "name"s, R"(default\nvalue)"s, "#"s, R"(current\n#\nvalue)"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_12) {
    auto line   = R"(label name "default value" # "current value")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(
        tokens, (std::vector<std::string_view>{"label"s, "name"s, R"("default value")"s, "#"s, R"("current value")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_13) {
    auto line   = R"(label name ""default value"" # ""current value"")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(
        tokens,
        (std::vector<std::string_view>{"label"s, "name"s, R"(""default value"")"s, "#"s, R"(""current value"")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_14) {
    auto line   = R"(label name """")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens,
                      (std::vector<std::string_view>{
                          "label"s,
                          "name"s,
                          R"("""")"s,
                      }));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_15) {
    auto line   = R"(label name """" # """")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"label"s, "name"s, R"("""")"s, R"("""")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tokenize_16) {
    auto line   = R"(label name ""  #  "" # ""  #  "")"s;
    auto tokens = tokenize_quotation(line);
    BOOST_CHECK_EQUAL(tokens, (std::vector<std::string_view>{"label"s, "name"s, R"(""  #  "")"s, R"(""  #  "")"s}));
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_without_quotes) {
    std::string line = "label name value";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_single_quotes) {
    std::string line = "label name 'value'";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes) {
    std::string line = "label name \"value\"";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes_including_new_line) {
    std::string line = R"(label name "value\nvalue")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value\nvalue");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes_including_spaces_and_regular_symbols) {
    std::string line = "label name \"value that is multiple token !!!! 23445 !^ & * ( )\"";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value that is multiple token !!!! 23445 !^ & * ( )");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes_including_hash_symbol) {
    std::string line = "label name \"valuewith#symbol\"";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "valuewith#symbol");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes_including_spaces_and_hash_symbol) {
    std::string line = "label name \"  value  with  #  symbol  \"  ";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "  value  with  #  symbol  ");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_value_with_dquotes_including_spaces_and_new_lines) {
    std::string line = R"(label name "value\n that\n is\n multiple\n token\n and\n new\n \nlines")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, false);

    Label expected("name", "value\n that\n is\n multiple\n token\n and\n new\n \nlines");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_and_current_value_with_dquotes_including_spaces) {
    std::string line = R"(label name " default value " # " current value ")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", " default value ", " current value ");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_and_current_value_with_dquotes_including_spaces_and_squotes) {
    std::string line = R"(label name " 'default' value " # " 'current' value ")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", " 'default' value ", " 'current' value ");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_and_current_value_with_dquotes_including_multiple_spaces) {
    std::string line = R"(label name "  default   value  " # "  current   value  ")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", "  default   value  ", "  current   value  ");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_with_default_and_current_value_with_dquotes_including_hash_symbol) {
    std::string line = R"(label name "  default  #  value  " # "  current  #  value  ")";
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", "  default  #  value  ", "  current  #  value  ");
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_1) {
    auto line = R"(label name "" # "#0 30/50 or 0.048 ~> 0.050")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", R"()"s, R"(#0 30/50 or 0.048 ~> 0.050)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_2) {
    auto line = R"(label name "NA")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("name", R"(NA)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_3) {
    auto line = R"(label BUG "to fix")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("BUG", R"(to fix)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_4) {
    auto line = R"(label date "" # "20000101")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("date", R"()"s, R"(20000101)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_5) {
    auto line = R"(label info "" # "00,ctrlfc,an,0000,20250407,Nodes:2, elapsed: 00:01:51")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("info", R"()"s, R"(00,ctrlfc,an,0000,20250407,Nodes:2, elapsed: 00:01:51)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_CASE(test_label_parsing_example_from_emos_server_6) {
    auto line = R"(label Traj "" # "#0 48/48")"s;
    std::vector<std::string> linetokens;
    Str::split(line, linetokens);

    Label label;
    label.parse(line, linetokens, true);

    Label expected("Traj", R"()"s, R"(#0 48/48)"s);
    BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
    BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                        "Expected " << expected.dump() << " but found " << label.dump());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
