/*
 * OnlineListRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 *      发送用户在线列表命令
 */

#ifndef OnlineListRequest_H_
#define OnlineListRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>
#include <livechat/ILiveChatClient.h>

class OnlineListRequest : public BaseRequest {
public:
	OnlineListRequest();
	virtual ~OnlineListRequest();

	bool StartRequest();
	void FinisRequest(bool bFinish);

	void SetParam(
			FreeswitchClient* freeswitch,
			ILiveChatClient* livechat,
			int seq,
			const list<string>& userList
			);

private:
	FreeswitchClient* mpFreeswitch;
	ILiveChatClient* mpLivechat;
	int mSeq;
	list<string> mUserList;
};

#endif /* OnlineListRequest_H_ */
