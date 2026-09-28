.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _multi_tenant_environment_on_kubernetes:

Multi-Tenant Environment on Kubernetes
**************************************

.. warning::

   This recipe is **experimental** and a **work in progress**.

   It describes an internal, development-oriented deployment of the multi-tenant ecFlow server, which
   is not yet ready for production use. The stack, the commands, and the configuration shown here may
   change at any time. Treat it as a starting point for experimentation rather than a supported
   deployment procedure.

This note describes how to run the multi-tenant reference stack on a local Kubernetes cluster, created
with `kind <https://kind.sigs.k8s.io/>`_ on a single machine. The stack is the one described in
:ref:`multi_tenant_reference_environment`, composed of:

- a reverse proxy (nginx), which terminates TLS and grants access to the ecFlow server only to the
  requests that the authentication service accepts;

- an ``auth-o-tron`` authentication service;

- an ecFlow server, sitting behind the reverse proxy, with an SFTP service through which the users
  deliver their suite files.

Compared with the Docker Compose stack, the deployment on Kubernetes keeps the ecFlow server and the
authentication service out of reach from the host, separates what only the administrator manages
from what the users provide, and describes every component in manifests that also apply to a
managed cluster.

The whole deployment is driven by one script, ``releng/imachination/k8s/imachination.py``. The
commands below are run from the root of the ecFlow repository. The reference documentation of the
script, and of every configuration file, is ``releng/imachination/k8s/README.md``.

The changes a suite requires to run against this kind of deployment are described in
:ref:`multi_tenant_environment_suite_setup`, and the general concept of running ecFlow behind an
authenticating reverse proxy in :ref:`how_to_setup_ecflow_with_https_authentication`.

How it works
============

The cluster
-----------

The cluster has a single node, a Docker container created by ``kind`` from
``releng/imachination/k8s/kind-cluster.yaml``. Every object of the stack lives in the namespace
``imachination``. Only two ports are published on the host:

.. list-table::
   :header-rows: 1

   * - Host port
     - Service
     - Purpose
   * - 443
     - ``revproxy``
     - HTTPS entry point of every ecFlow client; ``/v1/ecflow`` is gated by ``auth-o-tron``
   * - 2222
     - ``sftp``
     - SFTP (and ``scp``) into the workspace of the ecFlow server, with per-user keys

Nothing else listens on the host: the authentication service and the ecFlow server are reachable only
from within the cluster, and NetworkPolicies admit only the reverse proxy to them. The authenticated
path is therefore the only way in.

The components
--------------

.. list-table::
   :header-rows: 1

   * - Workload
     - Image
     - Role
   * - ``revproxy``
     - ``ecflow-revproxy-dev``
     - nginx, terminating TLS and checking every request against ``authotron`` before forwarding it to
       ``ecflow-server:8888``
   * - ``authotron``
     - ``auth-o-tron``
     - Validates Basic credentials against a list of test users, and Bearer tokens against the ECMWF API
   * - ``ecflow-server``
     - ``ecflow-server-dev``, ``ecflow-sftp-dev``
     - One Pod holding the ecFlow server and an SFTP sidecar, which share the workspace

The images are published to ``eccr.ecmwf.int/ecflow-dev-environments`` by the ``dockit`` workflow
(``.github/workflows/dockit.yml``), for ``linux/amd64`` and ``linux/arm64``; the tag ``latest``
follows ``develop``. The images are pulled on the host and loaded into the node, which never pulls an
image itself, so the cluster needs no registry credential.

The files
---------

The ecFlow server Pod sees three locations, each with its own owner and lifetime:

.. list-table::
   :header-rows: 1

   * - Location
     - Content
     - Provided by
   * - ``/workspace`` (``ECF_HOME``)
     - The task scripts and include files, the job files and outputs, the data of the tasks, and the
       log of the server
     - The users, over SFTP; stored on the PersistentVolumeClaim ``ecflow-workspace``
   * - ``/admin``, read-only
     - ``server_environment.cfg``, the troika wrapper and configurations, and the credentials that the
       jobs use to reach the server
     - The administrator, through the Secret ``ecflow-admin``
   * - ``/state``
     - The checkpoint of the server
     - The server; stored on the PersistentVolumeClaim ``ecflow-state``

The server starts in ``/admin``, where it reads ``server_environment.cfg``, with ``ECF_HOME`` set to
``/workspace``. The SFTP sidecar mounts the workspace only: a user delivering files never sees
``/admin``. The keys of the sidecar come from a separate Secret, ``sftp-keys``.

The request path
----------------

Every ecFlow client, whether ``ecflow_client``, ``ecflow_ui``, or the child commands of a job,
reaches the server through the reverse proxy, over HTTPS, and presents its credentials from an
``ecflowapirc`` file:

