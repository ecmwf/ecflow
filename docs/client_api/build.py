#!/usr/bin/env python3

import re
import pathlib
import subprocess

cmd_type_terms = {"child": "child command", "user": "user command", "option": ""}


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


def render_single_page_rst(name):
    o, _ = run_command(["ecflow_client", f"--help={name}"])

    # prefix each line of the output with the necessary indentation
    o = '\n'.join(['   ' + line for line in o.split('\n')]) + '\n'

    title = f"{name}"
    txt = f"""
.. _{name}_cli:

{title}
{"/" * len(title)}

::

"""

    txt += o
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

The command line interface is provided by the :term:`ecflow_client`
executable. Note that most of the commands that you
execute using :ref:`ecflow_ui` are actually CLI commands. 

The very first argument to :term:`ecflow_client` specifies the command and must be prefixed with ``--``, e.g. ``--load`` in the example below:

.. code-block:: shell

    ecflow_client --load=host1.3141.check

The comprehensive :ref:`list of ecflow_client commands <ecflow_client_commands>` is presented below.
These commands can be combined with :ref:`ecflow_client common options <ecflow_client_options>` to further customise the
:term:`ecflow_client` behaviour.

The list of commands, amongst other details, can be displayed by using the option ``--help``.

.. code-block:: shell

    ecflow_client --help


Some pages about CLI specific details can be found here:

.. toctree::
    :maxdepth: 1
    
    desc/index.rst
"""

    txt += """

.. list-table:: List of :term:`ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 20 60
    :name: ecflow_client_commands

    * - Command
      - Type
      - Description
    """

    command_entries = load_commands()
    for entry in command_entries:
        txt += f"""
    * - :ref:`{entry.name}_cli` 
      - :term:`{cmd_type_terms[entry.type]}`
      - {entry.desc}
"""

    txt += f"""

.. list-table:: List of common options for `ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 80
    :name: ecflow_client_options

    * - Option
      - Description
"""

    option_entries = load_options()
    for entry in option_entries:
        txt += f"""
    * - :ref:`{entry.name}_cli`
      - {entry.desc}
"""

    txt += f"""

.. toctree::
    :maxdepth: 1
    :hidden:
    
"""

    for entry in command_entries:
        txt += f"""    {entry.name} <api/{entry.name}.rst>\n"""
    for entry in option_entries:
        txt += f"""    {entry.name} (option) <api/{entry.name}.rst>\n"""

    return txt


if __name__ == "__main__":

    # Render and store index.rst
    content = render_index_rst()
    with open("index.rst", "w") as f:
        f.write(content)

    # Ensure api sub-folders is present
    pathlib.Path("api").mkdir(parents=True, exist_ok=True)

    # Render and store each of the command/option.rst
    entries = load_commands()
    for entry in entries:
        content = render_single_page_rst(entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)

    entries = load_options()
    for entry in entries:
        if entry.name == "user":
            content = render_user_option_rst()
        else:
            content = render_single_page_rst(entry.name)
        with open(f"api/{entry.name}.rst", "w") as f:
            f.write(content)
