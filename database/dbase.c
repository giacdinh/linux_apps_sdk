#include <mysql/mysql.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys_msg.h>
#include <unistd.h>
#include <pthread.h>

int dbase_init(MYSQL *dbase);
int dbase_create_table(MYSQL *dbase, char *tablename);
int dbase_sys_curtime(char *);
int dbase_insert_tag(MYSQL *dbase, char *tablename, char *tag, int antid, int rssi, int epcLen);
void dbase_msg_handler(DBASE_MSG_T *Incoming, MYSQL *dbase, char *tablename);
int dbase_check_table_exist(MYSQL *dbase, char *tablename);
void *dbase_dog_bark_task();
int dbase_check_existed_tag(MYSQL *dbase, char *tablename, char *tag);
int put_dash_to_tag(char *source, char *dash_tag, int epcLen);

#define TABLE_NAME	"reader_giacTable"

int dbase_main_task()
{
	MYSQL *dbase;
//	MYSQL_RES result, *presult;
	int result = -1;
	pthread_t dbase_wd_id = -1;

    DBASE_MSG_T dbase_msg;
	logging(DBG_INFO, "Create data base message\n");
	int msgid = create_msg(DBASE_MSGQ_KEY);
	
	if(msgid < 0)
	{
		logging(DBG_ERROR,"Failed to create data base message queue\n");
		return 0;
	}	

	/* Create thread for DBASE WD response */
	if(dbase_wd_id == -1)
	{
		result = pthread_create(&dbase_wd_id, NULL, dbase_dog_bark_task, NULL);
        if(result == 0)
	        logging(DBG_DETAILED, "Starting data base dog bark thread.\n");
        else
            logging(DBG_ERROR, "DBASE WD thread launch failed\n");
	}

	// Init data base;
	dbase = mysql_init(NULL);

    if(dbase == NULL)
    {
		logging(DBG_ERROR, "%s--%d: Dbase init error\n", __FUNCTION__, __LINE__);
        return -1;
    }

	// Open with user
    if(mysql_real_connect(dbase,"localhost","root","pierfrain2134!",NULL,0,NULL,0)== NULL)
    {
		logging(DBG_ERROR, "%s--%d: Dbase connect error\n", __FUNCTION__, __LINE__);
        mysql_close(dbase);
        return -1;
    }

    if(mysql_query(dbase, "use reader_control"))
    {
		logging(DBG_ERROR, "%s--%d: Dbase query error\n", __FUNCTION__, __LINE__);
        return -1;
    }
	
	// Check if existed table, if not create one
	if(0 >= dbase_check_table_exist(dbase, "reader_giacTable"))
	{
		if(-1 == dbase_create_table(dbase, "reader_giacTable"))
		{
			logging(DBG_ERROR, "%s - %d: dbase create failed\n", __FILE__, __LINE__);
			return -1;
		}
	}

	while(1) {
        recv_msg(msgid, (void *) &dbase_msg, sizeof(DBASE_MSG_T), MSG_TIMEOUT);
        dbase_msg_handler((DBASE_MSG_T *) &dbase_msg, dbase, "reader_giacTable");
        usleep(10000);
	}
}

void *dbase_dog_bark_task()
{
    while(1) {
        send_dog_bark(DBASE_MODULE_ID);
        sleep(1);
    }
}

void dbase_msg_handler(DBASE_MSG_T *Incoming, MYSQL *dbase, char *tablename)
{
	DBASE_MSG_T *reader_msg;
    logging(DBG_INFO, "**** Incoming->modname: %s ****\n", modname[Incoming->header.moduleID]);
	
    switch(Incoming->header.subType)
    {
		case MSG_TAG:
			logging(DBG_DBG, "%s %d: Table name: %s \n", 
				__FUNCTION__, __LINE__, tablename);
#ifndef NOTUSE
			if(dbase_check_existed_tag(dbase, tablename, Incoming->tag))
				dbase_insert_tag(dbase, tablename, Incoming->tag, Incoming->antenna, Incoming->rssi, Incoming->epcLen);	
			else
				logging(DBG_DBG, "%s %d: Tag existed skip table insert\n", __FUNCTION__, __LINE__);
#endif
			break;
		case MSG_TAG_REMOVE:
			break;
		case MSG_TAG_MONITOR:
			break;
        default:
            logging(DBG_INFO, "%s: Unknown message type %d\n",__FUNCTION__, Incoming->header.subType);
            break;
    }
}

