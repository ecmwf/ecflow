//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TEXTCODECWRAPPER_HPP
#define TEXTCODECWRAPPER_HPP

#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringConverter>

class TextCodecWrapper {
public:
    TextCodecWrapper(QStringConverter::Encoding e = QStringConverter::System) : value_{e} {}
    QStringConverter::Encoding value() const { return value_; }
    bool hasValue() const { return true; }
    static auto fromName(const QByteArray& codecName) {
        auto e = QStringConverter::encodingForName(codecName);
        return TextCodecWrapper(e.value_or(QStringConverter::System));
    }

protected:
    QStringConverter::Encoding value_{QStringConverter::System};
};
#else
    #include <QTextCodec>

class TextCodecWrapper {
public:
    TextCodecWrapper(QTextCodec* c = nullptr) : value_{c} {}
    QTextCodec* value() const { return value_; }
    bool hasValue() const { return value_ != nullptr; }
    static auto fromName(const QByteArray& codecName) {
        auto c = QTextCodec::codecForName(codecName);
        return TextCodecWrapper(c);
    }

protected:
    QTextCodec* value_{nullptr};
};
#endif

#endif // TEXTCODECWRAPPER_HPP
