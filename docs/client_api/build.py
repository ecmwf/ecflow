#!/usr/bin/env python3
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from validate_help_manifest import check_cross_references, check_schema, load_json  # noqa: E402

cmd_type_terms = {"task": "child command", "user": "user command"}

# Mirrors Help.cpp's CommandFilter::known_options exactly: the set of names for which
# show_command_help() suppresses the environment-variable banner. This disagrees with the
# manifest's own options[] array in both directions ('help'/'version'/'remove' are manifest
# options but are absent here, so they still get the banner; there is no equivalent entry for
# every manifest option) -- a pre-existing Help.cpp quirk tracked as Future Work, not something
# for build.py to paper over.
KNOWN_OPTIONS = {"add", "debug", "host", "password", "port", "rid", "ssl", "user", "http", "https"}

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


class Entry:
    def __init__(self, name, type, desc):
        self.name = name
        self.type = type
        self.desc = desc

    def __repr__(self):
        return f"Entry('{self.name}', '{self.type}', '{self.desc}')"


def load_manifest():
    manifest = load_json(HERE / "help.json")
    schema = load_json(HERE / "help.schema.json")

    schema_errors = check_schema(manifest, schema)
    if schema_errors:
        for error in schema_errors:
            location = "/".join(str(part) for part in error.path) or "<root>"
            print(f"help.json: {location}: {error.message}", file=sys.stderr)
        sys.exit(f"{len(schema_errors)} schema violation(s) found in help.json; aborting docs build.")

    reference_problems = check_cross_references(manifest)
    if reference_problems:
        for problem in reference_problems:
            print(f"help.json: {problem}", file=sys.stderr)
        sys.exit(f"{len(reference_problems)} cross-reference problem(s) found in help.json; aborting docs build.")

    return manifest


def load_commands(manifest):
    # 'internal' commands (e.g. move, constructed on the fly by PlugCmd) are never registered in
    # CtsCmdRegistry and have no CLI-reachable --help output; they are intentionally undocumented,
    # matching today's behaviour.
    visible = [command for command in manifest["commands"] if command["visibility"] != "internal"]
    entries = [Entry(command["name"], command["kind"], command["summary"]) for command in visible]
    return sorted(entries, key=lambda entry: entry.name)


def load_options(manifest):
    entries = [Entry(option["name"], option["kind"], option["summary"]) for option in manifest["options"]]
    return sorted(entries, key=lambda entry: entry.name)


def load_description_file(name):
    description_file = pathlib.Path(__file__).parent / "desc" / f"{name}.description"
    try:
        with open(description_file.resolve(), 'r') as file:
            return file.read()
    except FileNotFoundError:
        return f""  # If the file does not exist, there is no description available
    except IOError:
        return f""


def join_description(lines):
    # Reproduces HelpCatalog::description_for(): the description holds one line per element (a blank
    # line is an empty string), joined by a newline to reconstruct the original text verbatim,
    # including pre-formatted content.
    return "\n".join(lines)


def format_env_var(var):
    required = var["required"]
    if var.get("overridable_by"):
        required += "*"
    return f"  {var['name']} <{var['type']}> [{required}]\n    {var['description']}\n"


def render_client_env_description(env_vars):
    txt = "The client considers, for both user and task commands, the following environment variables:\n\n"
    for var in env_vars:
        if var["applies_to"] == "both":
            txt += format_env_var(var)
    txt += (
        "\nThe options marked with (*) must be specified in order for the client to communicate\n"
        "with the server, either by setting the environment variables or by specifying the\n"
        "command line options.\n"
    )
    return txt


def render_task_env_description(env_vars):
    txt = "The following environment variables are used specifically by task commands:\n\n"
    for var in env_vars:
        if var["applies_to"] == "task":
            txt += format_env_var(var)
    txt += "\nThe scripts are expected to export the mandatory variables, typically in shared include files\n"
    return txt


def render_help_output(manifest, name):
    # Reproduces Documentation::show_command_help()'s output byte-for-byte, without invoking the
    # built ecflow_client binary: the name banner, the manifest description, and (for anything other
    # than an option) the environment-variable sections that show_command_help() appends.
    commands_by_name = {command["name"]: command for command in manifest["commands"]}
    options_by_name = {option["name"]: option for option in manifest["options"]}

    entry = commands_by_name.get(name) or options_by_name[name]
    is_option = name in KNOWN_OPTIONS
    is_task_command = name in commands_by_name and commands_by_name[name]["kind"] == "task"

    description = join_description(entry["description"])
    txt = f"\n{name}\n{'-' * len(name)}\n\n{description}\n\n"
    if not is_option:
        txt += render_client_env_description(manifest["environment_variables"])
        if is_task_command:
            txt += "\n" + render_task_env_description(manifest["environment_variables"])
    return txt


def render_single_page_rst(manifest, name):

    description = load_description_file(name)
    description_title = "Description"
    description_header = "" if not description else f"\n.. rubric:: {description_title}\n\n"

    output = render_help_output(manifest, name)
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

The elements that can appear in a suite definition file (node types and attributes, such as
``trigger`` or ``cron``) are listed and documented the same way, using ``--help=definition``
and ``--help=defs/<item>`` respectively.

.. code-block:: shell

    ecflow_client --help=definition
    ecflow_client --help=defs/trigger


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

    manifest = load_manifest()

    # Render and store index.rst
    content = render_index_rst()
    with open("index.rst", "w") as f:
        f.write(content)

    command_entries = load_commands(manifest)
    with open("cli_commands.rst", "w") as f:
        f.write(render_commands_rst(command_entries))

    option_entries = load_options(manifest)
    with open("cli_options.rst", "w") as f:
        f.write(render_options_rst(option_entries))

    # Ensure api sub-folders is present
    pathlib.Path("api").mkdir(parents=True, exist_ok=True)

    # Render and store each of the command/option.rst
    for entry in command_entries:
        content = render_single_page_rst(manifest, entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)

    for entry in option_entries:
        content = render_single_page_rst(manifest, entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)
