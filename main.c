/* Magick -- main source file.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
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
char *remote_server = REMOTE_SERVER;		/* -remote server[:port] */
int remote_port = REMOTE_PORT;
char *server_name = SERVER_NAME;		/* -name servername */
char *server_desc = SERVER_DESC;		/* -desc serverdesc */
char *services_user = SERVICES_USER;		/* -user username */
char *services_host = SERVICES_HOST;		/* -host hostname */
char *services_dir = SERVICES_DIR;		/* -dir directory */
#ifdef OUTLET
  char *services_prefix = SERVICES_PREFIX;	/* -prefix prefix */
#endif
char *log_filename = LOG_FILENAME;		/* -log filename */
int update_timeout = UPDATE_TIMEOUT;		/* -update secs */
int ping_frequency = PING_FREQUENCY;		/* -ping secs */
int server_relink = SERVER_RELINK;		/* -relink secs or -norelink */
int services_level = SERVICES_LEVEL;		/* -level level */
float tz_offset = TZ_OFFSET;			/* -offset offset */

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

#ifdef OPERSERV
/* Reason for being OFF if it IS off */
char *offreason = NULL;
#endif

/* Nice version of all the ugly 0/1 veriables! */
long runflags = 0;


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

#ifdef DEVNULL
const char s_DevNull[] = DEVNULL_NAME;
#endif

/*************************************************************************/

/* If we get a weird signal, come here. */

static void sighandler(int signum)
{
    if (runflags & RUN_STARTED) {
	/* Prepare for a possible SIGKILL -- Typical shutdown sequence */
    	if (signum == SIGTERM) {
	    runflags |= (RUN_SIGTERM | RUN_SAVE_DATA);
    	    signal(SIGTERM, SIG_IGN);
    	    return;
	} else if (!waiting) {
	    log("PANIC! buffer = %s", inbuf);
	    /* Cut off if this would make IRC command >510 characters. */
	    if (strlen(inbuf) > 448) {
		inbuf[446] = '>';
		inbuf[447] = '>';
		inbuf[448] = 0;
	    }
	    wallops(NULL, "PANIC! buffer = %s\r\n", inbuf);
	} else if (waiting < 0) {
	    if (waiting == -1) {
		log("PANIC! in timed_update (%s)", strsignal(signum));
		wallops(NULL, "PANIC! in timed_update (%s)", strsignal(signum));
	    } else {
		log("PANIC! waiting=%d (%s)", waiting, strsignal(signum));
		wallops(NULL, "PANIC! waiting=%d (%s)", waiting, strsignal(signum));
	    }
	}
    }
    if (signum == SIGUSR1 || !(quitmsg = malloc(BUFSIZE))) {
	quitmsg = "Out of memory!";
	runflags |= RUN_QUITTING;
    } else {
    	if (signum == SIGHUP) /* No biggie -- just dbase reload ... */
	    snprintf(quitmsg, BUFSIZE, "Magick Recieved SIGHUP -- Reloading!");
	else 
#if HAVE_STRSIGNAL
	    snprintf(quitmsg, BUFSIZE, "Magick terminating: %s", strsignal(signum));
#else
	    snprintf(quitmsg, BUFSIZE, "Magick terminating on signal %d", signum);
#endif
	runflags |= RUN_QUITTING;
    }
    if (runflags & RUN_STARTED)
	longjmp(panic_jmp, 1);
    else {
	log("%s", quitmsg);
	exit(1);
    }
}

/*************************************************************************/

/* Log stuff to stderr with a datestamp. */

void log(const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256];

    va_start(args, fmt);
    time(&t);
    tm = *localtime(&t);
    strftime(buf, sizeof(buf)-1, "[%b %d %H:%M:%S %Y] ", &tm);
    if (runflags & RUN_LOG_IS_OPEN) {
	fputs(buf, stderr);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	fflush(stderr);
    }
}

