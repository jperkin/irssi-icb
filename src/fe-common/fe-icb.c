/*
 fe-icb.c : irssi

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
#include "module-formats.h"
#include "signals.h"
#include "commands.h"
#include "servers-setup.h"
#include "levels.h"

#include "icb.h"
#include "icb-servers.h"
#include "icb-channels.h"
#include "icb-protocol.h"

#include "printtext.h"
#include "themes.h"

static void event_status(ICB_SERVER_REC *server, const char *data)
{
	char **args;

	/* FIXME: status messages should probably divided into their own
	   signals so irssi could track joins, parts, etc. */
	args = icb_split(data, 2);
	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
        icb_split_free(args);
}

static void event_error(ICB_SERVER_REC *server, const char *data)
{
	printformat(server, NULL, MSGLEVEL_CRAP, ICBTXT_ERROR, data);
}

static void event_important(ICB_SERVER_REC *server, const char *data)
{
	char **args;

	args = icb_split(data, 2);
	printformat(server, NULL, MSGLEVEL_CRAP, ICBTXT_IMPORTANT,
		    args[0], args[1]);
        icb_split_free(args);
}

static void event_beep(ICB_SERVER_REC *server, const char *data)
{
	printformat(server, data, MSGLEVEL_CRAP, ICBTXT_BEEP, data);
}

static void event_open(ICB_SERVER_REC *server, const char *data)
{
	char **args;

	args = icb_split(data, 2);
	signal_emit("message public", 5, server, args[1], args[0], "",
		    server->group->name);
        icb_split_free(args);
}

static void event_personal(ICB_SERVER_REC *server, const char *data)
{
	char **args;

	args = icb_split(data, 2);
	signal_emit("message private", 4, server, args[1], args[0], "");
        icb_split_free(args);
}

static void cmdout_default(ICB_SERVER_REC *server, char **args)
{
	char *data;

	data = g_strjoinv(" ", args+1);
	printtext(server, server->group->name, MSGLEVEL_CRAP, "%s", data);
        g_free(data);
}

static void sig_server_add_fill(SERVER_SETUP_REC *rec,
				GHashTable *optlist)
{
	char *value;

	value = g_hash_table_lookup(optlist, "icbnet");
	if (value != NULL) {
		g_free_and_null(rec->chatnet);
		if (*value != '\0') rec->chatnet = g_strdup(value);
	}
}

void fe_icb_init(void)
{
	theme_register(fecommon_icb_formats);

	signal_add("icb event status", (SIGNAL_FUNC) event_status);
        signal_add("icb event error", (SIGNAL_FUNC) event_error);
        signal_add("icb event important", (SIGNAL_FUNC) event_important);
        signal_add("icb event beep", (SIGNAL_FUNC) event_beep);
        signal_add("icb event open", (SIGNAL_FUNC) event_open);
        signal_add("icb event personal", (SIGNAL_FUNC) event_personal);
        signal_add("default icb cmdout", (SIGNAL_FUNC) cmdout_default);

	signal_add("server add fill", (SIGNAL_FUNC) sig_server_add_fill);
	command_set_options("server add", "-icbnet");

	module_register("icb", "fe");
}

void fe_icb_deinit(void)
{
        signal_remove("icb event status", (SIGNAL_FUNC) event_status);
        signal_remove("icb event error", (SIGNAL_FUNC) event_error);
        signal_remove("icb event important", (SIGNAL_FUNC) event_important);
        signal_remove("icb event beep", (SIGNAL_FUNC) event_beep);
        signal_remove("icb event open", (SIGNAL_FUNC) event_open);
        signal_remove("icb event personal", (SIGNAL_FUNC) event_personal);
        signal_remove("default icb cmdout", (SIGNAL_FUNC) cmdout_default);

	signal_remove("server add fill", (SIGNAL_FUNC) sig_server_add_fill);
}
