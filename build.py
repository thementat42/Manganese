"""A small script that automates using cmake to build the manganese compiler"""
import sys
import os
import subprocess
import argparse
import shutil
from pathlib import Path

def run_command(command: list[str]):
    """A wrapper around `subprocess.run`"""
    print(f"\033[34mRunning: \"{' '.join(command)}\"\033[0m")
    try:
        subprocess.run(command, check = True)
    except subprocess.CalledProcessError as e:
        print(f"\033[31mCommand failed with exit code {e.returncode}\033[0m")
        sys.exit(e.returncode)

parser = argparse.ArgumentParser()

parser.add_argument(
    "-t", "--tests",
    action = "store_true",
    help = "Build the test suite instead of the main compiler"
)

parser.add_argument(
    "-d", "--debug",
    action = "store_true",
    help = "build the compiler in debug mode"
)

parser.add_argument(
    "-c", "--clean",
    action = "store_true",
    help = "Clean the build directory before compiling"
)

parser.add_argument(
    "-g", "--generator",
    help = "Set a generator for cmake's build file (default to user's CMake-configured generator)",
)

parser.add_argument(
    "-r", "--run",
    action="store_true",
    help="Run the executable immediately after building"
)

parser.add_argument(
    "--no-move",
    action="store_true",
    help="Don't move the executable from the build directory to the root directory"
)

parser.add_argument(
    "exec_args",
    nargs=argparse.REMAINDER,
    help="Arguments to pass to the executable when using --run"
)

parser.add_argument(
    "-j", "--jobs",
    type=int,
    help="Number of parallel build jobs"
)

parser.add_argument(
    "--target",
    help="Specific CMake target to build"
)

args = parser.parse_args()

BUILD_DIR = Path('build')

if args.clean:
    print(f"\033[34mCleaning build directory ({BUILD_DIR})\033[0m")
    try:
        shutil.rmtree(BUILD_DIR)
    except PermissionError:
        print(f"\033[31mPermission denied while cleaning {BUILD_DIR}\033[0m")
        sys.exit(1)
    print(f"\033[34mCleaned build directory ({BUILD_DIR})\033[0m")
os.makedirs(BUILD_DIR, exist_ok=True)

os.chdir(BUILD_DIR)

OUT_NAME = "manganese" + ("_tests" if args.tests else "") + (".exe" if os.name == "nt" else "")

cmake_args = [
    "cmake",
    "..",
    f"-DBUILD_TESTS={"ON" if args.tests else "OFF"}",
    f"-DDEBUG={"ON" if args.debug else "OFF"}",
]

if args.jobs:
    cmake_args.extend(["--parallel", str(args.jobs)])
if args.target:
    cmake_args.extend(["--target", args.target])
if args.generator is not None:
    cmake_args.extend(["-G", args.generator])

run_command(cmake_args)
run_command(["cmake", "--build", "."])

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

if args.run:
    # If we chose not to move the file, it should be executed from the build/bin directory
    EXECUTE_PATH = str(bin_dir / OUT_NAME if args.no_move else Path.cwd() / OUT_NAME)
    run_command([EXECUTE_PATH] + args.exec_args[1:])  # 1: to skip passing "exec_args" on
