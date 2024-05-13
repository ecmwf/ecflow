/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/AvisoParser.hpp"

#include <stdexcept>

#include <boost/program_options.hpp>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

namespace {

template <typename T>
auto get_option_value(const boost::program_options::variables_map& vm,
                      const std::string& option_name,
                      const std::string& line) {
    if (!vm.count(option_name)) {
        throw std::runtime_error("AvisoParser::doParse: Could not find '" + option_name + "' option in line: " + line);
    }
    return vm[option_name].as<T>();
}

} // namespace

ecf::AvisoAttr AvisoParser::parse_aviso_line(const std::string& line) {
    return parse_aviso_line(line, nullptr);
}

ecf::AvisoAttr AvisoParser::parse_aviso_line(const std::string& line, const std::string& name) {
    return parse_aviso_line(line, name, nullptr);
}

ecf::AvisoAttr AvisoParser::parse_aviso_line(const std::string& line, const std::string& name, Node* parent) {
    auto updated_line = line + " --name " + name;
    return parse_aviso_line(updated_line, parent);
}

ecf::AvisoAttr AvisoParser::parse_aviso_line(const std::string& line, Node* parent) {
    std::vector<std::string> tokens;
    {
        // Since po::command_line_parser requires a vector of strings, we need convert from string_view to string
        std::vector<std::string_view> extracted = ecf::Str::tokenize_quotation(line, "'");
        std::transform(std::begin(extracted),
                       std::end(extracted),
                       std::back_inserter(tokens),
                       [](const std::string_view& sv) { return std::string{sv}; });
    }

    namespace po = boost::program_options;
    po::options_description description("AvisoParser");
    description.add_options()("name", po::value<std::string>());
    description.add_options()("listener", po::value<std::string>());
    description.add_options()("url", po::value<std::string>());
    description.add_options()("schema", po::value<std::string>());
    description.add_options()("polling", po::value<std::string>()->default_value("40"));
    description.add_options()("revision", po::value<uint64_t>()->default_value(0));

    po::parsed_options parsed_options = po::command_line_parser(tokens).options(description).run();

    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    auto name     = get_option_value<ecf::AvisoAttr::name_t>(vm, "name", line);
    auto listener = get_option_value<ecf::AvisoAttr::listener_t>(vm, "listener", line);
    auto url      = get_option_value<ecf::AvisoAttr::url_t>(vm, "url", line);
    auto schema   = get_option_value<ecf::AvisoAttr::schema_t>(vm, "schema", line);
    auto polling  = get_option_value<ecf::AvisoAttr::polling_t>(vm, "polling", line);
    auto revision = get_option_value<ecf::AvisoAttr::revision_t>(vm, "revision", line);

    return ecf::AvisoAttr{parent, name, listener, url, schema, polling, revision};
}

bool AvisoParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("AvisoParser::doParse: Could not add aviso as node stack is empty at line: " + line);
    }

    Node* parent = nodeStack_top();

    auto parsed = parse_aviso_line(line, parent);
    nodeStack_top()->addAviso(parsed);
    nodeStack_top()->absNodePath();

    return true;
}
