#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures the Task API works as expected.

import time
import os
import pwd
import sys  # determine python version
from datetime import datetime
import shutil  # used to remove directory tree

import pytest

from ecflow import (
    Defs,
    Clock,
    DState,
    Event,
    Style,
    State,
    PrintStyle,
    File,
    Client,
    SState,
    File,
    debug_build,
)
import ecflow_test_util as Test


def ecf_includes():
    return File.source_dir() + "/libs/pyext/test/data/python_includes"


def create_defs(name, port, protocol):
    defs = Defs()
    suite_name = name
    if len(suite_name) == 0:
        suite_name = "s1"
    suite = defs.add_suite(suite_name)

    ecfhome = Test.ecf_home(port)
    suite.add_variable("ECF_HOME", ecfhome)
    suite.add_variable("ECF_INCLUDE", ecf_includes())
    protocol_option = "True" if protocol == Test.Protocol.HTTP else "False"
    suite.add_variable("ECF_USING_HTTP_BACKEND", protocol_option)

    # Setup the job command, based on the current environment
    job_cmd = ""
    if "PYTHONPATH" in os.environ:
        job_cmd = "export PYTHONPATH=" + os.environ["PYTHONPATH"] + ";"
    if "LD_LIBRARY_PATH" in os.environ:
        job_cmd += "export LD_LIBRARY_PATH=" + os.environ["LD_LIBRARY_PATH"] + ";"
    # Use the current python interpreter to run the job
    job_cmd += f"{sys.executable} %ECF_JOB% 1> %ECF_JOBOUT% 2>&1"

    suite.add_variable("ECF_JOB_CMD", job_cmd)

    family = suite.add_family("f1")
    t1 = family.add_task("t1")
    t1.add_event("event_fred")
    t1.add(Event("event_set", True))  # ECFLOW-1526
    t1.add_meter("meter", 0, 100)
    t1.add_label("label_name", "value")
    t1.add_queue("q1", ["1", "2", "3"])

    family.add_task("t2")  # test wait
    family.add_task("t3").add_trigger(
        "t1:q1 >= 3 and t1:event_fred and t1:event_set == clear"
    )  # wait on queue q1 and events
    family.add_task("t4").add_trigger(
        "t1:name1 == 1 and t1:name2 == 2 and t1:name3 == 3 and t1:name4 == 4"
    )  # test ECFLOW-1573

    defs.auto_add_externs(
        True
    )  # because variable name1,name2,name3,name4  are not added until t1 is active.(i.e. runtime)
    return defs


def wait_for_suite_to_complete(ci, suite_name):
    count = 0
    while 1:
        print(f"Waiting for suite {suite_name} to complete, loop {count}...")
        count += 1
        ci.sync_local()  # get the changes, synced with local defs
        suite = ci.get_defs().find_suite(suite_name)
        assert suite is not None, (
            " Expected to find suite " + suite_name + ":\n" + str(ci.get_defs())
        )
        if suite.get_state() == State.complete:
            break
        if suite.get_state() == State.aborted:
            print(ci.get_defs())
            assert False, " Suite aborted \n"
        time.sleep(2)
        if count > 20:
            assert False, (
                suite_name
                + " aborted after "
                + str(count)
                + " loops, printing defs:\n"
                + str(ci.get_defs())
            )

    ci.log_msg("Looped " + str(count) + " times")


@pytest.fixture(
    params=[Test.Protocol.CUSTOM, Test.Protocol.HTTP], ids=["custom", "http"]
)
def protocol(request):
    return request.param


@pytest.fixture
def server(protocol):
    with Test.Server(protocol) as ctx:
        yield ctx[0], ctx[1]


