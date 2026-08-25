#!/bin/bash
# ecflow's HPC build recipe for the nvidia toolchain, submitted as a SLURM job by
# build-on-hpc.
#
# THIS RECIPE RUNS NO TESTS, AND THAT IS THE WHOLE POINT OF ITS SHAPE.
#
# The legacy ci-hpc-config.yml's `nvidia-24.11` platform disables testing by
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

# Resources mirror the legacy CI rather than being sized here by hand.
# build-package-hpc's atos template (templates/macros.jinja, sbatch_atos) emits
# `--gres=ssdtmp:30G --mem=64GB --ntasks=<n> --cpus-per-task=<parallel//n>`, and
# .github/ci-hpc-config.yml sets `parallel: 64` for every ecflow platform against
# that tool's `ntasks` default of 1 -- so: one task, 64 CPUs, a flat 64 GB.
#
# This leg is why that matters. The earlier shape here asked for 8 tasks and
# named no --mem, taking SLURM's default: ECMWF's watch_cgroup killed the build
# (EC_MEMKILL, job 37369545) at 8785 MiB against an 8000 MiB cgroup SOFT limit
# -- the hard limit was 78 GB, so this was the watchdog, not the OOM killer.
# nvc++/nvcpfe carry roughly 1100 MiB per translation unit, several times what
# g++, icpx or clang need, which is why only this leg died.
#
# Building more narrowly would have "fixed" it while quietly diverging from the
# legacy CI, which compiles this same tree 64-way and does not hit the limit --
# because it asks for 64 GB. Asking for the memory is the fix.
#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Toolchain per releng/buildit/build.hpc.sh's load_nvidia*(), the authoritative
# list of what this cluster provides. Pinned, not left to prgenv's default: an
# unpinned prgenv/gnu silently selected the system gcc 8.5.0 against a GCC 15.2
# Boost and the link failed on a missing GLIBCXX_3.4.32 symbol.
module load prgenv/nvidia
module unload nvidia
module load nvidia/24.11
module load boost/1.87.0
module load ninja
module load python3/3.12.9-01
module load qt/6.6.1
module load cmake/new

cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=nvc \
  -DCMAKE_CXX_COMPILER=nvc++ \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DENABLE_CONFIG_MODE_BOOST=OFF \
  -DENABLE_STATIC_BOOST_LIBS=OFF \
  -DBoost_ROOT=/usr/local/apps/boost/1.87.0/NVIDIA/24.11 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.87.0/NVIDIA/24.11/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.12.9-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.12.9-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
# No ctest -- see the header.
cmake --install "${TMPDIR:-/tmp}/build"
