#!/usr/bin/env python3
"""
Comprehensive Symbol Analysis Tool for Pokemon FireRed GBA Decompilation
Focuses on predicting undefined reference errors at link time
"""

import os
import re
import glob
import json
from collections import defaultdict

class ComprehensiveSymbolAnalyzer:
    def __init__(self, base_path):
        self.base_path = base_path
        self.symbols = {}  # symbol_name -> detailed info
        self.gba_bios_funcs = set([
            'SoftReset', 'RegisterRamReset', 'VBlankIntrWait', 'Sqrt', 'ArcTan2',
            'CpuSet', 'CpuFastSet', 'BgAffineSet', 'ObjAffineSet', 'LZ77UnCompWram',
            'LZ77UnCompVram', 'RLUnCompWram', 'RLUnCompVram', 'MultiBoot', 'Div'
        ])
        self.standard_funcs = set([
            'abs', 'min', 'max', 'memcpy', 'memset', 'strlen', 'strcpy', 'strcat',
            'strcmp', 'sprintf', 'malloc', 'free', 'printf', 'scanf', 'qsort',
            'rand', 'srand', 'pow', 'sqrt', 'sin', 'cos', 'tan', 'atan2'
        ])
        self.macro_patterns = [
            r'^[A-Z][A-Z0-9_]+$',  # ALL_CAPS macros
            r'T\d+_READ_\d+',  # T1_READ_32, etc
            r'GET_', r'IS_', r'FLAG_', r'ITEM_', r'SPRITE_',
            r'EWRAM_DATA', r'IWRAM_DATA', r'COMMON_DATA', r'STATIC'
        ]
        
    def is_likely_macro(self, name):
        """Check if something is likely a macro"""
        for pattern in self.macro_patterns:
            if re.match(pattern, name):
                return True
        return False
    
    def scan_files(self):
        """Scan all source files"""
        c_files = glob.glob(os.path.join(self.base_path, "src_transformed/**/*.c"), recursive=True)
        cpp_files = glob.glob(os.path.join(self.base_path, "gdextension/src/**/*.cpp"), recursive=True)
        h_files = glob.glob(os.path.join(self.base_path, "include/**/*.h"), recursive=True)
        
        print(f"Scanning {len(c_files)} C files...")
        print(f"Scanning {len(cpp_files)} C++ files...")
        print(f"Scanning {len(h_files)} header files...")
        
        all_files = c_files + cpp_files + h_files
        
        for filepath in all_files:
            self.scan_file(filepath)
    
    def scan_file(self, filepath):
        """Scan a single file for symbols"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return
        
        in_multiline_comment = False
        
        for line_num, line in enumerate(lines, 1):
            # Handle multiline comments
            if '/*' in line:
                if '*/' not in line:
                    in_multiline_comment = True
                continue
            if in_multiline_comment:
                if '*/' in line:
                    in_multiline_comment = False
                continue
            
            # Remove single-line comments
            line_clean = re.sub(r'//.*$', '', line)
            
            # Pattern 1: Function definitions
            func_def_match = re.match(
                r'^\s*(\w+\s+)(\*?\w+)\s*\(([^)]*)\)\s*$',
                line_clean
            )
            if func_def_match:
                next_line_idx = line_num
                if next_line_idx < len(lines):
                    next_line = lines[next_line_idx].strip()
                    if next_line.startswith('{') or next_line.startswith('{'):
                        func_name = func_def_match.group(2)
                        if func_name not in self.symbols:
                            self.symbols[func_name] = {
                                'type': 'function',
                                'definitions': [],
                                'declarations': [],
                                'uses': []
                            }
                        self.symbols[func_name]['definitions'].append({
                            'file': filepath,
                            'line': line_num
                        })
            
            # Pattern 2: Function declarations (with semicolon)
            func_decl_match = re.match(
                r'^\s*(extern\s+)?(\w+\s+)(\*?\w+)\s*\(([^)]*)\)\s*;',
                line_clean
            )
            if func_decl_match:
                func_name = func_decl_match.group(3)
                if func_name not in self.symbols:
                    self.symbols[func_name] = {
                        'type': 'function',
                        'definitions': [],
                        'declarations': [],
                        'uses': []
                    }
                self.symbols[func_name]['declarations'].append({
                    'file': filepath,
                    'line': line_num,
                    'is_extern': bool(func_decl_match.group(1))
                })
            
            # Pattern 3: Variable declarations with extern
            var_decl_match = re.match(
                r'^\s*extern\s+(const\s+)?(\w+\s+)(\*?\w+)',
                line_clean
            )
            if var_decl_match:
                var_name = var_decl_match.group(3)
                if var_name not in self.symbols:
                    self.symbols[var_name] = {
                        'type': 'variable',
                        'definitions': [],
                        'declarations': [],
                        'uses': [],
                        'is_extern': True
                    }
                self.symbols[var_name]['declarations'].append({
                    'file': filepath,
                    'line': line_num
                })
            
            # Pattern 4: Variable definitions with GBA sections
            gba_var_match = re.match(
                r'^\s*(EWRAM_DATA|IWRAM_DATA|COMMON_DATA|EWRAM_BSS)\s+(\w+\s+)(\*?\w+)',
                line_clean
            )
            if gba_var_match:
                var_name = gba_var_match.group(3)
                if var_name not in self.symbols:
                    self.symbols[var_name] = {
                        'type': 'variable',
                        'definitions': [],
                        'declarations': [],
                        'uses': [],
                        'section': gba_var_match.group(1)
                    }
                self.symbols[var_name]['definitions'].append({
                    'file': filepath,
                    'line': line_num
                })
    
    def find_undefined_symbols(self):
        """Find symbols that are declared/used but not defined"""
        undefined = []
        
        for symbol_name, symbol_info in self.symbols.items():
            # Skip standard library functions
            if symbol_name in self.standard_funcs:
                continue
            
            # Skip GBA BIOS functions (provided by hardware)
            if symbol_name in self.gba_bios_funcs:
                continue
            
            # Skip macros
            if self.is_likely_macro(symbol_name):
                continue
            
            # Check if symbol has declarations but no definition
            if symbol_info['declarations'] and not symbol_info.get('definitions'):
                undefined.append({
                    'name': symbol_name,
                    'type': symbol_info['type'],
                    'declarations': symbol_info['declarations']
                })
        
        return undefined
    
    def categorize_symbol(self, name):
        """Categorize a symbol by type/purpose"""
        if name.startswith('g') and name[1].isupper():
            return 'global_variable'
        elif 'sound' in name.lower() or 'm4a' in name.lower() or 'music' in name.lower():
            return 'sound_music'
        elif 'irq' in name.lower() or 'intr' in name.lower() or 'interrupt' in name.lower():
            return 'interrupt'
        elif 'timer' in name.lower():
            return 'timer'
        elif 'dma' in name.lower():
            return 'dma'
        elif 'flash' in name.lower() or 'save' in name.lower():
            return 'save_flash'
        elif 'battle' in name.lower():
            return 'battle'
        elif 'script' in name.lower():
            return 'script'
        elif 'sprite' in name.lower() or 'oam' in name.lower():
            return 'sprite'
        elif 'bg' in name.lower():
            return 'background'
        else:
            return 'other'
    
    def generate_report(self):
        """Generate comprehensive report"""
        undefined = self.find_undefined_symbols()
        
        # Categorize undefined symbols
        categorized = defaultdict(list)
        for sym in undefined:
            category = self.categorize_symbol(sym['name'])
            categorized[category].append(sym)
        
        print("\n" + "="*80)
        print("COMPREHENSIVE SYMBOL ANALYSIS REPORT")
        print("Pokemon FireRed GBA Decompilation - Predicted Linker Errors")
        print("="*80)
        
        print(f"\nTotal symbols scanned: {len(self.symbols)}")
        print(f"Symbols with NO DEFINITION: {len(undefined)}")
        
        # Print by category
        priority_categories = [
            'interrupt', 'sound_music', 'save_flash', 'timer', 'dma',
            'global_variable', 'battle', 'script', 'sprite', 'background', 'other'
        ]
        
        for category in priority_categories:
            if category in categorized and categorized[category]:
                print(f"\n{'='*80}")
                print(f"CATEGORY: {category.upper().replace('_', ' ')}")
                print(f"{'='*80}")
                
                for sym in sorted(categorized[category], key=lambda x: x['name']):
                    print(f"\n[!] {sym['name']} ({sym['type']})")
                    print("    Declared at:")
                    for decl in sym['declarations'][:2]:
                        short_file = decl['file'].replace(self.base_path, '')
                        print(f"      - {short_file}:{decl['line']}")
                    if len(sym['declarations']) > 2:
                        print(f"      ... and {len(sym['declarations']) - 2} more")
        
        # Summary
        print(f"\n{'='*80}")
        print("SUMMARY BY CATEGORY")
        print(f"{'='*80}")
        for category in priority_categories:
            count = len(categorized.get(category, []))
            if count > 0:
                print(f"  {category:20s}: {count:4d} symbols")

if __name__ == "__main__":
    base_path = "D:/Godot/pokefirered-master"
    analyzer = ComprehensiveSymbolAnalyzer(base_path)
    analyzer.scan_files()
    analyzer.generate_report()
