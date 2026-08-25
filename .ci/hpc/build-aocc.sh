#!/bin/bash
# ecflow's HPC build recipe for the aocc toolchain, submitted as a SLURM job by
# build-on-hpc.
#
# This recipe runs no tests: the legacy ci-hpc-config.yml's `aocc-4.0.0` platform
# is build-only (it sets `ctest_options: --version`, that config's way of saying
# "no tests"). Here the test invocation lives in the recipe, so not testing is
# simply the absence of a ctest line. The runner lane says the same thing by
# omitting `ctest = true` from [matrix.build] in .ci/manifest.toml.
#
# ci-infrastructure wraps this file (it unpacks the transferred source into
# node-local $TMPDIR and cds there, exports $CMAKE_PREFIX_PATH /
# $CI_INSTALL_PREFIX, appends the sentinel), so this script owns only its #SBATCH
# resources, module loads and the build/install -- and must NOT print
# "Finished: ..." itself.

# Resources mirror the legacy CI rather than being sized here by hand.
# build-package-hpc's atos template (templates/macros.jinja, sbatch_atos) emits
# `--gres=ssdtmp:30G --mem=64GB --ntasks=<n> --cpus-per-task=<parallel//n>`, and
# .github/ci-hpc-config.yml sets `parallel: 64` for every ecflow platform against
# that tool's `ntasks` default of 1 -- so: one task, 64 CPUs, a flat 64 GB.
#
# The 64 GB is the load-bearing part: SLURM's default is far below what a 64-way
# compile needs, and ECMWF's watch_cgroup kills the job rather than the OOM
# killer. See build-nvidia.sh, where nvc++ makes the margin tightest.
#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Toolchain per releng/buildit/build.hpc.sh's load_aocc*(), the authoritative
# list of what this cluster provides. Pinned, not left to prgenv's default, which
# resolves to the system compiler and mismatches the Boost build below.
module load prgenv/amd
module unload aocc
module load aocc/4.0.0
module load boost/1.81.0
module load ninja
module load python3/3.10.10-01
module load qt/6.2.0
module load cmake/new

# Boost and Python come from cluster modules rather than the stack-deps artifact:
# ecflow links Boost.Python against a specific interpreter, and the cluster ships
# the matched pair. pybind11 still comes from stack-deps via $CMAKE_PREFIX_PATH.
#
# ENABLE_STATIC_BOOST_LIBS and ENABLE_CONFIG_MODE_BOOST are deliberately NOT
# passed. Both default ON in CMakeLists.txt, and .github/ci-hpc-config.yml names
# neither in any platform block -- so, like the legacy HPC CI, this links Boost
# statically and finds it through BoostConfig.cmake. Forcing them OFF is right
# for the RUNNER lane only (see .github/actions/build-ecflow: ubuntu 24.04 ships
# CMake 3.28, below config mode's 3.30 floor, and only shared distro Boost).
# Here it breaks the nvidia leg: the NVIDIA/24.11 libboost_context.so defines no
# assembly entry points, so everything linking it fails with
#
#   libboost_context.so: undefined reference to `jump_fcontext'
#                                               `make_fcontext'
#                                               `ontop_fcontext'
#
# Only the static libboost_context.a carries those objects.
cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DBoost_ROOT=/usr/local/apps/boost/1.81.0/AMD/4.0 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.81.0/AMD/4.0/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.10.10-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.10.10-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
# No ctest -- see the header.
cmake --install "${TMPDIR:-/tmp}/build"
