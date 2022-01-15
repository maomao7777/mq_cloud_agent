#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <admlink_ctrl.h>

/*****************************************************************************/
/**
*  @brief admlink ctrl open
**/
admlink_ctrl * admlink_ctrl_open(const char *ctrl_path)
{
    admlink_ctrl *ctrl;
    static int counter = 0;

    ctrl = malloc(sizeof(*ctrl));
    if (ctrl == NULL)
        return NULL;
    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ctrl->s < 0) {
        free(ctrl);
        return NULL;
    }

    ctrl->local.sun_family = AF_UNIX;
    snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
         "/var/admlink_ctrl_%d-%d", getpid(), counter++);
    if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
            sizeof(ctrl->local)) < 0) {
        close(ctrl->s);
        free(ctrl);
        return NULL;
    }

    ctrl->dest.sun_family = AF_UNIX;
    snprintf(ctrl->dest.sun_path, sizeof(ctrl->dest.sun_path), "%s",
         ctrl_path);
    if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
            sizeof(ctrl->dest)) < 0) {
        close(ctrl->s);
        unlink(ctrl->local.sun_path);
        free(ctrl);
        return NULL;
    }

    return ctrl;
}

/*****************************************************************************/
/**
*  @brief admlink ctrl close
**/
void admlink_ctrl_close(admlink_ctrl *ctrl)
{
    unlink(ctrl->local.sun_path);
    close(ctrl->s);
    free(ctrl);
}

/*****************************************************************************/
/**
*  @brief admlink ctrl request
**/

int admlink_ctrl_request(admlink_ctrl *ctrl, const char *cmd, size_t cmd_len,
             char *reply, size_t *reply_len,
             void (*msg_cb)(char *msg, size_t len))
{
    struct timeval tv;
    int res;
    fd_set rfds;
    if (send(ctrl->s, cmd, cmd_len, 0) < 0)
        return -1;

    for (;;) {
        tv.tv_sec = 42;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(ctrl->s, &rfds);
        res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
        if (FD_ISSET(ctrl->s, &rfds)) {
            res = recv(ctrl->s, reply, *reply_len, 0);
            if (res < 0)
                return res;
            if (res > 0 && reply[0] == '<') {
                /* This is an unsolicited message from
                 * system_daemon, not the reply to the
                 * request. Use msg_cb to report this to the
                 * caller. */
                if (msg_cb) {
                    /* Make sure the message is nul
                     * terminated. */
                    if ((size_t) res == *reply_len)
                        res = (*reply_len) - 1;
                    reply[res] = '\0';
                    msg_cb(reply, res);
                }
                continue;
            }
            *reply_len = res;
            break;
        } else {
            return -2;
        }
    }
    return 0;
}