def test_python_child_api(server):
    ci, protocol = server
    server_version = ci.server_version()
    print("Running ecflow server version " + server_version)
    print("Running ecflow client version " + ci.version())
    assert ci.version() == server_version, "Client version not same as server version"

    PrintStyle.set_style(Style.STATE)  # show node state
    suite_name = "test_python_child_api"
    host = ci.get_host()
    port = ci.get_port()

    ecf_home = Test.ecf_home(port)
    test_home = ecf_home + "/" + suite_name
    family_dir = test_home + "/f1"

    # Make the directory tree for the suite
    if not os.path.exists(family_dir):
        os.makedirs(family_dir)

    # Dump some information
    print("\n" + suite_name + " " + host + ":" + str(port))
    print(" ECF_HOME(" + ecf_home + ")")
    print(" ECF_INCLUDES(" + ecf_includes() + ")")

    # Clear the server
    ci.delete_all(True)

    # Setup defs on server (and a copy on disk)
    defs = create_defs(suite_name, port, protocol)
    suite = defs.find_suite(suite_name)
    suite.add_defstatus(DState.suspended)
    defs.save_as_defs(
        os.path.join(test_home, suite_name + ".def")
    )  # .../<suite-name>/<suite-name>.def

    # Set the log file to a location inside the test
    ci.new_log(
        os.path.join(test_home, suite_name + ".log")
    )  # .../<suite-name>/<suite-name>.log

    server_version = ci.server_version()

    if ci.version() != server_version:
        assert False, "Client and server versions different"

    # Create the Task script at .../<suite-name>/f1/t1
    file = family_dir + "/t1.ecf"
    contents = f"""
%include <head.py>

with Client(True) as ci:

    {'ci.enable_http()' if protocol == Test.Protocol.HTTP else ''}
    print('   doing some work: t1.ecf')
    ci.child_event('event_fred')      # set the event
    ci.child_event('event_set',False) # clear the event ECFLOW-1526
    ci.child_meter('meter',100)
    ci.child_label('label_name','100')
    step = ci.child_queue('q1','active')
    assert step == '1','expected first step to be 1'
    step = ci.child_queue('q1','complete',step)
    step = ci.child_queue('q1','active')
    assert step == '2','expected second step to be 2'
    step = ci.child_queue('q1','complete',step)
    step = ci.child_queue('q1','active')
    assert step == '3','expected third step to be 3'
    step = ci.child_queue('q1','complete',step)
    step = ci.child_queue('q1','active')
    assert step == '<NULL>','expected <NULL? for end of queue'
    print('   Finished event,meter,label and queue child commands')
"""
    open(file, "w").write(contents)
    print(" Created file " + file)

    # Create the Task script at /<suite-name>/f1/t2
    file = family_dir + "/t2.ecf"
    contents = f"""
%include <head.py>

with Client() as ci:

    {'ci.enable_http()' if protocol == Test.Protocol.HTTP else ''}
    print('   Waiting for /{suite_name}/f1/t1 == complete')
    ci.child_wait('/{suite_name}/f1/t1 == complete')
    print('   Finished waiting')
"""
    open(file, "w").write(contents)
    print(" Created file " + file)

    # Create the Task script at .../<suite-name>/f1/t3
    file = family_dir + "/t3.ecf"
    contents = f"""
%include <head.py>

with Client() as ci:
    print('   Running t3.ecf')
"""
    open(file, "w").write(contents)
    print(" Created file " + file)

    # Create the Task script at .../<suite-name>/f1/t4
    file = family_dir + "/t4.ecf"
    contents = """
%include <head.py>

with Client() as ci:
    print('   Running t4.ecf')
"""
    open(file, "w").write(contents)
    print(" Created file " + file)

    # Start the server
    ci.restart_server()

    # Load the definitions
    ci.load(defs)
    ci.checkpt()  # store the checkpoint, useful for debugging...

    # Start the merry-go-round!...
    ci.begin_all_suites()

    print(" Running the test, wait for suite to complete ...")
    ci.run(f"/{suite_name}", False)

    wait_for_suite_to_complete(ci, suite_name)

    ci.checkpt()  # store the checkpoint, useful for debugging...

    if not Test.debugging():
        print(" Test OK: removing directory ", test_home)
        shutil.rmtree(test_home, ignore_errors=True)
