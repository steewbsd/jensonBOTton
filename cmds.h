#ifndef _CMDS_H_
#define _CMDS_H_
#include <stdio.h>

#define MAX_COMMANDS 50

struct filter_cmd_response {
		/* message to write */
		char reply[BUFSIZ];
		/* origin channel */
		char chan[BUFSIZ];
		/* irc nickname */
		char nick[20];
		/* type of reply */
		enum {PING, NONE, DEFAULT} type;
};

struct command_t {
		/* function name */
		char name[20];
		/* function handler */
		void (* handler)(struct filter_cmd_response *);
};

void
cmd_handler(char * cmd, struct filter_cmd_response * response);

void
init_commands();

#endif /* _CMDS_H_ */
