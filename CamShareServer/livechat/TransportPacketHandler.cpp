/*
 * author: Samson.Fan
 *   date: 2015-03-25
 *   file: TransportPacketHandler.cpp
 *   desc: 传输包处理实现类
 */

#include "TransportPacketHandler.h"
#include "task/ITask.h"
#include "zlib.h"
#include <common/KLog.h>

// 使用 ntohl 需要include跨平台socket头文件
#include "ISocketHandler.h"

CTransportPacketHandler::CTransportPacketHandler(void)
{
	m_buffer = NULL;
	m_bufferLen = 0;
}

CTransportPacketHandler::~CTransportPacketHandler(void)
{
	delete []m_buffer;
}

// 组包
bool CTransportPacketHandler::Packet(ITask* task, void* data, unsigned int dataSize, unsigned int& dataLen)
{
	//printf("CTransportPacketHandler::Packet() task:%p, data:%p, dataLen:%d\n", task, data, dataLen);

	FileLog("LiveChatClient", "CTransportPacketHandler::Packet() begin");
	FileLog("LiveChatClient", "CTransportPacketHandler::Packet() task:%p, data:%p", task, data);
	bool result = false;

	if (task->GetSendDataProtocolType() == NOHEAD_PROTOCOL) {
		// 获取数据
		unsigned int bodyLen = 0;
		result = task->GetSendData(data, dataSize, bodyLen);
		if  (result)
		{
			dataLen = bodyLen;
		}
	}
	else {
		TransportSendHeader* sendHeader = (TransportSendHeader *)data;
		string serverId = task->GetServerId();
		int serverIdLength = serverId.length();
//		int headerLen = sizeof(TransportSendHeader) - sizeof(sendHeader->requesttype) + serverIdLength + sizeof(TransportHeader);
		if (true) {
			sendHeader->shiftKey = 0;
			sendHeader->requestremote = 1;
			// 没有服务器Id长度, 没有就为0
			sendHeader->serverNamelength = serverIdLength;
			if( serverIdLength > 0 ) {
				// 复制服务器Id
				memcpy(sendHeader->serverName, serverId.c_str(), serverId.length());
			} else {
				// 没有服务器Id默认填0, 长度改为1
				sendHeader->serverName[0] = (unsigned char)0;
				serverIdLength = 1;
			}

			FileLog("LiveChatClient", "CTransportPacketHandler::Packet() "
					"sendHeader->serverNamelength : %d, "
					"sendHeader->serverName : %s",
					sendHeader->serverNamelength,
					serverId.c_str()
					);

//		int headerLen = sizeof(TransportSendHeader) + sizeof(TransportHeader);
//		if (dataSize > headerLen) {

			int sendHeaderLen = sendHeader->GetHeaderLength();
			int headerLen = sendHeaderLen + sizeof(TransportHeader);
			FileLog("LiveChatClient", "CTransportPacketHandler::Packet() sendHeaderLen:%d, headerLen:%d", sendHeaderLen, headerLen);

			TransportProtocol* protocol = (TransportProtocol*)(sendHeader->serverName + serverIdLength);
			protocol->header.shiftKey = 0;
			protocol->header.remote = 0;
			protocol->header.request = 0;
			protocol->header.cmd = htonl(task->GetCmdCode());
			protocol->header.seq = htonl(task->GetSeq());
			protocol->header.zip = 0;	// 不压缩
			protocol->header.protocolType = (unsigned char)task->GetSendDataProtocolType();

			unsigned int bodyLen = 0;
			result = task->GetSendData(protocol->data, dataSize - headerLen, bodyLen);
			FileLog("LiveChatClient", "CTransportPacketHandler::Packet() task:%p, result:%d", task, result);
			if (result)
			{
				dataLen = headerLen + bodyLen;

				sendHeader->SetDataLength(sizeof(TransportHeader) + bodyLen);
				FileLog("LiveChatClient", "CTransportPacketHandler::Packet() sendHeader->length:%d, bodyLen:%d", sendHeader->length, bodyLen);
				sendHeader->length = htonl(sendHeader->length);

				protocol->SetDataLength(bodyLen);
				protocol->header.length = ntohl(protocol->header.length);

				FileLog("LiveChatClient", "CTransportPacketHandler::Packet() cmd:%d, seq:%d, dataLen:%d"
						, task->GetCmdCode(), task->GetSeq(), dataLen);
			}
		}
	}

	if (result) {
		string hex = "";
		char temp[4];
		for(unsigned int i = 0; i < dataLen; i++) {
			sprintf(temp, "%02x", ((unsigned char *)data)[i]);
			hex += temp;
			hex += " ";
		}
		FileLog("LiveChatClient", "CTransportPacketHandler::Packet() hex:%s", hex.c_str());
	}

	FileLog("LiveChatClient", "CTransportPacketHandler::Packet() end, result:%d", result);

	return result;
}
	
