/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHHIGHLIGHTER_HPP_
#define VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHHIGHLIGHTER_HPP_

#include <QColor>

#include "TextPagerDocument.hpp"
#include "syntaxhighlighter.hpp"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    #include <QRegExp>
#else
    #include <QtCore5Compat/QRegExp>
#endif

class TextPagerSearchHighlighter : public SyntaxHighlighter {
public:
    explicit TextPagerSearchHighlighter(QObject* parent = nullptr);
    void highlightBlock(const QString& string) override;
    void reset(QString txt, TextPagerDocument::FindMode mode, bool apply);
    void reset(QRegExp rx, TextPagerDocument::FindMode mode, bool apply);
    void clear();
    enum Mode { NoMode, TextMode, RegexpMode };

protected:
    bool isWordCharacter(const QChar& ch) const;

    Mode mode_{NoMode};
    QRegExp rx_;
    QString text_;
    QTextCharFormat format_;
    bool caseSensitive_{false};
    bool wholeWords_{false};
    static QColor bgColour_;
};

#endif /* VIEWER_SRC_TEXTPAGER_TEXTPAGERSEARCHHIGHLIGHTER_HPP_ */
