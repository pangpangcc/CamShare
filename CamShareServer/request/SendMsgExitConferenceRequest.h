/*
 * SendMsgExitConferenceRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef SendMsgExitConferenceRequest_H_
#define SendMsgExitConferenceRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>
#include <livechat/ILiveChatClient.h>

class SendMsgExitConferenceRequest : public BaseRequest {
public:
	SendMsgExitConferenceRequest();
	virtual ~SendMsgExitConferenceRequest();

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

#endif /* SENDMSGEXITCONFERENCEREQUEST_H_ */
