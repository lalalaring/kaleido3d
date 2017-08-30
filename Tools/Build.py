from CI.CMake import generate_cmake, build_cmake
import sys, os, platform

base_dir = os.path.split(os.path.abspath(__file__))[0]
src_dir = os.path.split(base_dir)[0]
plat_name = platform.system().lower()

bld_dir = os.path.join(src_dir, 'Build', plat_name)
inst_dir = os.path.join(src_dir, 'Build', 'Install')

if "darwin" == plat_name:
    generate_cmake('debug', src_dir, bld_dir, inst_dir, 'macos')
elif "windows" == plat_name:
    generate_cmake('debug', src_dir, bld_dir, inst_dir, 'windows')

build_cmake('debug', bld_dir)