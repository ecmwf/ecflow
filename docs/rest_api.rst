.. _rest_api:


..
   This reStructured Text file uses the following convention:
     # with overline, for parts
     * with overline, for chapters
     = for sections
     - for subsections
     ^ for subsubsections
     " for paragraphs


REST API
//////////////////////

.. caution:: 
  ecFlow's REST API is experimental, and its details are subject to change. The documentation reflects its current operation.

Compilation
===========

ecFlow REST API is implemented using an HTTP server. The HTTP server
compilation is enabled by default, and can be enabled (ON) or disabled (OFF) using CMake option:

.. code:: text

  -DENABLE_HTTP=ON

The compilation result is an executable called **ecflow_http**.

By default, the ecFlow HTTP server is built with support for compression (i.e. allows :code:`Accept-Encoding: gzip`
as header in the request), based on the following CMake option:

.. code:: text

  -DENABLE_HTTP_COMPRESSION=ON

To disable compression set the :code:`ENABLE_HTTP_COMPRESSION` to :code:`OFF`. Notice that compression support
requires the presence of *zlib* -- if this dependency is not available the CMake project configuration step will fail.

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
=====================

The HTTP server is started as so:

.. code-block:: shell

  ecflow_http --verbose

By default the HTTP server expects ecFlow server to be found from
location localhost:3141. This can be changed by setting environment
variables ``ECF_HOST`` and ``ECF_PORT`` before starting the HTTP server.

Command Line Options and Environment Variables
==============================================

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
==============

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
--------------------

Basic authentication is done with username and password. ecFlow server
acts as the authenticator, REST API is only passing the credentials
forward.

For instructions how the set the ecFlow server authentication, see
:ref:`Black list file <black_list_file>`.

The username and password are passed to REST API using standard HTTP
basic authentication mechanism. For example, with curl option ``--user``.

Token Based Authentication
--------------------------

REST api supports simple token-basic authentication. In this setting the
api will verify a token's validity and grant access to ecFlow server if
token is valid. ecFlow server itself does not know about the token. REST
api acts as a proxy for the user.

The token should be passed to the api with http header in a standard
fashion:

.. code-block:: text

  Authorization: Bearer <TOKEN>

Or with a custom header:

.. code-block:: text

  X-API-Key: <TOKEN>

The token itself is just an random alphanumerical string that does not
contain any information in itself. REST API does not support JWT's.

To authenticate the token, the API needs to have a local database of
valid tokens. Currently the only supported database backend is file in
json format. The API will search the token from current working directory
with name "api-tokens.json". Location can be changed with environment
variable :code:`ECF_API_TOKEN_FILE`. The API will automatically check the file
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

API v1 Documentation
====================

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
-----------

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

API v1 supports the following endpoints -- specification for each endpoing can be found in the related subsections.

.. list-table:: List of Endpoints
   :header-rows: 1

   * - Endpoint
     - Method(s)
     - Description
   * - /v1/suites
     - POST, GET
     - Operations related to Suites
   * - /v1/suites/tree
     - GET
     - Operations related to Suite trees
   * - /v1/suites/{path}
     - DELETE
     - Operations related to Nodes
   * - /v1/suites/{path}/tree
     - GET
     - Operations related to Node trees
   * - /v1/suites/{path}/definition
     - GET, PUT, DELETE
     - Operations related to Node definitions
   * - /v1/suites/{path}/status
     - GET, PUT
     - Operations related to Node status
   * - /v1/suites/{path}/attributes
     - POST, GET, PUT, DELETE
     - Operations related to Node attributes
   * - /v1/suites/{path}/script
     - GET
     - Obtain Node script
   * - /v1/suites/{path}/output
     - GET
     - Obtain Node job output
   * - /v1/server/status
     - GET, PUT
     - Operations related to server status
   * - /v1/server/attributes
     - POST, GET, PUT, DELETE
     - Operations related to server attributes
   * - /v1/server/ping
     - GET
     - Ping server
   * - /v1/statistics
     - GET
     - Obtain REST API statistics

.. note::

  All API v1 endpoints allow the Query Parameters.

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

Endpoint :code:`/v1/suites`
---------------------------