1. the client sends its command to ``https://localhost:443`` (from the host) or to
   ``https://revproxy.imachination.svc.cluster.local:443`` (from a job, inside the cluster);

2. nginx asks ``authotron`` to validate the credentials, and refuses the request with ``401`` when
   they are missing or wrong;

3. nginx forwards an accepted request to the ecFlow server.

The jobs run inside the ecFlow server container. Their child commands take the same path as any other
client, with the credentials of the owner of the suite, read from ``/admin``.

Setting up the cluster
======================

Prerequisites
-------------

- Docker, running, with 4 GB of memory or more for the Kubernetes node.
- ``kind`` and ``kubectl``.
- Python 3, and ``sftp``.
- A login to the registry from which the images are pulled:

  .. code-block:: shell

     docker login eccr.ecmwf.int

- An ecFlow client (``ecflow_client``, ``ecflow_ui``) built with SSL support, recent enough to read
  credentials from an ``ecflowapirc`` file.

Creating the cluster
--------------------

.. code-block:: shell

   releng/imachination/k8s/imachination.py up        # create the cluster, load the images, deploy the stack
   releng/imachination/k8s/imachination.py verify    # check that only authenticated users get in
   releng/imachination/k8s/imachination.py status    # show the cluster, the images and the objects of the stack

.. implementation::

   The commands that ``up``, ``verify`` and ``status`` run.

   .. extension::

   Every ``kubectl`` command below also carries ``--context kind-imachination``, which selects the
   cluster, and is shown without it.

   ``up`` creates the cluster, unless ``kind get clusters`` already lists it, then pulls each image on the
   host and loads it into the node:

   .. code-block:: shell

      kind create cluster --config releng/imachination/k8s/kind-cluster.yaml
      kubectl wait --for=condition=Ready nodes --all --timeout=180s

      ECCR=eccr.ecmwf.int/ecflow-dev-environments
      for image in $ECCR/ecflow-server-dev:latest $ECCR/ecflow-revproxy-dev:latest \
                   $ECCR/ecflow-sftp-dev:latest eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.3.7; do
          docker pull "$image"      # latest: always; other tags: only when not cached
          kind load docker-image "$image" --name imachination
      done

   It then loads the base server configuration, when no configuration is loaded yet, applies the
   manifests, and waits for each workload:

   .. code-block:: shell

      kubectl create namespace imachination --dry-run=client -o yaml | kubectl apply -f -
      kubectl -n imachination create secret generic ecflow-admin \
          --from-file=releng/imachination/ecflow/admin/server_environment.cfg \
          --dry-run=client -o yaml | kubectl apply -f -
      kubectl kustomize releng/imachination | kubectl apply -f -
      for workload in authotron ecflow-server revproxy; do
          kubectl -n imachination rollout status deployment/$workload --timeout=300s
      done

   ``verify`` sends ``ecflow_client --https --ping`` with the credentials of the test user ``admin``,
   then ``ecflow_client --https --query state /imachination-verify-<n>``, a path unique to the run,
   without credentials and with a wrong password, each of which must be refused with ``401``. Finally,
   it checks that the refused requests left no trace in the server log:

   .. code-block:: shell

      kubectl -n imachination exec deployment/ecflow-server -c ecflow-server -- \
          grep -c -e /imachination-verify-<n> /workspace/ecflow-server.8888.ecf.log

   ``status`` runs:

   .. code-block:: shell

      kind get clusters
      kubectl get nodes
      docker exec imachination-control-plane crictl images
      kubectl -n imachination get all,configmap,secret

``up`` creates the cluster, loads the images and applies the manifests, then waits for every workload
to become available; it takes about a minute when the images are already cached. Every step is
idempotent, so ``up`` can be run again at any time. ``verify`` checks that valid credentials reach the
server, that missing and wrong credentials are refused with ``401``, and that the refused requests
never reach the server.

The images of ``develop`` are used by default. To use those built from another branch, name them
before ``up``, for example:

.. code-block:: shell

   ECCR=eccr.ecmwf.int/ecflow-dev-environments
   export ECFLOW_SOURCE=$ECCR/ecflow-server-dev:<branch tag>
   releng/imachination/k8s/imachination.py up

.. implementation::

   How the image selected replaces the one named in the manifests.

   .. extension::

   The image selected is pulled and loaded under its own name, and replaces the image of the manifests
   in the output of ``kubectl kustomize``, before ``kubectl apply``:

   .. code-block:: shell

      kind load docker-image $ECCR/ecflow-server-dev:<branch tag> --name imachination
      kubectl kustomize releng/imachination \
          | sed "s|image: $ECCR/ecflow-server-dev:latest\$|image: $ECCR/ecflow-server-dev:<branch tag>|" \
          | kubectl apply -f -

