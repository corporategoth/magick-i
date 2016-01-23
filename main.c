/* Magick -- main source file.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file doc/COPYING
 * for details.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "services.h"


/******** Global variables! ********/

/* These can all be set by options. */
char config_file[512] = CONFIG_FILE;		/* -config file */
char services_dir[512] = SERVICES_DIR;		/* -dir directory */

char remote_server[256];	/* -remote server[:port] */
int remote_port;
char password[512];
char server_name[256];		/* -name servername */
char server_desc[128];		/* -desc serverdesc */
char services_user[512];	/* -user username */
char services_host[512];	/* -host hostname */
char services_prefix[NICKMAX];	/* -prefix prefix */
char log_filename[512];		/* -log filename */
int update_timeout;		/* -update secs */
int ping_frequency;		/* -ping secs */
int server_relink = -1;		/* -relink secs or -norelink */
int services_level;		/* -level level */
float tz_offset;		/* -offset hours */
Boolean_T nickserv_on = FALSE;
Boolean_T chanserv_on = FALSE;
Boolean_T helpserv_on = FALSE;
Boolean_T irciihelp_on = FALSE;
Boolean_T memoserv_on = FALSE;
Boolean_T memos_on = FALSE;
Boolean_T news_on = FALSE;
Boolean_T devnull_on = FALSE;
Boolean_T operserv_on = FALSE;
Boolean_T outlet_on = FALSE;
Boolean_T akill_on = FALSE;
Boolean_T clones_on = FALSE;
Boolean_T globalnoticer_on = FALSE;
Boolean_T show_sync_on = FALSE;
Boolean_T on_boot = TRUE;
char pid_filename[512];
int file_version = 5;

/* What gid should we give to all files?  (-1 = don't set) */
gid_t file_gid = -1;

/* Contains a message as to why services is terminating */
char *quitmsg = NULL;

/* Input buffer - global, so we can dump it if something goes wrong */
char inbuf[BUFSIZE];

/* Socket for talking to server */
int servsock = -1;

/* At what time were we started? */
time_t start_time;

/* At what time were we last reset (RELOAD, HUP, SQUIT, crash, etc)? */
time_t reset_time;

/* When did we last update the databases? */
time_t last_update;

/* Reason for being OFF if it IS off */
char *offreason = NULL;

/* Nice version of all the ugly 0/1 veriables! */
long runflags = 0;

/* amount of non-*, ? and . chars required */
int starthresh;


/******** Local variables! ********/

/* Set to 1 if we are waiting for input */
static int waiting = 0;

/* If we get a signal, use this to jump out of the main loop. */
static jmp_buf panic_jmp;

/* Time to wait before respawn if SQUIT */
static int waittime = 5;

/* Offset for services nick's LOGON TIME values (changed later
 * if WIERD_COLLIDE is not defined) */
static int offset = 65000;

char s_DevNull[NICKMAX];

/*************************************************************************/

/* If we get a weird signal, come here. */

static void
sighandler (int signum)
{
    if (runflags & RUN_STARTED)
    {
	/* Prepare for a possible SIGKILL -- Typical shutdown sequence */
	if (signum == SIGINT && (runflags & RUN_LIVE))
	{
	    if (fork () < 0) {
		runflags = RUN_TERMINATING | RUN_QUITTING;
		log_perror ("fork()");
	    } else
		runflags &= ~RUN_LIVE;
	}
	else if (signum == SIGTERM)
	{
	    runflags |= (RUN_SIGTERM | RUN_SAVE_DATA);
	    signal (SIGTERM, SIG_IGN);
	    return;
	}
	else if (!waiting)
	{
	    write_log ("PANIC! buffer = %s", inbuf);
	    /* Cut off if this would make IRC command >510 characters. */
	    if (strlen (inbuf) > 448)
	    {
		inbuf[446] = '>';
		inbuf[447] = '>';
		inbuf[448] = 0;
	    }
	    wallops (NULL, "PANIC! buffer = %s\r\n", inbuf);
	}
	else if (waiting < 0)
	{
	    if (waiting == -1)
	    {
		write_log ("PANIC! in timed_update (%s)", strsignal (signum));
		wallops (NULL, "PANIC! in timed_update (%s)", strsignal (signum));
	    }
	    else
	    {
		write_log ("PANIC! waiting=%d (%s)", waiting, strsignal (signum));
		wallops (NULL, "PANIC! waiting=%d (%s)", waiting, strsignal (signum));
	    }
	}
    }
    if (signum == SIGUSR1 || !(quitmsg = malloc (BUFSIZE)))
    {
	quitmsg = "Out of memory!";
	runflags |= RUN_QUITTING;
    }
    else
    {
#ifndef WIN32
	if (signum == SIGHUP)
	{
	    runflags |= RUN_NOSLEEP;
	    snprintf (quitmsg, BUFSIZE, "Magick Recieved SIGHUP -- Reloading!");
	}
	else
#endif
#if HAVE_STRSIGNAL
	    snprintf (quitmsg, BUFSIZE, "Magick terminating: %s", strsignal (signum));
#else
	    snprintf (quitmsg, BUFSIZE, "Magick terminating on signal %d", signum);
#endif
	runflags |= RUN_QUITTING;
    }
    if (runflags & RUN_STARTED)
	longjmp (panic_jmp, 1);
    else
    {
	write_log ("%s", quitmsg);
	exit (1);
    }
}

/*************************************************************************/

/* Log stuff to stderr with a datestamp. */

void
write_log (const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256];

    va_start (args, fmt);
    time (&t);
    tm = *localtime (&t);
    strftime (buf, sizeof (buf) - 1, "[%b %d %H:%M:%S %Y] ", &tm);
    if (runflags & RUN_LOG_IS_OPEN)
    {
	fputs (buf, stderr);
	vfprintf (stderr, fmt, args);
	fputc ('\n', stderr);
	fflush (stderr);
	if (runflags & RUN_LIVE)
	{
	    fputs (buf, stdout);
	    vfprintf (stdout, fmt, args);
	    fputc ('\n', stdout);
	    fflush (stdout);
	}
    }
}

/* Like log(), but tack a ": " and a system error message (as returned by
 * strerror() onto the end.
 */

