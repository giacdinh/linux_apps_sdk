
#define SET_TAG_DEV_MAX_ST_SIZE 100
#define SET_APPL_MAX_ST_SIZE 50
#define MAX_IO_MESSAGE_SND_SIZE 250
#define MAX_IO_MESSAGE_RCV_SIZE 248
#define SET_APPL_MAX_RD_CNF_SIZE 100
#define ANT_DEFAULT_POWER 3000
#define READER_READ_TIMEOUT 200


typedef struct TagdevSDKEPCStruct_tag
{   
    unsigned short EPCLen;
    unsigned short PCWord;
    unsigned char  tag[63];
    unsigned short tagcrc;
    unsigned char tagPresent;
    unsigned short phase;
    unsigned char antenna;
    unsigned char readCount;
    unsigned int rssi;
    unsigned int frequency;
    unsigned int timelow;
    unsigned int timehigh;
}TagdevSDKEPCStruct;

typedef struct TagdevSDKEPCStructDiscoverTags_tag
{
    int nTags;
/*    TagdevSDKReadTagMultiStruct *pTagdevSDKReadTagMultiStruct; */
    TagdevSDKEPCStruct *pTagdevSDKEPC;
    int timeout;
    int readtimeout;
    int RXAnt;
    int TXAnt;
    char region[50];

}TagdevSDKEPCStructDiscoverTags;

typedef enum _API_STATUS {
    API_SUCCESS         = 0,
    API_SDK_CALL_FAILURE,
    API_NULL_POINTER,
    API_NUM_OUTOFRANGE,
    API_ANTENNA_NOTSUPPORT,
    API_CANT_PARSE_VALUE,
    API_BAD_VALUE
} API_STATUS;

typedef struct SDKSetCurRegionStUser_tag
{
    unsigned char region[SET_TAG_DEV_MAX_ST_SIZE];
    unsigned char lbt;
}SDKSetCurRegionStUser;

typedef struct SDKSetPowerModeStruct_tag
{
    unsigned char mode[SET_APPL_MAX_ST_SIZE];
    unsigned char value;
}SDKSetPowerModeStruct;

typedef struct SDKSetUserModeStruct_tag
{
    unsigned char setting[SET_APPL_MAX_ST_SIZE];
    unsigned char value;
}SDKSetUserModeStruct;

typedef struct SDKGetReaderConfigRdInfo_tag
{
    unsigned char id;
    unsigned char Desc[SET_APPL_MAX_RD_CNF_SIZE];
}SDKGetReaderConfigRdInfo;

typedef struct SDKGetReaderConfigRdSt_tag
{
    SDKGetReaderConfigRdInfo  TGetReaderConfigRdInfo[SET_APPL_MAX_RD_CNF_SIZE];
    unsigned char len;
}SDKGetReaderConfigRdSt;

typedef struct BootldevSDKVersion_Tag
{
    unsigned char vBootl[100];
    unsigned char vHardw[100];
    unsigned char HardwareVname[100];
    unsigned char HardwareDescription[100];
    unsigned char vFirmware[100];
    unsigned char FirmwareDate[100];
    unsigned char suppProto[100];
}BootldevSDKVersionStruct;

typedef struct SDKGetHwVerStUser_tag
{
    unsigned char data[MAX_IO_MESSAGE_RCV_SIZE+2];
    unsigned char len;
}SDKGetHwVerStUser;

typedef struct SDKSetPwrTxAntPwr_tag
{
    unsigned char TxAnt;
    unsigned short ReadPwr;
    unsigned short WritePwr;
}SDKSetPwrTxAntPwr;

typedef struct SetapplSDKCurProtoSt_tag
{
    unsigned char desc[SET_APPL_MAX_ST_SIZE];
    unsigned short protovalue;
}SetapplSDKCurProtoSt;

typedef struct SetapplSDKGetAvailReqUserSt_tag
{
    unsigned char region[SET_APPL_MAX_ST_SIZE][50];
    unsigned char len;
}SetapplSDKGetAvailReqUserSt;

typedef struct BootlSDKProgramCode_tag
{
    unsigned char programCode;
    unsigned char description[100];
}BootlSDKProgramCode;

typedef struct SDKGetReaderProtoInfo_tag
{
    unsigned char id;
    unsigned char Desc[SET_APPL_MAX_RD_CNF_SIZE];
}SDKGetReaderProtoInfo;

typedef struct  SDKGetReaderProtoInfoSt_tag
{
    SDKGetReaderProtoInfo  TGetReaderProtoInfo[SET_APPL_MAX_RD_CNF_SIZE];
    unsigned char len;
}SDKGetReaderProtoInfoSt;

typedef struct TagdevSDKReadTagSingleStruct_tag
{
    unsigned char id;
    unsigned int address;
    unsigned char datalen;
    unsigned char *data;
    unsigned char *key;
    unsigned char keylen;
}TagdevSDKReadTagSingleSt;

typedef struct SetapplSDKGPIOSt_tag
{
    unsigned char id;
    bool high;
    bool output;
}SetapplSDKGPIOSt;

typedef struct SetapplSDKGetGpioInputSt_tag
{
    SetapplSDKGPIOSt GPIO1;
    SetapplSDKGPIOSt GPIO2;
    SetapplSDKGPIOSt GPIO3;
    SetapplSDKGPIOSt GPIO4;
}SetapplSDKGetGpioInputSt;

typedef struct SDKGetProtoConfigUserSt_tag
{
    unsigned char id;
    unsigned char idset;
    unsigned char value;
}SDKGetProtoConfigUserSt;

typedef struct SDKSetProtoConfigStUser_tag
{
    unsigned char id;
    unsigned char value;
}SDKSetProtoConfigStUser;

API_STATUS m6e_Port_Open( TMR_Reader *pReadplan , int *pSDKErr);
API_STATUS m6e_Port_Connect(TMR_Reader *pReadplan , int *pSDKErr);
API_STATUS m6esetPower( TMR_Reader *pReadplan, int setreadpower, int *pSDKErr);
API_STATUS setRegion( TMR_Reader *pReadplan, TMR_Region region, int *pSDKErr);
API_STATUS checkSupportAntenna( TMR_Reader *pReadplan, int *pSDKErr);
API_STATUS setupAntenna( TMR_ReadPlan * pPlan, int antCount, unsigned char *pAntList, int *pSDKErr);
API_STATUS Commit_ReadPlan( TMR_Reader *pReadplan, TMR_ReadPlan * pPlan, TMR_TRD_MetadataFlag * metadata, int *pSDKErr);
API_STATUS readTagDisc( TMR_Reader *pReadplan, TagdevSDKEPCStructDiscoverTags *pTaglist, int *pSDKErr);







