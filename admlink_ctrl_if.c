#include <admlink.h>
#include <admlink_misc.h>
#include <admlink_ctrl_if.h>
static char ctrl_buf[128];
/*****************************************************************************/
int x_get_time_of_day(struct timeval *tv)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	tv->tv_sec =ts.tv_sec;
	tv->tv_usec=ts.tv_nsec/1000;
	return 0; /* success*/
}
/*****************************************************************************/
int admlink_cmd_handle(agent* pag ,char *buf, char *reply)
{
	printf("%s-%d, receive msg [%s]\n", __FUNCTION__, __LINE__, buf);
	//Buf and reply must exist.
	if(!buf || !reply)
		return -1;
	if(strcmp(buf,"sendtest")==0)
	{
		mqconn *p =&pag->mqupld;
		char application_message[256]={0};
		time_t timer;
        time(&timer);
        struct tm* tm_info = localtime(&timer);
        char timebuf[64];
		char didbuf[64];
		char namebuf[64];
		char msverbuf[64];
		char agverbuf[64];
		memset(didbuf,0,sizeof(didbuf));
		sprintf(didbuf,",\"dev_id\":\"%s\"",pag->devid);
		
		memset(namebuf,0,sizeof(namebuf));
		sprintf(namebuf,",\"agt_name\":\"%s\"","maomaoag");
		memset(msverbuf,0,sizeof(msverbuf));
		sprintf(msverbuf,",\"ms_ver\":\"%s\"","9.9.9");
		
		memset(agverbuf,0,sizeof(agverbuf));
		sprintf(agverbuf,",\"agt_ver\":\"%s\"","9.9.9");
		
		memset(timebuf,0,sizeof(timebuf));
        strftime(timebuf,sizeof(timebuf),",\"date\":\"%Y/%m/%d %H:%M:%S\"",tm_info);
		
		sprintf(application_message,"{%s%s%s%s%s%s}",
			"\"type\":\"event\""
			",\"ver\":\"2.00\""
			",\"prdct\":\"unknown\""
			",\"evt_id\":1"
			",\"act_id\":9999"
			",\"act_trg\":0"
			",\"act_para\":\"Test message\""
			",\"act_sts\":0"
			",\"msg\":\"test123\""
			,msverbuf
			,agverbuf
			,namebuf
			,didbuf
			,timebuf
		);
		set_mqtt_publish_nexsync(p,p->topicsend, 
					application_message, MQTT_PUBLISH_QOS_1);
		strcpy(reply,"OK");
	}
	else if(strcmp(buf,"stop")==0)
	{
		eloop_register_timeout(1,0,eloop_set_exit,NULL,NULL);
		strcpy(reply,"OK");
	}
	else
		strcpy(reply,"UNKNOWNCMD");
	return strlen (reply);
}

/*****************************************************************************/
void admlink_debug_print_timestamp(void)
{
    struct timeval tv;
    char buf[16];

    x_get_time_of_day(&tv);

    if (strftime(buf, sizeof(buf), "%b %d %H:%M:%S",
             localtime((const time_t *) &tv.tv_sec)) <= 0) {
        snprintf(buf, sizeof(buf), "%u", (int) tv.tv_sec);
    }
    printf("%s.%06u: ", buf, (unsigned int) tv.tv_usec);
}

/*****************************************************************************/
static void _admlink_hexdump_ascii(int level, const char *title, const char *buf, size_t len, int show)
{
    int i, llen;
    const char *pos = buf;
    const int line_len = 16;

    admlink_debug_print_timestamp();

    if (!show) {
        printf("%s - hexdump_ascii(len=%lu): [REMOVED]\n",
               title, (unsigned long) len);
        return;
    }
    if (buf == NULL) {
        printf("%s - hexdump_ascii(len=%lu): [NULL]\n",
               title, (unsigned long) len);
        return;
    }
    printf("%s - hexdump_ascii(len=%lu):\n", title, (unsigned long) len);
    while (len) {
        llen = len > line_len ? line_len : len;
        printf("    ");
        for (i = 0; i < llen; i++)
            printf(" %02x", pos[i]);
        for (i = llen; i < line_len; i++)
            printf("   ");
        printf("   ");
        for (i = 0; i < llen; i++) {
            if (isprint(pos[i]))
                printf("%c", pos[i]);
            else
                printf("_");
        }
        for (i = llen; i < line_len; i++)
            printf(" ");
        printf("\n");
        pos += llen;
        len -= llen;
    }
}