void
log_perror (const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256];

    va_start (args, fmt);
    time (&t);
    tm = *localtime (&t);
    strftime (buf, sizeof (buf) - 1, "[%b %d %H:%M:%S %Y] ", &tm);
    if (runflags & RUN_LOG_IS_OPEN)
    {
	fputs (buf, stderr);
	vfprintf (stderr, fmt, args);
	fprintf (stderr, ": %s\n", strerror (errno));
	fflush (stderr);
	if (runflags & RUN_LIVE)
	{
	    fputs (buf, stdout);
	    vfprintf (stdout, fmt, args);
	    fprintf (stdout, ": %s\n", strerror (errno));
	    fflush (stdout);
	}
    }
}

/*************************************************************************/

/* We've hit something we can't recover from.  Let people know what
 * happened, then go down.
 */

void
fatal (const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256], buf2[4096];

    va_start (args, fmt);
    time (&t);
    tm = *localtime (&t);
    strftime (buf, sizeof (buf) - 1, "[%b %d %H:%M:%S %Y]", &tm);
    vsnprintf (buf2, sizeof (buf2), fmt, args);
	fprintf (stderr, "%s FATAL: %s\n", buf, buf2);
	fflush (stderr);
	if (runflags & RUN_LIVE && !(runflags & RUN_LOG_IS_OPEN))
	{
	    fprintf (stdout, "%s FATAL: %s\n", buf, buf2);
	    fflush (stdout);
	}
    if (servsock >= 0)
	wallops (NULL, "FATAL ERROR!  %s", buf2);
    exit (1);
}

/* Same thing, but do it like perror().
 */

void
fatal_perror (const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256], buf2[4096];

    va_start (args, fmt);
    time (&t);
    tm = *localtime (&t);
    strftime (buf, sizeof (buf) - 1, "[%b %d %H:%M:%S %Y]", &tm);
    vsnprintf (buf2, sizeof (buf2), fmt, args);
	fprintf (stderr, "%s FATAL: %s: %s\n", buf, buf2, strerror (errno));
	fflush (stderr);
	if (runflags & RUN_LIVE && !(runflags & RUN_LOG_IS_OPEN))
	{
	    fprintf (stdout, "%s FATAL: %s: %s\n", buf, buf2, strerror (errno));
	    fflush (stdout);
	}
    if (servsock >= 0)
	wallops (NULL, "FATAL ERROR!  %s", buf2);
    exit (1);
}

/*************************************************************************/

int
get_file_version (FILE * f, const char *filename)
{
    int version = fgetc (f) << 24 | fgetc (f) << 16 | fgetc (f) << 8 | fgetc (f);
    if (ferror (f) || feof (f))
	fatal_perror ("Error reading version number on %s", filename);
    else if (version > file_version || version < 1)
	fatal ("Invalid version number (%d) on %s", version, filename);
    return version;
}

void
write_file_version (FILE * f, const char *filename)
{
    if (
	   fputc (file_version >> 24 & 0xFF, f) < 0 ||
	   fputc (file_version >> 16 & 0xFF, f) < 0 ||
	   fputc (file_version >> 8 & 0xFF, f) < 0 ||
	   fputc (file_version & 0xFF, f) < 0
	)
	fatal_perror ("Error writing version number on %s", filename);
}

/*************************************************************************/

/* Send a NICK command for the given pseudo-client.  If `user' is NULL,
 * send NICK commands for all the pseudo-clients. */

#if defined(IRC_DALNET)
#ifdef DAL_SERV
#define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s 1 %ld %s %s %s 1 :%s", (nick), (offset-services_level)*10, \
		services_user, services_host, server_name, (name)); \
    } while (0)
#else
#define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s 1 %ld %s %s %s :%s", (nick), (offset-services_level)*10, \
		services_user, services_host, server_name, (name)); \
    } while (0)
#endif
#elif defined(IRC_UNDERNET)
#define NICK(nick,name) \
    do { \
	send_cmd(server_name, "NICK %s 1 %ld %s %s %s :%s", (nick), (offset-services_level)*10,\
		services_user, services_host, server_name, (name)); \
    } while (0)
#elif defined(IRC_TS8)
#define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s :1", (nick)); \
	send_cmd((nick), "USER %ld %s %s %s :%s", (offset-services_level)*10, \
		services_user, services_host, server_name, (name)); \
    } while (0)
#else
#define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s :1", (nick)); \
	send_cmd((nick), "USER %s %s %s :%s", \
		services_user, services_host, server_name, (name)); \
    } while (0)
#endif

int
i_am_backup ()
{
    int Result=0;
    if(nickserv_on==TRUE)
	if(finduser(s_NickServ))
	    Result=1;
    if(chanserv_on==TRUE)
	if(finduser(s_ChanServ))
	    Result=1;
    if(helpserv_on==TRUE)
	if(finduser(s_HelpServ))
	    Result=1;
    if(irciihelp_on==TRUE)
	if(finduser(s_IrcIIHelp))
	    Result=1;
    if(memoserv_on==TRUE)
	if(finduser(s_MemoServ))
	    Result=1;
    if(operserv_on==TRUE)
	if(finduser(s_OperServ))
	    Result=1;
    if(devnull_on==TRUE)
	if(finduser(s_DevNull))
	    Result=1;
    if(globalnoticer_on==TRUE)
	if(finduser(s_GlobalNoticer))
	    Result=1;
    return Result;
}

int
is_services_nick (const char *nick)
{
    if((nickserv_on==TRUE && stricmp(nick, s_NickServ)==0)
	|| (chanserv_on==TRUE && stricmp(nick, s_ChanServ)==0)
	|| (helpserv_on==TRUE && stricmp(nick, s_HelpServ)==0)
	|| (irciihelp_on==TRUE && stricmp(nick, s_IrcIIHelp)==0)
	|| (memoserv_on==TRUE && stricmp(nick, s_MemoServ)==0)
	|| (operserv_on==TRUE && stricmp(nick, s_OperServ)==0)
	|| (devnull_on==TRUE && stricmp(nick, s_DevNull)==0)
	|| (globalnoticer_on==TRUE && stricmp(nick, s_GlobalNoticer)==0)
	|| (outlet_on==TRUE && stricmp(nick, s_Outlet)==0))
	    return 1;
    else
	return 0;
}

