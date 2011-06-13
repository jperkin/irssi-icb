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

#include "icb-commands.h"
#include "icb-servers.h"
#include "icb-channels.h"
#include "icb-protocol.h"

static char *icb_commands[] = {
	"whois", "p", "delete", "cp", "rname",
	"phone", "addr", "email", "text", "www",
	"read", "write", "secure", "nosecure", "info", "?",

	"invite", "v", "echoback", "name", "motd", "topic", "status",
	"boot", "pass", "drop", "shutdown", "wall",
	"whereis", "brick", "away", "noaway", "nobeep", "cancel", 
	"exclude", "news", "notify", "s_help", "shuttime", "talk", "hush",
        NULL
};

static void cmd_self(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

        icb_command(server, current_command, data, NULL);
}

static void cmd_quote(const char *data, ICB_SERVER_REC *server)
{
        char *cmd, *args;
	void *free_arg;

	CMD_ICB_SERVER(server);

	if (!cmd_get_params(data, &free_arg, 2, &cmd, &args))
		return;
	if (*cmd == '\0') cmd_param_error(CMDERR_NOT_ENOUGH_PARAMS);

	icb_command(server, cmd, args, NULL);
	cmd_params_free(free_arg);
}

static void cmd_who(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

        icb_command(server, "w", data, NULL);
}

static void cmd_name(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

        icb_command(server, "name", data, NULL);
}

static void cmd_boot(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

        icb_command(server, "boot", data, NULL);
}

static void cmd_group(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

	if (*data == '\0')
		cmd_return_error(CMDERR_NOT_ENOUGH_PARAMS);

        icb_change_channel(server, data, FALSE);
}

static void cmd_beep(const char *data, ICB_SERVER_REC *server)
{
	CMD_ICB_SERVER(server);

	if (*data != '\0') {
		icb_command(server, "beep", data, NULL);
		/* disable irssi's default /beep command */
		signal_stop();
	}
}

void icb_commands_init(void)
{
	char **cmd;

	for (cmd = icb_commands; *cmd != NULL; cmd++)
		command_bind_icb(*cmd, NULL, (SIGNAL_FUNC) cmd_self);

	/* adds also some aliases known to IRC users :) */
	command_bind_icb("quote", NULL, (SIGNAL_FUNC) cmd_quote);
	command_bind_icb("w", NULL, (SIGNAL_FUNC) cmd_who);
	command_bind_icb("who", NULL, (SIGNAL_FUNC) cmd_who);
        command_bind_icb("nick", NULL, (SIGNAL_FUNC) cmd_name);
        command_bind_icb("kick", NULL, (SIGNAL_FUNC) cmd_boot);
        command_bind_icb("g", NULL, (SIGNAL_FUNC) cmd_group);
        command_bind_icb("beep", NULL, (SIGNAL_FUNC) cmd_beep);

	command_set_options("connect", "+icbnet");
}

void icb_commands_deinit(void)
{
	char **cmd;

	for (cmd = icb_commands; *cmd != NULL; cmd++)
		command_unbind(*cmd, (SIGNAL_FUNC) cmd_self);

	command_unbind("quote", (SIGNAL_FUNC) cmd_quote);
	command_unbind("w", (SIGNAL_FUNC) cmd_who);
        command_unbind("who", (SIGNAL_FUNC) cmd_who);
        command_unbind("nick", (SIGNAL_FUNC) cmd_name);
        command_unbind("kick", (SIGNAL_FUNC) cmd_boot);
        command_unbind("g", (SIGNAL_FUNC) cmd_group);
        command_unbind("beep", (SIGNAL_FUNC) cmd_beep);
}
