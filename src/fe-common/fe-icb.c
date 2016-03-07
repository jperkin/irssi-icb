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

#include <time.h>

#include "module.h"
#include "module-formats.h"
#include "signals.h"
#include "commands.h"
#include "servers-setup.h"
#include "levels.h"
#include "nicklist.h"

#include "icb.h"
#include "icb-servers.h"
#include "icb-channels.h"
#include "icb-nicklist.h"
#include "icb-protocol.h"

#include "printtext.h"
#include "themes.h"

static void icb_change_topic(ICB_SERVER_REC *server, const char *topic,
			     const char *setby, time_t settime)
{
	if (topic != NULL) {
		g_free_not_null(server->group->topic);
		server->group->topic = g_strdup(topic);
	}

	if (setby != NULL) {
		g_free_not_null(server->group->topic_by);
		server->group->topic_by = g_strdup(setby);
	}

	server->group->topic_time = settime;

	signal_emit("channel topic changed", 1, server->group);
}

/*
 * ICB makes it hard to keep track of nicks:
 *
 *  - moderators can come and go, and even return with a different nick, while
 *    still retaining their moderator status
 *
 *  - group moderator can change at any time, if the moderator is off-group
 *    while changing nick
 *
 *  - users can be moderator of multiple groups simultaneously, but can only
 *    be in one group at a time
 *
 * So for now we don't bother to track the moderator, just the group nicks
 */
static void icb_update_nicklist(ICB_SERVER_REC *server)
{
	/*
	 * In theory we should be able to just send '/who <group>' and parse,
	 * but the problem is that ICB does not send any kind of end-of-who
	 * marker when only listing one group, and sending a separate command
	 * isn't guaranteed to come back in the right order.
	 *
	 * So we're forced do perform a full /who and then match against our
	 * groupname.  A full /who is terminated with a 'Total: ' line which we
	 * can use as EOF>
	 */
	server->silentwho = TRUE;
	icb_command(server, "w", "", NULL);
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
	signal_emit("message private", 5, server, args[1], args[0], "",
		    server->nick);
        icb_split_free(args);
}

static void idle_time(char *buf, size_t bufsize, time_t idle)
{
#define MIN_LEN		60UL
#define HOUR_LEN	3600UL
#define DAY_LEN		86400UL
#define WEEK_LEN	604800UL

	if (idle >= WEEK_LEN)
		snprintf(buf, bufsize, "%2dw%2dd",
			 (int)(idle/WEEK_LEN), (int)((idle%WEEK_LEN)/DAY_LEN));
	else if (idle >= DAY_LEN)
		snprintf(buf, bufsize, "%2dd%2dh",
			 (int)(idle/DAY_LEN), (int)((idle%DAY_LEN)/HOUR_LEN));
	else if (idle >= HOUR_LEN)
		snprintf(buf, bufsize, "%2dh%2dm",
			 (int)(idle/HOUR_LEN), (int)((idle%HOUR_LEN)/MIN_LEN));
	else if (idle >= MIN_LEN)
		snprintf(buf, bufsize, "%2dm%2ds",
			 (int)(idle/MIN_LEN), (int)(idle%MIN_LEN));
	else
		snprintf(buf, bufsize, "   %2ds", (int)idle);
}

static void cmdout_co(ICB_SERVER_REC *server, char **args)
{
	char *p, *group, *topic;
	int len;

	static const char match_group[] = "Group: ";
	static const char match_topic[] = "Topic: ";
	static const char match_topicunset[] = "(None)";
	static const char match_topicis[] = "The topic is";
	static const char match_total[] = "Total: ";

	/*
	 * Use 'co' as the marker to denote wl lines have finished, so
	 * reset the nick updates
	 */
	server->updatenicks = FALSE;

	/* If we're running in silent mode, parse the output for nicks/topic */
	if (server->silentwho) {

		/* Match group lines */
		len = strlen(match_group);
		if (strncmp(args[0], match_group, len) == 0) {

			group = g_strdup(args[0] + len);
			p = strchr(group, ' ');
			*p = '\0';

			/* Check for our particular group */
			len = strlen(group);
			if (g_ascii_strncasecmp(group, server->group->name, len) == 0) {

				/* Start matching nicks */
				server->updatenicks = TRUE;

				p = strstr(args[0], match_topic);
				if (p != NULL && p != args[0]) {
					topic = p + strlen(match_topic);
					if (topic != NULL) {
						len = strlen(match_topicunset);
						if (strncmp(topic,
							    match_topicunset,
							    len) != 0) {

							/* No way to find who set the topic, mark as unknown */
							icb_change_topic(server, topic, "unknown", time(NULL));
						}
					}
				}
			}
			g_free(group);
		}

		/*
		 * End of /who output, stop silent mode and signal front-end
		 * to display /names list
		 */
		len = strlen(match_total);
		if (strncmp(args[0], match_total, len) == 0) {
			server->silentwho = FALSE;
			signal_emit("channel joined", 1, server->group);
		}
	} else {
		/* Now that /topic works correctly, ignore server output */
		len = strlen(match_topicis);
		if (strncmp(args[0], match_topicis, len) != 0) {
			printtext(server, NULL, MSGLEVEL_CRAP, "%s", args[0]);
		}
	}
}

