"""Build script for the smbt Python extension.

Project metadata lives in pyproject.toml; this file only describes the C++
extension. The three implementations plus the vendored rsdic (BSD-3) and the
small support library are compiled straight from src/ — there are no external
dependencies. NDEBUG is required: rsdic uses assert(false) as an unreachable
marker, which would abort the interpreter if assertions were enabled.
"""

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

SOURCES = [
    "bindings/smbt_core.cpp",
    # vendored rsdic (only these three are used; *Test.cpp are not)
    "src/rsdic/lib/EnumCoder.cpp",
    "src/rsdic/lib/RSDic.cpp",
    "src/rsdic/lib/RSDicBuilder.cpp",
    # support library
    "src/lib/suc_array.cpp",
    "src/lib/bit_array.cpp",
    "src/lib/var_byte.cpp",
    # the three index implementations
    "src/mbt/multibit_tree.cpp",
    "src/smbt_vla/succinct_multibit_tree_vla.cpp",
    "src/smbt_trie/succinct_multibit_tree_trie.cpp",
]

ext_modules = [
    Pybind11Extension(
        "smbt._core",
        sources=SOURCES,
        include_dirs=["src"],
        define_macros=[("NDEBUG", "1")],
        extra_compile_args=["-O3"],
        cxx_std=14,
    ),
]

setup(ext_modules=ext_modules, cmdclass={"build_ext": build_ext})
