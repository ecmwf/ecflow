/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_Revisions_HPP
#define aviso_Revisions_HPP

#include <fstream>

#include <nlohmann/json.hpp>

namespace aviso {

class Revisions {
public:
    using revision_t = int64_t;

    Revisions() : json_(load_revisions(get_default_location())){};

    void add(std::string_view path, std::string_view scheme_host_port, revision_t revision) {
        json_[std::string{path}][std::string{scheme_host_port}] = revision;
    }

    void store() { store_revisions(get_default_location(), json_); }

    revision_t get_revision(std::string_view path, std::string_view scheme_host_port) const {
        // find by path
        auto it1 = json_.find(std::string{path});
        if (it1 == json_.end()) {
            return 0;
        }
        // find by scheme_host_port
        auto it2 = it1->find(std::string{scheme_host_port});
        if (it2 == it1->end()) {
            return 0;
        }
        return it2->get<revision_t>();
    }

private:
    static nlohmann::json load_revisions(const std::string& location) {

        std::cout << "load_revisions: " << location << std::endl;
        std::ifstream in(location);
        if (!in) {
            nlohmann::json json = {};
        }
        nlohmann::json json;
        try {
            in >> json;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cout << "Failed to parse json: " << e.what() << ". Resetting revision cache..." << std::endl;
            nlohmann::json json = {};
        }
        return json;
    }

    static void store_revisions(const std::string& location, const nlohmann::json& json) {

        std::cout << "store_revisions: " << location << std::endl;
        std::ofstream out(location);
        if (!out) {
            throw std::runtime_error("Could not open file for writing: " + location);
        }
        out << json.dump(2);
    }

    static std::string get_default_location() { return "ecflow_revisions.json"; }

private:
    nlohmann::json json_;
};

} // namespace aviso

#endif /* aviso_Revisions_HPP */