static void cmdout_wl(ICB_SERVER_REC *server, char **args)
{
	struct tm *logintime;
	char logbuf[20];
	char idlebuf[20];
	char line[255];
	time_t temptime;
	int op;

	/* "wl" : In a who listing, a line of output listing a user. Has the following format:

	* Field 1: String indicating whether user is moderator or not. Usually "*" for
	* moderator, and " " for not.
	* Field 2: Nickname of user.
	* Field 3: Number of seconds user has been idle.
	* Field 4: Response Time. No longer in use.
	* Field 5: Login Time. Unix time_t format. Seconds since Jan. 1, 1970 GMT.
	* Field 6: Username of user.
	* Field 7: Hostname of user.
	* Field 8: Registration status.
	*/
	temptime = strtol(args[4], NULL, 10);
	logintime = gmtime(&temptime);
	strftime(logbuf, sizeof(logbuf), "%b %e %H:%M", logintime);
	temptime = strtol(args[2], NULL, 10);
	idle_time(idlebuf, sizeof(idlebuf), temptime);

	/* Update nicklist */
	if (server->updatenicks) {
		op = FALSE;
#ifdef NO_MOD_SUPPORT_YET
		switch(args[0][0]) {
		case '*':
		case 'm':
			op = TRUE;
			break;
		}
#endif
		icb_nicklist_insert(server->group, args[1], op);
	}
	if (!server->silentwho) {
		snprintf(line, sizeof(line), "*** %c%-14.14s %6.6s %12.12s %s@%s %s",
			  args[0][0] == ' '?' ':'*', args[1], idlebuf, logbuf, args[5],
			  args[6], args[7]);
		printtext(server, NULL, MSGLEVEL_CRAP, line);
	}
}

static void cmdout_default(ICB_SERVER_REC *server, char **args)
{
	char *data;

	data = g_strjoinv(" ", args+1);
	if (!server->silentwho) {
		printtext(server, NULL, MSGLEVEL_CRAP, "%s", data);
	}
        g_free(data);
}

/*
 * args0 = "Arrive"
 * args0 = "<nickname> (<user>@<host>) entered group"
 */
