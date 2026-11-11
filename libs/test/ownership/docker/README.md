# Privileged ownership tests

The tests of `s_ownership` check ownership with real accounts: the server runs under a dedicated account holding
`CAP_SETUID` and `CAP_SETGID`, and several users manage the Owners of its nodes. As this needs root and several
accounts, the tests only run in the image defined here:

- elsewhere, `s_ownership` reports itself skipped;
- in the image, recognised by its marker variable or its marker file, it is never skipped, and a missing
  prerequisite (account, tool, shared tree, build option) makes it fail.

The tests are labelled `privileged`, not `nightly`, and do not run on CI.

## Contents of the image

The image extends an ecFlow build image (by default `marcosbento/lumen:ubuntu-26.04`, set with the build argument
`BASE_IMAGE`) with:

| Item | Value |
|---|---|
| Server account | `ecflow`, running the servers |
| Server Owner | `ecfdefault` |
| Owners | `alice`, `bob`, `charlie` |
| Group of all the owners | `ecflow-owners` (`ecfdefault`, `alice`, `bob`, `charlie`) |
| Shared tree | `/srv/ecflow`, owned by `ecflow:ecflow-owners`, mode 3770 |
| Tools | `setpriv`, `setcap`, `setfacl` |
| Markers | `ECF_TEST_PRIVILEGED_IMAGE=1`, and the file `/etc/ecflow-privileged-test-image` |

The tests run as root. Each server is launched with `setpriv`, under the server account, with `CAP_SETUID` and
`CAP_SETGID` as ambient capabilities. Each server gets a private home, with password files generated for it:

- the custom password file (`ECF_CUSTOM_PASSWD`), with which the test users authenticate;
- the password file (`ECF_PASSWD`), with the same entries, as a remote plug forwards its user to the destination
  server as a regular user, not as a custom one.

## Building and running

Ownership can only be enabled in a build configured with `ENABLE_OWNERSHIP_UNDER_TEST=ON`, which is meant for
testing only. Run the commands below from the root of the ecFlow sources, with `ECBUILD` set to a checkout of
[ecbuild](https://github.com/ecmwf/ecbuild), which the image does not provide. The containers mount the sources at
`/workspace` and ecbuild at `/ecbuild`, so the paths given to CMake and CTest are container paths, here with the
build directory `/workspace/build-ownership`:

1. Build the image:

   ```bash
   docker build -t ecflow-ownership-test libs/test/ownership/docker
   ```

2. Configure and build the tested executables in the image:

   ```bash
   docker run --rm -v "$(pwd):/workspace" -v "$ECBUILD:/ecbuild:ro" ecflow-ownership-test bash -c \
     "cmake -S /workspace -B /workspace/build-ownership -Decbuild_DIR=/ecbuild/cmake \
        -DENABLE_OWNERSHIP_UNDER_TEST=ON && \
      cmake --build /workspace/build-ownership --target s_ownership ecflow_server ecflow_client"
   ```

3. Run the privileged tests:

   ```bash
   docker run --rm -v "$(pwd):/workspace" ecflow-ownership-test \
     ctest --test-dir /workspace/build-ownership -L privileged --output-on-failure
   ```

The default capabilities of a Docker container include those needed (`SETUID`, `SETGID`, `SETFCAP`): no option
such as `--privileged` is required.

The servers listen on ports reserved from 15000 (range 15000-15999), and use private homes under `/var/tmp`.
