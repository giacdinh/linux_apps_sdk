/**************************************************************************
 * File Name:
 * Copyright 2018 RFRain LLC.
 * Date (YYYY-MM-DD) 	Author		Description
 * 2018-03-30			G. Dinh     Created
 * ************************************************************************/

#include "tm_reader.h"
#include "serial_reader_imp.h"
#include "tmr_utils.h"
#include <stdio.h>
#include <string.h> 
#include <reader.h>
#include <stdlib.h>


#define SERIAL_COM_PORT "tmr:///dev/ttyS0"

#define READPLAN_WEIGHT	1000
#define MAX_TAG_LIST 300
#define ANT_MAX_POWER 3000
#define ANT_MIN_COUNT 1
#define ANT_MAX_COUNT 4
#define ANT_MIN_POWER 1000
#define ANTENNA_TYPE_SARGAS		"Sargas"
#define ANTENNA_TYPE_M6E_MICRO	"M6e Micro"
#define ANTENNA_TYPE_M6E_NANO	"M6e Nano"
#define MODEL_MAX_STRING_LEN 64+1
#define GPIO_MIN_PIN 0
#define GPIO_MAX_PIN 4
#define BAD_TEMPURATURE_READ -100
#define MAX_ANT_POWER 3000

int gReadTimeout = READER_READ_TIMEOUT; /* Initialize read time out to default value */

/* Declare for detail usage */

static const char *regions[] = {"UNSPEC", "NA", "EU", "KR", "IN", "JP", "PRC",
                                "EU2", "EU3", "KR2", "PRC2", "AU", "NZ", "REDUCED_FCC"};
static const char *powerModes[] = {"FULL", "MINSAVE", "MEDSAVE", "MAXSAVE", "SLEEP"};
static const char *userModes[] = {"NONE", "PRINTER", NULL, "PORTAL"};
static const char *tagEncodingNames[] = {"FM0", "M2", "M4", "M8"};
static const char *sessionNames[] = {"S0", "S1", "S2", "S3"};
static const char *targetNames[] = {"A", "B", "AB", "BA"};
static const char *gen2LinkFrequencyNames[] = {"250kHz", "300kHz", "320kHz", "40kHz", "640KHz"};
static const char *tariNames[] = {"25us", "12.5us", "6.25us"};
static const char *iso180006bLinkFrequencyNames[] = {"40kHz", "160kHz"};
static const char *iso180006bModulationDepthNames[] = {"99 percent", "11 percent"};
static const char *iso180006bDelimiterNames[] = {"", "Delimiter1", "", "", "Delimiter4"};
static const char *protocolNames[] = {NULL, NULL, NULL, "ISO180006B",
                                      NULL, "GEN2", "UCODE", "IPX64", "IPX256"};
static const char *bankNames[] = {"Reserved", "EPC", "TID", "User"};
static const char *selectOptionNames[] = {"EQ", "NE", "GT", "LT"};
static const char *tagopNames[] = {"Gen2.Read","Gen2.Write","Gen2.Lock",
                                   "Gen2.Kill"};

char *tag_proto[] = { "TMR_TAG_PROTOCOL_NONE",
					  "",
					  "",
					  "TMR_TAG_PROTOCOL_ISO180006B",
					  "",
					  "TMR_TAG_PROTOCOL_GEN2",
					  "TMR_TAG_PROTOCOL_ISO180006B_UCODE",
					  "TMR_TAG_PROTOCOL_IPX64",
					  "TMR_TAG_PROTOCOL_IPX256"
};

#define M6E_PROG_DES "M6e/M6e-Bootloader"
const char *listname(const char *list[], int listlen, unsigned int id);
extern bool strcasecmpcount(const char *a, const char *b, int *matches);
 

/******* These functions are utilities which needed for set up detail operation *******/
/*
const char *listname(const char *list[], int listlen, unsigned int id)
{
	if(list == NULL)
	{
		return ("Unknown");
	}

	if ((id < (unsigned)listlen) && list[id] != NULL)
	{
		return (list[id]);
	}
	return ("Unknown");
}
*/
int listid(const char *list[], int listlen, const char *name)
{
	int i, len, match, bestmatch, bestmatchindex;

	if(list == NULL)
	{
		return -1;
	}

	bestmatch = 0;
	bestmatchindex = -1;
	len = (int)strlen(name);
	for (i = 0; i < listlen; i++)
	{
		if (NULL != list[i])
		{
			if (strcasecmpcount(name, list[i], &match))
			{
				return i; /* Exact match - return immediately */
			}
			if (match == len)
			{
				if (bestmatch == 0)
				{
					/* Prefix match - return if nothing else conflicts */
					bestmatch = match;
					bestmatchindex = i;
				}
				else
				{
					/* More than one prefix match of the same length - ambiguous */
					bestmatchindex = -1;
				}
			}
		}
	}

	return bestmatchindex;
}

const char *regionName(TMR_Region region)
{

	if (region == TMR_REGION_OPEN)
	{
		return "OPEN";
	}
	return ((const char *) listname(regions, numberof(regions), region));
}

TMR_Region regionID(const char *name)
{
	if(name == NULL)
	{
		return TMR_REGION_OPEN;
	}
	if (0 == strcasecmp(name, "OPEN"))
	{
		return TMR_REGION_OPEN;
	}

	return listid(regions, numberof(regions), name);
}

/**************************************************************************/


