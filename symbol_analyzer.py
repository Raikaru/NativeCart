#!/usr/bin/env python3
"""
Symbol Analysis Tool for Pokemon FireRed GBA Decompilation
Scans for potential undefined reference errors at link time
"""

import os
import re
import glob
from collections import defaultdict

class SymbolAnalyzer:
    def __init__(self, base_path):
        self.base_path = base_path
        self.symbols = {}  # symbol_name -> {type, declarations, definition}
        self.undefined_symbols = []
        
    def scan_files(self):
        """Scan all relevant source files"""
        c_files = glob.glob(os.path.join(self.base_path, "src_transformed/**/*.c"), recursive=True)
        cpp_files = glob.glob(os.path.join(self.base_path, "gdextension/src/**/*.cpp"), recursive=True)
        h_files = glob.glob(os.path.join(self.base_path, "src_transformed/**/*.h"), recursive=True)
        
        print(f"Found {len(c_files)} C files")
        print(f"Found {len(cpp_files)} C++ files")
        print(f"Found {len(h_files)} header files")
        
        # Scan each file type
        for filepath in c_files + cpp_files:
            self.scan_c_file(filepath)
            
        for filepath in h_files:
            self.scan_header_file(filepath)
    
    def scan_c_file(self, filepath):
        """Scan a C/C++ file for symbols"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            return
            
        filename = os.path.basename(filepath)
        
        for line_num, line in enumerate(lines, 1):
            # Skip comments
            line_clean = re.sub(r'//.*$', '', line)
            
            # Pattern 1: Function declarations (ending with semicolon, no body)
            func_decl_match = re.match(
                r'^\s*(extern\s+)?(\w+\s+)(\*?\w+)\s*\(([^)]*)\)\s*;',
                line_clean
            )
            if func_decl_match:
                is_extern = bool(func_decl_match.group(1))
                return_type = func_decl_match.group(2).strip()
                func_name = func_decl_match.group(3)
                params = func_decl_match.group(4)
                
                if func_name not in self.symbols:
                    self.symbols[func_name] = {
                        'type': 'function',
                        'declarations': [],
                        'definition': None,
                        'is_extern': is_extern
                    }
                self.symbols[func_name]['declarations'].append({
                    'file': filepath,
                    'line': line_num,
                    'is_extern': is_extern
                })
            
            # Pattern 2: Function definitions (with body)
            func_def_match = re.match(
                r'^\s*(\w+\s+)(\*?\w+)\s*\(([^)]*)\)\s*\{',
                line_clean
            )
            if func_def_match:
                return_type = func_def_match.group(1).strip()
                func_name = func_def_match.group(2)
                
                if func_name in self.symbols:
                    if not self.symbols[func_name].get('definition'):
                        self.symbols[func_name]['definition'] = {
                            'file': filepath,
                            'line': line_num
                        }
            
            # Pattern 3: Variable declarations with extern
            var_decl_match = re.match(
                r'^\s*extern\s+(\w+\s+)(\*?\w+)\s*(?:\[([^\]]*)\])?\s*;',
                line_clean
            )
            if var_decl_match:
                var_type = var_decl_match.group(1).strip()
                var_name = var_decl_match.group(2)
                array_size = var_decl_match.group(3)
                
                if var_name not in self.symbols:
                    self.symbols[var_name] = {
                        'type': 'variable',
                        'declarations': [],
                        'definition': None,
                        'is_extern': True
                    }
                self.symbols[var_name]['declarations'].append({
                    'file': filepath,
                    'line': line_num,
                    'is_extern': True
                })
            
            # Pattern 4: Global variable definitions (EWRAM_DATA, etc.)
            gba_var_match = re.match(
                r'^\s*(EWRAM_DATA|IWRAM_DATA|VRAM|SRAM|EWRAM_BSS)\s+(\w+\s+)(\*?\w+)',
                line_clean
            )
            if gba_var_match:
                section = gba_var_match.group(1)
                var_type = gba_var_match.group(2).strip()
                var_name = gba_var_match.group(3)
                
                if var_name in self.symbols:
                    if not self.symbols[var_name].get('definition'):
                        self.symbols[var_name]['definition'] = {
                            'file': filepath,
                            'line': line_num,
                            'section': section
                        }
    
    def scan_header_file(self, filepath):
        """Scan header files for declarations"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return
            
        for line_num, line in enumerate(lines, 1):
            # Skip comments
            line_clean = re.sub(r'//.*$', '', line)
            
            # Function declarations in headers
            func_decl_match = re.match(
                r'^\s*(\w+\s+)(\*?\w+)\s*\(([^)]*)\)\s*;',
                line_clean
            )
            if func_decl_match:
                func_name = func_decl_match.group(2)
                if func_name not in self.symbols:
                    self.symbols[func_name] = {
                        'type': 'function',
                        'declarations': [],
                        'definition': None,
                        'is_header_decl': True
                    }
                self.symbols[func_name]['declarations'].append({
                    'file': filepath,
                    'line': line_num,
                    'is_header': True
                })
    
    def find_undefined_symbols(self):
        """Find symbols that are declared/used but not defined"""
        undefined = []
        
        for symbol_name, symbol_info in self.symbols.items():
            # Check if symbol has declarations but no definition
            if symbol_info['declarations'] and not symbol_info.get('definition'):
                # Skip if it's just a header declaration that might be defined elsewhere
                undefined.append({
                    'name': symbol_name,
                    'type': symbol_info['type'],
                    'declarations': symbol_info['declarations']
                })
        
        return undefined
    
    def generate_report(self):
        """Generate a comprehensive report"""
        print("\n" + "="*80)
        print("SYMBOL ANALYSIS REPORT - Pokemon FireRed GBA Decompilation")
        print("="*80)
        
        undefined = self.find_undefined_symbols()
        
        print(f"\nTotal symbols scanned: {len(self.symbols)}")
        print(f"Symbols with potential undefined reference errors: {len(undefined)}")
        
        if undefined:
            print("\n" + "-"*80)
            print("PREDICTED LINKER ERRORS (Undefined References)")
            print("-"*80)
            
            for sym in undefined:
                print(f"\n[!] UNDEFINED: {sym['name']} ({sym['type']})")
                print("    Declared at:")
                for decl in sym['declarations'][:3]:  # Show first 3
                    print(f"      - {decl['file']}:{decl['line']}")
                if len(sym['declarations']) > 3:
                    print(f"      ... and {len(sym['declarations']) - 3} more")

if __name__ == "__main__":
    base_path = "D:/Godot/pokefirered-master"
    analyzer = SymbolAnalyzer(base_path)
    analyzer.scan_files()
    analyzer.generate_report()