// 解包
UNPACKET_RESULT_TYPE CTransportPacketHandler::Unpacket(void* data, unsigned int dataLen, unsigned int maxLen, TransportProtocol** ppTp, unsigned int& useLen)
{
	UNPACKET_RESULT_TYPE result = UNPACKET_FAIL;
	useLen = 0;
	*ppTp = NULL;
	TransportProtocol* tp = (TransportProtocol*)data;
	unsigned int length = 0;

	FileLog("LiveChatClient", "CTransportPacketHandler::Unpacket() begin");

	if (dataLen >= sizeof(tp->header.length))
	{
		length = ntohl(tp->header.length);
		if (length == 0)
		{
			// 心跳包
			useLen = sizeof(tp->header.length);
		}
		else
		{
			if (dataLen >= length + sizeof(length))
			{
				tp->header.length = length;
				useLen = tp->header.length + sizeof(tp->header.length);
				ShiftRight((unsigned char*)data + sizeof(tp->header.length) + sizeof(tp->header.shiftKey), tp->header.length - sizeof(tp->header.shiftKey), tp->header.shiftKey);
				tp->header.cmd = ntohl(tp->header.cmd);
				tp->header.seq = ntohl(tp->header.seq);

				if (tp->header.zip == 1
					&& tp->GetDataLength() > 0)
				{
					// 需要解压
					result = Unzip(tp, ppTp) ? UNPACKET_SUCCESS : result;
				}
				else {
					*ppTp = tp;
					result = UNPACKET_SUCCESS;
				}
			}
			else if (maxLen >= length + sizeof(length))
			{
				// 需要更多数据，且有足够buffer接收
				result = UNPACKET_MOREDATA;
			}
			else {
				// 严重错误，需要重新连接
				result = UNPACKET_ERROR;
			}
		}
	}
	else {
		result = UNPACKET_MOREDATA;
	}

	if (result == UNPACKET_SUCCESS) {
		FileLog("LiveChatClient", "CTransportPacketHandler::Unpacket() end, result:%d, cmd:%d, seq:%d, length:%d"
				, result, tp->header.cmd, tp->header.seq, tp->header.length);
	}
	else {
		FileLog("LiveChatClient", "CTransportPacketHandler::Unpacket() end, result:%d", result);
	}

	return result;
}


// 移位解密
void CTransportPacketHandler::ShiftRight(unsigned char *data, int length, unsigned char bit) 
{
	if (length == 0) {
		return;
	}
	unsigned char b = bit % 8;
	if (b == 8) {
		return;
	}
	unsigned char mask = 0xff >> (8 - b);
	unsigned char last = data[length - 1];
	unsigned char *p;
	for (p = data; p < data + length; ++p) {
		unsigned char hight = (last & mask) << (8 - b);
		last = *p;		// & 0xff;
		*p = (last >> b) | hight;
	}
}

// 解压
bool CTransportPacketHandler::Unzip(TransportProtocol* tp, TransportProtocol** ppTp)
{
	// 初始化
	bool result = false;
	*ppTp = NULL;
	if (m_bufferLen == 0) {
		RebuildBuffer();
	}

	// 解压
	int unret = Z_OK;
//	uLongf needBuffLen = 0;
	do {

		if (NULL != m_buffer && m_bufferLen > sizeof(tp->header)) {
			uLongf needBuffLen = m_bufferLen - sizeof(tp->header);
			uLong dataLen = tp->GetDataLength();
			unret = uncompress(m_buffer + sizeof(tp->header), &needBuffLen, tp->data, dataLen);
			if (Z_BUF_ERROR == unret) {
				// 缓冲不足
				RebuildBuffer();
				continue;
			}
			else {
				if (Z_OK == unret) {
					// 解压成功
					*ppTp = (TransportProtocol*)m_buffer;
					(*ppTp)->header = tp->header;
					(*ppTp)->SetDataLength(needBuffLen);

					result = true;
				}
				break;
			}
		}
		else {
			// 缓冲出错
			break;
		}
	} while (true);

	return result;
}

// 重建buffer
bool CTransportPacketHandler::RebuildBuffer()
{
	if (m_bufferLen == 0) {
		const unsigned int initBufferLen = 1024;
		m_buffer = new unsigned char[initBufferLen];
		if (NULL != m_buffer) {
			m_bufferLen = initBufferLen;
		}
	}
	else {
		delete []m_buffer;
		unsigned int reLen = m_bufferLen * 2;
		m_buffer = new unsigned char[reLen];
		if (NULL != m_buffer) {
			m_bufferLen = reLen;
			FileLog("LiveChatClient", "CTransportPacketHandler::RebuildBuffer() rebuild buffer, m_bufferLen:%d", m_bufferLen);
		}
	}

	return NULL != m_buffer;
}
