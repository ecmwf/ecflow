#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import os
import ecflow as ecf
import itertools as it

import ecflow_test_util as Test


def can_create_aviso_from_parameters():
    aviso = ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth")
    assert aviso.name() == "name"
    assert aviso.listener() == "listener"
    assert aviso.url() == "url"
    assert aviso.schema() == "schema"
    assert aviso.polling() == "polling"
    assert aviso.auth() == "auth"


def can_add_aviso_to_task():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1")
    family.add_task(task)

    aviso = ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth")
    task.add_aviso(aviso)
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "listener"
    assert actual.url() == "url"
    assert actual.schema() == "schema"
    assert actual.polling() == "polling"
    assert actual.auth() == "auth"


def can_embed_aviso_into_task():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "listener"
    assert actual.url() == "url"
    assert actual.schema() == "schema"
    assert actual.polling() == "polling"
    assert actual.auth() == "auth"


def cannot_have_multiple_avisos_in_single_task():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    try:
        task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth"),
                        ecf.AvisoAttr("another", "listener", "url", "schema", "polling", "auth"))
        assert False, "Expected exception indicating multiple avisos are not allowed in a task"
    except RuntimeError as e:
        assert True


def can_check_job_creation_with_aviso():
    defs = ecf.Defs()

    suite = ecf.Suite("s")
    defs.add_suite(suite)

    family = ecf.Family("f")
    suite.add_family(family)

    task = ecf.Task("t")
    family.add_task(task)

    aviso = ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth")
    task.add_aviso(aviso)

    defs.check_job_creation()


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    can_create_aviso_from_parameters()
    can_add_aviso_to_task()
    can_embed_aviso_into_task()
    cannot_have_multiple_avisos_in_single_task()
    can_check_job_creation_with_aviso()

    print("All tests pass")
