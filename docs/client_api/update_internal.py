#!/usr/bin/env python3
"""
Refresh the ``Help`` blocks in command_internals.rst from help.json.

``command_internals.rst`` is a hand-maintained developer reference.

Each per-command section carries a ``.. tab:: Help`` block that reproduces the
``ecflow_client --help=<command>`` text (without the common environment-variable
footer). That text is the one part that must stay in step with the help manifest;
this script rewrites it in place from docs/client_api/help.json, leaving the
surrounding prose, tables and the other tabs (Usage, Request, Reply) untouched.

The update is content-only and markup-driven: command names are read from the
existing markup (the code-block's first line, or the panel ids of the HTML
selector), and only the rendered help text is replaced. Adding or removing a
command from a selector remains a manual edit.

Two Help-block shapes are handled:

 * single-command sections use a ``.. code-block:: none`` block;
 * multi-command sections use a ``.. raw:: html`` selector with one ``<pre>``
   panel per action.
"""
import html
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from build import join_description, load_manifest  # noqa: E402

RST = HERE / "command_internals.rst"
BODY_INDENT = " " * 12  # indentation of code-block / raw-html content


def help_text(command):
    """Render the ``--help=<command>`` body: name banner plus the joined description."""
    name = command["name"]
    return f"{name}\n{'-' * len(name)}\n\n{join_description(command['description'])}"


def render_code_block(command):
    """Return the code-block body lines (single-command Help tab), indented for the source."""
    return [BODY_INDENT + line if line else "" for line in help_text(command).split("\n")]


def render_pre(open_tag, command):
    """Return the ``<pre>`` panel lines (multi-command Help tab), HTML-escaped and indented.

    @param open_tag The verbatim ``            <pre id="..." ...>`` opening (indent included).
    """
    escaped = html.escape(help_text(command), quote=True).split("\n")
    lines = [open_tag + escaped[0]]
    for line in escaped[1:-1]:
        lines.append(BODY_INDENT + line if line else "")
    last = escaped[-1]
    lines.append((BODY_INDENT + last if last else "") + "</pre>")
    return lines


def indent_of(line):
    return len(line) - len(line.lstrip(" "))


def replace_help_block(lines, start, commands):
    """Rewrite the Help tab that begins at ``lines[start]`` (``.. tab:: Help``).

    @param commands Mapping of command name to its manifest entry.
    @return The index one past the rewritten block (unchanged length otherwise).
    """
    # The block runs until the next tab (``.. tab::`` at 4 spaces) or a dedent out of the tab.
    end = start + 1
    while end < len(lines):
        stripped = lines[end].strip()
        if stripped.startswith(".. tab::") and indent_of(lines[end]) == 4:
            break
        if stripped and indent_of(lines[end]) < 4:
            break
        end += 1

    block = lines[start:end]

    raw_at = next((i for i, l in enumerate(block) if l.strip() == ".. raw:: html"), None)
    code_at = next((i for i, l in enumerate(block) if l.strip() == ".. code-block:: none"), None)

    if raw_at is not None:
        new_block = _rewrite_raw_html(block, raw_at, commands)
    elif code_at is not None:
        new_block = _rewrite_code_block(block, code_at, commands)
    else:
        raise ValueError(f"Help tab at line {start + 1} has neither a code-block nor a raw-html body")

    lines[start:end] = new_block
    return start + len(new_block)


def _lookup(commands, name, where):
    command = commands.get(name)
    if command is None:
        raise KeyError(f"{where}: '{name}' is not a command in help.json")
    return command


def _rewrite_code_block(block, code_at, commands):
    # Body is everything after the code-block directive and its blank line.
    body_start = code_at + 1
    while body_start < len(block) and block[body_start].strip() == "":
        body_start += 1
    body_end = len(block)
    while body_end > body_start and block[body_end - 1].strip() == "":
        body_end -= 1

    name = next((l.strip() for l in block[body_start:body_end] if l.strip()), None)
    if name is None:
        raise ValueError("empty code-block Help body")
    command = _lookup(commands, name, "code-block Help")

    head = block[:code_at + 1] + [""]
    tail = block[body_end:]  # trailing blank line(s) before the next tab
    return head + render_code_block(command) + tail


_PRE_OPEN = re.compile(r'^(\s*<pre id="[^"]*"[^>]*>)(.*)$')


def _rewrite_raw_html(block, raw_at, commands):
    out = list(block[:raw_at + 1])
    i = raw_at + 1
    while i < len(block):
        line = block[i]
        m = _PRE_OPEN.match(line)
        if not m:
            out.append(line)
            i += 1
            continue
        open_tag, first = m.group(1), m.group(2)
        name = html.unescape(first).strip()
        # consume the panel body up to and including the line closing </pre>
        j = i
        while "</pre>" not in block[j]:
            j += 1
        command = _lookup(commands, name, "raw-html panel")
        out.extend(render_pre(open_tag, command))
        i = j + 1
    return out


def main():
    manifest = load_manifest()
    commands = {c["name"]: c for c in manifest["commands"]}

    lines = RST.read_text(encoding="utf-8").split("\n")
    updated = 0
    i = 0
    while i < len(lines):
        if lines[i].strip() == ".. tab:: Help":
            i = replace_help_block(lines, i, commands)
            updated += 1
        else:
            i += 1

    RST.write_text("\n".join(lines), encoding="utf-8")
    print(f"{RST.name}: refreshed {updated} Help tab(s) from help.json")


if __name__ == "__main__":
    main()
