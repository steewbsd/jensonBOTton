/* JensonBotton, the best IRC bot in existence */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <regex.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cmds.h"

/* default bot name */
char NAME[] = "JensonBotton";
/* set logging verbosity */
#define DEBUG 1
/* socket protocol */
#define TCP 6
#define IRC_NONSSL_PORT 6667
#define IRC_ADDR "irc.libera.chat"
#define IRC_PASS "none"
#define IRC_NICK "JensonBotton_"
#define IRC_USER "JensonBotton_ * * JensonBotton"

#define PREFIX '-'

/* macros */
#if DEBUG
#define LOG(msg) printf("[DEBUG]: %s\n", (msg));
#else
#define LOG(msg) do { } while(0)
#endif

regex_t		 match;
const char  *filter = ":.+!.+@.+ PRIVMSG [#]+.+ :.*";

regex_t		 ping_message;
const char  *ping_filter = "PING .+";

/* DEBUG: NEEDS EXT. FREE */
/* get line (no LF) from char buffer, free when done */
int
sgetline(char * buf, char **line, int * ptr)
{
		*line = malloc(BUFSIZ);
		int i = 0;

		while (buf[*ptr] != '\n'
			&& buf[*ptr] != '\r'
			&& buf[*ptr] != EOF
			&& buf[*ptr] != 0
			&& *ptr < BUFSIZ) {
				/* copy buffer character to line */
				(*line)[i++] = buf[*ptr];
				(*ptr)++;
		}
		
		/* increase by one if we find a carriage return,
		   as IRC sends CRLF, so we skip the now useless LF */
		if (buf[*ptr] == '\r') {
				/* check if we reached an end \r\n, which would mean
				   we'd go out of bounds on the next iteration */
				if (*ptr + 2 >= BUFSIZ)
						return -1;
				/* otherwise skip the LF and continue reading */
				(*ptr) += 2;
				return 0;
		}
		/* don't increment if we reach EOF otherwise we'd be
		   out of bounds, also end reading */
		if (buf[*ptr] == EOF || buf[*ptr] == 0) 
				return -1;
		/* increase char pointer */
		/* check if we reached out of bounds */
		if (*ptr >= BUFSIZ) {
				return -1;
		}

		
		/* continue reading */
		return 0;
}

struct filter_cmd_response
filter_cmd(char * msg)
{
		regmatch_t   matchchar[10];
		int nmatch = 5;
		char * recv = NULL;
		recv = strndup(msg, BUFSIZ);
		struct filter_cmd_response response;
		response.type  = NONE;
		strcpy(response.nick, IRC_NICK);
		
		int ping_result = regexec(&ping_message,
			msg,
			nmatch,
			matchchar,
			0);
		if (ping_result == 0) {
				LOG("Received ping, responding...");
				strsep(&recv, " ");
				/* return message to PONG */
				response.type = PING;
				strcpy(response.reply, recv);
				memset(recv, 0, BUFSIZ);
				free(recv);
				return response;
		}

		int res = regexec(&match,
			msg,
			nmatch,
			matchchar,
			0);

		if (res == 0) {
				LOG("Received something from an user");
		} else {
				memset(recv, 0, BUFSIZ);
				free(recv);
				return response;
		}

		char * dup = NULL;
		dup = strdup(recv);
		char * chan = NULL;
		/* get the channel name */
		for (int i = 0; i < 3; i++) {
				if (dup != NULL)
						chan = strsep(&dup, " ");
		}
		strcpy(response.chan, chan);
		free(dup);
		LOG("Channel:");
		LOG(response.chan);
		
		/* get the sent message */
		for (int i = 0; i < 2; i++) {
				if (recv != NULL)
						strsep(&recv, ":");
				printf("Step: %d, %s\n", i, recv);
		}
		/* safety check */
		if (recv != NULL &&
			recv[0] == PREFIX) {
			LOG("received command");
		}
		/* get the actual command without the prefix */
		recv += 1;
		LOG("Command:");
		LOG(recv);
		cmd_handler(recv, &response);
		/* TODO: actually return something to reply */
		memset(recv, 0, BUFSIZ);
		free(recv);
		return response;
}

