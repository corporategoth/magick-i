/* Functions for handling user actions.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

User *userlist = NULL;

int usercnt = 0, opcnt = 0, maxusercnt = 0;
char services_admin[1024];

/*************************************************************************/
/*************************************************************************/

/* Allocate a new User structure, fill in basic values, link it to the
 * overall list, and return it.  Always successful.
 */

static User *
new_user (const char *nick)
{
    User *user;

    user = scalloc (sizeof (User), 1);
    if (!nick)
	nick = "";
    strscpy (user->nick, nick, NICKMAX);
    user->messages = user->flood_offence = user->passfail = 0;
    user->next = userlist;
    if (userlist)
	userlist->prev = user;
    userlist = user;
    usercnt++;
    if (usercnt > maxusercnt)
	maxusercnt = usercnt;
    return user;
}

/*************************************************************************/

/* Change the nickname of a user, and move pointers as necessary. */

void
change_user_nick (User * user, const char *nick)
{
    strscpy (user->nick, nick, NICKMAX);
}

/*************************************************************************/

/* Remove and free a User structure. */

void
delete_user (User * user)
{
    struct u_chanlist *c, *c2;
    struct u_chaninfolist *ci, *ci2;

    usercnt--;
    if (hasmode("o", user->mode))
	opcnt--;
    if(nickserv_on==TRUE)
	cancel_user (user);
    if(clones_on==TRUE)
	clones_del (user->host);
    free (user->username);
    free (user->host);
    free (user->realname);
    free (user->server);
    c = user->chans;
    while (c)
    {
	c2 = c->next;
	chan_deluser (user, c->chan);
	free (c);
	c = c2;
    }
    if(chanserv_on==TRUE)
    {
	ci = user->founder_chans;
	while (ci)
	{
	    ci2 = ci->next;
	    free (ci);
	    ci = ci2;
	}
    }
    if (user->prev)
	user->prev->next = user->next;
    else
	userlist = user->next;
    if (user->next)
	user->next->prev = user->prev;
    free (user);
}

/*************************************************************************/
/*************************************************************************/

/* Return statistics.  Pointers are assumed to be valid. */

void
get_user_stats (long *nusers, long *memuse)
{
    long count = 0, mem = 0;
    User *user;
    struct u_chanlist *uc;
    struct u_chaninfolist *uci;

    for (user = userlist; user; user = user->next)
    {
	count++;
	mem += sizeof (*user);
	if (user->username)
	    mem += strlen (user->username) + 1;
	if (user->host)
	    mem += strlen (user->host) + 1;
	if (user->realname)
	    mem += strlen (user->realname) + 1;
	if (user->server)
	    mem += strlen (user->server) + 1;
	for (uc = user->chans; uc; uc = uc->next)
	    mem += sizeof (*uc);
	if(chanserv_on==TRUE)
	{
	    for (uci = user->founder_chans; uci; uci = uci->next)
		mem += sizeof (*uci);
	}
    }
    *nusers = count;
    *memuse = mem;
}

/*************************************************************************/

/* Send the current list of users to the named user. */

void
send_user_list (const char *who, const char *source, const char *x)
{
    User *u;

    for (u = userlist; u; u = u->next)
    {
	if (!x || match_wild_nocase (x, u->nick))
	{
	    char buf[BUFSIZE], *s;
	    struct u_chanlist *c;
	    struct u_chaninfolist *ci;
	    notice (who, source, "%s!%s@%s +%s %ld %s :%s",
		    u->nick, u->username, u->host, u->mode, u->signon,
		    u->server, u->realname);
	    buf[0] = 0;
	    s = buf;
	    for (c = u->chans; c; c = c->next)
		s += snprintf (s, BUFSIZE, " %s", c->chan->name);
	    notice (who, source, "%s%s", u->nick, buf);
	    if(chanserv_on==TRUE)
	    {
		buf[0] = 0;
		s = buf;
		for (ci = u->founder_chans; ci; ci = ci->next)
		    s += snprintf (s, BUFSIZE, " %s", ci->chan->name);
		notice (who, source, "%s%s", u->nick, buf);
	    }
	}
    }
}

/*************************************************************************/

/* Send the current list of users to the named user. */

