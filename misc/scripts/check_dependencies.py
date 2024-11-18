#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Final, NoReturn, cast

SCONSCRIPT_RE: Final[re.Pattern[str]] = re.compile(r'(?:^\s*SConscript\("?)(.*?)(?:"?\))', re.MULTILINE)
INCLUDE_RE: Final[re.Pattern[str]] = re.compile(r'(?:^#include ")(.*?)(?:")', re.MULTILINE)
ROOT_DIR: Final[Path] = Path(__file__).parent.parent.parent


def build_header_tree(header_tree: list[Path], path: Path) -> None:
    path_dir = path.parent
    header_tree.append(path_dir)

    with open(path, encoding="utf-8") as file:
        matches = cast(list[str], SCONSCRIPT_RE.findall(file.read()))

    for match in matches:
        if " " in match:
            # TODO: Can't handle non-generic strings *at all*, maybe SCons can give this info natively?
            # Ehh, just workaround for now, it's a proof-of-concept anyway.
            for child in ((path_dir / "platform") if (path_dir / "platform").exists() else path_dir).iterdir():
                if child.is_dir() and (sub := child / "SCsub").exists():
                    try:
                        build_header_tree(header_tree, sub)
                    except Exception as e:
                        print(e, file=sys.stderr)
            continue
        try:
            build_header_tree(header_tree, path_dir / match)
        except Exception as e:
            print(e, file=sys.stderr)


def parse_header(header: Path, header_tree: list[Path]) -> None:
    if (header_dir := header.parent.resolve()) not in header_tree:
        print(f'Header "{header}" not found in header tree', file=sys.stderr)
        return
    header_index = header_tree.index(header_dir)

    with open(header, encoding="utf-8") as file:
        matches = [Path(header) for header in INCLUDE_RE.findall(file.read())]

    for match in matches:
        if (match_dir := match.parent.resolve()) not in header_tree:
            print(f'Header "{header}" dependency "{match}" not found in header tree', file=sys.stderr)
            continue
        if header_tree.index(match_dir) > header_index:
            print(f'Header "{header}" has invalid dependency: "{match}"', file=sys.stderr)


def main() -> NoReturn:
    parser = argparse.ArgumentParser(
        prog="check_dependencies",
        description="Ensure that header files only include scripts at or below their dependency level.",
    )
    parser.add_argument("headers", nargs="+", help="A list of header files to validate.", type=Path)
    args = parser.parse_args()

    header_tree: list[Path] = []
    build_header_tree(header_tree, ROOT_DIR / "SConstruct")
    # print([header.as_posix() for header in header_tree])

    for header in cast(list[Path], args.headers):
        parse_header(header, header_tree)

    # Force fail
    sys.exit(1)


if __name__ == "__main__":
    main()
