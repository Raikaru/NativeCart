#!/usr/bin/env python3
"""
fix_keywords.py - Fixes C++ keyword conflicts in pokefirered source.

Main fix: Renames struct member `template` to `spriteTemplate` in struct Sprite
to allow compilation with g++ (C++ compiler).

The script:
1. Scans all .c and .h files in src_transformed/ and include/
2. Renames `template` to `spriteTemplate` in struct Sprite member declarations
3. Updates ALL usages of this member throughout the codebase
4. Is idempotent (safe to run multiple times)
"""

import os
import re
import sys
from pathlib import Path

# Configuration
SRC_DIR = Path("D:/Godot/pokefirered-master/src_transformed")
INCLUDE_DIR = Path("D:/Godot/pokefirered-master/include")

# Track modifications
files_modified = 0
replacements_made = 0

def is_already_fixed(content):
    """Check if file has already been fixed (idempotency check)."""
    # Look for the fixed struct member declaration
    has_template_fixed = "const struct SpriteTemplate *spriteTemplate;" in content
    has_protected_fixed = "protectedFlag:" in content
    return has_template_fixed or has_protected_fixed

def fix_sprite_template_member(content):
    """Fix the struct member declaration in sprite.h."""
    global replacements_made
    
    # Pattern for the struct member declaration
    # Match: const struct SpriteTemplate *template;
    pattern = r'(\/\*0x14\*\/\s*)const struct SpriteTemplate \*template;'
    replacement = r'\1const struct SpriteTemplate *spriteTemplate;'
    
    new_content, count = re.subn(pattern, replacement, content)
    if count > 0:
        replacements_made += count
        print(f"  - Fixed struct member declaration ({count} occurrence(s))")
    
    return new_content

def fix_arrow_template_usages(content):
    """Fix sprite->template usages."""
    global replacements_made
    
    # Pattern: sprite->template (but not if already spriteTemplate)
    # We need to be careful to only match struct member access, not variable names
    # Look for ->template followed by -> or . or ; or ) or end of line
    pattern = r'(\w+\s*->\s*)template(\s*(?:->|\.|;|\)|,|\s+|=|\[))'
    
    def replace_match(match):
        prefix = match.group(1)
        suffix = match.group(2)
        # Check if this looks like a Sprite struct access
        # Common patterns: sprite->template, gSprites[i].template, etc.
        if any(x in prefix for x in ['sprite', 'Sprite']):
            return f"{prefix}spriteTemplate{suffix}"
        return match.group(0)
    
    new_content = re.sub(pattern, replace_match, content)
    count = len(re.findall(pattern, content))
    if new_content != content:
        actual_changes = content.count('->template') - new_content.count('->template')
        if actual_changes > 0:
            replacements_made += actual_changes
            print(f"  - Fixed ->template usages ({actual_changes} occurrence(s))")
    
    return new_content

def fix_dot_template_usages_in_structs(content):
    """Fix .template usages in struct initializations and comparisons."""
    global replacements_made
    
    # Pattern for struct initialization: .template = 
    # or comparisons: .template == 
    # This specifically targets the Sprite struct member
    
    # Fix: .template = &gDummySpriteTemplate
    pattern1 = r'\.template\s*='
    
    def replace_init(match):
        return ".spriteTemplate ="
    
    new_content = re.sub(pattern1, replace_init, content)
    
    # Fix: .template == or .template !=
    pattern2 = r'\.template\s*(==|!=)'
    
    def replace_compare(match):
        op = match.group(1)
        return f".spriteTemplate {op}"
    
    new_content = re.sub(pattern2, replace_compare, new_content)
    
    if new_content != content:
        changes = content.count('.template') - new_content.count('.template')
        if changes > 0:
            replacements_made += changes
            print(f"  - Fixed .template struct usages ({changes} occurrence(s))")
    
    return new_content

def fix_gsprites_access(content):
    """Specific fix for gSprites[i].template patterns."""
    global replacements_made
    
    # Pattern: gSprites[...].template
    pattern = r'(gSprites\[[^\]]+\])\.template'
    replacement = r'\1.spriteTemplate'
    
    new_content, count = re.subn(pattern, replacement, content)
    if count > 0:
        replacements_made += count
        print(f"  - Fixed gSprites[].template ({count} occurrence(s))")
    
    return new_content

# ===== PROTECTED keyword fixes =====

