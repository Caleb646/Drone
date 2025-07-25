#!/usr/bin/env python3
"""
Test Registration Generator Script

This script scans test files for functions that start with 'test_' and generates
the registration code for the test runner.

Usage: python generate_test_registrations.py
"""

import os
import re
from pathlib import Path

def find_test_functions(file_path):
    """Find all test functions in a C file."""
    test_functions = []
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Find all function definitions that start with 'test_'
        # Pattern matches: void test_FunctionName(void) {
        pattern = r'void\s+(test_\w+)\s*\(\s*void\s*\)\s*\{'
        matches = re.findall(pattern, content, re.MULTILINE)
        
        for match in matches:
            test_functions.append(match)
            
    except FileNotFoundError:
        print(f"Warning: Could not read file {file_path}")
    
    return test_functions

def generate_registration_code(test_files):
    """Generate the registration code for all test functions."""
    
    all_functions = []
    declarations = []
    registrations = []
    
    for file_path in test_files:
        file_name = os.path.basename(file_path)
        functions = find_test_functions(file_path)
        
        if functions:
            declarations.append(f"    // {file_name} Tests")
            for func in functions:
                declarations.append(f"    extern void {func}(void);")
                registrations.append(f"    register_test(\"{func}\", {func});")
            declarations.append("")
            all_functions.extend(functions)
    
    # Remove the last empty line
    if declarations and declarations[-1] == "":
        declarations.pop()
    
    return all_functions, declarations, registrations

def main():
    """Main function to generate test registrations."""
    
    # Find all test files
    test_dir = Path(".")
    test_files = list(test_dir.glob("test_*.c"))
    
    if not test_files:
        print("No test files found!")
        return
    
    print(f"Found {len(test_files)} test files:")
    for file in test_files:
        print(f"  - {file}")
    
    # Generate registration code
    all_functions, declarations, registrations = generate_registration_code(test_files)
    
    print(f"\nFound {len(all_functions)} test functions:")
    for func in all_functions:
        print(f"  - {func}")
    
    # Generate the registration function
    registration_function = f"""
#include \"test_runner.h\"
// Manual test registration (for compilers without constructor support)
void register_all_tests(void) {{
    // Forward declarations for all test functions
{chr(10).join(declarations)}
    
    // Register all tests
{chr(10).join(registrations)}
}}
"""
    
    print("\n" + "="*60)
    print("Generated registration code:")
    print("="*60)
    print(registration_function)
    
    # Optionally write to a file
    output_file = "registered_tests.c"
    with open(output_file, 'w') as f:
        f.write(registration_function)
    
    print(f"\nCode written to {output_file}")

if __name__ == "__main__":
    main()
