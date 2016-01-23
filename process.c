/* Main processing code for Services.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include "version.h"

/*************************************************************************/

/* People to ignore (hashed by first character of nick). */

#ifdef DEVNULL
int ignorecnt = 0, ignore_size = 0;
Ignore *ignore = NULL;
#endif
int servcnt = 0, serv_size = 0;
Servers *servlist = NULL;
Timer *pings = NULL;

static void add_msg (const char *nick);
static void trigger_flood (User *u);
static int is_ignored (const char *nick);

void addtimer (const char *label)
{
    Timer *p;
    if (!gettimer(label)) {
	p = smalloc(sizeof(Timer));
	p->next = pings;
	p->prev = NULL;
	if (pings)
	    pings->prev = p;
	pings = p;
	p->label = sstrdup(label);
	p->start = time(NULL);
    }
}
void deltimer (const char *label)
{
    Timer *p;
    for (p = pings; p && stricmp(p->label, label)!=0; p = p->next) ;
    if (p) {
	if (p->next)
	    p->next->prev = p->prev;
	if (p->prev)
	    p->prev->next = p->next;
	else
	    pings = p->next;
	free(p->label);
	free(p);
    }
}
time_t gettimer (const char *label)
{
    Timer *p;
    for (p = pings; p && stricmp(p->label, label)!=0; p = p->next) ;
    if (p)
	return p->start;
    return 0;
}

#ifdef DEVNULL
/* Add message time to user dbase */
static void add_msg (const char *nick)
{
    User *u = finduser(nick);
    int i;
    if (is_ignored(nick)) return;
    if (is_oper(nick)) return;
    if (u) {
        if (u->messages == FLOOD_MESSAGES) u->messages--;
	for (i=0; i<FLOOD_MESSAGES; i++)
	    if (i+1 < FLOOD_MESSAGES && i+1 < u->messages)
		u->msg_times[i]=u->msg_times[i+1];
	    else {
		u->msg_times[i]=time(NULL);
		u->messages++;
		break;
	    }
	if (u->messages == FLOOD_MESSAGES &&
				(time(NULL) - u->msg_times[0] < FLOOD_TIME))
	    trigger_flood(u);
    }
}

/* Too many messages in too little time */
static void trigger_flood (User *u)
{
    u->messages = 0;
    u->flood_offence++;
    if (ignorecnt >= ignore_size) {
	if (ignore_size < 8)
	    ignore_size = 8;
	else
	    ignore_size *= 2;
	ignore = srealloc(ignore, sizeof(*ignore) * ignore_size);
    }
    strscpy(ignore[ignorecnt].nick, u->nick, NICKMAX);
    if (u->flood_offence < IGNORE_OFFENCES) {
	wallops(s_DevNull, "%s is FLOODING services (Placed on TEMP ignore).", u->nick);
	notice(s_DevNull, u->nick, "Services FLOOD triggered (>%d messages in %d seconds).  You are being ignored for %d seconds.",
				FLOOD_MESSAGES, FLOOD_TIME, IGNORE_TIME);
	log("TEMP ignore triggered on %s", u->nick);
	ignore[ignorecnt].start = time(NULL);
    } else {
	wallops(s_DevNull, "%s is FLOODING services (Placed on PERM ignore).", u->nick);
	notice(s_DevNull, u->nick, "Services FLOOD trigged >%d times on one login.  You are on perminant ignore.",
				IGNORE_OFFENCES);
	log("PERM ignore triggered on %s", u->nick);
	u->flood_offence = 0;
	ignore[ignorecnt].start = 0;
    }
    ignorecnt++;
}

static int is_ignored (const char *nick)
{
    int i;
    if (is_oper(nick)) return 0;
    for (i=0; i<ignorecnt; ++i)
	if (stricmp(nick, ignore[i].nick)==0) return 1;
    return 0;
}
#endif /* DEVNULL */

/*************************************************************************/

/* split_buf:  Split a buffer into arguments and store the arguments in an
 *             argument vector pointed to by argv (which will be malloc'd
 *             as necessary); return the argument count.  All argument
 *             values will also be malloc'd.  If colon_special is non-zero,
 *             then treat a parameter with a leading ':' as the last
 *             parameter of the line, per the IRC RFC.  Destroys the buffer
 *             by side effect.
 */

int split_buf(char *buf, char ***argv, int colon_special)
{
    int argvsize = 8;
    int argc;
    char *s, *t;

    if (!(*argv = malloc(sizeof(char *) * argvsize)))
	return -1;
    argc = 0;
    while (*buf) {
	if (argc == argvsize) {
	    argvsize += 8;
	    if (!(*argv = realloc(*argv, sizeof(char *) * argvsize)))
		return -1;
	}
	if (*buf == ':') {
	    (*argv)[argc++] = buf+1;
	    buf = "";
	} else {
	    s = strpbrk(buf, " ");
	    if (s) {
		*s++ = 0;
		while (isspace(*s))
		    s++;
	    } else
		s = buf + strlen(buf);
	    (*argv)[argc++] = buf;
	    buf = s;
	}
    }
    return argc;
}