const char *
any_service ()
{
    if(outlet_on==TRUE)
	return s_Outlet;
    else if(operserv_on==TRUE)
	return s_OperServ;
    else if(nickserv_on==TRUE)
	return s_NickServ;
    else if(chanserv_on==TRUE)
	return s_ChanServ;
    else if(memoserv_on==TRUE)
	return s_MemoServ;
    else if(globalnoticer_on==TRUE)
	return s_GlobalNoticer;
    else if(helpserv_on==TRUE)
	return s_HelpServ;
    else if(irciihelp_on==TRUE)
	return s_IrcIIHelp;
    else if(devnull_on==TRUE)
	return s_DevNull;
    else
	return NULL;
}

static void
check_introduce (const char *nick, const char *name)
{
    User *u;

    if (!(u = finduser (nick)))
	NICK (nick, name);
    else
#ifdef WIERD_COLLIDE
    if (u->signon < offset - services_level)
#else
    if (u->signon > offset - services_level)
#endif
	NICK (nick, name);
}

void
introduce_user (const char *user)
{
    if(nickserv_on==TRUE)
	if (!user || stricmp (user, s_NickServ) == 0)
	    check_introduce (s_NickServ, "Nickname Server");
    if(chanserv_on==TRUE)
	if (!user || stricmp (user, s_ChanServ) == 0)
	    check_introduce (s_ChanServ, "Channel Server");
    if(helpserv_on==TRUE)
	if (!user || stricmp (user, s_HelpServ) == 0)
	    check_introduce (s_HelpServ, "Help Server");
    if(irciihelp_on==TRUE)
	if (!user || stricmp (user, s_IrcIIHelp) == 0)
	    check_introduce (s_IrcIIHelp, "ircII Help Server");
    if(memoserv_on==TRUE)
	if (!user || stricmp (user, s_MemoServ) == 0)
	    check_introduce (s_MemoServ, "Memo Server");
    if(devnull_on==TRUE)
	if (!user || stricmp (user, s_DevNull) == 0)
	{
	    check_introduce (s_DevNull, "/dev/null -- message sink");
	    send_cmd (s_DevNull, "MODE %s +i", s_DevNull);
	}
    if(operserv_on==TRUE)
	if (!user || stricmp (user, s_OperServ) == 0)
	{
    	    check_introduce (s_OperServ, "Operator Server");
	    send_cmd (s_OperServ, "MODE %s +i", s_OperServ);
	}
    if(globalnoticer_on==TRUE)
	if (!user || stricmp (user, s_GlobalNoticer) == 0)
	{
	    check_introduce (s_GlobalNoticer, "Global Noticer");
	    send_cmd (s_GlobalNoticer, "MODE %s +io", s_GlobalNoticer);
	}
    if(outlet_on==TRUE)
	if (!user || stricmp (user, s_Outlet) == 0)
	{
	    check_introduce (s_Outlet, "Magick Outlet");
	    send_cmd (s_Outlet, "MODE %s +i", s_Outlet);
	}
}

#undef NICK

/*************************************************************************/

/* Is the 'user' a server? */
int
is_server (const char *nick)
{
    unsigned int i;

    for (i = 0; i < strlen (nick) && nick[i] != '.'; i++);

    if (nick[i] == '.')
	return 1;
    return 0;
}

/*************************************************************************/

/* Reset in preperation for re-start */
void
reset_dbases ()
{
    User *u, *un;
    NickInfo *ni, *nin;
    ChannelInfo *ci, *cin;
    MemoList *ml, *mln;
    NewsList *nl, *nln;
    Clone *c, *cn;
    Timeout *to, *ton;
    Timer *t, *tn;
    int i;

/* Some of these output ... stop that. */
    runflags |= RUN_NOSEND;

/* Clear active users (and channels) */
    u = userlist;
    while (u)
    {
	un = u->next;
	delete_user (u);
	u = un;
    }
    userlist = NULL;
    usercnt = opcnt = 0;

/* SQUIT all servers */
    while (servcnt)
    {
	free (servlist[0].server);
	free (servlist[0].desc);
	servcnt--;
	bcopy (servlist + 1, servlist, sizeof (*servlist) * servcnt);
    }
    servlist = NULL;
    serv_size = 0;

/* Ignore smignore */
    if(devnull_on==TRUE)
    {
	while (ignorecnt)
	{
	    ignorecnt--;
	    bcopy (ignore + 1, ignore, sizeof (*ignore) * ignorecnt);
	}
	ignore = NULL;
	ignore_size = 0;
    }

/* Nope, we got NO MORE sops! */
    if(operserv_on==TRUE)
    {
	while (nsop)
	{
	    --nsop;
	    bcopy (sops + 1, sops, sizeof (*sops) * nsop);
	}
	sop_size = 0;
    }

/* I OWN your ass! */
    if(nickserv_on==TRUE)
    {
	for (i = 33; i < 256; ++i)
	{
	    ni = nicklists[i];
	    while (ni)
	    {
		nin = ni->next;
		delnick (ni);
		ni = nin;
	    }
	    nicklists[i] = NULL;
	}
    }

/* muaahahahaha */
    if(chanserv_on==TRUE)
    {
	for (i = 33; i < 256; ++i)
	{
	    ci = chanlists[i];
	    while (ci)
	    {
		cin = ci->next;
		delchan (ci);
		ci = cin;
	    }
	    chanlists[i] = NULL;
	}
    }

/* Lost in the mail? */
    if(memoserv_on==TRUE)
    {
	if (memos_on==TRUE) {
	    for (i = 33; i < 256; ++i ) {
		ml = memolists[i];
		while (ml)
		{
		    mln = ml->next;
		    del_memolist(ml);
		    ml = mln;
		}
		memolists[i] = NULL;
	    }    
	}
	if (news_on==TRUE) {
	    for (i = 33; i < 256; ++i ) {
		nl = newslists[i];
		while (nl)
		{
		    nln = nl->next;
		    del_newslist(nl);
		    nl = nln;
		}
		newslists[i] = NULL;
	    }    
	}
    }

/* Lost in the mail? */
    if(memoserv_on==TRUE)
    {
	if (memos_on==TRUE) {
	    for (i = 33; i < 256; ++i ) {
		ml = memolists[i];
		while (ml)
		{
		    mln = ml->next;
		    del_memolist(ml);
		    ml = mln;
		}
		memolists[i] = NULL;
	    }    
	}
	if (news_on==TRUE) {
	    for (i = 33; i < 256; ++i ) {
		nl = newslists[i];
		while (nl)
		{
		    nln = nl->next;
		    del_newslist(nl);
		    nl = nln;
		}
		newslists[i] = NULL;
	    }    
	}
    }

/* Awwww, arent we nice ... */
    if(akill_on==TRUE)
    {
	while (nakill)
	    del_akill (akills[0].mask, 1);
	akills = NULL;
	akill_size = 0;
    }

/* bleep ... bleep */
    if(clones_on==TRUE)
    {
	while (nclone)
	    del_clone (clones[0].host);
	clone_size = 0;
	c = clonelist;
	while (c)
	{
	    cn = c->next;
	    while (c->amount > 1)
		clones_del (c->host);
	    clones_del (c->host);
	    c = cn;
	}
	clonelist = NULL;
	clones = NULL;
    }

/* Keep the plebs IGNORANT! */
    if(globalnoticer_on==TRUE)
    {
	while (nmessage)
	{
	    free (messages[0].text);
	    --nmessage;
	    bcopy (messages + 1, messages, sizeof (*messages) * nmessage);
	}
	message_size = 0;
	messages = NULL;
    }

/* Tick ... Tick ... Tick ... BOOM! */
    if(nickserv_on==TRUE)
    {
	to = timeouts;
	while (to)
	{
	    ton = to->next;
	    free (to);
	    to = ton;
	}
	timeouts = NULL;
    }

/*    t = pings;
    while (t)
    {
	tn = t->next;
	deltimer (t->label);
	t = tn;
    }
    pings = NULL; */

    runflags &= ~RUN_NOSEND;
}

