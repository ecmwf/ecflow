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
import io
import ecflow as ecf
import itertools as it

import ecflow_test_util as Test


def to_str(defs):
    buffer = io.StringIO()
    print(defs, file=buffer)
    return buffer.getvalue()


def can_create_aviso_from_parameters():
    aviso = ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth")
    assert aviso.name() == "name"
    assert aviso.listener() == "'listener'"
    assert aviso.url() == "url"
    assert aviso.schema() == "schema"
    assert aviso.polling() == "polling"
    assert aviso.auth() == "auth"


def can_create_aviso_from_default_parameters_0():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "%ECF_AVISO_URL%"
    assert actual.schema() == "%ECF_AVISO_SCHEMA%"
    assert actual.polling() == "%ECF_AVISO_POLLING%"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def can_create_aviso_from_default_parameters_1():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.schema() == "%ECF_AVISO_SCHEMA%"
    assert actual.polling() == "%ECF_AVISO_POLLING%"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def can_create_aviso_from_default_parameters_2():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url", "schema"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.schema() == "schema"
    assert actual.polling() == "%ECF_AVISO_POLLING%"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def can_create_aviso_from_default_parameters_3():
    suite = ecf.Suite("s1")

    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url", "schema", "polling"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.schema() == "schema"
    assert actual.polling() == "polling"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def can_create_aviso_with_listener_details():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    family = ecf.Family("f")
    suite.add_family(family)
    task = ecf.Task('t',
                    ecf.AvisoAttr('aviso',
                                  '{ "event": "dissemination", "request": { "destination": "CL1", "class": "od", "expver": "1", "stream": "oper", "step": [0, 12] } }',
                                  'https://aviso.ecmwf.int',
                                  'schema.json',
                                  '60',
                                  '/.ecmwfapirc'))
    family.add_task(task)

    content = to_str(defs)

    assert f"aviso" in content
    assert f'--listener \'{{ "event": "dissemination", "request": {{ "destination": "CL1", "class": "od", "expver": "1", "stream": "oper", "step": [0, 12] }} }}\'' in content
    assert f"--url https://aviso.ecmwf.int" in content
    assert f"--schema schema.json" in content
    assert f"--polling 60" in content
    assert f"--auth /.ecmwfapirc" in content


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
    assert actual.listener() == "'listener'"
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
    assert actual.listener() == "'listener'"
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
    can_create_aviso_from_default_parameters_0()
    can_create_aviso_from_default_parameters_1()
    can_create_aviso_from_default_parameters_2()
    can_create_aviso_from_default_parameters_3()
    can_create_aviso_with_listener_details()
    can_add_aviso_to_task()
    can_embed_aviso_into_task()
    cannot_have_multiple_avisos_in_single_task()
    can_check_job_creation_with_aviso()

    print("All tests pass")
