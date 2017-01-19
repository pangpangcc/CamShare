/*
 * HttpParser.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#include "HttpParser.h"

#include <common/LogManager.h>

#define HTTP_URL_MAX_FIRST_LINE 2048
#define HTTP_URL_MAX_PATH 4096

#define HTTP_PARAM_SEP ":"
#define HTTP_LINE_SEP "\r\n"
#define HTTP_HEADER_SEP "\r\n\r\n"

HttpParser::HttpParser() {
	// TODO Auto-generated constructor stub
	mHttpType = UNKNOW;
	mPath = "";
	miContentLength = -1;

	mState = HttpState_UnKnow;
	mpCallback = NULL;
}

HttpParser::~HttpParser() {
	// TODO Auto-generated destructor stub

}

void HttpParser::SetCallback(HttpParserCallback* callback) {
	mpCallback = callback;
}

int HttpParser::ParseData(char* buffer, int len) {
	int ret = -1;

	Lock();
	switch( mState ) {
	case HttpState_UnKnow : {
		int lineNumber = 0;
		bool bFlag = false;

		char line[HTTP_URL_MAX_PATH];
		int lineLen = 0;

		const char* header = buffer;
		const char* sepHeader = strstr(buffer, HTTP_HEADER_SEP);
		const char* sep = NULL;
		if( sepHeader ) {
			// 接收头部完成
			mState = HttpState_Header;

			// Parse HTTP header separator
			ret = sepHeader - buffer + strlen(HTTP_HEADER_SEP);

			// Parse HTTP header line separator
			while( true ) {
				if( (sep = strstr(header, HTTP_LINE_SEP)) && (sep != sepHeader) ) {
					lineLen = sep - header;
					if( lineLen < sizeof(line) - 1 ) {
						memcpy(line, header, lineLen);
						line[lineLen] = '\0';

						if( lineNumber == 0 ) {
							// 暂时只获取第一行
							bFlag = ParseFirstLine(line);
							break;
						} else {
							ParseHeader(line);
						}
					}

					// 换行
					header += lineLen + strlen(HTTP_LINE_SEP);
					lineNumber++;

				} else {
					break;
				}
			}
		}

		if( mpCallback ) {
			if( mState == HttpState_Header) {
				// 解析头部完成
				if( bFlag ) {
					// 解析第一行完成
					mpCallback->OnHttpParserHeader(this);
				} else {
					// 解析第一行错误
					mpCallback->OnHttpParserError(this);
				}
			} else if( len > HTTP_URL_MAX_PATH ) {
				// 头部超过限制 HTTP_URL_MAX_PATH
				mpCallback->OnHttpParserError(this);
			}
		}
	}break;
	case HttpState_Header:{
		// 暂时不处理
		// 接收头部完成, 继续接收body
//		if( miContentLength > 0 ) {
//
//		} else if( miContentLength == 0 ) {
//			ret = len;
//		} else {
//
//		}
	}break;
	case HttpState_Body:{
		// 暂时不处理
		// 接收body完成还有数据, 出错
		if( mpCallback ) {
			mpCallback->OnHttpParserError(this);
		}
	}break;
	default:break;
	}

	Unlock();

	return ret;
}

string HttpParser::GetParam(const string& key)  {
	string result = "";
	Parameters::iterator itr = mParameters.find(key.c_str());
	if( itr != mParameters.end() ) {
		result = (itr->second);
	}
	return result;
}

string HttpParser::GetPath() {
	return mPath;
}

HttpType HttpParser::GetType() {
	return mHttpType;
}

bool HttpParser::ParseFirstLine(const string& wholeLine) {
	bool bFlag = true;
	string line;
	int i = 0;
	string::size_type index = 0;
	string::size_type pos;

	while( string::npos != index ) {
		pos = wholeLine.find(" ", index);
		if( string::npos != pos ) {
			// 找到分隔符
			line = wholeLine.substr(index, pos - index);
			// 移动下标
			index = pos + 1;

		} else {
			// 是否最后一次
			if( index < wholeLine.length() ) {
				line = wholeLine.substr(index, pos - index);
				// 移动下标
				index = string::npos;

			} else {
				// 找不到
				index = string::npos;
				break;
			}
		}

		switch(i) {
		case 0:{
			// 解析http type
			if( strcasecmp("GET", line.c_str()) == 0 ) {
				mHttpType = GET;

			} else if( strcasecmp("POST", line.c_str()) == 0 ) {
				mHttpType = POST;

			} else {
				bFlag = false;
				break;
			}
		}break;
		case 1:{
			// 解析url
			Arithmetic ari;
			char temp[HTTP_URL_MAX_FIRST_LINE] = {'\0'};
			int decodeLen = ari.decode_url(line.c_str(), line.length(), temp);
			temp[decodeLen] = '\0';
			string path = temp;
			string::size_type posSep = path.find("?");
			if( (string::npos != posSep) && (posSep + 1 < path.length()) ) {
				// 解析路径
				mPath = path.substr(0, posSep);

				// 解析参数
				string param = path.substr(posSep + 1, path.length() - (posSep + 1));
				ParseParameters(param);

			} else {
				mPath = path;
			}

		}break;
		default:break;
		};

		i++;
	}

	return bFlag;
}

void HttpParser::ParseParameters(const string& wholeLine) {
	string key;
	string value;

	string line;
	string::size_type posSep;
	string::size_type index = 0;
	string::size_type pos;

	while( string::npos != index ) {
		pos = wholeLine.find("&", index);
		if( string::npos != pos ) {
			// 找到分隔符
			line = wholeLine.substr(index, pos - index);
			// 移动下标
			index = pos + 1;

		} else {
			// 是否最后一次
			if( index < wholeLine.length() ) {
				line = wholeLine.substr(index, pos - index);
				// 移动下标
				index = string::npos;

			} else {
				// 找不到
				index = string::npos;
				break;
			}
		}

		posSep = line.find("=");
		if( (string::npos != posSep) && (posSep + 1 < line.length()) ) {
			key = line.substr(0, posSep);
			value = line.substr(posSep + 1, line.length() - (posSep + 1));
			mParameters.insert(Parameters::value_type(key, value));

		} else {
			key = line;
		}

	}
}

void HttpParser::ParseHeader(const string& line) {
//	char* paramSep = NULL;
//	char* key = NULL;
//	char* value = NULL;
//
//	// Parse HTTP header line parameter separator
//	if( (paramSep = strstr(line, HTTP_PARAM_SEP)) ) {
//		*paramSep = '\0';
//		key = line;
//
//		value = paramSep + strlen(HTTP_PARAM_SEP);
//		value = switch_strip_spaces(value, SWITCH_FALSE);
//	}
}

void HttpParser::Lock() {
	mClientMutex.lock();
}

void HttpParser::Unlock() {
	mClientMutex.unlock();
}
