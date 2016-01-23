/* Routines for sending stuff to the network.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

/*************************************************************************/

/* Send a command to the server.  The two forms here are like
 * printf()/vprintf() and friends. */

void send_cmd(const char *source, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsend_cmd(source, fmt, args);
}

void vsend_cmd(const char *source, const char *fmt, va_list args)
{
    char buf[2048];	/* better not get this big... */

    vsprintf(buf, fmt, args);
    if (source) {
	sockprintf(servsock, ":%s %s\r\n", source, buf);
	if(debug)
	    log("debug: Sent: :%s %s", source, buf);
    } else {
	sockprintf(servsock, "%s\r\n", buf);
	if(debug)
	    log("debug: Sent: %s", buf);
    }
}

/*************************************************************************/

/* Send out a GLOBOPS.  This is called wallops() because it historically
 * sent a WALLOPS. */
void wallops(const char *whoami, const char *fmt, ...)
{
    va_list args;
    char buf[2048];

    va_start(args, fmt);
    sprintf(buf, "WALLOPS :%s", fmt);
    if (whoami)
	vsend_cmd(whoami, buf, args);
    else
	vsend_cmd(server_name, buf, args);
}

void globops(const char *whoami, const char *fmt, ...)
{
    va_list args;
    char buf[2048];

    va_start(args, fmt);
    sprintf(buf, "GLOBOPS :%s", fmt);
    if (whoami)
	vsend_cmd(whoami, buf, args);
    else
	vsend_cmd(server_name, buf, args);
}

/*************************************************************************/

/* Send a NOTICE from the given source to the given nick. */
void notice(const char *source, const char *dest, const char *fmt, ...)
{
    va_list args;
    char buf[2048];
    NickInfo *ni;

    if ((ni = findnick(dest)) && (ni->flags & NI_PRIVMSG) &&
    	(ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED)))
	privmsg(source, dest, fmt);
    else {
	va_start(args, fmt);
	sprintf(buf, "NOTICE %s :%s", dest, fmt);
	vsend_cmd(source, buf, args);
    }
}
void noticeall(const char *source, const char *fmt, ...)
{
    User *u;
    for (u = userlist;u;u = u->next)
	notice(source, u->nick, fmt);
}

/* Send a NULL-terminated array of text as NOTICEs. */
void notice_list(const char *source, const char *dest, const char **text)
{
    while (*text) {
    	/* Have to kludge around an ircII bug here: if a notice includes
    	 * no text, it is ignored, so we replace blank lines by lines
    	 * with a single space.
    	 */
	if (**text)
	    notice(source, dest, *text);
	else
	    notice(source, dest, " ");
	text++;
    }
}

/*************************************************************************/

/* Send a PRIVMSG from the given source to the given nick. */
void privmsg(const char *source, const char *dest, const char *fmt, ...)
{
    va_list args;
    char buf[2048];

    va_start(args, fmt);
    sprintf(buf, "PRIVMSG %s :%s", dest, fmt);
    vsend_cmd(source, buf, args);
}

/*************************************************************************/
