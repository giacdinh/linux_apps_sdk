#include <stdio.h>
#include <pthread.h>
#include <sys_msg.h>
#include <unistd.h>
#include <string.h>


extern void wdog_main_task();
extern void dbase_main_task();
extern void reader_main_task();
extern void remotem_main_task();
extern void ctrl_main_task();

int main(int argc, char **argv)
{

	pid_t dbase_pid = -1;
	pid_t reader_pid = -1;
	pid_t wdog_pid = -1;
	pid_t remotem_pid = -1;
	pid_t ctrl_pid = -1;

	logging(DBG_DBG, "\n");
	logging(DBG_DBG, "\n");
	logging(DBG_DBG, "**********************************\n");
	logging(DBG_DBG, "       Application start ...\n");
	logging(DBG_DBG, "**********************************\n");
	// Start watch dog for log
	if(wdog_pid == -1)
	{        
		wdog_pid = fork();
        if(wdog_pid == 0) // child process
        {
            logging(DBG_INFO, "Launch watchdog task ID: %i\n", getpid());
            memcpy(argv[0], "main_watchdog         \n", 26);
            wdog_main_task();
        }
    }

	// Start database
	if(dbase_pid == -1)
    {
        dbase_pid = fork();
        if(dbase_pid == 0) // child process
        {
            logging(DBG_INFO, "Launch data base task ID: %i\n", getpid());
            memcpy(argv[0], "dbase_system          \n", 24);
            dbase_main_task();
        }   
    }	

	// Start reader 
	if(reader_pid == -1)
    {
        reader_pid = fork();
        if(reader_pid == 0) // child process
        {
            logging(DBG_INFO, "Launch reader task ID: %i\n", getpid());
            memcpy(argv[0], "reader_system          \n", 24);
            reader_main_task();
        }   
    }	

	// Start remotem
	if(remotem_pid == -1)
    {
        remotem_pid = fork();
        if(remotem_pid == 0) // child process
        {
            logging(DBG_DBG, "Launch remotem task ID: %i\n", getpid());
            memcpy(argv[0], "remote_system          \n", 24);
            remotem_main_task();
        }   
    }	

	// Start controller 
	if(ctrl_pid == -1)
    {
        ctrl_pid = fork();
        if(ctrl_pid == 0) // child process
        {
            logging(DBG_DBG, "Launch controller task ID: %i\n", getpid());
            memcpy(argv[0], "controller_system      \n", 24);
            ctrl_main_task();
        }   
    }	

	while(1) {
		sleep(100); // Main task done after launch children processes
	}
}
