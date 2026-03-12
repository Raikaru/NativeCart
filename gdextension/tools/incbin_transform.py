#!/usr/bin/env python3
"""
incbin_transform.py - Preprocessor for pokefirered INCBIN transformation

Transforms static array INCBIN declarations into null-initialized pointers:
    static const u16 arr[] = INCBIN_U16("path");
  ->
    static const u16 *arr = NULL;

Preserves original sources by writing to src_transformed/
"""

import os
import re
import json
import argparse
import shutil
from pathlib import Path
from typing import List, Dict, Tuple, Optional


# Pattern to match INCBIN declarations
# Matches: static const uN varname[] = INCBIN_UN("path");
INCBIN_PATTERN = re.compile(
    r'^\s*static\s+const\s+(u8|u16|u32)\s+(\w+)\s*\[\]\s*=\s*INCBIN_(U8|U16|U32)\s*\(\s*"([^"]+)"\s*\)\s*;',
    re.MULTILINE
)

# Alternative pattern for arrays with explicit sizes (sometimes used)
INCBIN_PATTERN_EXPLICIT = re.compile(
    r'^\s*static\s+const\s+(u8|u16|u32)\s+(\w+)\s*\[\d*\]\s*=\s*INCBIN_(U8|U16|U32)\s*\(\s*"([^"]+)"\s*\)\s*;',
    re.MULTILINE
)

# Pattern for _() string macro declarations
STRING_PATTERN = re.compile(
    r'^\s*static\s+const\s+u8\s+(\w+)\s*\[\]\s*=\s*_\(\s*"([^"]+)"\s*\)\s*;',
    re.MULTILINE
)


def extract_incbin_declarations(content: str) -> List[Dict]:
    """
    Extract all INCBIN declarations and string macro declarations from source content.
    
    Returns list of dicts with:
        - type: 'u8', 'u16', 'u32', or 'string'
        - varname: variable name
        - asset_path: path to binary asset (or string content for type 'string')
        - original_line: the original declaration line
    """
    declarations = []
    
    # Find all matches from both INCBIN patterns
    for pattern in [INCBIN_PATTERN, INCBIN_PATTERN_EXPLICIT]:
        for match in pattern.finditer(content):
            type_decl = match.group(1)  # u8, u16, or u32
            varname = match.group(2)
            incbin_type = match.group(3).lower()  # U8, U16, U32
            asset_path = match.group(4)
            
            declarations.append({
                'type': type_decl,
                'varname': varname,
                'asset_path': asset_path,
                'original_line': match.group(0),
                'incbin_type': incbin_type
            })
    
    # Find all _() string macro declarations
    for match in STRING_PATTERN.finditer(content):
        varname = match.group(1)
        string_content = match.group(2)
        
        declarations.append({
            'type': 'string',
            'varname': varname,
            'asset_path': string_content,
            'original_line': match.group(0),
            'incbin_type': None
        })
    
    return declarations


def transform_content(content: str) -> Tuple[str, List[Dict]]:
    """
    Transform source content, replacing INCBIN declarations with null pointers.
    
    Returns:
        (transformed_content, list_of_declarations)
    """
    declarations = extract_incbin_declarations(content)
    
    if not declarations:
        # No INCBIN declarations found, return unchanged
        return content, []
    
    transformed = content
    
    for decl in declarations:
        # Create replacement based on type
        if decl['type'] == 'string':
            # String macro: static const u8 *varname = NULL;
            replacement = f"static const u8 *{decl['varname']} = NULL;"
        else:
            # INCBIN: static const uN *varname = NULL;
            replacement = f"static const {decl['type']} *{decl['varname']} = NULL;"
        
        # Replace the declaration
        original = decl['original_line']
        transformed = transformed.replace(original, replacement, 1)
    
    return transformed, declarations


