// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Safe Rust client for ECMWF's ecFlow workflow manager.
//!
//! Provides:
//! - [`Client`] - connection configuration, server probes, child (task)
//!   commands, definitions as text, and [`Client::invoke`] for every other
//!   command
//! - [`Error`] - with the [`Failure`] class ecFlow diagnosed
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

pub use client::{Client, DefsStyle, ssl_supported, version};
pub use error::{Error, Failure, Result};
