#!/usr/bin/env python3
"""
py_to_c_bridge.py - Python-to-C Bridge for Rubolt
Handles Python→C function calls and type marshalling
"""

import ctypes
import sys
from typing import Any, List, Optional, Dict


class PyToCBridge:
    """Bridge for calling C functions from Python"""
    
    def __init__(self):
        self.loaded_libs: Dict[str, ctypes.CDLL] = {}
        self.function_cache: Dict[str, Any] = {}
    
    def load_library(self, lib_path: str) -> Optional[ctypes.CDLL]:
        """Load a shared library"""
        if lib_path in self.loaded_libs:
            return self.loaded_libs[lib_path]
        
        try:
            lib = ctypes.CDLL(lib_path)
            self.loaded_libs[lib_path] = lib
            return lib
        except OSError as e:
            print(f"[bridge] Failed to load library {lib_path}: {e}", file=sys.stderr)
            return None
    
    def get_function(self, lib_path: str, func_name: str) -> Optional[Any]:
        """Get function from library"""
        cache_key = f"{lib_path}::{func_name}"
        
        if cache_key in self.function_cache:
            return self.function_cache[cache_key]
        
        lib = self.load_library(lib_path)
        if not lib:
            return None
        
        try:
            func = getattr(lib, func_name)
            self.function_cache[cache_key] = func
            return func
        except AttributeError:
            print(f"[bridge] Function not found: {func_name}", file=sys.stderr)
            return None
    
    def call_c_function(self, lib_path: str, func_name: str, args: List[Any], 
                        return_type: str = 'int', arg_types: Optional[List[str]] = None) -> Any:
        """Call C function with automatic type marshalling"""
        func = self.get_function(lib_path, func_name)
        if not func:
            raise RuntimeError(f"Cannot call function {func_name}")
        
        # Set return type
        func.restype = self._map_type(return_type)
        
        # Set argument types if provided
        if arg_types:
            func.argtypes = [self._map_type(t) for t in arg_types]
        
        # Convert arguments
        c_args = [self._py_to_c(arg, arg_types[i] if arg_types else None) 
                  for i, arg in enumerate(args)]
        
        # Call function
        result = func(*c_args)
        
        # Convert result back to Python
        return self._c_to_py(result, return_type)
    
    def _map_type(self, type_name: str) -> Any:
        """Map type name to ctypes type"""
        type_map = {
            'void': None,
            'int': ctypes.c_int,
            'long': ctypes.c_long,
            'float': ctypes.c_float,
            'double': ctypes.c_double,
            'char': ctypes.c_char,
            'char*': ctypes.c_char_p,
            'string': ctypes.c_char_p,
            'void*': ctypes.c_void_p,
            'ptr': ctypes.c_void_p,
            'bool': ctypes.c_bool,
            'int8': ctypes.c_int8,
            'int16': ctypes.c_int16,
            'int32': ctypes.c_int32,
            'int64': ctypes.c_int64,
            'uint8': ctypes.c_uint8,
            'uint16': ctypes.c_uint16,
            'uint32': ctypes.c_uint32,
            'uint64': ctypes.c_uint64,
        }
        return type_map.get(type_name, ctypes.c_int)
    
    def _py_to_c(self, value: Any, type_hint: Optional[str] = None) -> Any:
        """Convert Python value to C value"""
        if type_hint == 'string' or type_hint == 'char*':
            if isinstance(value, str):
                return value.encode('utf-8')
        return value
    
    def _c_to_py(self, value: Any, type_hint: str) -> Any:
        """Convert C value to Python value"""
        if type_hint == 'string' or type_hint == 'char*':
            if isinstance(value, bytes):
                return value.decode('utf-8')
        return value


# Global bridge instance
_bridge = PyToCBridge()


def call_c_function(lib_path: str, func_name: str, args: List[Any], 
                    return_type: str = 'int', arg_types: Optional[List[str]] = None) -> Any:
    """Convenience function to call C function"""
    return _bridge.call_c_function(lib_path, func_name, args, return_type, arg_types)


def load_library(lib_path: str) -> Optional[ctypes.CDLL]:
    """Convenience function to load library"""
    return _bridge.load_library(lib_path)


if __name__ == '__main__':
    # Example usage
    print("PyToCBridge - Example usage:")
    print("  from py_to_c_bridge import call_c_function")
    print("  result = call_c_function('mylib.so', 'add', [1, 2], 'int', ['int', 'int'])")
