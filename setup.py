from distutils.core import setup
from Cython.Build import cythonize

setup(
  name = 'ledky app',
  ext_modules = cythonize("ledky.pyx"),
)
