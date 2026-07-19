import os
import shutil
import subprocess
import sys
from pathlib import Path

DIRECTORIES = ["include", "src", "tests"]

BUILD_DIR = "build"
REPORT_FILE = "x.clang-tidy-report.txt"

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

CHECKS = ",".join([
    "clang-analyzer-*",
    "bugprone-*",
    "performance-*",
    "portability-*",
    "readability-*",
    "modernize-*",
    "misc-*",

    "-readability-magic-numbers",
    "-readability-identifier-length",
    "-modernize-use-trailing-return-type",
    "-misc-non-private-member-variables-in-classes"
    "-misc-confusable-identifiers"
])


def find_files(base_dir: Path):
    for root, _, files in os.walk(base_dir):
        for file in files:
            path = Path(root) / file
            if path.suffix.lower() in EXTENSIONS:
                yield path


def main():
    clang_tidy = shutil.which("clang-tidy")
    if clang_tidy is None:
        print("Error: clang-tidy not found in PATH.", file=sys.stderr)
        sys.exit(1)

    compile_commands = Path(BUILD_DIR) / "compile_commands.json"
    if not compile_commands.exists():
        print(
            f"Error: {compile_commands} not found.\n"
            "Generate it by configuring CMake with:\n"
            "  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\n"
            "  or running build.py with --compile-commands",
            file=sys.stderr,
        )
        sys.exit(1)

    checked = 0

    with open(REPORT_FILE, "w", encoding="utf-8") as report:
        for directory in DIRECTORIES:
            base = Path(directory)
            if not base.is_dir():
                print(f"Skipping missing directory: {directory}")
                continue

            for file in find_files(base):
                print(f"Checking {file}")

                report.write("=" * 80 + "\n")
                report.write(f"{file}\n")
                report.write("=" * 80 + "\n")

                result = subprocess.run(
                    [
                        clang_tidy,
                        str(file),
                        f"-p={BUILD_DIR}",
                        f"-checks={CHECKS}",
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                )

                report.write(result.stdout)
                report.write("\n\n")

                checked += 1

    print(f"\nDone. Checked {checked} file(s).")
    print(f"Report written to {REPORT_FILE}")


if __name__ == "__main__":
    main()