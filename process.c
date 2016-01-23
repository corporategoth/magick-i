/* Main processing code for Services.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include "version.h"

/*************************************************************************/

/* People to ignore (hashed by first character of nick). */

int ignorecnt = 0, ignore_size = 0;
Ignore *ignore = NULL;
int servcnt = 0, serv_size = 0;
Servers *servlist = NULL;
Timer *pings = NULL;
char motd_filename[512];
int flood_messages;
int flood_time;
int ignore_time;
int ignore_offences;
time_t last_squit;

static void add_msg (const char *nick);
static void trigger_flood (User * u);

void
addtimer (const char *label)
{
    Timer *p;
    if (!gettimer (label))
    {
	p = smalloc (sizeof (Timer));
	p->next = pings;
	p->prev = NULL;
	if (pings)
	    pings->prev = p;
	pings = p;
	p->label = sstrdup (label);
	p->start = time (NULL);
    }
}
void
deltimer (const char *label)
{
    Timer *p;
    for (p = pings; p && stricmp (p->label, label) != 0; p = p->next);
    if (p)
    {
	if (p->next)
	    p->next->prev = p->prev;
	if (p->prev)
	    p->prev->next = p->next;
	else
	    pings = p->next;
	if (p->label)
	    free (p->label);
	free (p);
    }
}
time_t
gettimer (const char *label)
{
    Timer *p;
    for (p = pings; p && stricmp (p->label, label) != 0; p = p->next);
    if (p)
	return p->start;
    return 0;
}

/* Add message time to user dbase */
static void
add_msg (const char *nick)
{
    User *u = finduser (nick);
    int i;
    if (is_ignored (nick))
	return;
    if (is_oper (nick))
	return;
    if (u)
    {
	if (u->messages == flood_messages)
	    u->messages--;
	for (i = 0; i < flood_messages; i++)
	    if (i + 1 < flood_messages && i + 1 < u->messages)
		u->msg_times[i] = u->msg_times[i + 1];
	    else
	    {
		u->msg_times[i] = time (NULL);
		u->messages++;
		break;
	    }
	if (u->messages == flood_messages &&
	    (time (NULL) - u->msg_times[0] < flood_time))
	    trigger_flood (u);
    }
}

/* Too many messages in too little time */
static void
trigger_flood (User * u)
{
    u->messages = 0;
    u->flood_offence++;
    if (ignorecnt >= ignore_size)
    {
	if (ignore_size < 8)
	    ignore_size = 8;
	else
	    ignore_size *= 2;
	ignore = srealloc (ignore, sizeof (*ignore) * ignore_size);
    }
    strscpy (ignore[ignorecnt].nick, u->nick, NICKMAX);
    if (u->flood_offence < ignore_offences)
    {
	wallops (s_DevNull, FLOODING, u->nick, "TEMP");
	notice (s_DevNull, u->nick, TEMP_FLOOD,
		flood_messages, flood_time, ignore_time);
	write_log ("TEMP ignore triggered on %s", u->nick);
	ignore[ignorecnt].start = time (NULL);
    }
    else
    {
	wallops (s_DevNull, FLOODING, u->nick, "PERM");
	notice (s_DevNull, u->nick, PERM_FLOOD,
		ignore_offences);
	write_log ("PERM ignore triggered on %s", u->nick);
	u->flood_offence = 0;
	ignore[ignorecnt].start = 0;
    }
    ignorecnt++;
}

