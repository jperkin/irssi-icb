/*
 icb-channels.c : irssi

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

#include "icb-channels.h"
#include "icb-protocol.h"

/* Create new ICB channel record */
ICB_CHANNEL_REC *icb_channel_create(ICB_SERVER_REC *server, const char *name,
				    const char *visible_name, int automatic)
{
	ICB_CHANNEL_REC *rec;

	g_return_val_if_fail(server == NULL || IS_ICB_SERVER(server), NULL);
	g_return_val_if_fail(name != NULL, NULL);

	rec = g_new0(ICB_CHANNEL_REC, 1);
	channel_init((CHANNEL_REC *) rec, (SERVER_REC *) server,
		     name, visible_name, automatic);
	return rec;
}

void icb_change_channel(ICB_SERVER_REC *server, const char *channel,
			int automatic)
{
	if (g_strcasecmp(server->group->name, channel) == 0)
		return;

	channel_destroy(CHANNEL(server->group));
	server->group = (ICB_CHANNEL_REC *)
		icb_channel_create(server, channel, NULL, automatic);

        icb_command(server, "g", channel, NULL);
}

static void sig_connected(ICB_SERVER_REC *server)
{
	if (!IS_ICB_SERVER(server))
		return;

	/* create the group for the channel */
	server->group = (ICB_CHANNEL_REC *)
		icb_channel_create(server, server->connrec->channels,
				   NULL, TRUE);
}

void icb_channels_init(void)
{
        signal_add_first("event connected", (SIGNAL_FUNC) sig_connected);
}

void icb_channels_deinit(void)
{
        signal_remove("event connected", (SIGNAL_FUNC) sig_connected);
}