**Create** a new Suite
^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites`
   * - Method
     - :code:`POST`
   * - Description
     - **Create** a new suite
   * - Parameters
     - *none*
   * - Payload
     - See below for details of the payload used to create a new suite.
   * - Response
     - :code:`{"ok"}`
   * - Example
     - :code:`curl -X POST https://localhost:8080/v1/suites -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"definition": "suite test2\n family a\n task a\n endfamily\nendsuite"}'`

Payload to create a new Suite
"""""""""""""""""""""""""""""

.. code-block:: json

  {
    "definition": "...",
    "auto_add_extern": "true|false"
  }

where

- :code:`definition` is the ecFlow node definition
- :code:`auto_add_extern` indicates whether to automatically add external triggers

List all Suites
^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the list with all suites
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`["a", "b", "c"]`

Endpoint :code:`/v1/suites/tree`
--------------------------------

Obtain the tree of all Suites
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/tree`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** a tree with all suites
   * - Parameters
     - :code:`content`, (optional), possible values: :code:`basic`, :code:`full`

       :code:`with_id`, (optional), possible values: :code:`true`, :code:`false`

       :code:`gen_vars`, (optional), possible values: :code:`true`

   * - Payload
     - *empty*
   * - Response
     - See :ref:`below <response with suite tree>` for details.

.. _response with suite tree:

Response with Suite tree
""""""""""""""""""""""""

When query parameter :code:`content` is not provided, or query parameter is :code:`content=basic`, the basic Suite tree is provided:

.. code:: json

    {
      "suite": {
        "family": {
          "task": ""
          }
      }
    }

When query parameter :code:`content=full` is used, the full Suite tree is provided:

.. code:: json

  {
    "some_suite": {
      "type": "suite",
      "state": {
        "node": "active",
        "default": "complete"
      },
      "attributes": [
        {
          "name": "YMD",
          "value": "20100114",
          "const": false,
          "type": "variable"
        }
      ],
      "children": {
        "some_family": {
          "type": "family",
          "state": {
            "node": "active",
            "default": "unknown"
          },
          "attributes": [],
          "children": {
            "some_task": {
              "type": "task",
              "state": {
                "node": "active",
                "default": "unknown"
              },
              "attributes": [
                {
                  "name": "some_label",
                  "value": "value",
                  "type": "label"
                },
                {
                  "name": "some_meter",
                  "min": 0,
                  "max": 30,
                  "value": 0,
                  "type": "meter"
                },
                {
                  "name": "some_event",
                  "value": false,
                  "initial_value": false,
                  "type": "event"
                }
              ],
              "aliases": {}
            }
          }
        }
      }
    }
  }

When query parameters :code:`content=full&with_id=true` are used, the full Suite tree with ids is provided:

.. code:: json

  {
    "some_suite": {
      "type": "suite",
      "id": "/some_suite",
      "state": {
        "node": "active",
        "default": "complete"
      },
      "attributes": [
        {
          "name": "YMD",
          "value": "20100114",
          "const": false,
          "type": "variable"
        }
      ],
      "children": {
        "some_family": {
          "type": "family",
          "id": "/some_suite/some_family",
          "state": {
            "node": "active",
            "default": "unknown"
          },
          "attributes": [],
          "children": {
            "some_task": {
              "type": "task",
              "id": "/some_suite/some_family/some_task",
              "state": {
                "node": "active",
                "default": "unknown"
              },
              "attributes": [
                {
                  "name": "some_label",
                  "value": "value",
                  "type": "label"
                },
                {
                  "name": "some_meter",
                  "min": 0,
                  "max": 30,
                  "value": 0,
                  "type": "meter"
                },
                {
                  "name": "some_event",
                  "value": false,
                  "initial_value": false,
                  "type": "event"
                }
              ],
              "aliases": {}
            }
          }
        }
      }
    }
  }

When using the query parameter :code:`gen_vars=true` the generated variables are included in the :code:`attributes` section.
Generated variables can be identified by a :code:`generate` attribute set to :code:`true`.

Endpoint :code:`/v1/suites/{path}`
----------------------------------

Delete a Node
^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}`
   * - Method
     - :code:`DELETE`
   * - Description
     - **Delete** the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"ok"}`

Endpoint :code:`/v1/suites/{path}/tree`
---------------------------------------

Obtain the tree of a Node
^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/tree`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** a tree view of the node at :code:`/{path}`
   * - Parameters
     - :code:`content`, (optional), possible values: :code:`basic`, :code:`full`

       :code:`with_id`, (optional), possible values: :code:`true`, :code:`false`

       :code:`gen_vars`, (optional), possible values: :code:`true`

   * - Payload
     - *empty*
   * - Response
     - Same as :ref:`Suite tree <response with suite tree>`.

Endpoint :code:`/v1/suites/{path}/definition`
---------------------------------------------

Obtain the definition of a Node
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/definition`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the definition of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"definition": "<defs>"}`, where :code:`<defs>` is the content of associated .def file

Update the definition of a Node
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/definition`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** the definition of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - See below for details of the payload used to update the definition of an existing node.
   * - Response
     - :code:`{"message": "Node updated successfully"}`

Payload to update the definition of a Node
""""""""""""""""""""""""""""""""""""""""""

.. code-block:: json

  {
    "definition": "...",
    "auto_add_extern": "true|false"
  }

where

- :code:`definition` is the ecFlow node definition
- :code:`auto_add_extern` indicates whether to automatically add external triggers

Delete a Node
^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/definition`
   * - Method
     - :code:`DELETE`
   * - Description
     - **Delete** the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"message": "Node deleted successfully"}`

Endpoint :code:`/v1/suites/{path}/status`
-----------------------------------------

Obtain a Node status
^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/status`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the status of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"status":"aborted"}`
   * - Example
     - :code:`curl https://localhost:8080/v1/suites/path/to/node/status`

       :code:`curl https://.../v1/suites/path/to/node/status?filter=default_status`

Update a Node status
^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/status`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** the status of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"status":"..."}`

Payload to update Node status (by User)
"""""""""""""""""""""""""""""""""""""""

