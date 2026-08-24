#!/bin/bash
# ecflow's HPC build recipe for the gnu toolchain, submitted as a SLURM job by
# build-on-hpc. Module versions follow the legacy ci-hpc-config.yml's gnu-15.2.0
# block, which is the newest toolchain that config exercised.
#
# ci-infrastructure wraps this file (it waits for the source transfer, unpacks
# into node-local $TMPDIR and cds there, exports $CMAKE_PREFIX_PATH /
# $CI_INSTALL_PREFIX, appends the sentinel), so this script owns only its #SBATCH
# resources, module loads and the build/test/install — and must NOT print
# "Finished: ..." itself.

# atos (hpc2020) selects on QoS rather than partition; ssdtmp sizes the
# node-local SSD behind $TMPDIR. ecflow builds a large C++ tree plus its Python
# extension, hence the larger allocation.
#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=8

# Toolchain pinned to gcc 15.2.0, matching releng/buildit/build.hpc.sh's
# load_gcc15_2() and the legacy ci-hpc-config.yml's `gnu-15.2.0` platform.
#
# The `module unload gcc` before the pin, and CC/CXX below, are both load-bearing.
# Without them prgenv/gnu alone left CMake to its default search, which picked
# /usr/bin/g++ -- the compute node's SYSTEM gcc 8.5.0 -- while this recipe links
# the GNU/15.2 build of Boost hardcoded below. That mismatch failed the link with
#
#   libboost_program_options.so.1.90.0: undefined reference to
#     `std::ios_base_library_init()@GLIBCXX_3.4.32'
#
# because that symbol is GCC 13+ libstdc++ and 8.5 does not have it. The intel
# leg never hit this only because it passes CMAKE_CXX_COMPILER explicitly.
module load prgenv/gnu
module unload gcc
module load gcc/15.2.0
module load boost/1.90.0
module load ninja
module load python3/3.13.13-01
module load qt/6.6.1
module load cmake/new

# Boost and Python come from cluster modules rather than the stack-deps artifact:
# ecflow links Boost.Python against a specific interpreter, and the cluster ships
# the matched pair. pybind11 still comes from stack-deps via $CMAKE_PREFIX_PATH.
# ENABLE_CONFIG_MODE_BOOST=OFF selects FindBoost over boost-config.cmake, as the
# legacy config does.
cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$(command -v gcc)" \
  -DCMAKE_CXX_COMPILER="$(command -v g++)" \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DENABLE_CONFIG_MODE_BOOST=OFF \
  -DENABLE_STATIC_BOOST_LIBS=OFF \
  -DBoost_ROOT=/usr/local/apps/boost/1.90.0/GNU/15.2 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.90.0/GNU/15.2/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.13.13-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.13.13-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_NTASKS:-8}"
# Test selection carried over from the legacy config: the nightly label, minus
# the s_http suite.
ctest --test-dir "${TMPDIR:-/tmp}/build" --output-on-failure -L nightly -E 's_http'
cmake --install "${TMPDIR:-/tmp}/build"
