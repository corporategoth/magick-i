/* Socket utility routines.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include <setjmp.h>


FILE **files;		/* Array of FILE *'s; files[s] = fdopen(s, "r+") */
int filescnt = 0;	/* Size of files array */


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
    if (setjmp(alarm_jmp))
	return (char *)-1;
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

int sockprintf(int s, char *fmt, ...)
{
    va_list args; int i;

    va_start(args, fmt);
    if (s >= filescnt) {
	int oldcnt = filescnt;
	filescnt *= 2;
	if (filescnt <= s)
	    filescnt = s+1;
	files = realloc(files, sizeof(FILE *) * filescnt);
	if (!files) {
	    filescnt = 0;
	    errno = ENOMEM;
	    return 0;
	}
	memset(files+oldcnt, 0, sizeof(FILE *) * (filescnt-oldcnt));
    }
    if (!files[s]) {
	if (!(files[s] = fdopen(s, "r+")))
	    return 0;
	setbuf(files[s], NULL);
    }
    i = vfprintf(files[s], fmt, args);
    fflush(files[s]);
    return i;
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

    if (filescnt <= sock) {
	int oldcnt = filescnt;
	filescnt *= 2;
	if (filescnt <= sock)
	    filescnt = sock+1;
	files = realloc(files, sizeof(FILE *) * filescnt);
	if (!files) {
	    filescnt = 0;
	    shutdown(sock, 2);
	    close(sock);
	    errno = ENOMEM;
	    return -1;
	}
	memset(files+oldcnt, 0, sizeof(FILE *) * (filescnt-oldcnt));
    }
    if (!(files[sock] = fdopen(sock, "r+"))) {
	int errno_save = errno;
	shutdown(sock, 2);
	close(sock);
	errno = errno_save;
	return -1;
    }
    setbuf(files[sock], NULL);

    if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
	int errno_save = errno;
	shutdown(sock, 2);
	fclose(files[sock]);
	errno = errno_save;
	return -1;
    }

    return sock;
}

void disconn(int s)
{
    shutdown(s, 2);
    if (s < filescnt)
	fclose(files[s]);
    else
	close(s);
}

