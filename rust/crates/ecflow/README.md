<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# ecflow

Safe Rust client for ECMWF's [ecFlow](https://github.com/ecmwf/ecflow)
workflow manager, wrapping the C++ `ClientInvoker` through
[`ecflow-sys`](https://crates.io/crates/ecflow-sys).

```rust
use ecflow::Client;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut client = Client::with_host_port("localhost", 3141)?;
    client.ping()?;
    println!("server {}", client.server_version()?);

    // A job reporting progress
    client.set_child_path("/suite/family/task");
    client.set_child_password(&std::env::var("ECF_PASS")?);
    client.set_child_pid(&std::process::id().to_string());
    client.set_child_try_no(1);
    client.child_init()?;
    client.child_meter("progress", 50)?;
    client.child_complete()?;

    // Any other command, as ecflow_client arguments
    client.invoke(["--suspend=/suite"])?;
    Ok(())
}
```

## Cargo build features

- `ssl` (default) - Build ecFlow with OpenSSL support.

## Copyright and License

Copyright 2009- European Centre for Medium-Range Weather Forecasts (ECMWF).

This software is licensed under the terms of the [Apache License, Version 2.0](LICENSE) which can also be obtained at http://www.apache.org/licenses/LICENSE-2.0.

In applying this licence, ECMWF does not waive the privileges and immunities granted to it by virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
