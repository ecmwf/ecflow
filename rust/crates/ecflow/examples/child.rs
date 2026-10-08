// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! A job reporting its progress to the server through the child commands.
//!
//! The client is configured from the job's environment, as the server sets
//! it up: `ECF_HOST`, `ECF_PORT`, `ECF_NAME`, `ECF_PASS`, `ECF_RID` and
//! `ECF_TRYNO`. The task needs `meter progress 0 100 50`, `label info ""`
//! and `event done`.
//!
//! ```sh
//! cargo run --example child
//! ```

use std::thread;
use std::time::Duration;

use ecflow::{Client, Error};

fn main() -> Result<(), Error> {
    let mut client = Client::new()?;
    client.child_init()?;
    match work(&mut client) {
        Ok(()) => client.child_complete(),
        Err(error) => {
            client.child_abort(&error.to_string())?;
            Err(error)
        }
    }
}

fn work(client: &mut Client) -> Result<(), Error> {
    for step in 0..=10 {
        thread::sleep(Duration::from_millis(200));
        client.child_meter("progress", step * 10)?;
    }
    client.child_label("info", "done")?;
    client.child_event("done", true)
}
