#!/usr/bin/env python3
"""Extract trigger/complete expressions from an ecFlow checkpoint (.check) file.

A checkpoint file contains lines of the form:

    trigger <expression>
    trigger -a <expression>
    complete <expression>
    complete -o <expression>

where the optional '-a' (AND) / '-o' (OR) modifier joins the expression to the
node's existing trigger/complete and is not part of the expression itself.

This script scans a single checkpoint file, in source order, and writes every
matching line to a JSON file:

    {
      "schema_version": 1,
      "checkpoint": "<value of --in>",
      "expression_count": <number of expressions>,
      "expressions": [
        {"kind": "trigger", "modifier": null, "expression": "...", "line": 23158},
        ...
      ]
    }

The output file is named after the input file, with the trailing ".check"
suffix replaced by ".expressions.json", and is written into --out-dir.
"""

import argparse
import json
import os
import re
import sys

# Matches a line starting with 'trigger ' or 'complete ' (no leading whitespace),
# an optional '-a'/'-o' modifier, and captures the remaining expression text.
_EXPR_RE = re.compile(r"^(trigger|complete) (?:(-[ao]) )?(.+)$")

# Strips a trailing '# comment' (and the whitespace before it) from an expression,
# e.g. checkpoint files commonly append "# free" after a trigger/complete expression.
_COMMENT_RE = re.compile(r"\s*#.*$")


def extract_from_file(path):
    """Return the list of expressions found in a checkpoint file, in source order.

    Every matching line is kept (no de-duplication); each entry records its
    1-based line number in the source file.
    """
    expressions = []
    # Checkpoint files are treated as raw byte streams: each byte is mapped to
    # the codepoint of the same value (latin-1), regardless of the actual
    # encoding used when the expression was written into the checkpoint.
    with open(path, "r", encoding="latin-1") as handle:
        for line_no, line in enumerate(handle, start=1):
            match = _EXPR_RE.match(line.rstrip("\n"))
            if not match:
                continue
            kind, modifier, expression = match.groups()
            expression = _COMMENT_RE.sub("", expression)
            expressions.append({
                "kind": kind,
                "modifier": modifier,
                "expression": expression,
                "line": line_no,
            })
    return expressions


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--in", dest="input", required=True,
                        help="Checkpoint (.check) file to scan")
    parser.add_argument("--out-dir", dest="out_dir", required=True,
                        help="Directory in which to write the "
                             "<name>.expressions.json file")
    parser.add_argument("--min-count", dest="min_count", type=int, default=0,
                        help="Skip writing the output file when fewer than "
                             "this many expressions are found (default: %(default)s)")
    args = parser.parse_args(argv)

    print(f"---------->{args.input} : {args.out_dir}")

    if not os.path.isfile(args.input):
        print("error: no such file: {!r}".format(args.input), file=sys.stderr)
        return 1

    basename = os.path.basename(args.input)
    if basename.endswith(".check"):
        basename = basename[: -len(".check")]
    out_path = os.path.join(args.out_dir, basename + ".expressions.json")

    expressions = extract_from_file(args.input)

    if len(expressions) < args.min_count:
        print("  {:>7} expressions -> skipped (below --min-count {})".format(
            len(expressions), args.min_count))
        return 0

    os.makedirs(args.out_dir, exist_ok=True)

    document = {
        "schema_version": 1,
        "checkpoint": args.input,
        "expression_count": len(expressions),
        "expressions": expressions,
    }

    with open(out_path, "w", encoding="utf-8") as out:
        json.dump(document, out, indent=2)
        out.write("\n")

    print("  {:>7} expressions -> {}".format(len(expressions), out_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
