// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Ping a server and print its version and statistics.
//!
//! ```sh
//! cargo run --example ping -- localhost 3141
//! ```

use ecflow::Client;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let host = args.next().unwrap_or_else(|| "localhost".to_string());
    let port = args.next().map_or(Ok(3141), |port| port.parse())?;

    let mut client = Client::with_host_port(&host, port)?;
    let round_trip = client.ping()?;
    println!(
        "server {} at {host}:{port}, ping {round_trip:?}",
        client.server_version()?
    );
    println!("{}", client.stats()?);
    Ok(())
}
