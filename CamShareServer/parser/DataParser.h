/*
 * DataParser.h
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 */

#ifndef DATAPARSER_H_
#define DATAPARSER_H_

#include "IDataParser.h"
#include <string.h>

class DataParser : public IDataParser {
public:
	DataParser();
	virtual ~DataParser();

	void SetNextParser(IDataParser *parser);
	virtual int ParseData(char* buffer, int len);

public:
	void* custom;

private:
	IDataParser *mParser;

};

#endif /* DATAPARSER_H_ */
