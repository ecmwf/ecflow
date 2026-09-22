// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PersistHelper.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

std::vector<std::string> defs_with_expected_warnings() {
    return {"limit/limit3.def"};
}

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_Parser)

std::vector<fs::path> list_test_data_files(const std::string& base_directory, const std::string& extn = ".def") {
    auto full_path = fs::absolute(base_directory);
    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));

    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(full_path)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == extn) {
            files.push_back(entry.path());
        }
    }
    return files;
}

///
/// @brief Collects the paths of the nodes declared in the given definition file.
///
/// @details The statements are extracted the same way the parser handles them, that is by
/// discarding the comment at the end of a line and splitting the line at ';', so that nodes
/// sharing a line with other statements are taken into account. Only suites, families and tasks
/// are considered, since their path can be reconstructed from the enclosing declarations alone.
///
/// @param[in] path Path to an existing definition file
/// @return The paths of the declared nodes, in order of appearance
///
std::vector<std::string> declared_node_paths(const std::string& path) {
    std::vector<std::string> paths;
    std::vector<std::string> ancestors;

    auto current_path = [&ancestors](const std::string& name) {
        std::string result;
        for (const auto& ancestor : ancestors) {
            result += "/" + ancestor;
        }
        return result + "/" + name;
    };

    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        auto comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        std::vector<std::string> statements;
        ecf::algorithm::split_at(statements, line, ";");
        for (const auto& statement : statements) {
            std::vector<std::string> tokens;
            ecf::algorithm::split_at(tokens, statement);
            if (tokens.empty()) {
                continue;
            }

            if (tokens[0] == "endsuite" || tokens[0] == "endfamily") {
                if (!ancestors.empty()) {
                    ancestors.pop_back();
                }
            }
            else if (tokens.size() >= 2 && (tokens[0] == "suite" || tokens[0] == "family")) {
                paths.push_back(current_path(tokens[1]));
                ancestors.push_back(tokens[1]);
            }
            else if (tokens.size() >= 2 && tokens[0] == "task") {
                paths.push_back(current_path(tokens[1]));
            }
        }
    }

    return paths;
}

void test_defs(const std::string& directory, bool pass) {
    auto full_path = fs::absolute(directory);

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));
    DebugEquality debug_equality; // only has effect in DEBUG build

    // std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";

    auto def_files = list_test_data_files(full_path.string(), ".def");
    for (const auto& relPath : def_files) {
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
                if (!ignore) {
                    BOOST_CHECK_MESSAGE(warn_msg.empty(),
                                        "Expected no warnings but found:" << warn_msg << "File: " << relPath.string());
                }

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
                    BOOST_CHECK_MESSAGE(defs2 == defs, "Parse by string != parse by filename for file:\n" << relPath);
                }

                // Make sure every declared node is present, since a statement silently dropped
                // by the parser would otherwise go unnoticed
                for (const auto& node_path : declared_node_paths(relPath.string())) {
                    BOOST_CHECK_MESSAGE(defs.findAbsNode(node_path),
                                        "Node " << node_path << " is declared in " << relPath.string()
                                                << " but was not parsed");
                }

                // Make sure all nodes can be found
                auto all_nodes = ecf::get_all_nodes(defs);
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
                                    "Expected problems with trigger/limits but found none for file" << relPath << "\n");
            }
            else {
                BOOST_CHECK_MESSAGE(!parsedOk, "Parse expected to fail for " << relPath << "\n" << errorMsg);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_defs) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs", "parser");

    // All the defs in this directory are expected to pass
    test_defs(path, true);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_bad_defs) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/bad_defs", "parser");

    // All the defs in this directory are expected to fail
    test_defs(path, false);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_defs_state) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs_state", "parser");

    // All the defs in this directory are expected to pass
    test_defs(path, true);
}

void test_node_defs(const std::string& directory, bool pass) {
    auto full_path = fs::absolute(directory);

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
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_node_defs", "parser");

    // All the defs in this directory are expected to pass
    test_node_defs(path, true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