/*************************************************************************/

/* process:  Main processing routine.  Takes the string in inbuf (global
 *           variable) and does something appropriate with it. */

void process()
{
    char source[64];
    char cmd[64];
    char buf[BUFSIZE];		/* Longest legal IRC command line */
    char *s;
    int ac;			/* Parameters for the command */
    char **av;
    FILE *f;
    time_t starttime, stoptime;	/* When processing started and finished */


    /* If debugging, log the buffer. */
    if(runflags & RUN_DEBUG)
	log("debug: Received: %s", inbuf);

    /* First make a copy of the buffer so we have the original in case we
     * crash - in that case, we want to know what we crashed on. */
    strscpy(buf, inbuf, sizeof(buf));

    /* Split the buffer into pieces. */
    if (*buf == ':') {
	s = strpbrk(buf, " ");
	if (!s)
	    return;
	*s = 0;
	while (isspace(*++s))
	    ;
	strscpy(source, buf+1, sizeof(source));
	strscpy(buf, s, BUFSIZE);
    } else
	*source = 0;
    if (!*buf)
	return;
    s = strpbrk(buf, " ");
    if (s) {
	*s = 0;
	while (isspace(*++s))
	    ;
    } else
	s = buf + strlen(buf);
    strscpy(cmd, buf, sizeof(cmd));
    ac = split_buf(s, &av, 1);



    /* Do something with the command. */
    if (stricmp(cmd, "PING")==0) {
	send_cmd(server_name, "PONG %s %s", ac>1 ? av[1] : server_name, av[0]);

/*    } else if (stricmp(cmd, "PONG")==0) {
	if (gettimer(av[0])) {
	    int i;
	    for (i=0;i<servcnt && stricmp(servlist[i].server, av[0])!=0;i++) ;
	    if (i<servcnt)
		servlist[i].lag = time(NULL) - gettimer(av[0]);
	    deltimer(av[0]);
	}
*/

    } else if (stricmp(cmd, "436")==0) {  /* Nick collision caused by us */
	User *u;
	if(services_level==1) introduce_user(av[0]);
	if((u = finduser(av[0]))) delete_user(u);

    } else if (stricmp(cmd, "401")==0) {  /* Non-Existant User/Channel */
	User *u;
	if((u = finduser(av[1]))) delete_user(u);

    } else if (stricmp(cmd, "AWAY")==0) {
	int i;
	if (ac == 0 || *av[0] == 0) {	/* un-away */
#ifdef MEMOS
	    if (services_level==1)
		check_memos(source);
#endif

#ifdef GLOBALNOTICER
	    /* Send global message to user when they set back */
	    for (i=0; i<nmessage; ++i)
		if (messages[i].type == M_LOGON)
		    notice(s_GlobalNoticer, source, "%s", messages[i].text);
	    /* Send global message to user when they set back */
	    if (is_oper(source))
		for (i=0; i<nmessage; ++i)
		    if (messages[i].type == M_OPER)
			notice(s_GlobalNoticer, source, "%s", messages[i].text);
#endif
	}

    } else if (stricmp(cmd, "GLOBOPS")==0
	    || stricmp(cmd, "GNOTICE")==0
	    || stricmp(cmd, "GOPER"  )==0
	    || stricmp(cmd, "WALLOPS")==0
	    || stricmp(cmd, "SQLINE"  )==0
	    || stricmp(cmd, "UNSQLINE")==0
	    || stricmp(cmd, "SVSNOOP")==0
	    || stricmp(cmd, "AKILL")==0
	    ) {

	/* Do nothing */

    } else if (stricmp(cmd, "JOIN")==0) {
	if (ac != 1)
	    return;
	do_join(source, ac, av);

    } else if (stricmp(cmd, "KICK")==0) {
	if (ac != 3)
	    return;
	do_kick(source, ac, av);

    } else if (stricmp(cmd, "KILL")==0 || stricmp(cmd, "SVSKILL")==0) {
	int i;
	ChannelInfo *ci;
	if (ac != 2)
	    return;
	do_kill(source, ac, av);
	if (is_services_nick(av[0])) introduce_user(av[0]);
#ifdef CHANSERV
	if (stricmp(av[0], s_ChanServ)==0)
	    for (i = 33; i < 256; ++i) {
		ci = chanlists[i];
		while (ci) {
		    if (findchan(source))
			if (ci->flags & CI_JOIN)
			    do_cs_join(ci->name);
		    else
			if (ci->mlock_key || (ci->mlock_on & CMODE_I))
		            do_cs_protect(ci->name);
		    ci = ci->next;
		}
	    }	
#endif

    } else if (stricmp(cmd, "MODE")==0 || stricmp(cmd, "SVSMODE")==0) {
	if (*av[0] == '#' || *av[0] == '&') {
	    if (ac < 2)
		return;
	    do_cmode(source, ac, av);
	} else {
	    if (ac != 2)
		return;
	    if (stricmp(cmd, "SVSMODE")==0)
		do_svumode(source, ac, av);
	    else
		do_umode(source, ac, av);
	}

    } else if (stricmp(cmd, "MOTD")==0) {
	FILE *f;
	char buf[BUFSIZE];

	f = fopen(MOTD_FILENAME, "r");
	if (f) {
		send_cmd(server_name, "375 %s :- %s Message of the Day",
			source, server_name);
		while (fgets(buf, sizeof(buf), f)) {
			buf[strlen(buf)-1] = 0;
			send_cmd(server_name, "372 %s :- %s", source, buf);
		}
		send_cmd(server_name, "376 %s :End of /MOTD command.", source);
		fclose(f);
	} else
		send_cmd(server_name, "422 %s :MOTD file not found!", source);

    } else if (stricmp(cmd, "NICK")==0 || stricmp(cmd, "SVSNICK")==0) {

#if defined(IRC_DALNET) || defined(IRC_UNDERNET)
# ifdef IRC_DALNET
#  ifdef DAL_SERV
	if ((!*source && ac != 8) || (*source && ac != 2))
#  else
	if ((!*source && ac != 7) || (*source && ac != 2))
#  endif
# else
	char *s = strchr(source, '.');
	if ((s && ac != 7) || (!s && ac != 2))
# endif
	    return;
	do_nick(source, ac, av);
#else
	/* Nothing to do yet; information comes from USER command. */
#endif

    } else if (stricmp(cmd, "NOTICE")==0) {
	/* Do nothing */

    } else if (stricmp(cmd, "PART")==0) {
	if (ac < 1 || ac > 2)
	    return;
	do_part(source, ac, av);

    } else if (stricmp(cmd, "PASS")==0) {
	/* Do nothing - we assume we're not being fooled */

    } else if (stricmp(cmd, "PRIVMSG")==0) {
      char buf[BUFSIZE];
      if (ac != 2)
	return;
      if (av[0][0]=='#')
	return;

#ifdef DEVNULL
      add_msg(source);
      if (!is_ignored(source)) {
#endif
#ifdef OPERSERV
	if (stricmp(av[0], s_OperServ)==0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_OperServ, source, "Access denied.");
	    if (!(runflags & RUN_MODE)) return;
#ifdef OUTLET
	} else if (stricmp(av[0], s_Outlet)==0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_Outlet, source, "Access denied.");
	    if (!(runflags & RUN_MODE)) return;
#endif
	} else if(runflags & RUN_MODE) {
#endif
#ifdef NICKSERV
	if (stricmp(av[0], s_NickServ)==0)
	    nickserv(source, av[1]);
#endif
#ifdef CHANSERV
	if (stricmp(av[0], s_ChanServ)==0)
	    chanserv(source, av[1]);
#endif
#ifdef MEMOSERV
	if (stricmp(av[0], s_MemoServ)==0)
	    memoserv(source, av[1]);
#endif
#ifdef HELPSERV
	if (stricmp(av[0], s_HelpServ)==0)
	    helpserv(s_HelpServ, source, av[1]);
#endif
#ifdef IRCIIHELP
	if (stricmp(av[0], s_IrcIIHelp)==0) {
	    char *s = smalloc(strlen(av[1]) + 7);
	    sprintf(s, "ircII %s", av[1]);
	    helpserv(s_IrcIIHelp, source, s);
	    free(s);
	}
#endif
#ifdef OPERSERV
	} else {
	    if (offreason)
		notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2 (%s).", offreason);
	    else
		notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2.");
	    return;
	}
#endif
#ifdef DAL_SERV
#ifdef OPERSERV
	snprintf(buf, sizeof(buf), "%s@%s", s_OperServ, server_name);
	if (stricmp(av[0], buf)==0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_OperServ, source, "Access denied.");
	    if (!(runflags & RUN_MODE)) return;
#ifdef OUTLET
	}
	snprintf(buf, sizeof(buf), "%s@%s", s_Outlet, server_name);
	if (stricmp(av[0], buf)==0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_Outlet, source, "Access denied.");
	    if (!(runflags & RUN_MODE)) return;
