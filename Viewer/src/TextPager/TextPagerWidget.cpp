//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

// abcde fghij klmno pqrst uvwxy z1234 56789 0!@#$ abcde fghij klmno pqrst uvwxy z1234 56789 0!@#$

//#include <QtWidgets>

#include <QWidget>
#include <QString>
#include <QFont>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QEvent>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>


#include "TextPagerWidget.hpp"



bool add = false;

TextPagerWidget::TextPagerWidget(QWidget *parent) :
   //TextPagerEdit(parent),
   doLineNumbers(true)
{
	QHBoxLayout* hb=new QHBoxLayout(this);
	hb->setContentsMargins(0,0,0,0);
	hb->setSpacing(0);

	textEditor_=new TextPagerEdit(this);
	lineNumArea_ = new TextPagerLineNumberArea(textEditor_);
	textEditor_->setLineNumberArea(lineNumArea_);

	hb->addWidget(lineNumArea_);
	hb->addWidget(textEditor_,1);

	setAttribute(Qt::WA_MouseTracking);

	//findHighlight = 0;
	//changeSelectionTimer.start(1000, this);

	/*bool replay = false;
    bool readOnly = true;
    int chunkSize = -1;
    QTextCodec *codec = 0;
    QString fontFamily;
    int fontSize = 20;

    const QStringList list = QApplication::arguments().mid(1);
    QRegExp fontFamilyRegexp("--font=(.+)");
    QRegExp fontSizeRegexp("--font-size=([0-9]+)");

    TextPagerDocument::DeviceMode mode = TextPagerDocument::Sparse;
    //mode = TextDocument::LoadAll;

*/
     /*lse if (arg == "--log") {
                fileName.clear();
                appendTimer.start(10, this);
                readOnly = true;
                chunkSize = 1000;
        */



     /*else if (arg.startsWith("--codec=")) {
                codec = QTextCodec::codecForName(arg.mid(8).toLatin1());
                if (!codec) {
                    qWarning("Can't load codec called '%s'", qPrintable(arg.mid(8)));
                    exit(1);
                }
            }*/


   /*     setCursorWidth(10);
        setObjectName("primary");
        if (chunkSize != -1) {
            document()->setChunkSize(chunkSize);
        }

        //l->addWidget(otherEdit = new Editor);
        connect(this, SIGNAL(cursorCharacter(QChar)),
                this, SLOT(onCursorCharacterChanged(QChar)));*/

        /*connect(otherEdit, SIGNAL(cursorCharacter(QChar)),
                this, SLOT(onCursorCharacterChanged(QChar)));
        otherEdit->setReadOnly(true);
        otherEdit->hide();
        otherEdit->setLineBreaking(false);
        otherEdit->setObjectName("otherEdit");
        textEdit->setReadOnly(readOnly);*/

       /*
        findEdit = new QLineEdit;
        l->addWidget(findEdit);

        connect(findEdit, SIGNAL(textChanged(QString)), this, SLOT(onFindEditTextChanged(QString)));
        QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
        connect(shortcut, SIGNAL(activated()), findEdit, SLOT(show()));
        connect(shortcut, SIGNAL(activated()), findEdit, SLOT(setFocus()));

        shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), findEdit);
        connect(shortcut, SIGNAL(activated()), findEdit, SLOT(clear()));
        connect(shortcut, SIGNAL(activated()), findEdit, SLOT(hide()));
        findEdit->hide();
*/
       /* if (!fontFamily.isEmpty()) {
            QFontDatabase fdb;
            foreach(const QString &family, fdb.families()) {
                if (fdb.isFixedPitch(family)) {
                    fontFamily = family;
                    break;
                }
            }
        }
        QFont f(fontFamily, fontSize);*/

        //setFont(QFont());

        //textEdit->addSyntaxHighlighter(new Highlighter(textEdit));
        //textEdit->addSyntaxHighlighter(new BlockLight(textEdit));
        //textEdit->addSyntaxHighlighter(new SpellCheck(textEdit));
#ifndef QT_NO_DEBUG_STREAM
//        if (codec) {
//            qDebug() << "using codec" << codec->name();
 //       }
#endif
 //       if (appendTimer.isActive())
 //           document()->setOption(TextPagerDocument::SwapChunks, true);

        //if (!fileName.isEmpty() && !textEdit->load(fileName, mode, codec)) {
#ifndef QT_NO_DEBUG_STREAM
        //    qDebug() << "Can't load" << fileName;
#endif
       // }
#if 0
        textEdit->document()->setText("This is a test");
//        textEdit->setSyntaxHighlighter(new Yellow(textEdit));
        QTextCharFormat format;
        format.setBackground(Qt::red);
        textEdit->document()->insertTextSection(0, 4, format)->setPriority(10000);
        format = QTextCharFormat();
        format.setBackground(Qt::blue);
        textEdit->insertTextSection(0, 7, format);
#endif
        //otherEdit->setDocument(textEdit->document());

        //lbl = new QLabel(w);
        //connect(textEdit, SIGNAL(cursorPositionChanged(int)),
        //        this, SLOT(onCursorPositionChanged(int)));
       // l->addWidget(lbl);

        /*new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), textEdit, SLOT(ensureCursorVisible()));
        new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this, SLOT(createTextSection()));
        new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_M), this, SLOT(markLine()));
        new QShortcut(QKeySequence(QKeySequence::Close), this, SLOT(close()));
        new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(changeTextSectionFormat()));*/

        //new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this, SLOT(save()));
        //new QShortcut(QKeySequence(Qt::ALT + Qt::Key_S), this, SLOT(saveAs()));


       /* QHBoxLayout *h = new QHBoxLayout;
        h->addWidget(box = new QSpinBox(centralWidget()));
        connect(textEdit->verticalScrollBar(), SIGNAL(valueChanged(int)),
                this, SLOT(onScrollBarValueChanged()));

        new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_G), this, SLOT(gotoPos()));

        box->setReadOnly(true);
        box->setRange(0, INT_MAX);
        l->addLayout(h);*/

     /*   connect(this, SIGNAL(sectionClicked(TextPagerSection *, QPoint)),
        		this, SLOT(onTextSectionClicked(TextPagerSection *, QPoint)));

        viewport()->setAutoFillBackground(true);
        connect(document(), SIGNAL(modificationChanged(bool)),
        		this, SLOT(onModificationChanged(bool)));*/
}

void TextPagerWidget::clear()
{
	textEditor_->document()->clear();
}

bool TextPagerWidget::load(const QString &fileName, TextPagerDocument::DeviceMode mode)
{
	return textEditor_->load(fileName, mode, NULL);
}

void TextPagerWidget::setFontProperty(VProperty* p)
{
	textEditor_->setFontProperty(p);
}

void TextPagerWidget::zoomIn()
{
	textEditor_->zoomIn();
}

void TextPagerWidget::zoomOut()
{
	textEditor_->zoomOut();
}

