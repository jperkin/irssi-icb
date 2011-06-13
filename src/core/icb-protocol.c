/*
 icb-protocol.c : irssi

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
#include "network.h"
#include "net-sendbuffer.h"
#include "rawlog.h"

#include "icb-servers.h"

static char *signal_names[] = {
	"login",	/* a */
	"open",		/* b */
	"personal",	/* c */
	"status",	/* d */
	"error",	/* e */
	"important",	/* f */
	"exit",		/* g */
	"command",	/* h */
	"cmdout",	/* i */
	"protocol",	/* j */
	"beep",		/* k */
	"ping",		/* l */
	"pong"		/* m */
};

#define SIGNAL_FIRST 'a'
#define SIGNALS_COUNT (sizeof(signal_names)/sizeof(signal_names[0]))

#ifdef BLOCKING_SOCKETS
#  define MAX_SOCKET_READS 1
#else
#  define MAX_SOCKET_READS 5
#endif

static void icb_send_cmd(ICB_SERVER_REC *server, int type, ...)
{
        const char *arg;
	va_list va;
        int pos, len, startpos;

	g_return_if_fail(IS_ICB_SERVER(server));

	server->sendbuf[1] = type;
	pos = 2;

	va_start(va, type);
	for (;;) {
		arg = va_arg(va, const char *);
		if (arg == NULL)
			break;

		len = strlen(arg);
                /* +2 == ^A + \0 at end of buffer */
		if (pos+len+2 > server->sendbuf_size) {
                        server->sendbuf_size += len + 128;
			server->sendbuf = g_realloc(server->sendbuf,
						    server->sendbuf_size);
		}

		if (pos != 2) {
			/* separate fields with ^A */
                        server->sendbuf[pos++] = '\001';
		}

		memcpy(server->sendbuf+pos, arg, len);
		pos += len;
	}
	va_end(va);

        server->sendbuf[pos++] = '\0';
	rawlog_output(server->rawlog, (char *) server->sendbuf+1);

	startpos = 0;
	while (startpos < pos) {
		len = pos-startpos-1;
		if (len > 255) len = 255;

		server->sendbuf[startpos] = len > 255 ? 0 : len;

		if (net_sendbuffer_send(server->handle,
					server->sendbuf+startpos, len+1) == -1) {
			/* something bad happened */
			server->connection_lost = TRUE;
			server_disconnect(SERVER(server));
			break;
		}

                startpos += 256;
	}
}

static void icb_login(ICB_SERVER_REC *server)
{
	icb_send_cmd(server, 'a',
		     server->connrec->username,
		     server->connrec->nick,
		     server->connrec->channels,
		     "login", /* login / w = just show who's online */
		     server->connrec->password,
		     NULL);
}

void icb_send_open_msg(ICB_SERVER_REC *server, const char *text)
{
	size_t remain;

	/*
	 * ICB has 255 byte line length limit, and public messages are sent
	 * out with our nickname, so split text accordingly.
	 *
	 * 250 = 255 - 'b' - 1 space after nick - ^A - nul - extra
	 *
	 * Taken from ircII's icb.c, thanks phone :-)
	 */
	remain = 250 - strlen(server->connrec->nick);

	while(*text) {
		char buf[256], *sendbuf;
		size_t len, copylen;

		len = strlen(text);
		copylen = remain;
		if (len > remain) {
			int i;

			/* try to split on a word boundary */
			for (i = 1; i < 128 && i < len; i++) {
				if (isspace(text[remain - i])) {
					copylen -= i - 1;
					break;
				}
			}
			strncpy(buf, text, copylen);
			buf[copylen] = 0;
			sendbuf = buf;
		} else {
			sendbuf = (char *)text;
		}
		icb_send_cmd(server, 'b', sendbuf, NULL);
		text += len > copylen ? copylen : len;
	}
}

