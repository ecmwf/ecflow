#!/usr/bin/env bash

set -e
set -x

BASE_DIR="/workspace"
SOURCE_DIR="${BASE_DIR}/source"
ECFLOW_DIR="${SOURCE_DIR}/ecflow"
ECBUILD_DIR="${SOURCE_DIR}/ecbuild"

function configure() {

    # Ensure the source directories exist
    if [[ ! -d "${ECBUILD_DIR}" ]]; then
        echo "ecbuild source code directory expected at ${ECBUILD_DIR}, but does not exist!"
        exit 1
    fi
    if [[ ! -d "${ECFLOW_DIR}" ]]; then
        echo "ecflow source code directory expected at ${ECFLOW_DIR}, but does not exist!"
        exit 1
    fi

    pushd "${ECFLOW_DIR}"

    # Configure the project
    cmake --preset linux.gcc.serveronly.relwithdebinfo

    popd
}

function build() {
    pushd "${ECFLOW_DIR}"

    # Build the project
    cmake --build --preset linux.gcc.serveronly.relwithdebinfo --parallel $(nproc) --target all

    popd
}

function package() {
    pushd "${ECFLOW_DIR}"

    # Create the package
    cmake --build --preset linux.gcc.serveronly.relwithdebinfo --target package

    popd
}

function workflow() {
    configure
    build
    package
}

workflow
