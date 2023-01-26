.. _rest_api:

REST API
//////////////////////

Compilation
-----------

ecFlow REST API is implemented using an HTTP server. The HTTP server
compilation can be enabled using cmake option :: 

  -DENABLE_HTTP=ON

The compilation result is an executable called **ecflow_http**.

When launched, the server will by default listen to port 8080. This can
be changed with command line option ``-p``, ``--port``. Using option ``-v``,
``--verbose`` will increase the logging output of the server.

If ecFlow is compiled with openssl (``-DENABLE_SSL=ON``), the server will
automatically try to start in secure mode (SSL). This can be turned off
with option ``--no_ssl``.

.. note::

    SSL is a requirement for any state-altering commands.        

When running in SSL mode, the server searches for **server.crt** and
**server.key** in **$HOME/.ecflowrc/ssl**. For more details see :ref:`this page <open_ssl>`.

.. note::

    Even though the REST API requires SSL for any communication  
    including passwords, the ecFlow server has no such requirement ie. 
    ecFlow server can be run without SSL.                              

Default interval to check any changes from ecFlow server is 10 seconds.
This can be changed with option ``--polling_interval``.

Starting the REST API
---------------------

The HTTP server is started as so:

.. code-block:: shell

  ecflow_http --verbose

By default the HTTP server expects ecFlow server to be found from
location localhost:3141. This can be changed by setting environment
variables ``ECF_HOST`` and ``ECF_PORT`` before starting the HTTP server.

Command Line Options and Environment Variables
----------------------------------------------

REST API options can be controlled either with command line options or
with environment variables. All environment variables with "RESTAPI" are
new.


.. list-table::
   :header-rows: 1

   * - Command line option
     - Environment variable
     - Default Value
     - Description
   * - --cert_directory
     - ECF_RESTAPI_CERT_DIRECTORY
     - $HOME/.ecflowrc/ssl
     - Directory where SSL certificates (server.crt and server.key) are found
   * - --ecflow_host
     - ECF_HOST
     - localhost
     - ecFlow server hostname
   * - --ecflow_port
     - ECF_PORT
     - 3141
     - ecFlow server port
   * - --max_polling_interval
     - ECF_RESTAPI_MAX_UPDATE_INTERVAL
     - 300 seconds
     - Maximum interval between ecFlow server updates, set to 0 to disable drift
   * - --no_ssl
     - ECF_RESTAPI_NOSSL
     - false
     - Disable SSL
   * - --polling_interval
     - ECF_RESTAPI_POLLING_INTERVAL
     - 10 seconds
     - Interval between ecFlow server updates
   * - --port,-p
     - ECF_RESTAPI_PORT
     - 8080
     - REST API port
   * - --tokens_file
     - ECF_RESTAPI_TOKENS_FILE
     - api-tokens.json
     - Location of file where valid tokens (API keys) are found
   * - --verbose,-v
     - ECF_RESTAPI_VERBOSE
     - false
     - Enable verbose mode

REST API uses the existing ecFlow environment variables to determine
ecFlow server location:

-  ECF_HOST: ecFlow server address

-  ECF_PORT: ecFlow server port

Two new environment variables are introduced:

-  ECF_RESTAPI_TOKEN_FILE: Location of file where API token hashes are
   found

-  ECF_RESTAPI_CERT_DIRECTORY: Directory where server.crt and server.key
   files are located. If not specified, **$HOME/.ecflowrc/ssl** is used.

Authentication
--------------

For HTTP methods POST, PUT and DELETE, authentication is always required
for user commands. Authentication requires that REST API is being run
with SSL (the connection between API and ecFlow server isn't required to
be encrypted).

If ecFlow server and REST API are being run on the same server, GET
operations can be run without authentication (and without SSL).
Similarly, child commands can be run without authentication and SSL, as
they are not authentication (in the traditional sense).

+-----------------+------------------+---------------------------------+
| Command type    | HTTP method      | Authentication (SSL) required   |
+=================+==================+=================================+
| user            | POST,PUT,DELETE  | yes                             |
+-----------------+------------------+---------------------------------+
| user            | GET              | no                              |
+-----------------+------------------+---------------------------------+
| child           | PUT,GET          | no                              |
+-----------------+------------------+---------------------------------+