void icb_send_private_msg(ICB_SERVER_REC *server, const char *target,
		const char *text)
{
	size_t mylen, targlen, remain;

	/*
	 * ICB has 255 byte line length limit.  Private messages are sent
	 * out with our nickname, but received with the target nickname,
	 * so deduct the larger of the two in addition to other parts.
	 *
	 * 248 = 255 - 'hm' - 1 space after nick - ^A's - nul - extra
	 *
	 * Taken from ircII's icb.c, thanks phone :-)
	 */
	mylen = strlen(server->connrec->nick);
	targlen = strlen(target);
	if (mylen > targlen) {
		remain = 248 - mylen;
	} else {
		remain = 248 - targlen;
	}
	while(*text) {
		char buf[256], *sendbuf;
		size_t len, copylen;

		len = strlen(text);
		copylen = remain;
		if (len > remain) {
			int i;

			/* try to split on a word boundary */
			for (i = 1; i < 128 && i < len; i++) {
				if (isspace(text[remain - i])) {
					copylen -= i - 1;
					break;
				}
			}
			strncpy(buf, text, copylen);
			buf[copylen] = 0;
			sendbuf = g_strconcat(target, " ", buf, NULL);
		} else {
			sendbuf = g_strconcat(target, " ", text, NULL);
		}
		icb_send_cmd(server, 'h', "m", sendbuf, NULL);
		text += len > copylen ? copylen : len;
	}
}

void icb_command(ICB_SERVER_REC *server, const char *cmd,
		 const char *args, const char *id)
{
        icb_send_cmd(server, 'h', cmd, args, id, NULL);
}

void icb_protocol(ICB_SERVER_REC *server, const char *level,
		  const char *hostid, const char *clientid)
{
	icb_send_cmd(server, 'j', level, hostid, clientid, NULL);
}

void icb_ping(ICB_SERVER_REC *server, const char *id)
{
	icb_send_cmd(server, 'l', id, NULL);
}

void icb_pong(ICB_SERVER_REC *server, const char *id)
{
	icb_send_cmd(server, 'm', id, NULL);
}

void icb_noop(ICB_SERVER_REC *server)
{
	icb_send_cmd(server, 'n', NULL);
}

static void icb_server_event(ICB_SERVER_REC *server, const char *data)
{
	char name[100];

	if (*data < SIGNAL_FIRST || *data >= SIGNAL_FIRST + SIGNALS_COUNT)
		return; /* unknown packet type */

	strcpy(name, "icb event ");
        strcat(name, signal_names[*data - SIGNAL_FIRST]);
        signal_emit(name, 2, server, data+1);
}

/* Read one ICB packet. Returns 1 if got it, 0 if not or -1 if disconnected */
static int icb_read_packet(ICB_SERVER_REC *server, int read_socket)
{
	char tmpbuf[512];
	int ret, pos, wpos, size;

	if (server->recvbuf_next_packet > 0) {
                /* remove the last packet from buffer */
		g_memmove(server->recvbuf,
			  server->recvbuf+server->recvbuf_next_packet,
			  server->recvbuf_pos - server->recvbuf_next_packet);
                server->recvbuf_pos -= server->recvbuf_next_packet;
                server->recvbuf_next_packet = 0;
	}

	ret = !read_socket ? 0 :
		net_receive(net_sendbuffer_handle(server->handle),
			    tmpbuf, sizeof(tmpbuf));
	if (ret > 0) {
		if (server->recvbuf_pos + ret > server->recvbuf_size) {
			server->recvbuf_size += ret + 256;
			server->recvbuf = g_realloc(server->recvbuf,
						    server->recvbuf_size);
		}
		memcpy(server->recvbuf+server->recvbuf_pos, tmpbuf, ret);
                server->recvbuf_pos += ret;
	}

	/* check that we have a full packet */
	pos = 0;
	while (pos < server->recvbuf_pos) {
		if (server->recvbuf[pos] != 0) {
			pos += server->recvbuf[pos];
			break;
		}
		pos += 256;
	}

	if (pos >= server->recvbuf_pos) {
		/* nope */
		if (ret == -1) {
			/* connection lost */
			server->connection_lost = TRUE;
			server_disconnect(SERVER(server));
			return -1;
		}
		return 0;
	}

	/* we have at least one full packet - combine the 256B blocks
	   into one big nul-terminated block */
        pos = wpos = 0;
	while (pos < server->recvbuf_pos) {
		if (server->recvbuf[pos] != 0) {
                        size = server->recvbuf[pos];
			g_memmove(server->recvbuf+wpos,
				  server->recvbuf+pos+1, size);
                        pos += size+1;
			wpos += size;
                        break;
		}

		g_memmove(server->recvbuf+wpos, server->recvbuf+pos+1, 255);
                pos += 256;
                wpos += 255;
	}

	server->recvbuf[wpos] = '\0';
	server->recvbuf_next_packet = pos;
        return 1;
}

