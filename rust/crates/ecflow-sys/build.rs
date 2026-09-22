// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Build script for ecflow-sys
//!
//! Builds ecFlow the documented way (`cmake -B build -S .` with ecbuild on
//! `CMAKE_PREFIX_PATH`), then compiles the CXX bridge with the public include
//! directories and definitions of the `ecflow_all` target and links that
//! archive with its public libraries. The lists mirror `libs/CMakeLists.txt`.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

const ECFLOW_REPO: &str = "https://github.com/ecmwf/ecflow.git";
const ECBUILD_REPO: &str = "https://github.com/ecmwf/ecbuild.git";
const ECBUILD_TAG: &str = "3.13.1";

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=cpp");
    for var in [
        "ECBUILD_DIR",
        "BOOST_ROOT",
        "OPENSSL_ROOT_DIR",
        "CMAKE_PREFIX_PATH",
        "DOCS_RS",
    ] {
        println!("cargo:rerun-if-env-changed={var}");
    }

    if bindman_utils::is_docs_rs() {
        return;
    }

    let crate_dir =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set"));
    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR not set"));
    let src_dir = out_dir.join("src");
    let build_dir = out_dir.join("build");
    fs::create_dir_all(&src_dir).expect("Failed to create src directory");

    let ecbuild = resolve_ecbuild(&src_dir);
    let ecflow = resolve_ecflow_src(&src_dir);
    let boost = dependency_prefix("BOOST_ROOT", "boost");
    let openssl = dependency_prefix("OPENSSL_ROOT_DIR", "openssl@3");

    configure(
        &ecflow,
        &build_dir,
        &ecbuild,
        boost.as_deref(),
        openssl.as_deref(),
    );
    bindman_utils::run_command(
        Command::new("cmake").args([
            "--build",
            &build_dir.display().to_string(),
            "--parallel",
            &bindman_utils::build_parallelism(),
            "--target",
            "ecflow_all",
        ]),
        "cmake build ecflow",
    );

    // The bridge archive must precede the ecFlow archive on the link line.
    build_bridge(
        &crate_dir,
        &ecflow,
        &build_dir,
        boost.as_deref(),
        openssl.as_deref(),
    );
    link(&build_dir, boost.as_deref(), openssl.as_deref());

    bindman_build::check_cpp_api(
        &ecflow.join("libs/client/src"),
        &crate_dir.join("src/lib.rs"),
    );

    // Export for downstream crates
    println!("cargo:src={}", ecflow.display());
    println!("cargo:build_dir={}", build_dir.display());
    println!("cargo:cpp_dir={}", crate_dir.join("cpp").display());
}

/// `cmake -B build -S .` as the install documentation describes it.
fn configure(
    ecflow: &Path,
    build_dir: &Path,
    ecbuild: &Path,
    boost: Option<&Path>,
    openssl: Option<&Path>,
) {
    // ecbuild is found through CMAKE_PREFIX_PATH, the way CI provides it.
    let mut prefix_path = vec![ecbuild.display().to_string()];
    if let Ok(user) = env::var("CMAKE_PREFIX_PATH")
        && !user.is_empty()
    {
        prefix_path.push(user);
    }

    let mut cmd = Command::new("cmake");
    cmd.arg("-B")
        .arg(build_dir)
        .arg("-S")
        .arg(ecflow)
        .arg(format!("-DCMAKE_PREFIX_PATH={}", prefix_path.join(";")))
        .arg(format!(
            "-DCMAKE_BUILD_TYPE={}",
            bindman_utils::cmake_build_type()
        ))
        // Client only: no server, UI, Python module or tests.
        .arg("-DENABLE_SERVER=OFF")
        .arg("-DENABLE_UI=OFF")
        .arg("-DENABLE_PYTHON=OFF")
        .arg("-DENABLE_TESTS=OFF")
        .arg("-DENABLE_DOCS=OFF")
        .arg(format!(
            "-DENABLE_SSL={}",
            bindman_utils::on_off(cfg!(feature = "ssl"))
        ));
    if let Some(boost) = boost {
        cmd.arg(format!("-DBoost_ROOT={}", boost.display()));
    }
    if let Some(openssl) = openssl {
        cmd.arg(format!("-DOPENSSL_ROOT_DIR={}", openssl.display()));
    }

    bindman_utils::run_command(&mut cmd, "cmake configure ecflow");
}

/// Compile the CXX bridge against the public includes and definitions of
/// `ecflow_all`.
fn build_bridge(
    crate_dir: &Path,
    ecflow: &Path,
    build_dir: &Path,
    boost: Option<&Path>,
    openssl: Option<&Path>,
) {
    let mut build = cxx_build::bridge("src/lib.rs");
    build
        .file(crate_dir.join("cpp/ClientWrapper.cc"))
        .include(crate_dir.join("cpp"))
        .include(build_dir.join("generated/src"));
    for lib in [
        "attribute",
        "base",
        "client",
        "core",
        "node",
        "service",
        "server",
        "udp",
    ] {
        build.include(ecflow.join("libs").join(lib).join("src"));
    }
    for vendored in ["cereal", "json", "cpp-httplib"] {
        build.include(ecflow.join("3rdparty").join(vendored).join("include"));
    }
    if let Some(boost) = boost {
        build.include(boost.join("include"));
    }
    if let Some(openssl) = openssl {
        build.include(openssl.join("include"));
    }

    build
        .define("CMAKE", None)
        .define("BOOST_ASIO_NO_DEPRECATED", None)
        .define("ECF_HTTP_COMPRESSION", None);
    if cfg!(feature = "ssl") {
        build.define("ECF_OPENSSL", "1");
    }

    build
        .std("c++17")
        .flag_if_supported("-ftemplate-depth=1024")
        .flag_if_supported("-Wno-unused-parameter")
        .compile("ecflow_sys_bridge");
}