/**************************************************************************
* Function: pReadPlan_Create()
* This function will setup pointer to readplan structure
*
* Parameters:
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS 
pReadPlan_Create( TMR_Reader *pReadplan)
{

	/* pReadplan = (TMR_Reader *) &Readplan; */

	if(pReadplan == NULL)
	{
		return API_NULL_POINTER;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: m6e_Port_Open()
* This function will setup serial port to talk to M6E reader
* Parameters: TMR_Reader *p_Readplan - reader handle
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS m6e_Port_Open( TMR_Reader *pReadplan
						, int *pSDKErr
						)
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Create connection port */
	ret = TMR_create(pReadplan, SERIAL_COM_PORT);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: m6e_Port_Connect()
* This function will setup serial port to talk to M6E reader
* It probes connection baudrate for different firmware
* Parameters: TMR_Reader *p_Readplan - reader handle
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS m6e_Port_Connect( TMR_Reader *pReadplan
						   , int *pSDKErr
						   )
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Try to connect to serial port with baudrate detect and all default parameters setup */
	ret = TMR_SR_connect(pReadplan);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: m6e_Port_Close()
* This function will destroy read plan and close connection
* Parameters:
* Parameters: TMR_Reader *p_Readplan - reader handle
*             int *pSDKErr           - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS m6e_Port_Close( TMR_Reader *pReadplan
						 , int *pSDKErr
						 )
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Destroy read plan session */
	ret = TMR_SR_destroy(pReadplan);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}