int
is_ignored (const char *nick)
{
    int i;
    if (is_oper (nick))
	return 0;
    for (i = 0; i < ignorecnt; ++i)
	if (stricmp (nick, ignore[i].nick) == 0)
	    return 1;
    return 0;
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

int
split_buf (char *buf, char ***argv, int colon_special)
{
    int argvsize = 8;
    int argc;
    char *s;

    if (!(*argv = malloc (sizeof (char *) * argvsize)))
	  return -1;
    argc = 0;
    while (*buf)
    {
	if (argc == argvsize)
	{
	    argvsize += 8;
	    if (!(*argv = realloc (*argv, sizeof (char *) * argvsize)))
		  return -1;
	}
	if (*buf == ':')
	{
	    (*argv)[argc++] = buf + 1;
	    buf = "";
	}
	else
	{
	    s = strpbrk (buf, " ");
	    if (s)
	    {
		*s++ = 0;
		while (isspace (*s))
		    s++;
	    }
	    else
		s = buf + strlen (buf);
	    (*argv)[argc++] = buf;
	    buf = s;
	}
    }
    return argc;
}

/*************************************************************************/

/* process:  Main processing routine.  Takes the string in inbuf (global
 *           variable) and does something appropriate with it. */

void
process ()
{
    char source[64];
    char cmd[64];
    char buf[BUFSIZE];		/* Longest legal IRC command line */
    char *s;
    int ac, i;			/* Parameters for the command */
    char **av;

    /* If debugging, log the buffer. */
    if (runflags & RUN_DEBUG)
	write_log ("debug: Received: %s", inbuf);

    /* First make a copy of the buffer so we have the original in case we
     * crash - in that case, we want to know what we crashed on. */
    strscpy (buf, inbuf, sizeof (buf));

    /* Split the buffer into pieces. */
    if (*buf == ':')
    {
	s = strpbrk (buf, " ");
	if (!s)
	    return;
	*s = 0;
	while (isspace (*++s))
	    ;
	strscpy (source, buf + 1, sizeof (source));
	strscpy (buf, s, BUFSIZE);
    }
    else
	*source = 0;
/*    if(s)
	free(s); */
    if (!*buf)
	return;
    s = strpbrk (buf, " ");
    if (s)
    {
	*s = 0;
	while (isspace (*++s))
	    ;
    }
    else
	s = buf + strlen (buf);
    strscpy (cmd, buf, sizeof (cmd));
    ac = split_buf (s, &av, 1);
/*    if (s)
	free(s); */


    /* Do something with the command. */
    if (stricmp (cmd, "PING") == 0)
    {
	send_cmd (server_name, "PONG %s %s", ac > 1 ? av[1] : server_name, av[0]);

/*    } else if (stricmp(cmd, "PONG")==0) {
   if (gettimer(av[0])) {
   int i;
   for (i=0;i<servcnt && stricmp(servlist[i].server, av[0])!=0;i++) ;
   if (i<servcnt)
   servlist[i].lag = time(NULL) - gettimer(av[0]);
   deltimer(av[0]);
   }
 */

    }
    else if (stricmp (cmd, "436") == 0)
    {				/* Nick collision caused by us */
	User *u;
	if (services_level == 1)
	    introduce_user (av[0]);
	if ((u = finduser (av[0])))
	    delete_user (u);

    }
    else if (stricmp (cmd, "401") == 0)
    {				/* Non-Existant User/Channel */
	User *u;
	if ((u = finduser (av[1])))
	    delete_user (u);

    }
    else if (stricmp (cmd, "AWAY") == 0)
    {
	int i;
	if (ac == 0 || *av[0] == 0)
	{			/* un-away */
	    if(memos_on==TRUE)
		if (services_level == 1)
		    check_memos (source);

	    if(globalnoticer_on==TRUE)
	    {
		/* Send global message to user when they set back */
		for (i = 0; i < nmessage; ++i)
		    if (messages[i].type == M_LOGON)
			notice (s_GlobalNoticer, source, "%s", messages[i].text);
		/* Send global message to user when they set back */
		if (is_oper (source))
		    for (i = 0; i < nmessage; ++i)
			if (messages[i].type == M_OPER)
			    notice (s_GlobalNoticer, source, "\37[\37\2OPER\2\37]\37 %s", messages[i].text);
	    }
	}
    }
    else if (   stricmp (cmd, "GLOBOPS" ) == 0
	     || stricmp (cmd, "GNOTICE" ) == 0
	     || stricmp (cmd, "GOPER"   ) == 0
	     || stricmp (cmd, "WALLOPS" ) == 0
	     || stricmp (cmd, "SQLINE"  ) == 0
	     || stricmp (cmd, "UNSQLINE") == 0
	     || stricmp (cmd, "SVSNOOP" ) == 0
	     || stricmp (cmd, "AKILL"   ) == 0
	     || stricmp (cmd, "PROTOCTL") == 0
	     || stricmp (cmd, "324"     ) == 0
	     || stricmp (cmd, "329"     ) == 0
	     || stricmp (cmd, "421"     ) == 0
	)
    {

	/* Do nothing */

    }
    else if (stricmp (cmd, "JOIN") == 0)
    {
	if (ac != 1)
	    goto endfunc;
	do_join (source, ac, av);

    }
    else if (stricmp (cmd, "KICK") == 0)
    {
	if (ac != 3)
	    goto endfunc;
	do_kick (source, ac, av);

    }
    else if (stricmp (cmd, "KILL") == 0 || stricmp (cmd, "SVSKILL") == 0)
    {
	int i;
	ChannelInfo *ci;
	if (ac != 2)
	    goto endfunc;
	do_kill (source, ac, av);
	if (is_services_nick (av[0]))
	    introduce_user (av[0]);
	if(chanserv_on==TRUE)
	{
	    if (stricmp (av[0], s_ChanServ) == 0)
		for (i = 33; i < 256; ++i)
		{
		    ci = chanlists[i];
		    while (ci)
		    {
			if (findchan (source))
			    if (ci->flags & CI_JOIN)
				do_cs_join (ci->name);
			    else if (ci->mlock_key || hasmode(ci->mlock_on, "i"))
				do_cs_protect (ci->name);
			ci = ci->next;
		    }
		}
	}
    }
    else if (stricmp (cmd, "MODE") == 0 || stricmp (cmd, "SVSMODE") == 0)
    {
	if (validchan(av[0]))
	{
	    if (ac < 2)
		goto endfunc;
	    do_cmode (source, ac, av);
	}
	else
	{
	    if (ac != 2)
		goto endfunc;
	    if (stricmp (cmd, "SVSMODE") == 0)
		do_svumode (source, ac, av);
	    else
		do_umode (source, ac, av);
	}

    }
    else if (stricmp (cmd, "MOTD") == 0)
    {
	FILE *f;
	char buf[BUFSIZE];

	f = fopen (motd_filename, "r");
	if (f)
	{
	    send_cmd (server_name, "375 %s :- %s Message of the Day",
		      source, server_name);
	    while (fgets (buf, sizeof (buf), f))
	    {
		buf[strlen (buf) - 1] = 0;
		send_cmd (server_name, "372 %s :- %s", source, buf);
	    }
	    send_cmd (server_name, "376 %s :End of /MOTD command.", source);
	    fclose (f);
	}
	else
	    send_cmd (server_name, "422 %s :MOTD file not found!", source);

    }
    else if (stricmp (cmd, "NICK") == 0 || stricmp (cmd, "SVSNICK") == 0)
    {

#if defined(IRC_DALNET) || defined(IRC_UNDERNET)
#ifdef IRC_DALNET
#ifdef DAL_SERV
	if ((!*source && ac != 8) || (*source && ac != 2))
#else
	if ((!*source && ac != 7) || (*source && ac != 2))
#endif
#else
	char *s = strchr (source, '.');
	if ((s && ac != 7) || (!s && ac != 2))
#endif
	    goto endfunc;
	do_nick (source, ac, av);
#else
	/* Nothing to do yet; information comes from USER command. */
#endif

    }
    else if (stricmp (cmd, "NOTICE") == 0)
    {
	/* Do nothing */

    }
    else if (stricmp (cmd, "PART") == 0)
    {
	if (ac < 1 || ac > 2)
	    goto endfunc;
	do_part (source, ac, av);

    }
    else if (stricmp (cmd, "PASS") == 0)
    {
	/* Do nothing - we assume we're not being fooled */
    }
    else if (stricmp (cmd, "PRIVMSG") == 0)
    {
	char buf[BUFSIZE];
	if (ac != 2)
	    goto endfunc;
	if (validchan(av[0]))
	    goto endfunc;

	if (devnull_on==TRUE)
	    add_msg (source);
	if (!is_ignored (source))
	{
	    if (operserv_on==TRUE)
	    {
		snprintf (buf, sizeof (buf), "%s@%s", s_OperServ, server_name);
		if (stricmp (av[0], s_OperServ) == 0 ||
		    stricmp (av[0], buf) == 0)
		{
		    if (is_oper (source))
			operserv (source, av[1]);
		    else
			notice (s_OperServ, source, ERR_ACCESS_DENIED);
		    if (!(runflags & RUN_MODE))
			goto endfunc;
		}
	    }
	    if (outlet_on==TRUE)
	    {
		snprintf (buf, sizeof (buf), "%s@%s", s_Outlet, server_name);
		if (stricmp (av[0], s_Outlet) == 0 ||
		    stricmp (av[0], buf) == 0)
		{
		    if (is_oper (source))
			operserv (source, av[1]);
		    else
			notice (s_Outlet, source, ERR_ACCESS_DENIED);
		    if (!(runflags & RUN_MODE))
			goto endfunc;
		}
	    }
	    if (runflags & RUN_MODE)
	    {
		if (nickserv_on==TRUE)
		{
		    snprintf (buf, sizeof (buf), "%s@%s", s_NickServ, server_name);
		    if (stricmp (av[0], s_NickServ) == 0 ||
			stricmp (av[0], buf) == 0)
			nickserv (source, av[1]);
		}
		if (chanserv_on==TRUE)
		{
		    snprintf (buf, sizeof (buf), "%s@%s", s_ChanServ, server_name);
		    if (stricmp (av[0], s_ChanServ) == 0 ||
			stricmp (av[0], buf) == 0)
			chanserv (source, av[1]);
		}
		if (memoserv_on==TRUE)
		{
		    snprintf (buf, sizeof (buf), "%s@%s", s_MemoServ, server_name);
		    if (stricmp (av[0], s_MemoServ) == 0 ||
			stricmp (av[0], buf) == 0)
			memoserv (source, av[1]);
		}
		if (helpserv_on==TRUE)
		{
		    snprintf (buf, sizeof (buf), "%s@%s", s_HelpServ, server_name);
		    if (stricmp (av[0], s_HelpServ) == 0 ||
			stricmp (av[0], buf) == 0)
			helpserv (s_HelpServ, source, av[1]);
		}
		if (irciihelp_on==TRUE)
		{
		    snprintf (buf, sizeof (buf), "%s@%s", s_IrcIIHelp, server_name);
		    if (stricmp (av[0], s_IrcIIHelp) == 0 ||
			stricmp (av[0], buf) == 0)
		    {
			char *s = smalloc (strlen (av[1]) + 7);
			sprintf (s, "ircII %s", av[1]);
			helpserv (s_IrcIIHelp, source, s);
			free (s);
		    }
		}
	    }
	    else
	    {
		if (offreason)
		    notice (s_OperServ, source, SERVICES_OFF_REASON, offreason);
		else
		    notice (s_OperServ, source, SERVICES_OFF);
		goto endfunc;
	    }
	}
    }
    else if (stricmp (cmd, "QUIT") == 0)
    {
	if (ac != 1)
	    goto endfunc;
	do_quit (source, ac, av);
	if (is_services_nick (av[0]))
	    introduce_user (av[0]);

    }
    else if (stricmp (cmd, "SERVER") == 0)
    {
	if (servcnt >= serv_size)
	{
	    if (serv_size < 8)
		serv_size = 8;
	    else
		serv_size *= 2;
	    servlist = srealloc (servlist, sizeof (*servlist) * serv_size);
	}
	servlist[servcnt].server = sstrdup (av[0]);
	servlist[servcnt].hops = atoi (av[1]);
	servlist[servcnt].desc = sstrdup (av[2]);
	servlist[servcnt].lag = 0;
	servcnt++;

    }
    else if (stricmp (cmd, "SQUIT") == 0)
    {
	int i;
	for (i = 0; i < servcnt && stricmp (servlist[i].server, av[0]) != 0; i++);
	if (i < servcnt)
	{
	    if (servlist[i].server)
		free (servlist[i].server);
	    if (servlist[i].desc)
		free (servlist[i].desc);
	    --servcnt;
	    if (i < servcnt)
		bcopy (servlist + i + 1, servlist + i, sizeof (*servlist) * (servcnt - i));
	}
	if (services_level != 1 && last_squit < time(NULL)) {
	    last_squit = time(NULL);
	    introduce_user (NULL);
	}
    }
    else if (stricmp (cmd, "TOPIC") == 0)
    {
	if (ac != 4)
	    goto endfunc;
	do_topic (source, ac, av);

    }
    else if (stricmp (cmd, "USER") == 0)
    {
#if defined(IRC_CLASSIC) || defined(IRC_TS8)
	char *new_av[7];

#ifdef IRC_TS8
	if (ac != 5)
#else
	if (ac != 4)
#endif
	    goto endfunc;
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
	do_nick (source, 7, new_av);
#else
	/* Do nothing - we get everything we need from the NICK command. */
#endif

    }
    else if (stricmp (cmd, "VERSION") == 0)
    {
	if (source)
	    send_cmd (server_name, "351 %s Magick %s v%s%s (%s%s%s%s%s%s%s%s%s%s%s%s%s%d) :-- %s%s",
		      source, server_name, version_number,
#ifdef WIN32
		      "+Win32",
#else
		      "",
#endif
		      (nickserv_on==TRUE)?"N":"n",
		      (chanserv_on==TRUE)?"C":"c",
		      (helpserv_on==TRUE)?"H":"h",
		      (irciihelp_on==TRUE)?"I":"i",
		      (memos_on==TRUE)?"M":"m",
		      (news_on==TRUE)?"W":"w",
		      (devnull_on==TRUE)?"D":"d",
		      (operserv_on==TRUE)?"O":"o",
		      (akill_on==TRUE)?"A":"a",
		      (clones_on==TRUE)?"L":"l",
		      (outlet_on==TRUE)?"T":"t",
		      (globalnoticer_on==TRUE)?"G":"g",
		      (show_sync_on==TRUE)?"Y":"y",
		      services_level, version_build,
		      (runflags & RUN_DEBUG) ? " (debug mode)" : "");
    }
    else
	write_log ("unknown message from server (%s)", inbuf);

    endfunc:
/*    for (i=ac-1; i>=0; i--)
	if (av[i])
	    free(av[i]); */
    if (av)
	free(av);
}