def process_file(
    source_path: Path,
    output_dir: Path,
    source_root: Path
) -> Optional[Dict]:
    """
    Process a single source file.
    
    Args:
        source_path: Path to source .c file
        output_dir: Root output directory for transformed files
        source_root: Root of source tree (to preserve structure)
    
    Returns:
        Dict with file info if transformations were made, None otherwise
    """
    try:
        with open(source_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {source_path}: {e}")
        return None
    
    transformed, declarations = transform_content(content)
    
    if not declarations:
        # No INCBIN found, copy file unchanged
        rel_path = source_path.relative_to(source_root)
        output_path = output_dir / rel_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, output_path)
        return None
    
    # Calculate output path preserving directory structure
    rel_path = source_path.relative_to(source_root)
    output_path = output_dir / rel_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    # Write transformed content
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(transformed)
    
    # Return transformation info
    return {
        'source_file': str(rel_path).replace('\\', '/'),
        'output_file': str(output_path.relative_to(output_dir)).replace('\\', '/'),
        'declarations': declarations
    }


def scan_and_transform(
    src_dir: Path,
    output_dir: Path,
    registry_path: Path
) -> int:
    """
    Scan source directory and transform all files with INCBIN declarations.
    
    Args:
        src_dir: Root of source tree (e.g., pokefirered-master/src/)
        output_dir: Output directory for transformed files
        registry_path: Path to write asset registry JSON
    
    Returns:
        Number of files transformed
    """
    if not src_dir.exists():
        raise FileNotFoundError(f"Source directory not found: {src_dir}")
    
    # Clean and recreate output directory
    if output_dir.exists():
        print(f"Cleaning existing output directory: {output_dir}")
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Find all .c files
    c_files = list(src_dir.rglob('*.c'))
    print(f"Found {len(c_files)} C source files to process")
    
    # Registry of all transformed assets
    asset_registry = []
    transformed_count = 0
    
    # Process each file
    for i, source_file in enumerate(c_files, 1):
        if i % 50 == 0:
            print(f"  Processing... {i}/{len(c_files)} files")
        
        result = process_file(source_file, output_dir, src_dir)
        
        if result:
            transformed_count += 1
            # Add to registry
            for decl in result['declarations']:
                asset_registry.append({
                    'varname': decl['varname'],
                    'type': decl['type'],
                    'asset_path': decl['asset_path'],
                    'source_file': result['source_file']
                })
    
    # Write registry
    registry_path.parent.mkdir(parents=True, exist_ok=True)
    with open(registry_path, 'w', encoding='utf-8') as f:
        json.dump(asset_registry, f, indent=2)
    
    print(f"\nTransformation complete:")
    print(f"  Files scanned: {len(c_files)}")
    print(f"  Files with INCBIN: {transformed_count}")
    print(f"  Total asset declarations: {len(asset_registry)}")
    print(f"  Registry written to: {registry_path}")
    
    return transformed_count


def main():
    parser = argparse.ArgumentParser(
        description='Transform pokefirered INCBIN declarations to null pointers'
    )
    parser.add_argument(
        '--src-dir',
        type=Path,
        default=Path('../src'),
        help='Source directory containing .c files (default: ../src)'
    )
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=Path('../../src_transformed'),
    )
    parser.add_argument(
        '--registry',
        type=Path,
        default=Path('asset_registry.json'),
    )
    
    args = parser.parse_args()
    
    # Ensure paths are absolute
    script_dir = Path(__file__).parent.absolute()
    
    src_dir = (script_dir / args.src_dir).resolve()
    output_dir = (script_dir / args.output_dir).resolve()
    registry_path = (script_dir / args.registry).resolve()
    
    print("=" * 70)
    print("INCBIN Transformer for PokeFireRed")
    print("=" * 70)
    print(f"Source: {src_dir}")
    print(f"Output: {output_dir}")
    print(f"Registry: {registry_path}")
    print("=" * 70)
    
    try:
        count = scan_and_transform(src_dir, output_dir, registry_path)
        return 0 if count >= 0 else 1
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    exit(main())