/**************************************************************************
* Function: setBaudRate()
* This function will set baudrate
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  unsigned int baudRate  - set baudrate
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setBaudRate(TMR_Reader *pReadplan
					  , unsigned int baudRate
					  , int *pSDKErr)
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Check for Antenna Region range */
	if(baudRate == 9600 || baudRate == 115200)
	{
		ret = TMR_SR_cmdSetBaudRate(pReadplan, baudRate);
		if(ret != TMR_SUCCESS)
		{
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}
	}
	else
	{
		return API_NUM_OUTOFRANGE;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: setRegion()
* This function will set selected region for reader
* Parameters: TMR_Reader *p_Readplan - reader handle
*             TMR_Region region	   - ENUM list of support regions
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setRegion( TMR_Reader *pReadplan
		     			, TMR_Region region
					, int *pSDKErr)
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}
	/* Check for Antenna Region range */
	if(region > TMR_REGION_OPEN)
	{
		return API_NUM_OUTOFRANGE;
	}

	if (TMR_REGION_NONE == region)
	{
	    TMR_RegionList regions;
	    TMR_Region _regionStore[32];
	    regions.list = _regionStore;
	    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
	    regions.len = 0;

	    ret = TMR_paramGet(pReadplan, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
	    region = regions.list[0];
	}

	/* Set region to read */
	ret = TMR_paramSet(pReadplan, TMR_PARAM_REGION_ID, &region);
	if(ret != TMR_SUCCESS)
	{
	    *pSDKErr = ret;
	    return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: getRegion()
* This function will get the list of region which embedded in the reader
*
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  TMR_RegionList *regions - list support regions
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getRegion( TMR_Reader *pReadplan
					, SDKSetCurRegionStUser *pRegions
					, int *pSDKErr
					)
{
	TMR_Status ret;
	TMR_Region _regionStore[32];
	TMR_RegionList *pRegionsList;
	int i;

	/* Check for null pointer */
	if(pReadplan == NULL || pRegions == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Setup space to store regions after get */

	pRegionsList->list = _regionStore;
	pRegionsList->max = sizeof(_regionStore)/sizeof(_regionStore[0]);
	pRegionsList->len = 0;

	/* Get support region list */
	ret = TMR_paramGet(pReadplan, TMR_PARAM_REGION_SUPPORTEDREGIONS, pRegionsList);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
	else
	{
		i = *(pRegionsList->list);
		strncpy(pRegions->region, regions[i], pRegionsList->len);

	}
	return API_SUCCESS;
}

/**************************************************************************
* Function: getAvailableRegions()
* This function will get the list of region which embedded in the reader
*
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  TMR_RegionList *regions - list support regions
*             int *pSDKErr           - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getAvailableRegions( TMR_Reader *pReadplan
					, SetapplSDKGetAvailReqUserSt *pRegions
					, int *pSDKErr
					)
{
	TMR_RegionList *pRegionsList;
	int i;

	/* Check for null pointer */
	if(pReadplan == NULL || pRegions == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}
	for(i = 0; i < numberof(regions); i++)
		strcpy(pRegions->region[i],regions[i]);
	pRegions->len = i;

	return API_SUCCESS;
}

/**************************************************************************
* Function: setPower()
* This function will set read power level to antenna
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  int readpower - power which antenna will set to
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS m6esetPower( TMR_Reader *pReadplan
				   , int setreadpower
				   , int *pSDKErr
				   )
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Check for allowance power setting range */
	if(setreadpower > ANT_MAX_POWER || setreadpower < ANT_MIN_POWER)
	{
		return API_NUM_OUTOFRANGE;
	}

	/* Set antenna read power */
    ret = TMR_paramSet(pReadplan, TMR_PARAM_RADIO_READPOWER, &setreadpower);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: getPower()
* This function will set read power level to antenna
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  int *pCurPowerLevel - antenna current power
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getPower( TMR_Reader *pReadplan
				   , SDKSetPwrTxAntPwr *pCurPowerLevel
				   , int *pSDKErr
				   )
{
	TMR_Status ret;
	int radioPower;
	unsigned char *antList = NULL;

	/* Check for null pointer */
	if(pReadplan == NULL || pCurPowerLevel == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	*antList = pCurPowerLevel->TxAnt;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_TAGOP_ANTENNA, antList);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	/* Get current set power for antenna */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_RADIO_READPOWER, &radioPower);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
	else
	{
		pCurPowerLevel->ReadPwr = radioPower;
	}
	/* Get current set power for antenna */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_RADIO_WRITEPOWER, &radioPower);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
	else
	{
		pCurPowerLevel->WritePwr = radioPower;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: setProtoGEN2()
* This function will set GEN2 protocol
* Parameters: TMR_Reader *pReadplan - reader handle
**             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setProtolGEN2( TMR_Reader *pReadplan
				   , int *pSDKErr
				   )
{
	TMR_Status ret;
	int protocol;
	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	protocol = TMR_TAG_PROTOCOL_GEN2;
	/* Set protocol GEN2 */
    	ret = TMR_paramSet(pReadplan, TMR_PARAM_TAGOP_PROTOCOL, &protocol);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	return API_SUCCESS;
}
/**************************************************************************
* Function: checkAntenna()
* This function will check list of antenna on reader to see if support
* Parameters:
* TMR_Reader *pReadplan; ReadPlan structure which contain connection information
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS checkSupportAntenna( TMR_Reader *pReadplan
		   	   	   	   	      , int *pSDKErr
		   	   	   	   	      )
{
	TMR_Status ret;
	TMR_String model;
	char str[MODEL_MAX_STRING_LEN];

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	/* Allocate space to store antenna name */
	model.value = str;
	model.max = MODEL_MAX_STRING_LEN;

	/* Get Antenna version */
	ret = TMR_paramGet(pReadplan, TMR_PARAM_VERSION_MODEL, &model);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	/* Check for antenna model */
	if (  ( !strcmp(ANTENNA_TYPE_SARGAS, model.value ) )   \
	   || ( !strcmp(ANTENNA_TYPE_M6E_MICRO, model.value) )  \
	   || ( !strcmp(ANTENNA_TYPE_M6E_NANO, model.value)  ) \
	   )
	{
		return API_ANTENNA_NOTSUPPORT;
	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: readTagSingleAntenna()
* This function will setup antenna and read from  single antenna a time
*
* Parameters:
* TMR_Reader *pReadplan; ReadPlan structure which contain connection information
* int antCount; Number of antenna to read. This case is 1
* char *antList; list of antenna. This case antenna number (ex 1,2,3 or 4 ...)
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS Commit_ReadPlan( TMR_Reader *pReadplan
			  , TMR_ReadPlan * pPlan
			  , TMR_TRD_MetadataFlag * metadata
			  , int *pSDKErr
			  )
{
	TMR_Status ret;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL || pPlan == NULL)
	{
		return API_NULL_POINTER;
	}

	if (pReadplan->readerType == TMR_READER_TYPE_SERIAL)
	{
		/* Set the metadata flags. Configurable Metadata param is not supported for llrp readers
		* metadata = TMR_TRD_METADATA_FLAG_ANTENNAID | TMR_TRD_METADATA_FLAG_FREQUENCY | TMR_TRD_METADATA_FLAG_PHASE; */
		ret = TMR_paramSet(pReadplan, TMR_PARAM_METADATAFLAG, metadata);
		if(ret != TMR_SUCCESS)
		{
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}
	}

	/* Commit read plan */
	ret = TMR_paramSet(pReadplan, TMR_PARAM_READ_PLAN, pPlan);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
	return API_SUCCESS;
}

/**************************************************************************
* Function: setupAntenna()
* This function will setup antenna and read from  single antenna a time
*
* Parameters:
* TMR_ReadPlan *pPlan; ReadPlan structure which contain connection information
* int antCount; Number of antenna to read. This case is 1
* char *antList; list of antenna. This case antenna number (ex 1,2,3 or 4 ...)
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setupAntenna( TMR_ReadPlan * pPlan
			   			  , int antCount
			   			  , unsigned char *pAntList
			   			  , int *pSDKErr
						  )
{
	API_STATUS ret;
	if(pPlan == NULL || pAntList == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}
	/* Check for allowance power setting range */
	if(antCount > ANT_MAX_COUNT || antCount < ANT_MIN_COUNT)
	{
		return API_NUM_OUTOFRANGE;
	}
	/* initialize the read plan */
	ret = TMR_RP_init_simple(pPlan, antCount, pAntList, TMR_TAG_PROTOCOL_GEN2, READPLAN_WEIGHT);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
}

/**************************************************************************
* Function: readTagDisc()
* This function will setup antenna and read from  single antenna a time
* Parameters: TMR_Reader *pReadplan - reader handle
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS readTagDisc( TMR_Reader *pReadplan
							   , TagdevSDKEPCStructDiscoverTags *pTaglist
		  	  	  	  	  	   , int *pSDKErr
		  	  	  	  	  	   )
{
	TMR_Status ret;
	int numofTag = 0;
	TMR_TagReadData trd;
	TagdevSDKEPCStruct *pTagEPC;

	/* Check for null pointer */
	if(pReadplan == NULL || pTaglist == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	pTagEPC = pTaglist->pTagdevSDKEPC;

	ret = TMR_SR_read(pReadplan, gReadTimeout, NULL);
	if(ret != TMR_SUCCESS)
	{
		pTaglist->nTags = 0;
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	while (TMR_SUCCESS == TMR_SR_hasMoreTags(pReadplan))
	{
	    ret = TMR_SR_getNextTag(pReadplan, &trd);
		if(ret != TMR_SUCCESS)
		{
			pTaglist->nTags = 0;
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}

	    /* Build up Tag list */
		pTagEPC->EPCLen = trd.tag.epcByteCount;
		pTagEPC->tagcrc = trd.tag.crc;
		memcpy((char *) pTagEPC->tag, (char *) trd.tag.epc, trd.tag.epcByteCount);
		pTagEPC->phase = trd.phase;
		pTagEPC->antenna = trd.antenna;
		pTagEPC->readCount = trd.readCount;
		pTagEPC->rssi = trd.rssi;
		pTagEPC->frequency = trd.frequency;
		pTagEPC->timelow = trd.timestampLow;
		pTagEPC->timehigh = trd.timestampHigh;
	    numofTag++;
	    pTagEPC++;
	}
	pTaglist->nTags = numofTag;

	ret = TMR_SR_destroy(pReadplan);
    return API_SUCCESS;
}

/**************************************************************************
* Function: readTagSingle()
* This function will setup antenna and read from  single antenna a time
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  TagDevEPCStruct *pTagSingle - only read on tag
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS readTagSingle( TMR_Reader *pReadplan
						, TagdevSDKReadTagSingleSt *pTagSingle
		  	  	  	  	, int *pSDKErr
		  	  	  	  	)
{
	TMR_Status ret;
	TMR_TagReadData trd;

	/* Check for null pointer */
	if(pReadplan == NULL || pTagSingle == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_read(pReadplan, gReadTimeout, NULL);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	if(TMR_SUCCESS == TMR_SR_hasMoreTags(pReadplan))
	{
	    ret = TMR_SR_getNextTag(pReadplan, &trd);
		if(ret != TMR_SUCCESS)
		{
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}

	    /* Only store first found tag epc */
	}
	ret = TMR_SR_destroy(pReadplan);

    return API_SUCCESS;
}

/**************************************************************************
* Function: readTagData()
* This function will setup antenna and read from  single antenna a time
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  unsigned char *pTagData - only read on tag data
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS readTagData( TMR_Reader *pReadplan
							   , unsigned char *pTagData
		  	  	  	  	  	   , int *pSDKErr
		  	  	  	  	  	   )
{
	TMR_Status ret;
	TMR_TagReadData trd;

	/* Check for null pointer */
	if(pReadplan == NULL || pTagData == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_read(pReadplan, gReadTimeout, NULL);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	if(TMR_SUCCESS == TMR_SR_hasMoreTags(pReadplan))
	{
	    ret = TMR_SR_getNextTag(pReadplan, &trd);
		if(ret != TMR_SUCCESS)
		{
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}

	    /* Only store epc data */
		strncpy((char *) pTagData, (const char *) trd.tag.epc, trd.tag.epcByteCount);
	}

	ret = TMR_SR_destroy(pReadplan);
    return API_SUCCESS;
}

/**************************************************************************
* Function: set_Ant_For_Writetag()
* This function will write tag to reader
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  unsigned char *pAntList - Antenna list need to write tag to
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setAntennaForWritetag( TMR_Reader *pReadplan
							   , unsigned char *pAntList
							   , int *pSDKErr
							   )
{
	TMR_Status ret;

	if(pReadplan == NULL || pAntList == NULL)
	{
		return API_NULL_POINTER;
	}

	/* set antenna list before write */
	ret = TMR_paramSet(pReadplan, TMR_PARAM_TAGOP_ANTENNA, pAntList);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

    return API_SUCCESS;
}

/**************************************************************************
* Function: writeTag()
* This function will write tag to reader
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  TMR_TagData *pOldEPC - Old tag
* 			  TMR_TagData *pNewEPC - new tag
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS writeTag( TMR_Reader *pReadplan
				   , TMR_TagData *pOldEPC
				   , TMR_TagData *pNewEPC
				   , int *pSDKErr
				   )
{
	TMR_Status ret;
    TMR_TagOp oldtagop;
    TMR_TagOp newtagop;
    TMR_TagFilter filter;

	/* Check for null pointer */
	if( pReadplan == NULL || pOldEPC == NULL || pNewEPC == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	/* Init setup old tag for GEN2 write Op */
    ret = TMR_TagOp_init_GEN2_WriteTag(&oldtagop, pOldEPC);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}
    ret = TMR_executeTagOp(pReadplan, &oldtagop, NULL, NULL);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	/* Now setup new tag and execute the write Op */
    /* Initialize the new tagop to write the new epc*/
     ret = TMR_TagOp_init_GEN2_WriteTag(&newtagop, pNewEPC);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

    /* Initialize the filter with the original epc of the tag which is set earlier*/
    ret = TMR_TF_init_tag(&filter, pOldEPC);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

    /* Execute the tag operation Gen2 writeTag with select filter applied*/
    ret = TMR_executeTagOp(pReadplan, &newtagop, &filter, NULL);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

	return API_SUCCESS;
}

