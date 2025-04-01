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

import ecflow_test_util as Test
import inspect

import ecflow as ecf

port = 31415
host = os.uname()[1]


class Tests:

    @staticmethod
    def can_setup_client_without_ssl_and_get_empty_certificate():
        Test.name_this_test()

        expected_crt = ""

        client = ecf.Client(host, port)
        crt = client.get_certificate()
        assert crt is expected_crt, f"Expected certificate to be '{expected_crt}', but got '{crt}'"


    @staticmethod
    def can_setup_client_with_ssl_with_shared_certificate_solely_based_envvars_and_ecf_ssl_empty():
        Test.name_this_test()

        expected_crt = f"server.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "", "ECF_HOST": host, "ECF_PORT": str(port)}) as env:
                client = ecf.Client()
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_shared_certificate_solely_based_envvars():
        Test.name_this_test()

        expected_crt = f"server.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "1", "ECF_HOST": host, "ECF_PORT": str(port)}) as env:
                client = ecf.Client()
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_requesting_shared_but_finding_specific_certificate_solely_based_envvars():
        Test.name_this_test()

        expected_crt = f"{host}.{port}.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "1", "ECF_HOST": host, "ECF_PORT": str(port)}) as env:
                client = ecf.Client()
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_specific_certificate_solely_based_envvars():
        Test.name_this_test()

        expected_crt = f"{host}.{port}.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "X", "ECF_HOST": host, "ECF_PORT": str(port)}) as env:
                client = ecf.Client()
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_shared_certificate_using_envvar():
        Test.name_this_test()

        expected_crt = f"server.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "1"}) as env:
                client = ecf.Client(host, port)
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_shared_certificate_using_empty_envvar():
        Test.name_this_test()

        expected_crt = f"server.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": ""}) as env:
                client = ecf.Client(host, port)
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_shared_certificate_using_explicit_option():
        Test.name_this_test()

        expected_crt = f"server.crt"

        with Test.MockFile(expected_crt) as crt_file:
            client = ecf.Client(host, port)
            client.enable_ssl()
            crt = client.get_certificate()
            assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_specific_certificate_using_envvar():
        Test.name_this_test()

        expected_crt = f"{host}.{port}.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "X"}) as env:
                client = ecf.Client(host, port)
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_with_specific_certificate_using_envvar_and_explicit_option():
        Test.name_this_test()

        expected_crt = f"{host}.{port}.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "X"}) as env:
                client = ecf.Client(host, port)
                client.enable_ssl()
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"

    @staticmethod
    def can_setup_client_with_ssl_requesting_shared_certificate_using_envvar_but_falling_back_to_specific_certificate ():
        Test.name_this_test()

        expected_crt = f"{host}.{port}.crt"

        with Test.MockFile(expected_crt) as crt_file:
            with Test.MockEnvironment({"ECF_SSL": "1"}) as env:
                client = ecf.Client(host, port)
                crt = client.get_certificate()
                assert crt.endswith(expected_crt), f"Expected certificate to end with '{expected_crt}', but got {crt}"


if __name__ == "__main__":
    Test.execute_all(Tests)