/* Like log(), but tack a ": " and a system error message (as returned by
 * strerror() onto the end.
 */

void log_perror(const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256];

    va_start(args, fmt);
    time(&t);
    tm = *localtime(&t);
    strftime(buf, sizeof(buf)-1, "[%b %d %H:%M:%S %Y] ", &tm);
    if (runflags & RUN_LOG_IS_OPEN) {
	fputs(buf, stderr);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, ": %s\n", strerror(errno));
	fflush(stderr);
    }
}

/*************************************************************************/

/* We've hit something we can't recover from.  Let people know what
 * happened, then go down.
 */

void fatal(const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256], buf2[4096];

    va_start(args, fmt);
    time(&t);
    tm = *localtime(&t);
    strftime(buf, sizeof(buf)-1, "[%b %d %H:%M:%S %Y]", &tm);
    vsnprintf(buf2, sizeof(buf2), fmt, args);
    if (runflags & RUN_LOG_IS_OPEN) {
	fprintf(stderr, "%s FATAL: %s\n", buf, buf2);
	fflush(stderr);
    }
    if (servsock >= 0)
	wallops(NULL, "FATAL ERROR!  %s", buf2);
    exit(1);
}

/* Same thing, but do it like perror().
 */

void fatal_perror(const char *fmt,...)
{
    va_list args;
    time_t t;
    struct tm tm;
    char buf[256], buf2[4096];

    va_start(args, fmt);
    time(&t);
    tm = *localtime(&t);
    strftime(buf, sizeof(buf)-1, "[%b %d %H:%M:%S %Y]", &tm);
    vsnprintf(buf2, sizeof(buf2), fmt, args);
    if (runflags & RUN_LOG_IS_OPEN) {
	fprintf(stderr, "%s FATAL: %s: %s\n", buf, buf2, strerror(errno));
	fflush(stderr);
    }
    if (servsock >= 0)
	wallops(NULL, "FATAL ERROR!  %s", buf2);
    exit(1);
}

/*************************************************************************/

int get_file_version(FILE *f, const char *filename)
{
    int version = fgetc(f)<<24 | fgetc(f)<<16 | fgetc(f)<<8 | fgetc(f);
    if (ferror(f) || feof(f))
	fatal_perror("Error reading version number on %s", filename);
    else if (version > FILE_VERSION || version < 1)
	fatal("Invalid version number (%d) on %s", version, filename);
    return version;
}

void write_file_version(FILE *f, const char *filename)
{
    if (
	fputc(FILE_VERSION>>24 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION>>16 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION>> 8 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION     & 0xFF, f) < 0
    )
	fatal_perror("Error writing version number on %s", filename);
}

/*************************************************************************/

/* Send a NICK command for the given pseudo-client.  If `user' is NULL,
 * send NICK commands for all the pseudo-clients. */

#if defined(IRC_DALNET)
# ifdef DAL_SERV
#  define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s 1 %ld %s %s %s 1 :%s", (nick), offset-services_level, \
		services_user, services_host, server_name, (name)); \
    } while (0)
# else
#  define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s 1 %ld %s %s %s :%s", (nick), offset-services_level, \
		services_user, services_host, server_name, (name)); \
    } while (0)
# endif
#elif defined(IRC_UNDERNET)
# define NICK(nick,name) \
    do { \
	send_cmd(server_name, "NICK %s 1 %ld %s %s %s :%s", (nick), offset-services_level,\
		services_user, services_host, server_name, (name)); \
    } while (0)
#elif defined(IRC_TS8)
# define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s :1", (nick)); \
	send_cmd((nick), "USER %ld %s %s %s :%s", offset-services_level, \
		services_user, services_host, server_name, (name)); \
    } while (0)
#else
# define NICK(nick,name) \
    do { \
	send_cmd(NULL, "NICK %s :1", (nick)); \
	send_cmd((nick), "USER %s %s %s :%s", \
		services_user, services_host, server_name, (name)); \
    } while (0)
