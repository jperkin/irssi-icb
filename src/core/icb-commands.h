#ifndef __ICB_COMMANDS_H
#define __ICB_COMMANDS_H

#include "commands.h"

#define command_bind_icb(cmd, section, signal) \
        command_bind_proto(cmd, ICB_PROTOCOL, section, signal)
#define command_bind_icb_first(cmd, section, signal) \
        command_bind_proto_first(cmd, ICB_PROTOCOL, section, signal)
#define command_bind_icb_last(cmd, section, signal) \
        command_bind_proto_last(cmd, ICB_PROTOCOL, section, signal)

/* Simply returns if server isn't for ICB protocol. Prints ERR_NOT_CONNECTED
   error if there's no server or server isn't connected yet */
#define CMD_ICB_SERVER(server) \
	G_STMT_START { \
          if (server != NULL && !IS_ICB_SERVER(server)) \
            return; \
          if (server == NULL || !(server)->connected) \
            cmd_return_error(CMDERR_NOT_CONNECTED); \
	} G_STMT_END

void icb_commands_init(void);
void icb_commands_deinit(void);

#endif
