
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Request
// Author      : Avi
// Revision    : $Revision$
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <iostream>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "DefsStructureParser.hpp"
#include "Ecf.hpp"
#include "File.hpp"
#include "Node.hpp"
#include "PersistHelper.hpp"
// #include "PrintStyle.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

std::vector<std::string> defs_with_expected_warnings() {
    return {"limit/limit3.def"};
}

BOOST_AUTO_TEST_SUITE(ParserTestSuite)

void test_defs(const std::string& directory, bool pass) {
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(directory));

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));
    DebugEquality debug_equality; // only as affect in DEBUG build

    // std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; ++dir_itr) {
        try {
            fs::path relPath(directory + "/" + dir_itr->path().filename().string());

            // recurse down directories
            if (fs::is_directory(dir_itr->status())) {
                test_defs(relPath.string(), pass);
                continue;
            }

            // std::cout << "......Parsing file " << relPath.string() << "\n";

            Defs defs;
            DefsStructureParser parser(&defs, relPath.string());

            std::string errorMsg, warningMsg;
            bool parsedOk = parser.doParse(errorMsg, warningMsg);
            if (pass) {
                // Test expected to pass
                BOOST_CHECK_MESSAGE(parsedOk, "Failed to parse file " << relPath << "\n" << errorMsg);

                if (parsedOk) {
                    // Check triggers and limits
                    std::string err_msg, warn_msg;
                    defs.check(err_msg, warn_msg);
                    BOOST_CHECK_MESSAGE(err_msg.empty(),
                                        "Expected no errors but found:" << err_msg << "File: " << relPath.string());

                    // expect warnings for some file:
                    bool ignore = false;
                    for (auto path : defs_with_expected_warnings()) {
                        if (relPath.string().find(path) != std::string::npos) {
                            ignore = true;
                            break;
                        }
                    }
                    if (!ignore)
                        BOOST_CHECK_MESSAGE(warn_msg.empty(),
                                            "Expected no warnings but found:" << warn_msg
                                                                              << "File: " << relPath.string());

                    // Write parsed file to a temporary file on disk, and reload, then compare defs, should be the same
                    PersistHelper helper;
                    BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, parser.get_file_type()),
                                        relPath.string() << " " << helper.errorMsg());
                    BOOST_CHECK_MESSAGE(helper.test_cereal_checkpt_and_reload(defs),
                                        relPath.string() << " " << helper.errorMsg());

                    // test copy constructor
                    Defs copy_of_defs = Defs(defs);
                    BOOST_CHECK_MESSAGE(copy_of_defs == defs, "Error copy constructor failed " << relPath);

                    // Test parse by string is same as parsing by file:
                    {
                        std::string contents;
                        BOOST_CHECK_MESSAGE(File::open(relPath.string(), contents),
                                            "Error could not open file " << relPath);
                        // cout << "parsing file " << relPath << " by string\n";

                        Defs defs2;
                        std::string errorMsg2, warningMsg2;
                        BOOST_CHECK_MESSAGE(defs2.restore_from_string(contents, errorMsg2, warningMsg2),
                                            "restore from string failed for file " << relPath << "\n"
                                                                                   << errorMsg2);
                        BOOST_CHECK_MESSAGE(defs2 == defs,
                                            "Parse by string != parse by filename for file:\n"
                                                << relPath);
                    }

                    // Make sure all nodes can be found
                    std::vector<Node*> all_nodes;
                    defs.getAllNodes(all_nodes);
                    for (auto node : all_nodes) {
                        node_ptr found_node = defs.findAbsNode(node->absNodePath());
                        BOOST_CHECK_MESSAGE(found_node.get(), "Could not find node " << node->debugNodePath());
                        BOOST_CHECK_MESSAGE(found_node.get() == node,
                                            " Expected to find " << node->debugNodePath() << " but found "
                                                                 << found_node->debugNodePath());
                    }
                }
            }
            else {
                // test expected to fail
                if (parsedOk) {
                    // Check triggers and limits
                    std::string err_msg, warn_msg;
                    defs.check(err_msg, warn_msg);
                    BOOST_CHECK_MESSAGE(!err_msg.empty() || !warn_msg.empty(),
                                        "Expected problems with trigger/limits but found none for file" << relPath
                                                                                                        << "\n");
                }
                else {
                    BOOST_CHECK_MESSAGE(!parsedOk, "Parse expected to fail for " << relPath << "\n" << errorMsg);
                }
            }
        }
        catch (const std::exception& ex) {
            std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_defs) {
    cout << "AParser:: ...test_parsing_for_good_defs\n";

    std::string path = File::test_data("ANode/parser/test/data/good_defs", "parser");

    // All the defs in this directory are expected to pass
    test_defs(path, true);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_bad_defs) {
    cout << "AParser:: ...test_parsing_for_bad_defs\n";

    std::string path = File::test_data("ANode/parser/test/data/bad_defs", "parser");

    // All the defs in this directory are expected to fail
    test_defs(path, false);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_defs_state) {
    cout << "AParser:: ...test_parsing_for_good_defs_state\n";

    std::string path = File::test_data("ANode/parser/test/data/good_defs_state", "parser");

    // All the defs in this directory are expected to pass
    test_defs(path, true);
}

void test_node_defs(const std::string& directory, bool pass) {
    fs::path full_path(fs::initial_path<fs::path>());
    full_path = fs::system_complete(fs::path(directory));

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));

    // std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; ++dir_itr) {
        try {
            fs::path relPath(directory + "/" + dir_itr->path().filename().string());

            // recurse down directories
            if (fs::is_directory(dir_itr->status())) {
                test_node_defs(relPath.string(), pass);
                continue;
            }

            // std::cout << "......Parsing file " << relPath.string() << "\n";
            std::string file_contents;
            if (!ecf::File::open(relPath.string(), file_contents)) {
                std::cout << "......Could not open file" << relPath.string() << "\n";
                continue;
            }

            // Parse string as a standalone node.
            DefsStructureParser parser(file_contents);
            std::string errorMsg, warningMsg;
            bool parsedOk = parser.doParse(errorMsg, warningMsg);
            if (pass) {
                // Test expected to pass
                BOOST_CHECK_MESSAGE(parsedOk, "Failed to parse file " << relPath << "\n" << errorMsg);
                BOOST_CHECK_MESSAGE(parser.the_node_ptr(), "Failed to parse file " << relPath << "\n" << errorMsg);
                // PrintStyle print_style(PrintStyle::MIGRATE);
                // parser.the_node_ptr()->print(cout);
            }
            else {
                // test expected to fail
                // std::cout << errorMsg << "\n";
                BOOST_CHECK_MESSAGE(!parser.the_node_ptr(),
                                    "Parse expected to fail for " << relPath << "\n"
                                                                  << errorMsg);
            }
        }
        catch (const std::exception& ex) {
            std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_node) {
    cout << "AParser:: ...test_parsing_node\n";

    std::string path = File::test_data("ANode/parser/test/data/good_node_defs", "parser");

    // All the defs in this directory are expected to pass
    test_node_defs(path, true);
}

BOOST_AUTO_TEST_SUITE_END()