#endif

int i_am_backup() {
    if (
#ifdef NICKSERV
	finduser(s_NickServ) ||
#endif
#ifdef CHANSERV
	finduser(s_ChanServ) ||
#endif
#ifdef HELPSERV
	finduser(s_HelpServ) ||
#endif
#ifdef IRCIIHELP
	finduser(s_IrcIIHelp) ||
#endif
#ifdef MEMOSERV
	finduser(s_MemoServ) ||
#endif
#ifdef OPERSERV 
	finduser(s_OperServ) ||
#endif
#ifdef DEVNULL
	finduser(s_DevNull) ||
#endif
#ifdef GLOBALNOTICER
	finduser(s_GlobalNoticer) ||
#endif
	0) return 1;
    return 0;
}

int is_services_nick(const char *nick) {
    if (
#ifdef NICKSERV
	stricmp(nick, s_NickServ)==0 ||
#endif
#ifdef CHANSERV
	stricmp(nick, s_ChanServ)==0 ||
#endif
#ifdef HELPSERV
	stricmp(nick, s_HelpServ)==0 ||
#endif
#ifdef IRCIIHELP
	stricmp(nick, s_IrcIIHelp)==0 ||
#endif
#ifdef MEMOSERV
	stricmp(nick, s_MemoServ)==0 ||
#endif
#ifdef OPERSERV 
	stricmp(nick, s_OperServ)==0 ||
#endif
#ifdef DEVNULL
	stricmp(nick, s_DevNull)==0 ||
#endif
#ifdef GLOBALNOTICER
	stricmp(nick, s_GlobalNoticer)==0 ||
#endif
#ifdef OUTLET
	stricmp(nick, s_Outlet)==0 ||
#endif
	0) return 1;
    return 0;
}

const char *any_service()
{
#ifdef OUTLET
	return s_Outlet;
#endif
#ifdef OPERSERV 
	return s_OperServ;
#endif
#ifdef NICKSERV
	return s_NickServ;
#endif
#ifdef CHANSERV
	return s_ChanServ;
#endif
#ifdef MEMOSERV
	return s_MemoServ;
#endif
#ifdef GLOBALNOTICER
	return s_GlobalNoticer;
#endif
#ifdef HELPSERV
	return s_HelpServ;
#endif
#ifdef IRCIIHELP
	return s_IrcIIHelp;
#endif
#ifdef DEVNULL
	return s_DevNull;
#endif
	return NULL;
}

static void check_introduce(const char *nick, const char *name)
{
    User *u;
    
    if (!(u = finduser(nick)))
	NICK(nick, name);
    else
#ifdef WIERD_COLLIDE
	if (u->signon < offset-services_level)
#else
	if (u->signon > offset-services_level)
#endif
	    NICK(nick, name);
}

void introduce_user(const char *user)
{
#ifdef NICKSERV
    if (!user || stricmp(user, s_NickServ)==0)
	check_introduce(s_NickServ, "Nickname Server");
#endif
#ifdef CHANSERV
    if (!user || stricmp(user, s_ChanServ)==0)
	check_introduce(s_ChanServ, "Channel Server");
#endif
#ifdef HELPSERV
    if (!user || stricmp(user, s_HelpServ)==0)
	check_introduce(s_HelpServ, "Help Server");
#endif
#ifdef IRCIIHELP
    if (!user || stricmp(user, s_IrcIIHelp)==0)
	check_introduce(s_IrcIIHelp, "ircII Help Server");
#endif
#ifdef MEMOSERV
    if (!user || stricmp(user, s_MemoServ)==0)
	check_introduce(s_MemoServ, "Memo Server");
#endif
#ifdef DEVNULL
    if (!user || stricmp(user, s_DevNull)==0) {
	check_introduce(s_DevNull, "/dev/null -- message sink");
	send_cmd(s_DevNull, "MODE %s +i", s_DevNull);
    }
#endif
#ifdef OPERSERV
    if (!user || stricmp(user, s_OperServ)==0) {
	check_introduce(s_OperServ, "Operator Server");
	send_cmd(s_OperServ, "MODE %s +i", s_OperServ);
    }
#endif
#ifdef GLOBALNOTICER
    if (!user || stricmp(user, s_GlobalNoticer)==0) {
	check_introduce(s_GlobalNoticer, "Global Noticer");
	send_cmd(s_GlobalNoticer, "MODE %s +io", s_GlobalNoticer);
    }
#endif
#ifdef OUTLET
    if (!user || stricmp(user, s_Outlet)==0) {
	check_introduce(s_Outlet, "Magick Outlet");
	send_cmd(s_Outlet, "MODE %s +i", s_Outlet);
    }
#endif
}

