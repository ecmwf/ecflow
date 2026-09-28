// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Confirmation of the commands `ecflow_client` asks about on the terminal.
//!
//! The client refuses such a command unless the question it asks was
//! approved. [`Confirming`] asks the caller instead, and approves on yes.

use std::fmt;

use ecflow_sys::Question;

use crate::client::Client;
use crate::error::Result;

/// A command that `ecflow_client` asks to confirm before running.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub enum Confirmation {
    /// Delete every suite.
    DeleteAll,
    /// Delete the nodes at these paths.
    DeleteNodes(Vec<String>),
    /// Halt the server.
    Halt,
    /// Shut the server down.
    Shutdown,
    /// Terminate the server process.
    Terminate,
}

impl Confirmation {
    /// What a question asks for; `None` for a command this crate does not
    /// know.
    fn of(question: &Question) -> Option<Self> {
        Some(match question.command.as_str() {
            "delete" if question.paths.is_empty() => Self::DeleteAll,
            "delete" => Self::DeleteNodes(question.paths.clone()),
            "halt" => Self::Halt,
            "shutdown" => Self::Shutdown,
            "terminate" => Self::Terminate,
            _ => return None,
        })
    }
}

impl fmt::Display for Confirmation {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::DeleteAll => write!(f, "Are you sure you want to delete all the suites?"),
            Self::DeleteNodes(paths) => write!(
                f,
                "Are you sure you want to delete the nodes at {}?",
                paths.join(", ")
            ),
            Self::Halt => write!(f, "Are you sure you want to halt the server?"),
            Self::Shutdown => write!(f, "Are you sure you want to shut down the server?"),
            Self::Terminate => write!(f, "Are you sure you want to terminate the server?"),
        }
    }
}

/// A view of a [`Client`] that asks before a command `ecflow_client` would
/// confirm on the terminal, and sends nothing when the answer is no.
///
/// Made with [`Client::confirming`]. Every method returns `Ok(None)` when the
/// command was declined. Words that already carry the `yes` `ecflow_client`
/// accepts are not asked about.
pub struct Confirming<'a, F> {
    client: &'a mut Client,
    confirm: F,
}

impl<'a, F: FnMut(&Confirmation) -> bool> Confirming<'a, F> {
    pub(crate) const fn new(client: &'a mut Client, confirm: F) -> Self {
        Self { client, confirm }
    }

    fn ask(&mut self, what: &Confirmation) -> bool {
        (self.confirm)(what)
    }

    /// Delete the nodes at the given paths. With `force`, even when they are
    /// active or submitted. Fails without paths: [`Confirming::delete_all`]
    /// deletes every suite.
    pub fn delete_nodes<I, S>(&mut self, paths: I, force: bool) -> Result<Option<()>>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let paths: Vec<String> = paths.into_iter().map(|p| p.as_ref().to_owned()).collect();
        if !paths.is_empty() && !self.ask(&Confirmation::DeleteNodes(paths.clone())) {
            return Ok(None);
        }
        self.client.delete_nodes(paths, force).map(Some)
    }

    /// Delete every suite. With `force`, even when nodes are active or
    /// submitted.
    pub fn delete_all(&mut self, force: bool) -> Result<Option<()>> {
        if !self.ask(&Confirmation::DeleteAll) {
            return Ok(None);
        }
        self.client.delete_all(force).map(Some)
    }

    /// Halt the server: no jobs are scheduled and child commands are blocked
    /// until a restart.
    pub fn halt_server(&mut self) -> Result<Option<()>> {
        if !self.ask(&Confirmation::Halt) {
            return Ok(None);
        }
        self.client.halt_server().map(Some)
    }

    /// Shut the server down: no jobs are scheduled, child commands still go
    /// through.
    pub fn shutdown_server(&mut self) -> Result<Option<()>> {
        if !self.ask(&Confirmation::Shutdown) {
            return Ok(None);
        }
        self.client.shutdown_server().map(Some)
    }

    /// Terminate the server process.
    pub fn terminate_server(&mut self) -> Result<Option<()>> {
        if !self.ask(&Confirmation::Terminate) {
            return Ok(None);
        }
        self.client.terminate_server().map(Some)
    }

    /// Run a command given as `ecflow_client` command line arguments, asking
    /// about every question it raises, and return the string reply, if the
    /// command has one. A `--group` asks for each member that would.
    pub fn invoke<I, S>(&mut self, args: I) -> Result<Option<String>>
    where
        I: IntoIterator<Item = S>,
        S: AsRef<str>,
    {
        let args: Vec<String> = args.into_iter().map(|a| a.as_ref().to_owned()).collect();
        let result = self.invoke_approving(&args);
        self.client.forget_approvals();
        result
    }

    /// Send the words; while they are refused on a question, ask, approve
    /// and send again.
    fn invoke_approving(&mut self, args: &[String]) -> Result<Option<String>> {
        loop {
            let error = match self.client.invoke_words(args) {
                Ok(reply) => return Ok(Some(reply)),
                Err(error) => error,
            };
            let Some(question) = self.client.refused_question() else {
                return Err(error);
            };
            let Some(what) = Confirmation::of(&question) else {
                return Err(error);
            };
            if !self.ask(&what) {
                return Ok(None);
            }
            self.client.approve(&question);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn question(command: &str, paths: &[&str]) -> Question {
        Question {
            command: command.to_owned(),
            paths: paths.iter().map(|p| (*p).to_owned()).collect(),
            approved: false,
        }
    }

    #[test]
    fn questions_name_what_they_ask_for() {
        assert_eq!(
            Confirmation::of(&question("delete", &["/s1", "/s2"])),
            Some(Confirmation::DeleteNodes(vec!["/s1".into(), "/s2".into()]))
        );
        assert_eq!(
            Confirmation::of(&question("delete", &[])),
            Some(Confirmation::DeleteAll)
        );
        assert_eq!(
            Confirmation::of(&question("halt", &[])),
            Some(Confirmation::Halt)
        );
        assert_eq!(
            Confirmation::of(&question("shutdown", &[])),
            Some(Confirmation::Shutdown)
        );
        assert_eq!(
            Confirmation::of(&question("terminate", &[])),
            Some(Confirmation::Terminate)
        );
        assert_eq!(Confirmation::of(&question("other", &[])), None);
    }
}
