#!/usr/bin/env python3
"""
ffi_helper.py - Foreign Function Interface Helper for Rubolt
Utilities for ctypes and cffi operations
"""

import ctypes
import sys
from typing import Any, Optional


class FFIHelper:
    """Helper for FFI operations"""
    
    @staticmethod
    def create_struct(name: str, fields: list) -> type:
        """Create a ctypes structure dynamically"""
        class DynamicStruct(ctypes.Structure):
            _fields_ = fields
        
        DynamicStruct.__name__ = name
        return DynamicStruct
    
    @staticmethod
    def create_callback(func, return_type: str, *arg_types):
        """Create C callback from Python function"""
        CFUNCTYPE = ctypes.CFUNCTYPE
        
        rtype = FFIHelper.map_type(return_type)
        atypes = [FFIHelper.map_type(t) for t in arg_types]
        
        CallbackType = CFUNCTYPE(rtype, *atypes)
        return CallbackType(func)
    
    @staticmethod
    def map_type(type_name: str) -> Any:
        """Map type name to ctypes type"""
        type_map = {
            'void': None,
            'int': ctypes.c_int,
            'long': ctypes.c_long,
            'float': ctypes.c_float,
            'double': ctypes.c_double,
            'char': ctypes.c_char,
            'char*': ctypes.c_char_p,
            'wchar*': ctypes.c_wchar_p,
            'void*': ctypes.c_void_p,
            'bool': ctypes.c_bool,
            'int8': ctypes.c_int8,
            'int16': ctypes.c_int16,
            'int32': ctypes.c_int32,
            'int64': ctypes.c_int64,
            'uint8': ctypes.c_uint8,
            'uint16': ctypes.c_uint16,
            'uint32': ctypes.c_uint32,
            'uint64': ctypes.c_uint64,
            'size_t': ctypes.c_size_t,
        }
        return type_map.get(type_name, ctypes.c_void_p)
    
    @staticmethod
    def pointer_to_array(ptr: ctypes.POINTER, length: int, ctype):
        """Convert pointer to array"""
        if not ptr:
            return []
        return [ptr[i] for i in range(length)]
    
    @staticmethod
    def string_to_c(s: str) -> bytes:
        """Convert Python string to C string"""
        return s.encode('utf-8')
    
    @staticmethod
    def c_to_string(c_str: bytes) -> str:
        """Convert C string to Python string"""
        if isinstance(c_str, bytes):
            return c_str.decode('utf-8')
        return str(c_str)
    
    @staticmethod
    def allocate_buffer(size: int) -> ctypes.Array:
        """Allocate a buffer"""
        return ctypes.create_string_buffer(size)
    
    @staticmethod
    def get_function_pointer(lib: ctypes.CDLL, func_name: str) -> Optional[Any]:
        """Get function pointer from library"""
        try:
            return getattr(lib, func_name)
        except AttributeError:
            return None


class MemoryManager:
    """Manage C memory allocations"""
    
    def __init__(self):
        self.allocations = []
    
    def allocate(self, size: int) -> ctypes.c_void_p:
        """Allocate memory"""
        ptr = ctypes.c_void_p(ctypes.pythonapi.malloc(size))
        self.allocations.append(ptr)
        return ptr
    
    def free(self, ptr: ctypes.c_void_p):
        """Free memory"""
        ctypes.pythonapi.free(ptr)
        if ptr in self.allocations:
            self.allocations.remove(ptr)
    
    def free_all(self):
        """Free all allocated memory"""
        for ptr in self.allocations:
            ctypes.pythonapi.free(ptr)
        self.allocations.clear()
    
    def __del__(self):
        self.free_all()


# Convenience functions
def create_callback(func, return_type: str, *arg_types):
    """Create C callback"""
    return FFIHelper.create_callback(func, return_type, *arg_types)


def create_struct(name: str, fields: list):
    """Create dynamic structure"""
    return FFIHelper.create_struct(name, fields)


if __name__ == '__main__':
    print("FFI Helper - Examples:")
    print("  callback = create_callback(my_func, 'int', 'int', 'int')")
    print("  MyStruct = create_struct('MyStruct', [('x', ctypes.c_int), ('y', ctypes.c_float)])")
