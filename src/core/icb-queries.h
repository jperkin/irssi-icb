#ifndef __ICB_QUERIES_H
#define __ICB_QUERIES_H

#include "queries.h"
#include "icb-servers.h"

/* Returns ICB_QUERY_REC if it's ICB query, NULL if it isn't. */
#define ICB_QUERY(query) \
	PROTO_CHECK_CAST(QUERY(query), QUERY_REC, chat_type, "ICB")

#define IS_ICB_QUERY(query) \
	(ICB_QUERY(query) ? TRUE : FALSE)

QUERY_REC *icb_query_create(const char *server_tag,
			    const char *nick, int automatic);

#define icb_query_find(server, name) \
	query_find(SERVER(server), name)

#endif
