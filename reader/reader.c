#include <mysql/mysql.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys_msg.h>
#include <unistd.h>
#include "tm_reader.h"
#include "serial_reader_imp.h"
#include "tmr_utils.h"
#include "reader.h"

void reader_msg_handler(READER_MSG_T *Incoming);
int reader_read_tag();
void *reader_dog_bark_task();
int reader_to_dbase(char *, int, int, int);

int reader_main_task()
{
    READER_MSG_T reader_msg;
	logging(DBG_INFO, "Create reader message\n");
	int msgid = create_msg(READER_MSGQ_KEY);
	pthread_t reader_wd_id = -1;
	int result;
	
	if(msgid < 0)
	{
		printf("Failed to create reader message queue\n");
		return 0;
	}	

    /* Create thread for DBASE WD response */
    if(reader_wd_id == -1)
    {
        result = pthread_create(&reader_wd_id, NULL, reader_dog_bark_task, NULL);
        if(result == 0)
            logging(DBG_DETAILED, "Starting reader dog bark thread.\n");
        else
            logging(DBG_ERROR, "Reader WD thread launch failed\n");
    }


	while(1) {
        recv_msg(msgid, (void *) &reader_msg, sizeof(READER_MSG_T), MSG_TIMEOUT);
        reader_msg_handler((READER_MSG_T *) &reader_msg);
        usleep(10000);
	}

}

void *reader_dog_bark_task()
{
    while(1) {
        send_dog_bark(READER_MODULE_ID);
        sleep(1);
    }
}

void reader_msg_handler(READER_MSG_T *Incoming)
{
    logging(DBG_DBG, "%s %d: *** Incoming->modname: %s ***\n", 
					__FUNCTION__, __LINE__, modname[Incoming->header.moduleID]);
    switch(Incoming->header.subType)
    {
		case MSG_TAG_READ:
			reader_read_tag();
			break;
        default:
            logging(DBG_INFO, "%s: Unknown message type %d\n",__FUNCTION__, Incoming->header.subType);
            break;
    }
}

int reader_read_tag()
{
    static TMR_Reader ReadPlan;
    static TMR_Reader *pReadplan = NULL;
    static TMR_ReadPlan plan;
    unsigned char *pAntList, antList[4] = {1,2,3,4};
	int antCnt = 4;
    int numofTag, SDKErr;
    TagdevSDKEPCStructDiscoverTags DiscTag, *pDiscTag;
    TagdevSDKEPCStruct Tags[100], *pTags;
	TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;

	pTags = (TagdevSDKEPCStruct *) &Tags[0];
	/* setup read plan */
	if(pReadplan == NULL)
	{
        memset(&ReadPlan, '\0', sizeof(TMR_Reader));
        pReadplan = &ReadPlan;
        if(API_SUCCESS < m6e_Port_Open(pReadplan, &SDKErr))
			return SDKErr;
        if(API_SUCCESS < m6e_Port_Connect(pReadplan, &SDKErr))
			return SDKErr;
        if(API_SUCCESS < m6esetPower(pReadplan, ANT_DEFAULT_POWER, &SDKErr))
			return SDKErr;
        if(API_SUCCESS < setRegion(pReadplan, TMR_REGION_NONE, &SDKErr))
			return SDKErr;
        if(API_SUCCESS < checkSupportAntenna(pReadplan, &SDKErr))
			return SDKErr;

		pAntList = (unsigned char *) &antList;
        if(API_SUCCESS < setupAntenna(&plan, antCnt, pAntList, &SDKErr))
			return SDKErr;
        if(API_SUCCESS < Commit_ReadPlan(pReadplan, &plan, &metadata, &SDKErr))
			return SDKErr;
	}

    pDiscTag = &DiscTag;
    pDiscTag->pTagdevSDKEPC = pTags;

	if(API_SUCCESS < readTagDisc(pReadplan, pDiscTag, &SDKErr))
		return SDKErr;

	int i;
	char temp[64];
	char epc[128];

	if(pDiscTag->nTags > 0)
	{
		for(i=0; i < pDiscTag->nTags; i++)
		{
			bzero(&temp, Tags[i].EPCLen);
			TMR_bytesToHex(Tags[i].tag, Tags[i].EPCLen, temp);
//			sprintf(epc, "EPC: %s %d %d %d\n", temp, Tags[i].EPCLen, 
//					Tags[i].antenna, Tags[i].rssi); 
//			logging(DBG_DBG, "Tags: %s\n", epc);
			
			/* Send tags to database */
			reader_to_dbase((char *) &temp, Tags[i].EPCLen, Tags[i].antenna, Tags[i].rssi);
		}
	}
	else
		logging(DBG_DBG, "Antenna read No tag\n");
	TMR_SR_destroy(pReadplan);
	pReadplan = NULL;
	return 0;
}

int reader_to_dbase(char *epc, int epcLen, int ant, int rssi)
{
	int msgid;
	DBASE_MSG_T reader_msg;
	bzero(&reader_msg, sizeof(DBASE_MSG_T));

	reader_msg.header.moduleID = READER_MODULE_ID;
	reader_msg.header.subType = MSG_TAG;
	memcpy(reader_msg.tag, epc, epcLen*2);
	reader_msg.antenna = ant;
	reader_msg.rssi = rssi;
	reader_msg.epcLen = epcLen;

	msgid = open_msg(DBASE_MSGQ_KEY);
    if(msgid < 0)
	{
        logging(DBG_ERROR, "Error invalid message queue\n");
        return -1;
    }

    send_msg(msgid, (void *) &reader_msg, sizeof(DBASE_MSG_T), 3);
}

