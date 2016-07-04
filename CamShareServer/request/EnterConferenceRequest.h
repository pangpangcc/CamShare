/*
 * EnterConferenceRequest.h
 *
 *  Created on: 2016年3月8日
 *      Author: Max.Chiu
 */

#ifndef EnterConferenceRequest_H_
#define EnterConferenceRequest_H_

#include "BaseRequest.h"

#include <FreeswitchClient.h>
#include <livechat/ILiveChatClient.h>

class EnterConferenceRequest : public BaseRequest {
public:
	EnterConferenceRequest();
	virtual ~EnterConferenceRequest();

	bool StartRequest();
	void FinisRequest(bool bFinish);
	void SetParam(
			FreeswitchClient* freeswitch,
			ILiveChatClient* livechat,
			int seq,
			const string& serverId,
			const string& fromId,
			const string& toId,
			MemberType type,
			const string& key
			);

	MemberType GetMemberType();

private:
	FreeswitchClient* mpFreeswitch;
	ILiveChatClient* mpLivechat;
	int mSeq;
	string mServerId;
	string mFromId;
	string mToId;
	MemberType mType;
	string mKey;
};

#endif /* ENTERCONFERENCERequest_H_ */
