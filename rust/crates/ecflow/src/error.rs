// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

//! Error type for ecFlow client operations.

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

impl Failure {
    /// The failure for an `ecf::ConnectionFailure` value.
    pub(crate) const fn from_code(code: i32) -> Self {
        match code {
            0 => Self::None,
            1 => Self::HostResolution,
            2 => Self::ConnectionRefused,
            3 => Self::Timeout,
            4 => Self::ClosedWithoutReply,
            5 => Self::HandshakeFailed,
            6 => Self::CertificateRejected,
            7 => Self::UndecodableReply,
            8 => Self::RejectedRequest,
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
}

impl Error {
    pub(crate) fn new(failure: Failure, message: impl Into<String>) -> Self {
        Self {
            failure,
            message: message.into(),
        }
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
}

impl From<ecflow_sys::Exception> for Error {
    fn from(e: ecflow_sys::Exception) -> Self {
        Self::new(Failure::None, e.what())
    }
}

/// Result type alias for ecFlow client operations.
pub type Result<T> = std::result::Result<T, Error>;
