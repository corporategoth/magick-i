/* Socket utility routines.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include <setjmp.h>


static jmp_buf alarm_jmp;

static void alarm_handler(int sig_unused)
{
    longjmp(alarm_jmp, 1);
}

/*************************************************************************/

static int lastchar = EOF;

int sgetc(int s)
{
    unsigned char c;

    if (lastchar != EOF) {
	c = lastchar;
	lastchar = EOF;
	return c;
    }
    if (read(s, &c, 1) <= 0)
	return EOF;
    return c;
}

int sungetc(int c, int s)
{
    return lastchar = c;
}

/*************************************************************************/

/* If connection was broken, return NULL.  If the read timed out, return
 * (char *)-1. */
char *sgets(char *buf, unsigned int len, int s)
{
    int c;
    char *ptr = buf;

    if (len == 0)
	return NULL;
    if (setjmp(alarm_jmp)) {
	return (char *)-1;
    }
    signal(SIGALRM, alarm_handler);
    alarm(READ_TIMEOUT);
    c = sgetc(s);
    alarm(0);
    signal(SIGALRM, SIG_IGN);
    while (--len && (*ptr++ = c) != '\n' && (c = sgetc(s)) >= 0)
	;
    if (c < 0)
	return NULL;
    *ptr = 0;
    return buf;
}

/*************************************************************************/

int sputs(char *str, int s)
{
    return write(s, str, strlen(str));
}

int sockprintf(int s, char *fmt,...)
{
    va_list args;
    static char buf[16384];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    sputs(buf, s);
    return strlen(buf);
}

/*************************************************************************/

int conn(char *host, int port)
{
    struct hostent *hp;
    struct sockaddr_in sa;
    int sock;

    if (!(hp = gethostbyname(host)))
	return -1;

    bzero(&sa, sizeof(sa));
    bcopy(hp->h_addr, (char *)&sa.sin_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((unsigned short)port);
    if ((sock = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
	return -1;

    if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
	shutdown(sock, 2);
	close(sock);
	return -1;
    }

    return sock;
}

void disconn(int s)
{
    shutdown(s, 2);
    close(s);
}
