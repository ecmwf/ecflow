#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Drives the `imachination` stack on a local Kubernetes cluster, through imachination.py, which holds every
# command; run `imachination.sh help` for the commands.

IMACHINATION_PROG="$(basename "$0")" exec python3 "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/imachination.py" "$@"
