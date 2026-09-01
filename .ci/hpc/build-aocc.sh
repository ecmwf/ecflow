#!/bin/bash
# ecflow's HPC build recipe for the aocc toolchain, submitted as a SLURM job by
# build-on-hpc.
#
# This recipe runs no tests!

#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

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
cmake --install "${TMPDIR:-/tmp}/build"

# The fetcher takes the artifact from CI_INSTALL_ARCHIVE, not from the install
# tree; .part + mv so it only ever appears complete.
mkdir -p "$(dirname "$CI_INSTALL_ARCHIVE")"
tar -czf "$CI_INSTALL_ARCHIVE.part" -C "$CI_INSTALL_PREFIX" .
mv "$CI_INSTALL_ARCHIVE.part" "$CI_INSTALL_ARCHIVE"
