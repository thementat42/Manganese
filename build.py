"""
A small script that automates using CMake to build the manganese compiler.
This script creates the cmake commands and runs them using Python's subprocess library
Usage:
    python build.py [options] [-- exec_args...]
Options:
    NOTE: These options may be out of date if the docstring has not been correctly updated.
    NOTE: For a the actual set of available arguments, run the script with the -h flag
    
    -b, --build-dir   Specify a custom build directory (default: 'build').
    -c, --clean       Clean the build directory after the build (removes everything except the output executable).
    -d, --debug       Build the compiler in debug mode instead of release mode.
    -f, --fresh       Run a fresh build by clearing the build directory before running CMake
    -g, --generator   Set a generator for CMake's build files.
    -h, --help        Print this help message
    -j, --jobs        Number of parallel build jobs.
    --no-move         Leave the executable in the build directory after building (by default it will be moved to the root directory)
    -r, --run         Run the executable immediately after building.
    -t, --tests       Build the test suite instead of the main compiler.
    --target          Specific CMake target to build.

    positional arguments:
    exec_args         Arguments to pass to the executable when using -r or --run.

Examples:
    python build.py -c -d -j 4
    python build.py --tests --run exec_args --all
Requirements:
    - Python 3.8+
    - CMake installed and available in PATH
    - The script should be run from the project root directory containing CMakeLists.txt
A small script that automates using cmake to build the manganese compiler
This script creates the cmake commands and runs them using Python's subprocess library
run `python build.py -h` to see the different command line arguments that are available
"""

import sys
import os
import subprocess
import argparse
import shutil
from pathlib import Path

def check_cmake_installation():
    """Check that cmake is installed before running"""
    try:
        subprocess.run(["cmake", "--version"], check = True, stdout = subprocess.PIPE)
    except FileNotFoundError:
        print("\033[31mCMake is either not installed or not in the PATH.\033[0m")
        sys.exit(1)

def run_command(command: list[str]):
    """A wrapper around `subprocess.run`"""
    print(f"\033[34mRunning: \"{' '.join(command)}\"\033[0m")
    try:
        subprocess.run(command, check = True)
    except subprocess.CalledProcessError as e:
        print(f"\033[31mCommand failed with exit code {e.returncode}\033[0m")
        sys.exit(e.returncode)

check_cmake_installation()

arg_parser = argparse.ArgumentParser(description = "Runs CMake to build the manganese compiler")

arg_parser.add_argument(
    "-b", "--build-dir",
    type=str,
    default="build",
    help="Specify a custom build directory (default: 'build')"
)

arg_parser.add_argument(
    "-c", "--clean",
    action = "store_true",
    help = "Clean the build directory after the build is done\
        (removes everything except the executable)"
)

arg_parser.add_argument(
    "-d", "--debug",
    action = "store_true",
    help = "build the compiler in debug mode instead of release mode"
)

arg_parser.add_argument(
    "-f", "--fresh-build",
    action = "store_true",
    help = "Do a fresh build by clearing the build directory before compiling"
)

arg_parser.add_argument(
    "-g", "--generator",
    help = "Set a generator for cmake's build file other than the default",
)

arg_parser.add_argument(
    "-j", "--jobs",
    type=int,
    help="Number of parallel build jobs"
)

arg_parser.add_argument(
    "--no-move",
    action="store_true",
    help="Leave the executable in the build directory after building"
)

arg_parser.add_argument(
    "-r", "--run",
    action="store_true",
    help="Run the executable immediately after building"
)

arg_parser.add_argument(
    "-t", "--tests",
    action = "store_true",
    help = "Build the test suite instead of the main compiler"
)

arg_parser.add_argument(
    "--target",
    help="Specific CMake target to build"
)

arg_parser.add_argument(
    "exec_args",
    nargs=argparse.REMAINDER,
    help="Arguments to pass to the executable when using -r or --run"
)

args = arg_parser.parse_args()

BUILD_DIR = Path(args.build_dir)
OUT_NAME = "manganese" + ("_tests" if args.tests else "") + (".exe" if os.name == "nt" else "")

if args.clean and args.no_move:
    print(
        "\033[33mWarning: --clean supersedes --no-move.",
        "The output will be moved to the root directory.\033[0m"
    )
    args.no_move = False

if not args.run and len(args.exec_args) > 0:
    print(
        "\033[33mWarning: positional arguments",
        f"({', '.join(args.exec_args[1:])})",  # [1:] to skip "exec_args"
        "were passed to the script but the run flag (-r) was not specified"
    )

if args.fresh_build and BUILD_DIR.exists():
    print(f"\033[34mCleaning build directory before building ({BUILD_DIR})\033[0m")
    try:
        shutil.rmtree(BUILD_DIR)
    except PermissionError:
        print(f"\033[31mPermission denied while cleaning {BUILD_DIR}\033[0m")
        sys.exit(1)
    print(f"\033[34mCleaned build directory ({BUILD_DIR})\033[0m")

os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

cmake_args = [
    "cmake",
    "..",
    f"-DBUILD_TESTS={"ON" if args.tests else "OFF"}",
    f"-DDEBUG={"ON" if args.debug or args.tests else "OFF"}",
]

cmake_build_args = [
    "cmake",
    "--build", ".",
]

# Extra args
if args.jobs:
    cmake_build_args.extend(["--parallel", str(args.jobs)])
if args.target:
    cmake_build_args.extend(["--target", args.target])
if args.generator is not None:
    cmake_args.extend(["-G", args.generator])

run_command(cmake_args)
run_command(cmake_build_args)

# Move the output file from build/bin to the root directory
os.chdir("..")  # since the build directory path is relative to the root, go back there
bin_dir = Path(BUILD_DIR) / "bin"
output_file = bin_dir / OUT_NAME
if not args.no_move:
    if bin_dir.exists() and output_file.exists():
        print(f"\033[34mMoving {OUT_NAME} from {bin_dir} to the root directory")
        # Move the output file to the root directory
        shutil.move(output_file, Path(OUT_NAME))
        print(f"\033[32mSuccessfully moved {OUT_NAME} to the root directory\033[0m")
    else:
        print(f"\033[33mCould not find {OUT_NAME} in {bin_dir.resolve()}\033[0m")

if args.clean:
    # Remove all files and folders in the build directory except the executable in bin
    print(f"\033[34mRemoving extra files from the build directory ({BUILD_DIR})\033[0m")
    for item in BUILD_DIR.glob("**/*"):
        # Skip the output executable
        if item == output_file:
            continue
        try:
            if item.is_file():
                item.unlink()
            elif item.is_dir():
                shutil.rmtree(item)
        except Exception as e:
            print(f"\033[33mWarning: Could not remove {item}: {e}\033[0m")
    print("\033[34mCleaned build directory\033[0m")

if args.run:
    # If we chose not to move the file, it should be executed from the build/bin directory
    EXECUTE_PATH = str(bin_dir / OUT_NAME if args.no_move else Path.cwd() / OUT_NAME)
    run_command([EXECUTE_PATH] + args.exec_args[1:])  # 1: to skip passing "exec_args"
