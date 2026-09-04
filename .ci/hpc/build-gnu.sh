#!/bin/bash

#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

module load prgenv/gnu
# gcc/old, not a version number: it is the alias for the compiler a login node
# gives you with nothing loaded (8.5.0 today), which is what this cluster's GNU
# builds actually target. `module avail gcc` lists no 8.5.0 to pin directly.
module unload gcc
module load gcc/old
# This boost/python/qt set is the one releng/buildit/build.hpc.sh's load_gcc8()
# pairs with gcc 8.5 -- same versions load_gcc13_2() uses, bar the interpreter.
module load boost/1.84.0
module load ninja
module load python3/3.11.8-01
module load qt/6.6.1
module load cmake/new

# The boost module ships one build per compiler, under .../GNU/<major>.<minor>.
# Derive that from the gcc actually loaded rather than hardcoding it: a 13.2-built
# libboost_program_options.a linked against gcc 8.5 compiles all the way through
# and then fails on every executable with undefined references to
# std::__throw_bad_array_new_length -- a libstdc++ symbol that only exists from
# GCC 11 on. Fail here instead, where the message says what is wrong.
boost_version=1.84.0
boost_root="/usr/local/apps/boost/$boost_version/GNU/$(gcc -dumpfullversion | cut -d. -f1,2)"
[ -d "$boost_root" ] || {
  echo "no boost $boost_version build for gcc $(gcc -dumpfullversion) at $boost_root" >&2
  exit 1
}

cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$(command -v gcc)" \
  -DCMAKE_CXX_COMPILER="$(command -v g++)" \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DBoost_ROOT="$boost_root" \
  -DBoost_INCLUDE_DIR="$boost_root/include" \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.11.8-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.11.8-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
# s_test also covers s_test_using_http_backend; s_http covers s_http* (which the
# legacy ci-hpc-config.yml disables on HPC without recording why).
ctest --test-dir "${TMPDIR:-/tmp}/build" --output-on-failure -L nightly -E 's_test|s_zombies|s_http' -j 8
cmake --install "${TMPDIR:-/tmp}/build"

# The fetcher takes the artifact from CI_INSTALL_ARCHIVE, not from the install
# tree; .part + mv so it only ever appears complete.
mkdir -p "$(dirname "$CI_INSTALL_ARCHIVE")"
tar -cf - -C "$CI_INSTALL_PREFIX" . | zstd -T0 -q -o "$CI_INSTALL_ARCHIVE.part"
mv "$CI_INSTALL_ARCHIVE.part" "$CI_INSTALL_ARCHIVE"
