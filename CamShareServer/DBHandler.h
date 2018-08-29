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

	/***************************** 录制完成记录 **************************************/
	/**
	 * 插入录制完成记录
	 * @param	record		记录
	 */
	bool InsertRecord(const Record& record);

	/**
	 * 获取录制完成记录
	 * @param	records[out]	记录列表
	 * @param	maxSize			记录列表容量
	 * @param	getSize			记录列表实际内容长度
	 */
	bool GetRecords(Record* pRecords, int maxSize, int& getSize);

	/**
	 * 删除录制完成记录
	 * @param	record		记录
	 */
	bool RemoveRecord(const Record& record);

	/**
	 * 删除录制完成记录(批量)
	 * @param	records		记录列表
	 * @param	size		记录列表实际内容长度
	 */
	bool RemoveRecords(const Record* pRecords, int size);

	/**
	 * 获取录制完成记录剩余数量
	 * @param	getSize		记录列表实际内容长度
	 */
	bool GetRecordsCount(unsigned int& getSize);
	/***************************** 录制完成记录 end **************************************/

	/***************************** 录制完成错误记录 **************************************/
	/**
	 * 插入录制完成错误上传记录
	 * @param	record		记录
	 * @param	errorCode	错误码
	 */
	bool InsertErrorRecord(const Record& record, const string& errorCode);

	/**
	 * 删除录制完成错误记录
	 * @param	recordId		记录
	 */
	bool RemoveErrorRecord(const string& recordId);

	/**
	 * 获取录制完成错误记录
	 * @param	record[out]		记录
	 * @param	recordId		记录ID
	 */
	bool GetErrorRecord(Record* pRecord, const string& recordId);

	/**
	 * 获取录制完成错误记录剩余数量
	 * @param	getSize		记录列表实际内容长度
	 */
	bool GetRecordsErrorCount(unsigned int& getSize);
	/***************************** 录制完成错误记录 end **************************************/

private:
	/**
	 *	创建表
	 */
	bool CreateTable();

	/**
	 * 录制完成记录库
	 */
	SqliteManager mSqliteManager;

	/**
	 * 录制完成上传失败记录库
	 */
	SqliteManager mSqliteManagerError;
};

#endif /* DBHANDLER_H_ */
