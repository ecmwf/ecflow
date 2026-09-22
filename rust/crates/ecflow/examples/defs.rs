// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Load a small suite into a server and print the server's definitions back.
//!
//! ```sh
//! cargo run --example defs -- localhost 3141
//! ```

use ecflow::{Client, DefsStyle};

const SUITE: &str = "\
suite rust_example
  task t1
    meter progress 0 100 50
    label info \"\"
    event done
endsuite
";

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let host = args.next().unwrap_or_else(|| "localhost".to_string());
    let port = args.next().map_or(Ok(3141), |port| port.parse())?;

    let mut client = Client::with_host_port(&host, port)?;
    client.load_defs_text(SUITE, true)?;
    println!("{}", client.get_defs_text(DefsStyle::Defs)?);

    client.invoke(["--suites"])?;
    println!("suites: {}", client.reply_strings().join(" "));
    Ok(())
}