When updating node status with a user command, with user authentication, the request payload is as follows:

.. code-block:: json

  {
    "action": "abort|begin|complete|defstatus|execute|requeue|rerun|resume|submit|suspend",
    "recursive": false
  }

where

-  :code:`name`: Name of the action that is taken against the given path
-  :code:`recursive`: Specify if same action is run recursive through the
   children of the node. Note: not all actions support recursive
   operation. Default: false

When :code:`action=defstatus`, the following additional options are necessary:

.. code-block:: text

  {
    ...
    "name": "defstatus",
    "defstatus_value": "aborted|complete|queued|suspended|unknown"
  }

Payload to update Node status (by Task)
"""""""""""""""""""""""""""""""""""""""

When updating the node status from a task (i.e. from a script with child command authentication), the request payload is as follows:

.. code-block:: json

  {
    "ECF_NAME": "...",
    "ECF_PASS": "...",
    "ECF_RID": "...",
    "ECF_TRYNO": "...",
    "action": "abort|complete|init|wait"
  }

where

-  :code:`action` is the name of the action to be taken taken
-  :code:`ECF_NAME`, :code:`ECF_PASS`, :code:`ECF_RID`, and :code:`ECF_TRYNO` are ecFlow generated parameters

When :code:`action=abort`, the followign additional parameters are necessary:

.. code-block:: text

  {
    ...
    "name": "abort",
    "abort_why": "..."
  }

When :code:`action=wait`, the followign additional parameters are necessary:

.. code-block:: text

  {
    ...
    "name": "wait",
    "wait_expression": "..."
  }

Endpoint :code:`/v1/suites/{path}/attributes`
---------------------------------------------

Create a new Node attribute
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/attributes`
   * - Method
     - :code:`POST`
   * - Description
     - **Create** new attribute for the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"...","name":"...","value":"..."}`
   * - Response
     - :code:`{"message": "Attribute added successfully"}`

Obtain all Node attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/attributes`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the attributes of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - See :ref:`below <response with node attributes>` for details
   * - Example
     - :code:`curl https://localhost:8080/v1/suites/path/to/node/attributes`

.. _response with node attributes:

Response with Node attributes
"""""""""""""""""""""""""""""

The response to a request for node attributes has the following JSON format.
Notice how the node variables are separated from inherited variables.
The inherited variables grouped by ancestor node, each identified by the node name and absolute path.

