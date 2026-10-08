# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import io

import pytest

import ecflow as ecf


def _to_str(defs):
    buffer = io.StringIO()
    print(defs, file=buffer)
    return buffer.getvalue()


def test_create_aviso_from_parameters():
    aviso = ecf.AvisoAttr("name", "listener", "url", auth="auth")
    assert aviso.name() == "name"
    assert aviso.listener() == "'listener'"
    assert aviso.url() == "url"
    assert aviso.auth() == "auth"


def test_create_aviso_from_default_parameters_0():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "%ECF_AVISO_URL%"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def test_create_aviso_from_default_parameters_1():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.auth() == "%ECF_AVISO_AUTH%"


def test_create_aviso_rejects_aviso_v1_parameters():
    # The Aviso v1 parameters schema and polling are no longer accepted, neither by keyword nor by position
    with pytest.raises(TypeError):
        ecf.AvisoAttr("name", "listener", "url", schema="schema")
    with pytest.raises(TypeError):
        ecf.AvisoAttr("name", "listener", "url", polling="60")
    with pytest.raises(TypeError):
        ecf.AvisoAttr("name", "listener", "url", "schema")
    with pytest.raises(TypeError):
        ecf.AvisoAttr("name", "listener", "url", "schema", "polling", "auth")


def test_create_aviso_with_listener_details():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    family = ecf.Family("f")
    suite.add_family(family)
    task = ecf.Task(
        "t",
        ecf.AvisoAttr(
            "aviso",
            '{ "event": "dissemination", "request": { "destination": "CL1", "class": "od", "expver": "1", "stream": "oper", "step": [0, 12] } }',
            "https://aviso.ecmwf.int",
            auth="/path/to/auth",
        ),
    )
    family.add_task(task)

    content = _to_str(defs)

    assert "aviso" in content
    assert (
        '--listener \'{ "event": "dissemination", "request": { "destination": "CL1", "class": "od", "expver": "1", "stream": "oper", "step": [0, 12] } }\''
        in content
    )
    assert "--url https://aviso.ecmwf.int" in content
    assert "--auth /path/to/auth" in content
    assert "--schema" not in content
    assert "--polling" not in content


def test_create_aviso_with_collapse():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_task(ecf.Task("a", ecf.AvisoAttr("a", '{ "event": "mars" }', collapse=True)))
    suite.add_task(ecf.Task("b", ecf.AvisoAttr("b", '{ "event": "mars" }')))

    content = _to_str(defs)

    # The option is printed only for the attribute that sets it
    assert content.count("--collapse") == 1


def test_aviso_generated_variables_are_defined():
    task = ecf.Task("t", ecf.AvisoAttr("a", '{ "event": "mars" }'))

    generated = {v.name(): v.value() for v in task.get_generated_variables()}

    assert generated["ECF_AVISO_EVENT_TYPE"] == ""
    assert generated["ECF_AVISO_EVENT_SEQUENCE"] == "0"
    assert generated["ECF_AVISO_EVENT_DATA_IDENTIFIER"] == ""
    assert generated["ECF_AVISO_EVENT_DATA_PAYLOAD"] == ""


def test_aviso_is_rejected_on_family_and_suite():
    # An Aviso attribute is only allowed on a task (or an alias)
    with pytest.raises(RuntimeError):
        ecf.Family("f").add_aviso(ecf.AvisoAttr("name", "listener"))
    with pytest.raises(RuntimeError):
        ecf.Suite("s").add_aviso(ecf.AvisoAttr("name", "listener"))
    with pytest.raises(RuntimeError):
        ecf.Family("f", ecf.AvisoAttr("name", "listener"))


def test_aviso_and_mirror_are_rejected_on_the_same_node():
    # An Aviso attribute is not allowed on a node with a Mirror attribute, and vice versa
    task = ecf.Task("t1")
    task.add_mirror(ecf.MirrorAttr("name", "r_path"))
    with pytest.raises(RuntimeError):
        task.add_aviso(ecf.AvisoAttr("name", "listener"))
    assert len(list(task.avisos)) == 0

    task = ecf.Task("t2")
    task.add_aviso(ecf.AvisoAttr("name", "listener"))
    with pytest.raises(RuntimeError):
        task.add_mirror(ecf.MirrorAttr("name", "r_path"))
    assert len(list(task.mirrors)) == 0


def test_add_aviso_to_task():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1")
    family.add_task(task)

    aviso = ecf.AvisoAttr("name", "listener", "url", auth="auth")
    task.add_aviso(aviso)
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.auth() == "auth"


def test_embed_aviso_into_task():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("f1", ecf.AvisoAttr("name", "listener", "url", auth="auth"))
    assert len(list(task.avisos)) == 1

    actual = list(task.avisos)[0]
    assert actual.name() == "name"
    assert actual.listener() == "'listener'"
    assert actual.url() == "url"
    assert actual.auth() == "auth"


def test_multiple_avisos_in_single_task_is_rejected():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    with pytest.raises(RuntimeError):
        ecf.Task(
            "f1",
            ecf.AvisoAttr("name", "listener", "url", auth="auth"),
            ecf.AvisoAttr("another", "listener", "url", auth="auth"),
        )


def test_check_job_creation_with_aviso():
    defs = ecf.Defs()
    suite = ecf.Suite("s")
    defs.add_suite(suite)

    family = ecf.Family("f")
    suite.add_family(family)

    task = ecf.Task("t")
    family.add_task(task)

    aviso = ecf.AvisoAttr("name", "listener", "url", auth="auth")
    task.add_aviso(aviso)

    defs.check_job_creation()
