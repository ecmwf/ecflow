#!/usr/bin/env python3

"""

  This script generates the Python API reference pages (reference/*.rst) by introspecting
  the ecflow module.

  Each public class of the ecflow module produces one reference/<ClassName>.rst page,
  containing the class directive, its bases and docstring, followed by one directive per
  public member (method, property or attribute), ordered alphabetically. The signature
  embedded by the bindings in the first line of each docstring is used as the directive
  signature, mirroring how sphinx.ext.autodoc renders these objects.

  The ecflow module location must be provided via PYTHONPATH (the ecflow_docs CMake
  target does this). The pages are written to the reference/ directory under the current
  working directory.

"""

import re
from pathlib import Path

try:
    import ecflow
except ImportError as e:
    raise SystemExit(
        "extract_api.py: unable to import the ecflow module, needed for docstring extraction. "
        "Set PYTHONPATH to the pyext build directory (e.g. <build>/libs/pyext/python3)."
    ) from e

MODULE = "ecflow"
OUTPUT_DIR = Path("reference")


def describe_value(value):
    """Render an attribute value like sphinx.util.inspect.object_description() does."""
    if isinstance(value, dict):
        try:
            items = sorted(value.items())
        except TypeError:
            items = value.items()  # unsortable keys; keep insertion order
        return "{%s}" % ", ".join(
            f"{describe_value(k)}: {describe_value(v)}" for k, v in items
        )
    return repr(value)


def split_docstring_signature(name, doc):
    """Split a bindings docstring into (signature, body lines).

    The bindings embed the signature in the first docstring line, e.g.
    "day(self: ecflow.Day) -> ecflow.Days"; return it (without the leading name)
    and the remaining lines, or (None, lines) when there is no signature line.
    """
    lines = docstring_lines(doc)
    if lines and re.match(rf"^{re.escape(name)}\(.*\)( -> .+)?$", lines[0]):
        signature = lines[0][len(name) :]
        body = lines[1:]
        if body and not body[0].strip():
            body = body[1:]
        return signature, body
    return None, lines


def docstring_lines(doc):
    """Normalise a docstring like sphinx.util.docstrings.prepare_docstring() does."""
    lines = (doc or "").expandtabs().splitlines()
    margin = min(
        (len(line) - len(line.lstrip()) for line in lines[1:] if line.strip()), default=0
    )
    lines = lines[:1] + [line[margin:] for line in lines[1:]]
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def member_lines(class_name, name, descriptor):
    """Return the reST lines documenting one class member."""
    if isinstance(descriptor, property):
        _, body = split_docstring_signature(name, descriptor.__doc__)
        lines = ["", "", f".. py:property:: {class_name}.{name}", f"   :module: {MODULE}"]
    elif isinstance(descriptor, staticmethod):
        signature, body = split_docstring_signature(name, descriptor.__func__.__doc__)
        lines = [
            "",
            "",
            f".. py:method:: {class_name}.{name}{signature or '()'}",
            f"   :module: {MODULE}",
            "   :staticmethod:",
        ]
    elif callable(descriptor):
        signature, body = split_docstring_signature(name, descriptor.__doc__)
        lines = [
            "",
            "",
            f".. py:method:: {class_name}.{name}{signature or '()'}",
            f"   :module: {MODULE}",
        ]
    else:
        return [
            "",
            "",
            f".. py:attribute:: {class_name}.{name}",
            f"   :module: {MODULE}",
            f"   :value: {describe_value(descriptor)}",
        ]

    if any(line.strip() for line in body):
        lines += [""] + body
    return lines


def class_lines(name, cls):
    """Return the reST lines for one class page."""
    title = f"{MODULE}.{name}"
    bases = ", ".join(f":py:class:`~{b.__module__}.{b.__qualname__}`" for b in cls.__bases__)
    lines = [
        title,
        "/" * len(title),
        "",
        "",
        f".. py:class:: {name}",
        f"   :module: {MODULE}",
        "",
        f"   Bases: {bases}",
        "",
    ]
    lines += docstring_lines(cls.__doc__)

    for member_name in sorted(vars(cls)):
        if not member_name.startswith("_"):
            lines += member_lines(name, member_name, vars(cls)[member_name])
    return lines


def generate():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for name in sorted(dir(ecflow)):
        # Intentionally skip module-level functions for now. The only current
        # public lowercase member is debug_build(), which we do not publish in
        # the generated reference yet.
        if name.startswith("_") or not name[0].isupper():
            continue
        cls = getattr(ecflow, name)
        if not isinstance(cls, type):
            raise SystemExit(f"extract_api.py: unexpected non-class module member: {name}")
        lines = class_lines(name, cls)
        content = "\n".join(lines) + ("\n" if not lines[-1] else "\n\n")
        (OUTPUT_DIR / f"{name}.rst").write_text(content)


if __name__ == "__main__":
    generate()