.. code-block:: text

  {
    "path": "..."

    "meters": [ { ... }, ... ],
    "limits": [ { ... }, ... ],
    "inlimits": [ { ... }, ... ],
    "events": [ { ... }, ... ],
    "labels": [ { ... }, ... ],
    "dates": [ { ... }, ... ],
    "days": [ { ... }, ... ],
    "crons": [ { ... }, ... ],
    "times": [ { ... }, ... ],
    "todays": [ { ... }, ... ],
    "repeat": { ... },
    "trigger": { ... },
    "complete": { ... },
    "flag": { ... },
    "late": { ... },
    "zombies": [ { ... }, ... ],
    "generics": [ { ... }, ... ],
    "queues": [ { ... }, ... ],
    "autocancel": { ... },
    "autoarchive": { ... },
    "autorestore": { ... },
    "avisos": [ { ... }, ... ],
    "mirrors" : [ { ... }, ... ],
    "variables": [
      {
        "name": "..."
        "value": "...",
        "const": true|false
      },
      {
        "name": "..."
        "value": "...",
        "generated": true        # only present if the variable is generated
        "const": true|false
      },
      ...
    ]
    "inherited_variables": [
      {
        "name": "...<node-name>..."
        "path": "...<node-path>...",
        "variables: [
          {
            "name": "..."
            "value": "...",
            "type": "variable",
            "const": true|false
          },
          {
            "name": "..."
            "value": "...",
            "type": "variable",
            "generated": true    # only present if the variable is generated
            "const": true|false
          },
          ...
        ]
      },
      ...
    }
  }

Update a Node attribute
^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/attributes`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** the attribute of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"...","name":"...","value":"..."}`
   * - Response
     - :code:`{"message":"Attribute changed successfully"}`

Payload to update Node Attribute (by User)
""""""""""""""""""""""""""""""""""""""""""

When updating a node attribute using a user command, with user authentication, the request payload is as follows:

.. code-block:: json

  {
    "name": "...",
    "type": "event|generic|inlimit|label|late|limit|meter|queue|time|today|trigger|variable",
    "value": "...",
    "old_value": "...",
    "min": "...",
    "max": "..."
  }

where

- :code:`name` is the name of the attribute
- :code:`type` is the type of the attribute
- :code:`value` is the value of the attribute, after the operation is complete. This is not necessary when :code:`type` is :code:`delete`.
- :code:`old_value` is used when updating attributes that do not have a name. In this case, :code:`old_value` specifies which attribute(s) are changed.

When :code:`type` is :code:`event`, the :code:`value` must be:

- :code:`true` or :code:`set`, if the event is to be set
- :code:`false` or :code:`clear`, if the event is to be cleared

When :code:`type` is :code:`limit`, the following keys must be

- :code:`value`, if the value of the limit is to be updated
- :code:`max`, if the upper limit of the limit is to be updated

When :code:`type` is :code:`meter`, the following keys are necessary:

- :code:`value`
- :code:`min`
- :code:`max`

When :code:`type` is :code:`today` or :code:`time`, the following keys are necessary:

- :code:`value` when setting the new value
- :code:`old_value` to specify the (possible multiple) attributes to be updated
- :code:`name` is not required

For unnamed attributes, such as :code:`repeat` or :code:`late`, consider that

- :code:`name` is not required

Payload to update Node attribute (by Task)
""""""""""""""""""""""""""""""""""""""""""

When updating a node attribute from a task (i.e. from a script with child command authentication), the request payload is as follows:

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
    "queue_step": "...",
    "queue_path": "..."
  }

where

- :code:`name` is the name of the attribute
- :code:`ECF_NAME`, :code:`ECF_PASS`, :code:`ECF_RID`, and :code:`ECF_TRYNO` are ecFlow generated parameters

When updating :code:`queue` attributes, the parameter :code:`value` is **not** used, and the following additional parameters are required:

.. code-block:: text

  {
    ...
    "queue_action": "...",
    "queue_step": "...",
    "queue_path": "..."
  }

The :code:`queue_step` and :code:`queue_path` parameters are optional.  The parameter :code:`queue_step` should only be
provided when the :code:`queue_action` is either :code:`complete` or :code:`aborted`.  If the field :code:`queue_path`
is provided the server will search for the queue only on the named node, otherwise, the absense of the
:code:`queue_path` parameter triggers the upward search of the queue starting from the target node (i.e. the node named
by ECF_NAME) through the node tree.

