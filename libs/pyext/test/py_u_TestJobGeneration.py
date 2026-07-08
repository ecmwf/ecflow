#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that job generation works as expected.

import os
import shutil

import pytest

from ecflow import Defs, JobCreationCtrl, File, Client


@pytest.fixture
def workspace_dir():
    return File.source_dir()


@pytest.fixture
def ecf_home(workspace_dir):
    return os.path.join(workspace_dir, "libs", "pyext", "test", "data", "ECF_HOME")


@pytest.fixture
def suite_name():
    return "suite_job_gen_" + str(os.getpid())


@pytest.fixture
def task_vec_and_defs(ecf_home, suite_name):
    defs = Defs()
    suite = defs.add_suite(suite_name)
    suite.add_variable("ECF_HOME", ecf_home)
    suite.add_variable("ECF_CLIENT_EXE_PATH", File.find_client())
    suite.add_variable("SLEEPTIME", "1")
    suite.add_variable(
        "ECF_INCLUDE",
        os.path.join(File.source_dir(), "libs", "pyext", "test", "data", "includes"),
    )

    fam = suite.add_family("family")
    t1 = fam.add_task("t1")
    t1.add_event("eventname")
    t1.add_meter("meter", 0, 100, 100)

    t2 = fam.add_task("t2")
    t3 = fam.add_task("t3")
    t4 = fam.add_task("dummy_task")
    t4.add_variable(
        "ECF_DUMMY_TASK", "any"
    )  # stop job generation errors for tasks without an .ecf

    task_vec = [t1, t2, t3, t4]

    # Create dir. **NOTE**, if the second component of os.path.join(..) is absolute,
    # the first path is ignored, hence remove first char
    dir_to_create = os.path.join(str(ecf_home), fam.get_abs_node_path()[1:])
    os.makedirs(dir_to_create, exist_ok=True)

    # copy ecf files from ecflow source, to create unique file per process
    src_dir = os.path.join(ecf_home, "suite_job_gen", "family")
    for task in (t1, t2, t3):
        src = os.path.join(src_dir, task.name() + ".ecf")
        dest = os.path.join(ecf_home, task.get_abs_node_path()[1:] + ".ecf")
        shutil.copy(src, dest)

    yield task_vec, defs

    # cleanup
    _delete_jobs(task_vec, ecf_home)
    _delete_suite_dir(ecf_home, suite_name)


def _delete_jobs(task_vec, ecf_home):
    for task in task_vec:
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
        if os.path.exists(the_job_file):
            try:
                os.remove(the_job_file)
            except OSError:
                pass
        man_file = ecf_home + task.get_abs_node_path() + ".man"
        if os.path.exists(man_file):
            try:
                os.remove(man_file)
            except OSError:
                pass


def _check_jobs(task_vec, ecf_home):
    for task in task_vec:
        variable = task.find_variable("ECF_DUMMY_TASK")
        if not variable.empty():
            continue
        the_job_file = ecf_home + task.get_abs_node_path() + ".job" + task.get_try_no()
        assert os.path.exists(the_job_file), "Could not find job file " + the_job_file


def _delete_suite_dir(ecf_home, name):
    dir_to_del = os.path.join(ecf_home, name)
    if os.path.exists(dir_to_del):
        shutil.rmtree(dir_to_del, ignore_errors=True)


def test_job_creation_default_location(task_vec_and_defs, ecf_home):
    task_vec, defs = task_vec_and_defs

    print(defs.check_job_creation())
    _check_jobs(task_vec, ecf_home)


def test_job_creation_with_job_creation_ctrl(task_vec_and_defs, ecf_home):
    task_vec, defs = task_vec_and_defs

    job_ctrl = JobCreationCtrl()
    defs.check_job_creation(job_ctrl)
    print(job_ctrl.get_error_msg())
    _check_jobs(task_vec, ecf_home)


def test_job_creation_under_node_path(task_vec_and_defs, ecf_home):
    task_vec, defs = task_vec_and_defs

    job_ctrl = JobCreationCtrl()
    job_ctrl.set_node_path(task_vec[0].get_abs_node_path())
    defs.check_job_creation(job_ctrl)
    print(job_ctrl.get_error_msg())
    # jobs under the specified path will have been created; clean them up
    _delete_jobs(task_vec, ecf_home)


def test_job_creation_to_specified_directory(
    task_vec_and_defs, workspace_dir, suite_name
):
    task_vec, defs = task_vec_and_defs

    job_ctrl = JobCreationCtrl()
    job_ctrl.set_dir_for_job_creation(
        os.path.join(workspace_dir, "libs", "pyext", "test", "data")
    )
    defs.check_job_creation(job_ctrl)
    print(job_ctrl.get_error_msg())

    generated_dir = os.path.join(job_ctrl.get_dir_for_job_creation(), suite_name)
    assert os.path.exists(generated_dir), "Expected generated job directory to exist"
    shutil.rmtree(generated_dir)


def test_job_creation_to_temp_directory(task_vec_and_defs, ecf_home):
    task_vec, defs = task_vec_and_defs

    # When run via rsh then TMPDIR may not be defined, hence expect exception to be thrown
    try:
        job_ctrl = JobCreationCtrl()
        job_ctrl.generate_temp_dir()
        defs.check_job_creation(job_ctrl)
        print(job_ctrl.get_error_msg())
        shutil.rmtree(job_ctrl.get_dir_for_job_creation(), ignore_errors=True)
        _delete_jobs(task_vec, ecf_home)
    except RuntimeError as e:
        pytest.skip(f"Could not generate temp dir: {e}")
