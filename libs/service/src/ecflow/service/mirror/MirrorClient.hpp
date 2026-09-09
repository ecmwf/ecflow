/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
    MirrorData()
        : state{0},
          regular_variables{},
          inherited_variables{},
          generated_variables{},
          labels{},
          meters{},
          events{} {}

    explicit MirrorData(int state)
        : state{state},
          regular_variables{},
          inherited_variables{},
          generated_variables{},
          labels{},
          meters{},
          events{} {}

    int state;

    std::vector<Variable> regular_variables;
    std::vector<Variable> inherited_variables;
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
