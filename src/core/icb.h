#ifndef __ICB_H
#define __ICB_H

#define DEFAULT_ICB_GROUP "1" /* default group to join if none is specified */

typedef struct _ICB_SERVER_CONNECT_REC ICB_SERVER_CONNECT_REC;
typedef struct _ICB_SERVER_REC ICB_SERVER_REC;
typedef struct _ICB_CHANNEL_REC ICB_CHANNEL_REC;

#define ICB_PROTOCOL (chat_protocol_lookup("ICB"))

char **icb_split(const char *data, int count);
void icb_split_free(char **args);

#endif
