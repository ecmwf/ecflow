#!/usr/bin/env python3

import glob
import os
import re

import yaml


def fix_glossary():
    with open("glossary.yaml", "r") as f:
        glossary_items = yaml.load(f, Loader=yaml.FullLoader)

    for fname in glob.glob("rst/*.rst"):
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