/*************************************************************************/

void
open_log ()
{
    /* Redirect stderr to logfile. */
    if (!(runflags & RUN_LOG_IS_OPEN))
	if (freopen (log_filename, "a", stderr))
	    runflags |= RUN_LOG_IS_OPEN;
	else
	    write_log ("Cannot open %s, no logging output used.", log_filename);
    else
	write_log ("Tried to open log, but already open!");
}
void
close_log ()
{
    if ((runflags & RUN_LOG_IS_OPEN))
	fclose (stderr);
    runflags &= ~RUN_LOG_IS_OPEN;
}

/*************************************************************************/

void
do_help (char *prog)
{

printf("
Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
    E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
This program is free but copyrighted software; see the file COPYING for
details.  Please do not forget to read ALL the files in the doc directory.

Syntax: %s [opts]

-remote server[:port]	Connect to the specified server.
-name servername	Our server name (e.g. hell.darker.net).
-desc string		Description of us (e.g. DarkerNet Services).
-user username		Username for Services' nicks (e.g. services).
-host hostname		Hostname for Services' nicks (e.g. darker.net).
-prefix prefix		Prefix for Magick Outlet (see magick.ini)
			    (e.g. \"Magick-\").
-dir directory		Directory containing Services' data files
			    (e.g. /usr/local/lib/services).
-config filename	Config file to be used (e.g. magick.ini).
-log filename		Services log filename (e.g. magick.log).
-update secs		How often to update databases, in seconds.
-debug			Enable debugging mode--more info sent to log.
-relink secs		Specify seconds to re-attempt link if SQUIT.
-norelink		Die on SQUIT or kill -1.
-level level		Specify services level (>1 = backup).
-offset hours		Specify the TimeZone offset for backups.
-live			Dont fork (log screen + log file).
", prog);

/*
-ping sec		Specify how often to ping servers for lag.
*/

}

/* Main routine.  (What does it look like? :-) ) */
/* (Yes, big and ugly, I know...) */

int
main (int ac, char **av)
{
    time_t last_ping;
    time_t last_check;		/* When did we last check NickServ timeouts? */
    int i;
    char *progname;
    char *s, *t;
#ifdef RUNGROUP
#ifndef WIN32
    struct group *gr;
#endif
    char *errmsg = NULL;
#endif
    FILE *pidfile;
#ifdef WIN32
    WORD wsaVersionRequested;
    WSADATA wsaData;
    int wsaerr;
    wsaVersionRequested = MAKEWORD (1, 1);
    if (wsaerr != 0)
	/* Tell the user that we couldn't find a useable */
	/* winsock.dll.     */
    {
	printf ("Error: Unable to find a valid winsock.dll\n");
	return -1;
    }

    /* Confirm that the Windows Sockets DLL supports 1.1. */
    /* Note that if the DLL supports versions greater */
    /* than 1.1 in addition to 1.1, it will still return */
    /* 1.1 in wVersion since that is the version we */
    /* requested. */

    if (LOBYTE (wsaData.wVersion) != 1 || HIBYTE (wsaData.wVersion) != 1)
    {
	/* Tell the user that we couldn't find a useable */
	/* winsock.dll. */
	WSACleanup ();
	printf ("Error: Unable to find a valid winsock.dll\n");
	return -1;
    }
    /* The Windows Sockets DLL is acceptable. Proceed. */

#endif

#ifdef DEFUMASK
    umask (DEFUMASK);
#endif

#ifdef RUNGROUP
#ifndef WIN32
    setgrent ();
    while (gr = getgrent ())
    {
	if (strcmp (gr->gr_name, RUNGROUP) == 0)
	    break;
    }
    endgrent ();
    if (gr)
	file_gid = gr->gr_gid;
    else
	errmsg = "Group not known";
#endif
#endif

	/* Find program name. */
    if (progname = strrchr (av[0], '/'))
	++progname;
    else
	progname = av[0];


    /* Parse -dir, -config and -help ahead of time. */
    for (i = 1; i < ac; ++i)
    {
	s = av[i];
	if (*s == '-' || *s == '/')
	{
	    ++s;
	    if (stricmp (s, "dir") == 0)
	    {
		if (++i >= ac)
		{
		    fprintf (stderr, ERR_REQ_PARAM, "-dir");
		    return 1;
		}
		strcpy (services_dir, av[i]);
	    }
	    else if (stricmp (s, "config") == 0)
	    {
		if (++i >= ac)
		{
		    fprintf (stderr, ERR_REQ_PARAM, "-config");
		    return 1;
		}
		strcpy (config_file, av[i]);
	    }
	    else if (stricmp (s, "help") == 0 || stricmp (s, "-help") == 0 ||
	 			   			stricmp (s, "?") == 0)
	    {
		do_help(av[0]);
		return 0;
	    }
	}
    }

    /* Chdir to services directory. */
    if (chdir (services_dir) < 0)
    {
	perror (services_dir);
#ifdef WIN32
	WSACleanup ();
#endif
	return 20;
    }

    start_time = time (NULL);
    while (!(runflags & RUN_TERMINATING))
    {
	long tempflags;
	reset_time = time(NULL);
	if (quitmsg) free(quitmsg);
	quitmsg = NULL;
	servsock = -1;
	
	tempflags = runflags;
	runflags = RUN_MODE;
	if (tempflags & RUN_DEBUG)
	    runflags |= RUN_DEBUG;
	if (tempflags & RUN_LIVE)
	    runflags |= RUN_LIVE;

	if (read_options () <= 0)
	{
#ifdef WIN32
	    WSACleanup ();
#endif
	    return 20;
	}

	/* Check for program name == "listnicks" or "listchans" and do
	 * appropriate special stuff if so. */

	if (strcmp (progname, "listnicks") == 0)
	    return do_list(ac, av, 0);
	else if (strcmp (progname, "listchans") == 0)
	    return do_list(ac, av, 1);

	for (i = 1; i < ac; ++i)
	{
	    s = av[i];
	    if (*s == '-' || *s == '/')
	    {
		++s;
		if (stricmp (s, "remote") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, "-remote requires hostname[:port]\n");
			return 1;
		    }
		    s = av[i];
		    if (t = strchr (s, ':'))
		    {
			*t++ = 0;
			if (((int) strtol ((t), (char **) ((void *) 0), 10)) > 0)
			    remote_port = ((int) strtol ((t), (char **) ((void *) 0), 10));
			else
			    fprintf (stderr, "-remote: port number must be a positive integer.  Using default.\n");
		    }
		    strcpy (remote_server, s);
		}
		else if (stricmp (s, "name") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-name");
			return 1;
		    }
		    strcpy (server_name, av[i]);
		}
		else if (stricmp (s, "desc") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-desc");
			return 1;
		    }
		    strcpy (server_desc, av[i]);
		}
		else if (stricmp (s, "user") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-user");
			return 1;
		    }
		    strcpy (services_user, av[i]);
		}
		else if (stricmp (s, "host") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-host");
			return 1;
		    }
		    strcpy (services_host, av[i]);
		}
		else if (stricmp (s, "prefix") == 0)
		{
		    if(outlet_on==TRUE)
		    {
			if (++i >= ac)
			{
			    fprintf (stderr, ERR_REQ_PARAM, "-prefix");
			    return 1;
			}
			strcpy (services_prefix, av[i]);
		    }
		}
		else if (stricmp (s, "dir") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-dir");
			return 1;
		    }
		    /* already handled, but we needed to ++i */
		}
		else if (stricmp (s, "config") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-config");
			return 1;
		    }
		    /* already handled, but we needed to ++i */
		}
		else if (stricmp (s, "log") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-log");
			return 1;
		    }
		    strcpy (log_filename, av[i]);
		}
		else if (stricmp (s, "debug") == 0)
		    runflags |= RUN_DEBUG;
		else if (stricmp (s, "live") == 0 || stricmp (s, "nofork") == 0)
		    runflags |= RUN_LIVE;
		else if (stricmp (s, "relink") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-relink");
			return 1;
		    }
		    if (atoi (av[i]) < 0)
		    {
			fprintf (stderr, "-relink parameter must be posetive.\n");
			return 1;
		    }
		    server_relink = atoi (av[i]);
		}
		else if (stricmp (s, "level") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-level");
			return 1;
		    }
		    if (atoi (av[i]) <= 0)
		    {
			fprintf (stderr, "-level parameter must be greater than 0.\n");
			return 1;
		    }
		    services_level = atoi (av[i]);
		}
		else if (stricmp (s, "offset") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-offset");
			return 1;
		    }
		    if (abs (atoi (av[i])) >= 24)
		    {
			fprintf (stderr, "-offset must be between -24 and 24.\n");
			return 1;
		    }
		    tz_offset = atof (av[i]);
		}
		else if (stricmp (s, "norelink") == 0)
		    server_relink = -1;
		else if (stricmp (s, "update") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-update");
			return 1;
		    }
		    s = av[i];
		    if (atoi (s) <= 0)
		    {
			fprintf (stderr, "-update: number of seconds must be positive.\n");
			return 1;
		    }
		    else
			update_timeout = (atoi (s));
		}
		else if (stricmp (s, "ping") == 0)
		{
		    if (++i >= ac)
		    {
			fprintf (stderr, ERR_REQ_PARAM, "-ping");
			return 1;
		    }
		    s = av[i];
		    if (atoi (s) <= 0)
		    {
			fprintf (stderr, "-ping: number of seconds must be positive.\n");
			return 1;
		    }
		    else
			ping_frequency = (atoi (s));
		}
		else
		{
		    fprintf (stderr, "Unknown option -%s.\n", s);
		    return 1;
		}
	    }
	    else
	    {
		fprintf (stderr, "Non-option arguments not allowed.\n");
		return 1;
	    }
	}
