
#include "cmds.h"
#include <stdlib.h>
#include <string.h>

struct command_t *commands;
int		last_command;

void
cmd_hello(struct filter_cmd_response *response)
{
	char		reply[BUFSIZ];
	sprintf(reply,
		"\nPRIVMSG %s :Hi! My name is %s\n",
		response->chan,
		response->nick);
	strcpy(response->reply, reply);
	response->type = DEFAULT;
}

void
register_command(char *name,
		 void (*handler) (struct filter_cmd_response *))
{
	if (last_command >= MAX_COMMANDS) {
		puts("max number of commands reached");
		return;
	}
	/* register new commands in global commands var */
	strcpy(commands[last_command].name, name);
	commands[last_command].handler = handler;
	/* increase cmd pointer */
	last_command += 1;
}

void
init_commands()
{
	commands = malloc(MAX_COMMANDS * sizeof(struct command_t));
	last_command = 0;
	/* call for every command */
	register_command("hello", cmd_hello);
}

/* TODO: this could receive just one argument */
void
cmd_handler(char *cmd,
	    struct filter_cmd_response *response)
{

	/* TODO: implement bsearch */
	for (int i = 0; i <= MAX_COMMANDS; i++) {
		if (strcmp(commands[i].name, cmd) == 0) {
			commands[i].handler(response);
			return;
		}
	}
}
