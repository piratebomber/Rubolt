#!/usr/bin/env python3
"""
op_dispatcher.py - Operation Dispatcher for Rubolt
Routes Rubolt operations to appropriate C/Python implementations
"""

import sys
from typing import Any, Callable, Dict, Optional
from .py_to_c_bridge import call_c_function
from .type_converter import py_to_c, c_to_py


class OperationDispatcher:
    """Dispatcher for Rubolt operations"""
    
    def __init__(self):
        self.c_ops: Dict[str, Dict] = {}
        self.py_ops: Dict[str, Callable] = {}
        self.default_backend = 'python'
    
    def register_c_operation(self, op_name: str, lib_path: str, func_name: str,
                            return_type: str = 'int', arg_types: Optional[list] = None):
        """Register a C-based operation"""
        self.c_ops[op_name] = {
            'lib': lib_path,
            'func': func_name,
            'return_type': return_type,
            'arg_types': arg_types or []
        }
    
    def register_py_operation(self, op_name: str, func: Callable):
        """Register a Python-based operation"""
        self.py_ops[op_name] = func
    
    def dispatch(self, op_name: str, *args, backend: Optional[str] = None) -> Any:
        """Dispatch operation to appropriate backend"""
        backend = backend or self.default_backend
        
        if backend == 'c' or (backend == 'auto' and op_name in self.c_ops):
            return self._dispatch_c(op_name, *args)
        elif backend == 'python' or (backend == 'auto' and op_name in self.py_ops):
            return self._dispatch_py(op_name, *args)
        else:
            raise ValueError(f"Operation not found: {op_name}")
    
    def _dispatch_c(self, op_name: str, *args) -> Any:
        """Dispatch to C implementation"""
        if op_name not in self.c_ops:
            raise ValueError(f"C operation not registered: {op_name}")
        
        op_info = self.c_ops[op_name]
        result = call_c_function(
            op_info['lib'],
            op_info['func'],
            list(args),
            op_info['return_type'],
            op_info['arg_types']
        )
        
        return result
    
    def _dispatch_py(self, op_name: str, *args) -> Any:
        """Dispatch to Python implementation"""
        if op_name not in self.py_ops:
            raise ValueError(f"Python operation not registered: {op_name}")
        
        return self.py_ops[op_name](*args)
    
    def has_operation(self, op_name: str, backend: Optional[str] = None) -> bool:
        """Check if operation is registered"""
        if backend == 'c':
            return op_name in self.c_ops
        elif backend == 'python':
            return op_name in self.py_ops
        else:
            return op_name in self.c_ops or op_name in self.py_ops


# Global dispatcher instance
_dispatcher = OperationDispatcher()


# Standard operations (Python implementations as fallback)
def op_add(a, b):
    return a + b


def op_sub(a, b):
    return a - b


def op_mul(a, b):
    return a * b


def op_div(a, b):
    if b == 0:
        raise ZeroDivisionError("Division by zero")
    return a / b


def op_mod(a, b):
    return a % b


def op_pow(a, b):
    return a ** b


def op_eq(a, b):
    return a == b


def op_ne(a, b):
    return a != b


def op_lt(a, b):
    return a < b


def op_le(a, b):
    return a <= b


def op_gt(a, b):
    return a > b


def op_ge(a, b):
    return a >= b


# Register standard operations
_dispatcher.register_py_operation('add', op_add)
_dispatcher.register_py_operation('sub', op_sub)
_dispatcher.register_py_operation('mul', op_mul)
_dispatcher.register_py_operation('div', op_div)
_dispatcher.register_py_operation('mod', op_mod)
_dispatcher.register_py_operation('pow', op_pow)
_dispatcher.register_py_operation('eq', op_eq)
_dispatcher.register_py_operation('ne', op_ne)
_dispatcher.register_py_operation('lt', op_lt)
_dispatcher.register_py_operation('le', op_le)
_dispatcher.register_py_operation('gt', op_gt)
_dispatcher.register_py_operation('ge', op_ge)


# Convenience functions
def dispatch(op_name: str, *args, backend: Optional[str] = None) -> Any:
    """Dispatch operation"""
    return _dispatcher.dispatch(op_name, *args, backend=backend)


def register_c_op(op_name: str, lib_path: str, func_name: str, 
                  return_type: str = 'int', arg_types: Optional[list] = None):
    """Register C operation"""
    _dispatcher.register_c_operation(op_name, lib_path, func_name, return_type, arg_types)


def register_py_op(op_name: str, func: Callable):
    """Register Python operation"""
    _dispatcher.register_py_operation(op_name, func)


if __name__ == '__main__':
    # Test operations
    print("Operation Dispatcher Tests:")
    
    result = dispatch('add', 10, 20)
    print(f"add(10, 20) = {result}")
    
    result = dispatch('mul', 5, 6)
    print(f"mul(5, 6) = {result}")
    
    result = dispatch('eq', 42, 42)
    print(f"eq(42, 42) = {result}")
