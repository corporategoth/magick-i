/* Channel-handling routines.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

Channel *chanlist = NULL;

/* Valid channel prefixes -- GLOBAL CHANNELS ONLY ... */
const char chanprefix[] = { '#', '+', '&', 0 };

/*************************************************************************/

/* Return statistics.  Pointers are assumed to be valid. */

void
get_channel_stats (long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    Channel *chan;
    struct c_userlist *cu;
    int i;

    for (chan = chanlist; chan; chan = chan->next)
    {
	count++;
	mem += sizeof (*chan);
	if (chan->topic)
	    mem += strlen (chan->topic) + 1;
	if (chan->key)
	    mem += strlen (chan->key) + 1;
	mem += sizeof (char *) * chan->bansize;
	for (i = 0; i < chan->bancount; i++)
	    if (chan->bans[i])
		mem += strlen (chan->bans[i]) + 1;
	for (cu = chan->users; cu; cu = cu->next)
	    mem += sizeof (*cu);
	for (cu = chan->chanops; cu; cu = cu->next)
	    mem += sizeof (*cu);
	for (cu = chan->voices; cu; cu = cu->next)
	    mem += sizeof (*cu);
    }
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/

/* Send the current list of channels to the named user. */

void
send_channel_list (const char *who, const char *user, const char *x)
{
    Channel *c;
    char s[16], buf[BUFSIZE], *end;
    struct c_userlist *u, *u2;
    int isop, isvoice;

    for (c = chanlist; c; c = c->next)
	if (!x || match_wild_nocase (x, c->name))
	{
	    snprintf (s, sizeof (s), " %d", c->limit);
	    notice (who, user, "%s %lu +%s%s%s%s%s %s", c->name,
		    c->creation_time, c->mode,
		    (c->limit) ? " " : "",
		    (c->limit) ? s : "",
		    (c->key) ? " " : "",
		    (c->key) ? c->key : "",
		    c->topic ? c->topic : "");
	    end = buf;
	    end += snprintf (end, sizeof (buf), "%s", c->name);
	    for (u = c->users; u; u = u->next)
	    {
		isop = isvoice = 0;
		for (u2 = c->chanops; u2; u2 = u2->next)
		    if (u2->user == u->user)
		    {
			isop = 1;
			break;
		    }
		for (u2 = c->voices; u2; u2 = u2->next)
		    if (u2->user == u->user)
		    {
			isvoice = 1;
			break;
		    }
		end += snprintf (end, sizeof (buf), " %s%s%s", isvoice ? "+" : "",
				 isop ? "@" : "", u->user->nick);
	    }
	    notice (who, user, buf);
	}
}

/* Send the current list of channels to the named user. */

void
send_chanmode_list (const char *who, const char *user, const char *x)
{
    Channel *c;
    char s[16], buf[BUFSIZE], *end;
    struct c_userlist *u, *u2;
    int isop, isvoice;

    for (c = chanlist; c; c = c->next)
	if (!x || hasmode(x, c->mode))
	{
	    snprintf (s, sizeof (s), " %d", c->limit);
	    notice (who, user, "%s %lu +%s%s%s%s%s %s", c->name,
		    c->creation_time, c->mode,
		    (c->limit) ? " " : "",
		    (c->limit) ? s : "",
		    (c->key) ? " " : "",
		    (c->key) ? c->key : "",
		    c->topic ? c->topic : "");
	    end = buf;
	    end += snprintf (end, sizeof (buf), "%s", c->name);
	    for (u = c->users; u; u = u->next)
	    {
		isop = isvoice = 0;
		for (u2 = c->chanops; u2; u2 = u2->next)
		    if (u2->user == u->user)
		    {
			isop = 1;
			break;
		    }
		for (u2 = c->voices; u2; u2 = u2->next)
		    if (u2->user == u->user)
		    {
			isvoice = 1;
			break;
		    }
		end += snprintf (end, sizeof (buf), " %s%s%s", isvoice ? "+" : "",
				 isop ? "@" : "", u->user->nick);
	    }
	    notice (who, user, buf);
	}
}

/* Send list of users on a single channel. */
void
send_channel_users (const char *who, const char *user, const char *chan)
{
    Channel *c = findchan (chan);
    struct c_userlist *u;
    char buf[BUFSIZE] = "";

    if (!c)
    {
	notice (who, user, CS_NOT_IN_USE,
		chan ? chan : "(null)");
	return;
    }
    notice (who, user, "Channel %s users:", chan);
    for (u = c->users; u; u = u->next)
	if (strlen (buf) + strlen (u->user->nick) >= 512)
	{
	    notice (who, user, "%s", buf);
	    strscpy (buf, "", BUFSIZE);
	}
	else
	    snprintf (buf, sizeof (buf), "%s %s%s", buf, is_chanop (u->user->nick, chan) ? "@" :
		      is_voiced (u->user->nick, chan) ? "+" : "",
		      u->user->nick);
    if (buf)
	notice (who, user, "%s", buf);
}

/*************************************************************************/

/* Return the Channel structure corresponding to the named channel, or NULL
 * if the channel was not found. */

Channel *
findchan (const char *chan)
{
    Channel *c = chanlist;

    for (c = chanlist; c; c = c->next)
	if (stricmp (c->name, chan) == 0)
	    return c;
    return NULL;
}

/*************************************************************************/

/* Add/remove a user to/from a channel, creating or deleting the channel as
 * necessary. */

void
chan_adduser (User * user, const char *chan)
{
    Channel *c = findchan (chan);
    int newchan = !c;
    struct c_userlist *u;

    if (newchan)
    {
	/* Allocate pre-cleared memory */
	c = scalloc (sizeof (Channel), 1);
	c->next = chanlist;
	if (chanlist)
	    chanlist->prev = c;
	chanlist = c;
	strscpy (c->name, chan, sizeof (c->name));
	c->creation_time = time (NULL);
	if(chanserv_on==TRUE)
	{
	    if (!i_am_backup ())
	    {
		do_cs_unprotect (chan);
		do_cs_join (chan);
		check_modes (chan);
		restore_topic (chan);
	    }
	}
    }
    if(chanserv_on==TRUE)
	if (!i_am_backup ())
	    check_should_op (user, chan);
    u = smalloc (sizeof (struct c_userlist));
    u->next = c->users;
    u->prev = NULL;
    if (c->users)
	c->users->prev = u;
    c->users = u;
    u->user = user;
}

void
chan_deluser (User * user, Channel * c)
{
    struct c_userlist *u;

    for (u = c->users; u && u->user != user; u = u->next)
	;
    if (!u)
	return;
    if (u->next)
	u->next->prev = u->prev;
    if (u->prev)
	u->prev->next = u->next;
    else
	c->users = u->next;
    free (u);
    for (u = c->chanops; u && u->user != user; u = u->next)
	;
    if (u)
    {
	if (u->next)
	    u->next->prev = u->prev;
	if (u->prev)
	    u->prev->next = u->next;
	else
	    c->chanops = u->next;
	free (u);
    }
    for (u = c->voices; u && u->user != user; u = u->next)
	;
    if (u)
    {
	if (u->next)
	    u->next->prev = u->prev;
	if (u->prev)
	    u->prev->next = u->next;
	else
	    c->voices = u->next;
	free (u);
    }
    if (!c->users)
    {
	if(chanserv_on==TRUE)
	{
	    if (!i_am_backup ())
	    {
		do_cs_part (c->name);
		do_cs_protect (c->name);
	    }
	}
	if (c->topic)
	    free (c->topic);
	if (c->key)
	    free (c->key);
	if (c->bancount)
	{
	    int i;
	    for (i = c->bancount - 1; i >= 0; --i)
		free (c->bans[i]);
	}
	if (c->bansize)
	    free (c->bans);
	if (c->chanops || c->voices)
	    write_log ("channel: Memory leak freeing %s: %s%s%s %s non-NULL!",
		       c->name,
		       c->chanops ? "c->chanops" : "",
		       c->chanops && c->voices ? " and " : "",
		       c->voices ? "c->voices" : "",
		       c->chanops && c->voices ? "are" : "is");
	if (c->next)
	    c->next->prev = c->prev;
	if (c->prev)
	    c->prev->next = c->next;
	else
	    chanlist = c->next;
	free (c);
    }
}

/*************************************************************************/

/* Handle a channel MODE command. */

void
do_cmode (const char *source, int ac, char **av)
{
    Channel *chan;
    struct c_userlist *u;
    User *user;
    char *s, *nick;
    int add = 1;		/* 1 if adding modes, 0 if deleting */
    char *modestr = av[1];

    chan = findchan (av[0]);
    if (!chan)
    {
	write_log ("channel: MODE %s for nonexistent channel %s",
		   merge_args (ac - 1, av + 1), av[0]);
	return;
    }
    s = modestr;
    ac -= 2;
    av += 2;

    while (*s)
    {

	switch (*s++)	/* Special action for certain modes */
	{
	case '+':
	    add = 1;
	    break;
	case '-':
	    add = 0;
	    break;
	case 'k':
	    if (--ac < 0)
	    {
		write_log ("channel: MODE %s %s: missing parameter for %ck",
			   chan->name, modestr, add ? '+' : '-');
		break;
	    }
	    if (chan->key)
	    {
		free (chan->key);
		chan->key = NULL;
	    }
	    if (add)
		chan->key = sstrdup (*av++);
	    break;

	case 'l':
	    if (add)
	    {
		if (--ac < 0)
		{
		    write_log ("channel: MODE %s %s: missing parameter for +l",
			       chan->name, modestr);
		    break;
		}
		chan->limit = atoi (*av++);
	    }
	    else
		chan->limit = 0;
	    break;

	case 'b':
	    if (--ac < 0)
	    {
		write_log ("channel: MODE %s %s: missing parameter for %cb",
			   chan->name, modestr, add ? '+' : '-');
		break;
	    }
	    if (add)
	    {
		ChannelInfo *ci;
		if (chan->bancount >= chan->bansize)
		{
		    chan->bansize += 8;
		    chan->bans = srealloc (chan->bans,
					   sizeof (char *) * chan->bansize);
		}
		chan->bans[chan->bancount++] = sstrdup (*av++);
		if(chanserv_on==TRUE)
		{
		    if ((ci = cs_findchan (chan->name)) && ((user = finduser (source))
			|| is_server (source)) && !is_services_nick (source))
		    {
			int setlev = 0, banlev = -1, i;
			char banee[NICKMAX];

			/* Find out the level of the BANNER (setlev) */
			if (is_server (source))
			    setlev = ci->cmd_access[CA_UNBAN] - 1;
			else
			    setlev = get_access (user, ci);

			/* Find out the level of the BANNEE (banlev) */
			if ((i = countusermask (chan->bans[chan->bancount - 1])) > 0)
			    while (i)
			    {
				user = findusermask (chan->bans[chan->bancount - 1], i);
				if (get_access (user, ci) > banlev)
				{
				    banlev = get_access (user, ci);
				    strscpy (banee, user->nick, NICKMAX);
				}
				i--;
			    }

			/* Unban if the BANEE is HIGHER or EQUAL TO the BANER */
			if (runflags & RUN_DEBUG)
			    write_log ("debug: actionee level is %d (%s), actioner is %d (%s)",
				    banlev, banee, setlev, source);

			if (banlev >= setlev)
			{
			    char *mask;
			    mask = smalloc (strlen (chan->bans[chan->bancount - 1]) + 1);
			    strscpy (mask, chan->bans[chan->bancount - 1], strlen (chan->bans[chan->bancount - 1]) + 1);
			    if (!i_am_backup ())
			    {
				if (get_revenge_level (ci) >= CR_REVERSE)
				    change_cmode (s_ChanServ, chan->name, "-b", mask);
				do_revenge (chan->name, source, banee, get_bantype (mask));
			    }
			    free (mask);
			}
		    }
		}
	    }
	    else
	    {
		char **s = chan->bans;
		int i = 0;
		for (i = chan->bancount-1; i >= 0 && stricmp(chan->bans[i], *av)!=0; i--) ;
		if (i >= 0)
		{
		    /* free(chan->bans[i]); */
		    --chan->bancount;
		    if (i < chan->bancount)
			bcopy (s + 1, s, sizeof (char *) * (chan->bancount - i));
		}
		++av;
	    }
	    break;

	case 'o':
	    if (--ac < 0)
	    {
		write_log ("channel: MODE %s %s: missing parameter for %co",
			   chan->name, modestr, add ? '+' : '-');
		break;
	    }
	    nick = *av++;
	    if (add)
	    {
		if (is_services_nick (nick))
		    break;
		for (u = chan->chanops; u && stricmp (u->user->nick, nick) != 0;
		     u = u->next)
		    ;
		if (u)
		    break;
		user = finduser (nick);
		if (!user)
		{
		    write_log ("channel: MODE %s +o for nonexistent user %s",
			       chan->name, nick);
		    break;
		}
		if (runflags & RUN_DEBUG)
		    write_log ("debug: Setting +o on %s for %s", chan->name, nick);
		u = smalloc (sizeof (*u));
		u->next = chan->chanops;
		u->prev = NULL;
		if (chan->chanops)
		    chan->chanops->prev = u;
		chan->chanops = u;
		u->user = user;
		if(chanserv_on==TRUE)
		    if (!i_am_backup ())
			check_valid_op (user, chan->name, !!strchr (source, '.'));
	    }
	    else
	    {
		if(chanserv_on==TRUE)
		    if (stricmp (s_ChanServ, nick) == 0 && !i_am_backup ())
			do_cs_reop (chan->name);
		if (is_services_nick (nick))
		    break;
		for (u = chan->chanops; u && stricmp (u->user->nick, nick) != 0;
		     u = u->next)
		    ;
		if (!u)
		    break;
		if (u->next)
		    u->next->prev = u->prev;
		if (u->prev)
		    u->prev->next = u->next;
		else
		    chan->chanops = u->next;
		free (u);
		if(chanserv_on==TRUE)
		{
		    if (!i_am_backup ())
			if (is_server (source))
			{
			    ChannelInfo *ci = cs_findchan (chan->name);
			    if (ci && get_access (finduser (nick), ci) >
				ci->cmd_access[CA_AUTOOP] - 1)
				change_cmode (s_ChanServ, chan->name, "+o", nick);
			}
			else if (do_cs_revenge (chan->name, source, nick, CR_DEOP))
			    if (get_revenge_level (cs_findchan (chan->name)) >= CR_REVERSE)
				change_cmode (s_ChanServ, chan->name, "+o", nick);
		}
	    }
	    break;

	case 'v':
	    if (--ac < 0)
	    {
		write_log ("channel: MODE %s %s: missing parameter for %cv",
			   chan->name, modestr, add ? '+' : '-');
		break;
	    }
	    nick = *av++;
	    if (is_services_nick (nick))
		break;
	    if (add)
	    {
		for (u = chan->voices; u && stricmp (u->user->nick, nick) != 0;
		     u = u->next)
		    ;
		if (u)
		    break;
		user = finduser (nick);
		if (!user)
		{
		    write_log ("channel: MODE %s +v for nonexistent user %s",
			       chan->name, nick);
		    break;
		}
		if (runflags & RUN_DEBUG)
		    write_log ("debug: Setting +v on %s for %s", chan->name, nick);
		u = smalloc (sizeof (*u));
		u->next = chan->voices;
		u->prev = NULL;
		if (chan->voices)
		    chan->voices->prev = u;
		chan->voices = u;
		u->user = user;
		if(chanserv_on==TRUE)
		    if (!i_am_backup ())
			check_valid_voice (user, chan->name, !!strchr (source, '.'));
	    }
	    else
	    {
		for (u = chan->voices; u && stricmp (u->user->nick, nick) != 0;
		     u = u->next)
		    ;
		if (!u)
		    break;
		if (u->next)
		    u->next->prev = u->prev;
		if (u->prev)
		    u->prev->next = u->next;
		else
		    chan->voices = u->next;
		free (u);
	    }
	    break;

	}			/* switch */

    }				/* while (*s) */

    strscpy(chan->mode, changemode(modestr, chan->mode), sizeof(chan->mode));
    strscpy(chan->mode, changemode("-bvo", chan->mode), sizeof(chan->mode));
    if (!chan->key && hasmode("k", chan->mode))
	strscpy(chan->mode, changemode("-k", chan->mode), sizeof(chan->mode));
    if (!chan->limit && hasmode("l", chan->mode))
	strscpy(chan->mode, changemode("-l", chan->mode), sizeof(chan->mode));

    /* Check modes against ChanServ mode lock */
    if(chanserv_on==TRUE && !is_services_nick(source))
	check_modes (chan->name);
}

int
validchan (char *chan)
{
    int i;

    for (i=0; chanprefix[i]; i++) {
	if (chanprefix[i] == chan[0] && chan[1] > 32)
	    return 1;
    }
    return 0;
}

int
get_bantype (const char *mask)
{
    int i;

    for (i = 0; mask[i] != '!' && i < strlen (mask); i++)
	if (!(mask[i] == '*' || mask[i] == '?'))
	    return CR_NICKBAN;
    i++;
    for (; mask[i] != '@' && i < strlen (mask); i++)
	if (!(mask[i] == '*' || mask[i] == '?'))
	    return CR_USERBAN;
    i++;
    for (; i < strlen (mask); i++)
	if (!(mask[i] == '*' || mask[i] == '?' || mask[i] == '.'))
	    return CR_HOSTBAN;
    return CR_NONE;
}

void
change_cmode (const char *who, const char *chan, const char *mode, const char *pram)
{
    char *av[3];
    av[0] = sstrdup (chan);
    av[1] = sstrdup (mode);
    av[2] = sstrdup (pram);
    if (!i_am_backup ())
    {
	send_cmd (who, "MODE %s %s %s", chan, mode, pram);
	do_cmode (who, 3, av);
    }
    free (av[2]);
    free (av[1]);
    free (av[0]);
}

void
kick_user (const char *who, const char *chan, const char *nick, const char *reason)
{
    char *av[3];
    av[0] = sstrdup (chan);
    av[1] = sstrdup (nick);
    av[2] = sstrdup (reason);
    if (!i_am_backup ())
    {
	send_cmd (who, "KICK %s %s :%s", chan, nick, reason);
	do_kick (who, 3, av);
    }
    free (av[2]);
    free (av[1]);
    free (av[0]);
}

/*************************************************************************/

/* Handle a TOPIC command. */

void
do_topic (const char *source, int ac, char **av)
{
    Channel *c = findchan (av[0]);

    if (!c)
    {
	write_log ("channel: TOPIC %s for nonexistent channel %s",
		   merge_args (ac - 1, av + 1), av[0]);
	return;
    }
    if(chanserv_on==TRUE)
	if (check_topiclock (av[0]))
	    return;
    strscpy (c->topic_setter, av[1], sizeof (c->topic_setter));
    c->topic_time = atol (av[2]);
    if (c->topic)
    {
	free (c->topic);
	c->topic = NULL;
    }
    if (ac > 3 && *av[3])
	c->topic = sstrdup (av[3]);
    if(chanserv_on==TRUE)
	record_topic (av[0]);
}

/*************************************************************************/
