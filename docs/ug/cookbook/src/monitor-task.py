# Copyright 2009-2019 ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.

import sys
import time

import ecflow


def monitor_critical_task(ci, path_to_task):

    # Query the server for any changes
    if ci.news_local():

        # get the incremental changes, and merge with defs stored on the Client
        ci.sync_local()

        # check to see if definition exists in the server
        defs = ci.get_defs()
        if len(defs) == 0:
            sys.exit(0)  # return server has no suites

        # find the task we are interested in
        critical_task = defs.find_abs_node(path_to_task)
        if critical_task is None:
            # No such task
            sys.exit(0)  # return

        # Check to see if task was aborted, if it was email me the job output
        if critical_task.get_state() == ecflow.State.aborted:

            # Get the job output
            the_aborted_task_output = ci.get_file(path_to_task, "jobout")
            # email(the_aborted_task_output)
            sys.exit(0)


try:
    # Create the client. This will read the default environment variables
    ci = ecflow.Client("localhost", "4143")

    # Continually monitor the suite
    while 1:

        monitor_critical_task(ci, "/suite/critical_node")

        # Sleep for 5 minutes.
        # To avoid overloading server ensure sleep is > 60 seconds
        time.sleep(300)

except RuntimeError as e:
    print(str(e))
