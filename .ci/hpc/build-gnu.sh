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

# ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH is carried over verbatim from the legacy
# ci-hpc-config.yml's `gnu-15.2.0` block, and it is the option this leg was
# missing. The python tests dlopen ecflow.so into the cluster's python3, whose
# loader resolves libstdc++.so.6 to the system /lib64 copy -- gcc 8.5.0's, which
# stops at GLIBCXX_3.4.25 -- while ecflow.so is built by gcc 15.2. All 55 py3_*
# tests died before collecting a case:
#
#   ImportError: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.32' not found
#     (required by .../libs/pyext/python3/ecflow.so)
#
# The C++ tests are unaffected, which is why that run came back 25/80 green.
# ecflow reads this cache variable in libs/pyext/python3/CMakeLists.txt and
# prepends it to LD_LIBRARY_PATH in the python tests' ctest ENVIRONMENT. The
# path is hardcoded rather than derived, to stay the same string the legacy
# config uses; it tracks the gcc module pinned above.
#
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
  -DECFLOW_PYEXT_TEST_LD_LIBRARY_PATH=/usr/local/apps/gcc/15.2.0/lib64 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
# Test selection carried over from the legacy config: the nightly label, minus
# the s_http suite.
ctest --test-dir "${TMPDIR:-/tmp}/build" --output-on-failure -L nightly -E 's_http' -j 8
cmake --install "${TMPDIR:-/tmp}/build"