void
send_usermask_list (const char *who, const char *source, const char *x)
{
    char assym[BUFSIZE];
    User *u;

    for (u = userlist; u; u = u->next)
    {
	snprintf (assym, BUFSIZE, "%s@%s", u->username, u->host);
	if (!x || match_wild_nocase (x, assym))
	{
	    char buf[BUFSIZE], *s;
	    struct u_chanlist *c;
	    struct u_chaninfolist *ci;
	    notice (who, source, "%s!%s@%s +%s %ld %s :%s",
		    u->nick, u->username, u->host, u->mode, u->signon,
		    u->server, u->realname);
	    buf[0] = 0;
	    s = buf;
	    for (c = u->chans; c; c = c->next)
		s += snprintf (s, BUFSIZE, " %s", c->chan->name);
	    notice (who, source, "%s%s", u->nick, buf);
	    if(chanserv_on==TRUE)
	    {
		buf[0] = 0;
		s = buf;
		for (ci = u->founder_chans; ci; ci = ci->next)
		    s += snprintf (s, BUFSIZE, " %s", ci->chan->name);
		notice (who, source, "%s%s", u->nick, buf);
	    }
	}
    }
}

/*************************************************************************/

/* Send the current list of users to the named user. */

void
send_usermode_list (const char *who, const char *source, const char *x)
{
    User *u;

    for (u = userlist; u; u = u->next)
    {
	if (!x || hasmode(x, u->mode))
	{
	    char buf[BUFSIZE], *s;
	    struct u_chanlist *c;
	    struct u_chaninfolist *ci;
	    notice (who, source, "%s!%s@%s +%s %ld %s :%s",
		    u->nick, u->username, u->host, u->mode, u->signon,
		    u->server, u->realname);
	    buf[0] = 0;
	    s = buf;
	    for (c = u->chans; c; c = c->next)
		s += snprintf (s, BUFSIZE, " %s", c->chan->name);
	    notice (who, source, "%s%s", u->nick, buf);
	    if(chanserv_on==TRUE)
	    {
		buf[0] = 0;
		s = buf;
		for (ci = u->founder_chans; ci; ci = ci->next)
		    s += snprintf (s, BUFSIZE, " %s", ci->chan->name);
		notice (who, source, "%s%s", u->nick, buf);
	    }
	}
    }
}

/*************************************************************************/

/* Find a user by nick.  Return NULL if user could not be found. */

User *
finduser (const char *nick)
{
    User *user;
    for (user = userlist; user && stricmp (user->nick, nick) != 0;
	 user = user->next);
    return user;
}

/* Find a user by nick!user@host.  Specify which match to return
 * (for when there are multipal matches).  Return NULL if specified
 * match could not be found.
 */
User *
findusermask (const char *mask, int matchno)
{
    User *user;
    int cnt = 0;

    for (user = userlist; user; user = user->next)
    {
	if (match_usermask (mask, user))
	    cnt++;
	if (cnt == matchno)
	    break;
    }
    return user;
}

/* Return number of users matching nick!user@host */
int
countusermask (const char *mask)
{
    User *user;
    int cnt = 0;

    for (user = userlist; user; user = user->next)
	if (match_usermask (mask, user))
	    cnt++;
    return cnt;
}

/*************************************************************************/
/*************************************************************************/

/* Handle a server NICK command.
 *    av[0] = nick
 *      If a new user:
 *              av[1] = hop count
 *              av[2] = signon time
 *              av[3] = username
 *              av[4] = hostname
 *              av[5] = user's server
 *              WITH DALnet 4.4.15+
 *                      av[6] = 1 = service 0 = user
 *                      av[7] = user's real name
 *              OTHERWISE
 *                      av[6] = user's real name
 *      Else:
 *              av[1] = time of change
 */

