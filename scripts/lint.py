"""Runs clang-tidy on all .cpp, .c, .hpp and .h files
except those prefixed with x.
"""

import sys
import os
import subprocess
from pathlib import Path


def check_clang_tidy_installation():
    """Check that clang-tidy is installed before running
    This script just runs clang-tidy <args>, so it needs clang-tidy in the path
    """
    try:
        subprocess.run(["clang-tidy", "--version"], check = True, stdout = subprocess.PIPE)
    except FileNotFoundError:
        print("\033[31mClang-tidy is either not installed or not in the system PATH.\033[0m")

def ensure_project_root():
    """
    Ensures the script runs from the project root directory by
    changing to the parent directory if run from scripts/
    """
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    if script_dir.name == "scripts":
        os.chdir(project_root)
        print(f"\033[34mChanged working directory to project root: {project_root}\033[0m")

    # Verify CMakeLists.txt exists in the current directory
    if not Path("CMakeLists.txt").exists():
        print("\033[31mError: CMakeLists.txt not found in the current directory.")
        print("This script must be run from the project root or scripts/ directory.\033[0m")
        sys.exit(1)

def check_file(filename: str, compile_commands_path: str):
    """A wrapper around `subprocess.run`"""
    print(f"\033[34mChecking: \"{filename}\"\033[0m")
    try:
        subprocess.run(["clang-tidy", "-p", compile_commands_path, filename], check = True)
    except subprocess.CalledProcessError:
        # clang-tidy exits with error code 1 on linting fails,
        # but we want to check every file, so ignore that error
        pass

def find_compile_commands():
    """
    Tries to locate compile_commands.json in the project root or in build/

    Returns:
    A the path to compile_commands.json, if found
    Otherwise, exits
    """
    root_path = Path("compile_commands.json")
    build_path = Path("build/compile_commands.json")
    if root_path.exists():
        print("\033[34mFound compile_commands.json in project root\033[0m")
        return root_path
    elif build_path.exists():
        print("\033[34mFound compile_commands.json in build directory\033[0m")
        return build_path
    else:
        print("\033[31mError: compile_commands.json not found in project root or build directory.")
        print(
            "Make sure to build the project to generate compile_commands.json, either using "
              +"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON, or running build.py with --compile-commands"
        )
        print("If compile_commands.json is in a different location, please "
              + " move it to the project root, then rerun this script\033[0m")
        sys.exit(1)

def __get_files_from_root(root_dir: str = "."):
    """Get all file in a directory, including all subdirectories"""
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            yield os.path.join(dirpath, filename)

def get_filenames_in(*paths: str) -> list[str]:
    """Get all files in all the arguments pass in *paths"""

    all_files: list[str] = []
    for path in paths:
        all_files.extend(__get_files_from_root(path))
    return all_files

ensure_project_root()
COMPILE_COMMANDS_DIR = str(find_compile_commands())

# Other files to check manually (in the root directory)
EXTRA_FILES = ["manganese.cpp", "manganese_tests.cpp"]

files: list[str] = [file for file in get_filenames_in("src", "include", "tests")] + EXTRA_FILES

files = [file for file in files if (file.endswith((".cpp", ".hpp", ".c", ".h")) and "x." not in file)]

for file in files:
    check_file(file, COMPILE_COMMANDS_DIR)