Response to update Node attribute (by Task)
"""""""""""""""""""""""""""""""""""""""""""

The typical response to updating a node attribute has the following format

.. code-block:: json

  {
    "message": "..."
  }

However, when updating an attribude of type :code:`queue` the response body has additional information.
When the requesting :code:`queue_action` is :code:`active`, the response will contain

.. code-block:: json

  {
    "message": "...",
    "step": "<selected-step>"
  }

and when the requesting :code:`queue_action` is :code:`no_of_aborted`, the response will contain

.. code-block:: json

  {
    "message": "...",
    "no_of_aborted": "<current-number-of-queued-or-aborted-steps>"
  }

Delete a Node attribute
^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/attributes`
   * - Method
     - :code:`DELETE`
   * - Description
     - **Delete** the attribute of the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"...","name":"..."}`
   * - Response
     - :code:`{"message":""Attribute deleted successfully"}`

Endpoint :code:`/v1/suites/{path}/script`
-----------------------------------------

Obtain the Node script
^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/script`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the script associated to the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"script": "..."}`
   * - Example
     - :code:`curl https://localhost:8080/v1/suites/path/to/node/script`

Update the Node script
^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/script`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** the script associated to the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"script": "..."}`
   * - Response
     - :code:`{"message": "Script updated successfully"}`

Payload to update Node script
"""""""""""""""""""""""""""""

.. warning::

   A script can only be updated through the REST API if it already exists in the server.

.. code-block:: json

  {
    "script" : "..."
  }

Endpoint :code:`/v1/suites/{path}/output`
-----------------------------------------

Obtain the Node job output
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/suites/{path}/output`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the job output associated to the node at :code:`/{path}`
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"job_output": "..."}`
   * - Example
     - :code:`curl https://localhost:8080/v1/suites/path/to/node/output`

Endpoint :code:`/v1/server/status`
----------------------------------

Obtain the Server status
^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/status`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the server status information
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"statistics":{...}}`
   * - Example
     - :code:`curl https://localhost:8080/v1/server/status`

Update the Server status
^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/status`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** the server status (i.e. reload configuration)
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"action":"..."}`
   * - Response
     - :code:`{"message":"Server updated successfully"}`

Payload to update the Server Status
"""""""""""""""""""""""""""""""""""

.. code-block:: json

  {
    "action" : "reload_whitelist_file|reload_passwd_file|reload_custom_passwd_file"
  }

Endpoint :code:`/v1/server/attributes`
--------------------------------------

Create a new Server attribute
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/attributes`
   * - Method
     - :code:`POST`
   * - Description
     - **Create** a new server attribute
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"variable","name":"...","value":".."}`
   * - Response
     - :code:`{"message":"Attribute added successfully"}`

Obtain all Server attributes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/attributes`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the server attributes
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"variables":[...]}`
   * - Example
     - :code:`curl https://localhost:8080/v1/server/attributes`

Update a Server attribute
^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/attributes`
   * - Method
     - :code:`PUT`
   * - Description
     - **Update** a server attribute
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"variable","name":"...","value":"..."}`
   * - Response
     - :code:`{"message":"Attribute changed successfully"}`

Delete a Server attribute
^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/attributes`
   * - Method
     - :code:`DELETE`
   * - Description
     - **Delete** a server attribute
   * - Parameters
     - *none*
   * - Payload
     - :code:`{"type":"variable","name":"..."}`
   * - Response
     - :code:`{"message":"Attribute deleted successfully"}`

Endpoint :code:`/v1/server/ping`
--------------------------------

Ping Server
^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/server/ping`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the rount-trip-time between ecflow_http and ecflow_server
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"host":"...","round-trip-time":"..."}`
   * - Example
     - :code:`curl https://localhost:8080/v1/server/ping`

Endpoint :code:`/v1/statistics`
-------------------------------

