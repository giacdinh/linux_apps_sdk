#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys_msg.h>
#include <unistd.h>
#include <pthread.h>

void *ctrl_dog_bark_task();
void ctrl_msg_handler(CTRL_MSG_T *Incoming);

int ctrl_main_task()
{
	int result = -1;
	pthread_t ctrl_wd_id = -1;

    CTRL_MSG_T ctrl_msg;
	logging(DBG_INFO, "Create controller message\n");
	int msgid = create_msg(CTRL_MSGQ_KEY);
	
	if(msgid < 0)
	{
		logging(DBG_ERROR,"Failed to create controller message queue\n");
		return 0;
	}	

	/* Create thread for DBASE WD response */
	if(ctrl_wd_id == -1)
	{
		result = pthread_create(&ctrl_wd_id, NULL, ctrl_dog_bark_task, NULL);
        if(result == 0)
	        logging(DBG_DETAILED, "Starting controller dog bark thread.\n");
        else
            logging(DBG_ERROR, "CTRL WD thread launch failed\n");
	}

	while(1) {
        recv_msg(msgid, (void *) &ctrl_msg, sizeof(CTRL_MSG_T), MSG_TIMEOUT);
        ctrl_msg_handler((CTRL_MSG_T *) &ctrl_msg);
        usleep(10000);
	}
}

void *ctrl_dog_bark_task()
{
    while(1) {
        send_dog_bark(CTRL_MODULE_ID);
        sleep(1);
    }
}

void ctrl_msg_handler(CTRL_MSG_T *Incoming)
{
	CTRL_MSG_T *ctrl_msg;
    logging(DBG_INFO, "**** Incoming->modname: %s ****\n", modname[Incoming->header.moduleID]);
	
    switch(Incoming->header.subType)
    {
        default:
            logging(DBG_INFO, "%s: Unknown message type %d\n",__FUNCTION__, Incoming->header.subType);
            break;
    }
}

