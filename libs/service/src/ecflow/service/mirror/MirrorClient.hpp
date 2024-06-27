/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_mirror_MirrorClient_HPP
#define ecflow_service_mirror_MirrorClient_HPP

#include <memory>
#include <string>
#include <vector>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/attribute/Variable.hpp"

namespace ecf::service::mirror {

struct MirrorData
{
    MirrorData() : state{0}, regular_variables{}, generated_variables{}, labels{}, meters{}, events{} {}

    explicit MirrorData(int state)
        : state{state},
          regular_variables{},
          generated_variables{},
          labels{},
          meters{},
          events{} {}

    int state;

    std::vector<Variable> regular_variables;
    std::vector<Variable> generated_variables;

    std::vector<Label> labels;
    std::vector<Meter> meters;
    std::vector<Event> events;
};

class MirrorClient {
public:
    MirrorClient();
    ~MirrorClient();

    MirrorData get_node_status(const std::string& remote_host,
                               const std::string& remote_port,
                               const std::string& node_path,
                               bool ssl,
                               const std::string& remote_username,
                               const std::string& remote_password) const;

private:
    struct Impl;
    mutable std::unique_ptr<Impl> impl_;
};

} // namespace ecf::service::mirror

#endif /* ecflow_service_mirror_MirrorClient_HPP */
