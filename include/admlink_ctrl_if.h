#ifndef ADMLINK_CTRL_IF_H
#define ADMLINK_CTRL_IF_H

#define CTRL_INTERFACE "/var/admlink_if"
#define CTRL_INTERFACE_IF "if0"
/** admlink_ctrl result */
#define STR_REPLY_OK              "ok\n"
#define STR_REPLY_ERROR           "error\n"
#define STR_REPLY_UNKOWN          "UNKNOWN COMMAND\n"

#define REPLY_SIZE 128
#include<admlink.h>
typedef struct admlinkdaemon_data {
    struct admlinkdaemon_config *conf;
    //char *config_fname;
    int ctrl_sock;
    struct admlink_ctrl_dst *ctrl_dst;

}admlinkdaemon;

struct admlinkdaemon_cmd {
    const char* cmd;
    int (*handler)(char *cmd, char *reply);
    const char* description;
};

typedef struct _admlink_interfaces {
    int count;
    admlinkdaemon **admcd;
}admlink_interfaces;

struct admlink_ctrl_dst
{
    struct admlink_ctrl_dst *next;
    struct sockaddr_un addr;
    socklen_t addrlen;
    int debug_level;
    int errors;
}; 

enum {DMSG_MSGDUMP, DMSG_DEBUG, DMSG_INFO, DMSG_WARNING, DMSG_ERROR };

int admlink_ctrl_iface_init(admlinkdaemon *admlinkd);
void admlink_ctrl_iface_deinit(admlinkdaemon *admlinkd);
int admlink_cmd_handle(agent* p ,char *buf, char *reply);
void admlink_ctrl_iface_receive(int sock, void *eloop_ctx, void *sock_ctx);
#endif /* ADMLINK_CTRL_IFACE_H */