void
do_nick (const char *source, int ac, char **av)
{
    User *user;

    if (!*source)
    {
	/* This is a new user; create a User structure for it. */
	if (runflags & RUN_DEBUG)
	    write_log ("debug: new user: %s", av[0]);

	/* Ignore the ~ from a lack of identd.  (Identd was never meant
	 * to be a security protocol.  Read the RFCs, people!) */
	if (av[3][0] == '~')
	    ++av[3];

	if(akill_on==TRUE)
	{
	    /* First check for AKILLs. */
	    if (!i_am_backup ())
		if (check_akill (av[0], av[3], av[4]))
		    return;
	}

	/* Allocate User structure and fill it in. */
	user = new_user (av[0]);
	user->signon = atol (av[2]);
	user->username = sstrdup (av[3]);
	user->host = sstrdup (av[4]);
	user->server = sstrdup (av[5]);
#ifdef DAL_SERV
	user->realname = sstrdup (av[7]);
#else
	user->realname = sstrdup (av[6]);
#endif

	/* Check to see if it looks like clones. */
	if(clones_on==TRUE)
	{
	    if (!is_services_nick (av[0]))
		clones_add (av[0], av[4]);
	}

	if(globalnoticer_on==TRUE)
	{
	    /* Send global message to user when they log on */
	    if (!is_services_nick (av[0]) && !i_am_backup ())
	    {
		int i;
		for (i = 0; i < nmessage; ++i)
		    if (messages[i].type == M_LOGON)
			notice (s_GlobalNoticer, av[0], "%s", messages[i].text);
	    }
	}
	if (devnull_on==TRUE && is_ignored (av[0]))
	    notice(s_DevNull, av[0], IS_IGNORED);

    }
    else
    {
	/* An old user changing nicks. */
	long prefl = 0;
	NickInfo *ni;
	user = finduser (source);
	if (!user)
	{
	    write_log ("user: NICK from nonexistent nick %s: %s", source,
		       merge_args (ac, av));
	    return;
	}
	if (runflags & RUN_DEBUG)
	    write_log ("debug: %s changes nick to %s", source, av[0]);
	if(nickserv_on==TRUE)
	{
	    if (ni = findnick (user->nick))
	    {
		prefl = ni->flags;
	    }
	    cancel_user (user);
	    if (prefl && (ni = findnick (av[0])))
	    {
		if (stricmp (host (ni)->nick, host (findnick(user->nick))->nick) == 0)
		{
		    if (prefl & NI_RECOGNIZED)
			ni->flags |= NI_RECOGNIZED;
		    if (prefl & NI_IDENTIFIED)
			ni->flags |= NI_IDENTIFIED;
#ifdef DAL_SERV
		} else {
		    if (!is_services_nick(av[0]) && !i_am_backup())
		    {
			send_cmd(any_service(), "SVSMODE %s -Rra", user->nick);
			strscpy(user->mode, changemode("-Rra", user->mode), sizeof(user->mode));
		    }
#endif
		}
	    }
#ifdef DAL_SERV
	    else
	    {
	    if (!is_services_nick(av[0]) && !i_am_backup())
		send_cmd(any_service(), "SVSMODE %s -Rra", user->nick);
		strscpy(user->mode, changemode("-Rra", user->mode), sizeof(user->mode));
	    }
#endif
	}
#ifdef DAL_SERV
	else
	{
	if (!is_services_nick(av[0]) && !i_am_backup())
	    send_cmd(any_service(), "SVSMODE %s -Rra", user->nick);
	    strscpy(user->mode, changemode("-Rra", user->mode), sizeof(user->mode));
	}
#endif
	change_user_nick (user, av[0]);
/*	user->signon = atol (av[1]); */
    }
    user->my_signon = time (NULL);

#ifdef DAL_SERV
    if (!is_services_nick (av[0]) && !i_am_backup ())
    {
	send_cmd (any_service (), "SVSMODE %s -Rra", user->nick);
	strscpy(user->mode, changemode("-Rra", user->mode), sizeof(user->mode));
    }
#endif
	if (!is_services_nick (av[0]) && !i_am_backup ())
	    if (validate_user (user))
		if (services_level == 1 && memos_on==TRUE)
		    check_memos (user->nick);
}

/*************************************************************************/

/* Handle a JOIN command.
 *    av[0] = channels to join
 */

void
do_join (const char *source, int ac, char **av)
{
    User *user;
    char *s, *t;
    struct u_chanlist *c;

    user = finduser (source);
    if (!user)
    {
	write_log ("user: JOIN from nonexistent user %s: %s", source,
		   merge_args (ac, av));
	return;
    }
    t = av[0];
    while (*(s = t))
    {
	t = s + strcspn (s, ",");
	if (*t)
	    *t++ = 0;
	if(chanserv_on==TRUE)
	{
	    if (check_kick (user, s))
		continue;
	}
	if (runflags & RUN_DEBUG)
	    write_log ("debug: %s joins %s", source, s);
	chan_adduser (user, s);
	c = smalloc (sizeof (*c));
	c->next = user->chans;
	c->prev = NULL;
	if (user->chans)
	    user->chans->prev = c;
	user->chans = c;
	c->chan = findchan (s);
    }
    if(news_on==TRUE)
    {
	if (services_level == 1)
	    check_newss (av[0], source);
    }
}

