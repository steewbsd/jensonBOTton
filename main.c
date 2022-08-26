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

/* default bot name */
char NAME[] = "JensonBotton";
/* set logging verbosity */
#define DEBUG 0
/* socket protocol */
#define TCP 6
#define IRC_NONSSL_PORT 6667
#define IRC_ADDR "irc.libera.chat"
#define IRC_PASS "none"
#define IRC_NICK "JensonBotton_"
#define IRC_USER "JensonBotton_ * * JensonBotton"

#define PREFIX '>'

/* macros */
#if DEBUG
#define LOG(msg) printf("[DEBUG]: %s\n", (msg));
#else
#define LOG(msg) do { } while(0)
#endif

/* DEBUG: NEEDS EXT. FREE */
/* get line from char buffer, free when done */
char *
sgetline(char * buf, int * ptr)
{
		char * line;
		line = malloc(BUFSIZ);
		
		while (buf[*ptr] != EOF
			&& buf[*ptr] != '\n') {
				line[*ptr] = buf[*ptr];
				*ptr += 1;
		}
		/* increment char pointer to point to newline char */
		line[*ptr++] = '\n';
		return line;
}

char *
filter_cmd(char * msg)
{
		regex_t MESSAGE_MATCH;
		const char   *MESSAGE_FILT;
		MESSAGE_FILT = ":.+!.+@.+ PRIVMSG [#]+.+ :.*"; 
		regmatch_t           match[BUFSIZ];

		int compose = regcomp(&MESSAGE_MATCH,
			MESSAGE_FILT,
			REG_EXTENDED | REG_NOSUB);
		if (compose != 0) {
				LOG("could not compose regex");
				exit(1);
		}

		if (regexec(
					&MESSAGE_MATCH,
					msg,
					BUFSIZ,
					match,
					0) == 0) {
				puts("----- reveived a command -----");
				LOG("----- received an user message -----");
				
		}
		regfree(&MESSAGE_MATCH);
		/* char * recv = strdup(msg); */
		/* char * cmd = strsep(&recv, " "); */
		/* /\* safety check *\/ */
		/* if (cmd != NULL && */
		/* 	cmd[0] == PREFIX) { */
		/* 		LOG("received command"); */
		/* 		LOG(recv); */
		/* } */
		return NULL;
}

int
main ()
{
	/* initialize kqueue event for listening to PING messages */
   struct kevent event;
   struct kevent trigger;
   printf("welcome to %s\n", NAME);
   /* start a socket using the TCP protocol */
   int sock = socket(PF_INET, SOCK_STREAM, TCP);
   if (!sock) exit(1);
   LOG("sock created successfully");
   LOG("trying to open socket...");
   
   /* resolve target server's hostname, and store the MSB address */
   struct hostent * name = gethostbyname(IRC_ADDR);

   /* TODO: replace with environment variables */
   struct sockaddr_in saddr;
   saddr.sin_family = AF_INET;
   saddr.sin_port = htons(IRC_NONSSL_PORT);

   /* copy hostname address to the sockaddr */
   memcpy(&saddr.sin_addr, name->h_addr_list[0], name->h_length);

   /* attempt to connect */
   int res = connect(sock, (struct sockaddr *)&saddr, sizeof(saddr));
   if (res < 0) {
	   LOG("error when connecting to socket");
	   exit(1);
   }
   LOG("socket connected successfully");
   char buffer[BUFSIZ+1];
   register int bytes;
   char login[200];

   int kq = kqueue();
   EV_SET(&event, sock, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
   int ret = kevent(kq, &event, 1, NULL, 0, NULL);
   if (ret == -1) {
	   LOG("could not register kqueue event");
	   exit(1);
   }

   /* initialize null-valued timespec to poll on kqueue */
   struct timespec timeout;
   timeout.tv_sec = 0;
   timeout.tv_nsec = 0;

   LOG("Sending auth details...");
   sprintf(login, "CAP LS 302\nPASS %s\nNICK %s\nUSER %s\nCAP END\nJOIN #steew\n",
		   IRC_PASS,
		   IRC_NICK,
		   IRC_USER);
   write(sock, login, sizeof(login));

   for (;;) {
	   /* add trigger event, this will trigger when one of our queued
	   * events (see above in kevent() call) meets the filter, and
	   * will store the event information here.
	   * we add the timespec timeout, zero valued, so kevent is a
	   * non-blocking call */
	   ret = kevent(kq, NULL, 0, &trigger, 1, &timeout);
	   if (ret > 0) {
		   LOG("received something in network socket: ");
		   bytes = read(sock, buffer, BUFSIZ);

		   char * line;
		   int ptr = 0;
		   line = sgetline(buffer, &ptr);
//		   printf("Ptr: %d, Line [%d]: %s\n",ptr ,i, line);
		   		   
		   char * recv = strdup(buffer);
		   char * cmd = strsep(&recv, " ");
		   printf("Received ----->%s\n", cmd);
		   if (strcasecmp("PING", cmd) == 0) {
				   LOG("received ping request, responding...");
				   char pong[BUFSIZ];
				   sprintf(pong, "PONG %s\n", recv);
				   write(sock, pong, sizeof(pong));
		   } else
				   filter_cmd(line);
		   free(line);
		   free(recv);
		   write(STDOUT_FILENO, buffer, bytes);
		   LOG("entire socket was read");
		   /* clear the buffer for further use */
		   memset(buffer, 0, sizeof(buffer));
	   }
   }

   LOG("closing socket...");
   close(sock);
}