int dbase_check_existed_tag(MYSQL *dbase, char *tablename, char *tag)
{
	MYSQL_RES *result;
	char querystr[256];
	int row;

	sprintf((char *) &querystr, "select * from %s where tagnum='%s'", tablename, tag);

	if(mysql_query(dbase, (char *) &querystr))
	{
		logging(DBG_ERROR, "%s--%d: Dbase query error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	result = mysql_store_result(dbase);
    if(result == NULL)
    {
		logging(DBG_ERROR, "%s %d: Store result error\n", __FUNCTION__, __LINE__);
        return -1;
    }

	row = mysql_num_rows(result);

	if(row > 0)
		return 1;
	
	return -1;
}

int dbase_insert_tag(MYSQL *dbase, char *tablename, char *tag, int antid, int rssi, int epcLen)
{

    char querystr[256];
    char dash_tag[128];
    char timestring[64];

    dbase_sys_curtime((char *) &timestring);

	put_dash_to_tag(tag, (char *) &dash_tag, epcLen);
    sprintf((char *) &querystr,"INSERT INTO %s VALUES('%s',NULL,'PRES',%d,%d,'%s')",
											tablename, dash_tag,rssi,antid,&timestring);

    printf("%s - %d: %s\n", __FILE__, __LINE__, &querystr);

	if(mysql_query(dbase, querystr))
	{
		logging(DBG_ERROR, "%s--%d: Dbase query error\n", __FUNCTION__, __LINE__);
		return -1; // RETURN DEFINE ERROR
	}
}

int dbase_sys_curtime(char *timestring)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(timestring, 64, "%Y-%m-%d %H:%M:%S", tm);

    return 0;
}

int dbase_check_table_exist(MYSQL *dbase, char *tablename)
{
	MYSQL_RES *result;
	char querystr[256];
	int row;

	sprintf((char *) &querystr, "show tables like \"%s\"; ", tablename);

	if(mysql_query(dbase, (char *) &querystr))
	{
		logging(DBG_ERROR, "%s--%d: Dbase query error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	result = mysql_store_result(dbase);
    if(result == NULL)
    {
		logging(DBG_ERROR, "%s %d: Store result error\n", __FUNCTION__, __LINE__);
        return -1;
    }

	row = mysql_num_rows(result);

	if(row > 0)
		return 1;
	
	return -1;
}

int dbase_create_table(MYSQL *dbase, char *tablename)
{
	char querystr[256];
	if(dbase == NULL || tablename == NULL)
	{
		logging(DBG_ERROR, "%s %d: Null pointer present\n", __FUNCTION__, __LINE__);
		return -1; // PROCESS_NULL_POINTER
	}

	sprintf((char *) &querystr, "CREATE TABLE %s(tagnum TEXT,tagname TEXT,detectstat TEXT,rssi INT,antid INT,access TEXT)",tablename);
    if(mysql_query(dbase, (char *) &querystr))
    {
		logging(DBG_ERROR, "%s %d: Dbase Query error\n", __FUNCTION__, __LINE__);
        return -1;
    }
}

int dbase_missing_tag(MYSQL *dbase, MYSQL_RES *result, char *tablename, char *var1, char *var2)
{
	// Check pointer error
	char querystr[256];
	if(dbase == NULL || result == NULL || tablename == NULL || var1 == NULL || var2 == NULL)
	{
		logging(DBG_ERROR, "%s %d: Null pointer present\n", __FUNCTION__, __LINE__);
		return -1; // PROCESS_NULL_POINTER
	}
    sprintf(querystr,"SELECT %s,%s FROM %s WHERE detectstat='PRES' AND cmd='READ' AND execstat='NORM'",var1, var2,tablename);

	if(mysql_query(dbase, querystr))
	{
		logging(DBG_ERROR, "%s %d: Dbase Query error\n", __FUNCTION__, __LINE__);
		return -1; // RETURN DEFINE ERROR
	}

	result = mysql_store_result(dbase);
	if(result == NULL)
	{
		logging(DBG_ERROR, "%s %d: Store result error\n", __FUNCTION__, __LINE__);
		return -1;
	}
}

int put_dash_to_tag(char *source, char *dash_tag, int epcLen)
{
	int i;
	for(i = 0; i < epcLen*2; i++)
	{
		if(i%2 == 0 && i > 0)
		*dash_tag++ = '-';
		*dash_tag++ = *source++;
	}
	*dash_tag = '\0'; // End string
}
