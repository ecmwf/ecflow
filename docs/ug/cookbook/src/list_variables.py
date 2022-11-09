# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.

import ecflow
import os
import sys
import argparse  # for argument parsing


def list_used_variables(node, ARGS):
    # make a list of all nodes up the parent hierarchy
    parent_hierarchy = []
    parent_hierarchy.append(node)
    parent = node.get_parent()
    while parent:
        parent_hierarchy.append(parent)
        parent = parent.get_parent()

    # now reverse the list
    parent_hierarchy.reverse()

    # now create a map of all used variables going down the hierarchy.
    # Allow variable lower down the hierarchy to override parent variables
    variable_map = {}
    for node in parent_hierarchy:
        for var in node.variables:
            variable_map[var.name()] = var.value()

    # finally print the used variables
    if ARGS.var_name:
        if ARGS.not_value:
            # use exact match for key
            for key in variable_map:
                if ARGS.var_name == key:
                    if ARGS.not_value != variable_map[key]:
                        print("edit " + key + " '" + variable_map[key] + "'")
        else:
            # use substring match for variable name
            for key in variable_map:
                if ARGS.var_name in key:
                    print("edit " + key + " '" + variable_map[key] + "'")
    else:
        for key in variable_map:
            print("edit " + key + " '" + variable_map[key] + "'")


def get_host():
    host = os.getenv("ECF_NODE")
    if not host:
        host = "localhost"
    return host


if __name__ == "__main__":

    DESC = """List variables below a given path for a given task name,
              - optionally match variable name
              - optionally report when the value does match.
           Usage:
           Example1: List all the variables used by task fred, below path /suite/x
                     ./list_variables.py --host cca --port 4141 --path /suite/x --task fred
           Example2: List all the variables used by task pdb, below path /emc_41r2 and which match PGNODES
                     ./list_variables.py --host vsms2 --port 43333 --path /emc_41r2 --task pdb --var_name PGNODES
           Example3: List all the variables used by task prodgen, below path /emc_41r2 and which match PGNODES
                     and where the variable value does *NOT* match '6'
                     ./list_variables.py --host vsms2 --port 43333 --path /emc_41r2 --task prodgen --var_name PGNODES --not_value 6
            """
    PARSER = argparse.ArgumentParser(
        description=DESC, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    PARSER.add_argument(
        "--host",
        default=get_host(),
        help="The name of the host machine. Can be obtained from the ecflow GUI",
    )
    PARSER.add_argument(
        "--port",
        default=os.getenv("ECF_PORT"),
        help="The port on the host, Can be obtained from the ecflow GUI",
        type=int,
    )
    PARSER.add_argument(
        "--path",
        required=True,
        help="The path to start listing of variables below. i.e. /emc_41r2",
    )
    PARSER.add_argument(
        "--task",
        required=True,
        help="The name of the task for which we want to list variables",
    )
    PARSER.add_argument(
        "--var_name",
        help="Optionally match with the variable name. Uses exact match when using --not_value",
    )
    PARSER.add_argument(
        "--not_value",
        help="report if variable value does not match the provided value.Only works when --var_name specified",
    )
    ARGS = PARSER.parse_args()
    print(ARGS)

    # ===========================================================================
    CL = ecflow.Client(ARGS.host, ARGS.port)
    try:
        CL.ping()

        # get the incremental changes, and merge with defs stored on the Client
        CL.sync_local()

        # check to see if definition exists in the server
        defs = CL.get_defs()
        if len(defs) == 0:
            print("No suites found, exiting...")
            sys.exit(0)

        selected_node = defs.find_abs_node(ARGS.path)
        if selected_node == None:
            print("No node found at path " + ARGS.path)
            sys.exit(0)

        # now get all task of the given name below this path
        node_vec = selected_node.get_all_nodes()
        for node in node_vec:
            if not isinstance(node, ecflow.Task):
                continue
            if ARGS.task in node.name():
                print("====================== ", node.get_abs_node_path())
                list_used_variables(node, ARGS)

    except RuntimeError as ex:
        print("Error: " + str(ex))
        print("Check host and port number are correct.")
