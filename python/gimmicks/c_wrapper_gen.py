#!/usr/bin/env python3
"""
c_wrapper_gen.py - C Wrapper Generator for Rubolt
Auto-generates Python wrappers for C libraries
Production version with header file and JSON parsing
"""

import argparse
import ctypes
import sys
import re
import json
from pathlib import Path
from typing import List, Dict, Optional


class HeaderParser:
    """Parse C header files to extract function signatures"""
    
    @staticmethod
    def parse_header(header_path: str) -> List[Dict]:
        """Parse C header file and extract function declarations"""
        functions = []
        
        with open(header_path, 'r') as f:
            content = f.read()
        
        # Remove comments
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        content = re.sub(r'//.*?\n', '\n', content)
        
        # Pattern for function declarations
        # Matches: return_type function_name(params);
        func_pattern = r'([\w\s\*]+)\s+(\w+)\s*\(([^)]*)\)\s*;'
        
        for match in re.finditer(func_pattern, content):
            return_type = match.group(1).strip()
            func_name = match.group(2)
            params_str = match.group(3).strip()
            
            # Skip common non-function patterns
            if return_type in ['typedef', 'struct', 'enum', 'union']:
                continue
            
            # Parse parameters
            params = []
            if params_str and params_str != 'void':
                for param in params_str.split(','):
                    param = param.strip()
                    if param:
                        # Extract type and name
                        parts = param.split()
                        if len(parts) >= 2:
                            param_type = ' '.join(parts[:-1])
                            param_name = parts[-1].strip('*')
                            params.append({
                                'name': param_name,
                                'type': param_type
                            })
            
            functions.append({
                'name': func_name,
                'return_type': return_type,
                'params': params
            })
        
        return functions
    
    @staticmethod
    def parse_json(json_path: str) -> List[Dict]:
        """Parse function definitions from JSON file"""
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        # Support both array and object with 'functions' key
        if isinstance(data, list):
            return data
        elif isinstance(data, dict) and 'functions' in data:
            return data['functions']
        else:
            raise ValueError("JSON must be array or object with 'functions' key")


class CWrapperGenerator:
    """Generates Python wrappers for C libraries"""
    
    def __init__(self, lib_path: str, output_file: str, functions: List[Dict]):
        self.lib_path = Path(lib_path)
        self.output_file = Path(output_file)
        self.functions = functions
    
    def generate(self):
        """Generate Python wrapper module"""
        with open(self.output_file, 'w') as f:
            self._write_header(f)
            self._write_library_loader(f)
            self._write_function_wrappers(f)
            self._write_exports(f)
        
        print(f"[wrapper_gen] Generated: {self.output_file}")
    
    def _write_header(self, f):
        f.write('#!/usr/bin/env python3\n')
        f.write(f'"""Auto-generated wrapper for {self.lib_path.name}"""\n\n')
        f.write('import ctypes\n')
        f.write('import os\n')
        f.write('from typing import Any, Optional\n\n')
    
    def _write_library_loader(self, f):
        f.write('# Load C library\n')
        f.write(f'_lib_name = "{self.lib_path.name}"\n')
        f.write('_lib_path = os.path.join(os.path.dirname(__file__), _lib_name)\n\n')
        f.write('try:\n')
        f.write('    _lib = ctypes.CDLL(_lib_path)\n')
        f.write('except OSError as e:\n')
        f.write('    raise ImportError(f"Failed to load library {_lib_path}: {e}")\n\n')
    
    def _write_function_wrappers(self, f):
        for func in self.functions:
            name = func['name']
            return_type = func.get('return_type', 'int')
            params = func.get('params', [])
            
            f.write(f'# Wrapper for: {name}\n')
            
            # Configure ctypes
            f.write(f'_lib.{name}.restype = ctypes.{self._get_ctype(return_type)}\n')
            
            if params:
                argtypes = ', '.join([f'ctypes.{self._get_ctype(p["type"])}' for p in params])
                f.write(f'_lib.{name}.argtypes = [{argtypes}]\n')
            
            # Generate wrapper function
            param_list = ', '.join([p['name'] for p in params])
            f.write(f'\ndef {name}({param_list}):\n')
            f.write(f'    """Call C function {name}"""\n')
            f.write(f'    return _lib.{name}({param_list})\n\n')
    
    def _write_exports(self, f):
        f.write('# Exported functions\n')
        f.write('__all__ = [\n')
        for i, func in enumerate(self.functions):
            comma = ',' if i < len(self.functions) - 1 else ''
            f.write(f'    "{func["name"]}"{comma}\n')
        f.write(']\n')
    
    def _get_ctype(self, type_name: str) -> str:
        """Map C type to ctypes type"""
        type_map = {
            'void': 'None',
            'int': 'c_int',
            'long': 'c_long',
            'float': 'c_float',
            'double': 'c_double',
            'char*': 'c_char_p',
            'void*': 'c_void_p',
            'bool': 'c_bool',
        }
        return type_map.get(type_name, 'c_void_p')


def main():
    parser = argparse.ArgumentParser(
        description='Generate Python wrapper for C library',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # From header file
  python c_wrapper_gen.py mylib.so -o wrapper.py -H mylib.h
  
  # From JSON file
  python c_wrapper_gen.py mylib.so -o wrapper.py -f functions.json
  
  # Auto-discover headers
  python c_wrapper_gen.py mylib.so -o wrapper.py --auto-headers
        """
    )
    
    parser.add_argument('library', help='Path to shared library (.so/.dll)')
    parser.add_argument('-o', '--output', required=True, help='Output Python file')
    parser.add_argument('-f', '--functions', help='Function definitions (JSON file)')
    parser.add_argument('-H', '--header', help='C header file to parse')
    parser.add_argument('--auto-headers', action='store_true', 
                       help='Auto-discover headers in gimmicks/headers/')
    
    args = parser.parse_args()
    
    functions = []
    
    # Parse from header file
    if args.header:
        print(f"[wrapper_gen] Parsing header: {args.header}")
        functions = HeaderParser.parse_header(args.header)
        print(f"[wrapper_gen] Found {len(functions)} functions in header")
    
    # Parse from JSON file
    elif args.functions:
        print(f"[wrapper_gen] Loading functions from JSON: {args.functions}")
        functions = HeaderParser.parse_json(args.functions)
        print(f"[wrapper_gen] Loaded {len(functions)} functions")
    
    # Auto-discover headers
    elif args.auto_headers:
        headers_dir = Path(__file__).parent / 'headers'
        if headers_dir.exists():
            print(f"[wrapper_gen] Scanning headers in: {headers_dir}")
            for header_file in headers_dir.glob('*.h'):
                print(f"[wrapper_gen] Parsing: {header_file.name}")
                funcs = HeaderParser.parse_header(str(header_file))
                functions.extend(funcs)
            print(f"[wrapper_gen] Total functions found: {len(functions)}")
        else:
            print(f"[wrapper_gen] Headers directory not found: {headers_dir}")
            sys.exit(1)
    
    # Fallback to example
    else:
        print("[wrapper_gen] Warning: No header or JSON specified, using example functions")
        functions = [
            {'name': 'example_func', 'return_type': 'int', 'params': [
                {'name': 'x', 'type': 'int'},
                {'name': 'y', 'type': 'int'}
            ]}
        ]
    
    if not functions:
        print("[wrapper_gen] Error: No functions found to wrap")
        sys.exit(1)
    
    generator = CWrapperGenerator(args.library, args.output, functions)
    generator.generate()
    print(f"[wrapper_gen] Successfully generated {len(functions)} function wrappers")


if __name__ == '__main__':
    main()
