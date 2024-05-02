#! /usr/bin/env python3

import glob
import os


def list_headers(directory):
    def extract_name(header):
        return header.split("/")[-1].removesuffix('.hpp')

    raw_headers = glob.glob(f"{directory}/*.hpp")
    return [extract_name(header) for header in raw_headers]


def include_line(name):
    return f"#include <brain_indexer/{name}.hpp>"


def cmake_line(name):
    return f"    PRIVATE ${{CMAKE_CURRENT_SOURCE_DIR}}/{name}.cpp"


if __name__ == "__main__":
    headers = list_headers("include/brain_indexer")
    output_dir = "tests/cpp/check_headers/"

    for header in headers:
        file_contents = [
            include_line(header),
            ""
        ]

        with open(os.path.join(output_dir, f"{header}.cpp"), "w") as f:
            f.write("\n".join(file_contents))

    cmake_contents = "\n".join(
        ["target_sources(si_check_headers"]
        + [cmake_line(header) for header in headers]
        + [")", ""]
    )
    with open(os.path.join(output_dir, "files.cmake"), "w") as f:
        f.write(cmake_contents)
