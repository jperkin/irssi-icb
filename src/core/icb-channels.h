#ifndef __ICB_CHANNELS_H
#define __ICB_CHANNELS_H

#include "channels.h"
#include "icb-servers.h"

/* Returns ICB_CHANNEL_REC if it's ICB channel, NULL if it isn't. */
#define ICB_CHANNEL(channel) \
	PROTO_CHECK_CAST(CHANNEL(channel), ICB_CHANNEL_REC, chat_type, "ICB")

#define IS_ICB_CHANNEL(channel) \
	(ICB_CHANNEL(channel) ? TRUE : FALSE)

#define STRUCT_SERVER_REC ICB_SERVER_REC
struct _ICB_CHANNEL_REC {
#include "channel-rec.h"
};

/* Create new ICB channel record */
ICB_CHANNEL_REC *icb_channel_create(ICB_SERVER_REC *server, const char *name,
				    const char *visible_name, int automatic);

void icb_change_channel(ICB_SERVER_REC *server, const char *channel,
			int automatic);

#define icb_channel_find(server, name) \
	ICB_CHANNEL(channel_find(SERVER(server), name))

void icb_channels_init(void);
void icb_channels_deinit(void);

#endif
