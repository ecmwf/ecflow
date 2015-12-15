#ifndef WEAKPOINTER_H
#define WEAKPOINTER_H

#include <qglobal.h>

#if (QT_VERSION < QT_VERSION_CHECK(4, 6, 0))
// while QWeakPointer existed in Qt 4.5 it lacked certain APIs (like
// data so I'll stick with QPointer until 4.6)
#include <QPointer>
#define WeakPointer QPointer
#else
#include <QWeakPointer>
#define WeakPointer QWeakPointer
#endif

#endif
