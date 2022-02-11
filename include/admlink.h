#ifndef ADMLINK_H
#define ADMLINK_H
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/klog.h>
#include <sys/syslog.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>
#include <linux/if_packet.h> 
#include "eloop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <curl/curl.h>
#include <cJSON.h>
#include <mqtt.h>
#include <signal.h>
#include <sys/un.h>
#define MQTT_SYN_T 200000U //unit usec
#define SM_CHECK_T 30U //unit sec
#define BUF_SEND_SIZE 4096
#define BUF_RECV_SIZE 2048
#define AG_NAME "maomaoag"
#define AG_VER "9.9.9"
#define MSG_VER "9.9.9"
enum agstatus
{
    isErr=-1,
    isOk,
    isReboot,
    isFwup
};
struct _mqconn
{
	SSL_CTX* ssl_ctx;
	BIO* biofd;
	int connfd;
	int status;
	char daddr[128];
	char dport[128];
	char ca_file[128];
	char clt_key[128];
	char clt_cert[128];
	char cltid[128];
	char topicsend[128];
	char topicrecv[128];
	uint8_t sendbuf[BUF_SEND_SIZE]; /* sendbuf should be large enough to hold multiple whole mqtt messages */
	uint8_t recvbuf[BUF_RECV_SIZE]; /* recvbuf should be large enough any whole mqtt message expected to be received */
	struct mqtt_client mqclt;
};
typedef struct _mqconn mqconn;
struct _agent 
{
	int agstatus;
	char devid[128];
	mqconn mqrecv;
	mqconn mqupld;
	struct tm* tinfo;
};
typedef struct _agent agent;
void exit_link(int exitval);
void close_conn(mqconn* conn);
int dbg_printf(char *format, ...);
#endif