Any command can be run with ``--dryrun``, which shows the commands that would change the cluster
without running them, or with ``--verbose``, which shows every command as it runs.

Connecting a client
-------------------

Create an ``ecflowapirc`` file holding the credentials of a user. The test users are defined in
``releng/imachination/authotron/config.yaml``:

.. code-block:: json

   {
     "version": 1,
     "tokens": [
       { "type": "basic", "server": "https://localhost:443",
         "api": { "username": "<username>", "password": "<password>" } }
     ]
   }

The client reads the file named by ``ECF_AUTHTOKENS``, or else ``~/.ecflowapirc``:

.. code-block:: shell

   export ECF_HOST=localhost ECF_PORT=443 ECF_AUTHTOKENS=<path to the ecflowapirc file>
   ecflow_client --https --ping

In ``ecflow_ui``, add the server ``localhost``, port ``443``, using HTTPS.

.. note::

   The ecFlow server starts **halted**, and does so again every time its Pod is replaced: no job is
   submitted until an administrator issues ``ecflow_client --https --restart``.

Provisioning the environment
============================

The environment is provisioned with three directories, one for each destination. Each directory is
copied to its destination as a whole; how the directories are produced does not matter to the
deployment, as long as each holds the content its destination expects.

.. list-table::
   :header-rows: 1

   * - Directory
     - Content
     - Destination
     - Provisioned by
   * - Server configuration
     - ``server_environment.cfg``, ``troikaw``, ``troika/<user>/troika.yml``,
       ``secrets/<user>/ecflowapirc``
     - ``/admin``
     - The administrator, through the cluster
   * - SFTP keys
     - ``<user>.authorized_keys`` for each user, and the host key ``ssh_host_ed25519_key``
     - The SFTP sidecar
     - The administrator, through the cluster
   * - Workspace
     - The task scripts and include files, and the directory tree of the job outputs
     - ``/workspace``
     - A user, over SFTP only

The administrator's files
-------------------------

The administrator loads the server configuration and the SFTP keys through the cluster, one at a
time; each replaces what was loaded before:

.. code-block:: shell

   releng/imachination/k8s/imachination.py provision --role admin --target ecflow-server --dir <server configuration>
   releng/imachination/k8s/imachination.py provision --role admin --target ecflow-sftp   --dir <SFTP keys>

.. implementation::

   The commands that ``provision --role admin`` runs.

   .. extension::

   Each file of the directory becomes a key of the Secret, named after its path with ``__`` in place of
   ``/`` (a Secret has flat keys); hidden files are left out. For the server configuration:

   .. code-block:: shell

      kubectl -n imachination create secret generic ecflow-admin \
          --from-file=server_environment.cfg=<dir>/server_environment.cfg \
          --from-file=troikaw=<dir>/troikaw \
          --from-file=troika__<user>__troika.yml=<dir>/troika/<user>/troika.yml \
          --from-file=secrets__<user>__ecflowapirc=<dir>/secrets/<user>/ecflowapirc \
          --dry-run=client -o yaml | kubectl apply -f -

   and in the same way for the SFTP keys, in the Secret ``sftp-keys``. The ecFlow server Pod is then
   replaced, and the command returns once the new Pod is available and the old one is gone:

   .. code-block:: shell

      OLD=$(kubectl -n imachination get pods -l app=ecflow-server -o name)
      kubectl -n imachination rollout restart deployment/ecflow-server
      kubectl -n imachination rollout status deployment/ecflow-server --timeout=300s
      kubectl -n imachination wait --for=delete $OLD --timeout=300s

   An init container of the new Pod expands the keys of ``ecflow-admin`` into ``/admin``.

Each command stores the directory in a Secret, then replaces the ecFlow server Pod, so that the new
files take effect. As the server starts halted, restart it once both are loaded:

.. code-block:: shell

   ECF_AUTHTOKENS=<ecflowapirc of an administrator> ecflow_client --https --restart

The ``ecflowapirc`` files under ``secrets/<user>/`` are those used by the jobs of each user. Their
entries must name the address of the reverse proxy inside the cluster,
``https://revproxy.imachination.svc.cluster.local:443``.

The workspace
-------------

A user copies a directory into the workspace over SFTP, with no access to the cluster:

.. code-block:: shell

   releng/imachination/k8s/imachination.py provision --role user --dir <workspace> --user <user>

The content of the directory is merged into ``/workspace``. The user logs in with the private key
``<host>/<user>/id_ed25519``, and checks the host key of the sidecar against ``<host>/known_hosts``,
where ``<host>`` is the directory given with ``--host``, by default the directory ``host/`` beside the
one provisioned. The same delivery can be made with ``sftp`` or ``scp`` directly, on port 2222.

