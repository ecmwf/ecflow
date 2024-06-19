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

template <>
auto get_option_value<bool>(const boost::program_options::variables_map& vm,
                            const std::string& option_name,
                            const std::string& line) {
    return vm.count(option_name) ? true : false;
}

} // namespace

ecf::MirrorAttr MirrorParser::parse_mirror_line(const std::string& line) {
    return parse_mirror_line(line, nullptr);
}

ecf::MirrorAttr MirrorParser::parse_mirror_line(const std::string& line, const std::string& name) {
    return parse_mirror_line(line, name, nullptr);
}

ecf::MirrorAttr MirrorParser::parse_mirror_line(const std::string& line, const std::string& name, Node* parent) {
    auto updated_line = line + " --name " + name;
    return parse_mirror_line(updated_line, parent);
}

ecf::MirrorAttr MirrorParser::parse_mirror_line(const std::string& line, Node* parent) {
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
    description.add_options()(option_name, po::value<std::string>());
    description.add_options()(option_remote_path, po::value<std::string>());
    description.add_options()(option_remote_host,
                              po::value<std::string>()->default_value(ecf::MirrorAttr::default_remote_host));
    description.add_options()(option_remote_port,
                              po::value<std::string>()->default_value(ecf::MirrorAttr::default_remote_port));
    description.add_options()(option_polling,
                              po::value<std::string>()->default_value(ecf::MirrorAttr::default_polling));
    description.add_options()(option_ssl, "Use SSL");
    description.add_options()(option_remote_auth,
                              po::value<std::string>()->default_value(ecf::MirrorAttr::default_remote_auth));
    description.add_options()(option_reason, po::value<std::string>()->default_value(""));

    po::parsed_options parsed_options = po::command_line_parser(tokens).options(description).run();

    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    auto name        = get_option_value<ecf::MirrorAttr::name_t>(vm, option_name, line);
    auto ecflow_path = get_option_value<ecf::MirrorAttr::remote_path_t>(vm, option_remote_path, line);
    auto ecflow_host = get_option_value<ecf::MirrorAttr::remote_host_t>(vm, option_remote_host, line);
    auto ecflow_port = get_option_value<ecf::MirrorAttr::remote_port_t>(vm, option_remote_port, line);
    auto polling     = get_option_value<ecf::MirrorAttr::polling_t>(vm, option_polling, line);
    auto ssl         = get_option_value<ecf::MirrorAttr::flag_t>(vm, option_ssl, line);
    auto auth        = get_option_value<ecf::MirrorAttr::auth_t>(vm, option_remote_auth, line);
    auto reason      = get_option_value<ecf::MirrorAttr::reason_t>(vm, option_reason, line);

    return ecf::MirrorAttr{parent, name, ecflow_path, ecflow_host, ecflow_port, polling, ssl, auth, reason};
}

bool MirrorParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("MirrorParser::doParse: Could not add 'mirror' as node stack is empty at line: " +
                                 line);
    }

    Node* parent = nodeStack_top();

    auto parsed = parse_mirror_line(line, parent);
    nodeStack_top()->addMirror(parsed);

    return true;
}
