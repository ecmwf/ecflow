// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/ArchivedNodes.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"

namespace ecf {

namespace {

// Leaves the change numbers of the process untouched while it lives: parsing an archive is not a change
class Unchanged {
public:
    Unchanged()
        : server_{Ecf::server()},
          state_change_no_{Ecf::state_change_no()},
          modify_change_no_{Ecf::modify_change_no()} {
        Ecf::set_server(false);
    }
    Unchanged(const Unchanged&)            = delete;
    Unchanged& operator=(const Unchanged&) = delete;
    ~Unchanged() {
        Ecf::set_state_change_no(state_change_no_);
        Ecf::set_modify_change_no(modify_change_no_);
        Ecf::set_server(server_);
    }

private:
    bool server_;
    Ecf::counter_t state_change_no_;
    Ecf::counter_t modify_change_no_;
};

// Returns false when an archive could not be read
bool walk(const Node& node,
          const std::string& home,
          const std::string& port,
          const std::function<void(const Node&)>& visit) {
    visit(node);

    if (const auto* container = node.isNodeContainer();
        container && container->get_flag().is_set(Flag::ARCHIVED) && container->children().empty()) {
        bool complete = true;
        try {
            // The nodes read from an archive sit under shallow ancestors, without the ECF_HOME of the live ones
            std::string own_home;
            const auto& where =
                node.findParentUserVariableValue(ecf::environment::ECF_HOME, own_home) ? own_home : home;
            std::string name = node.absNodePath();
            std::replace(name.begin(), name.end(), '/', ':');
            const auto path = where + "/" + Host{}.prefix_host_and_port(port, name + ".check");
            if (!fs::exists(path)) {
                throw std::runtime_error("no archive file " + path);
            }
            defs_ptr archive;
            {
                const Unchanged unchanged;
                archive = Defs::create();
                archive->restore(path);
                // The nested archives resolve ECF_HOME as the live nodes do, not from the default of the scratch
                // definitions
                archive->server_state().add_or_update_server_variable(ecf::environment::ECF_HOME, where);
            }
            const auto archived = archive->findAbsNode(node.absNodePath());
            if (!archived || !archived->isNodeContainer()) {
                // NodeContainer::restore() would refuse it too
                throw std::runtime_error("the archive file " + path + " does not hold the container " +
                                         node.absNodePath());
            }
            std::vector<node_ptr> children;
            archived->immediateChildren(children);
            for (const auto& child : children) {
                complete = walk(*child, where, port, visit) && complete;
            }
        }
        catch (const std::exception& e) {
            ecf::log(Log::WAR, "permissions: the archive of " + node.absNodePath() + " could not be read: " + e.what());
            return false;
        }
        return complete;
    }

    bool complete = true;
    std::vector<node_ptr> children;
    node.immediateChildren(children);
    for (const auto& child : children) {
        complete = walk(*child, home, port, visit) && complete;
    }
    return complete;
}

} // namespace

bool for_each_node(const Node& subtree, const std::function<void(const Node&)>& visit) {
    std::string home;
    (void)subtree.findParentUserVariableValue(ecf::environment::ECF_HOME, home);
    std::string port;
    if (const Defs* defs = subtree.defs(); defs) {
        port = defs->server_state().find_variable(ecf::environment::ECF_PORT);
    }
    if (port.empty()) {
        port = ecf::string_constants::default_port_number;
    }
    return walk(subtree, home, port, visit);
}

bool carries_permissions(const Node& node) {
    bool found          = false;
    const bool complete = for_each_node(node, [&found](const Node& n) { found = found || n.permissions() != nullptr; });
    return found || !complete; // an archive that cannot be read counts as carrying attributes (fail closed)
}

} // namespace ecf
