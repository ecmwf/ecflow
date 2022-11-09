# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.

import sys

import ecflow

try:
    # Create the client
    ci = ecflow.Client("localhost", "4143")

    # Get the node tree suite definition as stored in the server
    # The definition is retrieved and stored on the variable 'ci'
    ci.sync_local()

    # access the definition retrieved from the server
    defs = ci.get_defs()

    if defs is None or len(defs) == 0:
        print("The server has no suites")
        sys.exit(1)

    # get the tasks, *alternatively* could use defs.get_all_nodes()
    # to include suites, families and tasks.
    task_vec = defs.get_all_tasks()

    # iterate over tasks and print path and state
    for task in task_vec:
        print(task.get_abs_node_path() + " " + str(task.get_state()))

except RuntimeError as e:
    print("Failed: " + str(e))
