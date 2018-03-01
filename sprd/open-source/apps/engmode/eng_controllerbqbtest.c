#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <utils/Log.h>

#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <fcntl.h>

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include "eng_controllerbqbtest.h"
#include "engopt.h"
#include <sys/inotify.h>


static int bt_fd = -1;
static pthread_t ntid_bqb = (pthread_t)(-1);

extern int eng_controller2tester(char * controller_buf, unsigned int data_len);
extern int bt_hci_init_transport (int *bt_fd);
extern int sprd_config_init(int fd, char *bdaddr, struct termios *ti);
static void thread_exit_handler(int sig)
{
    ENG_LOG("receive signal %d , eng_receive_data_thread exit\n", sig);
    close(bt_fd);
    pthread_exit(0);
}

/*
** Function: eng_receive_data_thread
**
** Description:
**	  Receive the data from controller, and send the data to the tester
**
** Arguments:
**	 void
**
** Returns:
**
*/
void  eng_receive_data_thread(void)
{
    unsigned int nRead = 0;
    char buf[1030] = {0};
    struct sigaction actions;

    ENG_LOG("bqb test eng_receive_data_thread");

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR2, &actions, NULL);
    while(1) {
       memset(buf, 0x0, sizeof(buf));
       nRead = read(bt_fd, buf, 1000);

        ENG_LOG("bqb test receive data from controller: %d", nRead);
        if(nRead > 0) {
             eng_controller2tester(buf, nRead);
        }
    }
 }

/*
** Function: eng_controller_bqb_start
**
** Description:
**	  open the ttys0 and create a thread, download bt controller code
**
** Arguments:
**	 void
**
** Returns:
**	  0
**    -1 failed
*/
int eng_controller_bqb_start(void)
{
    pthread_t thread;
    int ret = -1;

    ENG_LOG(" bqb test test eng_controller_bqb_start");

    bt_hci_init_transport(&bt_fd);
    sprd_config_init(bt_fd, NULL,NULL);

    ret = pthread_create(&ntid_bqb, NULL, (void *)eng_receive_data_thread, NULL);   /*create thread*/
    if(ret== -1)  {
        ENG_LOG("bqb test create thread failed");
    }

    return ret;
}

int eng_controller_bqb_stop(void)
{
    ENG_LOG("stop BQB test receive data thread");

    if(pthread_kill(ntid_bqb, SIGUSR2) != 0){
        ENG_LOG("pthread_kill BQB test receive data thread error\n");
        return -1;
    }

    return 0;
}

/*
** Function: eng_send_data
**
** Description:
**	  Recieve the data from tester and send the data to controller
**
** Arguments:
**	 data  from the tester
**   the data length
**
** Returns:
**	  0  success
**    -1 failed
*/
void eng_send_data(char * data, int data_len)
{
    int nWritten = 0;
    int count = data_len;
    char * data_ptr = data;

    ENG_LOG("bqb test eng_send_data, fd=%d, len=%d", bt_fd, count);

    while(count)
    {
        nWritten = write(bt_fd, data, data_len);
        count -= nWritten;
        data_ptr  += nWritten;

    }
    ENG_LOG("bqb test eng_send_data nWritten %d ", nWritten);
}