/*************************************************************************/

/* Handle a PART command.
 *    av[0] = channels to leave
 */

void
do_part (const char *source, int ac, char **av)
{
    User *user;
    char *s, *t;
    struct u_chanlist *c;

    user = finduser (source);
    if (!user)
    {
	write_log ("user: PART from nonexistent user %s: %s", source,
		   merge_args (ac, av));
	return;
    }
    t = av[0];
    while (*(s = t))
    {
	t = s + strcspn (s, ",");
	if (*t)
	    *t++ = 0;
	if (runflags & RUN_DEBUG)
	    write_log ("debug: %s leaves %s", source, s);
	for (c = user->chans; c && stricmp (s, c->chan->name) != 0; c = c->next)
	    ;
	if (c)
	{
	    chan_deluser (user, c->chan);
	    if (c->next)
		c->next->prev = c->prev;
	    if (c->prev)
		c->prev->next = c->next;
	    else
		user->chans = c->next;
	    free (c);
	}
    }
}

/*************************************************************************/

/* Handle a KICK command.
 *    av[0] = channel
 *      av[1] = nick(s) being kicked
 *      av[2] = reason
 */

void
do_kick (const char *source, int ac, char **av)
{
    User *user;
    char *s, *t;
    struct u_chanlist *c;

    if(chanserv_on==TRUE)
    {
	if (stricmp (s_ChanServ, av[1]) == 0 && !i_am_backup ())
	    do_cs_join (av[0]);
    }
    if (is_services_nick (av[1]))
	return;
    t = av[1];
    while (*(s = t))
    {
	t = s + strcspn (s, ",");
	if (*t)
	    *t++ = 0;
	user = finduser (s);
	if (!user)
	{
	    write_log ("user: KICK for nonexistent user %s on %s: %s", s, av[0],
		       merge_args (ac - 2, av + 2));
	    continue;
	}
	if (runflags & RUN_DEBUG)
	    write_log ("debug: kicking %s from %s", s, av[0]);
	for (c = user->chans; c && stricmp (av[0], c->chan->name) != 0;
	     c = c->next)
	    ;
	if (c)
	{
	    chan_deluser (user, c->chan);
	    if (c->next)
		c->next->prev = c->prev;
	    if (c->prev)
		c->prev->next = c->next;
	    else
		user->chans = c->next;
	    free (c);
	}
	if(chanserv_on==TRUE)
	{
	    if (!i_am_backup ())
		do_cs_revenge (av[0], source, s, CR_KICK);
	}
    }
    if (!source)
    {
	write_log ("user: KICK by nonexistent user %s on %s: %s", source, av[0],
		   merge_args (ac - 2, av + 2));
	return;
    }
}

/*************************************************************************/

/* Handle a MODE command for a user.
 *    av[0] = nick to change mode for
 *      av[1] = modes
 */

void
do_umode (const char *source, int ac, char **av)
{
    if (stricmp (source, av[0]) != 0 && !is_services_nick (source))
    {
	write_log ("user: MODE %s %s from different nick %s!", av[0], av[1], source);
	wallops (NULL, "%s attempted to change mode %s for %s", source, av[1], av[0]);
	return;
    }
    do_svumode (source, ac, av);
}