#undef NICK

/*************************************************************************/

/* Is the 'user' a server? */
int is_server(const char *nick)
{
    int i;
    
    for (i=0;i<strlen(nick) && nick[i]!='.';i++) ;
    
    if (nick[i]=='.')
	return 1;
    return 0;
}

/*************************************************************************/

/* Reset in preperation for re-start */
void reset_dbases()
{
    User *u, *un;
    NickInfo *ni, *nin;
    ChannelInfo *ci, *cin;
    Clone *c, *cn;
    Timeout *to, *ton;
    Timer *t, *tn;
    int i;

/* Some of these output ... stop that. */
    runflags |= RUN_NOSEND;

/* Clear active users (and channels) */
    u = userlist;
    while (u) {
	un = u->next;
	delete_user(u);
	u=un;
    }
    usercnt = opcnt = 0;

/* SQUIT all servers */
    while (servcnt) {
	free(servlist[0].server);
	free(servlist[0].desc);
	servcnt--;
	bcopy(servlist+1, servlist, sizeof(*servlist) * servcnt);
    }
    serv_size = 0;

/* Ignore smignore */
#ifdef DEVNULL
    while (ignorecnt) {
	ignorecnt--;
	bcopy(ignore+1, ignore, sizeof(*ignore) * ignorecnt);
    }
    ignore_size = 0;
#endif

/* Nope, we got NO MORE sops! */
#ifdef OPERSERV
    while (nsop) {
	--nsop;
	bcopy(sops+1, sops, NICKMAX * nsop);
    }
    sop_size = 0;
#endif

/* I OWN your ass! */
#ifdef NICKSERV
    for (i = 33; i < 256; ++i) {
	ni = nicklists[i];
	while (ni) {
	    nin = ni->next;
	    delnick(ni);
	    ni = nin;
	}
    }
#endif

/* muaahahahaha */
#ifdef CHANSERV
    for (i = 33; i < 256; ++i) {
	ci = chanlists[i];
	while (ci) {
	    cin = ci->next;
	    delchan(ci);
	    ci = cin;
	}
    }
#endif

/* Awwww, arent we nice ... */
#ifdef AKILL
    while (nakill)
	del_akill(akills[0].mask, 1);
    akill_size = 0;
#endif

/* bleep ... bleep */
#ifdef CLONES
    while (nclone)
	del_clone(clones[0].host);
    clone_size = 0;
    c = clonelist;
    while (c) {
	cn = c->next;
	clones_del(c->host);
	c = cn;
    }
#endif

/* Keep the plebs IGNORANT! */
#ifdef GLOBALNOTICER
    while (nmessage) {
	free(messages[0].text);
	--nmessage;
	bcopy(messages+1, messages, sizeof(*messages) * nmessage);
    }	
    message_size = 0;
#endif

/* Tick ... Tick ... Tick ... BOOM! */
#ifdef NICKSERV
    to = timeouts;
    while (to) {
	ton = to->next;
	free(to);
	to = ton;
    }
#endif
    t = pings;
    while (t) {
	t = t->next;
	deltimer(t->label);
	t = tn;
    }

    runflags &= ~RUN_NOSEND;
}

