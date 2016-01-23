/* Routines for sending stuff to the network.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

/*************************************************************************/

/* Send a command to the server.  The two forms here are like
 * printf()/vprintf() and friends. */

void
send_cmd (const char *source, const char *fmt,...)
{
    va_list args;

    va_start (args, fmt);
    vsend_cmd (source, fmt, args);
    va_end (args);
}

void
vsend_cmd (const char *source, const char *fmt, va_list args)
{
    char buf[2048];		/* better not get this big... */

    if (runflags & RUN_NOSEND)	/* Run silent, Run deep! */
	return;

    vsnprintf (buf, sizeof (buf), fmt, args);
    if (source)
    {
	if (outlet_on==TRUE&&(stricmp (source, s_OperServ) == 0 && finduser (s_OperServ)))
	    sockprintf (servsock, ":%s %s\r\n", s_Outlet, buf);
	else
	    sockprintf (servsock, ":%s %s\r\n", source, buf);
	if (runflags & RUN_DEBUG)
	    write_log ("debug: Sent: :%s %s", source, buf);
    }
    else
    {
	sockprintf (servsock, "%s\r\n", buf);
	if (runflags & RUN_DEBUG)
	    write_log ("debug: Sent: %s", buf);
    }
}

/*************************************************************************/

/* Send out a GLOBOPS.  This is called wallops() because it historically
 * sent a WALLOPS -- Still does on NON-DALnet servers. */
void
wallops (const char *whoami, const char *fmt,...)
{
    va_list args;
    char buf[2048];

    va_start (args, fmt);
#ifdef IRC_DALNET
    snprintf (buf, sizeof (buf), "GLOBOPS :%s", fmt);
#else
    snprintf (buf, sizeof (buf), "WALLOPS :%s", fmt);
#endif

    if (whoami)
	vsend_cmd (whoami, buf, args);
    else
	vsend_cmd (server_name, buf, args);
    va_end (args);
}

/*************************************************************************/

/* Send a NOTICE from the given source to the given nick. */
void
notice (const char *source, const char *dest, const char *fmt,...)
{
    va_list args;
    char buf[2048];
    NickInfo *ni;

    va_start (args, fmt);
    if (nickserv_on && ((ni = findnick (dest)) && (host (ni)->flags & NI_PRIVMSG) &&
	(ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED))))
	snprintf (buf, sizeof (buf), "PRIVMSG %s :%s", dest, fmt);
    else
	snprintf (buf, sizeof (buf), "NOTICE %s :%s", dest, fmt);
    vsend_cmd (source, buf, args);
    va_end (args);
}
void
noticeall (const char *source, const char *fmt,...)
{
    User *u;
    for (u = userlist; u; u = u->next)
	notice (source, u->nick, fmt);
}

/* Send a NULL-terminated array of text as NOTICEs. */
void
notice_list (const char *source, const char *dest, const char **text)
{
    while (*text)
    {
	/* Have to kludge around an ircII bug here: if a notice includes
	 * no text, it is ignored, so we replace blank lines by lines
	 * with a single space.
	 */
	if (**text)
	    notice (source, dest, *text);
	else
	    notice (source, dest, " ");
	text++;
    }
}

/*************************************************************************/
