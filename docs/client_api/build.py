#!/usr/bin/env python3
import os
import re
import pathlib
import subprocess

cmd_type_terms = {"child": "child command", "user": "user command", "option": ""}

# Maps each ecflow_client command name to the C++ command class section (anchor) that documents its
# wire-level protocol in command_internals.rst. Several classes implement many command-line options, so the
# mapping is many-to-one. Commands with no server-side protocol section (client-side helpers such as
# 'help'/'version', the '--remove' option of '--complete', and the common options) are intentionally
# absent and receive no cross-link.
PROTOCOL_ANCHORS = {
    # Task (child) commands
    "abort": "AbortCmd",
    "complete": "CompleteCmd",
    "wait": "CtsWaitCmd",
    "event": "EventCmd",
    "init": "InitCmd",
    "label": "LabelCmd",
    "meter": "MeterCmd",
    "queue": "QueueCmd",
    # User commands implemented by a dedicated class
    "alter": "AlterCmd",
    "begin": "BeginCmd",
    "file": "CFileCmd",
    "check_pt": "CheckPtCmd",
    "delete": "DeleteCmd",
    "edit_script": "EditScriptCmd",
    "force": "ForceCmd",
    "free-dep": "FreeDepCmd",
    "group": "GroupCTSCmd",
    "load": "LoadDefsCmd",
    "log": "LogCmd",
    "msg": "LogMessageCmd",
    "order": "OrderNodeCmd",
    "plug": "PlugCmd",
    "query": "QueryCmd",
    "replace": "ReplaceNodeCmd",
    "requeue": "RequeueNodeCmd",
    "run": "RunNodeCmd",
    "server_version": "ServerVersionCmd",
    "show": "ShowCmd",
    # CtsCmd: argument-less server signals
    "ping": "CtsCmd",
    "restart": "CtsCmd",
    "halt": "CtsCmd",
    "shutdown": "CtsCmd",
    "terminate": "CtsCmd",
    "restore_from_checkpt": "CtsCmd",
    "reloadwsfile": "CtsCmd",
    "reloadpasswdfile": "CtsCmd",
    "reloadcustompasswdfile": "CtsCmd",
    "force-dep-eval": "CtsCmd",
    "zombie_get": "CtsCmd",
    "stats": "CtsCmd",
    "stats_reset": "CtsCmd",
    "stats_server": "CtsCmd",
    "suites": "CtsCmd",
    "server_load": "CtsCmd",
    "debug_server_on": "CtsCmd",
    "debug_server_off": "CtsCmd",
    # PathsCmd: node operations
    "suspend": "PathsCmd",
    "resume": "PathsCmd",
    "kill": "PathsCmd",
    "status": "PathsCmd",
    "check": "PathsCmd",
    "edit_history": "PathsCmd",
    "archive": "PathsCmd",
    "restore": "PathsCmd",
    # CtsNodeCmd: node/definition queries
    "get": "CtsNodeCmd",
    "get_state": "CtsNodeCmd",
    "why": "CtsNodeCmd",
    "migrate": "CtsNodeCmd",
    "job_gen": "CtsNodeCmd",
    "checkJobGenOnly": "CtsNodeCmd",
    # ClientHandleCmd: client handle management
    "ch_register": "ClientHandleCmd",
    "ch_drop": "ClientHandleCmd",
    "ch_drop_user": "ClientHandleCmd",
    "ch_add": "ClientHandleCmd",
    "ch_rem": "ClientHandleCmd",
    "ch_auto_add": "ClientHandleCmd",
    "ch_suites": "ClientHandleCmd",
    # CSyncCmd: client/server synchronisation
    "news": "CSyncCmd",
    "sync": "CSyncCmd",
    "sync_full": "CSyncCmd",
    "sync_clock": "CSyncCmd",
    # ZombieCmd: zombie resolution
    "zombie_fob": "ZombieCmd",
    "zombie_fail": "ZombieCmd",
    "zombie_adopt": "ZombieCmd",
    "zombie_remove": "ZombieCmd",
    "zombie_block": "ZombieCmd",
    "zombie_kill": "ZombieCmd",
}


def run_command(cmd):
    print(f"*** Executing: {cmd}")
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.stdout, p.stderr


class Entry:
    def __init__(self, name, type, desc):
        self.name = name
        self.type = type
        self.desc = desc

    def __repr__(self):
        return f"Entry('{self.name}', '{self.type}', '{self.desc}')"


def load_help_table(command):
    entries = []
    o, _ = run_command(command)
    o = o.split("\n")
    for line in o:
        m = re.search(r"""(\S+)\s+(child|user|option)\s+(.*)""", line)
        if m and m.groups and len(m.groups()) == 3:
            entry = Entry(m.group(1), m.group(2), m.group(3))
            entries.append(entry)
    return entries


def load_commands():
    return load_help_table(["ecflow_client", "--help=summary"])