int
main ()
{
   /* initialize commands */
   init_commands();
   /* initialize regex */
   int compose = regcomp(&match,
	   filter,
	   REG_EXTENDED);
   if (compose != 0) {
		   /* TODO: err handling */
		   LOG("could not compose COMMAND regex");
		   exit(1);
   }
   
   int ping_compose = regcomp(&ping_message,
	   ping_filter,
	   REG_EXTENDED);
   if (ping_compose != 0) {
		   /* TODO: err handling */
		   LOG("could not compose PING regex");
		   exit(1);
   }
   
   /* initialize kqueue event for listening to PING messages */
   struct kevent	event;
   struct kevent	trigger;
   printf("welcome to %s\n", NAME);
   /* start a socket using the TCP protocol */
   int				sock = socket(PF_INET, SOCK_STREAM, TCP);
   if (!sock) exit(1);
   LOG("sock created successfully");
   LOG("trying to open socket...");
   
   /* resolve target server's hostname, and store the MSB address */
   struct hostent * name = gethostbyname(IRC_ADDR);

   /* TODO: replace with environment variables */
   struct sockaddr_in	saddr;
   saddr.sin_family = AF_INET;
   saddr.sin_port	= htons(IRC_NONSSL_PORT);

   /* copy hostname address to the sockaddr */
   memcpy(&saddr.sin_addr, name->h_addr_list[0], name->h_length);

   /* attempt to connect */
   int	res = connect(sock,
	   (struct sockaddr *)&saddr,
	   sizeof(saddr));
   if (res < 0) {
	   LOG("error when connecting to socket");
	   exit(1);
   }
   LOG("socket connected successfully");
   char			buffer[BUFSIZ+1];
   register int bytes;
   char			login[200];

   int	kq	= kqueue();
   EV_SET(&event, sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
   int	ret = kevent(kq, &event, 1, NULL, 0, NULL);
   if (ret == -1) {
	   LOG("could not register kqueue event");
	   exit(1);
   }

   /* initialize null-valued timespec to poll on kqueue */
   struct timespec	timeout;
   timeout.tv_sec  = 0;
   timeout.tv_nsec = 0;

   LOG("sending auth details...");
   sprintf(login,
	   "CAP LS 302\nPASS %s\n"
	   "NICK %s\nUSER %s\nCAP END\nJOIN #steew\n",
		   IRC_PASS,
		   IRC_NICK,
		   IRC_USER);
   write(sock, login, sizeof(login));

   for (;;) {
	   /* add trigger event, this will trigger when one of our queued
	   * events (see above in kevent() call) meets the filter, and
XS	   * will store the event information here.
	   * we add the timespec timeout, zero valued, so kevent is a
	   * non-blocking call */
	   ret = kevent(kq, NULL, 0, &trigger, 1, &timeout);
	   if (ret > 0) {
		   LOG("received something in network socket");
		   bytes = read(sock, buffer, BUFSIZ);

		   char *	line;
		   /* pass to sgetline to know if we have reached the
			  boundary (BUFSIZ) */
		   int		ptr = 0;
		   /* send buffer, get a line back, pass the ptr of the last
			  read character, and pass a count of the read
			  characters */
		   while(sgetline(buffer, &line, &ptr) == 0) {
				   struct filter_cmd_response response;
				   response = filter_cmd(line);
				   if (response.type == PING) {
						   char pong[BUFSIZ];
						   sprintf(pong, "PONG %s\n", response.reply);
						   LOG("responding with:");
						   LOG(pong);
						   LOG("end ping response");
						   write(sock, pong, sizeof(pong));
				   } else if (response.type == DEFAULT) {
						   LOG("writing:");
						   LOG(response.reply);
						   write(sock,
							   response.reply,
							   sizeof(response.reply));
				   }
				   memset(line          , 0, BUFSIZ);
				   memset(response.reply, 0, BUFSIZ);
				   memset(response.chan , 0, BUFSIZ);
				   free(line);
		   };
		   
		   /* clear the buffer for further use */
		   memset(buffer, 0, sizeof(buffer));
	   }
   }

   LOG("closing socket...");
   close(sock);
   regfree(&match);
   regfree(&ping_message);
}