void
do_svumode (const char *source, int ac, char **av)
{
    User *user;
    int add = 1;
    char *s, *modestr = av[1];

    /* Moved checking of who changed it to umode, this is now svumode */

    if (is_services_nick (av[0])) return;
    user = finduser (av[0]);
    if (!user)
    {
	write_log ("user: MODE %s for nonexistent nick %s: %s", av[1], av[0],
		   merge_args (ac, av));
	return;
    }
    if (runflags & RUN_DEBUG)
	write_log ("debug: Changing mode for %s to %s", av[0], av[1]);
    s = modestr;
    while (*s)
    {
	switch (*s++)	/* Special action for certain modes. */
	{
	    case '+': add = 1; break;
	    case '-': add = 0; break;
	    case 'o':
		if (add)
		{
		    ++opcnt;
		    if(globalnoticer_on==TRUE)
		    {
			/* Send global message to user when they oper up */
			if (!is_services_nick (av[0]) && !i_am_backup ())
			{
			    int i;
			    for (i = 0; i < nmessage; ++i)
				if (messages[i].type == M_OPER)
				    notice (s_GlobalNoticer, source, "\37[\37\2OPER\2\37]\37 %s", messages[i].text);
			}
		    }
		    if(nickserv_on==TRUE)
		    {
			NickInfo *hni;
			if ((hni = host(findnick (source))) && (hni->flags & NI_IDENTIFIED)
			    && !(hni->flags & NI_IRCOP) && !i_am_backup ())
			{
			    notice (s_NickServ, source, "You have not set the \2IRC Operator\2 flag for your nick.");
			    notice (s_NickServ, source, "Please set this with \2/msg %s SET IRCOP ON\2.", s_NickServ);
			}
		    }
		    if(clones_on==TRUE)
			clones_del (user->host);
		}
		else
		{
		    if(clones_on==TRUE)
			clones_add (user->nick, user->host);
		    --opcnt;
		}
		break;
	}
    }
    strscpy(user->mode, changemode(av[1], user->mode), sizeof(user->mode));

#ifdef DAL_SERV
    if (!i_am_backup()) {
    if (is_services_op (av[0]) && !hasmode("a", user->mode))
    {
	strscpy(user->mode, changemode("a", user->mode), sizeof(user->mode));
	send_cmd (any_service (), "SVSMODE %s +a", av[0]);
    }
    else if (!is_services_op (av[0]) && hasmode("a", user->mode))
    {
	strscpy(user->mode, changemode("-a", user->mode), sizeof(user->mode));
	send_cmd (any_service (), "SVSMODE %s -a", av[0]);
    }
    }
#endif
}

/*************************************************************************/

/* Handle a QUIT command.
 *    av[0] = reason
 */

void
do_quit (const char *source, int ac, char **av)
{
    User *user;

    user = finduser (source);
    if (!user)
    {
	/* Reportedly Undernet IRC servers will sometimes send duplicate
	 * QUIT messages for quitting users, so suppress the log warning. */
#ifndef IRC_UNDERNET
	write_log ("user: QUIT from nonexistent user %s: %s", source,
		   merge_args (ac, av));
#endif
	return;
    }
    if (runflags & RUN_DEBUG)
	write_log ("debug: %s quits", source);
    delete_user (user);
}

/*************************************************************************/

/* Handle a KILL command.
 *    av[0] = nick being killed
 *      av[1] = reason
 */

void
do_kill (const char *source, int ac, char **av)
{
    User *user;

    user = finduser (av[0]);
    if (!user)
	return;
    if (runflags & RUN_DEBUG)
	write_log ("debug: %s killed", av[0]);
    delete_user (user);
}

/*************************************************************************/
/*************************************************************************/

/* Is the given nick an oper? */

int
is_oper (const char *nick)
{
    User *user = finduser (nick);
    return (user && hasmode("o", user->mode));
}

/*************************************************************************/

/* Is the given nick a Services Admin? */

int
is_services_admin (const char *nick)
{
    NickInfo *ni;
    char tmp[NICKMAX + 2];
    char tmp2[BUFSIZE + 2] = " ";

    strcat (tmp2, services_admin);
    strcat (tmp2, " ");
    strscpy (tmp + 1, nick, NICKMAX);
    tmp[0] = ' ';
    tmp[strlen (tmp) + 1] = 0;
    tmp[strlen (tmp)] = ' ';
    if (stristr (tmp2, tmp) == NULL)
	return 0;
    if(nickserv_on==TRUE)
    {
	if ((ni = findnick (nick)) && (ni->flags & NI_IDENTIFIED) && is_oper (nick))
	{
	    return 1;
	}
	else return 0;
    }
    else return 1;
}

int
is_justservices_admin (const char *nick)
{
    char tmp[NICKMAX + 2];
    char tmp2[BUFSIZE + 2] = " ";

    strcat (tmp2, services_admin);
    strcat (tmp2, " ");
    strscpy (tmp + 1, nick, NICKMAX);
    tmp[0] = ' ';
    tmp[strlen (tmp) + 1] = 0;
    tmp[strlen (tmp)] = ' ';
    if (stristr (tmp2, tmp) == NULL)
	return 0;
    return 1;
}

/*************************************************************************/

/* Is the given nick on the given channel? */

int
is_on_chan (const char *nick, const char *chan)
{
    User *u = finduser (nick);
    struct u_chanlist *c;

    if (!u)
	return 0;
    for (c = u->chans; c; c = c->next)
	if (stricmp (c->chan->name, chan) == 0)
	    return 1;
    return 0;
}

