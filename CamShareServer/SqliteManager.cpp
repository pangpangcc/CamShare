/*
 * SqliteManager.cpp
 *
 *  Created on: 2016-7-12
 *      Author: Max
 */
#include "SqliteManager.h"

SqliteManager::SqliteManager() {
	// TODO Auto-generated constructor stub
	mdb = NULL;
	sqlite3_config(SQLITE_CONFIG_MEMSTATUS, false);
}

SqliteManager::~SqliteManager() {
	// TODO Auto-generated destructor stub
}

bool SqliteManager::Init(const string& dbname) {
	printf("# SqliteManager initializing %s ... \n", dbname.c_str());

	bool bFlag = false;
	int ret;

	ret = sqlite3_open(dbname.c_str(), &mdb);
	if( ret == SQLITE_OK ) {
		bFlag = ExecSQL(mdb, (char*)"PRAGMA synchronous = OFF;", 0);
	}

	printf("# SqliteManager initialize %s OK. \n", dbname.c_str());

	return bFlag;
}

void SqliteManager::FinishQuery(char** result) {
	// free memory
	sqlite3_free_table(result);
}

bool SqliteManager::Query(char* sql, char*** result, int* iRow, int* iColumn) {
	char *msg = NULL;
	bool bFlag = true;

	sqlite3* db = mdb;
	bFlag = QuerySQL( db, sql, result, iRow, iColumn, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"SqliteManager::Query( "
								"[执行查询, 失败], "
								"sql : %s, "
								"msg : %s "
								")",
								sql,
								msg
								);
		bFlag = false;
		sqlite3_free(msg);
		msg = NULL;
	}

	return bFlag;
}

bool SqliteManager::QuerySQL(sqlite3 *db, const char* sql, char*** result, int* iRow, int* iColumn, char** msg) {
	int ret;
	do {
		ret = sqlite3_get_table( db, sql, result, iRow, iColumn, msg );
		if( ret == SQLITE_BUSY ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"SqliteManager::QuerySQL( "
					"[执行查询, 繁忙], "
					"sql : %s, "
					"msg : %s "
					")",
					sql,
					sqlite3_errmsg(db)
					);

			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}

		if( ret != SQLITE_OK ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"SqliteManager::QuerySQL( "
					"[执行查询, 失败], "
					"ret : %d, "
					"sql : %s, "
					"msg : %s "
					")",
					ret,
					sql,
					sqlite3_errmsg(db)
					);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}

bool SqliteManager::ExecSQL(const char* sql) {
	bool bFlag = true;
	char *msg = NULL;

	sqlite3* db = mdb;
	bFlag = ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"SqliteManager::ExecSQL( "
								"[执行查询, 失败], "
								"sql : %s, "
								"msg : %s "
								")",
								sql,
								msg
								);
		bFlag = false;
		sqlite3_free(msg);
		msg = NULL;
	}

	return bFlag;
}

bool SqliteManager::ExecSQL(sqlite3 *db, const char* sql, char** msg) {
	int ret;
	do {
		ret = sqlite3_exec( db, sql, NULL, NULL, msg );
		if( ret == SQLITE_BUSY ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"SqliteManager::ExecSQL( "
					"[执行查询, 繁忙], "
					"sql : %s, "
					"msg : %s "
					")",
					sql,
					sqlite3_errmsg(db)
					);

			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}
		if( ret != SQLITE_OK ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"SqliteManager::ExecSQL( "
					"[执行查询, 失败], "
					"ret : %d, "
					"sql : %s, "
					"msg : %s "
					")",
					ret,
					sql,
					sqlite3_errmsg(db)
					);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}
