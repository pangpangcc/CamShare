/*
 * SendMsgEnterConferenceRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef SendMsgEnterConferenceRequest_H_
#define SendMsgEnterConferenceRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>
#include <livechat/ILiveChatClient.h>

#include <list>
using namespace std;

class SendMsgEnterConferenceRequest : public BaseRequest {
public:
	SendMsgEnterConferenceRequest();
	virtual ~SendMsgEnterConferenceRequest();

	bool StartRequest();
	void FinisRequest(bool bFinish);
	string ParamString();

	void SetParam(
			FreeswitchClient* freeswitch,
			ILiveChatClient* livechat,
			int seq,
			const string& fromId,
			const string& toId,
			const list<string>& userList
			);

	MemberType GetMemberType();

private:
	FreeswitchClient* mpFreeswitch;
	ILiveChatClient* mpLivechat;
	int mSeq;
	string mFromId;
	string mToId;
	list<string> mUserList;
};

#endif /* SENDMSGENTERCONFERENCEREQUEST_H_ */
