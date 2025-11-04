#!/usr/bin/env python3
"""
gcc_to_py.py - GCC-to-Python Compiler for Rubolt
Compiles C code into Python-compatible bytecode/calls
"""

import sys
import os
import subprocess
import re
import argparse
from pathlib import Path


class GCCToPythonCompiler:
    """Compiles C code to Python wrapper"""
    
    def __init__(self, input_file, output_file=None):
        self.input_file = Path(input_file)
        self.output_file = Path(output_file) if output_file else self.input_file.with_suffix('.py')
        self.functions = []
        self.includes = []
        
    def parse_c_file(self):
        """Parse C file and extract function signatures"""
        with open(self.input_file, 'r') as f:
            content = f.read()
        
        # Extract includes
        self.includes = re.findall(r'#include\s+[<"](.+?)[>"]', content)
        
        # Extract function signatures (simple regex)
        func_pattern = r'(\w+)\s+(\w+)\s*\(([^)]*)\)\s*{'
        matches = re.finditer(func_pattern, content)
        
        for match in matches:
            return_type = match.group(1)
            func_name = match.group(2)
            params = match.group(3).strip()
            
            # Parse parameters
            param_list = []
            if params and params != 'void':
                for param in params.split(','):
                    param = param.strip()
                    parts = param.split()
                    if len(parts) >= 2:
                        param_type = ' '.join(parts[:-1])
                        param_name = parts[-1].strip('*')
                        param_list.append((param_type, param_name))
            
            self.functions.append({
                'name': func_name,
                'return_type': return_type,
                'params': param_list
            })
    
    def compile_to_shared_lib(self):
        """Compile C to shared library using GCC"""
        so_file = self.input_file.with_suffix('.so')
        
        if sys.platform == 'win32':
            dll_file = self.input_file.with_suffix('.dll')
            cmd = ['gcc', '-shared', '-o', str(dll_file), str(self.input_file), '-fPIC']
            lib_file = dll_file
        else:
            cmd = ['gcc', '-shared', '-o', str(so_file), str(self.input_file), '-fPIC']
            lib_file = so_file
        
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"[gcc_to_py] Compiled to: {lib_file}")
            return lib_file
        except subprocess.CalledProcessError as e:
            print(f"[gcc_to_py] GCC compilation failed: {e.stderr.decode()}")
            return None
    
    def generate_python_wrapper(self, lib_file):
        """Generate Python wrapper using ctypes"""
        with open(self.output_file, 'w') as f:
            f.write('#!/usr/bin/env python3\n')
            f.write('"""Auto-generated Python wrapper for C library"""\n\n')
            f.write('import ctypes\n')
            f.write('import os\n\n')
            
            f.write(f'# Load shared library\n')
            f.write(f'_lib_path = os.path.join(os.path.dirname(__file__), "{lib_file.name}")\n')
            f.write('_lib = ctypes.CDLL(_lib_path)\n\n')
            
            # Type mapping
            type_map = {
                'int': 'ctypes.c_int',
                'float': 'ctypes.c_float',
                'double': 'ctypes.c_double',
                'char*': 'ctypes.c_char_p',
                'void*': 'ctypes.c_void_p',
                'void': 'None'
            }
            
            # Generate wrapper for each function
            for func in self.functions:
                fname = func['name']
                rtype = func['return_type']
                params = func['params']
                
                f.write(f'# Function: {fname}\n')
                f.write(f'_lib.{fname}.restype = {type_map.get(rtype, "ctypes.c_int")}\n')
                
                if params:
                    argtypes = [type_map.get(ptype, 'ctypes.c_void_p') for ptype, _ in params]
                    f.write(f'_lib.{fname}.argtypes = [{", ".join(argtypes)}]\n')
                
                f.write(f'\ndef {fname}(')
                if params:
                    param_names = [pname for _, pname in params]
                    f.write(', '.join(param_names))
                f.write('):\n')
                f.write(f'    """Wrapper for C function {fname}"""\n')
                
                if params:
                    call_args = ', '.join([pname for _, pname in params])
                    f.write(f'    return _lib.{fname}({call_args})\n')
                else:
                    f.write(f'    return _lib.{fname}()\n')
                
                f.write('\n')
            
            # Add __all__ export
            f.write('__all__ = [\n')
            for i, func in enumerate(self.functions):
                comma = ',' if i < len(self.functions) - 1 else ''
                f.write(f'    "{func["name"]}"{comma}\n')
            f.write(']\n')
        
        print(f"[gcc_to_py] Generated Python wrapper: {self.output_file}")
    
    def compile(self):
        """Full compilation pipeline"""
        print(f"[gcc_to_py] Compiling {self.input_file} -> {self.output_file}")
        
        self.parse_c_file()
        print(f"[gcc_to_py] Found {len(self.functions)} functions")
        
        lib_file = self.compile_to_shared_lib()
        if not lib_file:
            return False
        
        self.generate_python_wrapper(lib_file)
        return True


def main():
    parser = argparse.ArgumentParser(description='GCC-to-Python compiler for Rubolt')
    parser.add_argument('input', help='Input C file')
    parser.add_argument('-o', '--output', help='Output Python file')
    
    args = parser.parse_args()
    
    compiler = GCCToPythonCompiler(args.input, args.output)
    success = compiler.compile()
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
