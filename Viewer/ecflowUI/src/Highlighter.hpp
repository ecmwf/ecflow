/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_Highlighter_HPP
#define ecflow_viewer_Highlighter_HPP

#include <string>

#include <QSyntaxHighlighter>
#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
    #include <QRegExp>
#endif
#include <QRegularExpression>

class Highlighter : public QSyntaxHighlighter {
public:
    Highlighter(QTextDocument* parent, QString id);
    static void init(const std::string& parFile);
    void toHtml(QString& html);

protected:
    void highlightBlock(const QString& text) override;
    void addRule(QString, QTextCharFormat);

private:
    void load(QString);

    struct HighlightingRule
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        QRegularExpression pattern;
#else
        QRegExp pattern;
#endif
        QTextCharFormat format;
    };

    QList<HighlightingRule> rules_;
    static std::string parFile_;
};

#endif /* ecflow_viewer_Highlighter_HPP */
