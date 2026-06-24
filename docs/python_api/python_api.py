#!/usr/bin/env python3

import glob
import os
import pathlib
import yaml

"""

  This script creates the `python_api.rst` file, which is used to generate the Python API documentation.

"""

RST_DIR = (pathlib.Path() / "reference").absolute()


def check_usage(names):
    # check if each toc item has a corresponding rst file
    for name in names:
        if not os.path.exists(f"{RST_DIR}/{name}.rst"):
            print(f"toc item={name}: no rst file found!")

    # check if rest file has a corresponding toc item
    for fname in glob.glob(f"{RST_DIR}/*.rst"):
        name = os.path.basename(fname)
        if name[0].isupper():
            name, _, _ = name.rpartition(".")
            if name not in names:
                print(f"file={fname}: no toc item found!")


def build_toc():
    title = "Python API"
    t = f"""
.. _python_api:

{title}
{"*" * len(title)}

"""
    names = set()
    with open("categories.yaml", "r") as f:
        conf = yaml.load(f, Loader=yaml.FullLoader)
        for category in conf:
            title = category["title"]
            intro = category["intro"]
            t += f"""
{title}
{"=" * len(title)}

{intro}

.. toctree::
   :maxdepth: 1
   :glob:

"""

            for name in category["items"]:
                t += f"""   reference/{name}\n"""

            t +="""

.. contents::
   :depth: 2
   :local:
   :backlinks: top
    
"""

    with open("python_api.rst", "w") as f:
        f.write(t)

    check_usage(names)


build_toc()
