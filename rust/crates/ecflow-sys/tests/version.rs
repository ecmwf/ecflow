// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Guard against drift between the crate version and the version declared
//! by the repository's top-level `CMakeLists.txt` (cargo cannot read it
//! dynamically).

#[test]
fn crate_version_matches_cmake_project_version() {
    let path = concat!(env!("CARGO_MANIFEST_DIR"), "/../../../CMakeLists.txt");
    let Ok(text) = std::fs::read_to_string(path) else {
        // Not building from the repo checkout (e.g. a packaged crate).
        return;
    };
    let version = text
        .lines()
        .map(str::trim_start)
        .find(|l| l.starts_with("project(") || l.starts_with("project ("))
        .and_then(|l| {
            let mut tokens = l.split_whitespace().skip_while(|t| *t != "VERSION");
            tokens.next()?;
            tokens.next().map(|v| v.trim_end_matches(')'))
        })
        .expect("CMakeLists.txt declares a project version");
    assert_eq!(
        version,
        env!("CARGO_PKG_VERSION"),
        "rust/Cargo.toml workspace version and CMakeLists.txt project version are out of sync"
    );
}

#[test]
fn library_reports_its_version() {
    assert!(ecflow_sys::version().starts_with(env!("CARGO_PKG_VERSION")));
}
