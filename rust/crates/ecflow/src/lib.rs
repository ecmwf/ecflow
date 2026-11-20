// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Safe Rust client for ECMWF's ecFlow workflow manager.
//!
//! Provides:
//! - [`Client`] - connection configuration, server probes and control, node
//!   commands, child (task) commands and definitions as text
//! - [`Error`] - with the [`Failure`] class and any machine-readable server error
//!
//! ```no_run
//! use ecflow::Client;
//!
//! let mut client = Client::with_host_port("localhost", 3141)?;
//! client.ping()?;
//! println!("server {}", client.server_version()?);
//! # Ok::<(), ecflow::Error>(())
//! ```

pub mod client;
pub mod error;

pub use client::{CheckPtMode, Client, DefsStyle, NodeOrder, NodeState, ssl_supported, version};
pub use ecflow_sys::Zombie;
pub use error::{
    EMPTY_CONTENT, Error, FILE_UNAVAILABLE, Failure, GroupErrorEntry, Result, UNCLASSIFIED,
};
