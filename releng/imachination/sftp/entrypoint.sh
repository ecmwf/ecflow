#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Creates the SFTP accounts and the host key, then runs sshd in the foreground.

set -euo pipefail

workspace=${ECFLOW_WORKSPACE_DIR:-/workspace}
keys_dir=${SFTP_KEYS_DIR:-${workspace}/secrets}
users=${SFTP_USERS:?SFTP_USERS must list the accounts to create}

# Every account has the group of the workspace as its primary group, as the ecFlow server does, so that
# what one writes the other can use.
gid=$(stat -c %g "${workspace}")
group=$(getent group "${gid}" | cut -d: -f1 || true)
if [[ -z "${group}" ]]; then
    group=workspace
    groupadd --non-unique --gid "${gid}" "${group}"
fi

for user in ${users}; do
    if ! id "${user}" >/dev/null 2>&1; then
        useradd --no-create-home --home-dir "${workspace}" --shell /usr/sbin/nologin --gid "${gid}" "${user}"
        # An account without a password is locked, and sshd refuses a locked account even for a public key
        usermod --password '*' "${user}"
        echo "entrypoint: created account '${user}' (group ${group}, gid ${gid})"
    fi
    [[ -r "${keys_dir}/${user}/authorized_keys" ]] \
        || echo "entrypoint: warning: no ${keys_dir}/${user}/authorized_keys; '${user}' cannot log in yet" >&2
done

# The host key is taken from the workspace, so that it survives a restart of the Pod; without one, a
# temporary key is generated, which clients will see change at the next restart.
host_key=/etc/ssh/keys/ssh_host_ed25519_key
if [[ -r "${keys_dir}/sshd/ssh_host_ed25519_key" ]]; then
    install -m 0600 "${keys_dir}/sshd/ssh_host_ed25519_key" "${host_key}"
else
    echo "entrypoint: warning: no ${keys_dir}/sshd/ssh_host_ed25519_key; generating a temporary host key" >&2
    ssh-keygen -q -t ed25519 -N '' -f "${host_key}"
fi

exec /usr/sbin/sshd -D -e -f /etc/ssh/sshd_config
