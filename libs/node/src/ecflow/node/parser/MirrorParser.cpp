/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/MirrorParser.hpp"

#include <stdexcept>

#include <boost/program_options.hpp>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/MirrorAttr.hpp"
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

auto parse_mirror_line(const std::string& line, Node* parent) {
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
    po::options_description description("MirrorParser");
    description.add_options()("name", po::value<std::string>());
    description.add_options()("remote_path", po::value<std::string>());
    description.add_options()("remote_host", po::value<std::string>());
    description.add_options()("remote_port", po::value<std::string>());
    description.add_options()("polling", po::value<std::string>()->default_value("40"));

    po::parsed_options parsed_options = po::command_line_parser(tokens).options(description).run();

    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    auto name        = get_option_value<ecf::MirrorAttr::name_t>(vm, "name", line);
    auto ecflow_path = get_option_value<ecf::MirrorAttr::remote_path_t>(vm, "remote_path", line);
    auto ecflow_host = get_option_value<ecf::MirrorAttr::remote_host_t>(vm, "remote_host", line);
    auto ecflow_port = get_option_value<ecf::MirrorAttr::remote_port_t>(vm, "remote_port", line);
    auto polling     = get_option_value<ecf::MirrorAttr::polling_t>(vm, "polling", line);

    return ecf::MirrorAttr{parent, name, ecflow_path, ecflow_host, ecflow_port, polling};
}

} // namespace

bool MirrorParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("AvisoParser::doParse: Could not add aviso as node stack is empty at line: " + line);
    }

    Node* parent = nodeStack_top();

    auto parsed = parse_mirror_line(line, parent);
    nodeStack_top()->addMirror(parsed);

    return true;
}