static void status_arrive(ICB_SERVER_REC *server, char **args)
{
	char *nick, *p;

	nick = g_strdup(args[1]);
	p = strchr(nick, ' ');
	*p = '\0';
	/* XXX: new arrivals can still be moderator */
	icb_nicklist_insert(server->group, nick, FALSE);
        g_free(nick);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Depart"
 * args1 = "<nickname> (<user>@<host>) just left"
 */
static void status_depart(ICB_SERVER_REC *server, char **args)
{
	NICK_REC *nickrec;
	char *nick, *p;

	nick = g_strdup(args[1]);
	p = strchr(nick, ' ');
	*p = '\0';

	nickrec = nicklist_find(CHANNEL(server->group), nick);
	if (nickrec != NULL) {
		nicklist_remove(CHANNEL(server->group), nickrec);
	}
	g_free(nick);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Sign-on"
 * args1 = "<nickname> (<user>@<host>) entered group"
 */
static void status_signon(ICB_SERVER_REC *server, char **args)
{
	char *nick, *p;

	nick = g_strdup(args[1]);
	p = strchr(nick, ' ');
	*p = '\0';

	icb_nicklist_insert(server->group, nick, FALSE);

        g_free(nick);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Sign-off"
 * args1 = "<nickname> (<user>@<host>) has signed off."
 */
static void status_signoff(ICB_SERVER_REC *server, char **args)
{
	NICK_REC *nickrec;
	char *nick, *p;

	nick = g_strdup(args[1]);
	p = strchr(nick, ' ');
	*p = '\0';

	nickrec = nicklist_find(CHANNEL(server->group), nick);
	if (nickrec != NULL) {
		nicklist_remove(CHANNEL(server->group), nickrec);
	}
	g_free(nick);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * In theory should be status_status() but that's just silly :-)
 *
 * args0 = "Status"
 * args0 = "You are now in group <group>[ as moderator]"
 */
static void status_join(ICB_SERVER_REC *server, char **args)
{
	icb_update_nicklist(server);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Name"
 * args1 = "<oldnick> changed nickname to <newnick>"
 */
static void status_name(ICB_SERVER_REC *server, char **args)
{
	NICK_REC *nickrec;
	char *oldnick, *newnick, *p;

	oldnick = g_strdup(args[1]);
	p = strchr(oldnick, ' ');
	if (p != NULL) *p = '\0';

	p = g_strdup(args[1]);
	newnick = strrchr(p, ' ');
	if (newnick != NULL)
	       newnick++;

	nickrec = nicklist_find(CHANNEL(server->group), oldnick);
	if (nickrec != NULL)
		nicklist_rename(SERVER(server), oldnick, newnick);

	/* Update our own nick */
	if (strcmp(oldnick, server->connrec->nick) == 0) {
		server_change_nick(SERVER(server), newnick);
		g_free(server->connrec->nick);
		server->connrec->nick = g_strdup(newnick);
	}

	g_free(oldnick);
	g_free(p);

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Topic"
 * args1 = "<nickname> changed the topic to "<topic>"
 */
static void status_topic(ICB_SERVER_REC *server, char **args)
{
	char *topic, *setby, *p1, *p2;

	p1 = strchr(args[1], '"');
	p2 = strrchr(args[1], '"');

	if (p1++) {
		topic = g_strdup(p1);
		p2 = strrchr(topic, '"');
		*p2 = '\0';

		setby = g_strdup(args[1]);
		p2 = strchr(setby, ' ');
		*p2 = '\0';

		icb_change_topic(server, topic, setby, time(NULL));

		g_free(topic);
		g_free(setby);
	}

	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

/*
 * args0 = "Pass"
 * args1 = "<nickname> just passed you moderation of group <group>"
 * args1 = "<nickname> has passed moderation to <nickname>"
 * args1 = "<nickname> is now mod."
 *
 * If the moderator signs off and you are passed moderation, then the third
 * args1 is used.
 *
 */
static void status_pass(ICB_SERVER_REC *server, char **args)
{
	/*
	 * Eventually we might want to track this, for now just print status
	 * to the group window
	 */
	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
}

static void status_default(ICB_SERVER_REC *server, char **args)
{
	/* Send messages to the group window by default */
	printformat(server, server->group->name, MSGLEVEL_CRAP,
		    ICBTXT_STATUS, args[0], args[1]);
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

        signal_add("icb event error", (SIGNAL_FUNC) event_error);
        signal_add("icb event important", (SIGNAL_FUNC) event_important);
        signal_add("icb event beep", (SIGNAL_FUNC) event_beep);
        signal_add("icb event open", (SIGNAL_FUNC) event_open);
        signal_add("icb event personal", (SIGNAL_FUNC) event_personal);
        signal_add("icb cmdout co", (SIGNAL_FUNC) cmdout_co);
        signal_add("icb cmdout wl", (SIGNAL_FUNC) cmdout_wl);
        signal_add("default icb cmdout", (SIGNAL_FUNC) cmdout_default);
        signal_add("icb status arrive", (SIGNAL_FUNC) status_arrive);
        signal_add("icb status depart", (SIGNAL_FUNC) status_depart);
        signal_add("icb status sign-on", (SIGNAL_FUNC) status_signon);
        signal_add("icb status sign-off", (SIGNAL_FUNC) status_signoff);
        signal_add("icb status status", (SIGNAL_FUNC) status_join);
        signal_add("icb status topic", (SIGNAL_FUNC) status_topic);
        signal_add("icb status name", (SIGNAL_FUNC) status_name);
        signal_add("icb status pass", (SIGNAL_FUNC) status_pass);
        signal_add("default icb status", (SIGNAL_FUNC) status_default);

	signal_add("server add fill", (SIGNAL_FUNC) sig_server_add_fill);
	command_set_options("server add", "-icbnet");

	module_register("icb", "fe");
}

void fe_icb_deinit(void)
{
        signal_remove("icb event error", (SIGNAL_FUNC) event_error);
        signal_remove("icb event important", (SIGNAL_FUNC) event_important);
        signal_remove("icb event beep", (SIGNAL_FUNC) event_beep);
        signal_remove("icb event open", (SIGNAL_FUNC) event_open);
        signal_remove("icb event personal", (SIGNAL_FUNC) event_personal);
        signal_remove("icb cmdout co", (SIGNAL_FUNC) cmdout_co);
        signal_remove("icb cmdout wl", (SIGNAL_FUNC) cmdout_wl);
        signal_remove("default icb cmdout", (SIGNAL_FUNC) cmdout_default);
        signal_remove("icb status arrive", (SIGNAL_FUNC) status_arrive);
        signal_remove("icb status depart", (SIGNAL_FUNC) status_depart);
        signal_remove("icb status sign-on", (SIGNAL_FUNC) status_signon);
        signal_remove("icb status sign-off", (SIGNAL_FUNC) status_signoff);
        signal_remove("icb status status", (SIGNAL_FUNC) status_join);
        signal_remove("icb status topic", (SIGNAL_FUNC) status_topic);
        signal_remove("icb status name", (SIGNAL_FUNC) status_name);
        signal_remove("icb status pass", (SIGNAL_FUNC) status_pass);
        signal_remove("default icb status", (SIGNAL_FUNC) status_default);

	signal_remove("server add fill", (SIGNAL_FUNC) sig_server_add_fill);
}

#ifdef IRSSI_ABI_VERSION
void
fe_icb_abicheck(int * version)
{
	*version = IRSSI_ABI_VERSION;
}
#endif
