#!/bin/bash

# Does not execute tests!

#SBATCH --qos=nf
#SBATCH --gres=ssdtmp:30G
#SBATCH --mem=64GB
#SBATCH --time=01:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

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
  -DBoost_ROOT=/usr/local/apps/boost/1.87.0/NVIDIA/24.11 \
  -DBoost_INCLUDE_DIR=/usr/local/apps/boost/1.87.0/NVIDIA/24.11/include \
  -DPython3_ROOT_DIR=/usr/local/apps/python3/3.12.9-01 \
  -DPython3_EXECUTABLE=/usr/local/apps/python3/3.12.9-01/bin/python3 \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
  -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
  -DCMAKE_INSTALL_PREFIX="$CI_INSTALL_PREFIX"
cmake --build "${TMPDIR:-/tmp}/build" --parallel "${SLURM_CPUS_PER_TASK:-64}"
cmake --install "${TMPDIR:-/tmp}/build"
