# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# This test ensures the SSL setup for the ecFlow client works as expected.

import os
from contextlib import ExitStack

import pytest

import ecflow_test_util as Test
import ecflow as ecf

port = 31415
host = os.uname()[1]


@pytest.fixture(autouse=True)
def isolated_ssl_directory(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv("ECF_SSL_DIR", str(tmp_path) + os.sep)


@pytest.mark.parametrize("mode", ["", "1", "X", "other"])
@pytest.mark.parametrize(
    "specific,shared", [(True, True), (True, False), (False, True), (False, False)]
)
@pytest.mark.parametrize("explicit", [False, True])
def test_certificate_selection_strategies(
    mode, specific, shared, explicit, monkeypatch
):
    specific_crt = f"{host}.{port}.crt"
    monkeypatch.delenv("ECF_SSL", raising=False)

    with ExitStack() as files:
        if specific:
            files.enter_context(Test.MockFile(specific_crt))
        if shared:
            files.enter_context(Test.MockFile("server.crt"))

        client = ecf.Client(host, port) if explicit else None
        monkeypatch.setenv("ECF_SSL", mode)

        def enable():
            if explicit:
                client.enable_ssl()
                return client
            return ecf.Client(host, port)

        if not specific and not shared:
            with pytest.raises(RuntimeError):
                enable()
        else:
            expected = (
                specific_crt
                if specific and (mode not in ("", "1") or not shared)
                else "server.crt"
            )
            assert enable().get_certificate() == expected


def test_setup_client_without_ssl_and_get_empty_certificate():
    expected_crt = ""

    client = ecf.Client(host, port)
    crt = client.get_certificate()
    assert (
        crt is expected_crt
    ), f"Expected certificate to be '{expected_crt}', but got '{crt}'"


def test_setup_client_with_ssl_with_shared_certificate_solely_based_envvars_and_ecf_ssl_empty():
    expected_crt = f"server.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment(
            {"ECF_SSL": "", "ECF_HOST": host, "ECF_PORT": str(port)}
        ) as env:
            client = ecf.Client()
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_shared_certificate_solely_based_envvars():
    expected_crt = f"server.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment(
            {"ECF_SSL": "1", "ECF_HOST": host, "ECF_PORT": str(port)}
        ) as env:
            client = ecf.Client()
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_requesting_shared_but_finding_specific_certificate_solely_based_envvars():
    expected_crt = f"{host}.{port}.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment(
            {"ECF_SSL": "1", "ECF_HOST": host, "ECF_PORT": str(port)}
        ) as env:
            client = ecf.Client()
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_specific_certificate_solely_based_envvars():
    expected_crt = f"{host}.{port}.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment(
            {"ECF_SSL": "X", "ECF_HOST": host, "ECF_PORT": str(port)}
        ) as env:
            client = ecf.Client()
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_shared_certificate_using_envvar():
    expected_crt = f"server.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment({"ECF_SSL": "1"}) as env:
            client = ecf.Client(host, port)
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_shared_certificate_using_empty_envvar():
    expected_crt = f"server.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment({"ECF_SSL": ""}) as env:
            client = ecf.Client(host, port)
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_shared_certificate_using_explicit_option():
    expected_crt = f"server.crt"

    with Test.MockFile(expected_crt) as crt_file:
        client = ecf.Client(host, port)
        client.enable_ssl()
        crt = client.get_certificate()
        assert crt.endswith(
            expected_crt
        ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_specific_certificate_using_envvar():
    expected_crt = f"{host}.{port}.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment({"ECF_SSL": "X"}) as env:
            client = ecf.Client(host, port)
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_with_specific_certificate_using_envvar_and_explicit_option():
    expected_crt = f"{host}.{port}.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment({"ECF_SSL": "X"}) as env:
            client = ecf.Client(host, port)
            client.enable_ssl()
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"


def test_setup_client_with_ssl_requesting_shared_certificate_using_envvar_but_falling_back_to_specific_certificate():
    expected_crt = f"{host}.{port}.crt"

    with Test.MockFile(expected_crt) as crt_file:
        with Test.MockEnvironment({"ECF_SSL": "1"}) as env:
            client = ecf.Client(host, port)
            crt = client.get_certificate()
            assert crt.endswith(
                expected_crt
            ), f"Expected certificate to end with '{expected_crt}', but got {crt}"