static void icb_parse_incoming(ICB_SERVER_REC *server)
{
	int count;

        count = 0;
	while (icb_read_packet(server, count < MAX_SOCKET_READS) > 0) {
		rawlog_input(server->rawlog, (char *) server->recvbuf);
                icb_server_event(server, (char *) server->recvbuf);

		count++;
		if (g_slist_find(servers, server) == NULL)
			break; /* disconnected */
	}
}

static void sig_server_connected(ICB_SERVER_REC *server)
{
	if (!IS_ICB_SERVER(server))
                return;

	server->readtag =
		g_input_add(net_sendbuffer_handle(server->handle),
			    G_INPUT_READ,
			    (GInputFunction) icb_parse_incoming, server);
}

static void event_protocol(ICB_SERVER_REC *server, const char *data)
{
	/* ignore parameters - just send the login packet */
        icb_login(server);
}

static void event_login(ICB_SERVER_REC *server, const char *data)
{
	/* Login OK */
        server->connected = TRUE;
	signal_emit("event connected", 1, server);
}

static void event_ping(ICB_SERVER_REC *server, const char *data)
{
        icb_pong(server, data);
}

static void event_cmdout(ICB_SERVER_REC *server, const char *data)
{
	char **args, *event;

	args = g_strsplit(data, "\001", -1);
	if (args[0] != NULL) {
		event = g_strdup_printf("icb cmdout %s", args[0]);
		if (!signal_emit(event, 2, server, args+1))
                        signal_emit("default icb cmdout", 2, server, args);
                g_free(event);
	}
        g_strfreev(args);
}

static void event_status(ICB_SERVER_REC *server, const char *data)
{
	char **args, *event;

	args = g_strsplit(data, "\001", -1);
	if (args[0] != NULL) {
		event = g_strdup_printf("icb status %s", g_ascii_strdown(args[0], strlen(args[0])));
		if (!signal_emit(event, 2, server, args))
                        signal_emit("default icb status", 2, server, args);
                g_free(event);
	}
        g_strfreev(args);
}

void icb_protocol_init(void)
{
        signal_add("server connected", (SIGNAL_FUNC) sig_server_connected);
        signal_add("icb event protocol", (SIGNAL_FUNC) event_protocol);
        signal_add("icb event login", (SIGNAL_FUNC) event_login);
        signal_add("icb event ping", (SIGNAL_FUNC) event_ping);
        signal_add("icb event cmdout", (SIGNAL_FUNC) event_cmdout);
	signal_add("icb event status", (SIGNAL_FUNC) event_status);
}

void icb_protocol_deinit(void)
{
        signal_remove("server connected", (SIGNAL_FUNC) sig_server_connected);
        signal_remove("icb event protocol", (SIGNAL_FUNC) event_protocol);
        signal_remove("icb event login", (SIGNAL_FUNC) event_login);
        signal_remove("icb event ping", (SIGNAL_FUNC) event_ping);
        signal_remove("icb event cmdout", (SIGNAL_FUNC) event_cmdout);
        signal_remove("icb event status", (SIGNAL_FUNC) event_status);
}
