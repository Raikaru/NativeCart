import os
import re
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path("D:/Godot/pokefirered-master")

# Files that are expected to exist but are generated during build
generated_patterns = [
    'data/layouts/*.inc',
    'data/maps/*.inc', 
    'data/maps/*/header.inc',
    'data/events.inc',
    'data/graphics/*.inc',
]

def find_all_source_files():
    extensions = ['.c', '.h', '.s', '.inc']
    files = []
    for ext in extensions:
        files.extend(BASE_DIR.rglob(f"*{ext}"))
    return files

def extract_includes(file_path):
    includes = []
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                # Match C-style #include
                c_match = re.match(r'^#include\s+([<"])([^>"]+)[>"]', line.strip())
                if c_match:
                    includes.append({
                        'line': line_num,
                        'path': c_match.group(2),
                        'type': 'c',
                        'is_system': c_match.group(1) == '<'
                    })
                # Match assembly-style .include
                asm_match = re.match(r'^\s*\.include\s+"([^"]+)"', line.strip())
                if asm_match:
                    includes.append({
                        'line': line_num,
                        'path': asm_match.group(1),
                        'type': 'asm',
                        'is_system': False
                    })
    except Exception as e:
        pass
    return includes

def file_exists(rel_path):
    full_path = BASE_DIR / rel_path
    return full_path.exists()

def is_standard_c_header(path):
    standard_headers = [
        'limits.h', 'string.h', 'stdarg.h', 'stdio.h', 'stdint.h', 
        'stddef.h', 'stdlib.h', 'stdbool.h', 'assert.h', 'ctype.h',
        'errno.h', 'float.h', 'iso646.h', 'locale.h', 'math.h',
        'setjmp.h', 'signal.h', 'stdalign.h', 'stdatomic.h', 'stdbit.h',
        'stdckdint.h', 'stdfix.h', 'stdint.h', 'stdioint.h', 'stdnoreturn.h',
        'string.h', 'tgmath.h', 'threads.h', 'time.h', 'uchar.h',
        'wchar.h', 'wctype.h'
    ]
    return path in standard_headers or path.startswith('sys/')

def categorize_missing(path):
    if 'data/layouts' in path and path.endswith('.inc'):
        return "Generated: Layout data from .json files"
    elif 'data/maps' in path and path.endswith('.inc'):
        if 'header' in path:
            return "Generated: Map header from map.json"
        elif 'groups' in path:
            return "Generated: Map groups table"
        elif 'connections' in path:
            return "Generated: Map connections"
        elif 'events' in path:
            return "Generated: Map events"
    elif 'data/graphics' in path:
        return "Generated: Graphics data"
    elif 'data' in path and path.endswith('.inc'):
        return "Generated: Data file"
    elif 'constants' in path:
        return "Generated: Constants"
    elif path.startswith('gba/'):
        return "GBA Platform header"
    elif is_standard_c_header(path):
        return "Standard C library"
    else:
        return "Unknown"

def main():
    print("=" * 80)
    print("MISSING #include/.include DIRECTIVES REPORT")
    print("=" * 80)
    
    source_files = find_all_source_files()
    print(f"\nScanned {len(source_files)} source files")
    
    all_includes = []
    for file_path in source_files:
        includes = extract_includes(file_path)
        for inc in includes:
            all_includes.append({
                'source_file': str(file_path.relative_to(BASE_DIR)),
                'source_line': inc['line'],
                'include_path': inc['path'],
                'include_type': inc['type'],
                'is_system': inc['is_system'],
                'exists': file_exists(inc['path'])
            })
    
    missing = [inc for inc in all_includes if not inc['exists']]
    
    # Filter out standard C library headers
    missing_non_standard = [inc for inc in missing if not is_standard_c_header(inc['include_path'])]
    
    print(f"\nTotal includes found: {len(all_includes)}")
    print(f"Missing includes (including standard C): {len(missing)}")
    print(f"Missing includes (non-standard only): {len(missing_non_standard)}")
    
    if missing_non_standard:
        print("\n" + "=" * 80)
        print("MISSING NON-STANDARD INCLUDES")
        print("=" * 80)
        
        # Group by category
        by_category = defaultdict(list)
        for inc in missing_non_standard:
            cat = categorize_missing(inc['include_path'])
            by_category[cat].append(inc)
        
        for category in sorted(by_category.keys()):
            includes = by_category[category]
            print(f"\n### {category} ({len(includes)} files) ###")
            
            # Group by source file
            by_source = defaultdict(list)
            for inc in includes:
                by_source[inc['source_file']].append(inc)
            
            for source_file in sorted(by_source.keys()):
                incs = by_source[source_file]
                print(f"\n  From: {source_file}")
                for inc in incs:
                    if inc['include_type'] == 'c':
                        if inc['is_system']:
                            print(f"    Line {inc['source_line']}: <{inc['include_path']}>")
                        else:
                            print(f"    Line {inc['source_line']}: \"{inc['include_path']}\"")
                    else:
                        print(f"    Line {inc['source_line']}: .include \"{inc['include_path']}\"")
    
    # Standard C library headers (for reference)
    missing_standard = [inc for inc in missing if is_standard_c_header(inc['include_path'])]
    if missing_standard:
        print("\n" + "=" * 80)
        print("STANDARD C LIBRARY HEADERS (provided by compiler)")
        print("=" * 80)
        by_source = defaultdict(list)
        for inc in missing_standard:
            by_source[inc['source_file']].append(inc)
        
        for source_file in sorted(by_source.keys()):
            incs = by_source[source_file]
            print(f"\n  {source_file}:")
            for inc in incs:
                print(f"    Line {inc['source_line']}: <{inc['include_path']}>")
    
    print("\n" + "=" * 80)
    print("SUMMARY BY CATEGORY")
    print("=" * 80)
    for category in sorted(by_category.keys()):
        print(f"  {category}: {len(by_category[category])} references")
    print(f"  Standard C library: {len(missing_standard)} references")

if __name__ == '__main__':
    main()