/*************************************************************************/

void open_log()
{
    /* Redirect stderr to logfile. */
    if (!(runflags & RUN_LOG_IS_OPEN))
	if (freopen(log_filename, "a", stderr))
	    runflags |= RUN_LOG_IS_OPEN;
	else
	    log("Cannot open %s, no logging output used.", log_filename);
    else
	log("Tried to open log, but already open!");
}
void close_log()
{
    if ((runflags & RUN_LOG_IS_OPEN))
	fclose(stderr);
    runflags &= ~RUN_LOG_IS_OPEN;
}

/*************************************************************************/

/* Main routine.  (What does it look like? :-) ) */
/* (Yes, big and ugly, I know...) */

int main(int ac, char **av)
{
    time_t last_update;	/* When did we last update the databases? */
    time_t last_ping;	/* When did we last ping all servers? */
#ifdef NICKSERV
    time_t last_check;	/* When did we last check NickServ timeouts? */
#endif
    int i;
    char *progname;
    char *s, *t;
#ifdef RUNGROUP
    struct group *gr;
    char *errmsg = NULL;
#endif
    FILE *pidfile;

  while (!(runflags & RUN_TERMINATING)) {
    quitmsg = NULL;
    servsock = -1;
    file_gid = -1;
    if (runflags & RUN_DEBUG)
	runflags = RUN_MODE | RUN_DEBUG;
    else
	runflags = RUN_MODE;

#ifdef DEFUMASK
    umask(DEFUMASK);
#endif

#ifdef RUNGROUP
    setgrent();
    while (gr = getgrent()) {
	if (strcmp(gr->gr_name, RUNGROUP) == 0)
	    break;
    }
    endgrent();
    if (gr)
	file_gid = gr->gr_gid;
    else
	errmsg = "Group not known";
#endif

    /* Find program name. */
    if (progname = strrchr(av[0], '/'))
	++progname;
    else
	progname = av[0];


    /* Parse -dir ahead of time. */
    for (i = 1; i < ac; ++i) {
	s = av[i];
	if (*s == '-') {
	    ++s;
	    if (strcmp(s, "dir") == 0) {
		if (++i >= ac) {
		    log("-dir requires a parameter");
		    break;
		}
		services_dir = av[i];
	    }
	}
    }

    /* Chdir to services directory. */

    if (chdir(services_dir) < 0) {
	perror(services_dir);
	return 20;
    }


    /* Check for program name == "listnicks" or "listchans" and do
     * appropriate special stuff if so. */

    if (strcmp(progname, "listnicks") == 0) {
#ifndef NICKSERV
	fprintf(stderr, "NickServ was not compiled into this version of services\n");
#else
	int count = 0;	/* Count only rather than display? */
	int usage = 0;	/* Display command usage?  (>0 also indicates error) */
	int i;
	runflags |= RUN_TERMINATING;

#ifdef RUNGROUP
	if (errmsg)
	    log("%s: %s", errmsg, RUNGROUP);
#endif

	i = 1;
	while (i < ac) {
	    if (av[i][0] == '-') {
		switch (av[i][1]) {
		case 'h':
		    usage = -1; break;
		case 'c':
		    if (i > 1)
			usage = 1;
		    count =  1; break;
		default :
		    usage =  1; break;
		}
		--ac;
		if (i < ac)
		bcopy(av+i+1, av+i, sizeof(char *) * ac-i);
	    } else {
		if (count)
		    usage = 1;
		++i;
	    }
	}
	if (usage) {
	    fprintf(stderr, "\
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
	    return usage>0 ? 1 : 0;
	}
	load_ns_dbase();
	if (ac > 1) {
	    for (i = 1; i < ac; ++i)
		listnicks(0, av[i]);
	} else
	    listnicks(count, NULL);
	return 0;

#endif
    } else if (strcmp(progname, "listchans") == 0) {
#ifndef CHANSERV
	fprintf(stderr, "ChanServ was not compiled into this version of services\n");
#else
	int count = 0;	/* Count only rather than display? */
	int usage = 0;	/* Display command usage?  (>0 also indicates error) */
	int i;
	runflags |= RUN_TERMINATING;

#ifdef RUNGROUP
	if (errmsg)
	    log("%s: %s", errmsg, RUNGROUP);
#endif

	i = 1;
	while (i < ac) {
	    if (av[i][0] == '-') {
		switch (av[i][1]) {
		case 'h':
		    usage = -1; break;
		case 'c':
		    if (i > 1)
			usage = 1;
		    count =  1; break;
		default :
		    usage =  1; break;
		}
		--ac;
		if (i < ac)
		bcopy(av+i+1, av+i, sizeof(char *) * ac-i);
	    } else {
		if (count)
		    usage = 1;
		++i;
	    }
	}
	if (usage) {
	    fprintf(stderr, "\
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
	    return usage>0 ? 1 : 0;
	}
	load_ns_dbase();
	load_cs_dbase();
	if (ac > 1) {
	    for (i = 1; i < ac; ++i)
		listchans(0, av[i]);
	} else
	    listchans(count, NULL);
	return 0;

#endif
    }

    for (i = 1; i < ac; ++i) {
	s = av[i];
	if (*s == '-' || *s == '/') {
	    ++s;
	    if (strcmp(s, "remote") == 0) {
		if (++i >= ac) {
		    log("-remote requires hostname[:port]");
		    break;
		}
		s = av[i];
		if (t = strchr(s, ':')) {
		    *t++ = 0;
		    if (((int) strtol((  t  ), (char **) ((void *)0) , 10) )  > 0)
			remote_port = ((int) strtol((  t  ), (char **) ((void *)0) , 10) ) ;
		    else
			log("-remote: port number must be a positive integer.  Using default.");
		}
		remote_server = s;
	    } else if (strcmp(s, "name") == 0) {
		if (++i >= ac) {
		    log("-name requires a parameter");
		    break;
		}
		server_name = av[i];
	    } else if (strcmp(s, "desc") == 0) {
		if (++i >= ac) {
		    log("-desc requires a parameter");
		    break;
		}
		server_desc = av[i];
	    } else if (strcmp(s, "user") == 0) {
		if (++i >= ac) {
		    log("-user requires a parameter");
		    break;
		}
		services_user = av[i];
	    } else if (strcmp(s, "host") == 0) {
		if (++i >= ac) {
		    log("-host requires a parameter");
		    break;
		}
		services_host = av[i];
#ifdef OUTLET
	    } else if (strcmp(s, "prefix") == 0) {
		if (++i >= ac) {
		    log("-prefix requires a parameter");
		    break;
		}
		services_prefix = av[i];
#endif
	    } else if (strcmp(s, "dir") == 0) {
		if (++i >= ac) {
		    log("-dir requires a parameter");
		    break;
		}
		/* already handled, but we needed to ++i */
	    } else if (strcmp(s, "log") == 0) {
		if (++i >= ac) {
		    log("-log requires a parameter");
		    break;
		}
		log_filename = av[i];
	    } else if (strcmp(s, "debug") == 0)
		runflags |= RUN_DEBUG;
	    else if (strcmp(s, "relink") == 0) {
		if (++i >= ac) {
		    log("-relink requires a parameter");
		    break;
		}
		if (atoi(av[i])<0) {
		    log("-relink parameter must be posetive");
		    break;
		}
		server_relink = atoi(av[i]);
	    } else if (strcmp(s, "level") == 0) {
		if (++i >= ac) {
		    log("-level requires a parameter");
		    break;
		}
		if (atoi(av[i])<=0) {
		    log("-level parameter must be greater than 0");
		    break;
		}
		services_level = atoi(av[i]);
	    } else if (strcmp(s, "offset") == 0) {
		if (++i >= ac) {
		    log("-level requires a parameter");
		    break;
		}
		if (abs(atoi(av[i]))>=24) {
		    log("-offset must be between -24 and 24.");
		    break;
		}
		tz_offset = atof(av[i]);
	    } else if (strcmp(s, "norelink") == 0)
		server_relink = -1;
	    else if (strcmp(s, "update") == 0) {
		if (++i >= ac) {
		    log("-update requires a parameter");
		    break;
		}
		s = av[i];
		if (atoi(s) <= 0)
		    log("-update: number of seconds must be positive");
		else
		    update_timeout = (atoi(s));
	    } else if (strcmp(s, "ping") == 0) {
		if (++i >= ac) {
		    log("-ping requires a parameter");
		    break;
		}
		s = av[i];
		if (atoi(s) <= 0)
		    log("-ping: number of seconds must be positive");
		else
		    ping_frequency = (atoi(s));
	    } else
		log("Unknown option -%s", s);
	} else
	    log("Non-option arguments not allowed");
    }
#ifndef WIERD_COLLIDE
    offset = services_level * 2;
#endif
#ifdef OUTLET
    snprintf(s_Outlet, NICKMAX, "%s%d", services_prefix, services_level);
#endif

    open_log();

#ifdef RUNGROUP
    if (errmsg)
	log("%s: %s", errmsg, RUNGROUP);
#endif

    /* Detach ourselves */
    if ((i = fork()) < 0) {
	log_perror("fork()");
	return 1;
    } else if (i != 0)
	return 0;
    if (setpgid(0, 0) < 0) {
	log_perror("setpgid()");
	return 1;
    }

    /* Write our PID to the PID file. */

    pidfile = fopen(PID_FILE, "w");
    if (pidfile) {
    	fprintf(pidfile, "%d\n", getpid());
    	fclose(pidfile);
    } else
    	log_perror("Warning: cannot write to PID file %s", PID_FILE);


    log("Services starting up");
    start_time = time(NULL);

    /* Set signal handlers */
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, SIG_IGN);		/* We don't care about broken pipes */
    signal(SIGQUIT, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGBUS, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGHUP, sighandler);
    signal(SIGILL, sighandler);
    signal(SIGTRAP, sighandler);
    signal(SIGIOT, sighandler);
    signal(SIGFPE, sighandler);
    signal(SIGALRM, SIG_IGN);		/* Used by sgets() for read timeout */
    signal(SIGUSR2, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGWINCH, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGUSR1, sighandler);	/* This is our "out-of-memory" panic switch */


    /* Load up databases. */

    reset_dbases();
#ifdef NICKSERV
    load_ns_dbase();
#endif
#ifdef CHANSERV
    load_cs_dbase();
#endif
if (services_level==1) {
#ifdef MEMOS
    load_ms_dbase();
#endif
#ifdef NEWS
    load_news_dbase();
#endif
}
#ifdef AKILL
    load_akill();
#endif
#ifdef CLONES
    load_clone();
#endif
#ifdef GLOBALNOTICER
    load_message();
#endif
    load_sop();

    /* Connect to the remote server */
    servsock = conn(remote_server, remote_port);
    if (servsock < 0) {
	log_perror("Can't connect to server");
	goto restart;
    }
    send_cmd(NULL, "PASS :%s", PASSWORD);
    send_cmd(NULL, "SERVER %s 1 :%s", server_name, server_desc);
    sgets2(inbuf, sizeof(inbuf), servsock);
    if (strnicmp(inbuf, "ERROR", 5) == 0) {
	log("Remote server returned: %s", inbuf);
	goto restart;
    }

    /* Bring in our pseudo-clients. */
    introduce_user(NULL);

    /*** Main loop. ***/
    /* We have a line left over from earlier, so process it first. */
    process();

    /* The signal handler routine will drop back here with quitting != 0
     * if it gets called. */
    setjmp(panic_jmp);
    runflags |= RUN_STARTED;

    last_update = time(NULL);

    while (!(runflags & RUN_QUITTING)) {
	time_t t = time(NULL);
	waiting = -3;
	if ((runflags & RUN_SAVE_DATA) || t-last_update >= update_timeout) {
	  if (services_level==1) {
	    /* First check for expired nicks/channels */
	    waiting = -22;
#if defined(NICKSERV) && NICK_EXPIRE > 0
	    expire_nicks();
#endif
#if defined(CHANSERV) && CHANNEL_EXPIRE > 0
	    expire_chans();
#endif
#if defined(NEWS) && NEWS_EXPIRE > 0
	    if (services_level==1)
		expire_news();
#endif
#if defined(AKILL) && AKILL_EXPIRE > 0
	    expire_akill();
#endif
	  }
	    /* Now actually save stuff */
	    waiting = -2;
	  if (services_level==1) {
#ifdef NICKSERV
	    save_ns_dbase();
#endif
#ifdef CHANSERV
	    save_cs_dbase();
#endif
	    if (services_level==1) {
#ifdef MEMOS
		save_ms_dbase();
#endif
#ifdef NEWS
		save_news_dbase();
#endif
	    }
#ifdef AKILL
	    save_akill();
#endif
#ifdef CLONES
	    save_clone();
#endif
#ifdef GLOBALNOTICER
	    save_message();
#endif
	    save_sop();
	  }
	    runflags &= ~RUN_SAVE_DATA;
	    last_update = t;
	    if (runflags & RUN_SIGTERM) {
		runflags &= ~RUN_SIGTERM;
		break;
	    }
	}
/*	if ((runflags & RUN_SEND_PINGS) || t-last_ping >= ping_frequency) {
	    int i;
	    for (i=0; i<servcnt; ++i) {
		addtimer(servlist[i].server);
		send_cmd(server_name, "PING %s", servlist[i].server);
		runflags &= ~RUN_SEND_PINGS;
	    }
	    last_ping = t;
	}
*/	{
	    int i;
	    for (i=0; i<ignorecnt; ++i)
		if (ignore[i].start &&
				(time(NULL) - ignore[i].start > IGNORE_TIME)) {
		    --ignorecnt;
		    if (i < ignorecnt)
			bcopy(ignore+i+1, ignore+i, sizeof(*ignore) * (ignorecnt-i));
		    i--;
		}
	}
	waiting = -1;
#ifdef NICKSERV
	/* Ew... hardcoded constant.  Somebody want to come up with a good
	 * name for this? */
	if (t-last_check >= 3)
	    check_timeouts();	/* Nick kill stuff */
#endif
	waiting = 1;
	i = (int)sgets2(inbuf, sizeof(inbuf), servsock);
	waiting = 0;
	if (i > 0)
	    process();
	else if (i == 0) {
	    if (quitmsg = malloc(BUFSIZE))
		snprintf(quitmsg, BUFSIZE, "Read error from server: %s", strerror(errno));
	    else
		quitmsg = "Read error from server";
	    runflags |= RUN_QUITTING;
	}
	waiting = -4;
    }

    /* Disconnect and exit */
    if (!quitmsg)
	quitmsg = "Terminating, reason unknown";
    log("%s", quitmsg);
    if (runflags & RUN_STARTED)
	send_cmd(server_name, "SQUIT %s :%s", server_name, quitmsg);
restart:
    disconn(servsock);
    close_log();

    if (server_relink > 0)
	sleep(server_relink);
    else if (server_relink < 0)
	goto endproc;
  }
endproc:
  remove(PID_FILE);
  return 0;
}
