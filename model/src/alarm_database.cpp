#include "model/alarm_database.h"

// Very Quick & Dirty Class using SQLite to persist notifications

AlarmDatabase::AlarmDatabase(std::string uri) {
  int result;
  result = sqlite3_open_v2(uri.c_str(), &database,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  if (result != SQLITE_OK) {
    wxLogError("Alarm Database, Open error: (%d)\n", result);
  } else {
    wxLogMessage("Alarm Database Opened successfully\n");
  }
}

// BUG BUG Close the database during the destructor or elsewhere ??
AlarmDatabase::~AlarmDatabase(void) {
  int result;
  result = sqlite3_close(database);
  if (result != SQLITE_OK) {
    wxLogError("Alarm Database, Close error: %d\n", result);
  } else {
    printf("Alarm Database closed successfully\n");
  }
}

// Just informational, perhaps log it somewhere
DBError AlarmDatabase::GetVersion(std::string* version) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery = "SELECT sqlite_version()";

  result =
      sqlite3_prepare_v2(database, sqlQuery.c_str(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    for (;;) {
      result = sqlite3_step(sqlStatement);

      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
        *version = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 0)));
      } else {
        wxLogError("Error SQL Step %s (%d): %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    result = sqlite3_finalize(sqlStatement);
    if (result == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
    ;
  }
  return DB_NotFound;
}

// Add a single notification into the database
DBError AlarmDatabase::AddOne(const int& category, const int& severity,
                              const std::string& message) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery =
      "INSERT INTO Alarms (category, severity, message) VALUES (?,?,?)";

  // Generate the insert statement
  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // The first parameter; Category
    result = sqlite3_bind_int(sqlStatement, 1, category);
    if (result == SQLITE_OK) {
    } else {
      wxLogError("Error SQL Bind %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }

    // The second parameter; Severity
    result = sqlite3_bind_int(sqlStatement, 2, severity);
    if (result == SQLITE_OK) {
    } else {
      wxLogError("Error SQL Bind %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }

    // Third parameter; Message
    result = sqlite3_bind_text(sqlStatement, 3, message.data(), message.size(),
                               NULL);
    if (result == SQLITE_OK) {
    } else {
      wxLogError("Error SQL Bind, %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }

    // Execute the SQL Statement
    result = sqlite3_step(sqlStatement);
    if (result == SQLITE_DONE) {
    } else {
      wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }

    result = sqlite3_finalize(sqlStatement);
    if (result == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Finalize %s(%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Retrieve a single notification from the database
DBError AlarmDatabase::GetOne(int id, Alarm* alarm) {
  bool bFound = false;
  int result;
  std::string sqlQuery =
      "SELECT id, category, severity, message, activationTime FROM Alarms "
      "WHERE id = ?";
  sqlite3_stmt* sqlStatement = NULL;

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // Bind the first query parameter; id
    result = sqlite3_bind_int(sqlStatement, 1, id);
    if (result != SQLITE_OK) {
      wxLogError("Error SQL Bind %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
      return DB_NotFound;
    }
    // Execute the SQL Statement
    for (;;) {
      result = sqlite3_step(sqlStatement);
      if (result == SQLITE_DONE) {
        break;
      }
      // If we have a row of data
      else if (result == SQLITE_ROW) {
        bFound = true;

        alarm->id = sqlite3_column_int(sqlStatement, 0);
        alarm->category = sqlite3_column_int(sqlStatement, 1);
        alarm->severity = sqlite3_column_int(sqlStatement, 2);
        alarm->message = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 3)));
        alarm->activationTime = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 4)));

      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }

    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      if (bFound) {
        return DB_Success;
      }
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Retrieve all notifications from the database
DBError AlarmDatabase::GetAll(std::vector<Alarm>* alarms) {
  alarms->clear();
  Alarm alarm;
  int result;
  std::string sqlQuery =
      "SELECT id, category, severity, message, activationTime FROM Alarms "
      "ORDER BY id";
  sqlite3_stmt* sqlStatement = NULL;

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // Execute the SQL Statement
    for (;;) {
      result = sqlite3_step(sqlStatement);
      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
        alarm.id = sqlite3_column_int(sqlStatement, 0);
        alarm.category = sqlite3_column_int(sqlStatement, 1);
        alarm.severity = sqlite3_column_int(sqlStatement, 2);
        alarm.message = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 3)));
        alarm.activationTime = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 4)));
        alarms->push_back(alarm);
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Retrieve all notifications with a matching category from the database
DBError AlarmDatabase::GetByCategory(std::vector<Alarm>* alarms,
                                     const int& filter) {
  alarms->clear();
  Alarm alarm;
  int result;
  std::string sqlQuery =
      "SELECT id, category, severity, message, activationTime FROM Alarms "
      "WHERE category = ? ORDER BY id";
  sqlite3_stmt* sqlStatement = NULL;

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // Bind the first query parameter; category
    result = sqlite3_bind_int(sqlStatement, 1, filter);
    if (result != SQLITE_OK) {
      wxLogError("Error SQL Bind %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
      return DB_NotFound;
    }

    // Execute the SQL Statement
    for (;;) {
      result = sqlite3_step(sqlStatement);
      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
        alarm.id = sqlite3_column_int(sqlStatement, 0);
        alarm.category = sqlite3_column_int(sqlStatement, 1);
        alarm.severity = sqlite3_column_int(sqlStatement, 2);
        alarm.message = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 3)));
        alarm.activationTime = std::string(reinterpret_cast<const char*>(
            sqlite3_column_text(sqlStatement, 4)));
        alarms->push_back(alarm);
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Informational, retrieve the total number of notifications in the database
DBError AlarmDatabase::GetCount(int* count) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery = "SELECT COUNT(*) FROM Alarms";

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // Execute the SQL Statement
    for (;;) {
      result = sqlite3_step(sqlStatement);
      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
        *count = sqlite3_column_int(sqlStatement, 0);
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Delete a single notification from the database
DBError AlarmDatabase::DeleteOne(int id) {
  int result;
  std::string sqlQuery = "DELETE FROM Alarms WHERE id = ?";
  sqlite3_stmt* sqlStatement = NULL;

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    // Bind the first query parameter: id
    result = sqlite3_bind_int(sqlStatement, 1, id);

    if (result != SQLITE_OK) {
      wxLogError("Error SQL Bind %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
      return DB_NotFound;
    }

    for (;;) {
      result = sqlite3_step(sqlStatement);

      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

// Delete all notifications from the database
DBError AlarmDatabase::DeleteAll(void) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery = "DELETE FROM Alarms";

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    for (;;) {
      result = sqlite3_step(sqlStatement);

      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

DBError AlarmDatabase::DeleteDatabase(void) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery = "DROP TABLE Alarms";

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    for (;;) {
      result = sqlite3_step(sqlStatement);

      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
        ;
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

DBError AlarmDatabase::CreateDatabase(void) {
  int result;
  sqlite3_stmt* sqlStatement = NULL;
  std::string sqlQuery =
      "CREATE TABLE IF NOT EXISTS Alarms (id INTEGER PRIMARY KEY, category "
      "INTEGER NOT NULL, severity INTEGER NOT NULL, message TEXT, "
      "activationTime TEXT DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'NOW')) );";

  // Alternative Options for time stamps
  // timeStamp REAL DEFAULT (unixepoch('now','subsec')),
  // Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP - doesn't have millisecond
  // precision TIMESTAMP DATETIME DEFAULT(datetime('subsec')) - in later
  // versions eg 3.42

  result =
      sqlite3_prepare_v2(database, sqlQuery.data(), -1, &sqlStatement, NULL);
  if (sqlStatement != NULL) {
    for (;;) {
      result = sqlite3_step(sqlStatement);

      if (result == SQLITE_DONE) {
        break;
      } else if (result == SQLITE_ROW) {
      } else {
        wxLogError("Error SQL Step %s (%d) : %s\n", sqlQuery.c_str(), result,
                   sqlite3_errmsg(database));
        ;
      }
    }
    if (sqlite3_finalize(sqlStatement) == SQLITE_OK) {
      return DB_Success;
    } else {
      wxLogError("Error SQL Finalize %s (%d) : %s\n", sqlQuery.c_str(), result,
                 sqlite3_errmsg(database));
    }
  } else {
    wxLogError("Error SQL Prepare %s (%d) : %s\n", sqlQuery.c_str(), result,
               sqlite3_errmsg(database));
  }
  return DB_NotFound;
}

DBError AlarmDatabase::BackupDatabase(std::string backupFile) {
  int result;
  sqlite3_backup* pBackup;
  sqlite3* backupDatabase;

  result = sqlite3_open(backupFile.c_str(), &backupDatabase);
  if (result == SQLITE_OK) {
    pBackup = sqlite3_backup_init(backupDatabase, "main", database, "main");
    if (pBackup) {
      result = sqlite3_backup_step(pBackup, -1);
      if ((result == SQLITE_OK) || (result == SQLITE_DONE)) {
        result = sqlite3_backup_finish(pBackup);
        if (result == SQLITE_OK) {
          return DB_Success;
        }
      }
    }
  }
  return DB_NotFound;
}
