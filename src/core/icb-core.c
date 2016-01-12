/*
 icb-core.c : ICB protocol module for irssi

    Copyright (C) 2001 Timo Sirainen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "module.h"
#include "signals.h"

#include "chat-protocols.h"
#include "chatnets.h"
#include "servers-setup.h"
#include "channels-setup.h"

#include "icb-commands.h"
#include "icb-servers.h"
#include "icb-channels.h"
#include "icb-queries.h"
#include "icb-protocol.h"

void icb_session_init(void);
void icb_session_deinit(void);

void icb_servers_reconnect_init(void);
void icb_servers_reconnect_deinit(void);

char **icb_split(const char *data, int count)
{
        const char *start;
	char **ret;
        int pos;

	ret = g_new(char *, count+1);
	memset(ret, 0, sizeof(char *) * (count+1));

	if (count == 1) {
                /* only one parameter - return it */
		ret[0] = g_strdup(data);
                return ret;
	}

        start = data; pos = 0;
	while (*data != '\0') {
		if (*data == '\001') {
			ret[pos++] = g_strndup(start, data-start);
                        start = data+1;
			if (pos == count-1)
                                break;
		}
                data++;
	}

	ret[pos] = g_strdup(start);
	return ret;
}

void icb_split_free(char **args)
{
	char **pos;

	for (pos = args; *pos != NULL; pos++)
		g_free(*pos);
        g_free(args);
}

static CHATNET_REC *create_chatnet(void)
{
        return g_malloc0(sizeof(CHATNET_REC));
}

static SERVER_SETUP_REC *create_server_setup(void)
{
        return g_malloc0(sizeof(SERVER_SETUP_REC));
}

static CHANNEL_SETUP_REC *create_channel_setup(void)
{
        return g_malloc0(sizeof(CHANNEL_SETUP_REC));
}

static SERVER_CONNECT_REC *create_server_connect(void)
{
        return g_malloc0(sizeof(SERVER_CONNECT_REC));
}

static void destroy_server_connect(SERVER_CONNECT_REC *conn)
{
}

static CHANNEL_REC *_channel_create(SERVER_REC *server, const char *name,
				    const char *visible_name, int automatic)
{
	return (CHANNEL_REC *)
		icb_channel_create((ICB_SERVER_REC *) server, name,
				   visible_name, automatic);
}

void icb_core_init(void)
{
	CHAT_PROTOCOL_REC *rec;

	rec = g_new0(CHAT_PROTOCOL_REC, 1);
	rec->name = "ICB";
	rec->fullname = "Internet Citizen's Band";
	rec->chatnet = "icbnet";

        rec->case_insensitive = TRUE;

	rec->create_chatnet = create_chatnet;
        rec->create_server_setup = create_server_setup;
        rec->create_channel_setup = create_channel_setup;
	rec->create_server_connect = create_server_connect;
	rec->destroy_server_connect = destroy_server_connect;

	rec->server_init_connect = icb_server_init_connect;
	rec->server_connect = icb_server_connect;
	rec->channel_create = _channel_create;
	rec->query_create = icb_query_create;

	chat_protocol_register(rec);
        g_free(rec);

	icb_servers_init();
	icb_servers_reconnect_init();
        icb_channels_init();
	icb_protocol_init();
	icb_commands_init();
        icb_session_init();

	module_register("icb", "core");
}

void icb_core_deinit(void)
{
	icb_servers_deinit();
	icb_servers_reconnect_deinit();
        icb_channels_deinit();
	icb_protocol_deinit();
        icb_commands_deinit();
        icb_session_deinit();

	signal_emit("chat protocol deinit", 1, chat_protocol_find("ICB"));
	chat_protocol_unregister("ICB");
}

#ifdef IRSSI_ABI_VERSION
void
icb_core_abicheck(int * version)
{
	*version = IRSSI_ABI_VERSION;
}
#endif
