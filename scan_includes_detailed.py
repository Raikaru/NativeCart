import os
import re
import json
from pathlib import Path
from collections import defaultdict

# Directories to scan
SRC_DIR = Path("D:/Godot/pokefirered-master/src_transformed")
INCLUDE_DIR = Path("D:/Godot/pokefirered-master/include")
BASE_DIR = Path("D:/Godot/pokefirered-master")

# Search paths for includes (in order of priority)
SEARCH_PATHS = [
    "",  # Same directory as source
    "include",
    "include/gba",
    "include/constants",
    "src",
    "src/data",
    "src_transformed",
    "src_transformed/data",
]

def find_all_source_files():
    """Find all source files in the target directories."""
    extensions = ['.c', '.h', '.s', '.inc', '.asm']
    files = []
    for directory in [SRC_DIR, INCLUDE_DIR]:
        if directory.exists():
            for ext in extensions:
                files.extend(directory.rglob(f"*{ext}"))
    return files

def extract_includes(file_path):
    """Extract all #include directives from a file."""
    includes = []
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                match = re.match(r'^#include\s+([<"])([^>"]+)[>"]', line.strip())
                if match:
                    include_type = "system" if match.group(1) == "<" else "local"
                    include_path = match.group(2)
                    includes.append({
                        'line': line_num,
                        'path': include_path,
                        'type': include_type
                    })
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
    return includes

def check_file_exists(include_path, source_file):
    """Check if an included file exists in various search paths."""
    source_dir = source_file.parent
    
    # Try each search path
    for search_path in SEARCH_PATHS:
        if search_path == "":
            check_path = source_dir / include_path
        else:
            check_path = BASE_DIR / search_path / include_path
        
        if check_path.exists():
            return True, str(check_path.relative_to(BASE_DIR))
    
    return False, None

def analyze_incbin(file_path):
    """Extract INCBIN declarations from assembly files."""
    incbins = []
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                match = re.match(r'^\s*INCBIN\s+"([^"]+)"', line.strip(), re.IGNORECASE)
                if match:
                    incbins.append({
                        'line': line_num,
                        'path': match.group(1)
                    })
    except Exception as e:
        pass
    return incbins

def get_hint(include_path):
    """Get a hint about what type of file this might be."""
    if include_path.startswith('constants/'):
        return "Generated: constants from build system"
    elif include_path.startswith('data/'):
        return "Generated: data file from build system"
    elif include_path.endswith('.inc'):
        return "Generated: assembly include file"
    elif '/s' in include_path and (include_path.endswith('.h') or include_path.endswith('.c')):
        return "Generated: sprite data"
    elif 'graphics' in include_path.lower():
        return "Generated: graphics data"
    elif 'tile' in include_path.lower() or 'palette' in include_path.lower():
        return "Generated: tile/palette data"
    elif include_path.startswith('gba/'):
        return "GBA platform header"
    elif include_path in ['limits.h', 'string.h', 'stdarg.h', 'stdio.h', 'stdint.h', 'stddef.h', 'stdlib.h', 'stdbool.h', 'string.h']:
        return "Standard C library (compiler provided)"
    else:
        return "Unknown"

