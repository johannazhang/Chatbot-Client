/*
 * The server should provide this string as banner, and the client should
 * verify that it matches exactly.
 */
#define chatserver_banner "chatserver 2"

/* maximum size of the user's "handle": */
#define MAXHANDLE 79

/* maximum size of one message, not including newline (nor \0) */
#define MAXMESSAGE 256