/// Link `ecflow_all` and its public libraries, in link order.
fn link(build_dir: &Path, boost: Option<&Path>, openssl: Option<&Path>) {
    let archive_dir = ["libs", "lib"]
        .iter()
        .map(|dir| build_dir.join(dir))
        .find(|dir| dir.join("libecflow_all.a").exists())
        .expect("the ecflow build produced no libecflow_all.a");
    println!("cargo:rustc-link-search=native={}", archive_dir.display());
    println!("cargo:rustc-link-lib=static=ecflow_all");

    // ecFlow links Boost statically by default; Boost.Process joined the list
    // with Boost 1.86 and brings Boost.Filesystem with it.
    let mut boost_libs = vec!["boost_program_options", "boost_date_time"];
    if let Some(boost) = boost {
        let lib_dir = bindman_utils::resolve_lib_dir(boost);
        println!("cargo:rustc-link-search=native={}", lib_dir.display());
        for lib in ["boost_process", "boost_filesystem"] {
            if lib_dir.join(format!("lib{lib}.a")).exists() {
                boost_libs.push(lib);
            }
        }
        for lib in &boost_libs {
            println!("cargo:rustc-link-lib=static={lib}");
        }
    } else {
        for lib in &boost_libs {
            println!("cargo:rustc-link-lib={lib}");
        }
    }

    if cfg!(feature = "ssl") {
        if let Some(openssl) = openssl {
            println!(
                "cargo:rustc-link-search=native={}",
                bindman_utils::resolve_lib_dir(openssl).display()
            );
        }
        println!("cargo:rustc-link-lib=ssl");
        println!("cargo:rustc-link-lib=crypto");
    }

    println!("cargo:rustc-link-lib=z");
    #[cfg(target_os = "linux")]
    println!("cargo:rustc-link-lib=crypt");
    bindman_utils::link_cpp_stdlib();
}

/// Locate ecbuild: `ECBUILD_DIR` when set, else a shallow clone of the
/// pinned tag, as the install documentation suggests.
fn resolve_ecbuild(src_dir: &Path) -> PathBuf {
    if let Ok(dir) = env::var("ECBUILD_DIR")
        && !dir.is_empty()
    {
        return PathBuf::from(dir);
    }
    bindman_utils::git_clone(ECBUILD_REPO, ECBUILD_TAG, &src_dir.join("ecbuild"))
}

/// Locate the ecFlow C++ sources: prefer the in-tree checkout when the crate
/// lives inside the ecFlow repository (path or git dependency), falling back
/// to cloning the release tag (packaged crates.io case).
fn resolve_ecflow_src(src_dir: &Path) -> PathBuf {
    const ECFLOW_TAG: &str = env!("CARGO_PKG_VERSION");

    let manifest_dir =
        PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set"));
    if let Some(root) = manifest_dir.ancestors().nth(3)
        && root.join("CMakeLists.txt").exists()
        && root
            .join("libs/client/src/ecflow/client/ClientInvoker.hpp")
            .exists()
    {
        eprintln!("ecflow-sys: building in-tree sources at {}", root.display());

        // Retrigger on C++ source edits.
        for path in ["CMakeLists.txt", "cmake", "libs", "3rdparty"] {
            println!("cargo:rerun-if-changed={}", root.join(path).display());
        }

        // Diverging is fine mid-development, but should never go unnoticed.
        let tree_version = project_version(&root.join("CMakeLists.txt")).unwrap_or_default();
        if tree_version != ECFLOW_TAG {
            println!(
                "cargo:warning=ecflow-sys {ECFLOW_TAG} is building in-tree ecflow {tree_version} (versions differ)"
            );
        }

        return root.to_path_buf();
    }
    bindman_utils::git_clone(ECFLOW_REPO, ECFLOW_TAG, &src_dir.join("ecflow"))
}

/// The version declared by the `project(...)` call of a `CMake` listfile.
fn project_version(cmakelists: &Path) -> Option<String> {
    let text = fs::read_to_string(cmakelists).ok()?;
    let line = text
        .lines()
        .map(str::trim_start)
        .find(|l| l.starts_with("project(") || l.starts_with("project ("))?;
    let mut tokens = line.split_whitespace().skip_while(|t| *t != "VERSION");
    tokens.next()?;
    tokens.next().map(|v| v.trim_end_matches(')').to_string())
}

/// The install prefix of a dependency: the environment variable when set,
/// else the Homebrew keg on macOS, else none (system paths).
fn dependency_prefix(var: &str, homebrew_name: &str) -> Option<PathBuf> {
    if let Ok(dir) = env::var(var)
        && !dir.is_empty()
    {
        return Some(PathBuf::from(dir));
    }
    if cfg!(target_os = "macos") {
        return ["/opt/homebrew/opt", "/usr/local/opt"]
            .iter()
            .map(|root| Path::new(root).join(homebrew_name))
            .find(|prefix| prefix.exists());
    }
    None
}
