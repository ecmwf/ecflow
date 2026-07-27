.. _multi_tenant_reference_environment:

Multi-Tenant Reference Environment
**********************************

.. warning::

   This recipe is **experimental** and a **work in progress**.

   It describes an internal, development-oriented reference deployment of the multi-tenant ecFlow
   server, which is not yet ready for production use. The stack, the commands, and the configuration
   shown here may change at any time. Treat it as a starting point for experimentation rather than a
   supported deployment procedure.

This note documents the deployment of the multi-tenant reference stack. This environment is currently
deployed on a host referred to below as ``<host>``, and is composed of:

 - a reverse proxy (nginx) that redirects HTTP to HTTPS and terminates TLS, granting access to the
   ecFlow server behind an ``auth-o-tron`` authentication service;

 - an ``auth-o-tron`` authentication service

 - an ecFlow server sitting behind the nginx reverse proxy.

It is an internal, development-oriented reference deployment, used to exercise the HTTP/HTTPS
authentication path end-to-end. It also serves as a testing ground for the multi-tenant ecFlow server,
which is not yet ready for production use.

.. implementation::

    This is not a user guide!

    It is a record of how the stack was deployed on a specific host for internal development.

    It is expected to include implementation details that are not relevant to end users, and
    that may change over time.

The generic version of this stack, intended to be run with Docker Compose (or ``podman-compose``),
is described in ``releng/imachination/INSTRUCTIONS.md``.

The concept of running ecFlow behind an authenticating reverse proxy is described more generally in
:ref:`how_to_setup_ecflow_with_https_authentication`.

This note considers the specific case of ``<host>`` because it has neither Docker Compose nor
``podman-compose`` available, so the stack was deployed with individual ``podman`` commands instead,
and those commands ended up differing from ``releng/imachination/compose.yaml`` in a few respects,
detailed below.

Deployment environment
======================

``<host>`` runs:

- **Operating system**: Debian GNU/Linux 11 (bullseye), kernel ``6.1.0-0.deb11.50-amd64``.

- **Resources**: 2 virtual CPUs, approximately 2 GiB of RAM, approximately 1 GiB of swap.

- **Storage**: separate LVM volumes for ``/``, ``/tmp``, ``/var``, ``/opt`` and ``/var/log`` (a few
  GiB each), plus a large NFS-mounted ``/home/<user>``.

- **Container runtime**: rootless Podman 3.0.1.

  This version predates ``podman compose`` and no ``podman-compose`` wrapper is installed, which is why
  the stack is deployed with plain ``podman run``/``podman network`` commands rather than a single
  Compose invocation.

- **User**: containers are run rootless, as user ``<user>``.

  On this host the unprivileged port range starts at 1024 (the kernel default), so this user cannot bind
  ports below 1024 directly; this is the reason the reverse proxy publishes remapped host ports instead
  of the conventional 80/443, as described below.

- **Workspace**: the ecFlow server's ``ECF_HOME`` is ``/home/<user>/<host>``, bind-mounted into
  the ``ecflow-server`` container.

  This directory contains ``server_environment.cfg``, checkpoint and log files, and a ``secrets`` subdirectory.

Components
==========

The stack has three services on a shared Podman bridge network named ``inner``
(subnet ``172.30.0.0/16``), each with a static IP address, mirroring the layout described in
``releng/imachination/INSTRUCTIONS.md``:

The Reverse Proxy: ``revproxy`` (172.30.0.2)
--------------------------------------------

An nginx reverse proxy, built locally from ``releng/imachination/revproxy/Dockerfile``
(``debian:12.7-slim`` with ``nginx`` and ``openssl`` installed).

A self-signed TLS certificate (``CN=localhost``) is generated and baked into the image at build time.

The proxy terminates TLS, redirects plain HTTP to HTTPS, and gates the ``/v1/ecflow`` location behind
an ``auth_request`` call to ``authotron``.

The Auth-o-tron: ``authotron`` (172.30.0.3)
-------------------------------------------

The ``auth-o-tron`` authentication service, using image
``eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.2.8``.

