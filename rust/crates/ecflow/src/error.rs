// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Error type for ecFlow client operations.

/// An unclassified server failure. Zero is still an error, not success.
pub const UNCLASSIFIED: i32 = 0;
/// The requested file was confirmed absent after applicable fallbacks.
pub const FILE_UNAVAILABLE: i32 = 1;
/// A manual file was found but had no usable content.
pub const EMPTY_CONTENT: i32 = 2;

/// A failed child of a grouped server reply, in reply order.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GroupErrorEntry {
    code: i32,
    message: String,
}

impl GroupErrorEntry {
    /// The raw numeric server code, including zero or an unknown future value.
    #[must_use]
    pub const fn code(&self) -> i32 {
        self.code
    }

    /// The unformatted server diagnostic.
    #[must_use]
    pub fn message(&self) -> &str {
        &self.message
    }
}

/// The class of failure ecFlow observed while talking to the server.
///
/// Mirrors `ecf::ConnectionFailure`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub enum Failure {
    /// No transport failure was observed: the server answered, refusing the
    /// request, or the error arose before a request was sent, such as an
    /// invalid definition or a missing setting.
    None,
    /// The host name could not be resolved.
    HostResolution,
    /// No listener accepted the connection.
    ConnectionRefused,
    /// The connection attempt received no answer, as when a firewall drops it.
    ConnectionUnanswered,
    /// The peer accepted the connection but did not reply in time.
    Timeout,
    /// The peer accepted the connection and then closed it without replying.
    ClosedWithoutReply,
    /// The TLS handshake did not complete.
    HandshakeFailed,
    /// The TLS handshake failed while verifying the peer certificate.
    CertificateRejected,
    /// A reply was received that the transport cannot decode.
    UndecodableReply,
    /// The peer answered, refusing the request.
    RejectedRequest,
    /// A failure that none of the other values describes.
    Other,
}

impl From<ecflow_sys::ConnectionFailure> for Failure {
    fn from(failure: ecflow_sys::ConnectionFailure) -> Self {
        use ecflow_sys::ConnectionFailure as Bridge;
        match failure {
            Bridge::None => Self::None,
            Bridge::HostResolution => Self::HostResolution,
            Bridge::ConnectionRefused => Self::ConnectionRefused,
            Bridge::ConnectionUnanswered => Self::ConnectionUnanswered,
            Bridge::Timeout => Self::Timeout,
            Bridge::ClosedWithoutReply => Self::ClosedWithoutReply,
            Bridge::HandshakeFailed => Self::HandshakeFailed,
            Bridge::CertificateRejected => Self::CertificateRejected,
            Bridge::UndecodableReply => Self::UndecodableReply,
            Bridge::RejectedRequest => Self::RejectedRequest,
            _ => Self::Other,
        }
    }
}

/// An error from the ecFlow client.
#[derive(Debug, Clone, thiserror::Error)]
#[error("{message}")]
pub struct Error {
    failure: Failure,
    message: String,
    code: Option<i32>,
    server_message: Option<String>,
    group_errors: Option<Vec<GroupErrorEntry>>,
}

impl Error {
    pub(crate) fn new(failure: Failure, message: impl Into<String>) -> Self {
        Self {
            failure,
            message: message.into(),
            code: None,
            server_message: None,
            group_errors: None,
        }
    }

    pub(crate) fn from_reply(
        failure: Failure,
        message: impl Into<String>,
        reply: ecflow_sys::ServerError,
    ) -> Self {
        let mut error = Self::new(failure, message);
        if reply.has_code {
            error.code = Some(reply.code);
            error.server_message = Some(reply.message);
        } else if reply.is_group {
            error.group_errors = Some(
                reply
                    .entries
                    .into_iter()
                    .map(|entry| GroupErrorEntry {
                        code: entry.code,
                        message: entry.message,
                    })
                    .collect(),
            );
        }
        error
    }

    /// The class of failure ecFlow observed.
    #[must_use]
    pub const fn failure(&self) -> Failure {
        self.failure
    }

    /// The message ecFlow reported.
    #[must_use]
    pub fn message(&self) -> &str {
        &self.message
    }

    /// The raw code of a single server failure, including zero or unknown values.
    /// `None` means no classified server reply (for example a local or transport failure).
    #[must_use]
    pub const fn code(&self) -> Option<i32> {
        self.code
    }

    /// The unformatted diagnostic of a single server failure.
    #[must_use]
    pub fn server_message(&self) -> Option<&str> {
        self.server_message.as_deref()
    }

    /// Ordered failed children of a group; `None` for a single or local failure.
    #[must_use]
    pub fn group_errors(&self) -> Option<&[GroupErrorEntry]> {
        self.group_errors.as_deref()
    }
}

impl From<ecflow_sys::Exception> for Error {
    fn from(e: ecflow_sys::Exception) -> Self {
        Self::new(Failure::None, e.what())
    }
}

/// Result type alias for ecFlow client operations.
pub type Result<T> = std::result::Result<T, Error>;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn preserves_zero_unknown_and_group_codes() {
        let reply = ecflow_sys::ServerError {
            has_code: true,
            code: UNCLASSIFIED,
            message: "old server".into(),
            is_group: false,
            entries: Vec::new(),
        };
        let error = Error::from_reply(Failure::None, "formatted", reply);
        assert_eq!(error.code(), Some(0));
        assert_eq!(error.server_message(), Some("old server"));
        assert_eq!(error.message(), "formatted");

        let reply = ecflow_sys::ServerError {
            has_code: false,
            code: 0,
            message: String::new(),
            is_group: true,
            entries: vec![
                ecflow_sys::ErrorEntry {
                    code: 9001,
                    message: "future".into(),
                },
                ecflow_sys::ErrorEntry {
                    code: UNCLASSIFIED,
                    message: "unclassified".into(),
                },
            ],
        };
        let error = Error::from_reply(Failure::None, "group", reply);
        let entries = error.group_errors().expect("ordered group errors");
        assert_eq!(entries[0].code(), 9001);
        assert_eq!(entries[0].message(), "future");
        assert_eq!(entries[1].code(), UNCLASSIFIED);
        assert_eq!(error.code(), None);

        let local = Error::new(Failure::ConnectionRefused, "cannot connect");
        assert_eq!(local.code(), None);
        assert_eq!(local.server_message(), None);
        assert_eq!(local.group_errors(), None);
    }
}