/*****************************************************************************/
static void admlink_hexdump_ascii(int level, const char *title, const char *buf, size_t len)
{
    _admlink_hexdump_ascii(level, title, buf, len, 1);
}

/*****************************************************************************/
static char * admlink_ctrl_iface_path(admlinkdaemon *hapd)
{
    memset(ctrl_buf, 0, 128);
    sprintf(ctrl_buf, "%s/%s", CTRL_INTERFACE, CTRL_INTERFACE_IF);
    return ctrl_buf;
}

/*****************************************************************************/
void admlink_ctrl_iface_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
    char buf[256];
    int res;
	agent* p=(agent*)eloop_ctx;
    struct sockaddr_un from;
    socklen_t fromlen = sizeof(from);
    char *reply, replybuf[REPLY_SIZE];
    int reply_len;

    res = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *) &from, &fromlen);
    if(res < 0)
    {
        perror("recvfrom(ctrl_iface)");
        return;
    }
    buf[res] = '\0';
	//     admlink_hexdump_ascii(DMSG_DEBUG, "admlink RX ctrl_iface", (char *) buf, res);
    reply = &replybuf[0];

    /** Process commands only if system is not going to reboot */
    if(p->agstatus!=isReboot 
		&&p->agstatus!=isFwup)
    {
        reply_len = admlink_cmd_handle(p,buf, reply);
    }
    else
    {
        /* Just return an "ok" to cheat the caller */
        memcpy(reply, STR_REPLY_OK, strlen(STR_REPLY_OK));
        reply_len = strlen(STR_REPLY_OK);
    }

    if(reply_len < 0)
    {
        memcpy(reply, STR_REPLY_ERROR, strlen(STR_REPLY_ERROR));
        reply_len = strlen(STR_REPLY_ERROR);
    }
    sendto(sock, reply, reply_len, 0, (struct sockaddr *) &from, fromlen);
}

/*****************************************************************************/
int admlink_ctrl_iface_init(admlinkdaemon *admlinkd)
{
    struct sockaddr_un addr;
    int s = -1;
    char *fname = NULL;

    admlinkd->ctrl_sock = -1;
     if(mkdir(CTRL_INTERFACE, S_IRWXU | S_IRWXG) < 0)
    {
        if(errno == EEXIST)
        {
            printf("Using existing control interface directory.\n");
        }
        else
        {
            perror("mkdir[ctrl_interface]");
            goto fail;
        }
    }

	s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(s < 0)
    {
        perror("socket(PF_UNIX)");
        goto fail;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    fname = admlink_ctrl_iface_path(admlinkd);

    strncpy(addr.sun_path, fname, sizeof(addr.sun_path));
    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
        perror("bind(PF_UNIX)");
        goto fail;
    }

    if(chmod(fname, S_IRWXU | S_IRWXG) < 0)
    {
        perror("chmod[ctrl_interface/ifname]");
        goto fail;
    }

    admlinkd->ctrl_sock = s;
    //eloop_register_read_sock(s, admlink_ctrl_iface_receive, admlinkd, NULL);//maomao regestier after init for using cust eloop_ctx
    return 0;

fail:
    if(s >= 0)
        close(s);

    if(fname)
    {
        unlink(fname);
    }

    return -1;
}

/*****************************************************************************/
void admlink_ctrl_iface_deinit(admlinkdaemon *admlinkd)
{
    struct admlink_ctrl_dst *dst, *prev;

    if (admlinkd->ctrl_sock > -1) {
        char *fname;
        eloop_unregister_read_sock(admlinkd->ctrl_sock);
        close(admlinkd->ctrl_sock);
        admlinkd->ctrl_sock = -1;
        fname = admlink_ctrl_iface_path(admlinkd);
        if (fname)
            unlink(fname);

        rmdir(CTRL_INTERFACE);
    }

    dst = admlinkd->ctrl_dst;
    while (dst) {
        prev = dst;
        dst = dst->next;
        free(prev);
    }
}