/*************************************************************************/

/* Is the given nick a channel operator on the given channel? */

int
is_chanop (const char *nick, const char *chan)
{
    Channel *c = findchan (chan);
    struct c_userlist *u;

    if (!c)
	return 0;
    for (u = c->chanops; u; u = u->next)
	if (stricmp (u->user->nick, nick) == 0)
	    return 1;
    return 0;
}

/*************************************************************************/

/* Is the given nick voiced (channel mode +v) on the given channel? */

int
is_voiced (const char *nick, const char *chan)
{
    Channel *c = findchan (chan);
    struct c_userlist *u;

    if (!c)
	return 0;
    for (u = c->voices; u; u = u->next)
	if (stricmp (u->user->nick, nick) == 0)
	    return 1;
    return 0;
}

/*************************************************************************/
/*************************************************************************/

/* Kill a user and do all the nicities. */
void
kill_user (const char *who, const char *nick, const char *reason)
{
    char *av[2];
    av[0] = sstrdup (nick);
    av[1] = sstrdup (reason);
    if (!i_am_backup ())
    {
	send_cmd (who, "KILL %s :%s", nick, reason);
	do_kill (who, 2, av);
    }
    free (av[1]);
    free (av[0]);
}


/* Does the user's usermask match the given mask (either nick!user@host or
 * just user@host)?
 */

int
match_usermask (const char *mask, User * user)
{
    char *mask2 = sstrdup (mask);
    char *nick, *username, *host;
    int result;

    if (strchr (mask2, '!'))
    {
	nick = strtok (mask2, "!");
	username = strtok (NULL, "@");
    }
    else
    {
	nick = NULL;
	username = strtok (mask2, "@");
    }
    host = strlower (strtok (NULL, ""));
    if (!username || !host) {
	free(mask2);
	return 0;
    }
    result = (nick ? match_wild_nocase (nick, user->nick) : 1) &&
	match_wild_nocase (username, user->username) &&
	match_wild_nocase (host, user->host);
    free(mask2);
    return result;
}

/*************************************************************************/

/* Split a usermask up into its constitutent parts.  Returned strings are
 * malloc()'d, and should be free()'d when done with.  Returns "*" for
 * missing parts.
 */

void
split_usermask (const char *mask, char **nick, char **user, char **host)
{
    char *mask2 = sstrdup (mask);

    *nick = strtok (mask2, "!");
    *user = strtok (NULL, "@");
    *host = strtok (NULL, "");
    /* Handle special case: mask == user@host */
    if (*nick && !*user && strchr (*nick, '@'))
    {
	*nick = NULL;
	*user = strtok (mask2, "@");
	*host = strtok (NULL, "");
    }
    if (!*nick)
	*nick = "*";
    if (!*user)
	*user = "*";
    if (!*host)
	*host = "*";
    *nick = sstrdup (*nick);
    *user = sstrdup (*user);
    *host = sstrdup (*host);
    free (mask2);
}

/*************************************************************************/

/* Given a user, return a mask that will most likely match any address the
 * user will have from that location.  For IP addresses, wildcards the
 * appropriate subnet mask (e.g. 35.1.1.1 -> 35.*; 128.2.1.1 -> 128.2.*);
 * for named addresses, wildcards the leftmost part of the name unless the
 * name only contains two parts.  If the username begins with a ~, delete
 * it.  The returned character string is malloc'd and should be free'd
 * when done with.
 */

char *
create_mask (User * u)
{
    char *mask, *s, *end;

    end = mask = smalloc (strlen (u->username) + strlen (u->host) + 2);
    end += sprintf (end, "*%s@", u->username);
    if (strspn (u->host, "0123456789.") == strlen (u->host))
    {				/* IP addr */
	s = sstrdup (u->host);
	*strrchr (s, '.') = 0;
	if (atoi (u->host) < 192)
	    *strrchr (s, '.') = 0;
	if (atoi (u->host) < 128)
	    *strrchr (s, '.') = 0;
	sprintf (end, "%s.*", s);
	free (s);
    }
    else
    {
	if ((s = strchr (u->host, '.')) && strchr (s + 1, '.'))
	{
	    s = sstrdup (strchr (u->host, '.') - 1);
	    *s = '*';
	}
	else
	    s = sstrdup (u->host);
	strcpy (end, s);
	free (s);
    }
    return mask;
}

/*************************************************************************/
