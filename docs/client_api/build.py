import os
import re

RST_DIR = "api"


def build_rst(name):
    os.system(f"ecflow_client --help={name} > help_cmd.txt")
    with open("help_cmd.txt", "r") as f:
        title = f"{name}"
        txt = f"""
.. _{name}_cli:

{title}
{"/" * len(title)}

::

"""

        for line in f.read().split("\n"):
            txt += f"   {line}\n"

    with open(f"{RST_DIR}/{name}.rst", "w") as f:
        f.write(txt)


def build():
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


:numref:`Table %s <cli_command_table>` below shows the full list of the available commands. You can also get it via the ``--help`` switch:

.. code-block:: shell

    ecflow_client --help


Some pages about some specific details about the CLI can be find here:

.. toctree::
    :maxdepth: 1
    
    desc/index.rst


.. list-table:: The list of available CLI commands
    :header-rows: 1
    :widths: 10 10 80
    :name: cli_command_table

    * - Command
      - Type
      - Description
    """

    cmd_type_terms = {"child": "child command", "user": "user command"}

    names = []
    os.system("ecflow_client --help=summary > help.txt")
    with open("help.txt", "r") as f:
        t = f.read()
        _, _, t = t.rpartition("Ecflow client commands:")
        for line in t.split("\n"):
            line = line.strip()
            if line != "":
                m = re.match(r"""(\S+)\s+(\S+)\s+([\S,\s]+)""", line)
                if m and m.groups and len(m.groups()) == 3:
                    name = m.group(1).strip()
                    cmd_type = m.group(2).strip()
                    desc = m.group(3).strip()
                    names.append(name)
                    build_rst(name)
                    txt += f"""
    * - :ref:`{name}_cli` 
      - :term:`{cmd_type_terms[cmd_type]}`
      - {desc}                
        """

    txt += f"""

.. toctree::
    :maxdepth: 1
    :hidden:

"""

    for name in names:
        txt += f"""    {RST_DIR}/{name}.rst\n"""

    with open("index.rst", "w") as f:
        f.write(txt)


build()
