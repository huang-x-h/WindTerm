#!/usr/bin/env python3
"""
Static analysis script for CSI 2026 Terminal module
Checks for common C++ syntax errors and issues
"""

import os
import re
import sys

def read_file(filepath):
    """Read file content"""
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def check_header_guards(content, filename):
    """Check for header guards in .h files"""
    guard_name = filename.replace('.', '_').upper()
    if f'#ifndef {guard_name}' not in content or f'#define {guard_name}' not in content:
        return f"Missing or incorrect header guards in {filename}"
    return None

def check_qobject_macro(content, filename):
    """Check for Q_OBJECT macro in QObject classes"""
    if 'class' in content and 'QObject' in content:
        if 'Q_OBJECT' not in content:
            return f"Missing Q_OBJECT macro in {filename}"
    return None

def check_include_guards(content, filename):
    """Check for proper include statements"""
    issues = []

    # Check for Qt includes
    qt_includes = re.findall(r'#include\s+<Q([A-Za-z]+)>', content)
    common_qt_headers = ['Object', 'ByteArray', 'List', 'Set', 'Hash', 'Pointer', 'String', 'DateTime']

    return None

def check_syntax_issues(content, filename):
    """Check for common syntax issues"""
    issues = []

    # Check for unmatched braces (simple check)
    open_braces = content.count('{')
    close_braces = content.count('}')
    if open_braces != close_braces:
        issues.append(f"Unmatched braces: {open_braces} open, {close_braces} close")

    # Check for unmatched parentheses (simple check)
    open_parens = content.count('(')
    close_parens = content.count(')')
    if open_parens != close_parens:
        issues.append(f"Unmatched parentheses: {open_parens} open, {close_parens} close")

    # Check for semicolons after class definitions
    if filename.endswith('.h'):
        class_pattern = r'class\s+\w+\s*:\s*public\s+\w+\s*\{[^}]*\}[^;]*$'
        if re.search(class_pattern, content, re.MULTILINE):
            if '#endif' not in content or not content.rstrip().endswith(';'):
                pass  # Complex check, skip for now

    return issues if issues else None

def check_function_definitions(h_content, cpp_content, filename_base):
    """Check that declared functions are defined"""
    # Extract function declarations from header
    func_pattern = r'(?:bool|void|int|QString|QByteArray|size_t|qint64)\s+(\w+)\s*\([^)]*\)(?:\s*const)?\s*;'
    declarations = re.findall(func_pattern, h_content)

    issues = []
    for func in declarations:
        if func == 'default':
            continue
        # Check if function is defined in cpp
        if func not in cpp_content:
            issues.append(f"Function '{func}' declared but not defined")

    return issues if issues else None

def analyze_file(filepath):
    """Analyze a single file"""
    filename = os.path.basename(filepath)
    content = read_file(filepath)

    issues = []

    # Header file checks
    if filename.endswith('.h'):
        result = check_header_guards(content, filename)
        if result:
            issues.append(result)

        result = check_qobject_macro(content, filename)
        if result:
            issues.append(result)

    # Common checks
    result = check_syntax_issues(content, filename)
    if result:
        issues.extend(result)

    return issues

def analyze_module(directory):
    """Analyze the entire Terminal module"""
    files = [
        'TerminalMode.h', 'TerminalMode.cpp',
        'SynchronizedBuffer.h', 'SynchronizedBuffer.cpp',
        'CSIParser.h', 'CSIParser.cpp',
        'TerminalHandler.h', 'TerminalHandler.cpp',
    ]

    print("=" * 60)
    print("CSI 2026 Terminal Module - Static Analysis")
    print("=" * 60)

    all_issues = []

    for filename in files:
        filepath = os.path.join(directory, filename)
        if not os.path.exists(filepath):
            print(f"\n[MISSING] {filename}")
            all_issues.append(f"Missing file: {filename}")
            continue

        print(f"\n[CHECKING] {filename}")
        issues = analyze_file(filepath)

        if issues:
            for issue in issues:
                print(f"  [ISSUE] {issue}")
                all_issues.append(f"{filename}: {issue}")
        else:
            print(f"  [OK] No obvious issues found")

    # Check header/implementation pairs
    print("\n" + "=" * 60)
    print("Checking header/implementation consistency...")
    print("=" * 60)

    pairs = [
        ('TerminalMode.h', 'TerminalMode.cpp'),
        ('SynchronizedBuffer.h', 'SynchronizedBuffer.cpp'),
        ('CSIParser.h', 'CSIParser.cpp'),
        ('TerminalHandler.h', 'TerminalHandler.cpp'),
    ]

    for h_file, cpp_file in pairs:
        h_path = os.path.join(directory, h_file)
        cpp_path = os.path.join(directory, cpp_file)

        if os.path.exists(h_path) and os.path.exists(cpp_path):
            h_content = read_file(h_path)
            cpp_content = read_file(cpp_path)
            issues = check_function_definitions(h_content, cpp_content, h_file.replace('.h', ''))

            if issues:
                print(f"\n[{h_file}/{cpp_file}]")
                for issue in issues:
                    print(f"  [ISSUE] {issue}")
                    all_issues.append(f"{h_file}: {issue}")
            else:
                print(f"  [OK] {h_file} <-> {cpp_file}")

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)

    if all_issues:
        print(f"\nFound {len(all_issues)} issue(s):")
        for issue in all_issues:
            print(f"  - {issue}")
        return 1
    else:
        print("\n[PASS] No obvious issues found in static analysis!")
        print("\nNote: This is basic syntax checking only.")
        print("Full compilation requires Qt5/Qt6 and a C++ compiler.")
        return 0

if __name__ == '__main__':
    terminal_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'src', 'Terminal')
    terminal_dir = os.path.abspath(terminal_dir)

    if not os.path.exists(terminal_dir):
        print(f"Error: Terminal directory not found: {terminal_dir}")
        sys.exit(1)

    sys.exit(analyze_module(terminal_dir))
