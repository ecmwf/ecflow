#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

"""

  This script generates the Rust API reference (reference.inc), included by
  rust_api.rst, from the documentation comments in the sources of the ecflow crate.

  Each public item of the ecflow crate (type or function) produces one section,
  containing its declaration and documentation, followed by one entry per public
  member (method, variant or field), in source order. The methods of a type are
  grouped under the section comments of its impl block. The declaration in the
  sources is used as the entry signature.

  The reference is written to the current working directory.

"""

import re
from pathlib import Path

# The tags below are emitted into the generated pages, not applied to this
# file; the fence keeps the licence tooling from reading them as its own.
# REUSE-IgnoreStart
SPDX_HEADER = (
    ".. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)\n"
    ".. SPDX-License-Identifier: Apache-2.0\n"
    "\n"
)
# REUSE-IgnoreEnd

OUTPUT = Path("reference.inc")
CRATES_DIR = Path(__file__).resolve().parents[2] / "rust" / "crates"

# The sources of the ecflow crate, and the types it re-exports from ecflow-sys.
SOURCES = [
    (CRATES_DIR / "ecflow" / "src" / "client.rs", None),
    (CRATES_DIR / "ecflow" / "src" / "error.rs", None),
    (CRATES_DIR / "ecflow-sys" / "src" / "lib.rs", {"Zombie"}),
]

# The items that come first; the others follow in source order.
FIRST = ["Client"]

GROUP = re.compile(r"^// =+ (.+?) =+$")
TYPE = re.compile(r"^(pub )?(struct|enum) (\w+)")
ALIAS = re.compile(r"^pub type (\w+).*;$")
IMPL = re.compile(r"^impl (\w+) \{$")
FUNCTION = re.compile(r"^pub (const )?fn (\w+)")
VARIANT = re.compile(r"^(\w+),$")
FIELD = re.compile(r"^(pub )?(\w+): (.+),$")
LINK = re.compile(r"\[`([\w:]+)`\]")
LITERAL = re.compile(r"(?<!`)`([^`]+)`(?!`)(?=(\w?))")


def signature(lines, start):
    """Join the lines of a function declaration, up to the brace opening its body."""
    parts = []
    for line in lines[start:]:
        text = line.strip()
        if text.endswith("{"):
            parts.append(text[:-1].strip())
            break
        parts.append(text)
    joined = " ".join(part for part in parts if part)
    return joined.replace("( ", "(").replace(", )", ")").rstrip(",")


def indent_of(line):
    return line[: len(line) - len(line.lstrip())]


def parse(path, only):
    """Return the public items of a source file as {name: (declaration, doc, members)}.

    A member is (group, name, declaration, doc). When `only` is given, the file is a
    cxx bridge: just the types it names are returned, with all their fields.
    """
    lines = path.read_text().splitlines()
    items = {}
    doc = []
    members = None  # the members of the type whose braces enclose the current line
    closing = None  # the line closing those braces
    is_enum = False
    group = None
    for number, line in enumerate(lines):
        text = line.strip()
        if text.startswith("///"):
            doc.append(text[3:].removeprefix(" "))
            continue
        if text.startswith("#["):
            continue

        indent = indent_of(line)
        if members is not None and line == closing:
            members = None
        elif members is None:
            closing = indent + "}"
            if match := TYPE.match(text):
                public, kind, name = match.groups()
                if name in only if only is not None else public:
                    items[name] = (f"pub {kind} {name}", doc, [])
                    members, is_enum, group = items[name][2], kind == "enum", None
            elif match := IMPL.match(text):
                members = items.get(match.group(1), (None, None, None))[2]
                is_enum, group = False, None
            elif only is None and (match := ALIAS.match(text)):
                items[match.group(1)] = (text.rstrip(";"), doc, [])
            elif only is None and not indent and (match := FUNCTION.match(text)):
                items[match.group(2)] = (signature(lines, number), doc, [])
        elif match := GROUP.match(text):
            group = match.group(1)
        elif is_enum and (match := VARIANT.match(text)):
            members.append((group, match.group(1), match.group(1), doc))
        elif line.startswith(indent_of(closing) + "    pub ") and (match := FUNCTION.match(text)):
            members.append((group, match.group(2), signature(lines, number), doc))
        elif indent == indent_of(closing) + "    " and (match := FIELD.match(text)):
            public, name, kind = match.groups()
            if public or only is not None:
                members.append((group, name, f"{name}: {kind}", doc))
        doc = []
    return items


def label(*names):
    return "rust_" + "_".join(name.lower() for name in names)


def doc_lines(doc, labels):
    """Convert the Markdown of a documentation comment to reST lines."""

    def link(path):
        target = label(*path.split("::"))
        return f":ref:`{path} <{target}>`" if target in labels else f"``{path}``"

    def literal(match):
        # An inline literal followed by a word character needs an escaped space
        return f"``{match.group(1)}``" + ("\\ " if match.group(2) else "")

    def inline(text):
        pieces = LINK.split(text)
        for index, piece in enumerate(pieces):
            pieces[index] = link(piece) if index % 2 else LITERAL.sub(literal, piece)
        return "".join(pieces)

    lines = []
    in_code = False
    for line in doc:
        if line.startswith("```"):
            in_code = not in_code
            lines += [".. code-block:: rust", ""] if in_code else [""]
        elif in_code:
            # Lines starting with "# " are hidden by rustdoc
            if not line.startswith("# "):
                lines.append(f"   {line}")
        elif line.startswith("# "):
            lines.append(f".. rubric:: {line[2:]}")
        else:
            lines.append(inline(line))
    return lines


def entry_lines(target, declaration, doc, labels):
    """Return the reST lines for one declaration and its documentation."""
    lines = ["", f".. _{target}:", "", f".. describe:: {declaration}", ""]
    return lines + [f"   {line}" for line in doc_lines(doc, labels)]


def item_lines(name, declaration, doc, members, labels):
    """Return the reST lines for the section of one item."""
    lines = ["", name, "-" * len(name)]
    lines += entry_lines(label(name), declaration, doc, labels)

    group = None
    for member_group, member_name, member_declaration, member_doc in members:
        if member_group != group:
            group = member_group
            lines += ["", group, "^" * len(group)]
        lines += entry_lines(label(name, member_name), member_declaration, member_doc, labels)
    return lines


def generate():
    items = {}
    for path, only in SOURCES:
        items.update(parse(path, only))

    labels = {label(name) for name in items}
    labels |= {label(name, member[1]) for name in items for member in items[name][2]}

    lines = []
    for name in FIRST + [name for name in items if name not in FIRST]:
        declaration, doc, members = items[name]
        lines += item_lines(name, declaration, doc, members, labels)
    content = "\n".join(line.rstrip() for line in lines).strip("\n") + "\n"
    OUTPUT.write_text(SPDX_HEADER + content)


if __name__ == "__main__":
    generate()
