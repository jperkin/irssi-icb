/*
 icb-servers.c : irssi

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
#include "rawlog.h"
#include "net-sendbuffer.h"

#include "channels-setup.h"
#include "icb-servers.h"
#include "icb-channels.h"
#include "icb-protocol.h"

SERVER_REC *icb_server_init_connect(SERVER_CONNECT_REC *conn)
{
	ICB_SERVER_REC *server;

	g_return_val_if_fail(IS_ICB_SERVER_CONNECT(conn), NULL);
	if (conn->address == NULL || *conn->address == '\0') return NULL;
	if (conn->nick == NULL || *conn->nick == '\0') return NULL;

	server = g_new0(ICB_SERVER_REC, 1);
	server->chat_type = ICB_PROTOCOL;

	server->silentwho = FALSE;
	server->updatenicks = FALSE;

        server->recvbuf_size = 256;
	server->recvbuf = g_malloc(server->recvbuf_size);

        server->sendbuf_size = 256;
	server->sendbuf = g_malloc(server->sendbuf_size);

	server->connrec = (ICB_SERVER_CONNECT_REC *) conn;
        server_connect_ref(SERVER_CONNECT(conn));

	if (server->connrec->port <= 0)
		server->connrec->port = 7326;

        server_connect_init((SERVER_REC *) server);
	return (SERVER_REC *) server;
}

void icb_server_connect(SERVER_REC *server)
{
	if (!server_start_connect(server)) {
                server_connect_unref(server->connrec);
		g_free(server);
	}
}

static void sig_server_disconnected(ICB_SERVER_REC *server)
{
	if (!IS_ICB_SERVER(server))
		return;

	if (server->handle != NULL) {
		/* disconnect it here, so the core won't try to leave it
		   later - ICB has no quit messages so it doesn't really
		   matter if the server got our last messages or not */
		net_sendbuffer_destroy(server->handle, TRUE);
		server->handle = NULL;
	}

        g_free(server->recvbuf);
        g_free(server->sendbuf);
}

char *icb_server_get_channels(ICB_SERVER_REC *server)
{
	g_return_val_if_fail(IS_ICB_SERVER(server), FALSE);

	return g_strdup(server->group->name);
}

static void channels_join(SERVER_REC *server, const char *channel,
			  int automatic)
{
	icb_change_channel(ICB_SERVER(server), channel, automatic);
}

static int isnickflag_func(char flag)
{
        return flag == '*';
}

static int ischannel_func(SERVER_REC *server, const char *data)
{
        return g_strcasecmp(ICB_SERVER(server)->group->name, data) == 0;
}

static const char *get_nick_flags(void)
{
	static const char flags[] = { '*', '\0', '\0' };
        return flags;
}

static void send_message(SERVER_REC *server, const char *target,
			 const char *msg, int target_type)
{
	ICB_SERVER_REC *icbserver;

        icbserver = ICB_SERVER(server);
	g_return_if_fail(server != NULL);
	g_return_if_fail(target != NULL);
	g_return_if_fail(msg != NULL);

	if (target_type == SEND_TARGET_CHANNEL) {
		/* channel message */
                icb_send_open_msg(icbserver, msg);
	} else {
		/* private message */
		icb_send_private_msg(icbserver, target, msg);
	}
}

static void sig_connected(ICB_SERVER_REC *server)
{
	if (!IS_ICB_SERVER(server))
		return;

	server->channels_join = channels_join;
	server->isnickflag = isnickflag_func;
	server->ischannel = ischannel_func;
	server->get_nick_flags = get_nick_flags;
	server->send_message = send_message;
}

static void sig_setup_fill_connect(ICB_SERVER_CONNECT_REC *conn)
{
	GSList *tmp;

	if (!IS_ICB_SERVER_CONNECT(conn))
                return;

        /* We'll always need to be in one group in ICB */
	if (conn->channels != NULL && *conn->channels != '\0')
                return;

	g_free_not_null(conn->channels);

        /* find the first ICB channel from channel settings */
	for (tmp = setupchannels; tmp != NULL; tmp = tmp->next) {
		CHANNEL_SETUP_REC *rec = tmp->data;

		if (rec->chat_type == ICB_PROTOCOL &&
		    channel_chatnet_match(rec->chatnet, conn->chatnet)) {
			conn->channels = g_strdup(rec->name);
                        break;
		}
	}

	if (conn->channels == NULL)
                conn->channels = g_strdup(DEFAULT_ICB_GROUP);
}

void icb_servers_init(void)
{
	signal_add_first("server connected", (SIGNAL_FUNC) sig_connected);
        signal_add("server disconnected", (SIGNAL_FUNC) sig_server_disconnected);
	signal_add("server setup fill connect", (SIGNAL_FUNC) sig_setup_fill_connect);
}

void icb_servers_deinit(void)
{
	signal_remove("server connected", (SIGNAL_FUNC) sig_connected);
        signal_remove("server disconnected", (SIGNAL_FUNC) sig_server_disconnected);
	signal_remove("server setup fill connect", (SIGNAL_FUNC) sig_setup_fill_connect);
}
