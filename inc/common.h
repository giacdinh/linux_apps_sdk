typedef struct TagDevEPCStruct_tag {
    unsigned short EPCLen;
    unsigned short PCWord;
    unsigned char  tag[63];
    unsigned short tagcrc;
    unsigned char  tagPresent;
    unsigned char antid;
    unsigned int rssi;
}TagDevEPCStruct;

typedef enum _API_STATUS {
    API_SUCCESS         = 0,
    API_SDK_CALL_FAILURE,
    API_NULL_POINTER,
    API_NUM_OUTOFRANGE,
    API_ANTENNA_NOTSUPPORT,
    API_BAD_VALUE
} API_STATUS;


typedef enum _ANT_POWER {
	POWER_LEVEL_1 = 1500,
	POWER_LEVEL_2 = 1750,
	POWER_LEVEL_3 = 2000,
	POWER_LEVEL_4 = 2250,
	POWER_LEVEL_5 = 2500,
	POWER_LEVEL_6 = 2750,
	POWER_LEVEL_7 = 3000
} power_level;
	