Obtain Server statistics
^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :stub-columns: 1
   :width: 100%
   :widths: 20 80

   * - Endpoint
     - :code:`/v1/statistics`
   * - Method
     - :code:`GET`
   * - Description
     - **Read** the ecflow_http statistics
   * - Parameters
     - *none*
   * - Payload
     - *empty*
   * - Response
     - :code:`{"num_requests":"...","num_errors":"..."}`
   * - Example
     - :code:`curl https://localhost:8080/v1/statistics`

OpenAPI Specification
---------------------

The OpenAPI specification file is available at `openapi.yaml <https://github.com/ecmwf/ecflow/blob/develop/Http/doc/openapi.yaml>`_.

.. note::

  The OpenAPI specification allows the graphical visualisation of the supported endpoints, using Swagger UI.

  To run Swagger UI in a container, use the following Docker file:

  .. code-block::

    FROM swaggerapi/swagger-ui
    ADD openapi.yaml /tmp
    ENV SWAGGER_JSON=/tmp/openapi.yaml

More Examples
-------------

Authentication options
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

  curl [...] https://localhost:8080/v1/suites -H 'authorization: Bearer <MYTOKEN>'
  curl [...] https://localhost:8080/v1/suites -H 'x-api-key: <MYTOKEN>'
  curl [...] https://localhost:8080/v1/suites?key=<MYTOKEN>

Create a new family
^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

  curl -X PUT https://localhost:8080/v1/suites/test -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"definition": "family b\nendfamily\n"}'

Create a new attribute
^^^^^^^^^^^^^^^^^^^^^^

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
^^^^^^^^^^^^^^^^^^^^^^^^^

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
^^^^^^^^^^^^^

.. code-block:: bash
    
  curl -X PUT https://localhost:8080/v1/suites/test/status -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"action":"complete"}'
  curl -X PUT https://localhost:8080/v1/suites/test/status -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>' -d '{"action":"requeue"}'


Delete an attribute
^^^^^^^^^^^^^^^^^^^

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
^^^^^^^^^^^^^^

.. code-block:: bash

  curl -X DELETE https://localhost:8080/v1/suites/test/definition -H 'content-type: application/json' -H 'authorization: Bearer <MYTOKEN>'


Implementation Details
======================

:code:`ecflow_http` is basically a wrapper that transforms requests in web-syntax to
ecflow syntax, and similarly transforms the results from plain-text to
valid json.

:code:`ecflow_http` is internally using the normal ClientInvoker method to
communicate with the server. From the ecFlow servers' point of view the
API is just another client.

:code:`ecflow_http` can done some things that the command line tool ecflow_client
cannot, mostly to enable adding attributes to existing suites.
ecflow_client can do to this to some attributes, but the API has a
broader support. The API also caches the server state and updates it
only in certain configurable intervals.

:code:`ecflow_http` will keep a cached copy of definitions in its memory. The copy
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

:code:`ecflow_http` uses two external libraries, both libraries are header only
and licensed with MIT license:

-  `cpp-httplib <https://github.com/yhirose/cpp-httplib>`__: provides
   http server implementation.

-  `nlohmann/json <https://github.com/nlohmann/json>`__: provides json
   encoding/decoding functions

:code:`ecflow_http` supports the usual REST API versioning, meaning that the current
version is "v1" and that version number is a part of the URL. :code:`ecflow_http`
can support multiple different version side-by-side. The v1 code is
basically split into two files: **ApiV1.hpp/cpp**, and
**ApiV1Impl.hpp/cpp**. The first one registers the endpoints used to the
HTTP server and deals with all the HTTP specific things. The latter
(ApiV1Impl) contains all business logic: contacting ecflow server and
formulating requests/responses.

Compiled successfully with following compilers (CMAKE_BUILD_TYPE=Debug):

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
^^^^^^^^^^^^^^^^^^^^^

By default :code:`ecflow_http` will increase the update interval length for
ecFlow server if the :code:`ecflow_http` server is inactive. This is called drift.

For every one minutes that goes by without requests from users, the
update interval (given with :code:`--polling_interval`, default value 10
seconds) is increased linearly by one second. The default maximum value
is 300 seconds. Whenever :code:`ecflow_http` receives a request from user, the
update interval value is reset to normal value.

The maximum polling interval can changed with command line option
:code:`--max_polling_interval`. If drift is enabled, the minimum value is hard
coded to 30 seconds.
