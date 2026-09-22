// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Build script for ecflow-sys
//!
//! Builds ecFlow the documented way (`cmake -B build -S .` with ecbuild on
//! `CMAKE_PREFIX_PATH`), compiles the CXX bridge with the public include
//! directories and definitions of the `ecflow_all` target, and links that
//! archive with the libraries `CMake` found for it, read from `CMakeCache.txt`.

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
    for var in ["ECBUILD_DIR", "BOOST_ROOT", "CMAKE_PREFIX_PATH", "DOCS_RS"] {
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

    configure(&ecflow, &build_dir, &ecbuild);
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

    let cache = CMakeCache::read(&build_dir);

    // The bridge archive must precede the ecFlow archive on the link line.
    build_bridge(&crate_dir, &ecflow, &build_dir, &cache);
    link(&build_dir, &cache);

    bindman_build::check_cpp_api(
        &ecflow.join("libs/client/src"),
        &crate_dir.join("src/lib.rs"),
    );
}

/// `cmake -B build -S .` as the install documentation describes it.
fn configure(ecflow: &Path, build_dir: &Path, ecbuild: &Path) {
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
        .arg("-DENABLE_HTTP=OFF")
        .arg("-DENABLE_UDP=OFF")
        .arg("-DENABLE_UI=OFF")
        .arg("-DENABLE_PYTHON=OFF")
        .arg("-DENABLE_TESTS=OFF")
        .arg("-DENABLE_DOCS=OFF")
        // FindBoost module mode, as CI uses: it caches the include and library paths.
        .arg("-DENABLE_CONFIG_MODE_BOOST=OFF")
        .arg(format!(
            "-DENABLE_SSL={}",
            bindman_utils::on_off(cfg!(feature = "ssl"))
        ));
    // Passed as the CMake variable find_package reads; the environment
    // variable itself would only draw CMake's CMP0144 warning.
    if let Ok(boost) = env::var("BOOST_ROOT")
        && !boost.is_empty()
    {
        cmd.arg(format!("-DBoost_ROOT={boost}"));
        cmd.env_remove("BOOST_ROOT");
    }

    bindman_utils::run_command(&mut cmd, "cmake configure ecflow");
}

/// Compile the CXX bridge against the public includes and definitions of
/// `ecflow_all`.
fn build_bridge(crate_dir: &Path, ecflow: &Path, build_dir: &Path, cache: &CMakeCache) {
    let mut build = cxx_build::bridge("src/lib.rs");
    build
        .file(crate_dir.join("cpp/EcflowBridge.cc"))
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
    for var in ["Boost_INCLUDE_DIR", "OPENSSL_INCLUDE_DIR"] {
        if let Some(dir) = cache.path(var) {
            build.include(dir);
        }
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
        .warnings(false)
        .compile("ecflow_sys_bridge");
}

/// Link `ecflow_all` and the libraries `CMake` found for it, in link order.
fn link(build_dir: &Path, cache: &CMakeCache) {
    println!(
        "cargo:rustc-link-search=native={}",
        build_dir.join("libs").display()
    );
    println!("cargo:rustc-link-lib=static=ecflow_all");

    for library in cache.paths_matching("Boost_", "_LIBRARY_RELEASE") {
        link_library(&library);
    }
    for var in [
        "OPENSSL_SSL_LIBRARY",
        "OPENSSL_CRYPTO_LIBRARY",
        "ZLIB_LIBRARY_RELEASE",
        "Crypt_LIBRARIES",
    ] {
        if let Some(library) = cache.path(var) {
            link_library(&library);
        }
    }

    bindman_utils::link_cpp_stdlib();
}

/// Link a library given by its file path: static for an archive, dynamic
/// otherwise.
fn link_library(path: &Path) {
    let (Some(dir), Some(name)) = (path.parent(), path.file_name().and_then(|n| n.to_str())) else {
        return;
    };
    let stem = name.strip_prefix("lib").unwrap_or(name);
    let (kind, stem) = stem.strip_suffix(".a").map_or_else(
        || ("dylib", stem.split('.').next().unwrap_or(stem)),
        |stem| ("static", stem),
    );
    println!("cargo:rustc-link-search=native={}", dir.display());
    println!("cargo:rustc-link-lib={kind}={stem}");
}

/// The variables `CMake` cached while configuring ecFlow.
struct CMakeCache(String);

impl CMakeCache {
    fn read(build_dir: &Path) -> Self {
        Self(
            fs::read_to_string(build_dir.join("CMakeCache.txt"))
                .expect("CMake wrote no CMakeCache.txt"),
        )
    }

    /// The value of a path variable, when set and found.
    fn path(&self, name: &str) -> Option<PathBuf> {
        self.entries()
            .find(|(key, _)| *key == name)
            .map(|(_, value)| PathBuf::from(value))
    }

    /// The values of all found path variables whose names have the given
    /// prefix and suffix, in the order of the cache.
    fn paths_matching(&self, prefix: &str, suffix: &str) -> Vec<PathBuf> {
        self.entries()
            .filter(|(key, _)| key.starts_with(prefix) && key.ends_with(suffix))
            .map(|(_, value)| PathBuf::from(value))
            .collect()
    }

    /// `NAME=value` for every `NAME:TYPE=value` line with a found value.
    fn entries(&self) -> impl Iterator<Item = (&str, &str)> {
        self.0.lines().filter_map(|line| {
            let (key, value) = line.split_once('=')?;
            let (name, _) = key.split_once(':')?;
            (!value.is_empty() && !value.ends_with("-NOTFOUND")).then_some((name, value))
        })
    }
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