Configured through ``authotron/config.yaml``, with two providers:

- ``ecmwf-api-provider``, which validates Bearer tokens against ``https://api.ecmwf.int/v1``.
- ``plain-provider``, a fixed list of test users for local Basic authentication testing.

The relevant configuration is reproduced below with placeholder test credentials. It is not suitable
for use outside an internal development environment.

.. code-block:: yaml

   version: 1.0.0

   logging:
     level: "debug"
     format: "console"

   auth:
     timeout_in_ms: 3000 # 3 seconds

   providers:
     - name: "ecmwf-api-provider"
       type: "ecmwf-api"
       uri: https://api.ecmwf.int/v1
       realm: "ecmwf"
     # For testing purposes only, do not use in production
     - name: "plain-provider"
       type: "plain"
       realm: "local"
       users:
       # For testing purposes only, do not use in production
         - username: "admin"
           password: "somesecret"
           # ... additional test users, following the same pattern

   store:
     enabled: false

   services: []

   jwt:
     exp: 3600
     iss: authotron-issuer
     secret: authotron-secret-key
     aud: authotron-audience

   include_legacy_headers: True

   bind_address: 0.0.0.0:8081

.. warning::
   The ``plain-provider`` passwords and the ``jwt.secret`` value above are placeholder, test-only
   values, explicitly marked as such in the configuration file itself. They must be replaced with
   properly generated secrets before this stack is treated as anything other than an internal
   development reference.

The ecFlow server: ``ecflow-server`` (172.30.0.4)
-------------------------------------------------

The ecFlow server image, ``eccr.ecmwf.int/ecflow-dev-environments/ecflow-serveronly-dev:latest``,
is built by the separate ``releng/dockit/`` pipeline (``.github/workflows/dockit.yml``).

The image currently running corresponds to ecFlow 5.18.0, revision ``33fad7437e01a8ef``.

The image entrypoint, ``/opt/local/bin/launch.sh``, starts ``ecflow_server --http --port 8888``
followed by ``ecflow_http --no_ssl --port 8889``, both in the background. The ``ECFLOW_WORKSPACE_DIR``
environment variable drives ``ECF_HOME`` for both processes.

Detailed deployment procedure
=============================

The commands below are the exact invocations of ``podman`` used to create the three running
containers on ``<host>``. Run them from ``releng/imachination/`` unless noted otherwise.

* Create the shared network:

.. code-block:: shell

   podman network create --subnet 172.30.0.0/16 inner

* Build and run ``revproxy``:

.. code-block:: shell

   podman build -t revproxy ./revproxy

   podman run -d \
       --name revproxy --hostname revproxy \
       --network inner --ip 172.30.0.2 \
       -p 3141:443 \
       -v ./revproxy/server:/usr/share/nginx/html/server \
       -v ./revproxy/cfgs/nginx/default.conf:/etc/nginx/conf.d/default.conf \
       revproxy

* Run ``authotron``:

.. code-block:: shell

   podman run -d \
       --name authotron --hostname authotron \
       --network inner --ip 172.30.0.3 \
       -p 8080:8080 \
       -v ./authotron/config.yaml:/app/config.yaml \
       eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.2.8

* Run ``ecflow-server``:

.. code-block:: shell

   podman run -d \
       --name ecflow-server --hostname ecflow-server \
       --network inner --ip 172.30.0.4 \
       --platform linux/amd64 \
       -p 8888:8888 \
       -p 8889:8889 \
       -v /home/<user>/<host>:/home/<user>/<host> \
       -e ECFLOW_WORKSPACE_DIR=/home/<user>/<host> \
       eccr.ecmwf.int/ecflow-dev-environments/ecflow-serveronly-dev:latest

* Verify deployment:

.. code-block:: shell

   podman ps

Confirm all three containers are ``Up``, then exercise the authentication path through the reverse
proxy (the ``-k`` option is required because the certificate is self-signed):

.. code-block:: shell

   BASIC_TOKEN=$(echo -n 'admin:somesecret' | base64)
   curl -k -X GET -H "Authorization: Basic ${BASIC_TOKEN}" https://<host>:3141/v1/ecflow

