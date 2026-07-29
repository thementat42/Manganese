# Quickly run clang-format on all headers and source files.

import os
import shutil
import subprocess
import sys
from pathlib import Path

DIRECTORIES = ["include", "src", "tests"]

ROOT_FILES = [
    "manganese.cpp",
    "manganese_tests.cpp",
]

EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
}


def find_files(base_dir: Path):
    for root, _, files in os.walk(base_dir):
        for file in files:
            path = Path(root) / file
            if path.suffix.lower() in EXTENSIONS:
                yield path


def get_files():
    files = []

    # Collect files from project directories.
    for directory in DIRECTORIES:
        base = Path(directory)
        if not base.is_dir():
            print(f"Skipping missing directory: {directory}")
            continue

        files.extend(find_files(base))

    # Collect additional root-level files.
    for file in map(Path, ROOT_FILES):
        if not file.is_file():
            print(f"Skipping missing file: {file}")
            continue

        files.append(file)

    return sorted(files)


def run_clang_format(clang_format, file: Path):
    subprocess.run(
        [clang_format, "-i", str(file)],
        check=True,
    )


def main():
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        print("Error: clang-format not found in PATH.", file=sys.stderr)
        sys.exit(1)

    files = get_files()
    total = len(files)

    for index, file in enumerate(files, start=1):
        print(f"[{index}/{total}] Formatting {file}")
        run_clang_format(clang_format, file)

    print(f"\nDone. Formatted {total} file(s).")


if __name__ == "__main__":
    main()