// #define MAIN

#include <string.h>
#include <sstream>
#include <QFile>
#include <QString>
#include <QIODevice>
#include <QTextStream>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QVector>
#include <QRegExp>
#include <QStringList>
#include <QStringRef>
#include <QColor>
#include <QGraphicsScene>
#include <QWidget>
#include <QGraphicsTextItem>
/*

shell = new QProcess(this);
QStringList argv() << myfile.fileName();
shell.start("./src/unzip",argv); // or in win32 - src/unzip.exe
*/

#include <QTextStream>
// foreach(QString x, strings)    QTextStream(stdout) << x << endl;
#include "LogEvent.hpp"

const int boxSize = 3;
// namespace status {
const QVector<QString> status (QVector<QString>() << "unknown" 
			       << "suspended"
			       << "complete"
			       << "queued"
			       << "submitted"
			       << "active"
			       << "aborted"
			       << "shutdown"
			       << "halted"
			       << "event"
			       << "meter"
			       << "label"
			       );
// }
static EventSorter *sorter = 0x0;

counted::counted()
  : count_(0)
{}

counted::~counted()
{}

void counted::attach()
{ ++count_; }

void counted::detach()
{ if (--count_==0) delete this; }

static int compare(const void*a, const void*b) {
  LogEvent** pa = (LogEvent**)a;
  LogEvent** pb = (LogEvent**)b;
  return sorter->compare(*pa, *pb);
}

bool lessThan( const LogEvent* le1, const LogEvent *le2 )
{
  if (le1 && le2) return le1->time() < le2->time();
  return true;
}

class LogCache // : public QArray<LogEvent*> 
{
public: 
  QVector<LogEvent*> add;
  void reset() { int c = add.size(); while (c) { add[--c]->detach(); } add.clear(); }
  void sort() { qSort(add.begin(), add.end(), lessThan); }
  ~LogCache() {}
};

static LogCache cache;
static QString  cached;

LogEvent::LogEvent(const QString& path, const QDateTime& time)
  : time_(time)
  , path_(path)
{
  attach();
  cache.add.append(this);
  // observe
}

LogEvent::~LogEvent()
{}

int LogEvent::load(bool reset)
{
  if (reset) {
    cache.reset();
    cached = QString();
  }
  return 0;
}

int LogEvent::sort(EventSorter& s)
{
  sorter = &s;
  cache.sort();
  sorter = 0x0;
  return 0;
}

int LogEvent::scan(const QString& path, EventLister&l)
{
  int i = cache.add.size();
  while (i--) {
    if (cache.add[i]->path_.indexOf(path) != -1)
      l.next(cache.add[i]);
  }
  return 0;
}

int LogEvent::find(const QString&path) { return 0; }

/********************************/

class StatusEvent: public LogEvent {
  QString status_;
  virtual bool start() { return status_ == "submitted"; }
  virtual bool end() { return status_ == "complete"; }
  virtual const QString& text(QString&) { return path_;}
  virtual QString status() { return status_; }
public:
  StatusEvent(const QString&path, const QDateTime& time, const QString& status)
    : LogEvent(path, time)
    , status_ (status)
  {}
};

class EventEvent: public LogEvent {
  bool set_;
public:
  virtual QString status() { return "event"; }
  EventEvent(const QString&path, const QDateTime& time, bool b)
    : LogEvent(path, time)
    , set_ (b)
  {}  
};

class MeterEvent: public LogEvent {
  int step_;
public:
  virtual QString status() { return "meter"; }
  MeterEvent(const QString&path, const QDateTime& time, int step)
    : LogEvent(path, time)
    , step_ (step)
  {}  
};

/********************************/
QString logs = "/media/map/boxster/map/ecm/201502/ect/tmp/map/work/p4/metapps/suites/o/def";
QString logf1 = logs + "/timeline/ibis.900130.log";
QString logf2 = logs + "/tkinter/logs/vsms1.ecf.3.log";
typedef QMap<QString, QVector<LogEvent*> > log_map_type;
log_map_type log_map;
QDateTime dt_min(QDateTime::fromString("00:00:00 1.1.2101", "hh:mm:ss d.M.yyyy"));
QDateTime dt_max(QDateTime::fromString("00:00:00 1.1.2001", "hh:mm:ss d.M.yyyy"));

