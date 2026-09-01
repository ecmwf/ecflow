#!/bin/bash

#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

module load prgenv/gnu
module unload gcc
module load gcc/15.2.0
module load boost/1.90.0
module load ninja
module load python3/3.13.13-01
module load qt/6.6.1
module load cmake/new

cmake -S "$CI_SOURCE_DIR" -B "${TMPDIR:-/tmp}/build" \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$(command -v gcc)" \
  -DCMAKE_CXX_COMPILER="$(command -v g++)" \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DENABLE_ALL_TESTS=ON \
  -DBoost_ROOT=/usr/local/apps/boost/1.90.0/GNU/15.2 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.90.0/GNU/15.2/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.13.13-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.13.13-01/bin/python3 \
  -DECFLOW_PYEXT_TEST_LD_LIBRARY_PATH=/usr/local/apps/gcc/15.2.0/lib64 \
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
