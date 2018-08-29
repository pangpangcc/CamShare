/*
 * author: Samson.Fan
 *   date: 2015-03-24
 *   file: ITask.h
 *   desc: Task（任务）接口类，一般情况下每个Task对应处理一个协议
 */

#include "ITask.h"
#include "RecvEnterConferenceTask.h"
#include "RecvKickUserFromConferenceTask.h"
#include "RecvGetOnlineListTask.h"

// 根据 cmd 创建 task
ITask* ITask::CreateTaskWithCmd(int cmd)
{
	ITask* task = NULL;
	switch (cmd) {
	case TCMD_RECVENTERCONFERENCE:
		task = new RecvEnterConferenceTask();
		break;
	case TCMD_RECVKICKUSERFROMCONFERENCE:
		task = new RecvKickUserFromConferenceTask();
		break;
	case TCMD_RECV_GET_ONLINE_LIST:
		task = new RecvGetOnlineListTask();
		break;
//	case TCMD_RECVEMOTION:
//		task = new RecvEmotionTask();
//		break;
//	case TCMD_RECVVOICE:
//		task = new RecvVoiceTask();
//		break;
//	case TCMD_RECVWARNING:
//		task = new RecvWarningTask();
//		break;
//	case TCMD_UPDATESTATUS:
//		task = new UpdateStatusTask();
//		break;
//	case TCMD_UPDATETICKET:
//		task = new UpdateTicketTask();
//		break;
//	case TCMD_RECVEDITMSG:
//		task = new RecvEditMsgTask();
//		break;
//	case TCMD_RECVTALKEVENT:
//		task = new RecvTalkEventTask();
//		break;
//	case TCMD_RECVTRYBEGIN:
//		task = new RecvTryTalkBeginTask();
//		break;
//	case TCMD_RECVTRYEND:
//		task = new RecvTryTalkEndTask();
//		break;
//	case TCMD_RECVEMFNOTICE:
//		task = new RecvEMFNoticeTask();
//		break;
//	case TCMD_RECVKICKOFFLINE:
//		task = new RecvKickOfflineTask();
//		break;
//	case TCMD_RECVPHOTO:
//		task = new RecvPhotoTask();
//		break;
//	case TCMD_RECVSHOWPHOTO:
//		task = new RecvShowPhotoTask();
//		break;
//    case TCMD_RECVLADYVOICECODE:
//        task = new RecvLadyVoiceCodeTask();
//        break;
//    case TCMD_RECVIDENTIFYCODE:
//        task = new RecvIdentifyCodeTask();
//        break;
//    case TCMD_RECVVIDEO:
//        task = new RecvVideoTask();
//        break;
//    case TCMD_RECVSHOWVIDEO:
//    	task = new RecvShowVideoTask();
//    	break;
	}

	return task;
}