def load_options():
    return load_help_table(["ecflow_client", "--help=option"])


def load_description_file(name):
    description_file = pathlib.Path(__file__).parent / "desc" / f"{name}.description"
    try:
        with open(description_file.resolve(), 'r') as file:
            return file.read()
    except FileNotFoundError:
        return f""  # If the file does not exist, there is no description available
    except IOError:
        return f""


def render_single_page_rst(name):

    description = load_description_file(name)
    description_title = "Description"
    description_header = "" if not description else f"\n.. rubric:: {description_title}\n\n"

    output, _ = run_command(["ecflow_client", f"--help={name}"])
    output = '\n'.join(['   ' + line for line in output.split('\n')]) + '\n' # prefix each line of the output with the necessary indentation
    output_title = f"Output of :code:`--help={name}`"
    output_header = f"\n.. rubric:: {output_title}\n\n"

    # Cross-link to the wire-level protocol reference, when a matching section exists
    protocol_class = PROTOCOL_ANCHORS.get(name)
    protocol_seealso = "" if not protocol_class else f"""
.. seealso::

   :ref:`Protocol reference for {protocol_class} <proto_{protocol_class}>` — the request/reply
   payloads exchanged with the server and the C++ command class that implements this command.
"""

    title = f"{name}"
    txt = f"""
.. _{name}_cli:

{title}
{"*" * len(title)}

{description_header}

{description}

{output_header}

The following help text is generated by :code:`ecflow_client --help={name}`

::

{output}
{protocol_seealso}
"""

    return txt


def render_user_option_rst():
    title = f"user"
    txt = f"""
.. _user_cli:

{title}
{"/" * len(title)}

::

   
   user
   ----
   
   Specifies the user name used to contact the server. Must be used in combination with option --password.
   
   
"""
    return txt


def render_index_rst():
    title = "Command line interface (CLI)"
    txt = f"""
.. _ecflow_cli:

{title}
{"/" * len(title)} 

The :term:`ecFlow command line interface (CLI) <ecflow_client>` is provided by the :code:`ecflow_client` executable.
A large number of commands/options enabled by the :ref:`ecflow_ui` are also available as CLI commands.

:code:`ecflow_client` accepts a variety of commands, specified as ``--<command>``.
For example, the command :code:`--load` can be used, as in the example below, to load the given file into the server.

.. code-block:: shell

    ecflow_client --load host1.3141.check

The comprehensive :ref:`list of ecflow_client commands <ecflow_client_commands>` is presented below.
These commands can be combined with :ref:`ecflow_client common options <ecflow_client_options>` to further customise the
:term:`ecflow_client` behaviour.

.. rubric:: Getting help

The list of commands, amongst other details, can be displayed by using the option ``--help``.

.. code-block:: shell

    ecflow_client --help


.. toctree::
    :maxdepth: 1
    
    desc/cli_option_overriding
    desc/cli_scripting_in_batch
    desc/using_backup_servers

----

.. toctree::
    :maxdepth: 1

    cli_commands
    cli_options
    command_internals
"""

    return txt


def render_commands_rst(entries):
    txt = """
Commands
========

.. list-table:: List of :term:`ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 20 60
    :name: ecflow_client_commands

    * - Command
      - Type
      - Description
"""

    for entry in entries:
        txt += f"""
    * - :ref:`{entry.name}_cli`
      - :term:`{cmd_type_terms[entry.type]}`
      - {entry.desc}
"""

    txt += """

.. toctree::
    :maxdepth: 1
    :hidden:

"""

    for entry in entries:
        txt += f"""    {entry.name} <api/{entry.name}.rst>\n"""

    return txt


def render_options_rst(entries):
    txt = """
Options
=======

.. list-table:: List of common options for `ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 80
    :name: ecflow_client_options

    * - Option
      - Description
"""

    for entry in entries:
        txt += f"""
    * - :ref:`{entry.name}_cli`
      - {entry.desc}
"""

    txt += """

.. toctree::
    :maxdepth: 1
    :hidden:

"""

    for entry in entries:
        txt += f"""    {entry.name} (option) <api/{entry.name}.rst>\n"""

    return txt


if __name__ == "__main__":

    # Render and store index.rst
    content = render_index_rst()
    with open("index.rst", "w") as f:
        f.write(content)

    command_entries = load_commands()
    with open("cli_commands.rst", "w") as f:
        f.write(render_commands_rst(command_entries))

    option_entries = load_options()
    with open("cli_options.rst", "w") as f:
        f.write(render_options_rst(option_entries))

    # Ensure api sub-folders is present
    pathlib.Path("api").mkdir(parents=True, exist_ok=True)

    # Render and store each of the command/option.rst
    for entry in command_entries:
        content = render_single_page_rst(entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)

    for entry in option_entries:
        if entry.name == "user":
            content = render_user_option_rst()
        else:
            content = render_single_page_rst(entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)