/**************************************************************************
* Function: showVersion()
* This function will get all version types
* Parameters: TMR_Reader *pReadplan - reader handle
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS showVersion(TMR_Reader *pReadplan
					   , BootldevSDKVersionStruct *pVersion
		  	  	  	 , int *pSDKErr
		  	  	  	 )
{
    TMR_Status ret;
    unsigned short productGroupID;
    unsigned short productID;
    TMR_String str;
    char string[MODEL_MAX_STRING_LEN];
    char *src, *dst;
    int i = 0;
    str.value = string;
    str.max = MODEL_MAX_STRING_LEN;

    /* Check for null pointer */
    if( pReadplan == NULL || pSDKErr == NULL )
    {
        return API_NULL_POINTER;
    }

    /* Get hardware version */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_VERSION_HARDWARE, &str);
    if(ret != TMR_SUCCESS)
    {
    	*pSDKErr = ret;
    	return API_SDK_CALL_FAILURE;
    }
    else /* Copy HW version string to return */
    {
    	strcpy(pVersion->vHardw, str.value);
    }

    /* Get version of serial */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_VERSION_SERIAL, &str);
    if(ret != TMR_SUCCESS)
    {
 	*pSDKErr = ret;
 	return API_SDK_CALL_FAILURE;
    }

    /* Get version of model */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_VERSION_MODEL, &str);
    if(ret != TMR_SUCCESS)
    {
 	*pSDKErr = ret;
 	return API_SDK_CALL_FAILURE;
    }

    /* Get version of software */
    ret = TMR_paramGet(pReadplan,TMR_PARAM_VERSION_SOFTWARE, &str);
    if(ret != TMR_SUCCESS)
    {
 	*pSDKErr = ret;
 	return API_SDK_CALL_FAILURE;
    }
    else
    {
	    src = (char *) str.value;
	    dst = (char *) pVersion->vFirmware;
	    while(*src != '-' && i++ < MODEL_MAX_STRING_LEN)
	    {
	        *dst++ = *src++;
	    }
	    *dst = '\0';
	    src++; /* Move pass '-' */
	    i = 0;
	    dst = (char *) pVersion->FirmwareDate;
	    while(*src != '-' && i++ < MODEL_MAX_STRING_LEN)
	    {
	        *dst++ = *src++;
	    }
	    *dst = '\0';
	    src++; /* Move pass '-' */
	    i = 0;
	    dst = (char *) pVersion->vBootl;
	    while(*src != '\0' && i++ < MODEL_MAX_STRING_LEN)
	    {
	        *dst++ = *src++;
	    }
	    *dst = '\0';
    }
    /* Get version of productID */
    ret = TMR_paramGet(pReadplan,TMR_PARAM_PRODUCT_ID, &productID);
    if(ret != TMR_SUCCESS)
    {
 	*pSDKErr = ret;
 	return API_SDK_CALL_FAILURE;
    }

    /* Get version of productGroupID */
    ret = TMR_paramGet(pReadplan,TMR_PARAM_PRODUCT_GROUP_ID, &productGroupID);
    if(ret != TMR_SUCCESS)
    {
 	*pSDKErr = ret;
 	return API_SDK_CALL_FAILURE;
    }
    /* Clean the rest entries */
    dst = (char *) pVersion->HardwareVname;
    *dst = '\0';
    strcpy(dst, "HW_VERID_2_0");
    dst = (char *) pVersion->suppProto;
    strcpy(dst, "0.0.0.20");
    dst = (char *) pVersion->HardwareDescription;
    *dst = '\0';

    return API_SUCCESS;
}