Basic Authentication
~~~~~~~~~~~~~~~~~~~~

Basic authentication is done with username and password. ecFlow server
acts as the authenticator, REST API is only passing the credentials
forward.

For instructions how the set the ecFlow server authentication, see
:ref:`Black list file <black_list_file>`.

The username and password are passed to REST API using standard HTTP
basic authentication mechanism. For example, with curl option ``--user``.

Token Based Authentication
~~~~~~~~~~~~~~~~~~~~~~~~~~

REST api supports simple token-basic authentication. In this setting the
api will verify a token's validity and grant access to ecFlow server if
token is valid. ecFlow server itself does not know about the token. REST
api acts as a proxy for the user.

The token should be passed to the api with http header in a standard
fashion::

  Authorization: Bearer <TOKEN>

Or with a custom header::

  X-API-Key: <TOKEN>

The token itself is just an random alphanumerical string that does not
contain any information in itself. REST API does not support JWT's.

To authenticate the token, the API needs to have a local database of
valid tokens. Currently the only supported database backend is file in
json format. The API will search the token from current working diretory
with name "api-tokens.json". Location can be changed with environment
variable ``ECF_API_TOKEN_FILE``. The API will automatically check the file
for changes every 20 seconds.

The contents of the json file are:

.. code-block:: json

  [
    {
      "hash": "...",
      "description": "...",
      "expires_at": "yyyy-mm-ddTHH:MM:SSZ",
      "revoked_at": "yyyy-mm-ddTHH:MM:SSZ"
    }
  ] 

And the field values are:

-  **hash** : hashed and salted token, format is identical to python
   library 'werkzeug': METHOD$SALT$HASH

-  **description**: a free-form description of token (application)

-  **expires_at**: iso8601 timestamp when this token will expire in UTC
   time, OPTIONAL: if missing or zero length, no expiration time is set

-  **revoked_at**: iso8601 timestamp when this token was revoked in UTC
   time, OPTIONAL: if missing or zero length, no revoke time is set

Currently supported hashing algorithms are:

-  sha256 (hmac)

-  pbkdf2 sha256

A token file can be created with a simple script:

.. code-block:: bash
    
  > cat create-token-file.sh 
  set -eu

  desc=$1
  pw=$2
  salt=$(openssl rand -hex 8)
  method=sha256
  hash=$(echo -n $pw | openssl sha256 -hmac "$salt")
  expires=$(gdate +"%Y-%m-%dT%H:%M:%SZ" -d 'tomorrow') # token expires in 24 hrs

  jq --null-input \
    --arg hash "$method\$$salt\$$hash" \
    --arg desc "$desc" \
    --arg exp "$expires" \
    '[{ "hash" : $hash, "description" : $desc, "expires_at" : $exp }]'


Run the script for application "my app" using token
"myrandomtokenstring":

.. code-block:: bash

  > sh create-token-file.sh "my app" myrandomtokenstring
  [
    {
      "hash": "sha256$75f838a880872d20$ca8391ae4e3dc53d68befac3ab0f6f6c13ad2a770fc1e06fb7a7fba87169f21d",
      "description": "my app",
      "expires_at": "2022-10-07T11:23:02Z"
    }
  ]

Container Images
----------------

For development and experimentation purposes a container image exists
at:

https://hub.docker.com/repository/docker/partio/ecflow-http

The image can be used for quick prototyping. It expects that ecFlow
server is found from localhost:3141, change address with ECF_HOST and
ECF_PORT if needed. Server does not have a SSL certificates defined, so it can only be used to query ecFlow server (not alter states etc).

Usage example:

.. code-block:: bash

  podman run --rm -p 8080:8080 -it docker.io/partio/ecflow-http
  curl -kv https://localhost:8080/v1/server/ping

API v1 Documentation
--------------------

The API supports operations using GET, POST, PUT and DELETE methods.
Generally the last word of the URL defines the target of the query. For
example, https://localhost/v1/suites. There are seven different
supported targets:

-  attributes
-  definition
-  output
-  ping
-  script
-  status
-  suites
-  tree

A short description of the targets.

API Targets
~~~~~~~~~~~

