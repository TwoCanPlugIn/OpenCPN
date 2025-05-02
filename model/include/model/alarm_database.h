#ifndef ALARM_DB_H
#define ALARM_DB_H

// STL
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>

// sqlite database
#include <sqlite3.h>

// wxWidgets
#include <wx/log.h>
#include <wx/fileconf.h>
#include <wx/string.h>

// Unused, but consider if using Litesync
typedef enum databaseType { single = 0, master = 1, slave = 2 } DatabaseType;

// SQLite error codes
typedef enum dBError {
  DB_Success = 0,
  DB_NotFound = 1,
  DB_InUse = 2,
  DB_Duplicate = 3
} DBError;

typedef enum categories {
  Navigation = 0,
  Distress = 1,
  Autopilot = 2,
  Depth = 3,
  Weather = 4,
  Engine = 5,
  Electrical = 6,
  Bilge = 7,
  Uncategorised = 99
} AlarmCategories;

// BUG BUG How to align with Core OpenCPN Notification Class.
typedef struct _alarm {
  int id;  // GUID or should we use an auto incrementing database column
  int category;
  int severity;
  std::string activationTime;
  std::string message;
} Alarm;

class AlarmDatabase {
public:
  AlarmDatabase(std::string uri);
  ~AlarmDatabase(void);
  DBError CreateDatabase(void);
  DBError DeleteDatabase(void);
  DBError BackupDatabase(std::string backupFile);
  DBError GetVersion(std::string* version);
  DBError GetCount(int* count);
  DBError GetOne(int notificationID, Alarm* alarm);
  DBError GetAll(std::vector<Alarm>* alarms);
  DBError GetByCategory(std::vector<Alarm>* alarms, const int& filter);
  DBError AddOne(const int& category, const int& severity,
                 const std::string& message);
  DBError DeleteOne(int notificationID);
  DBError DeleteAll(void);

protected:
private:
  sqlite3* database;
};

#endif
