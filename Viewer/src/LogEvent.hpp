#ifndef LogEvent_H
#define LogEvent_H

class LogEvent;
class EventLister {
public:
  virtual void next(LogEvent*) = 0;
};


class EventSorter {
public: 
  virtual int compare(LogEvent*, LogEvent*) = 0;
};

// inline bool operator<(const DateTime& 
const QDateTime kSmallDate(QDate(1900, 1, 1), QTime());
const QDateTime kLargeDate(QDate(2100, 1, 1), QTime());

class counted 
{
public: 
  counted();
  void attach();
  void detach();
protected:
  ~counted();
private:
  counted(const counted&);
  counted& operator=(const counted&);
  int count_;
};

class LogEvent: public counted // , public observer
{
public:
  LogEvent(const QString&, const QDateTime&);
  const QDateTime& time() const { return time_; }
  virtual bool start() { return false; }
  virtual bool end() { return false; }
  virtual const QString& get_node() { return path_; }
  virtual QString status() { return "none"; }
  
  static int load(bool reset);
  static int scan(const QString&, EventLister&);
  static int sort(EventSorter&);
  static int find(const QString&);
  static int compare(const LogEvent*, const LogEvent*);
  virtual ~LogEvent();

protected:
  QDateTime time_;
  QString   path_;
private:
  LogEvent(const LogEvent&);
  LogEvent& operator=(const LogEvent&);
  // notification
  // adoption
  // gone
};
#endif
