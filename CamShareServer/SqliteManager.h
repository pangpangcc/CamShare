/*
 * SqliteManager.h
 *
 *  Created on: 2016-7-12
 *      Author: Max
 */

#ifndef SQLITEMANAGER_H_
#define SQLITEMANAGER_H_

#include "LogManager.h"

#include <common/TimeProc.hpp>
#include <common/StringHandle.h>
#include <common/KThread.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;
#include <string.h>

#include <sqlite3.h>

class SqliteManager;
class SqliteManagerListener {
public:
	virtual ~SqliteManagerListener(){};
};
class SqliteManager {
public:
	SqliteManager();
	virtual ~SqliteManager();

	bool Init(const string& dbname);
	bool Close();
	bool Query(char* sql, char*** result, int* iRow, int* iColumn);
	void FinishQuery(char** result);
	bool ExecSQL(const char* sql);

private:
	bool ExecSQL(sqlite3 *db, const char* sql, char** msg);
	bool QuerySQL(sqlite3 *db, const char* sql, char*** result, int* iRow, int* iColumn, char** msg);

	sqlite3* mdb;
	string mDbname;
};

#endif /* SQLITEMANAGER_H_ */
