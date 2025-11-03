from setuptools import setup, Extension
import os

rubolt_module = Extension(
    'rubolt',
    sources=[
        'rubolt_module.c',
        '../src/lexer.c',
        '../src/parser.c',
        '../src/ast.c',
        '../src/interpreter.c'
    ],
    include_dirs=['../src'],
    extra_compile_args=['-std=c11'],
    libraries=['m'] if os.name != 'nt' else []
)

setup(
    name='rubolt',
    version='1.0.0',
    description='Rubolt programming language',
    ext_modules=[rubolt_module],
    author='Rubolt Team',
    author_email='team@rubolt.dev',
    url='https://github.com/rubolt/rubolt',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Programming Language :: Python :: 3',
        'Programming Language :: C',
    ],
    python_requires='>=3.6',
)
