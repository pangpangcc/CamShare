/*
 * DBHandler.cpp
 *
 *  Created on: 2016年7月12日
 *      Author: max
 */

#include "DBHandler.h"
#include <common/Math.h>

DBHandler::DBHandler() {
	// TODO Auto-generated constructor stub

}

DBHandler::~DBHandler() {
	// TODO Auto-generated destructor stub
}

bool DBHandler::Init() {
	return (mSqliteManager.Init() && CreateTable());
}

bool DBHandler::CreateTable() {
	char sql[2048] = {'\0'};

	// 建表
	sprintf(sql,
			"CREATE TABLE IF NOT EXISTS record("
						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"conference TEXT,"
						"siteid TEXT,"
						"filepath TEXT,"
						"starttime TEXT,"
						"endtime TEXT"
						");"
	);

	if( !mSqliteManager.ExecSQL(sql) ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::CreateTable( "
				"tid : %d, "
				"[创建录制完成记录表, 失败]"
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);

		return false;
	}

	return true;
}

bool DBHandler::InsertRecord(const Record& record) {
	char sql[2048] = {0};
	snprintf(sql, sizeof(sql) - 1,
			"REPLACE INTO record(`conference`, `siteid`, `filepath`, `starttime`, `endtime`) VALUES('%s', '%s', '%s', '%s', '%s')",
			record.conference.c_str(),
			record.siteId.c_str(),
			record.filePath.c_str(),
			record.startTime.c_str(),
			record.endTime.c_str()
			);

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"DBHandler::InsertRecord( "
			"tid : %d, "
			"[插入录制完成记录], "
			"sql : %s "
			")",
			(int)syscall(SYS_gettid),
			sql
			);

	if( !mSqliteManager.ExecSQL(sql) ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::InsertRecord( "
				"tid : %d, "
				"[插入录制完成记录, 失败]"
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);

		return false;
	}

	return true;
}

bool DBHandler::GetRecords(Record* records, int maxSize, int& getSize) {
	// 执行查询
	char sql[2048] = {'\0'};

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;
	getSize = 0;

	sprintf(sql, "SELECT * FROM record ORDER BY `id` ASC LIMIT %d;", maxSize);
	bResult = mSqliteManager.Query(sql, &result, &iRow, &iColumn);
	if( bResult && result && iRow > 0 && iColumn > 5 ) {
		int min = MIN(maxSize, iRow + 1);
		for(int i = 1, j = 0; i < min; i++, j++) {
			Record* pRecord = &(records[j]);
			int rowIndex = i * iColumn;
			if( result[rowIndex] ) {
				pRecord->id = result[rowIndex];
			}
			if( result[rowIndex + 1] ) {
				pRecord->conference = result[rowIndex + 1];
			}
			if( result[rowIndex + 2] ) {
				pRecord->siteId = result[rowIndex + 2];
			}
			if( result[rowIndex + 3] ) {
				pRecord->filePath = result[rowIndex + 3];
			}
			if( result[rowIndex + 4] ) {
				pRecord->startTime = result[rowIndex + 4];
			}
			if( result[rowIndex + 5] ) {
				pRecord->endTime = result[rowIndex + 5];
			}
		}
		getSize = iRow;
	}
	mSqliteManager.FinishQuery(result);

	if( !bResult ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::GetRecords( "
				"tid : %d, "
				"[获取录制完成记录, 失败], "
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);
	}

	return bResult;
}

bool DBHandler::RemoveRecord(const Record& record) {
	bool bResult = false;
	char sql[2048] = {'\0'};

	sprintf(sql, "DELETE FROM record WHERE conference = '%s' AND siteid = '%s';", record.conference.c_str(), record.siteId.c_str());
	bResult = mSqliteManager.ExecSQL(sql);

	if( !bResult ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::RemoveRecord( "
				"tid : %d, "
				"[删除录制完成记录, 失败]"
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);
	}

	return bResult;
}

bool DBHandler::RemoveRecords(Record* records, int size) {
	bool bResult = false;
	char sql[2048] = {'\0'};

	bResult = mSqliteManager.ExecSQL("BEGIN;");
	if( bResult ) {
		for(int i = 0; i < size; i++) {
			Record record = records[i];
			sprintf(sql, "DELETE FROM record WHERE conference = '%s' AND siteid = '%s';", record.conference.c_str(), record.siteId.c_str());
			bResult = mSqliteManager.ExecSQL(sql);
		}

		bResult = mSqliteManager.ExecSQL("COMMIT");
	}

	if( !bResult ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::RemoveRecords( "
				"tid : %d, "
				"[删除录制完成记录(批量), 失败]"
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);
	}

	return bResult;
}

bool DBHandler::GetRecordsCount(unsigned int& getSize) {
	// 执行查询
	char sql[2048] = {'\0'};

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	sprintf(sql, "SELECT count(id) FROM record;");
	bResult = mSqliteManager.Query(sql, &result, &iRow, &iColumn);
	if( bResult && result && iRow > 0 && iColumn > 0 ) {
		if( result[1 * iColumn] ) {
			getSize = atoi(result[1 * iColumn]);
		}
	}
	mSqliteManager.FinishQuery(result);

	if( !bResult ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBHandler::GetRecordsCount( "
				"tid : %d, "
				"[获取录制完成记录剩余数量, 失败], "
				"sql : %s "
				")",
				(int)syscall(SYS_gettid),
				sql
				);
	}

	return bResult;
}