/**************************************************************************
* Function: getHWVersion()
* This function will get all version types
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  TMR_String * pHWVersion
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getHWVersion(TMR_Reader *pReadplan
					   , SDKGetHwVerStUser *pHWVersion
					   , int *pSDKErr
		  	  	  	   )
{
	TMR_Status ret;
	TMR_String str;
	char string[MODEL_MAX_STRING_LEN];
	str.value = string;
	str.max = MODEL_MAX_STRING_LEN;

	/* Check for null pointer */
	if( pReadplan == NULL || pHWVersion == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}
    /* Get hardware version */
    ret = TMR_paramGet(pReadplan, TMR_PARAM_VERSION_HARDWARE, &str);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}
 	else
 	{
 		strcpy(pHWVersion->data , str.value);
 		pHWVersion->len = strlen(str.value);
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getCurrentProg()
* This function will get all version types
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  BootlSDKProgramCode *pProgramCode
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* **************************************************************************/
API_STATUS getCurrentProg(TMR_Reader *pReadplan
				  		  , BootlSDKProgramCode *pProgramCode
				  		  , int *pSDKErr
		  	  	  	  	  )
{
	TMR_Status ret;
	unsigned char program;

	/* Check for null pointer */
	if( pReadplan == NULL || pProgramCode == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_cmdGetCurrentProgram(pReadplan, &program);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}
 	else
 	{
 		pProgramCode->programCode = program;
 		strcpy(pProgramCode->description, M6E_PROG_DES);
	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: setGPIO()
* This function will set GPIOs
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  int gpioPin - GPIO pin (1,2,3,4)
* 			  int status - 1/0 true/false
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setGPIO(TMR_Reader *pReadplan
				  , int gpioPin
				  , int status
		  	  	  , int *pSDKErr
		  	  	  )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	/* Check for setting range */
	if( (gpioPin < GPIO_MIN_PIN || gpioPin > GPIO_MAX_PIN )
	  ||(status < 0 || status > 1 ))
	{
		return API_NUM_OUTOFRANGE;
	}

	ret = TMR_SR_cmdSetGPIO(pReadplan, gpioPin, status);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;

}
/**************************************************************************
* Function: getGPIO()
* This function will get GPIOs
* Parameters: TMR_Reader *pReadplan - reader handle
* 			  int gpioPin - GPIO pin (1,2,3,4)
* 			  int status - 1/0 true/false
*             int *pSDKErr  - SDK errors
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getGPIO(TMR_Reader *pReadplan
				  , SetapplSDKGetGpioInputSt *pSetapplSDKGetGpioInputSt
		  	  	  , int *pSDKErr
		  	  	  )
{
	TMR_Status ret;
	TMR_GpioPin pinStates[4];
	int numPins;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL || pSetapplSDKGetGpioInputSt == NULL)
	{
		return API_NULL_POINTER;
	}
	numPins = 4;
	ret = TMR_SR_cmdGetGPIO(pReadplan, (unsigned char *) &numPins, pinStates);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	/* copy information for return */
	pSetapplSDKGetGpioInputSt->GPIO1.id = 1;
	pSetapplSDKGetGpioInputSt->GPIO1.high = pinStates[1].high;
	pSetapplSDKGetGpioInputSt->GPIO1.output = pinStates[1].output;

	pSetapplSDKGetGpioInputSt->GPIO2.id = 2;
	pSetapplSDKGetGpioInputSt->GPIO2.high = pinStates[2].high;
	pSetapplSDKGetGpioInputSt->GPIO2.output = pinStates[2].output;

	pSetapplSDKGetGpioInputSt->GPIO3.id = 3;
	pSetapplSDKGetGpioInputSt->GPIO3.high = pinStates[3].high;
	pSetapplSDKGetGpioInputSt->GPIO3.output = pinStates[3].output;

	pSetapplSDKGetGpioInputSt->GPIO4.id = 4;
	pSetapplSDKGetGpioInputSt->GPIO4.high = pinStates[4].high;
	pSetapplSDKGetGpioInputSt->GPIO4.output = pinStates[4].output;

 	return API_SUCCESS;

}

/**************************************************************************
* Function: setReadTimeout()
* This function will setup antenna and read from  single antenna a time
* Parameters: int readTimeout - set universal read timeout for all read plan
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setReadTimeout(int readTimeout)
{
	gReadTimeout = readTimeout;
	return API_SUCCESS;
}

/**************************************************************************
* Function: getReadTimeout()
* This function will setup antenna and read from  single antenna a time
* Parameters: int readTimeout - set universal read timeout for all read plan
*
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getReadTimeout(int *pReadTimeout)
{
	/* Check for null pointer */
	if( pReadTimeout == NULL)
	{
		return API_NULL_POINTER;
	}

	*pReadTimeout = gReadTimeout;

	return API_SUCCESS;
}

