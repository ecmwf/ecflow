#!/bin/bash
# ecflow's HPC build recipe for the aocc toolchain, submitted as a SLURM job by
# build-on-hpc.
#
# THIS RECIPE RUNS NO TESTS, AND THAT IS THE WHOLE POINT OF ITS SHAPE.
#
# The legacy ci-hpc-config.yml's `aocc-4.0.0` platform disables testing by
# setting `ctest_options: --version` -- ctest is still invoked, but with a flag
# that makes it print its version and exit, because that config has nowhere to
# say "no tests". Here the test invocation lives in the recipe, so not testing is
# expressed by not writing a ctest line. Nothing to disable, no sentinel flag, and
# the reader sees build-then-install with no test between them.
#
# The runner lane expresses the same thing by omitting `ctest = true` from
# [matrix.build] in .ci/manifest.toml.
#
# ci-infrastructure wraps this file (it waits for the source transfer, unpacks
# into node-local $TMPDIR and cds there, exports $CMAKE_PREFIX_PATH /
# $CI_INSTALL_PREFIX, appends the sentinel), so this script owns only its #SBATCH
# resources, module loads and the build/install -- and must NOT print
# "Finished: ..." itself.

#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=8

# Toolchain per releng/buildit/build.hpc.sh's load_aocc*(), the authoritative
# list of what this cluster provides. Pinned, not left to prgenv's default: an
# unpinned prgenv/gnu silently selected the system gcc 8.5.0 against a GCC 15.2
# Boost and the link failed on a missing GLIBCXX_3.4.32 symbol.
module load prgenv/amd
module unload aocc
module load aocc/4.0.0
module load boost/1.81.0
module load ninja
module load python3/3.10.10-01
module load qt/6.2.0
module load cmake/new

cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DENABLE_CONFIG_MODE_BOOST=OFF \
  -DENABLE_STATIC_BOOST_LIBS=OFF \
  -DBoost_ROOT=/usr/local/apps/boost/1.81.0/AMD/4.0 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.81.0/AMD/4.0/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.10.10-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.10.10-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_NTASKS:-8}"
# No ctest -- see the header.
cmake --install "${TMPDIR:-/tmp}/build"
