#!/usr/bin/env python3

import glob
import os
import re

import yaml

"""

  This script is used to patch the generated rst files to replace glossary items with the correct Sphinx directives.

  The docstrings for all ecFlow Python API are extracted into separate files, using the sphinx.ext.autodoc feature.
  Then, the list of ecFlow related terms is loaded from `glossary.yaml`, and these terms are used to filter out
  the `*.rst` files to correct all the references (making the file fully valid ReStructuredText).  

"""

def fix_glossary():
    with open("glossary.yaml", "r") as f:
        glossary_items = yaml.load(f, Loader=yaml.FullLoader)

    for fname in glob.glob("reference/*.rst"):
        if os.path.basename(fname)[0].isupper():
            t = ""
            with open(fname, "r") as f:
                t = f.read()
                if t:
                    for item in glossary_items:
                        t = re.sub(rf"`{item}`_", rf":term:`{item}`", t)
                    for item in ['cron definition<text_based_def_cron>']:
                        t = re.sub(rf"`{item}`_", rf":ref:`{item}`", t)

            if t:
                with open(fname, "w") as f:
                    f.write(t)


fix_glossary()