def main():
    print("=" * 80)
    print("COMPREHENSIVE #include SCAN")
    print("=" * 80)
    
    print("\nScanning for source files...")
    source_files = find_all_source_files()
    print(f"Found {len(source_files)} source files")
    
    all_includes = []
    all_incbins = []
    
    print("\nExtracting #include directives...")
    for i, file_path in enumerate(source_files):
        if i % 100 == 0:
            print(f"  Processing file {i+1}/{len(source_files)}...")
        
        includes = extract_includes(file_path)
        for inc in includes:
            exists, found_path = check_file_exists(inc['path'], file_path)
            all_includes.append({
                'source_file': str(file_path.relative_to(BASE_DIR)),
                'source_line': inc['line'],
                'include_path': inc['path'],
                'include_type': inc['type'],
                'exists': exists,
                'found_at': found_path
            })
        
        # Also check INCBIN directives in .s and .inc files
        if file_path.suffix in ['.s', '.inc', '.asm']:
            incbins = analyze_incbin(file_path)
            for incbin in incbins:
                exists, found_path = check_file_exists(incbin['path'], file_path)
                all_incbins.append({
                    'source_file': str(file_path.relative_to(BASE_DIR)),
                    'source_line': incbin['line'],
                    'incbin_path': incbin['path'],
                    'exists': exists,
                    'found_at': found_path
                })
    
    # Categorize missing includes
    missing_includes = [inc for inc in all_includes if not inc['exists']]
    
    # Group by hint type
    by_hint = defaultdict(list)
    for inc in missing_includes:
        hint = get_hint(inc['include_path'])
        by_hint[hint].append(inc)
    
    # Print detailed report
    print("\n" + "=" * 80)
    print("MISSING #include FILES REPORT")
    print("=" * 80)
    
    for hint_type in sorted(by_hint.keys()):
        includes = by_hint[hint_type]
        print(f"\n### {hint_type} ({len(includes)} files) ###")
        
        # Group by source file
        by_source = defaultdict(list)
        for inc in includes:
            by_source[inc['source_file']].append(inc)
        
        for source_file in sorted(by_source.keys()):
            incs = by_source[source_file]
            print(f"\n  {source_file}:")
            for inc in incs:
                type_str = f"<{inc['include_path']}>" if inc['include_type'] == 'system' else f'"{inc["include_path"]}"'
                print(f"    Line {inc['source_line']}: {type_str}")
    
    # Summary statistics
    print(f"\n{'='*80}")
    print("SUMMARY")
    print("=" * 80)
    print(f"Total source files scanned: {len(source_files)}")
    print(f"Total #include directives: {len(all_includes)}")
    print(f"Total missing includes: {len(missing_includes)}")
    print(f"  - Standard C library headers: {len(by_hint.get('Standard C library (compiler provided)', []))}")
    print(f"  - Generated constants: {len(by_hint.get('Generated: constants from build system', []))}")
    print(f"  - Generated data files: {len(by_hint.get('Generated: data file from build system', []))}")
    print(f"  - Generated .inc files: {len(by_hint.get('Generated: assembly include file', []))}")
    print(f"  - Generated sprite data: {len(by_hint.get('Generated: sprite data', []))}")
    print(f"  - Graphics data: {len(by_hint.get('Generated: graphics data', []))}")
    print(f"  - Tile/palette data: {len(by_hint.get('Generated: tile/palette data', []))}")
    print(f"  - Unknown: {len(by_hint.get('Unknown', []))}")
    
    # INCBIN report
    if all_incbins:
        missing_incbins = [inc for inc in all_incbins if not inc['exists']]
        print(f"\nTotal INCBIN directives: {len(all_incbins)}")
        print(f"Missing INCBIN files: {len(missing_incbins)}")
        
        if missing_incbins:
            print(f"\n{'='*80}")
            print("MISSING INCBIN FILES (Top 20)")
            print("=" * 80)
            
            for inc in missing_incbins[:20]:
                print(f"  {inc['source_file']}:{inc['source_line']} -> \"{inc['incbin_path']}\"")
    
    # Save to JSON
    output = {
        'missing_includes': missing_includes,
        'missing_incbins': [inc for inc in all_incbins if not inc['exists']],
        'stats': {
            'total_files': len(source_files),
            'total_includes': len(all_includes),
            'missing_includes': len(missing_includes),
            'total_incbins': len(all_incbins),
            'missing_incbins': len([inc for inc in all_incbins if not inc['exists']])
        }
    }
    
    output_path = BASE_DIR / 'include_scan_results.json'
    with open(output_path, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\nDetailed results saved to: {output_path}")

if __name__ == '__main__':
    main()
