/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEXTCODECWRAPPER_HPP
#define TEXTCODECWRAPPER_HPP

#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringConverter>

class TextCodecWrapper {
public:
    explicit TextCodecWrapper(QStringConverter::Encoding e = QStringConverter::System)
        : value_{e} {}
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
    TextCodecWrapper(QTextCodec* c = nullptr)
        : value_{c} {}
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