.. implementation::

   The commands that ``provision --role user`` runs.

   .. extension::

   A first ``sftp`` session checks that the sidecar accepts the key, and is repeated for up to 30
   seconds while the sidecar cannot be reached; a second one copies the directory:

   .. code-block:: shell

      SFTP="sftp -P 2222 -i <host>/<user>/id_ed25519 -o IdentitiesOnly=yes -o IdentityAgent=none \
                -o BatchMode=yes -o UserKnownHostsFile=<host>/known_hosts -b - <user>@localhost"
      echo pwd | $SFTP
      printf 'lcd <workspace>\nput -pr *\n' | $SFTP

Running a suite
===============

Once the workspace holds the task scripts and include files, load the suite definition from the host.
The paths it contains are those seen by the server, under ``/workspace``:

.. code-block:: shell

   ecflow_client --https --load <suite>.def
   ecflow_client --https --begin <suite>

The child commands of the jobs must reach the server through the reverse proxy, as the server cannot
be reached directly. As ``ECF_HOST`` and ``ECF_PORT`` are generated by the server, and cannot be
overridden by a suite, hold the address of the proxy in variables of the suite's own:

.. code-block:: none

   edit ECFLOW_PROXY_HOST 'revproxy.imachination.svc.cluster.local'
   edit ECFLOW_PROXY_PORT '443'

and export them in the job header, with the credentials of the owner of the suite:

.. code-block:: shell

   export ECF_HOST=%ECFLOW_PROXY_HOST%
   export ECF_PORT=%ECFLOW_PROXY_PORT%
   export ECF_AUTHTOKENS=/admin/secrets/%OWNER%/ecflowapirc

The child commands also need the ``--https`` option. See
:ref:`multi_tenant_environment_suite_setup` for the complete set of changes, including job
submission through troika.

Maintaining the environment
===========================

.. list-table::
   :header-rows: 1

   * - After changing
     - Run
   * - The server configuration, the troika configurations, or the credentials of the jobs
     - ``imachination.py provision --role admin --target ecflow-server --dir <directory>``
   * - The SSH keys of the SFTP sidecar
     - ``imachination.py provision --role admin --target ecflow-sftp --dir <directory>``
   * - ``authotron/config.yaml``, the nginx configuration, or any manifest
     - ``imachination.py apply``
   * - An image published again, or built locally
     - ``imachination.py reload``, then ``ecflow_client --https --restart``

``logs [workload]`` follows the output of every workload, or of the one named, and ``restart
[workload]`` replaces it.

.. implementation::

   The commands that ``apply``, ``restart``, ``reload`` and ``logs`` run.

   .. extension::

   ``apply`` renders and applies the manifests, and waits for the workloads, as ``up`` does.
   ``restart`` replaces the workloads, and waits for the old Pods to be gone:

   .. code-block:: shell

      OLD=$(kubectl -n imachination get pods -l app=<workload> -o name)
      kubectl -n imachination rollout restart deployment/<workload>
      kubectl -n imachination rollout status deployment/<workload> --timeout=300s
      kubectl -n imachination wait --for=delete $OLD --timeout=300s

   ``reload`` pulls every image again, even when cached, loads it into the node, applies the
   manifests, and restarts every workload. ``logs`` runs:

   .. code-block:: shell

      kubectl -n imachination logs -f deployment/<workload> --all-containers --tail=50

To remove the deployment:

.. code-block:: shell

   releng/imachination/k8s/imachination.py down       # delete the stack, keeping the cluster and its images
   releng/imachination/k8s/imachination.py destroy    # delete the cluster

.. implementation::

   The commands that ``down`` and ``destroy`` run.

   .. extension::

   .. code-block:: shell

      kubectl delete namespace imachination --ignore-not-found    # down
      kind delete cluster --name imachination                     # destroy

Both delete the workspace, with the suites and their outputs, and the checkpoint of the server. Copy
out anything worth keeping first, over SFTP.

Known limitations
=================

.. note::

   The deployment is not meant for production: the credentials of ``authotron/config.yaml`` are test
   values, and the certificate of the reverse proxy is self-signed, unless one is provided in the
   Secret ``revproxy-tls``.

.. note::

   Every job runs as the same user in the ecFlow server container, and can read the files of every
   user in ``/admin``.

.. implementation::

    Just after the ecFlow server Pod is replaced, a request can reach the reverse proxy while its
    route to the server still leads to the Pod just removed. Such a request waits for the connect
    timeout of nginx, 90 seconds, before failing. As ``ecflow_ui`` sends one request at a time to a
    server, it shows no change during that time, even when refreshed or reset. The command-line client
    fails in the same way, with a ``502`` or ``504`` from the reverse proxy.
