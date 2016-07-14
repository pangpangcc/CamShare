/*
 * DBHandler.h
 *
 *  Created on: 2016年7月12日
 *      Author: max
 */

#ifndef DBHANDLER_H_
#define DBHANDLER_H_

#include "SqliteManager.h"

typedef struct Record {
	Record() {
		id = "";
		conference = "";
		siteId = "";
		filePath = "";
		startTime = "";
		endTime = "";
	}
	Record(const string& conference,
			const string& siteId,
			const string& filePath,
			const string& startTime,
			const string& endTime
			) {
		this->id = "";
		this->conference = conference;
		this->siteId = siteId;
		this->filePath = filePath;
		this->startTime = startTime;
		this->endTime = endTime;
	}
	Record(const Record& item){
		this->id = item.id;
		this->conference = item.conference;
		this->siteId = item.siteId;
		this->filePath = item.filePath;
		this->startTime = item.startTime;
		this->endTime = item.endTime;
	}
	Record& operator=(const Record& item) {
		this->id = item.id;
		this->conference = item.conference;
		this->siteId = item.siteId;
		this->filePath = item.filePath;
		this->startTime = item.startTime;
		this->endTime = item.endTime;
		return *this;
	}
	~Record() {

	}

	string id;
	string conference;
	string siteId;
	string filePath;
	string startTime;
	string endTime;
}Record;

class DBHandler {
public:
	DBHandler();
	virtual ~DBHandler();

	/**
	 * 初始化
	 */
	bool Init();
	/**
	 * 插入录制完成记录
	 */
	bool InsertRecord(const Record& record);
	/**
	 * 获取录制完成记录
	 */
	bool GetRecords(Record* records, int maxSize, int& getSize);
	/**
	 * 删除录制完成记录
	 */
	bool RemoveRecords(Record* records, int size);
	/**
	 * 获取录制完成记录剩余数量
	 */
	bool GetRecordsCount(unsigned int& getSize);

private:
	/**
	 *	创建表
	 */
	bool CreateTable();

	SqliteManager mSqliteManager;
};

#endif /* DBHANDLER_H_ */
