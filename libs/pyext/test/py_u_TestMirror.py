#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import pytest

import ecflow as ecf


def test_create_mirror_from_parameters():
    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth"
    )
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "r_port"
    assert mirror.polling() == "polling"
    assert mirror.ssl() is True
    assert mirror.auth() == "auth"


def test_create_mirror_from_default_parameters_0():
    mirror = ecf.MirrorAttr("name", "r_path")
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "%ECF_MIRROR_REMOTE_HOST%"
    assert mirror.remote_port() == "%ECF_MIRROR_REMOTE_PORT%"
    assert mirror.polling() == "%ECF_MIRROR_REMOTE_POLLING%"
    assert mirror.ssl() is False
    assert mirror.auth() == "%ECF_MIRROR_REMOTE_AUTH%"


def test_create_mirror_from_default_parameters_1():
    mirror = ecf.MirrorAttr("name", "r_path", "r_host")
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "%ECF_MIRROR_REMOTE_PORT%"
    assert mirror.polling() == "%ECF_MIRROR_REMOTE_POLLING%"
    assert mirror.ssl() is False
    assert mirror.auth() == "%ECF_MIRROR_REMOTE_AUTH%"


def test_create_mirror_from_default_parameters_2():
    mirror = ecf.MirrorAttr("name", "r_path", "r_host", "r_port")
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "r_port"
    assert mirror.polling() == "%ECF_MIRROR_REMOTE_POLLING%"
    assert mirror.ssl() is False
    assert mirror.auth() == "%ECF_MIRROR_REMOTE_AUTH%"


def test_create_mirror_from_default_parameters_3():
    mirror = ecf.MirrorAttr("name", "r_path", "r_host", "r_port", "polling")
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "r_port"
    assert mirror.polling() == "polling"
    assert mirror.ssl() is False
    assert mirror.auth() == "%ECF_MIRROR_REMOTE_AUTH%"


def test_create_mirror_from_default_parameters_4():
    mirror = ecf.MirrorAttr("name", "r_path", "r_host", "r_port", "polling", True)
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "r_port"
    assert mirror.polling() == "polling"
    assert mirror.ssl() is True
    assert mirror.auth() == "%ECF_MIRROR_REMOTE_AUTH%"


def test_create_mirror_propagate_defaults_to_false():
    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth"
    )
    assert mirror.propagate() is False


def test_create_mirror_with_propagate_as_positional_argument():
    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth", True
    )
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.remote_host() == "r_host"
    assert mirror.remote_port() == "r_port"
    assert mirror.polling() == "polling"
    assert mirror.ssl() is True
    assert mirror.auth() == "auth"
    assert mirror.propagate() is True


def test_create_mirror_with_propagate_as_keyword_argument():
    mirror = ecf.MirrorAttr("name", "r_path", propagate=True)
    assert mirror.name() == "name"
    assert mirror.remote_path() == "r_path"
    assert mirror.ssl() is False
    assert mirror.propagate() is True


def test_mirror_str_reflects_ssl_and_propagate_flags():
    enabled = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth", True
    )
    assert "ssl=1" in str(enabled)
    assert "propagate=1" in str(enabled)

    disabled = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", False, "auth", False
    )
    assert "ssl=0" in str(disabled)
    assert "propagate=0" in str(disabled)


def test_add_mirror_with_propagate_to_task():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("t1")
    family.add_task(task)

    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth", True
    )
    task.add_mirror(mirror)
    assert len(list(task.mirrors)) == 1

    actual = list(task.mirrors)[0]
    assert actual.name() == "name"
    assert actual.propagate() is True


def test_add_mirror_to_task():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task("t1")
    family.add_task(task)

    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth"
    )
    task.add_mirror(mirror)
    assert len(list(task.mirrors)) == 1

    actual = list(task.mirrors)[0]
    assert actual.name() == "name"
    assert actual.remote_path() == "r_path"
    assert actual.remote_host() == "r_host"
    assert actual.remote_port() == "r_port"
    assert actual.ssl() is True
    assert actual.auth() == "auth"


def test_embed_mirror_into_task():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    task = ecf.Task(
        "f1",
        ecf.MirrorAttr("name", "r_path", "r_host", "r_port", "polling", True, "auth"),
    )
    assert len(list(task.mirrors)) == 1

    actual = list(task.mirrors)[0]
    assert actual.name() == "name"
    assert actual.remote_path() == "r_path"
    assert actual.remote_host() == "r_host"
    assert actual.remote_port() == "r_port"
    assert actual.ssl() is True
    assert actual.auth() == "auth"


def test_multiple_mirrors_in_single_task_is_rejected():
    suite = ecf.Suite("s1")
    family = ecf.Family("f1")
    suite.add_family(family)

    with pytest.raises(RuntimeError):
        ecf.Task(
            "f1",
            ecf.MirrorAttr(
                "name", "r_path", "r_host", "r_port", "polling", True, "auth"
            ),
            ecf.MirrorAttr(
                "another", "r_path", "r_host", "r_port", "polling", True, "auth"
            ),
        )


def test_check_job_creation_with_mirror():
    defs = ecf.Defs()
    suite = ecf.Suite("s")
    defs.add_suite(suite)

    family = ecf.Family("f")
    suite.add_family(family)

    task = ecf.Task("t")
    family.add_task(task)

    mirror = ecf.MirrorAttr(
        "name", "r_path", "r_host", "r_port", "polling", True, "auth"
    )
    task.add_mirror(mirror)

    defs.check_job_creation()
