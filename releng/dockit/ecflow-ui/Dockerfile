# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

FROM debian:13.7-slim

# Provision placeholders for the Mesa drivers
# Qt GUI links the vendor-neutral GL dispatch libraries (libglvnd), which Debian makes depend on the Mesa
# drivers. ecFlowUI does not render with OpenGL over X11, so empty packages stand in for the drivers, which
# would otherwise bring about 180 MB into the image (LLVM, among others). The epoch of their version satisfies
# any versioned dependency, and keeps apt from replacing them. Qt is told not to look for a GL integration
# (QT_XCB_GL_INTEGRATION, below).
RUN for package in libegl-mesa0 libglx-mesa0 libgbm1; do \
        mkdir -p "/tmp/placeholders/${package}/DEBIAN" \
        && printf 'Package: %s\nVersion: 99:0~ecflow\nArchitecture: all\nMaintainer: software@ecmwf.int\nDescription: Placeholder for %s, unused by ecFlowUI\n' \
            "${package}" "${package}" > "/tmp/placeholders/${package}/DEBIAN/control" \
        && dpkg-deb --build "/tmp/placeholders/${package}" "/tmp/placeholders/${package}.deb" > /dev/null \
        && dpkg -i "/tmp/placeholders/${package}.deb" \
        || exit 1; \
    done \
    && rm -rf /tmp/placeholders

# Provision the runtime of ecFlowUI that the package does not declare
# Qt loads its platform and image format plugins at run time, so dpkg-shlibdeps cannot derive them from the
# executables: qt6-qpa-plugins provides the platform plugins (including xcb, used to display over X11), and
# qt6-svg-plugins the SVG icons. Without a font, Qt shows no text. procps provides pkill, which the ecflow_ui
# launcher script runs on exit. proxychains4 lets ecFlowUI reach the servers through a SOCKS proxy (ecflow_ui -pc4).
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        fonts-dejavu-core \
        procps \
        proxychains4 \
        qt6-qpa-plugins \
        qt6-svg-plugins \
        tini \
    && rm -rf /var/lib/apt/lists/*

# Provision ecFlow package
# This Dockerfile assumes that the ecFlow package for each target platform, built with ecFlowUI only
# (preset linux.gcc.ui.release), is available in the build context (i.e. this directory), named after the
# platform architecture: ecflow-amd64.deb, ecflow-arm64.deb.
# TARGETARCH, set by BuildKit, selects the package, so that a single multi-platform build installs the
# matching package on each platform.
# The package is bind-mounted, rather than copied, so that it is not kept in an image layer, and installed
# with apt-get, so that the runtime libraries it depends on are installed as well.
ARG TARGETARCH

RUN --mount=type=bind,source=ecflow-${TARGETARCH}.deb,target=/tmp/ecflow.deb \
    apt-get update \
    && apt-get install -y --no-install-recommends /tmp/ecflow.deb \
    && rm -rf /var/lib/apt/lists/*

# Check that the libraries needed by ecFlowUI, and by the Qt platform plugin it displays with, are all found
RUN for executable in "$(command -v ecflow_ui.x)" /usr/lib/*-linux-gnu/qt6/plugins/platforms/libqxcb.so; do \
        missing="$(ldd "${executable}" | grep 'not found' || true)"; \
        if [ -n "${missing}" ]; then \
            echo "${executable} cannot be run, missing libraries:" >&2; \
            echo "${missing}" >&2; \
            exit 1; \
        fi; \
    done

ARG ECFLOW_LAUNCH="launch.sh"
COPY ${ECFLOW_LAUNCH} /opt/local/bin/${ECFLOW_LAUNCH}
RUN chmod +x /opt/local/bin/${ECFLOW_LAUNCH}

# Run as an unprivileged user
# ecFlowUI runs as the user 'ecflow'. The container starts as root, and launch.sh aligns the UID/GID of
# 'ecflow' with the owner of the configuration directory (typically bind-mounted from the host) before
# dropping privileges to it, so that the configuration is writable whatever UID owns it on the host.
ARG ECFLOW_UID=1000
ARG ECFLOW_GID=1000
RUN groupadd --non-unique --gid "${ECFLOW_GID}" ecflow \
    && useradd --create-home --uid "${ECFLOW_UID}" --gid "${ECFLOW_GID}" ecflow \
    && install -d -o ecflow -g ecflow /home/ecflow/.ecflow_ui_v5

# Qt depends on a UTF-8 locale, the X shared memory extension does not work across the container boundary,
# and no OpenGL driver is installed (see above)
ENV LANG=C.UTF-8 \
    QT_X11_NO_MITSHM=1 \
    QT_XCB_GL_INTEGRATION=none

WORKDIR /home/ecflow

# tini runs as PID 1, forwarding signals to launch.sh and reaping the processes started by ecFlowUI
ENTRYPOINT ["/usr/bin/tini", "--", "/opt/local/bin/launch.sh"]

# Any arguments given to the container are passed on to ecflow_ui; -log writes the log to standard output,
# so that it is available through `docker logs`
CMD ["-log"]
