#!/bin/bash
# ecflow's HPC build recipe for the intel toolchain, submitted as a SLURM job by
# build-on-hpc. Module versions follow the legacy ci-hpc-config.yml's
# intel-2025.3.1 block, the newest intel toolchain that config exercised.
#
# Differs from build-gnu.sh only in the prgenv module, the Boost prefix, and
# CMAKE_CXX_COMPILER=icpx.
#
# ci-infrastructure wraps this file (it waits for the source transfer, unpacks
# into node-local $TMPDIR and cds there, exports $CMAKE_PREFIX_PATH /
# $CI_INSTALL_PREFIX, appends the sentinel), so this script owns only its #SBATCH
# resources, module loads and the build/test/install — and must NOT print
# "Finished: ..." itself.

# Resources mirror the legacy CI rather than being sized here by hand.
# build-package-hpc's atos template (templates/macros.jinja, sbatch_atos) emits
# `--gres=ssdtmp:30G --mem=64GB --ntasks=<n> --cpus-per-task=<parallel//n>`, and
# .github/ci-hpc-config.yml sets `parallel: 64` for every ecflow platform against
# that tool's `ntasks` default of 1 -- so: one task, 64 CPUs, a flat 64 GB.
#
# The 64 GB is the load-bearing part. An earlier 8-task shape here named no
# --mem at all and took SLURM's default, which is what killed the nvidia leg;
# see build-nvidia.sh for the post-mortem.
#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Toolchain pinned to intel 2025.3.1, matching releng/buildit/build.hpc.sh's
# load_intel2025_3() and the legacy ci-hpc-config.yml's `intel-2025.3.1` platform.
#
# Note prgenv/intel-llvm, NOT prgenv/intel: the latter is what this recipe used to
# load, and it resolved to IntelLLVM 2021.4.0 while the Boost prefix hardcoded
# below is the INTEL/2025.3 build. The job still linked, so the mismatch was
# invisible -- but 2021.4.0 is a different row of the legacy test table
# (`-E '(py3_|s_http)'`, i.e. Python tests disabled too), so we were running a
# suite that toolchain is not expected to pass.
module load prgenv/intel-llvm
module unload intel
module load intel/2025.3.1
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
  -DCMAKE_C_COMPILER=icx \
  -DCMAKE_CXX_COMPILER=icpx \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DENABLE_CONFIG_MODE_BOOST=OFF \
  -DENABLE_STATIC_BOOST_LIBS=OFF \
  -DBoost_ROOT=/usr/local/apps/boost/1.90.0/INTEL/2025.3 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.90.0/INTEL/2025.3/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.13.13-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.13.13-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
# Test selection carried over from the legacy config: the nightly label, minus
# the s_http suite.
ctest --test-dir "${TMPDIR:-/tmp}/build" --output-on-failure -L nightly -E 's_http' -j 8
cmake --install "${TMPDIR:-/tmp}/build"
