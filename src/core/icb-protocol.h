#ifndef __ICB_PROTOCOL_H
#define __ICB_PROTOCOL_H

#define ICB_PROTOCOL_LEVEL 1

void icb_send_open_msg(ICB_SERVER_REC *server, const char *text);
void icb_send_private_msg(ICB_SERVER_REC *server, const char *target,
		const char *text);
void icb_command(ICB_SERVER_REC *server, const char *cmd,
		 const char *args, const char *id);
void icb_protocol(ICB_SERVER_REC *server, const char *level,
		  const char *hostid, const char *clientid);
void icb_ping(ICB_SERVER_REC *server, const char *id);
void icb_pong(ICB_SERVER_REC *server, const char *id);
void icb_noop(ICB_SERVER_REC *server);

void icb_protocol_init(void);
void icb_protocol_deinit(void);

#endif
