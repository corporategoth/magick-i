/* Main processing code for Services.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include "version.h"

/*************************************************************************/

/* Use ignore code? */

int allow_ignore = 1;

/* People to ignore (hashed by first character of nick). */

IgnoreData *ignore[256];


/* add_ignore: Add someone to the ignorance list for the next `delta'
 *             seconds.
 */

void add_ignore(const char *nick, time_t delta)
{
    IgnoreData *ign;
    char who[NICKMAX];
    time_t now = time(NULL);
    IgnoreData **whichlist = &ignore[tolower(nick[0])];

    strscpy(who, nick, NICKMAX);
    for (ign = *whichlist; ign; ign = ign->next) {
	if (stricmp(ign->who, who) == 0)
	    break;
    }
    if (ign) {
	if (ign->time > now)
	    ign->time += delta;
	else
	    ign->time = now + delta;
    } else {
	ign = smalloc(sizeof(*ign));
	strcpy(ign->who, who);
	ign->time = now + delta;
	ign->next = *whichlist;
	*whichlist = ign;
    }
}


/* get_ignore: Retrieve an ignorance record for a nick.  If the nick isn't
 *             being ignored, return NULL and flush the record from the
 *             in-core list if it exists (i.e. ignore timed out).
 */

IgnoreData *get_ignore(const char *nick)
{
    IgnoreData *ign, *prev;
    time_t now = time(NULL);
    IgnoreData **whichlist = &ignore[tolower(nick[0])];

    for (ign = *whichlist, prev = NULL; ign; prev = ign, ign = ign->next) {
	if (stricmp(ign->who, nick) == 0)
	    break;
    }
    if (ign && ign->time <= now) {
	if (prev)
	    prev->next = ign->next;
	else
	    *whichlist = ign->next;
	free(ign);
	ign = NULL;
    }
    return ign;
}

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
	    } else {
		s = buf + strlen(buf);
	    }
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
    char buf[512];		/* Longest legal IRC command line */
    char *s;
    int ac;			/* Parameters for the command */
    char **av;
    FILE *f;
    time_t starttime, stoptime;	/* When processing started and finished */


    /* If debugging, log the buffer. */
    if(debug)
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
	strcpy(buf, s);
    } else {
	*source = 0;
    }
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

    if (stricmp(cmd, "PING") == 0) {

	send_cmd(server_name, "PONG %s %s", ac>1 ? av[1] : server_name, av[0]);

    } else if (stricmp(cmd, "436") == 0) {  /* Nick collision caused by us */

   if(services_level==1) introduce_user(av[0]);

    } else if (stricmp(cmd, "AWAY") == 0) {
#ifdef GLOBALNOTICER
	FILE *f;
	char buf[BUFSIZE];
#endif

	if (ac == 0 || *av[0] == 0) {	/* un-away */
#ifdef MEMOS
	    check_memos(source);
#endif

#ifdef GLOBALNOTICER
	    /* Send global message to user when they set back */
	    if (f = fopen(LOGON_MSG, "r")) {
		while (fgets(buf, sizeof(buf), f)) {
		    buf[strlen(buf)-1] = 0;
		    notice(s_GlobalNoticer, source, "%s", buf ? buf : " ");
		}
		fclose(f);
	    }
	    /* Send global message to user when they set back */
	    if (is_oper(source)) {
		if (f = fopen(OPER_MSG, "r")) {
		    while (fgets(buf, sizeof(buf), f)) {
			buf[strlen(buf)-1] = 0;
			notice(s_GlobalNoticer, source, "\37[\37\2OPER\2\37]\37 %s", buf ? buf : " ");
		    }
		    fclose(f);
		}
	    }
#endif
	}

    } else if (stricmp(cmd, "GLOBOPS") == 0
	    || stricmp(cmd, "GNOTICE") == 0
	    || stricmp(cmd, "GOPER"  ) == 0
	    || stricmp(cmd, "WALLOPS") == 0
	    || stricmp(cmd, "QLINE"  ) == 0
	    || stricmp(cmd, "UNQLINE") == 0
	    || stricmp(cmd, "SVSNOOP") == 0
	    ) {

	/* Do nothing */

    } else if (stricmp(cmd, "JOIN") == 0) {

	if (ac != 1)
	    return;
	do_join(source, ac, av);

    } else if (stricmp(cmd, "KICK") == 0) {

	if (ac != 3)
	    return;
	do_kick(source, ac, av);

    } else if (stricmp(cmd, "KILL") == 0 || stricmp(cmd, "SVSKILL") == 0) {

	if (ac != 2)
	    return;
	do_kill(source, ac, av);
	if (is_services_nick(av[0])) introduce_user(av[0]);

    } else if (stricmp(cmd, "MODE") == 0 || stricmp(cmd, "SVSMODE") == 0) {

	if (*av[0] == '#' || *av[0] == '&') {
	    if (ac < 2)
		return;
	    do_cmode(source, ac, av);
	} else {
	    if (ac != 2)
		return;
	    do_umode(source, ac, av);
	}

    } else if (stricmp(cmd, "MOTD") == 0) {

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
	} else {
		send_cmd(server_name, "422 %s :MOTD file not found!", source);
	}

    } else if (stricmp(cmd, "NICK") == 0 || stricmp(cmd, "SVSNICK") == 0) {

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

    } else if (stricmp(cmd, "NOTICE") == 0) {

	/* Do nothing */

    } else if (stricmp(cmd, "PART") == 0) {

	if (ac < 1 || ac > 2)
	    return;
	do_part(source, ac, av);

    } else if (stricmp(cmd, "PASS") == 0) {

	/* Do nothing - we assume we're not being fooled */

    } else if (stricmp(cmd, "PRIVMSG") == 0) {

	char buf[BUFSIZE];
	if (ac != 2)
	    return;

	/* Check if we should ignore.  Operators always get through. */
	if (allow_ignore && !is_oper(source)) {
	    IgnoreData *ign = get_ignore(source);
	    if (ign && ign->time > time(NULL)) {
		log("Ignored message from %s: \"%s\"", source, inbuf);
		return;
	    }
	}

	starttime = time(NULL);

#ifdef OPERSERV
	if (stricmp(av[0], s_OperServ) == 0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_OperServ, source, "Access denied.");
	    if (mode==0) return;
	} else if(mode!=0) {
#endif
#ifdef NICKSERV
	if (stricmp(av[0], s_NickServ) == 0)
	    nickserv(source, av[1]);
#endif
#ifdef CHANSERV
	if (stricmp(av[0], s_ChanServ) == 0)
	    chanserv(source, av[1]);
#endif
#ifdef MEMOSERV
	if (stricmp(av[0], s_MemoServ) == 0)
	    memoserv(source, av[1]);
#endif
#ifdef HELPSERV
	if (stricmp(av[0], s_HelpServ) == 0)
	    helpserv(s_HelpServ, source, av[1]);
#endif
#ifdef IRCIIHELP
	if (stricmp(av[0], s_IrcIIHelp) == 0) {
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
	sprintf(buf, "%s@%s", s_OperServ, SERVER_NAME);
	if (stricmp(av[0], buf) == 0) {
	    if (is_oper(source))
		operserv(source, av[1]);
	    else
		notice(s_OperServ, source, "Access denied.");
	    if (mode==0) return;
	} else if (mode!=0) {
#endif
#ifdef NICKSERV
	sprintf(buf, "%s@%s", s_NickServ, SERVER_NAME);
	if (stricmp(av[0], buf) == 0)
	    nickserv(source, av[1]);
#endif
#ifdef CHANSERV
	sprintf(buf, "%s@%s", s_ChanServ, SERVER_NAME);
	if (stricmp(av[0], buf) == 0)
	    chanserv(source, av[1]);
#endif
#ifdef MEMOSERV
	sprintf(buf, "%s@%s", s_MemoServ, SERVER_NAME);
	if (stricmp(av[0], buf) == 0)
	    memoserv(source, av[1]);
#endif
#ifdef HELPSERV
	sprintf(buf, "%s@%s", s_HelpServ, SERVER_NAME);
	if (stricmp(av[0], buf) == 0)
	    helpserv(s_HelpServ, source, av[1]);
#endif
#ifdef IRCIIHELP
	sprintf(buf, "%s@%s", s_IrcIIHelp, SERVER_NAME);
	if (stricmp(av[0], buf) == 0) {
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

	/*Add to ignore list if the command took a significant amount of time.*/
	if (allow_ignore) {
	    stoptime = time(NULL);
	    if (stoptime > starttime && *source && !strchr(source, '.'))
		add_ignore(source, stoptime-starttime);
	}

    } else if (stricmp(cmd, "QUIT") == 0) {

	if (ac != 1)
	    return;
	do_quit(source, ac, av);
	if (is_services_nick(av[0])) introduce_user(av[0]);

    } else if (stricmp(cmd, "SERVER") == 0) {

	/* nothing */

    } else if (stricmp(cmd, "SQUIT") == 0) {

	if (services_level!=1) introduce_user(NULL);

    } else if (stricmp(cmd, "TOPIC") == 0) {

	if (ac != 4)
	    return;
	do_topic(source, ac, av);

    } else if (stricmp(cmd, "USER") == 0) {

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

    } else if (stricmp(cmd, "VERSION") == 0) {

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
#ifdef IRCOP_OVERRIDE
			"V",
#else
			"v",
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
#ifdef GLOBALNOTICER
			"G",
#else
			"g",
#endif
			services_level, version_build,
			debug ? " (debug mode)" : "");

    } else {

	log("unknown message from server (%s)", inbuf);

    }

}
