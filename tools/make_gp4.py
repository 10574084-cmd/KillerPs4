#!/usr/bin/env python3
"""Generate the minimal GP4 project manifest used by PkgTool.Core."""

import argparse
import datetime
from xml.sax.saxutils import quoteattr


def build_directory_tree(paths):
    root = {}
    for path in paths:
        node = root
        parts = path.replace("\\", "/").split("/")[:-1]
        for part in parts:
            node = node.setdefault(part, {})
    return root


def emit_directories(node, depth, output):
    for name in sorted(node):
        pad = "\t" * depth
        if node[name]:
            output.append(f"{pad}<dir targ_name={quoteattr(name)}>")
            emit_directories(node[name], depth + 1, output)
            output.append(f"{pad}</dir>")
        else:
            output.append(f"{pad}<dir targ_name={quoteattr(name)} />")


def generate_gp4(output_path, content_id, input_files):
    files = [path.replace("\\", "/") for path in input_files]
    timestamp = datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S")

    lines = [
        '<?xml version="1.0"?>',
        '<psproject xmlns:xsd="http://www.w3.org/2001/XMLSchema" '
        'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" fmt="gp4" version="1000">',
        "\t<volume>",
        "\t\t<volume_type>pkg_ps4_app</volume_type>",
        "\t\t<volume_id>PS4VOLUME</volume_id>",
        f"\t\t<volume_ts>{timestamp}</volume_ts>",
        f'\t\t<package content_id={quoteattr(content_id)} '
        'passcode="00000000000000000000000000000000" storage_type="digital50" app_type="full" />',
        '\t\t<chunk_info chunk_count="1" scenario_count="1"><chunks>'
        '<chunk id="0" layer_no="0" label="Chunk #0" /></chunks>'
        '<scenarios default_id="0"><scenario id="0" type="sp" '
        'initial_chunk_count="1" label="Scenario #0">0</scenario></scenarios>'
        '</chunk_info>',
        "\t</volume>",
        '\t<files img_no="0">',
    ]

    for path in files:
        quoted = quoteattr(path)
        lines.append(f"\t\t<file targ_path={quoted} orig_path={quoted} />")

    lines.extend(["\t</files>", "\t<rootdir>"])
    emit_directories(build_directory_tree(files), 2, lines)
    lines.extend(["\t</rootdir>", "</psproject>"])

    with open(output_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--content-id", required=True)
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()

    generate_gp4(args.out, args.content_id, args.files)


if __name__ == "__main__":
    main()
