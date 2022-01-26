import os
from os.path import join
import sys
from setuptools import setup

builddir = "build-Release"
sdkdir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(join(sdkdir, "api"))
import gmsh

setup(
      name='gmsh',
      description='gmsh from cmake',
    #   longdescription='gmsh.py from cmake build',
      version=gmsh.__version__,
      packages=[],
      data_files={
          join(sdkdir, "api", "gmsh.py"),
          join(sdkdir, builddir, f"libgmsh.so.{gmsh.__version__}")
      }
     )