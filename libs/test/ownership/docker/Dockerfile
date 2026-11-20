# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# The privileged test image of ecFlow ownership (see README.md): the accounts, tools and shared tree used by the
# s_ownership tests, which run as root and launch the servers under the server account.

ARG BASE_IMAGE=marcosbento/lumen:ubuntu-26.04
FROM ${BASE_IMAGE}

USER root

# setpriv (util-linux) launches the server with ambient capabilities; setcap and setfacl prepare executables
RUN apt-get update \
 && apt-get install -y --no-install-recommends util-linux libcap2-bin acl \
 && rm -rf /var/lib/apt/lists/*

# The server account, the server Owner, the owners, and the group of all the owners
RUN groupadd --system ecflow-owners \
 && useradd --create-home --user-group ecflow \
 && for account in ecfdefault alice bob charlie; do \
      useradd --create-home --user-group --groups ecflow-owners "${account}"; \
    done

# The marked root of the shared tree: the server account, the group of the owners, mode 3770
RUN install -d -o ecflow -g ecflow-owners -m 3770 /srv/ecflow

# The markers of the image: the tests run, and are never skipped, when either is present
RUN touch /etc/ecflow-privileged-test-image
ENV ECF_TEST_PRIVILEGED_IMAGE=1
