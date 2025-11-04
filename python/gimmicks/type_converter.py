#!/usr/bin/env python3
"""
type_converter.py - Type Converter for Rubolt
Bidirectional type conversion between Python, C, and Rubolt types
"""

import ctypes
from typing import Any, Optional


class TypeConverter:
    """Convert between Python, C, and Rubolt types"""
    
    # Type mappings
    PY_TO_C_MAP = {
        int: ctypes.c_int64,
        float: ctypes.c_double,
        str: ctypes.c_char_p,
        bool: ctypes.c_bool,
        bytes: ctypes.c_char_p,
    }
    
    C_TO_PY_MAP = {
        'c_int': int,
        'c_int32': int,
        'c_int64': int,
        'c_long': int,
        'c_float': float,
        'c_double': float,
        'c_char_p': str,
        'c_bool': bool,
    }
    
    RUBOLT_TO_PY_MAP = {
        'int': int,
        'float': float,
        'str': str,
        'bool': bool,
        'list': list,
        'dict': dict,
        'null': type(None),
    }
    
    @staticmethod
    def py_to_c(value: Any, c_type: Optional[str] = None) -> Any:
        """Convert Python value to C value"""
        if value is None:
            return ctypes.c_void_p(0)
        
        py_type = type(value)
        
        # String conversion
        if py_type == str:
            return value.encode('utf-8')
        
        # Automatic type inference
        if c_type is None:
            if py_type in TypeConverter.PY_TO_C_MAP:
                c_class = TypeConverter.PY_TO_C_MAP[py_type]
                return c_class(value)
        else:
            # Explicit type conversion
            c_class = getattr(ctypes, c_type, None)
            if c_class:
                return c_class(value)
        
        return value
    
    @staticmethod
    def c_to_py(value: Any, c_type: str) -> Any:
        """Convert C value to Python value"""
        if value is None:
            return None
        
        # Handle c_char_p specially
        if c_type in ('c_char_p', 'char*') and isinstance(value, bytes):
            return value.decode('utf-8', errors='replace')
        
        # Use type map
        if c_type in TypeConverter.C_TO_PY_MAP:
            py_type = TypeConverter.C_TO_PY_MAP[c_type]
            return py_type(value)
        
        return value
    
    @staticmethod
    def rubolt_to_py(value: Any, rubolt_type: str) -> Any:
        """Convert Rubolt value to Python value"""
        if rubolt_type == 'null':
            return None
        
        if rubolt_type in TypeConverter.RUBOLT_TO_PY_MAP:
            py_type = TypeConverter.RUBOLT_TO_PY_MAP[rubolt_type]
            return py_type(value)
        
        return value
    
    @staticmethod
    def py_to_rubolt(value: Any) -> tuple:
        """Convert Python value to (value, rubolt_type) tuple"""
        if value is None:
            return (None, 'null')
        
        py_type = type(value)
        
        type_map = {
            int: 'int',
            float: 'float',
            str: 'str',
            bool: 'bool',
            list: 'list',
            dict: 'dict',
        }
        
        rubolt_type = type_map.get(py_type, 'object')
        return (value, rubolt_type)
    
    @staticmethod
    def infer_c_type(value: Any) -> str:
        """Infer C type from Python value"""
        py_type = type(value)
        
        type_map = {
            int: 'c_int64',
            float: 'c_double',
            str: 'c_char_p',
            bool: 'c_bool',
            bytes: 'c_char_p',
        }
        
        return type_map.get(py_type, 'c_void_p')


# Convenience functions
def py_to_c(value: Any, c_type: Optional[str] = None) -> Any:
    """Convert Python to C"""
    return TypeConverter.py_to_c(value, c_type)


def c_to_py(value: Any, c_type: str) -> Any:
    """Convert C to Python"""
    return TypeConverter.c_to_py(value, c_type)


def rubolt_to_py(value: Any, rubolt_type: str) -> Any:
    """Convert Rubolt to Python"""
    return TypeConverter.rubolt_to_py(value, rubolt_type)


def py_to_rubolt(value: Any) -> tuple:
    """Convert Python to Rubolt"""
    return TypeConverter.py_to_rubolt(value)


if __name__ == '__main__':
    # Test conversions
    print("Type Converter Tests:")
    
    # Python to C
    c_val = py_to_c(42)
    print(f"py_to_c(42) = {c_val} (type: {type(c_val)})")
    
    # C to Python
    py_val = c_to_py(c_val, 'c_int64')
    print(f"c_to_py(c_val, 'c_int64') = {py_val}")
    
    # Python to Rubolt
    rb_val, rb_type = py_to_rubolt("hello")
    print(f"py_to_rubolt('hello') = ({rb_val}, '{rb_type}')")
