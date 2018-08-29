/*
 * OnlineStatusRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 *      发送用户在线状态改变命令
 */

#ifndef OnlineStatusRequest_H_
#define OnlineStatusRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>
#include <livechat/ILiveChatClient.h>

class OnlineStatusRequest : public BaseRequest {
public:
	OnlineStatusRequest();
	virtual ~OnlineStatusRequest();

	bool StartRequest();
	void FinisRequest(bool bFinish);

	void SetParam(
			FreeswitchClient* freeswitch,
			ILiveChatClient* livechat,
			int seq,
			const string& userId,
			bool online
			);

private:
	FreeswitchClient* mpFreeswitch;
	ILiveChatClient* mpLivechat;
	int mSeq;
	string mUserId;
	bool mbOnline;
};

#endif /* OnlineStatusRequest_H_ */
