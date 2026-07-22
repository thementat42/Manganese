# Quickly run clang-format on all headers and source files

import os
import shutil
import subprocess
import sys
from pathlib import Path

DIRECTORIES = ["include", "src", "tests"]

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

def main():
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        print("Error: clang-format not found in PATH.", file=sys.stderr)
        sys.exit(1)

    formatted = 0

    for directory in DIRECTORIES:
        base = Path(directory)
        if not base.is_dir():
            print(f"Skipping missing directory: {directory}")
            continue

        for file in find_files(base):
            print(f"Formatting {file}")
            subprocess.run(
                [clang_format, "-i", str(file)],
                check=True,
            )
            formatted += 1

    print(f"\nDone. Formatted {formatted} file(s).")

if __name__ == "__main__":
    main()