#ifndef WIERD_COLLIDE
	offset = services_level * 2;
#endif

	if (check_config())
	{
#ifdef WIN32
	    WSACleanup ();
#endif
	    return 20;
	}
	on_boot = FALSE;

	if(outlet_on==TRUE)
	    snprintf (s_Outlet, NICKMAX, "%s%d", services_prefix, services_level);

	open_log ();

#ifdef RUNGROUP
	if (errmsg)
	    write_log ("%s: %s", errmsg, RUNGROUP);
#endif

	/* Detach ourselves */
#ifndef WIN32
        if (!(runflags & RUN_LIVE))
	    if ((i = fork ()) < 0)
	    {
		log_perror ("fork()");
		return 1;
	    }
	    else if (i != 0)
		return 0;
	if (setpgid (0, 0) < 0)
	{
	    log_perror ("setpgid()");
	    return 1;
	}
#endif

	/* Write our PID to the PID file. */

	pidfile = fopen (pid_filename, "w");
	if (pidfile)
	{
	    fprintf (pidfile, "%d\n", getpid ());
	    fclose (pidfile);
	}
	else
	    log_perror ("Warning: cannot write to PID file %s", pid_filename);


	write_log ("All systems nominal");

	/* Set signal handlers */
	signal (SIGINT, sighandler);
	signal (SIGTERM, sighandler);
