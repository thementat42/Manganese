"""Takes in a .cpp file, creates the corresponding .h file"""

import re
import sys
from pathlib import Path

def extract_includes_and_declarations(file_content: list[str], source_filename: str):
    """Pull out #includes and function declarations (but not the bodies)"""
    includes: list[str] = []
    declarations: list[str] = []
    namespaces: list[str] = []
    manganese_macros: list[str] = []

    # Regular expressions
    include_pattern = re.compile(r'^\s*#include\s+[<"](.+)[>"]')
    func_pattern = re.compile(r'^\s*([a-zA-Z_][\w:<>\s*&]*?)\s+([a-zA-Z_][\w]*)\s*\(([^)]*)\)\s*\{')
    namespace_pattern = re.compile(r'^\s*namespace\s+([a-zA-Z_][\w]*)\s*\{')
    define_pattern = re.compile(r'^\s*#define\s+(MANGANESE_(?:BEGIN|END)).*')

    for line in file_content:
        include_match = include_pattern.match(line)
        define_match = define_pattern.match(line)

        if include_match:
            included_file = include_match.group(1)
            if included_file != source_filename:  # don't include a file in itself
                includes.append(line.strip())
        elif define_match:
            manganese_macros.append(line.strip())
        elif func_match := func_pattern.match(line):
            return_type = func_match.group(1).strip()
            name = func_match.group(2).strip()
            args = func_match.group(3).strip()
            declarations.append(f"{return_type} {name}({args});")
        elif namespace_match := namespace_pattern.match(line):
            namespace_name = namespace_match.group(1).strip()
            namespaces.append(namespace_name)

    return includes, declarations, namespaces, manganese_macros


def find_source_file(filename: str):
    """Recursively search for a file with the given filename
    in the current directory and subdirectories."""
    for path in Path('.').rglob(filename):
        if path.is_file():
            return path
    return None

def generate_header_file(source_filename: str):
    """Take in source file name, output a corresponding header file"""
    source_path = find_source_file(source_filename)
    if source_path is None or source_path.suffix != '.cpp':
        print(f"Could not find a valid .cpp file named\
              '{source_filename}' in the current directory or subdirectories.")
        return

    with open(source_path, 'r', encoding = "utf8") as f:
        content = f.readlines()

    includes, declarations, namespaces, manganese_macros = extract_includes_and_declarations(content, source_path.name)

    # Generate include guard name from filename
    filename_base = source_path.stem.upper()
    include_guard = f"{filename_base}_H"

    header_path = source_path.with_suffix('.h')
    with open(header_path, 'w', encoding = "utf8") as f:
        # Write header guard
        f.write(f"#ifndef {include_guard}\n")
        f.write(f"#define {include_guard}\n\n")
          # Write includes
        for inc in includes:
            f.write(inc + '\n')
        f.write('\n')

        if manganese_macros:
            f.write("MANGANESE_BEGIN\n")

        # Start namespace blocks if any
        for namespace in namespaces:
            f.write(f"namespace {namespace} {{\n\n")

        # Write declarations
        for decl in declarations:
            f.write(decl + '\n')
        f.write('\n')
          # Close namespace blocks if any
        for _ in namespaces:
            f.write("}\n")

        # Close the Manganese namespace
        if manganese_macros:
            f.write("\nMANGANESE_END\n\n")

        # Close header guard
        f.write(f"#endif // {include_guard}\n")

    print(f"Header file generated: {header_path}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python gen_header.py <source.cpp>")
    else:
        for file in sys.argv[1:]:
            generate_header_file(file)
