#ifndef __ICB_SERVERS_H
#define __ICB_SERVERS_H

#include "chat-protocols.h"
#include "servers.h"

/* returns ICB_SERVER_REC if it's ICB server, NULL if it isn't */
#define ICB_SERVER(server) \
	PROTO_CHECK_CAST(SERVER(server), ICB_SERVER_REC, chat_type, "ICB")

#define ICB_SERVER_CONNECT(conn) \
	PROTO_CHECK_CAST(SERVER_CONNECT(conn), ICB_SERVER_CONNECT_REC, \
			 chat_type, "ICB")

#define IS_ICB_SERVER(server) \
	(ICB_SERVER(server) ? TRUE : FALSE)

#define IS_ICB_SERVER_CONNECT(conn) \
	(ICB_SERVER_CONNECT(conn) ? TRUE : FALSE)

struct _ICB_SERVER_CONNECT_REC {
#include "server-connect-rec.h"
};

#define STRUCT_SERVER_CONNECT_REC ICB_SERVER_CONNECT_REC
struct _ICB_SERVER_REC {
#include "server-rec.h"

        ICB_CHANNEL_REC *group; /* ICB server can have only one channel active - and it's called group. */

	unsigned char *sendbuf;
	int sendbuf_size;

	int silentwho;		/* silence /who output when updating nicks */
	int updatenicks;	/* parse /who output for topic/nicks */

	unsigned char *recvbuf;
	int recvbuf_size, recvbuf_pos;
        int recvbuf_next_packet;
};

SERVER_REC *icb_server_init_connect(SERVER_CONNECT_REC *conn);
void icb_server_connect(SERVER_REC *server);

char *icb_server_get_channels(ICB_SERVER_REC *server);

void icb_servers_init(void);
void icb_servers_deinit(void);

#endif
