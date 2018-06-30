/**
 * Sample program that reads tags for a fixed period of time (500ms)
 * and prints the tags found.
 * @file read.c
 */

#include "tm_reader.h"
#include "tmr_utils.h"
#include "serial_reader_imp.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include "common.h"

int setup_reader()
{
    int SDKErr;
    int i, antCnt = 2;
    int numofTag = 0;
    unsigned char *pAntList;
    TagDevEPCStruct Taglist[100];
    API_STATUS run_status = 0;
    TMR_Reader Readplan, *pReadplan;
    char epcStr[128];
 
    pReadplan = &Readplan;
    run_status = m6e_Port_Open(pReadplan, &SDKErr);
    if(run_status > API_SUCCESS)
    {
        return -1;
    }
    run_status = m6e_Port_Connect(pReadplan, &SDKErr);
    if(run_status > API_SUCCESS)
    {
        return -1;
    }

    run_status = setRegion(pReadplan, TMR_REGION_NONE, &SDKErr);
    if(run_status > API_SUCCESS)
    {
        return -1;
    }
    run_status = checkSupportAntenna(pReadplan, &SDKErr);
    if(run_status > API_SUCCESS)
    {
        return -1;
    } 

	unsigned char antList[4] = {0,2,0,0};
	test1(pReadplan, &antList, POWER_LEVEL_7);

}

void test1(TMR_Reader *pReadplan, unsigned char *pAntList, int power)
{
	int SDKErr;
	int run_status;
	int antCnt = 1;
    TagDevEPCStruct Taglist[100];
	int numofTag;
	TMR_SR_ProtocolConfiguration protokey;
	int i;
	char epcStr[128];


	run_status = setPower(pReadplan, power, &SDKErr);
    if(run_status > API_SUCCESS)
		printf("Setpoer error\n");

//    run_status = Commit_ReadPlan(pReadplan,antCnt, pAntList, &SDKErr);
//    if(run_status > API_SUCCESS)
//		printf("Commit plan error\n");
 
    /* set SESSION 2 */
    int value;
    protokey.protocol = TMR_TAG_PROTOCOL_GEN2;
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_SESSION;
    value = 2;

    TMR_SR_cmdSetProtocolConfiguration(pReadplan, protokey.protocol, protokey, &value);

    /* Set Read Mode A */
    protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARGET;
    value = 2;

    TMR_SR_cmdSetProtocolConfiguration(pReadplan, protokey.protocol, protokey, &value);

    run_status = readTagDisc(pReadplan,(TagDevEPCStruct *) &Taglist,&numofTag,&SDKErr);
    if(run_status > API_SUCCESS)
		printf("Read tag failed\n");


	for(i=0; i < numofTag; i++)
	{
		TMR_bytesToHex(Taglist[i].tag, Taglist[i].EPCLen, epcStr);
		printf("%s: %s ant: %d rssi: %d\n", __FUNCTION__,
			epcStr, Taglist[i].antid, Taglist[i].rssi);
	}

}