#ifdef SIGPIPE
	signal (SIGPIPE, SIG_IGN);	/* We don't care about broken pipes */
#endif
#ifdef SIGQUIT
	signal (SIGQUIT, sighandler);
#endif
	signal (SIGSEGV, sighandler);
#ifdef SIGBUS
	signal (SIGBUS, sighandler);
#endif
#ifdef SIGHUP
	signal (SIGHUP, sighandler);
#endif
	signal (SIGILL, sighandler);
#ifdef SIGTRAP
	signal (SIGTRAP, sighandler);
#endif
#ifdef SIGIOT
	signal (SIGIOT, sighandler);
#endif
	signal (SIGFPE, sighandler);
	signal (SIGALRM, SIG_IGN);	/* Used by sgets() for read timeout */
#ifdef SIGUSR2
	signal (SIGUSR2, SIG_IGN);
#endif
#ifdef SIGCHLD
	signal (SIGCHLD, SIG_IGN);
#endif
#ifdef SIGWINCH
	signal (SIGWINCH, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal (SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal (SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif
	signal (SIGUSR1, sighandler);	/* This is our "out-of-memory" panic switch */


	/* Load up databases. */

	reset_dbases ();
	if(nickserv_on==TRUE)
	    load_ns_dbase ();
	if(chanserv_on==TRUE)
	    load_cs_dbase ();
	if (services_level == 1)
	{
	    if(memos_on==TRUE)
		load_ms_dbase ();
	    if(news_on==TRUE)
		load_news_dbase ();
	}
	if(akill_on==TRUE)
	    load_akill ();
	if(clones_on==TRUE)
	    load_clone ();
	if(globalnoticer_on==TRUE)
	    load_message ();
	if(operserv_on==TRUE)
	    load_sop ();

	/* Connect to the remote server */
	servsock = conn (remote_server, remote_port);
	if (servsock < 0)
	{
	    log_perror ("Can't connect to server");
	    goto restart;
	}
	send_cmd (NULL, "PASS :%s", password);
	send_cmd (NULL, "SERVER %s 1 :%s", server_name, server_desc);
	sgets2 (inbuf, sizeof (inbuf), servsock);
	if (strnicmp (inbuf, "ERROR", 5) == 0)
	{
	    write_log ("Remote server returned: %s", inbuf);
	    goto restart;
	}

	/* Bring in our pseudo-clients. */
	introduce_user (NULL);

/*** Main loop. ***/
	/* We have a line left over from earlier, so process it first. */
	process ();

	/* The signal handler routine will drop back here with quitting != 0
	 * if it gets called. */
	setjmp (panic_jmp);
	runflags |= RUN_STARTED;

	last_update = time (NULL);

	while (!(runflags & RUN_QUITTING))
	{
	    time_t t = time (NULL);
	    waiting = -3;
	    if ((runflags & RUN_SAVE_DATA) || t - last_update >= update_timeout)
	    {
		if (services_level == 1)
		{
		    /* First check for expired nicks/channels */
		    waiting = -22;
		    if(nickserv_on==TRUE && nick_expire > 0)
			expire_nicks ();
		    if(chanserv_on==TRUE && channel_expire > 0)
			expire_chans ();
		    if(news_on==TRUE && news_expire > 0)
		    {
			if (services_level == 1)
			    expire_news ();
		    }
		    if(akill_on==TRUE && akill_expire > 0)
			expire_akill ();
		}
		/* Now actually save stuff */
		waiting = -2;
		if (services_level == 1)
		{
		    if(nickserv_on==TRUE)
			save_ns_dbase ();
		    if(chanserv_on==TRUE)
			save_cs_dbase ();
		    if (services_level == 1)
		    {
			if(memos_on==TRUE)
			    save_ms_dbase ();
			if(news_on==TRUE)
			    save_news_dbase ();
		    }
		    if(akill_on==TRUE)
			save_akill ();
		    if(clones_on==TRUE)
			save_clone ();
		    if(globalnoticer_on==TRUE)
			save_message ();
		    save_sop ();
		}
		runflags &= ~RUN_SAVE_DATA;
		last_update = t;
		if (runflags & RUN_SIGTERM)
		{
		    runflags &= ~RUN_SIGTERM;
		    break;
		}
	    }
/*      if ((runflags & RUN_SEND_PINGS) || t-last_ping >= ping_frequency) {
   int i;
   for (i=0; i<servcnt; ++i) {
   addtimer(servlist[i].server);
   send_cmd(server_name, "PING %s", servlist[i].server);
   runflags &= ~RUN_SEND_PINGS;
   }
   last_ping = t;
   }
 */
	    {
		int i;
		for (i = 0; i < ignorecnt; ++i)
		    if (ignore[i].start &&
			(time (NULL) - ignore[i].start > ignore_time))
		    {
			--ignorecnt;
			if (i < ignorecnt)
			    bcopy (ignore + i + 1, ignore + i, sizeof (*ignore) * (ignorecnt - i));
			i--;
		    }
	    }
	    time_to_die();
	    waiting = -1;
	    if(nickserv_on==TRUE)
	    {
		/* Ew... hardcoded constant.  Somebody want to come up with a good
		* name for this? */
		if (t - last_check >= 3)
		    check_timeouts ();	/* Nick kill stuff */
	    }
	    waiting = 1;
	    i = (int) sgets2 (inbuf, sizeof (inbuf), servsock);
	    waiting = 0;
	    if (i > 0)
		process ();
	    else if (i == 0)
	    {
		if (quitmsg = malloc (BUFSIZE))
		    snprintf (quitmsg, BUFSIZE, "Read error from server: %s", strerror (errno));
		else
		    quitmsg = "Read error from server";
		runflags |= RUN_QUITTING;
	    }
	    waiting = -4;
	}

	/* Disconnect and exit */
	if (!quitmsg)
	    quitmsg = "Terminating, reason unknown";
	write_log ("%s", quitmsg);
	if (runflags & RUN_STARTED)
	    send_cmd (server_name, "SQUIT %s :%s", server_name, quitmsg);
      restart:
	disconn (servsock);
	close_log ();

	if (server_relink > 0 && !(runflags & RUN_NOSLEEP))
#ifdef WIN32
	    SleepEx (server_relink * 1000, FALSE);
#else
	    sleep (server_relink);
#endif
	else if (server_relink < 0)
	    goto endproc;
    }
endproc:
    remove (pid_filename);
#ifdef WIN32
    WSACleanup ();
#endif
    return 0;
}

int
do_list(int ac, char **av, int call)
{
    char *errmsg = NULL;
    if (call==0)
	{
	    if(nickserv_on==FALSE)
		fprintf (stderr, "NickServ was not configured into this version of Magick\n");
	    else
	    {
		int count = 0;	/* Count only rather than display? */
		int usage = 0;	/* Display command usage?  (>0 also indicates error) */
		int i;
   
#ifdef RUNGROUP
		if (errmsg)
		    write_log ("%s: %s", errmsg, RUNGROUP);
#endif

		i = 1;
		while (i < ac)
		{
		    if (av[i][0] == '-')
		    {
			switch (av[i][1])
			{
			case 'h':
			    usage = -1;
			    break;
			case 'c':
			    if (i > 1)
				usage = 1;
			    count = 1;
			    break;
			default:
			    usage = 1;
			    break;
			}
			--ac;
			if (i < ac)
			    bcopy (av + i + 1, av + i, sizeof (char *) * ac - i);
		    }
		    else
		    {
			if (count)
			    usage = 1;
			++i;
		    }
		}
		if (usage)
		{
		    fprintf (stderr, "\
\n\
Usage: listnicks [-c] [nick [nick...]]\n\
     -c: display count of registered nicks only\n\
            (cannot be combined with nicks)\n\
   nick: nickname(s) to display information for\n\
\n\
If no nicks are given, the entire nickname database is printed out in\n\
compact format followed by the number of registered nicks (with -c, the\n\
list is suppressed and only the count is printed).  If one or more nicks\n\
are given, detailed information about those nicks is displayed.\n\
\n");
#ifdef WIN32
		WSACleanup ();
#endif
		    return usage > 0 ? 1 : 0;
		}
		load_ns_dbase ();
		if (ac > 1)
		{
		    for (i = 1; i < ac; ++i)
			listnicks (0, av[i]);
		}
		else
		    listnicks (count, NULL);
#ifdef WIN32
		WSACleanup ();
#endif
		return 0;
	    }
	}
    else if (call==1)
	{
	    if(chanserv_on==FALSE)
		fprintf (stderr, "ChanServ was not configured into this version of Magick\n");
	    else
	    {
		int count = 0;	/* Count only rather than display? */
		int usage = 0;	/* Display command usage?  (>0 also indicates error) */
		int i;

#ifdef RUNGROUP
		if (errmsg)
		    write_log ("%s: %s", errmsg, RUNGROUP);
#endif

		i = 1;
		while (i < ac)
		{
		    if (av[i][0] == '-')
		    {
			switch (av[i][1])
			{
			case 'h':
			    usage = -1;
			    break;
			case 'c':
			    if (i > 1)
				usage = 1;
			    count = 1;
			    break;
			default:
			    usage = 1;
			    break;
			}
			--ac;
			if (i < ac)
			    bcopy (av + i + 1, av + i, sizeof (char *) * ac - i);
		    }
		    else
		    {
			if (count)
			    usage = 1;
			++i;
		    }
		}
		if (usage)
		{
		    fprintf (stderr, "\
\n\
Usage: listchans [-c] [chan [chan...]]\n\
     -c: display count of registered channels only\n\
            (cannot be combined with channels)\n\
   chan: channel(s) to display information for\n\
\n\
If no channels are given, the entire channel database is printed out in\n\
compact format followed by the number of registered channelss (with -c, the\n\
list is suppressed and only the count is printed).  If one or more channels\n\
are given, detailed information about those channels is displayed.\n\
\n");
#ifdef WIN32
		    WSACleanup ();
#endif
		    return usage > 0 ? 1 : 0;
		}
		load_ns_dbase ();
		load_cs_dbase ();
		if (ac > 1)
		{
		    for (i = 1; i < ac; ++i)
			listchans (0, av[i]);
		}
		else
		    listchans (count, NULL);
#ifdef WIN32
		WSACleanup ();
#endif
		return 0;
	    }
	}
}

int 
read_options (void)
{
    int Result, i;
    Boolean_T stop_on = FALSE;

    char *sections[] =
    {
	"[StartUp]",
	"[Services]",
	"[Files]",
	"[Config]",
	"[ChanServ]",
	"[NickServ]",
	"[MemoServ]",
	"[OperServ]",
	"[DevNull]",
	NULL
    };

    struct Config_Tag configs[] =
    {
	{"REMOTE_SERVER", String_Tag, remote_server},
	{"REMOTE_PORT", DWord_Tag, &remote_port},
	{"PASSWORD", String_Tag, password},
	{"SERVER_NAME", String_Tag, server_name},
	{"SERVER_DESC", String_Tag, server_desc},
	{"SERVICES_USER", String_Tag, services_user},
	{"SERVICES_HOST", String_Tag, services_host},
	{"SERVICES_LEVEL", DWord_Tag, &services_level},
	{"TZ_OFFSET", Float_Tag, &tz_offset},
	{"NICKSERV", Boolean_Tag, &nickserv_on},
	{"CHANSERV", Boolean_Tag, &chanserv_on},
	{"HELPSERV", Boolean_Tag, &helpserv_on},
	{"IRCIIHELP", Boolean_Tag, &irciihelp_on},
	{"MEMOSERV", Boolean_Tag, &memoserv_on},
	{"MEMOS", Boolean_Tag, &memos_on},
	{"NEWS", Boolean_Tag, &news_on},
	{"DEVNULL", Boolean_Tag, &devnull_on},
	{"OPERSERV", Boolean_Tag, &operserv_on},
	{"OUTLET", Boolean_Tag, &outlet_on},
	{"AKILL", Boolean_Tag, &akill_on},
	{"CLONES", Boolean_Tag, &clones_on},
	{"GLOBALNOTICER", Boolean_Tag, &globalnoticer_on},
	{"SHOW_SYNC", Boolean_Tag, &show_sync_on},
	{"NICKSERV_NAME", String_Tag, s_NickServ},
	{"CHANSERV_NAME", String_Tag, s_ChanServ},
	{"OPERSERV_NAME", String_Tag, s_OperServ},
	{"MEMOSERV_NAME", String_Tag, s_MemoServ},
	{"HELPSERV_NAME", String_Tag, s_HelpServ},
	{"GLOBALNOTICER_NAME", String_Tag, s_GlobalNoticer},
	{"DEVNULL_NAME", String_Tag, s_DevNull},
	{"IRCIIHELP_NAME", String_Tag, s_IrcIIHelp},
	{"SERVICES_PREFIX", String_Tag, services_prefix},
	{"OVERRIDE_LEVEL", DWord_Tag, &override_level_val},
	{"LOG_FILENAME", String_Tag, log_filename},
	{"MOTD_FILENAME", String_Tag, motd_filename},
	{"NICKSERV_DB", String_Tag, nickserv_db},
	{"CHANSERV_DB", String_Tag, chanserv_db},
	{"MEMOSERV_DB", String_Tag, memoserv_db},
	{"NEWSSERV_DB", String_Tag, newsserv_db},
	{"AKILL_DB", String_Tag, akill_db},
	{"CLONE_DB", String_Tag, clone_db},
	{"SOP_DB", String_Tag, sop_db},
	{"MESSAGE_DB", String_Tag, message_db},
	{"PID_FILE", String_Tag, pid_filename},
	{"HELPSERV_DIR", String_Tag, helpserv_dir},
	{"SERVER_RELINK", DWord_Tag, &server_relink},
	{"UPDATE_TIMEOUT", DWord_Tag, &update_timeout},
	{"PING_FREQUENCY", DWord_Tag, &ping_frequency},
	{"READ_TIMEOUT", DWord_Tag, &read_timeout},
	{"CHANNEL_EXPIRE", DWord_Tag, &channel_expire},
	{"AKICK_MAX", DWord_Tag, &akick_max},
	{"DEF_AKICK_REASON", String_Tag, def_akick_reason},
	{"NICK_EXPIRE", DWord_Tag, &nick_expire},
	{"RELEASE_TIMEOUT", DWord_Tag, &release_timeout},
	{"WAIT_COLLIDE", DWord_Tag, &wait_collide},
	{"PASSFAIL_MAX", DWord_Tag, &passfail_max},
	{"NEWS_EXPIRE", DWord_Tag, &news_expire},
	{"SERVICES_ADMIN", String_Tag, services_admin},
	{"AKILL_EXPIRE", DWord_Tag, &akill_expire },
	{"CLONES_ALLOWED", DWord_Tag, &clones_allowed },
	{"DEF_CLONE_REASON", String_Tag, def_clone_reason },
	{"FLOOD_MESSAGES", DWord_Tag, &flood_messages },
	{"FLOOD_TIME", DWord_Tag, &flood_time },
	{"IGNORE_TIME", DWord_Tag, &ignore_time },
	{"IGNORE_OFFENCES", DWord_Tag, &ignore_offences },
	{"STARTHRESH", DWord_Tag, &starthresh },
	{"STOP",Boolean_Tag, &stop_on },
	{NULL, Error_Tag, NULL}	/* Terminating record   */
    };

    for (Result=0, i=0; sections[i]; i++)
	Result += input_config (config_file, configs, sections[i]);

/* Ungod -- goto config on Result <= 0, and just idle if stop_on */

    if (Result == 0)
	fatal("Configuration File is Empty.");
    else if (Result < 0)
	fatal("Configuration File not found.");
    else if (stop_on)
    {
	fatal("STOP code recieved in config.");
	Result = -1;
    }	
    return Result;
}

int
check_config (void)
{
    /* Dependancies */
    if (nickserv_on==FALSE)
    {
	chanserv_on==FALSE;
	memoserv_on==FALSE;
    }
    if (chanserv_on==FALSE)
    {
	news_on==FALSE;
    }
    if (memoserv_on==FALSE)
    {
	memos_on==FALSE;
	news_on==FALSE;
    }
    if (memos_on==FALSE && news_on==FALSE)
    {
	memoserv_on==FALSE;
    }
    if (operserv_on==FALSE)
    {
	globalnoticer_on==FALSE;
	outlet_on==FALSE;
	akill_on==FALSE;
	clones_on==FALSE;
    }
    if (clones_allowed < 1)
    {
	clones_on==FALSE;
    }

    if (services_level < 1)
    {
	fatal("CONFIG: Cannot set SERVICES_LEVEL < 1");
	return 1;
    }
    if (tz_offset >= 24 || tz_offset <= -24)
    {
	fatal("CONFIG: TZ_OFFSET must fall between -24 and 24.");
	return 1;
    }
    if (update_timeout < 30)
    {
	fatal("CONFIG: Cannot set UPDATE_TIMEOUT < 30.");
	return 1;
    }
    if (read_timeout < 1)
    {
	fatal("CONFIG: Cannot set READ_TIMEOUT < 1.");
	return 1;
    }
    if (passfail_max < 1)
    {
	fatal("CONFIG: Cannot set PASSFAIL_MAX < 1.");
	return 1;
    }
    if (flood_messages > LASTMSGMAX)
    {
	fatal("CONFIG: Cannot set FLOOD_MESSAGES > %d.", LASTMSGMAX);
	return 1;
    }
    return 0;
}