/**************************************************************************
* Function: cmdBootFirmware()
* This function will send command boot firmware
* Parameters: TMR_Reader *pReadplan - read handle
	      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS cmdBootFirmware( TMR_Reader *pReadplan
			  , BootldevSDKVersionStruct *pVersion
			  , int *pSDKErr
			  )
{
	TMR_Status ret;
	unsigned char program;

	/* Check for null pointer */
	if( pReadplan == NULL || pVersion == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_cmdGetCurrentProgram(pReadplan, &program);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}
	
	/* Check and only boot at certain status */
	if((program & 0x3) == 1)	
	{
		ret = TMR_SR_cmdBootFirmware((TMR_Reader *) pReadplan);
 		if(ret != TMR_SUCCESS)
 		{
 			*pSDKErr = ret;
 			return API_SDK_CALL_FAILURE;
 		}
	}

	ret = showVersion(pReadplan, pVersion, pSDKErr);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}	

	return API_SUCCESS;
}

/**************************************************************************
* Function: cmdBootBootloader()
* This function will send command boot at boot loader
* Parameters: TMR_Reader *pReadplan - read handle
	      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS cmdBootBootloader( TMR_Reader *pReadplan
			  	  	  	    , int *pSDKErr
			  	  	  	  	)
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	/* Send command boot at bootloader request */
	ret = TMR_SR_cmdBootBootloader(pReadplan);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: GetProtocolConfiguration()
* This function will get protocol configuration
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_GEN2_Bap *getBapParams - configuration storage
* 			  TMR_Param key - protocol key. Ex TMR_TAG_PROTOCOL_GEN2
*		      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getProtocolConfiguration( TMR_Reader *pReadplan
					, SDKGetProtoConfigUserSt * pProtoConfigUser
					, int *pSDKErr
			  	  	  	  			)
{
	TMR_Status ret;
	TMR_SR_ProtocolConfiguration protokey;
	int value;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL || pProtoConfigUser == NULL)
	{
		return API_NULL_POINTER;
	}
	/* Translate Configuration STACK value to SDK value */
	/* SESSSION */
	if(pProtoConfigUser->id > 0 && pProtoConfigUser->id <= 4)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_SESSION;
	}
	/* Target search A, B, AB, BA */
	else if(pProtoConfigUser->id > 4 && pProtoConfigUser->id <= 8)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARGET;
	}
	/* Miller value */
	else if(pProtoConfigUser->id > 8 && pProtoConfigUser->id <= 11)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TAGENCODING;
	}
	/* Q value */
	else if(pProtoConfigUser->id > 11 && pProtoConfigUser->id <= 13)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_Q;
	}

	protokey.protocol = TMR_TAG_PROTOCOL_GEN2;
	ret = TMR_SR_cmdGetProtocolConfiguration(pReadplan, protokey.protocol,
					protokey, &value);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

	if(value > 0)
	{
		if(pProtoConfigUser->id > 0 && pProtoConfigUser->id <= 4 && pProtoConfigUser->id == value)
		{
			pProtoConfigUser->idset = pProtoConfigUser->id;
			pProtoConfigUser->value = value;
		}
		else if(pProtoConfigUser->id > 4 && pProtoConfigUser->id <= 8 && (pProtoConfigUser->id - 4) == value)
		{
			pProtoConfigUser->idset = pProtoConfigUser->id;
			pProtoConfigUser->value = value;
		}
		else if(pProtoConfigUser->id > 8 && pProtoConfigUser->id <= 11 && (pProtoConfigUser->id - 8) == value)
		{
			pProtoConfigUser->idset = pProtoConfigUser->id;
			pProtoConfigUser->value = value;
		}
		else if(pProtoConfigUser->id > 11 && pProtoConfigUser->id <= 13 && (pProtoConfigUser->id - 11) == value)
		{
			pProtoConfigUser->idset = pProtoConfigUser->id;
			pProtoConfigUser->value = value;
		}
	}
	else
	{
		pProtoConfigUser->idset = 0;
		pProtoConfigUser->value = value;
	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: SetProtocolConfiguration()
* This function will get protocol configuration
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_GEN2_Bap *getBapParams - configuration to set to reader
* 			  TMR_Param key - protocol key. Ex TMR_TAG_PROTOCOL_GEN2
*		      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setProtocolConfiguration( TMR_Reader *pReadplan
				, SDKSetProtoConfigStUser *pProtoConfigUser
				, int *pSDKErr
			 	)
{
	TMR_Status ret;
	TMR_SR_ProtocolConfiguration protokey;
	int value;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL || pProtoConfigUser == NULL)
	{
		return API_NULL_POINTER;
	}
	/* Translate Configuration STACK value to SDK value */
	/* SESSSION */
	if(pProtoConfigUser->id > 0 && pProtoConfigUser->id <= 4)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_SESSION;
		value = pProtoConfigUser->id;
	}
	/* Target search A, B, AB, BA */
	else if(pProtoConfigUser->id > 4 && pProtoConfigUser->id <= 8)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TARGET;
		value = pProtoConfigUser->id - 4;
	}
	/* Miller value */
	else if(pProtoConfigUser->id > 8 && pProtoConfigUser->id <= 11)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_TAGENCODING;
		value = pProtoConfigUser->id - 8;
	}
	/* Q value */
	else if(pProtoConfigUser->id > 11 && pProtoConfigUser->id <= 13)
	{
		protokey.u.gen2 = TMR_SR_GEN2_CONFIGURATION_Q;
		value = pProtoConfigUser->id - 11;
	}
	protokey.protocol = TMR_TAG_PROTOCOL_GEN2;
	
	ret = TMR_SR_cmdSetProtocolConfiguration(pReadplan, protokey.protocol,
					protokey, &value);

 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}