attributes
^^^^^^^^^^

Attributes are properties of a node. Supported REST methods are: GET,
POST, PUT, DELETE.

definition
^^^^^^^^^^

Definition is the definition of an ecflow suite or a part of it in the
ecflow domain specific language. Supported REST methods are: GET, PUT.

output
^^^^^^

Output target is used to retrieve the task output. Supported REST
methods are: GET

ping
^^^^

Pings the ecflow server from the REST api. Supported REST methods are:
GET.

script
^^^^^^

Scripts are the files that ecflow executes when running tasks. It is
possible to view a script content. Supported REST methods are: GET.

suites
^^^^^^

This target is used to either list all current suites or to create a new
suite. Supported REST methods are: GET, POST.

status
^^^^^^

This target is used to access the runtime status of a node. Supported
REST methods are: GET, PUT.

tree
^^^^

This is a special target that shows a tree-view of the ecflow node
structure. Supported REST methods are: GET.

Endpoints
~~~~~~~~~

Listing
^^^^^^^

The API has following endpoints.


.. list-table::
   :header-rows: 1

   * - #
     - Endpoint
     - Method
     - Comment
     - Payload
     - Example Result
   * - 1
     - /v1/suites
     - GET
     - Get a list of all suites
     -
     - ["a"]
   * - 2
     - /v1/suites/tree
     - GET
     - Get a tree view of all suites
     -
     - {"a":{"b":{"c":""}}}
   * - 3
     - /v1/suites
     - POST
     - Create a new suite
     - {"definition": "..."}
     - {"ok"}
   * - 4
     - /v1/suites/{node_path}
     - DELETE
     - Delete a node
     -
     - {"ok"}
   * - 5
     - /v1/suites/{node_path}/tree
     - GET
     - Get node tree
     -
     - {"b":{"c":""}}
   * - 6
     - /v1/suites/{node_path}/definition
     - GET
     - Get node definition
     -
     - {"definition": "..."}
   * - 7
     - /v1/suites/{node_path}/definition
     - PUT
     - Update node definition
     - {"definition":"family foo\n endfamily"}}
     - {"message": "Node updated successfully"}
   * - 8
     - /v1/suites/{node_path}/definition
     - DELETE
     - Delete node
     - {"message": "Node delete successfully"}
     -
   * - 9
     - /v1/suites/{node_path}/status
     - GET
     - Get node status
     -
     - {"status":"aborted"}
   * - 10
     - /v1/suites/{node_path}/status
     - PUT
     - Update node status
     - {"action":"complete"}
     - {"message":"Status changed successfully"}
   * - 11
     - /v1/suites/{node_path}/attributes
     - POST
     - Create a new node attribute
     - {"type":"...","name":"...","value":"..."}
     - {"message": "Attribute added succesfully"}
   * - 12
     - /v1/suites/{node_path}/attributes
     - GET
     - Get node attributes
     -
     - {"meters":[],"variables":[]}
   * - 13
     - /v1/suites/{node_path}/attributes
     - PUT
     - Update node attributes
     - {"type":"...","name":"...","value":"..."}
     - {"message":"Attribute changed successfully"}
   * - 14
     - /v1/suites/{node_path}/attributes
     - DELETE
     - Delete node attribute
     - {"type":"..","name":"..."}
     - {"message":""Attribute deleted succesfully"}
   * - 15
     - /v1/suites/{node_path}/script
     - GET
     - Get task script and job file
     -
     - {"script": "..."}
   * - 16
     - /v1/suites/{node_path}/output
     - GET
     - Get task output
     -
     - {"job_output": "..."}
   * - 17
     - /v1/server/status
     - GET
     - Get ecflow server information
     -
     - {"statistics":{...}}
   * - 18
     - /v1/server/status
     - PUT
     - Update ecflow server status (reload configuration)
     - {"action":"..."}
     - {"message":"Server updated successfully"}
   * - 19
     - /v1/server/attributes
     - GET
     - Get ecflow server attributes
     -
     - {"variables": [ ... ]]}
   * - 20
     - /v1/server/attributes
     - POST
     - Add ecflow server attribute
     - {"type" : "variable", "name" : "...", "value" : ".."}
     - {"message":"Attribute added successfully"
   * - 21
     - /v1/server/attributes
     - PUT
     - Update ecflow server attribute
     - {"type" : "variable", "name" : "...", "value" : ".."}
     - {"message":"Attribute changed successfully"
   * - 22
     - /v1/server/attributes
     - DELETE
     - Delete server attribute
     - {"type" : "variable", "name" : "..."}
     - {"message": "Attribute deleted successfully"}
   * - 23
     - /v1/server/ping
     - GET
     - Ping ecflow server
     -
     - {"host":"...","round-trip-time":"..."}
   * - 24
     - /v1/statistics
     - GET
     - GET API statistics
     -
     - {"num_requests":"...","num_errors":"..."}


Payload Format for Creating a New Suite or Updating Node Definition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: json

  {
    "definition": "...",
    "auto_add_extern": true|false
  }

where

-  definition: ecFlow suite definition

-  auto_add_extern: whether to automatically add external triggers

Payload Format for Updating Node Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Updating node status with a user command with user authentication.


.. code-block:: json

  {
    "action": "abort|begin|complete|defstatus|execute|requeue|rerun|resume|submit|suspend",
    "recursive": false
  }

where

-  name: Name of the action that is taken against the given path

-  recursive: Specify if same action is run recursive through the
   children of the node. Note: not all actions support recursive
   operation. Default: false

For action=defstatus there is additional option:

.. code-block:: json
    
  {
    "name": "defstatus",
    "defstatus_value": "aborted|complete|queued|suspended|unknown"
  }

Payload Format for Updating Node Status From a Child Command
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Updating node status with a child command (ie. from a script with child
command authentication).

.. code-block:: json
    
  {
    "ECF_NAME": "...",
    "ECF_PASS": "...",
    "ECF_RID": "...",
    "ECF_TRYNO": "...",
    "action": "abort|complete|init|wait"
  }

where

-  name: Name of the action that is taken against the given path

-  ECF_NAME, ECF_PASS, ECF_RID, ECF_TRYNO: ecFlow generated parameters

Some actions have additional parameters:

abort:

.. code-block:: json
    
  {
    "name": "abort",
    "abort_why": "..."
  }

wait:

.. code-block:: json
    
  {
    "name": "wait",
    "wait_expression": "..."
  }

Payload Format for Updating Node Attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Updating node attributes with a user command with user authentication.

.. code-block:: json
    
  {
    "name": "...",
    "type": "event|generic|inlimit|label|late|limit|meter|queue|time|today|trigger|variable",
    "value": "...",
    "old_value": "...",
    "min": "...",
    "max": "..."
  }


-  name: name of the attribute that is changed

-  type: type of the attribute

-  value: value of the added or changed attribute. For delete the key is
   ignored.

-  old_value: for some attributes that don't have a name, old_value is
   needed to specify which one of the possible multiple attributes are
   changed.

For event, the value must be

-  true or "set", if the event is to be set

-  false or "clear", if the event is to be cleared

For limit, the name of the "value" key must be

-  "value", if the value of the limit is changed

-  "max", if the upper limit of the limit is changed

For meter the following keys need to be defined:

-  "value"

-  "min"

-  "max"

For today and time, the keys for setting values are:

-  "value" to set the new value

-  "old_value", to specify which one of the (possible multiple) todays
   are to be updated

-  "name" is not needed

For attributes that are not named, such as repeat or late

-  "name" is not needed

Payload Format for Updating Node Attributes From a Child Command
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Updating node attributes with a child command (ie. from a script with
child command authentication).

.. code-block:: json
    
  {
    "ECF_NAME": "...",
    "ECF_PASS": "...",
    "ECF_RID": "...",
    "ECF_TRYNO": "...",
    "name": "...",
    "type": "event|label|limit|meter|queue",
    "value": "...",
    "queue_action": "...",
    "queue_step"
  }

where

-  name: name of the attribute that is changed

-  ECF_NAME, ECF_PASS, ECF_RID, ECF_TRYNO: ecFlow generated parameters

Some actions have additional parameters:

queue:

.. code-block:: json

  {
    "name": "queue",
    "queue_action": "...",
    "queue_step": "..."
  }


Payload Format for Updating Server Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: json
    
  {
    "action" : "reload_whitelist_file|reload_passwd_file|reload_custom_passwd_file"
  }


Payload Format for Updating Script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note! SCript can only be updated through the REST API if it already
exists in the server.

.. code-block:: json
    
  {
    "script" : "..."
  }

Queryparameters
~~~~~~~~~~~~~~~

Supported queryparameters:

.. list-table::
   :header-rows: 1

   * - Name
     - Value
     - Comment
   * - filter
     - a.b.c[0]
     - filter returned json
   * - key
     - abcdf
     - API key (token), if client is unable to pass the key with HTTP headers
  

Swagger UI / OpenAPI
~~~~~~~~~~~~~~~~~~~~

For a more graphical documentation of the API, see the accompanied
openapi specification file (openapi.yaml).

To run swagger ui in a container, use the following Containerfile:

.. code-block::
    
  FROM swaggerapi/swagger-ui
  ADD openapi.yaml /tmp
  ENV SWAGGER_JSON=/tmp/openapi.yaml

Implementation Details
----------------------

Bsaically the API is a wrapper that transforms requests in web-syntax to
ecflow syntax, and similarly transforming the results from plain-text to
valid json.

The API is internally using the normal ClientInvoker method to
communicate with the server. From the ecFlow servers' point of view the
API is just another client.

The API can done some things that the command line tool ecflow_client
cannot, mostly to enable adding attributes to existing suites.
ecflow_client can do to this to some attributes, but the API has a
broader support. The API also caches the server state and updates it
only in certain configurable intervals.

The API will keep a cached copy of definitions in its memory. The copy
is updated by default every 10 seconds (adjustable with a command line
option). This means that when issuing a GET query to API, it will touch
the cached copy of definitions and no connection to ecFlow server is
made. There are some exceptions to this: when querying output, script,
server ping, server status a connections to ecFlow server is opened.
Also all altering commands PUT, POST and DELETE result in a connection
to ecFlow server.


.. list-table::
   :header-rows: 1

   * - Method
     - Endpoint
     - Will result into a connection to ecFlow server
   * - GET
     - v1/suites/.../output
     - YES
   * - GET
     - v1/suites/.../script
     - YES
   * - GET
     - v1/server/ping
     - YES
   * - GET
     - v1/server/status
     - YES
   * - POST
     - Any
     - YES
   * - PUT
     - Any
     - YES
   * - DELETE
     - Any
     - YES
   * - GET
     - Anything else
     - NO

The API includes two external libraries, both libraries are header only
and licensed with MIT license:

-  `cpp-httplib <https://github.com/yhirose/cpp-httplib>`__: provides
   http server implementation.

-  `nlohmann/json <https://github.com/nlohmann/json>`__: provides json
   encoding/decoding functions

The API supports the usual REST API versioning, meaning that the current
version is "v1" and that version number is a part of the URL. The API
can support multiple different version side-by-side. The v1 code is
basically split into two files: **ApiV1.hpp/cpp**, and
**ApiV1Impl.hpp/cpp**. The first one registers the endpoints used to the
HTTP server and deals with all the HTTP specific things. The latter
(ApiV1Impl) contains all business logic: contacting ecflow server and
formulating requests/responses.

Compiled succesfully with following compilers (CMAKE_BUILD_TYPE=Debug):

-  gnu

   -  8.5

   -  9.2

   -  10.3

   -  11.2

   -  12.0

-  clang

   -  11.1

   -  13.0

   -  14.0

   -  15.0

Update Interval Drift
~~~~~~~~~~~~~~~~~~~~~

By default the REST API will increase the update interval length for
ecFlow server if the API server is inactive. This is called drift.

For every one minutes that goes by without requests from users, the
update interval (given with ``--polling_interval``, default value 10
seconds) is increased linearly by one second. The default maximum value
is 300 seconds. Whenever the API receives a request from user, the
update interval value is reset to normal value.

The maximum polling interval can changed with command line option
``--max_polling_interval``. If drift is enabled, the minimum value is hard
coded to 30 seconds.

Examples
--------

All examples assume that:

-  api server is located at https://localhost:8080

-  a valid token is supplied

-  a suite named "test" exists

Ping ecflow server
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl https://localhost:8080/v1/server/ping

Get ecflow server status
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl https://localhost:8080/v1/server/status

Get ecflow server attributes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl https://localhost:8080/v1/server/attributes

Get API server statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl https://localhost:8080/v1/statistics

Get suite status
~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl https://localhost:8080/v1/suites/test/status

Get suite status with filtering just for defstatus
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  
  curl https://localhost:8080/v1/suites/test/status?filter=default_status

Get all suite attributes
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  
  curl https://localhost:8080/v1/suites/test/attributes

Get all suite variables
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  
  curl https://localhost:8080/v1/suites/test/attributes?filter=variables

Get task output
~~~~~~~~~~~~~~~

.. code-block:: bash
  
  curl https://localhost:8080/v1/suites/test/path/to/task/output

Get task script
~~~~~~~~~~~~~~~

.. code-block:: bash
  
  curl https://localhost:8080/v1/suites/test/path/to/task/script

Authentication options
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl [...] https://localhost:8080/v1/suites -H 'authorization: Bearer <MYTOKEN>'
  curl [...] https://localhost:8080/v1/suites -H 'x-api-key: <MYTOKEN>'
  curl [...] https://localhost:8080/v1/suites?key=<MYTOKEN>

Create a new suite
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl -X POST https://localhost:8080/v1/suites -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"definition": "suite test2\n family a\n task a\n endfamily\nendsuite"}'

Create a new family
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl -X PUT https://localhost:8080/v1/suites/test -H 'content-type: application/json' -H authorization: Bearer <MYTOKEN>' -d '{"definition": "family b\nendfamily\n"}'

Create a new attribute
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
    
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autoarchive","value":"+01:00"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autocancel","value":"+01:00"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autorestore","value":"/test/a"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"complete","value":"/test/a eq complete"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"cron","value":"-w 0,1 10:00"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"date","value":"1.*.*"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"day","value":"monday"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"event","name":"foo","value":"set"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"label","name":"foo","value":"bar"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"late","value":"-s +00:01 -a 14:30 -c +00:01"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"limit","name":"foo","value":"0"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"meter","name":"foo","value":"10","min":"0","max":"20"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"time","value":"+00:20"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"today","value":"03:00"}'
  curl -X POST https://localhost:8080/v1/suites/test/dynamic/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"variable","name":"foo","value":"bar"}'


Update an attribute value
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autoarchive","value":"0"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autocancel","value":"0"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autorestore","value":"/test"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"complete","value":"/test/a eq active"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"cron","old_value":"-w 0,1 10:00","value":"23:00"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"date","old_value":"1.*.*","value":"2.*.*"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"day","old_value":"monday","value":"tuesday"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"event","name":"foo","value":false}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"label","name":"foo","value":"baz"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"late","old_value":"-s +00:01 -a 14:30 -c +00:01","value":"-c +00:01"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"limit","name":"foo","value":"6"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"meter","name":"foo","value":"15"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"time","old_value":"+00:20","value":"+00:25"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"today","old_value":"03:00","value":"03:00 05:00 01:00"}'
  curl -X PUT https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"variable","name":"foo","value":"baz"}'


Update status
~~~~~~~~~~~~~

.. code-block:: bash
    
  curl -X PUT https://localhost:8080/v1/suites/test/status -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"action":"complete"}'
  curl -X PUT https://localhost:8080/v1/suites/test/status -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"action":"requeue"}'


Delete an attribute
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autoarchive"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autocancel"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"autorestore"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"complete","value":"/test/a eq active"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"cron","value":"23:00"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"date","value":"2.*.*"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"day","value":"tuesday"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"event","name":"foo"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"label","name":"foo"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"late","value":"-c +00:01"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"limit","name":"foo"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"meter","name":"foo"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"time","value":"+00:25"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"today","value":"03:00 05:00 01:00"}'
  curl -X DELETE https://localhost:8080/v1/suites/test/attributes -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"type":"variable","name":"foo"}'

Delete a suite
~~~~~~~~~~~~~~

.. code-block:: bash

  curl -X DELETE https://localhost:8080/v1/suites/test/definition -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>'
