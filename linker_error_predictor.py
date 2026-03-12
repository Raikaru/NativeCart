#!/usr/bin/env python3
"""
Focused Linker Error Predictor
Finds symbols that are CALLED but never DEFINED
"""

import os
import re
import glob
from collections import defaultdict

class LinkerErrorPredictor:
    def __init__(self, base_path):
        self.base_path = base_path
        self.definitions = {}  # symbol -> [locations where defined]
        self.calls = defaultdict(list)  # symbol -> [locations where called]
        self.declarations = {}  # symbol -> [locations where declared]
        
        # Known external/system symbols
        self.external_symbols = set([
            # GBA BIOS functions
            'SoftReset', 'RegisterRamReset', 'VBlankIntrWait', 'Sqrt', 'ArcTan2',
            'CpuSet', 'CpuFastSet', 'BgAffineSet', 'ObjAffineSet', 'LZ77UnCompWram',
            'LZ77UnCompVram', 'RLUnCompWram', 'RLUnCompVram', 'MultiBoot', 'Div',
            # Standard library
            'abs', 'labs', 'min', 'max', 'memcpy', 'memset', 'strlen', 'strcpy', 
            'strcat', 'strcmp', 'strncmp', 'strchr', 'strstr', 'sprintf', 'snprintf',
            'malloc', 'free', 'calloc', 'realloc', 'printf', 'scanf', 'qsort',
            'rand', 'srand', 'pow', 'sqrt', 'sin', 'cos', 'tan', 'atan', 'atan2',
            'floor', 'ceil', 'round', 'exit', 'abort', 'memmove', 'memcmp',
            # String functions
            'StringCopy', 'StringAppend', 'StringLength', 'StringCompare',
            'StringCopyN', 'StringFill', 'StringGetEnd10', 'ConvertIntToDecimalStringN',
            'ConvertIntToHexStringN', 'StringExpandPlaceholders',
            # Known macros
            'T1_READ_8', 'T1_READ_16', 'T1_READ_32', 'T2_READ_8', 'T2_READ_16', 'T2_READ_32',
            'GET_BATTLER_SIDE2', 'GET_BATTLER_POSITION', 'IS_DOUBLE_BATTLE',
            'GET_MON_COORDS_HEIGHT', 'GET_MON_COORDS_WIDTH', 'ITEM_TO_BERRY',
            'min', 'max', 'abs', 'FALSE', 'TRUE', 'NULL', 'EOS',
        ])
    
    def scan_files(self):
        """Scan all source files"""
        c_files = glob.glob(os.path.join(self.base_path, "src_transformed/**/*.c"), recursive=True)
        
        print(f"Scanning {len(c_files)} C files...")
        
        for filepath in c_files:
            self.scan_c_file(filepath)
    
    def scan_c_file(self, filepath):
        """Scan a C file for definitions and calls"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
        except Exception as e:
            return
        
        in_multiline_comment = False
        in_string = False
        
        for line_num, line in enumerate(lines, 1):
            # Skip multiline comments
            if '/*' in line and '*/' not in line:
                in_multiline_comment = True
                continue
            if in_multiline_comment:
                if '*/' in line:
                    in_multiline_comment = False
                continue
            
            # Remove single-line comments
            line_clean = re.sub(r'//.*$', '', line)
            
            # Find function definitions (type name(params) {)
            func_def = re.match(
                r'^\s*(?:static\s+)?(?:EWRAM_DATA|IWRAM_DATA|COMMON_DATA)?\s*(\w+)\s+(\*?\w+)\s*\([^)]*\)\s*(?:\{|$)',
                line_clean
            )
            if func_def:
                func_name = func_def.group(2)
                if func_name not in self.definitions:
                    self.definitions[func_name] = []
                self.definitions[func_name].append({'file': filepath, 'line': line_num})
                continue
            
            # Find function declarations (extern type name(params);)
            func_decl = re.match(
                r'^\s*extern\s+\w+\s+(\*?\w+)\s*\([^)]*\)\s*;',
                line_clean
            )
            if func_decl:
                func_name = func_decl.group(1)
                if func_name not in self.declarations:
                    self.declarations[func_name] = []
                self.declarations[func_name].append({'file': filepath, 'line': line_num})
                continue
            
            # Find variable definitions with GBA sections
            var_def = re.match(
                r'^\s*(EWRAM_DATA|IWRAM_DATA|COMMON_DATA|EWRAM_BSS)\s+\w+\s+(\*?\w+)',
                line_clean
            )
            if var_def:
                var_name = var_def.group(2)
                if var_name not in self.definitions:
                    self.definitions[var_name] = []
                self.definitions[var_name].append({'file': filepath, 'line': line_num, 'type': 'variable'})
                continue
            
            # Find global const definitions
            const_def = re.match(
                r'^\s*const\s+\w+\s+(\*?\w+)',
                line_clean
            )
            if const_def:
                var_name = const_def.group(1)
                if var_name not in self.definitions:
                    self.definitions[var_name] = []
                self.definitions[var_name].append({'file': filepath, 'line': line_num, 'type': 'const'})
                continue
            
            # Find function calls (name(args))
            # Look for patterns like: FunctionName( or FunctionName (
            func_calls = re.findall(
                r'\b([a-zA-Z_]\w*)\s*\(',
                line_clean
            )
            for func_name in func_calls:
                # Skip keywords and types
                if func_name in ['if', 'while', 'for', 'switch', 'return', 'sizeof', 
                                  'typeof', 'offsetof', 'static', 'const', 'extern',
                                  'void', 'int', 'char', 'u8', 'u16', 'u32', 's8', 's16', 's32',
                                  'bool', 'bool8', 'struct', 'union', 'enum', 'typedef']:
                    continue
                # Skip likely macros (all caps)
                if re.match(r'^[A-Z][A-Z0-9_]+$', func_name):
                    continue
                self.calls[func_name].append({'file': filepath, 'line': line_num})
    
    def find_undefined_symbols(self):
        """Find symbols that are called but never defined"""
        undefined = []
        
        for symbol, call_sites in self.calls.items():
            # Skip external symbols
            if symbol in self.external_symbols:
                continue
            
            # Skip if defined
            if symbol in self.definitions:
                continue
            
            # Skip if it's a declaration-only header symbol
            if symbol in self.declarations and len(call_sites) == 0:
                continue
            
            # Check if it looks like a function call
            if call_sites:
                undefined.append({
                    'name': symbol,
                    'call_sites': call_sites[:5],  # First 5 call sites
                    'total_calls': len(call_sites)
                })
        
        return undefined
    
    def categorize_symbol(self, name):
        """Categorize by purpose"""
        name_lower = name.lower()
        
        if 'irq' in name_lower or 'intr' in name_lower or name_lower == 'intr_main':
            return 'INTERRUPT HANDLERS'
        elif any(x in name_lower for x in ['sound', 'm4a', 'music', 'cry', 'se_', 'bgm']):
            return 'SOUND/MUSIC'
        elif any(x in name_lower for x in ['flash', 'save', 'sector']):
            return 'SAVE/FLASH'
        elif 'timer' in name_lower:
            return 'TIMER'
        elif 'dma' in name_lower:
            return 'DMA'
        elif name.startswith('g') and name[1].isupper():
            return 'GLOBAL VARIABLES'
        elif 'battle' in name_lower:
            return 'BATTLE SYSTEM'
        elif 'script' in name_lower:
            return 'SCRIPT SYSTEM'
        elif 'sprite' in name_lower or 'oam' in name_lower:
            return 'SPRITE SYSTEM'
        elif 'bg' in name_lower:
            return 'BACKGROUND SYSTEM'
        elif any(x in name_lower for x in ['task', 'func', 'cb']):
            return 'TASKS/CALLBACKS'
        else:
            return 'OTHER FUNCTIONS'
    
    def generate_report(self):
        """Generate focused report"""
        undefined = self.find_undefined_symbols()
        
        # Categorize
        categorized = defaultdict(list)
        for sym in undefined:
            category = self.categorize_symbol(sym['name'])
            categorized[category].append(sym)
        
        print("\n" + "="*80)
        print("PREDICTED LINKER ERRORS - Pokemon FireRed GBA Decompilation")
        print("="*80)
        print(f"\nTotal unique symbols called: {len(self.calls)}")
        print(f"Total symbols defined: {len(self.definitions)}")
        print(f"POTENTIAL UNDEFINED REFERENCES: {len(undefined)}")
        
        # Priority categories
        priority_order = [
            'INTERRUPT HANDLERS',
            'SOUND/MUSIC', 
            'SAVE/FLASH',
            'TIMER',
            'DMA',
            'GLOBAL VARIABLES',
            'BATTLE SYSTEM',
            'SCRIPT SYSTEM',
            'SPRITE SYSTEM',
            'BACKGROUND SYSTEM',
            'TASKS/CALLBACKS',
            'OTHER FUNCTIONS'
        ]
        
        for category in priority_order:
            if category in categorized and categorized[category]:
                print(f"\n{'='*80}")
                print(f"{category} ({len(categorized[category])} symbols)")
                print(f"{'='*80}")
                
                # Sort by name
                for sym in sorted(categorized[category], key=lambda x: x['name']):
                    print(f"\n  [ERROR] {sym['name']}")
                    print(f"          Called {sym['total_calls']} time(s)")
                    print("          At:")
                    for site in sym['call_sites'][:3]:
                        short_file = site['file'].replace(self.base_path, '').lstrip('\\/')
                        print(f"            - {short_file}:{site['line']}")
                    if len(sym['call_sites']) > 3:
                        print(f"            ... and {len(sym['call_sites']) - 3} more")
        
        # Summary
        print(f"\n{'='*80}")
        print("SUMMARY BY CATEGORY")
        print(f"{'='*80}")
        for category in priority_order:
            count = len(categorized.get(category, []))
            if count > 0:
                print(f"  {category:25s}: {count:4d} missing definitions")
        
        # Save detailed report
        self.save_detailed_report(undefined)
    
    def save_detailed_report(self, undefined):
        """Save detailed JSON report"""
        report_path = os.path.join(self.base_path, "linker_errors_report.json")
        with open(report_path, 'w') as f:
            json.dump(undefined, f, indent=2)
        print(f"\nDetailed report saved to: {report_path}")

if __name__ == "__main__":
    base_path = "D:/Godot/pokefirered-master"
    predictor = LinkerErrorPredictor(base_path)
    predictor.scan_files()
    predictor.generate_report()