def fix_protected_bitfield_declarations(content):
    """Fix bitfield declarations like 'u32 protected:1;' -> 'u32 protectedFlag:1;'."""
    global replacements_made
    
    # Pattern: type protected:N; where N is a number (bitfield syntax)
    # Matches: u32 protected:1;, u8 protected:2;, etc.
    # Requires whitespace or start of line before type to avoid matching C++ keyword
    pattern = r'((?:^|\s+)u\d+\s+)protected(:\s*\d+\s*;)'
    replacement = r'\1protectedFlag\2'
    
    new_content, count = re.subn(pattern, replacement, content)
    if count > 0:
        replacements_made += count
        print(f"  - Fixed protected bitfield declarations ({count} occurrence(s))")
    
    return new_content

def fix_arrow_protected_usages(content):
    """Fix ->protected usages."""
    global replacements_made
    
    # Pattern: ->protected followed by terminator or end of line/string
    pattern = r'(\w+\s*->\s*)protected(\s*(?:->|\.|;|\)|,|\s+|=|\[|$))'
    
    def replace_match(match):
        prefix = match.group(1)
        suffix = match.group(2)
        return f"{prefix}protectedFlag{suffix}"
    
    new_content = re.sub(pattern, replace_match, content)
    if new_content != content:
        actual_changes = content.count('->protected') - new_content.count('->protected')
        if actual_changes > 0:
            replacements_made += actual_changes
            print(f"  - Fixed ->protected usages ({actual_changes} occurrence(s))")
    
    return new_content

def fix_dot_protected_usages(content):
    """Fix .protected usages in struct initializations and comparisons."""
    global replacements_made
    
    # Pattern for struct initialization: .protected =
    pattern1 = r'\.protected\s*='
    
    def replace_init(match):
        return ".protectedFlag ="
    
    new_content = re.sub(pattern1, replace_init, content)
    
    # Pattern for comparisons: .protected == or .protected !=
    pattern2 = r'\.protected\s*(==|!=)'
    
    def replace_compare(match):
        op = match.group(1)
        return f".protectedFlag {op}"
    
    new_content = re.sub(pattern2, replace_compare, new_content)
    
    if new_content != content:
        changes = content.count('.protected') - new_content.count('.protected')
        if changes > 0:
            replacements_made += changes
            print(f"  - Fixed .protected struct usages ({changes} occurrence(s))")
    
    return new_content

def process_file(filepath, apply_protected_fixes=False):
    """Process a single file and apply fixes."""
    global files_modified
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return
    
    original_content = content
    
    # Skip if already fixed (idempotency)
    if is_already_fixed(content):
        print(f"Skipping {filepath} (already fixed)")
        return
    
    print(f"Processing: {filepath}")
    
    # Apply template fixes (all files)
    content = fix_sprite_template_member(content)
    content = fix_gsprites_access(content)
    content = fix_arrow_template_usages(content)
    content = fix_dot_template_usages_in_structs(content)
    
    # Apply protected fixes (only for src_transformed/)
    if apply_protected_fixes:
        content = fix_protected_bitfield_declarations(content)
        content = fix_arrow_protected_usages(content)
        content = fix_dot_protected_usages(content)
    
    # Write back if modified
    if content != original_content:
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            files_modified += 1
            print(f"  -> Modified")
        except Exception as e:
            print(f"Error writing {filepath}: {e}")
    else:
        print(f"  -> No changes needed")

def main():
    print("="*60)
    print("Fix Keywords Script - C++ Compatibility for pokefirered")
    print("="*60)
    print()
    
    # Verify directories exist
    if not SRC_DIR.exists():
        print(f"Error: Source directory not found: {SRC_DIR}")
        sys.exit(1)
    if not INCLUDE_DIR.exists():
        print(f"Error: Include directory not found: {INCLUDE_DIR}")
        sys.exit(1)
    
    print(f"Scanning directories:")
    print(f"  - {SRC_DIR}")
    print(f"  - {INCLUDE_DIR}")
    print()
    
    # Collect all files
    files_to_process = []
    
    # Add .c files from src_transformed
    for filepath in SRC_DIR.rglob("*.c"):
        files_to_process.append(filepath)
    
    # Add .h files from include
    for filepath in INCLUDE_DIR.rglob("*.h"):
        files_to_process.append(filepath)
    
    print(f"Found {len(files_to_process)} files to scan")
    print()
    
    # Process each file
    for filepath in sorted(files_to_process):
        # Only apply protected fixes to src_transformed/, never to include/
        apply_protected = filepath.is_relative_to(SRC_DIR)
        process_file(filepath, apply_protected_fixes=apply_protected)
    
    print()
    print("="*60)
    print("SUMMARY")
    print("="*60)
    print(f"Files modified: {files_modified}")
    print(f"Total replacements: {replacements_made}")
    print()
    
    if files_modified > 0:
        print("SUCCESS: C++ keyword conflicts fixed!")
        print("The codebase should now compile with g++.")
    else:
        print("No changes were necessary (files may already be fixed).")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
