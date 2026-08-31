"""
A small script that automates using CMake to build the manganese compiler.
This script creates the cmake commands and runs them using Python's subprocess library
Usage:
    python build.py [options] [-- exec_with...]
Options:
    NOTE: These options may be out of date if the docstring has not been correctly updated.
    NOTE: For the most recent set of available arguments, run the script with the -h flag
    
    -b, --build-dir                                        Specify a build directory (default: 'build').
    -c, --clean                                            Clean the build directory after the build (removes everything except the output executable).
    -cc, --ccompiler CCOMPILER                             Specify a C compiler to use for the build instead of the default for the system
    --compile-commands                                     Generate compile_commands.json in the build directory.
    -cxx, -cpp, --cxxcompiler, --cppcompiler CXXCOMPILER   Specify a C++ compiler to use for the build instead of the default for the system
    -d, --debug                                            Build the compiler in debug mode instead of release mode.
    --disable-ansi                                          Disable ANSI terminal colours in compiler output.
    -f, --fresh-build                                       Run a fresh build by clearing the build directory before running CMake
    -g, --generator                                        Set a generator for CMake's build files.
    -h, --help                                             Print this help message and exit
    -j, --jobs                                             Number of parallel build jobs.
    -l, --linker LINKER                                    Specify a linker to use (e.g. gcc, ld, etc.) instead of the default for the system
    -m, --memory-tracking                                  Track the total amount of heap-allocated memory the program uses (ignores deallocations)
    -mc, --memory-tracking-continuous                      Continuously track the amount of heap-allocated memory (accounts for deallocations, accuracy may vary with different compilers)
    --no-move                                              Leave the executable in the build directory after building (by default it will be moved to the root directory)
    -r, --run                                              Run the executable immediately after building.
    -t, --tests                                            Build the test suite instead of the main compiler.
    --target                                               Specific CMake target to build.

    positional arguments:
    --cmake-arg CMAKE_ARG                                  Specify an additional argument to pass to CMake. Can be used multiple times to pass multiple args (e.g. --cmake-arg arg1 --cmake-arg arg2)
    --build-arg BUILD_ARG                                  Specify an additional argument to pass to the build process. Can be used multiple times to pass multiple args (e.g. --build-arg arg1 --build-arg arg2)
    --exec_with ...                                        Arguments to pass to the executable when using -r or --run.

Examples:
    python build.py -c -d -j 4
    python build.py --tests --run --exec_with --all
Requirements:
    - Python 3.8+
    - CMake installed and available in PATH
    - The script should be run from the project root directory containing CMakeLists.txt
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

def check_cmake_installation():
    """Check that cmake is installed before running
    This script just runs cmake <args>, so it needs cmake in the path
    """
    cmake = shutil.which("cmake")
    if cmake is None:
        print("CMake is either not installed or not in the system PATH.")
        sys.exit(1)

def ensure_project_root():
    """
    Ensures the script runs from the project root directory by
    changing to the parent directory if run from scripts/
    """
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    if script_dir.name == "scripts":
        os.chdir(project_root)
        print(f"Changed working directory to project root: {project_root}")

    # Verify CMakeLists.txt exists in the current directory
    if not Path("CMakeLists.txt").exists():
        print("Error: CMakeLists.txt not found in the current directory.")
        print("This script must be run from the project root or scripts/ directory.")
        sys.exit(1)

def run_command(command: list[str]):
    """A wrapper around `subprocess.run`"""
    print(f"Running: \"{' '.join(command)}\"")
    try:
        subprocess.run(command, check = True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with exit code {e.returncode}")
        sys.exit(e.returncode)

check_cmake_installation()
ensure_project_root()

arg_parser = argparse.ArgumentParser(description = "Runs CMake to build the manganese compiler")

arg_parser.add_argument(
    "-b", "--build-dir",
    type=str,
    default="build",
    help="Specify a build directory (default: 'build')"
)

arg_parser.add_argument(
    "-c", "--clean",
    action = "store_true",
    help = "Clean the build directory after the build is done\
        (removes everything except the executable)"
)

arg_parser.add_argument(
    "-cc", "--ccompiler",
    type=str,
    help="Specify a C compiler to use for the build instead of the default for the system"
)

arg_parser.add_argument(
    "--compile-commands",
    action="store_true",
    help="Generate compile_commands.json in the build directory",
    default=False
)

arg_parser.add_argument(
    "-cxx", "-cpp", "--cxxcompiler", "--cppcompiler",
    type=str,
    help="Specify a C++ compiler to use for the build instead of the default for the system"
)

arg_parser.add_argument(
    "-d", "--debug",
    action = "store_true",
    help = "Build the compiler in debug mode instead of release mode"
)

arg_parser.add_argument(
    "--disable-ansi",
    action="store_true",
    help="Disable ANSI terminal colours in compiler output"
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
    type = int,
    help="Number of parallel build jobs"
)

arg_parser.add_argument(
    "-l", "--linker",
    type = str,
    help = "Specify a linker to use (e.g. gcc, ld, etc.) instead of the default for the system"
)

arg_parser.add_argument(
    "-m", "--memory-tracking",
    action = "store_true",
    help = "Track the total amount of heap-allocated memory the program uses (only available in debug mode)"
)

arg_parser.add_argument(
    "-mc", "--memory-tracking-continuous",
    action = "store_true",
    help = "Enable detailed continuous memory tracking with allocation/deallocation logging to file (requires -m/--memory-tracking) (accuracy may vary depending on the compiler)"
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
    "--exec-with",
    nargs=argparse.REMAINDER,
    help="Arguments to pass to the executable when using -r or --run"
)

arg_parser.add_argument (
    "--cmake-arg",
    nargs = 1,
    action = "append",
    help = "Specify an additional argument to pass to CMake. Can be used multiple times to pass multiple args (e.g. --cmake-arg arg1 --cmake-arg arg2)"
)

arg_parser.add_argument (
    "--build-arg",
    nargs = 1,
    action = "append",
    help = "Specify an additional argument to pass to the build process. Can be used multiple times to pass multiple args (e.g. --build-arg arg1 --build-arg arg2)"
)

args = arg_parser.parse_args()

BUILD_DIR = Path(args.build_dir)
OUT_NAME = "manganese" + (".exe" if os.name == "nt" else "")

if args.clean and args.no_move:
    print(
        "Warning: --clean supersedes --no-move.",
        "The output will be moved to the root directory."
    )
    args.no_move = False

if args.exec_with:
    if not args.run:
        print(
            "Warning: positional arguments",
            f"({', '.join(args.exec_with)})",
            "were passed to the script but the run flag (-r) was not specified"
        )
    else:
        print(
            "Running with positional arguments:", *args.exec_with, ""
        )

if args.memory_tracking:
    if not args.debug:
        print(
            "Warning: --memory-tracking only has an effect in debug mode"
        )
    else:
        print(
            "Tracking cumulative memory usage"
        )

if args.memory_tracking_continuous:
    if not args.memory_tracking:
        print(
            "Warning: --memory-tracking-continuous has no effect if --memory-tracking is off"
        )
    else:
        print(
            "Tracking estimated continuous memory usage"
        )
    print(
        "Warning: --memory-tracking-continuous may not be completely accurate and can vary depending on the compiler "
    )

if args.tests and not args.debug:
    print(
        "Warning: --tests will compile in debug mode",
         "even if the debug flag (-d or --debug) is not set"
    )

if args.fresh_build and BUILD_DIR.exists():
    print(f"Cleaning build directory before building ({BUILD_DIR})")
    try:
        shutil.rmtree(BUILD_DIR)
    except PermissionError:
        print(f"Permission denied while cleaning {BUILD_DIR}")
        sys.exit(1)
    print(f"Cleaned build directory ({BUILD_DIR})")

os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

cmake_args = [
    "cmake",
    "..",
    f"-DBUILD_TESTS={"ON" if args.tests else "OFF"}",
    f"-DCMAKE_BUILD_TYPE={("Debug" if args.debug or args.tests else "Release")}",
    f"-DMEMORY_TRACKING={"ON" if args.memory_tracking else "OFF"}",
    f"-DCONTINUOUS_MEMORY_TRACKING={"ON" if args.memory_tracking_continuous else "OFF"}",
    f"-DCMAKE_EXPORT_COMPILE_COMMANDS={"ON" if args.compile_commands else "OFF"}",
    f"-DDISABLE_ANSI_COLOURS={"ON" if args.disable_ansi else "OFF"}",
]

build_args = [
    "cmake",
    "--build", ".",
]

# Extra args
if args.jobs:
    build_args.extend(["--parallel", str(args.jobs)])
if args.target:
    build_args.extend(["--target", args.target])
if args.generator is not None:
    cmake_args.extend(["-G", args.generator])
if args.ccompiler:
    cmake_args.append(f"-DCMAKE_C_COMPILER={args.ccompiler}")
if args.cxxcompiler:
    cmake_args.append(f"-DCMAKE_CXX_COMPILER={args.cxxcompiler}")
if args.linker:
    cmake_args.append(f"-DCMAKE_LINKER={args.linker}")
if args.cmake_arg:
    # appended arguments are a list of list of strings (each sub-list contains 1 item)
    # so collect all those items
    cmake_args.extend(list(arg[0] for arg in args.cmake_arg))
if args.build_arg:
    build_args.append("--")  # tells cmake to treat all subsequent arguments as arguments to the build system
    # appended arguments are a list of list of strings (each sub-list contains 1 item)
    # so collect all those items
    build_args.extend(list(arg[0] for arg in args.build_arg))

build_start = time.perf_counter()

run_command(cmake_args)
run_command(build_args)

build_time = time.perf_counter() - build_start

# Move the output file from build/bin to the root directory
os.chdir("..")  # since the build directory path is relative to the root, go back there
bin_dir = Path(BUILD_DIR) / "bin"
output_file = bin_dir / OUT_NAME
if not args.no_move:
    if bin_dir.exists() and output_file.exists():
        dest_file = Path(OUT_NAME)
        if dest_file.exists():
            print(f"Warning: {OUT_NAME} already exists in the root directory and will be overwritten")
        print(f"Moving {OUT_NAME} from {bin_dir} to the root directory")
        # Move the output file to the root directory
        shutil.move(output_file, dest_file)
        print(f"Successfully moved {OUT_NAME} to the root directory")
    else:
        print(f"Could not find {OUT_NAME} in {bin_dir.resolve()}")

if args.clean:
    # Remove all files and folders in the build directory except the executable in bin
    print(f"Removing extra files from the build directory ({BUILD_DIR})")
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
            print(f"Warning: Could not remove {item}: {e}")
    print("Cleaned build directory")

print(f"Build completed in {build_time:.2f} seconds")

if args.run:
    # If we chose not to move the file, it should be executed from the build/bin directory
    EXECUTE_PATH = str((bin_dir if args.no_move else Path.cwd()) / OUT_NAME)
    cmd = [EXECUTE_PATH]
    if args.exec_with:
        cmd.extend(args.exec_with)
    run_command(cmd)
