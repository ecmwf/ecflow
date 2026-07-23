#!/usr/bin/env python3
"""Validate docs/client_api/help.json against docs/client_api/help.schema.json.

Beyond JSON Schema conformance, this also checks the cross-referential rules that a
schema alone cannot express: pairs_with symmetry, command-option -> command
linkage, and name uniqueness within each array.
"""
import argparse
import json
import pathlib
import sys

import jsonschema


def load_json(path: pathlib.Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def check_schema(manifest, schema):
    validator_cls = jsonschema.validators.validator_for(schema)
    validator_cls.check_schema(schema)
    validator = validator_cls(schema)
    return sorted(validator.iter_errors(manifest), key=lambda error: list(map(str, error.path)))


def duplicates(values):
    seen = set()
    dupes = set()
    for value in values:
        if value in seen:
            dupes.add(value)
        seen.add(value)
    return dupes


def check_cross_references(manifest):
    problems = []

    command_names = [command["name"] for command in manifest.get("commands", [])]
    if dupes := duplicates(command_names):
        problems.append(f"duplicate command name(s): {', '.join(sorted(dupes))}")

    option_names = [option["name"] for option in manifest.get("options", [])]
    if dupes := duplicates(option_names):
        problems.append(f"duplicate option name(s): {', '.join(sorted(dupes))}")

    env_var_names = [variable["name"] for variable in manifest.get("environment_variables", [])]
    if dupes := duplicates(env_var_names):
        problems.append(f"duplicate environment variable name(s): {', '.join(sorted(dupes))}")

    topic_names = [topic["name"] for topic in manifest.get("topics", [])]
    if dupes := duplicates(topic_names):
        problems.append(f"duplicate topic name(s): {', '.join(sorted(dupes))}")

    known_commands = set(command_names)
    known_options = set(option_names)
    pairs_with = {}
    for option in manifest.get("options", []):
        if option.get("kind") == "command-option":
            owner = option.get("command")
            if owner not in known_commands:
                problems.append(f"option '{option['name']}' names unknown command '{owner}'")
        if "pairs_with" in option:
            pairs_with[option["name"]] = option["pairs_with"]

    for name, partner in pairs_with.items():
        if partner not in known_options:
            problems.append(f"option '{name}' pairs_with unknown option '{partner}'")
        elif pairs_with.get(partner) != name:
            problems.append(f"option '{name}' pairs_with '{partner}', which does not pair back")

    return problems


def main():
    here = pathlib.Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=pathlib.Path, default=here / "help.json")
    parser.add_argument("--schema", type=pathlib.Path, default=here / "help.schema.json")
    args = parser.parse_args()

    schema = load_json(args.schema)
    manifest = load_json(args.manifest)

    schema_errors = check_schema(manifest, schema)
    if schema_errors:
        for error in schema_errors:
            location = "/".join(str(part) for part in error.path) or "<root>"
            print(f"{args.manifest}: {location}: {error.message}", file=sys.stderr)
        print(f"{len(schema_errors)} schema violation(s) found.", file=sys.stderr)
        return 1

    reference_problems = check_cross_references(manifest)
    if reference_problems:
        for problem in reference_problems:
            print(f"{args.manifest}: {problem}", file=sys.stderr)
        print(f"{len(reference_problems)} cross-reference problem(s) found.", file=sys.stderr)
        return 1

    print(
        f"{args.manifest}: valid "
        f"({len(manifest.get('commands', []))} commands, "
        f"{len(manifest.get('options', []))} options, "
        f"{len(manifest.get('environment_variables', []))} environment variables, "
        f"{len(manifest.get('topics', []))} topics)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