/**************************************************************************
* Function: clearTagBuffer()
* This function will clear tag buffer
* Parameters: TMR_Reader *pReadplan - read handle
	      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS clearTagBuffer( TMR_Reader *pReadplan
			  	  	  	  , int *pSDKErr
			  	  	  	  )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_cmdClearTagBuffer(pReadplan);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: showMetadata()
* This function will send command boot firmware
* Parameters: TMR_Reader *pReadplan - read handle
	      int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS showMetadata( TMR_Reader *pReadplan
			  	  	  	  , int *pSDKErr
			  	  	  	  )
{
	TMR_Status ret;
	TMR_TagReadData trd;

	/* Check for null pointer */
	if(pReadplan == NULL || pSDKErr == NULL)
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_read(pReadplan, gReadTimeout, NULL);
	if(ret != TMR_SUCCESS)
	{
		*pSDKErr = ret;
		return API_SDK_CALL_FAILURE;
	}

	if(TMR_SUCCESS == TMR_SR_hasMoreTags(pReadplan))
	{
	    ret = TMR_SR_getNextTag(pReadplan, &trd);
		if(ret != TMR_SUCCESS)
		{
			*pSDKErr = ret;
			return API_SDK_CALL_FAILURE;
		}

	    /* Show all metadata of one tag */
		printf("Read Count : %d\n", trd.readCount);
		printf("RSSI : %d\n", trd.rssi);
		printf("Antenna ID : %d\n", trd.antenna);
		printf("Frequency : %d\n", trd.frequency);
		printf("Phase : %d\n", trd.phase);
		printf("Protocol : %d\n", trd.tag.protocol);
        {
            TMR_GpioPin state[16];
            unsigned char i, stateCount = numberof(state);
            ret = TMR_gpiGet(pReadplan, &stateCount, state);
            if (TMR_SUCCESS != ret)
            {
    			*pSDKErr = ret;
    			return API_SDK_CALL_FAILURE;
            }
            printf("GPIO stateCount: %d\n", stateCount);
            for (i = 0 ; i < stateCount ; i++)
            {
                printf("Pin %d: %s\n", state[i].id, state[i].high ? "High" : "Low");
            }
        }

        if(trd.u.gen2.target == TMR_GEN2_TARGET_A)
        {
        	printf("Target A\n");
        }
        else if(trd.u.gen2.target == TMR_GEN2_TARGET_B)
        {
        	printf("Target B\n");
        }
        else
        {
        	printf("Target Unknown\n");
        }
	}


    return API_SUCCESS;
}

/**************************************************************************
* Function: setReadTxPower()
* This function will send command boot at boot loader
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int setPower - Power value set to antenna to read
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setReadTxPower( TMR_Reader *pReadplan
						 , int setPower
			  	  	  	 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_cmdSetReadTxPower(pReadplan, setPower);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: setWriteTxPower()
* This function will send command boot at boot loader
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int setPower - Power value set to antenna to read
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setWriteTxPower( TMR_Reader *pReadplan
						 , int setPower
			  	  	  	 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_SR_cmdSetWriteTxPower(pReadplan, setPower);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: setFREQHPTBL()
* This function will set frequency hop table for available region
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int setPower - Power value set to antenna to read
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setFREQHopTable( TMR_Reader *pReadplan
						 , int *setPower
			  	  	  	 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}
	ret = TMR_paramSet(pReadplan, TMR_PARAM_REGION_HOPTABLE, setPower);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getFREQHPTBL()
* This function will set frequency hop table for available region
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int setPower - Power value set to antenna to read
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getFREQHopTable( TMR_Reader *pReadplan
						 , int *setPower
			  	  	  	 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;

	/* Check for null pointer */
	if( pReadplan == NULL || pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}
	ret = TMR_paramSet(pReadplan, TMR_PARAM_REGION_HOPTABLE, setPower);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getReaderTemp()
