

#ifndef __INCSYSMSGh
#define __INCSYSMSGh

#ifdef __cplusplus
extern "C"  {
#endif

#include <sys/msg.h>
#include <sys/ipc.h>

// Define each module message key value
#define DBASE_MSGQ_KEY		0x00001111
#define WD_MSGQ_KEY		    0x00002222
#define READER_MSGQ_KEY		0x00003333
#define REMOTEM_MSGQ_KEY	0x00004444
#define CTRL_MSGQ_KEY		0x00005555

#define SYS_MSG_TYPE 0xf1a322

#define MSG_TIMEOUT 5 /* Try 5 second before read timeout */

typedef enum {
    READER_MODULE_ID,
    DBASE_MODULE_ID,
	REMOTEM_MODULE_ID,
	CTRL_MODULE_ID,
    UNKNOWN_MODULE_ID
} MODULE_ID_ENUM;

typedef enum {
    MSG_BARK,
    MSG_TAG,
    MSG_TAG_READ,
	MSG_REBOOT,
	MSG_TAG_INSERT,
	MSG_TAG_REMOVE,
	MSG_TAG_MONITOR,
    MSG_INVALID
} MSG_TYPE_ENUM;

typedef struct {
    MODULE_ID_ENUM     moduleID;     /* sender's module ID */
    MSG_TYPE_ENUM      subType;      /* message command */
    unsigned long	   msgtime;	 /* For watchdog time keeper */
    int			   data;
} GENERIC_MSG_HEADER_T;

///* generic container that the above common messages will be delivered in  */
typedef struct {
    GENERIC_MSG_HEADER_T header;
} READER_MSG_T;

typedef struct {
    GENERIC_MSG_HEADER_T header;
	unsigned char tag[128];
    int epcLen;
    int antenna;
    int rssi;
} DBASE_MSG_T;

typedef struct {
    GENERIC_MSG_HEADER_T header;
	unsigned char tag[128];
    int epcLen;
    int antenna;
    int rssi;
} TAGS_MSG_T;

typedef struct {
    GENERIC_MSG_HEADER_T header;
} REMOTEM_MSG_T;

typedef struct {
    GENERIC_MSG_HEADER_T header;
} CTRL_MSG_T;

typedef enum {
    DBG_ERROR = 0,
	DBG_DBG = 0,
    DBG_WARNING,
    DBG_EVENT,
    DBG_INFO,
    DBG_DETAILED,
    DBG_ALL
} LOG_LEVEL_ENUM_T;

typedef struct {
    unsigned int module_id;
    int     timer;
    int     reboot;
} WD_RESPONSE_T;

int create_msg(key_t p_key);
int send_msg(int msgid, void *msg, int size, int timeout);
int recv_msg(int msgid, void *msg, int size, int timeout);
void send_dog_bark(int);
int logging(int level, char *logstr, ...);
int open_msg(key_t key);

extern char *module_list[];
extern char *modname[];


#endif