#endif
	} else if (runflags & RUN_MODE) {
#endif
#ifdef NICKSERV
	snprintf(buf, sizeof(buf), "%s@%s", s_NickServ, server_name);
	if (stricmp(av[0], buf)==0)
	    nickserv(source, av[1]);
#endif
#ifdef CHANSERV
	snprintf(buf, sizeof(buf), "%s@%s", s_ChanServ, server_name);
	if (stricmp(av[0], buf)==0)
	    chanserv(source, av[1]);
#endif
#ifdef MEMOSERV
	snprintf(buf, sizeof(buf), "%s@%s", s_MemoServ, server_name);
	if (stricmp(av[0], buf)==0)
	    memoserv(source, av[1]);
#endif
#ifdef HELPSERV
	snprintf(buf, sizeof(buf), "%s@%s", s_HelpServ, server_name);
	if (stricmp(av[0], buf)==0)
	    helpserv(s_HelpServ, source, av[1]);
#endif
#ifdef IRCIIHELP
	snprintf(buf, sizeof(buf), "%s@%s", s_IrcIIHelp, server_name);
	if (stricmp(av[0], buf)==0) {
	    char *s = smalloc(strlen(av[1]) + 7);
	    sprintf(s, "ircII %s", av[1]);
	    helpserv(s_IrcIIHelp, source, s);
	    free(s);
	}
#endif
#ifdef OPERSERV
	} else {
	    if (offreason)
		notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2 (%s).", offreason);
	    else
		notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2.");
	    return;
	}
