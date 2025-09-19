/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_NodeAlgorithms_hpp
#define ecflow_node_NodeAlgorithms_hpp

#include <vector>

class Alias;
class Defs;
class Node;
class Task;

namespace ecf {

// Select Nodes

std::vector<Node*> get_all_nodes(Defs& defs);
std::vector<const Node*> get_all_nodes(const Defs& defs);

std::vector<Node*> get_all_nodes(Node& node);

// Select Tasks

std::vector<Task*> get_all_tasks(Defs& defs);
std::vector<const Task*> get_all_tasks(const Defs& defs);

std::vector<Task*> get_all_tasks(Node& node);
std::vector<const Task*> get_all_tasks(const Node& node);

// Select Alias

std::vector<Alias*> get_all_aliases(Defs& defs);

std::vector<Alias*> get_all_aliases(Node& node);
std::vector<const Alias*> get_all_aliases(const Node& node);

} // namespace ecf

#endif // ecflow_node_NodeAlgorithms_hpp
