// Copyright 2010 Anders Bakken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TEXTPAGERSECTION_P_HPP_
#define TEXTPAGERSECTION_P_HPP_

#include <QObject>
#include <QCoreApplication>
class TextPagerSection;
class TextSectionManager : public QObject
{
    Q_OBJECT
public:
    static TextSectionManager *instance() { static auto *inst = new TextSectionManager; return inst; }
Q_SIGNALS:
    void sectionFormatChanged(TextPagerSection *section);
    void sectionCursorChanged(TextPagerSection *section);
private:
    TextSectionManager() : QObject(QCoreApplication::instance()) {}
    friend class TextPagerSection;
};

#endif