#endif
#endif
#ifdef DEVNULL
      }
#endif

    } else if (stricmp(cmd, "QUIT")==0) {
	if (ac != 1)
	    return;
	do_quit(source, ac, av);
	if (is_services_nick(av[0])) introduce_user(av[0]);

    } else if (stricmp(cmd, "SERVER")==0) {
	if (servcnt >= serv_size) {
	    if (serv_size < 8)
		serv_size = 8;
	    else
		serv_size *= 2;
	    servlist = srealloc(servlist, sizeof(*servlist) * serv_size);
	}
	servlist[servcnt].server = sstrdup(av[0]);
	servlist[servcnt].hops = atoi(av[1]);
	servlist[servcnt].desc = sstrdup(av[2]);
	servlist[servcnt].lag = 0;
	servcnt++;

    } else if (stricmp(cmd, "SQUIT")==0) {
	int i;
	for (i=0;i<servcnt && stricmp(servlist[i].server, av[0])!=0;i++) ;
	if (i<servcnt) {
	    free(servlist[i].server);
	    free(servlist[i].desc);
	    --servcnt;
	    if (i < servcnt)
		bcopy(servlist+i+1, servlist+i, sizeof(*servlist) * (servcnt-i));
	}
	if (services_level!=1) introduce_user(NULL);

    } else if (stricmp(cmd, "TOPIC")==0) {
	if (ac != 4)
	    return;
	do_topic(source, ac, av);

    } else if (stricmp(cmd, "USER")==0) {
#if defined(IRC_CLASSIC) || defined(IRC_TS8)
	char *new_av[7];

#ifdef IRC_TS8
	if (ac != 5)
#else
	if (ac != 4)
#endif
	    return;
	new_av[0] = source;	/* Nickname */
	new_av[1] = "0";	/* # of hops (was in NICK command... we lose) */
#ifdef IRC_TS8
	new_av[2] = av[0];	/* Timestamp */
	av++;
#else
	new_av[2] = "0";
#endif
	new_av[3] = av[0];	/* Username */
	new_av[4] = av[1];	/* Hostname */
	new_av[5] = av[2];	/* Server */
	new_av[6] = av[3];	/* Real name */
	do_nick(source, 7, new_av);
#else
	/* Do nothing - we get everything we need from the NICK command. */
#endif

    } else if (stricmp(cmd, "VERSION")==0) {
	if (source)
	    send_cmd(server_name, "351 %s Magick %s v%s (%s%s%s%s%s%s%s%s%s%s%s%s%d) :-- %s%s",
			source, server_name, version_number,
#ifdef NICKSERV
			"N",
#else
			"n",
#endif
#ifdef CHANSERV
			"C",
#else
			"c",
#endif
#ifdef HELPSERV
			"H",
#else
			"h",
#endif
#ifdef IRCIIHELP
			"I",
#else
			"i",
#endif
#ifdef MEMOS
			"M",
#else
			"m",
#endif
#ifdef NEWS
			"W",
#else
			"w",
#endif
#ifdef DEVNULL
			"D",
#else
			"d",
#endif
#ifdef OPERSERV
			"O",
#else
			"o",
#endif
#ifdef AKILL
			"A",
#else
			"a",
#endif
#ifdef CLONES
			"L",
#else
			"l",
#endif
#ifdef OUTLET
			"T",
#else
			"t",
#endif
#ifdef GLOBALNOTICER
			"G",
#else
			"g",
#endif
			services_level, version_build,
			(runflags & RUN_DEBUG) ? " (debug mode)" : "");

    } else
	log("unknown message from server (%s)", inbuf);
}