void reader(QString filename, 
	    bool onlyTask=true, 
	    int max=-1, 
	    int debug=0 ) {

  QFile inputFile(filename);
  log_map_type& rc = log_map;
  
  if (inputFile.open(QIODevice::ReadOnly)) {
   QTextStream in(&inputFile);
   QTextStream out (stdout);
   int num = 0;
   QString pkind (""), ppath("");
   while (!in.atEnd()) {
     QString line = in.readLine();
     int pos (line.indexOf("LOG:")), 
       sql (line.indexOf("[")),
       sqr (line.indexOf("]"));
     if (pos == -1 || sqr == -1) continue;
     QString time(line.left(sqr).mid(sql+1));
     QString log(line.mid(sqr+1).trimmed());
     QDateTime dt(QDateTime::fromString(time, "hh:mm:ss d.M.yyyy"));
     if (dt < dt_min) dt_min = dt;
     if (dt > dt_max) dt_max = dt;

     int ikd(log.indexOf(":")),
       isp(log.indexOf(" "));
     QString kind (log.left(ikd)),
       path(log.mid(ikd+1).trimmed());
     
     if (onlyTask && ppath.indexOf(path) == 0) continue;
     if (kind=="meter")
       rc[path].append(new MeterEvent(path, dt, 1));
     else if (kind=="event")
       rc[path].append(new EventEvent(path, dt, 1));
     else
       rc[path].append(new StatusEvent(path, dt, kind));

     pkind = kind;
     ppath = path.split(" ")[0];
     if (debug) {
       out << time << " " << dt.toString()
	 // << endl << log << endl << line << endl; 
	   << " " << kind << " " << path.trimmed() << endl;
       if (max > 0 && num > max) break;
       ++num;
     }
   }
   inputFile.close();
  }
  // return rc;
}

#ifdef MAIN_LOG
/*
make VERBOSE=1
*/
#include <QApplication>
#include "TimeItemWidget.hpp"

int drawScene(TimeItemWidget* qwd)
{
  QGraphicsScene* scene = new QGraphicsScene();
  QMap<QString, QColor> colors;
  colors.insert("submitted",  Qt::blue);
  colors["complete"] = Qt::yellow;
  colors["aborted"] = Qt::red;
  colors["active"] = Qt::green;
  colors["meter"] = Qt::blue;
  colors["event"] = Qt::black;
  colors["unknown"] = Qt::gray;
  // colors["unknown"] = Qt::grey;

  qwd->view()->setScene(scene);
  if (1) { //Populate the scene
    for(int x = 0; x < 8400; x = x + 25) {
        for(int y = 0; y < 100; y = y + 25) {
            if(x % 100 == 0 && y % 100 == 0) {
	      scene->addRect(x, y, 2, 2);
	      QString pointString;
	      QTextStream stream(&pointString);
	      if (0) stream << "(" << x << "," << y << ")";
	      QGraphicsTextItem* item = scene->addText(pointString);
	      item->setPos(x, y);
            } else {
	      scene->addRect(x, y, 1, 1);
            }
        }
    }
  }
  /* ./work/sms/qt4/qt-book/chap08/diagram
     /mnt/wdir/map/work/qt-5-4-1   */
  int num = 0, inc = 10;
  log_map_type::const_iterator mit = log_map.begin();
  QVector<LogEvent*>::const_iterator lit;
  while (mit != log_map.end()) {
    num += inc;
    QString line;
    QTextStream stream(&line);
    stream << mit.key();
    QGraphicsTextItem* item = scene->addText(line);
    item->setPos(-100, num);
    // scene->addText(mit.key());
    lit = mit.value().begin();
    while (lit != mit.value().end()) {
      int yy = ((*lit)->time().toTime_t() - 
		// kSmallDate.toTime_t()) / 10;
		dt_min.toTime_t()) / 10;

      /* QPixmap pm(5, 5);
      pm.fill();
      QPainter p(&pm);
      p.setRenderHint(QPainter::Antialiasing, true);
      p.setPen(colors[(*lit)->status()]);
      p.setBrush(QBrush(colors[(*lit)->status()])); */
      // QGraphicsEllipseItem *ell = p.drawEllipse(yy, num, 5, 5);

      stream << yy << "\n";
      QGraphicsEllipseItem *ell = scene->addEllipse(yy, num, 10, 10, 
						    QPen(colors[(*lit)->status()]),
						    QBrush(colors[(*lit)->status()]));
      ell->setPos(yy, num);

      ++lit; 
    } 
    ++mit; 
  }
}

int main(int argc, char* argv[])
{
  // Q_INIT_RESOURCE(images);
  reader(logf1, 1, 500, 1);
  reader(logf2, 1, 500, 1);

  QApplication app(argc, argv);
  TimeItemWidget window(0x0);
  window.setWindowTitle("TimeLine");
  window.show();
  log_map.clear();
  reader(logf2, 1);
  drawScene(&window);
  return app.exec();
}
#endif