* Tear down the stack

.. code-block:: shell

   podman rm -f revproxy authotron ecflow-server
   podman network rm inner


.. implementation::

    These commands differ from ``releng/imachination/compose.yaml`` and the instructions at
    ``releng/imachination/INSTRUCTIONS.md`` in the following ways:

    - ``revproxy`` publishes host ports ``8080`` and ``3141`` instead of ``80`` and ``443``.

      While nginx still listens on container ports 8080 (HTTP) and 8443 (HTTPS) internally, host port
      443 cannot be used because privileged ports are unavailable to the rootless container runtime on
      this host.

      Port 3141 was chosen for the published HTTPS port as it is the conventional ecFlow port.

    - ``authotron`` publishes and listens on port ``8081`` instead of ``8080``, leaving host port 8080
      available for ``revproxy``'s HTTP listener.

    - ``revproxy/cfgs/nginx/default.conf`` was adjusted on ``<host>`` in three places relative to the
      committed file:

      - the ``listen`` directives were changed from 80/443 to 8080/8443, matching the remapped ports
        above;

      - the ``/auth`` internal location's ``proxy_pass`` was corrected from
        ``http://172.42.0.3:8080/authenticate`` (the committed value, which points at a subnet that does
        not match the ``inner`` network at all, and would never have worked) to
        ``http://172.30.0.3:8081/authenticate`` (``authotron``'s actual address on ``inner``);

      - the ``/v1/ecflow`` location's active ``proxy_pass`` was switched from
        ``http://host.docker.internal:8888/v1/ecflow`` (the committed value) to the in-network address
        ``http://172.30.0.4:8888/v1/ecflow``. This is necessary because ``host.docker.internal`` is a
        macOS/Windows-specific feature that does not exist in Linux.

    - ``ecflow-server``'s workspace is bind-mounted at the same absolute path on both sides
      (``/home/<user>/<host>``), rather than at the example's relative ``ecflow/workspace`` path, so
      that the mount survives independently of the git checkout.


Network exposure
==================

While five host ports are currently published by the stack, only
TCP/3141 is intended to be reachable by end users:

.. list-table::
   :header-rows: 1

   * - Port
     - Service
     - Purpose
     - Intended reachability
   * - 3141
     - ``revproxy`` (HTTPS, self-signed certificate)
     - Public entry point
     - Public
   * - 8080
     - ``revproxy`` (HTTP, redirects to HTTPS)
     - Redirect helper
     - Internal-only
   * - 8081
     - ``authotron``
     - Direct access to the authentication service
     - Internal-only
   * - 8888
     - ``ecflow-server`` (native protocol, ``--http``)
     - ecFlow client/UI access
     - Internal-only
   * - 8889
     - ``ecflow-server`` (``ecflow_http`` REST API)
     - REST API access
     - Internal-only

.. implementation::

    At present, all five ports above are published to the host (bound on all interfaces),
    and reaching only 3141 from outside the host depends entirely on the perimeter firewall.

    A simple improvement would be not to publish 8081, 8888 and 8889 to the host at all,
    since ``revproxy`` reaches ``authotron`` and ``ecflow-server`` over the ``inner``
    network by their static IPs. Publishing ports 8081, 8888 and 8889 to the host is unnecessary
    and could be dropped.

    Port 8080, used for the HTTP-to-HTTPS redirect, could also be left unpublished if no plain-HTTP
    entry point is needed.

Known limitations
====================

.. note::

   The stack is started manually with ``podman run``.

   No systemd unit, no ``loginctl`` lingering configuration, and no crontab entry exist to manage it.

   This means the stack does not restart automatically after a host reboot; restarting it
   requires manually re-running (or scripting) the commands in `Detailed deployment procedure`_.

.. note::

   TLS uses a self-signed certificate baked into the ``revproxy`` image at build time.

   Clients must explicitly accept it (for example, ``curl -k``, or by importing the certificate).
   This is not suitable for an audience beyond internal development use.
