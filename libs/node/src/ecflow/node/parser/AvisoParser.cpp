// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/parser/AvisoParser.hpp"

#include <stdexcept>

#include <boost/program_options.hpp>

#include "ecflow/core/Str.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/service/aviso/Aviso.hpp"

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
    return parse_aviso_line(line, parent, false);
}

ecf::AvisoAttr AvisoParser::parse_aviso_line(const std::string& line, Node* parent, bool accept_v1_options) {
    std::vector<std::string> tokens;
    {
        // Since po::command_line_parser requires a vector of strings, we need convert from string_view to string
        std::vector<std::string_view> extracted = ecf::algorithm::split_within_quotes(line, "'");
        std::transform(std::begin(extracted),
                       std::end(extracted),
                       std::back_inserter(tokens),
                       [](const std::string_view& sv) { return std::string{sv}; });
    }

    namespace po = boost::program_options;

    po::options_description description("AvisoParser");
    description.add_options()(option_name, po::value<std::string>());
    description.add_options()(option_listener, po::value<std::string>());
    description.add_options()(option_url, po::value<std::string>()->default_value(ecf::AvisoAttr::default_url));
    description.add_options()(option_v1_schema, po::value<std::string>());
    description.add_options()(option_v1_polling, po::value<std::string>());
    description.add_options()(option_revision, po::value<uint64_t>()->default_value(0));
    description.add_options()(option_auth, po::value<std::string>()->default_value(ecf::AvisoAttr::default_auth));
    description.add_options()(option_reason, po::value<std::string>()->default_value(""));
    description.add_options()(option_collapse, po::bool_switch()->default_value(false));
    description.add_options()(option_event, po::value<std::string>());
    description.add_options()(option_free, po::bool_switch()->default_value(false));

    po::parsed_options parsed_options = po::command_line_parser(tokens).options(description).run();

    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);

    if (!accept_v1_options) {
        for (const auto* option : {option_v1_schema, option_v1_polling}) {
            if (vm.count(option)) {
                throw std::runtime_error(std::string("AvisoParser::doParse: Aviso v1 option '--") + option +
                                         "' is not supported (" + std::string{ecf::service::aviso::unsupported_v1} +
                                         ") in line: " + line);
            }
        }
    }

    auto name     = get_option_value<ecf::AvisoAttr::name_t>(vm, option_name, line);
    auto listener = get_option_value<ecf::AvisoAttr::listener_t>(vm, option_listener, line);
    auto url      = get_option_value<ecf::AvisoAttr::url_t>(vm, option_url, line);
    auto revision = get_option_value<ecf::AvisoAttr::revision_t>(vm, option_revision, line);
    auto auth     = get_option_value<ecf::AvisoAttr::auth_t>(vm, option_auth, line);
    auto reason   = get_option_value<ecf::AvisoAttr::reason_t>(vm, option_reason, line);
    auto collapse = get_option_value<bool>(vm, option_collapse, line);

    auto aviso = ecf::AvisoAttr{parent, name, listener, url, revision, auth, reason, collapse};
    if (vm.count(option_event)) {
        aviso.set_event(ecf::AvisoEvent::from_option(vm[option_event].as<std::string>()));
    }
    if (get_option_value<bool>(vm, option_free, line)) {
        aviso.setFree();
    }
    return aviso;
}

bool AvisoParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    if (nodeStack().empty()) {
        throw std::runtime_error("AvisoParser::doParse: Could not add aviso as node stack is empty at line: " + line);
    }

    Node* parent = nodeStack_top();

    // Definitions that carry state (e.g. checkpoints written by an earlier version of ecFlow) may hold Aviso v1
    // options, which are accepted and ignored
    bool accept_v1_options = rootParser()->get_file_type() != PrintStyle::DEFS;

    auto parsed = parse_aviso_line(line, parent, accept_v1_options);
    nodeStack_top()->addAviso(parsed);
    nodeStack_top()->absNodePath();

    return true;
}