* This function will get current Reader Temperature
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int *getTemp - store reader temperature
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getReaderTemperature( TMR_Reader *pReadplan
						 	 	, unsigned char *getTemp
						 	 	, int *pSDKErr
			  	  	  	 	  	)
{
	TMR_Status ret;
	TMR_Reader_StatsValues stats;
	int setFlag;

	if( pReadplan == NULL || getTemp == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	setFlag = TMR_READER_STATS_FLAG_TEMPERATURE;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	TMR_STATS_init(&stats);

 	ret = TMR_paramGet(pReadplan, TMR_PARAM_READER_STATS, &stats);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	if(TMR_READER_STATS_FLAG_TEMPERATURE & stats.valid)
 	{
 		/* store temperature in Celcius to return to caller */
 		sprintf(getTemp, "%x",stats.temperature);
 	}
 	return API_SUCCESS;
}

/**************************************************************************
* Function: getCurAntenna()
* This function will get current connected Antenna
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int *curAnt - store reader temperature
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getCurAntenna( TMR_Reader *pReadplan
						 , int *curAnt
						 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;
	TMR_Reader_StatsValues stats;
	int setFlag;

	if( pReadplan == NULL || curAnt == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	setFlag = TMR_READER_STATS_FLAG_ANTENNA_PORTS;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	TMR_STATS_init(&stats);

 	ret = TMR_paramGet(pReadplan, TMR_PARAM_READER_STATS, &stats);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	if(TMR_READER_STATS_FLAG_ANTENNA_PORTS & stats.valid)
 	{
 		/* store antenna to return to caller */
 		*curAnt = stats.antenna;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getCurFreq()
* This function will get current communicate Frequency
* Parameters: TMR_Reader *pReadplan - read handle
* 			  int *curFreg - store current frequency
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getCurFreq( TMR_Reader *pReadplan
						 , int *curFreq
						 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;
	TMR_Reader_StatsValues stats;
	int setFlag;

	if( pReadplan == NULL || curFreq == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	setFlag = TMR_READER_STATS_FLAG_FREQUENCY;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	TMR_STATS_init(&stats);

 	ret = TMR_paramGet(pReadplan, TMR_PARAM_READER_STATS, &stats);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	if(TMR_READER_STATS_FLAG_FREQUENCY & stats.valid)
 	{
 		/* store Frequency hoptable to return to caller */
 		*curFreq = stats.frequency;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getCurPROTO()
* This function will get current communicate protocol
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_TagProtocol *curProto - store current protocol
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getCurProto( TMR_Reader *pReadplan
						 , SetapplSDKCurProtoSt *pCurProto
						 , int *pSDKErr
			  	  	  	 )
{
	TMR_Status ret;
	TMR_Reader_StatsValues stats;
	int setFlag;

	if( pReadplan == NULL || pCurProto == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	setFlag = TMR_READER_STATS_FLAG_PROTOCOL;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_READER_STATS_ENABLE, &setFlag);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	TMR_STATS_init(&stats);

 	ret = TMR_paramGet(pReadplan, TMR_PARAM_READER_STATS, &stats);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	if(TMR_READER_STATS_FLAG_PROTOCOL & stats.valid)
 	{
 		/* store current protocol to return to caller */
 		strcpy(pCurProto->desc,tag_proto[stats.protocol]);
 		pCurProto->protovalue = stats.protocol;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getUserMode()
* This function will get current user mode
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_SR_UserMode *UserMode - User mode
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getUserMode( TMR_Reader *pReadplan
					   , SDKSetUserModeStruct *pUserMode
					   , int *pSDKErr
			  	  	   )
{
	TMR_Status ret;
	TMR_SR_UserMode mode;

	if( pReadplan == NULL || pUserMode == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_paramGet(pReadplan, TMR_PARAM_USERMODE, &mode);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}
 	else
 	{
 		pUserMode->value = mode;
 		strcpy(pUserMode->setting,userModes[mode]);
 	}
 	return API_SUCCESS;
}

/**************************************************************************
* Function: setUserMode()
* This function will set current user mode
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_SR_UserMode *UserMode - User mode
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setUserMode( TMR_Reader *pReadplan
					   , TMR_SR_UserMode *pUserMode
					   , int *pSDKErr
			  	  	   )
{
	TMR_Status ret;
	TMR_SR_UserMode mode;
	int i;

	if( pReadplan == NULL || pUserMode == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	/* Get mode from the list */
    i = listid(userModes, numberof(userModes), (const char *) pUserMode);
    if (i == -1)
    {
    	return API_CANT_PARSE_VALUE;
    }
    mode = i;
    ret = TMR_paramSet(pReadplan, TMR_PARAM_USERMODE, &mode);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: getPowerMode()
* This function will get current power mode
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_SR_PowerMode *pPowerMode - Power mode
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS getPowerMode( TMR_Reader *pReadplan
					   , SDKSetPowerModeStruct *pPowerMode
					   , int *pSDKErr
			  	  	   )
{
	TMR_Status ret;
	TMR_SR_PowerMode mode;

	if( pReadplan == NULL || pPowerMode == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

	ret = TMR_paramGet(pReadplan, TMR_PARAM_POWERMODE, &mode);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}
 	else
 	{
 		pPowerMode->value = mode;
 		strcpy(pPowerMode->mode,powerModes[mode]);
 	}
 	return API_SUCCESS;
}

/**************************************************************************
* Function: setPowerMode()
* This function will get current power mode
* Parameters: TMR_Reader *pReadplan - read handle
* 			  TMR_SR_PowerMode *pPowerMode - Power mode
	      	  int *pSDKErr - SDK errors
* Return:
* API_STATUS
* *************************************************************************/
API_STATUS setPowerMode( TMR_Reader *pReadplan
 		   , unsigned char *pPowerMode
					   , int *pSDKErr
			  	  	   )
{
	TMR_Status ret;
	TMR_SR_PowerMode mode;
	int i;

	if( pReadplan == NULL || pPowerMode == NULL ||pSDKErr == NULL )
	{
		return API_NULL_POINTER;
	}

    i = listid(powerModes, numberof(powerModes), (const char *) pPowerMode);
    if (i == -1)
    {
    	return API_CANT_PARSE_VALUE;


    }
    mode = i;
	ret = TMR_paramSet(pReadplan, TMR_PARAM_POWERMODE, &mode);
 	if(ret != TMR_SUCCESS)
 	{
 		*pSDKErr = ret;
 		return API_SDK_CALL_FAILURE;
 	}

 	return API_SUCCESS;
}

/**************************************************************************
* Function: createDefaultReadplan()
* This function will create Readplan on generic setting
* Return:
* TMR_Reader *pReadplan
* *************************************************************************/

int getReadplan(TMR_Reader *pReadplan)
{
	TMR_Status status;
	int SDKErr;
	int antCount = 1;
	unsigned char antList = 1;
	unsigned char *pAntList;
	TMR_ReadPlan plan;
	TMR_TRD_MetadataFlag metadata = TMR_TRD_METADATA_FLAG_ALL;

	if(pReadplan <= 0)
	{
		pReadplan = malloc(sizeof(TMR_Reader));
		if(pReadplan <= 0)
			return -1;	
	}
	
	/* Open COM port and hook to Read plan */
	status = m6e_Port_Open(pReadplan, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Connect COM port to Reader */
	status = m6e_Port_Connect(pReadplan, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Set Antenna power */
	status = m6esetPower(pReadplan, 3000, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Set Region for Antenna */
	status = setRegion(pReadplan, TMR_REGION_NONE, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Check Antenna support types */
	status = checkSupportAntenna(pReadplan, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Commit to Read plan and setup is complete. Ready to read tag */
	pAntList = &antList;
	status = setupAntenna(&plan, 1, pAntList, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}
	status = Commit_ReadPlan(pReadplan, &plan, &metadata, &SDKErr);
	if(status > API_SUCCESS)
	{
		return -1;
	}

	/* Set default read time out to 500 ms */
	if(gReadTimeout != READER_READ_TIMEOUT)
	{
		gReadTimeout = READER_READ_TIMEOUT;
	}

	return 0;
}

