/* NickServ functions.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
NickInfo *nicklists[256];	/* One for each initial character */

char s_NickServ[NICKMAX];
#include "ns-help.c"
Timeout *timeouts = NULL;
char nickserv_db[512];
int nick_expire = 21;
int release_timeout = 60;
int wait_collide = 0;
int passfail_max = 5;
int nickhold = 0;
int killlist_size = 0;

struct KillList {
	char nick[NICKMAX];
	time_t time;
} *killlist;

/* Stats */
int S_nick_reg = 0, S_nick_drop = 0, S_nick_ghost = 0, S_nick_kill = 0;
int S_nick_ident = 0, S_nick_link = 0, S_nick_recover = 0, S_nick_release = 0;
int S_nick_getpass = 0, S_nick_forbid = 0, S_nick_suspend = 0, S_nick_unsuspend = 0;

static int is_on_access (User * u, NickInfo * ni);
static void alpha_insert_nick (NickInfo * ni);
static NickInfo *makenick (const char *nick);
static void collide (NickInfo * ni);
static void release (NickInfo * ni);
static void add_timeout (NickInfo * ni, int type, time_t delay);
static void del_timeout (NickInfo * ni, int type);
static void do_help (const char *source);
static void do_register (const char *source);
static void do_link (const char *source);
static void do_identify (const char *source);
static void do_slaves (const char *source);
static void do_drop (const char *source);
static void do_slavedrop (const char *source);
static void do_set (const char *source);
static void do_set_password (NickInfo * ni, char *param);
static void do_set_email (NickInfo * ni, char *param);
static void do_set_url (NickInfo * ni, char *param);
static void do_set_kill (NickInfo * ni, char *param);
static void do_set_private (NickInfo * ni, char *param);
static void do_set_secure (NickInfo * ni, char *param);
static void do_set_ircop (NickInfo * ni, char *param);
static void do_set_privmsg (NickInfo * ni, char *param);
static void do_access (const char *source);
static int do_access_help (const char *source, char *cmd, char *nick);
static void do_access_add (const char *source);
static void do_access_current (const char *source);
static void do_access_del (const char *source);
static void do_access_list (const char *source);
static void do_ignore (const char *source);
static int do_ignore_help (const char *source, char *cmd, char *nick);
static void do_ignore_add (const char *source);
static void do_ignore_del (const char *source);
static void do_ignore_list (const char *source);
static void do_deop (const char *source);
static void do_info (const char *source);
static void do_list (const char *source);
static void do_listoper (const char *source);
static void do_recover (const char *source);
static void do_release (const char *source);
static void do_ghost (const char *source);
static void do_getpass (const char *source);
static void do_forbid (const char *source);
static void do_suspend (const char *source);
static void do_unsuspend (const char *source);
static void do_status (const char *source);

/*************************************************************************/

/* Display total number of registered nicks and info about each; or, if
 * a specific nick is given, display information about that nick (like
 * /msg NickServ INFO <nick>).  If count_only != 0, then only display the
 * number of registered nicks (the nick parameter is ignored).
 */

#define CR	printf ("\n");
void
listnicks (int count_only, const char *nick)
{
    long count = 0;
    NickInfo *ni, *hni;
    int i;

    if (count_only)
    {
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next)
		++count;
	printf (NS_INFO_COUNT, count); CR

    }
    else if (nick)
    {
	char buf[BUFSIZE];
	long flags;

	if (!(ni = findnick (nick)))
	{
	    printf (NS_NOT_REGISTERED, nick); CR
	    return;
	}
	else if (ni->flags & NI_VERBOTEN)
	{
	    printf (NS_CANNOT_REGISTER, ni->nick); CR
	    return;
	}
	flags = getflags (ni);
	hni = host (ni);

	printf (NS_INFO_INTRO, ni->nick, ni->last_realname); CR 
	if (flags & NI_SLAVE) {
	    printf (NS_INFO_HOST, ni->last_usermask); CR
	}
	if (host (ni)->email) {
	    printf (NS_INFO_EMAIL, hni->email); CR
	}
	if (host (ni)->url) {
	    printf (NS_INFO_URL, hni->url); CR
	}
	if (flags & NI_SUSPENDED) {
	    printf (NS_INFO_SUSPENDED, hni->last_usermask); CR
	} else if (!userisnick (hni->nick) && !slaveonline(hni->nick) &&
				strlen(hni->last_usermask)) {
	    printf (NS_INFO_USERMASK, hni->last_usermask); CR
	}
	printf (NS_INFO_REGISTERED, time_ago (ni->time_registered, 1)); CR
	if (slaveonline (hni->nick) || userisnick (hni->nick))
	{
	    char *slaves;
	    NickInfo *sni;
	    int sc = slavecount (ni->nick);
	    slaves = smalloc (((sc + 1) * NICKMAX) + ((sc + 1) * 2));
	    if (stricmp (ni->nick, hni->nick) != 0 && userisnick (hni->nick))
		strcpy (slaves, finduser (hni->nick)->nick);
	    else
		*slaves = 0;
	    for (i = sc; i; i--)
	    {
		sni = slave (hni->nick, i);
		if (userisnick (sni->nick) && stricmp (sni->nick, ni->nick) != 0)
		{
		    if (*slaves)
			strcat (slaves, ", ");
		    strcat (slaves, sni->nick);
		}
	    }
	    if (*slaves) {
		printf (userisnick (ni->nick) ?
			NS_INFO_AONLINE_AS : NS_INFO_ONLINE_AS, slaves); CR
	    }
	    if (slaves)
		free (slaves);
	}
	else
	{
	    if (!userisnick (ni->nick))
	    {
		printf (NS_INFO_LAST_SEEN, time_ago (ni->last_seen, 1)); CR
		if (!slaveonline (hni->nick) && !userisnick (hni->nick) &&
		    (ni->last_seen != hni->last_seen)) {
		    printf (NS_INFO_LAST_ONLINE, time_ago (hni->last_seen, 1)); CR
		}
	    }
	}
	*buf = 0;
	if (flags & NI_SUSPENDED)
	    strcat (buf, NS_FLAG_SUSPENDED);
	else
	{
	    if (flags & NI_KILLPROTECT)
		strcat (buf, NS_FLAG_KILLPROTECT);
	    if (flags & NI_SECURE)
	    {
		if (*buf)
		    strcat (buf, ", ");
		strcat (buf, NS_FLAG_SECURE);
	    }
	    if (flags & NI_PRIVATE)
	    {
		if (*buf)
		    strcat (buf, ", ");
		strcat (buf, NS_FLAG_PRIVATE);
	    }
	    if (flags & NI_IRCOP)
	    {
		load_sop();
		if (*buf)
		    strcat (buf, ", ");
		if (is_justservices_op (nick))
		    strcat (buf, NS_FLAG_SOP);
		else
		    strcat (buf, NS_FLAG_IRCOP);
	    }
	    if (!*buf)
		strscpy (buf, NS_FLAG_NONE, BUFSIZE);
	}
	printf (NS_INFO_OPTIONS, buf); CR
	if (userisnick (ni->nick) && !(flags & NI_SUSPENDED)) {
	    printf (NS_IN_USE, finduser (nick)->nick); CR
	}
	if (show_sync_on==TRUE) {
	    printf (INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0)); CR
	}
    }
    else
    {
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next)
	    {
		printf ("    %-20s  %s\n", ni->nick,
			ni->flags & NI_VERBOTEN ? "Disallowed (FORBID)"
			: getflags (ni) & NI_SUSPENDED ? "Disallowed (SUSPEND)"
			: host(ni)->last_usermask ? host(ni)->last_usermask
			: "");
		++count;
	    }
	printf (NS_INFO_COUNT, count); CR
    }
}

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

void
get_nickserv_stats (long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    NickInfo *ni;
    char **accptr;

    for (i = 0; i < 256; i++)
	for (ni = nicklists[i]; ni; ni = ni->next)
	{
	    count++;
	    mem += sizeof (*ni);
	    if (ni->last_usermask)
		mem += strlen (ni->last_usermask) + 1;
	    if (ni->last_realname)
		mem += strlen (ni->last_realname) + 1;
	    mem += sizeof (char *) * ni->accesscount;
	    for (accptr = ni->access, j = 0; j < ni->accesscount; accptr++, j++)
		if (*accptr)
		    mem += strlen (*accptr) + 1;
	}
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/
/*************************************************************************/

/* Main NickServ routine. */

void
nickserv (const char *source, char *buf)
{
    char *cmd, *s;

    cmd = strtok (buf, " ");

    if (!cmd)
	return;

    else if (stricmp (cmd, "\1PING") == 0)
    {
	if (!(s = strtok (NULL, "")))
	    s = "\1";
	notice (s_NickServ, source, "\1PING %s", s);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"HELP", H_NONE, do_help},
	    {"REG*", H_NONE, do_register},
	    {"*LINK*", H_NONE, do_link},
	    {"ID*", H_NONE, do_identify},
	    {"S*DROP", H_NONE, do_slavedrop},
	    {"DROP*S", H_NONE, do_slavedrop},
	    {"DROP*", H_NONE, do_drop},
	    {"SLAVE*", H_NONE, do_slaves},
	    {"SET*", H_NONE, do_set},
	    {"ACC*", H_NONE, do_access},
	    {"*INFO", H_NONE, do_info},
	    {"LIST", H_NONE, do_list},
	    {"U*LIST", H_NONE, do_list},
	    {"LISTU*", H_NONE, do_list},
	    {"REC*", H_NONE, do_recover},
	    {"REL*", H_NONE, do_release},
	    {"GHOST", H_NONE, do_ghost},
	    {"IGN*", H_NONE, do_ignore},
	    {"LISTO*", H_OPER, do_listoper},
	    {"O*LIST", H_OPER, do_listoper},
	    {"GETPASS", H_SOP, do_getpass},
	    {"FORBID", H_SOP, do_forbid},
	    {"SUSPEND", H_SOP, do_suspend},
	    {"UNSUSPEND", H_SOP, do_unsuspend},
	    {"DEOP*", H_SOP, do_deop},
	    {"STATUS", H_NONE, do_status},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	{
	    if(memos_on==FALSE && command->process==do_ignore)
		notice (s_NickServ, source, ERR_UNKNOWN_COMMAND, cmd, s_NickServ);
	    else
		(*command->process) (source);
	}
	else
	    notice (s_NickServ, source, ERR_UNKNOWN_COMMAND,
		    cmd, s_NickServ);
    }
}

/*************************************************************************/

/* Load/save data files. */

void
load_ns_dbase (void)
{
    FILE *f = fopen (nickserv_db, "r");
    int i, j;
    NickInfo *ni;

    if (!f)
    {
	log_perror ("Can't read NickServ database %s", nickserv_db);
	return;
    }
    switch (i = get_file_version (f, nickserv_db))
    {
    case 5:
    case 4:
	for (i = 33; i < 256; ++i)
	while (fgetc (f) == 1)
	{
	    ni = smalloc (sizeof (NickInfo));
	    if (1 != fread (ni, sizeof (NickInfo), 1, f))
		fatal_perror ("Read error on %s", nickserv_db);
	    ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
	    alpha_insert_nick (ni);
	    if (ni->email) {
		ni->email = read_string (f, nickserv_db);
		if (!strlen(ni->email)) {
		    free(ni->email);
		    ni->email = NULL;
		}
	    } else
		ni->email = NULL;
	    if (ni->url) {
		ni->url = read_string (f, nickserv_db);
		if (!strlen(ni->url)) {
		    free(ni->url);
		    ni->url = NULL;
		}
	    } else
		ni->url = NULL;
	    ni->last_usermask = read_string (f, nickserv_db);
	    ni->last_realname = read_string (f, nickserv_db);
	    if (ni->accesscount)
	    {
		char **access;
		access = smalloc (sizeof (char *) * ni->accesscount);
		ni->access = access;
		for (j = 0; j < ni->accesscount; ++j, ++access)
		    *access = read_string (f, nickserv_db);
	    }
	    if (ni->ignorecount)
	    {
		char **ignore;
		ignore = smalloc (sizeof (char *) * ni->ignorecount);
		ni->ignore = ignore;
		for (j = 0; j < ni->ignorecount; ++j, ++ignore)
		    *ignore = read_string (f, nickserv_db);
	    }
	}
	break;
    case 3:
	{
	NickInfo_V3 *old_ni;
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ni = smalloc (sizeof (NickInfo));
		old_ni = smalloc (sizeof (NickInfo_V3));
		if (1 != fread (old_ni, sizeof (NickInfo_V3), 1, f))
		    fatal_perror ("Read error on %s", nickserv_db);
		strscpy(ni->nick, old_ni->nick, sizeof(ni->nick));
		strscpy(ni->pass, old_ni->pass, sizeof(ni->pass));
		ni->time_registered = old_ni->time_registered;
		ni->last_seen = old_ni->last_seen;
		ni->accesscount = old_ni->accesscount;
		ni->ignorecount = old_ni->ignorecount;
		ni->flags = old_ni->flags;
		ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);

		alpha_insert_nick (ni);
		if (old_ni->email)
		{
		    old_ni->email = read_string (f, nickserv_db);
		    if (strlen(old_ni->email) > 0)
			ni->email = old_ni->email;
		    else
			ni->email = NULL;
		}
		else
		    ni->email = NULL;
		
		if (old_ni->url)
		{
		    old_ni->url = read_string (f, nickserv_db);
		    if (strlen(old_ni->url) > 0)
			ni->url = old_ni->url;
		    else
			ni->url = NULL;
		}
		else
		    ni->url = NULL;
		old_ni->last_usermask = read_string (f, nickserv_db);
		ni->last_usermask = old_ni->last_usermask;
		old_ni->last_realname = read_string (f, nickserv_db);
		ni->last_realname = old_ni->last_realname;
		free(old_ni);

		if (ni->accesscount)
		{
		    char **access;
		    access = smalloc (sizeof (char *) * ni->accesscount);
		    ni->access = access;
		    for (j = 0; j < ni->accesscount; ++j, ++access)
			*access = read_string (f, nickserv_db);
		}
	    }
	break;
	}
    case 2:
    case 1:
	{
	NickInfo_V1 *old_ni;
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ni = smalloc (sizeof (NickInfo));
		old_ni = smalloc (sizeof (NickInfo_V1));
		if (1 != fread (ni, sizeof (NickInfo_V1), 1, f))
		    fatal_perror ("Read error on %s", nickserv_db);
		strscpy(ni->nick, old_ni->nick, sizeof(ni->nick));
		strscpy(ni->pass, old_ni->pass, sizeof(ni->pass));
		ni->time_registered = old_ni->time_registered;
		ni->last_seen = old_ni->last_seen;
		ni->accesscount = old_ni->accesscount;
		ni->flags = old_ni->flags;
		ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);

		alpha_insert_nick (ni);
		old_ni->last_usermask = read_string (f, nickserv_db);
		ni->last_usermask = old_ni->last_usermask;
		old_ni->last_realname = read_string (f, nickserv_db);
		ni->last_realname = old_ni->last_realname;
		free(old_ni);

		if (ni->accesscount)
		{
		    char **access;
		    access = smalloc (sizeof (char *) * ni->accesscount);
		    ni->access = access;
		    for (j = 0; j < ni->accesscount; ++j, ++access)
			*access = read_string (f, nickserv_db);
		}
	    }
	break;
	}
    default:
	fatal ("Unsupported version number (%d) on %s", i, nickserv_db);
    }				/* switch (version) */
    fclose (f);
}

void
save_ns_dbase (void)
{
    FILE *f;
    int i, j;
    NickInfo *ni;
    char **access;
    char **ignore;
    char nickservsave[2048];

    strcpy (nickservsave, nickserv_db);
    strcat (nickservsave, ".save");
    remove (nickservsave);
    if (rename (nickserv_db, nickservsave) < 0)
	log_perror ("Can't back up %s", nickserv_db);
    f = fopen (nickserv_db, "w");
    if (!f)
    {
	log_perror ("Can't write to NickServ database %s", nickserv_db);
	if (rename (nickservsave, nickserv_db) < 0)
	    fatal_perror ("Can't restore backup of %s", nickserv_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't change group of %s to %d", nickserv_db, file_gid);
#endif
    write_file_version (f, nickserv_db);

    for (i = 33; i < 256; ++i)
    {
	for (ni = nicklists[i]; ni; ni = ni->next)
	{
	    fputc (1, f);
	    if (1 != fwrite (ni, sizeof (NickInfo), 1, f))
		fatal_perror ("Write error on %s", nickserv_db);
	    if (ni->email)
		write_string (ni->email, f, nickserv_db);
	    if (ni->url)
		write_string (ni->url, f, nickserv_db);
	    write_string (ni->last_usermask ? ni->last_usermask : "",
			  f, nickserv_db);
	    write_string (ni->last_realname ? ni->last_realname : "",
			  f, nickserv_db);
	    for (access = ni->access, j = 0; j < ni->accesscount; ++access, ++j)
		write_string (*access, f, nickserv_db);
	    for (ignore = ni->ignore, j = 0; j < ni->ignorecount; ++ignore, ++j)
		write_string (*ignore, f, nickserv_db);
	}
	fputc (0, f);
    }
    fclose (f);
    remove (nickservsave);
}

/*************************************************************************/

/* Check whether a user is on the access list of the nick they're using.
 * If not, send warnings as appropriate.  If so (and not NI_SECURE), update
 * last seen info.  Return 1 if the user is valid and recognized, 0
 * otherwise (note that this means an NI_SECURE nick will always return 0
 * from here). */

int
validate_user (User * u)
{
    NickInfo *ni, *hni;
    int on_access;

    if (!(ni = findnick (u->nick)) || !(hni = host (ni)))
	return 0;
    if (ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED))
    {
	if (!(hni->flags & NI_SUSPENDED))
	{
	    if (hni->last_usermask)
		free (hni->last_usermask);
	    hni->last_usermask = smalloc (strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3);
	    snprintf (hni->last_usermask, strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3,
	    			"%s!%s@%s", u->nick, u->username, u->host);
	}
	return 0;		/* Why? Linked nick changing ... */
    }

    if (ni->flags & NI_VERBOTEN)
    {
	if (!i_am_backup()) {
	    notice (s_NickServ, u->nick, ERR_NICK_FORBIDDEN);
	    notice (s_NickServ, u->nick, ERR_WILL_KILL_YOU);
	    add_timeout (ni, TO_COLLIDE, 60);
	}
	return 0;
    }

    on_access = is_on_access (u, ni);

    if (!(hni->flags & NI_SECURE) && on_access)
    {
	ni->flags |= NI_RECOGNIZED;
	ni->last_seen = hni->last_seen = time (NULL);
	if (!(hni->flags & NI_SUSPENDED))
	{
	    if (hni->last_usermask)
		free (hni->last_usermask);
	    hni->last_usermask = smalloc (strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3);
	    snprintf (hni->last_usermask, strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3,
	    			"%s!%s@%s", u->nick, u->username, u->host);
	}
	if (ni->last_realname)
	    free (ni->last_realname);
	ni->last_realname = sstrdup (u->realname);
#ifdef DAL_SERV
	if (!i_am_backup())
	    send_cmd (s_NickServ, "SVSMODE %s +r", u->nick);
	strscpy(u->mode, changemode ("r", u->mode), sizeof(u->mode));
#endif
	return 1;
    }

    if (i_am_backup()) return 1;

    if (hni->flags & NI_SECURE)
    {
	notice (s_NickServ, u->nick, ERR_NICK_SECURE);
	notice (s_NickServ, u->nick, ERR_NICK_IDENTIFY, s_NickServ);
    }
    else
    {
	notice (s_NickServ, u->nick, ERR_NICK_OWNED);
	notice (s_NickServ, u->nick, ERR_NICK_IDENTIFY, s_NickServ);
    }

    if ((hni->flags & NI_KILLPROTECT)
	&& !((hni->flags & NI_SECURE) && on_access))
    {
	notice (s_NickServ, u->nick, ERR_WILL_KILL_YOU);
	add_timeout (ni, TO_COLLIDE, 60);
    }

    return 0;
}

/*************************************************************************/

/* Cancel validation flags for a nick (i.e. when the user with that nick
 * signs off or changes nicks).  Also cancels any impending collide. */

void
cancel_user (User * u)
{
    NickInfo *ni;

    if (ni = findnick (u->nick))
    {
	if (ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED))
	    ni->last_seen = host(ni)->last_seen = time(NULL);
	ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
	del_timeout (ni, TO_COLLIDE);
    }
}

/*************************************************************************/

/* Check the timeout list for any pending actions. */

void
check_timeouts ()
{
    Timeout *to, *to2;
    time_t t = time (NULL);
    User *u;

    to = timeouts;
    while (to)
    {
	if (t < to->timeout)
	{
	    to = to->next;
	    continue;
	}
	to2 = to->next;
	if (to->next)
	    to->next->prev = to->prev;
	if (to->prev)
	    to->prev->next = to->next;
	else
	    timeouts = to->next;
	switch (to->type)
	{
	case TO_COLLIDE:
	    /* If they identified or don't exist anymore, don't kill them. */
	    if (to->ni->flags & NI_IDENTIFIED
		|| !(u = finduser (to->ni->nick))
		|| u->my_signon > to->settime)
		break;
	    /* The RELEASE timeout will always add to the beginning of the
	     * list, so we won't see it.  Which is fine because it can't be
	     * triggered yet anyway. */
	    collide (to->ni);
	    break;
	case TO_RELEASE:
	    release (to->ni);
	    break;
	default:
	    write_log ("%s: Unknown timeout type %d for nick %s", s_NickServ,
		       to->type, to->ni->nick);
	}
	free (to);
	to = to2;
    }
}

/*************************************************************************/

/* Remove all nicks which have expired. */

void
expire_nicks ()
{
    NickInfo *ni, *ni2;
    int i;
    const time_t expire_time = nick_expire * 24 * 60 * 60;
    time_t now = time (NULL);

    for (i = 33; i < 256; ++i)
    {
	ni = nicklists[i];
	while (ni)
	{
	    if (now - ni->last_seen >= expire_time && !slavecount (ni->nick)
		&& !(host(ni)->flags & (NI_VERBOTEN | NI_IRCOP)))
	    {
		ni2 = ni->next;
		write_log ("Expiring nickname %s", ni->nick);
		delnick (ni);
		ni = ni2;
	    }
	    else
		ni = ni->next;
	}
    }
}

/*************************************************************************/

/* Return the NickInfo structure for the given nick, or NULL if the nick
 * isn't registered. */

NickInfo *
findnick (const char *nick)
{
    NickInfo *ni;

    for (ni = nicklists[tolower (*nick)]; ni; ni = ni->next)
	if (stricmp (ni->nick, nick) == 0)
	    return ni;
    return NULL;
}

NickInfo *
host (NickInfo * ni)
{
    if (!ni)
	return NULL;
    if (ni->flags & NI_SLAVE)
	return findnick (ni->last_usermask);
    else
	return ni;
}

NickInfo *
slave (const char *nick, int num)
{
    NickInfo *ni;
    int i, cnt = 0;

    for (i = 33; i < 256; ++i)
	for (ni = nicklists[i]; ni; ni = ni->next)
	{
	    if (stricmp (ni->last_usermask, nick) == 0)
		cnt++;
	    if (cnt == num)
		return ni;
	}
    return NULL;
}

int
issibling (NickInfo * ni, const char *target)
{
    if (ni)
    {
	NickInfo *hni = host (ni);
	int i;
	if (stricmp (ni->nick, target) == 0)
	    return 1;
	if (stricmp (hni->nick, target) == 0)
	    return 1;
	for (i = slavecount (hni->nick); i; i--)
	    if (stricmp (target, slave (hni->nick, i)->nick) == 0)
		return 1;
    }
    return 0;
}

int
slavecount (const char *nick)
{
    NickInfo *ni;
    int i, cnt = 0;

    for (i = 33; i < 256; ++i)
	for (ni = nicklists[i]; ni; ni = ni->next)
	    if (stricmp (ni->last_usermask, nick) == 0)
		cnt++;
    return cnt;
}

int
userisnick (const char *nick)
{
    NickInfo *ni = findnick (nick);
    if (!finduser (nick) || !ni)
	return 0;
    if ((getflags (ni) & NI_SECURE) ? (ni->flags & NI_IDENTIFIED)
	: (ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	return 1;
    return 0;
}

int
slaveonline (const char *nick)
{
    int i = slavecount (nick);

    for (i = slavecount (nick); i; i--)
	if (userisnick (slave (nick, i)->nick))
	    return 1;
    return 0;
}

/* Find flags (assume host's flags if there is one) */
long
getflags (NickInfo * ni)
{
    NickInfo *hni = host (ni);
    long flags;

    if (hni)
    {
	flags = hni->flags;
	flags &= ~NI_RECOGNIZED;
	flags &= ~NI_IDENTIFIED;
	flags &= ~NI_KILL_HELD;
	if (ni->flags & NI_RECOGNIZED)
	    flags |= NI_RECOGNIZED;
	if (ni->flags & NI_IDENTIFIED)
	    flags |= NI_IDENTIFIED;
	if (ni->flags & NI_KILL_HELD)
	    flags |= NI_KILL_HELD;
	if (ni->flags & NI_SLAVE)
	    flags |= NI_SLAVE;
	return flags;
    }
    return 0;
}

/*************************************************************************/
/*********************** NickServ private routines ***********************/
/*************************************************************************/

/* Is the given user's address on the given nick's access list?  Return 1
 * if so, 0 if not. */

static int
is_on_access (User * u, NickInfo * ni)
{
    int i;
    char *buf;
    NickInfo *hni = host (ni);

    if (!ni || !hni) return 0;
    buf = smalloc (strlen (u->username) + strlen (u->host) + 2);
    snprintf (buf, strlen (u->username) + strlen (u->host) + 2, "%s@%s",
							u->username, u->host);
    for (i = 0; i < hni->accesscount; i++)
    {
	if (match_wild_nocase (hni->access[i], buf))
	{
	    free (buf);
	    return 1;
	}
    }
    free (buf);
    return 0;
}

/* Is the given user's nick on the given nick's ignore list?  Return 1
 * if so, 0 if not. */

int
is_on_ignore (const char *source, const char *target)
{
    int i;
    NickInfo *ni, *hni = host (findnick (source));
    char **ignore, mynick[NICKMAX];

    if (hni)
	strscpy (mynick, hni->nick, NICKMAX);
    else
	strscpy (mynick, source, NICKMAX);

    if (ni = host (findnick (target)))
    {
	if (ni->flags & NI_SUSPENDED)
	    return 1;
	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
	    if (stricmp (*ignore, mynick) == 0 || stricmp (*ignore, source) == 0)
		return 1;
    }
    return 0;
}

/*************************************************************************/

/* Insert a nick alphabetically into the database. */

static void
alpha_insert_nick (NickInfo * ni)
{
    NickInfo *ni2, *ni3;
    char *nick = ni->nick;

    for (ni3 = NULL, ni2 = nicklists[tolower (*nick)];
	 ni2 && stricmp (ni2->nick, nick) < 0;
	 ni3 = ni2, ni2 = ni2->next)
	;
    ni->prev = ni3;
    ni->next = ni2;
    if (!ni3)
	nicklists[tolower (*nick)] = ni;
    else
	ni3->next = ni;
    if (ni2)
	ni2->prev = ni;
}

/*************************************************************************/

/* Add a nick to the database.  Returns a pointer to the new NickInfo
 * structure if the nick was successfully registered, NULL otherwise.
 * Assumes nick does not already exist. */

static NickInfo *
makenick (const char *nick)
{
    NickInfo *ni;

    ni = scalloc (sizeof (NickInfo), 1);
    strscpy (ni->nick, nick, NICKMAX);
    ni->accesscount = ni->ignorecount = 0;
    alpha_insert_nick (ni);
    return ni;
}

/*************************************************************************/

/* Remove a nick from the NickServ database.  Return 1 on success, 0
 * otherwise. */

int
delnick (NickInfo * ni)
{
    int i, cnt = 1;
    NickInfo *hni = host (ni);
    ChannelInfo *ci, *ci2;
    ChanAccess *access;
    int j;

/* For slave nicks (they have to be del'd last) */
    for (i = slavecount (ni->nick); i; --i)
	cnt += delnick (slave (ni->nick, 1));

/* Delete the user's MEMOS  */
    if(memos_on==TRUE)
	if (services_level == 1)
	{
	    MemoList *ml;
	    if (ml = find_memolist (ni->nick))
		del_memolist (ml);
	}

/* Delete the user from channel access lists
 * and delete any channels this user owns */
    if(chanserv_on==TRUE)
    {
	for (i = 33; i < 256; ++i)
	{
	    ci = chanlists[i];
	    while (ci)
	    {
		if (stricmp (ci->founder, ni->nick) == 0)
		{
		    if (stricmp (ni->nick, hni->nick) == 0)
		    {
			ci2 = ci->next;
			delchan (ci);
			ci = ci2;
		    }
		    else
		    {
			strscpy (ci->founder, hni->nick, NICKMAX);
			ci = ci->next;
		    }
		}
		else
		{
		    for (access = ci->access, j = 0; access && j < ci->accesscount; access++, j++)
		    {
			if (access->is_nick > 0 && stricmp (access->name, ni->nick) == 0)
			{
			    if (stricmp (ni->nick, hni->nick) == 0)
			    {
				if (access->name)
				    free (access->name);
				access->is_nick = -1;
			    }
			    else
			    {
				if (access->name)
				    free (access->name);
				access->name = sstrdup (hni->nick);
			    }
			}
		    }
		    ci = ci->next;
		}
	    }
	}
    }

/* Delete from SOP list if on it */
    for (i = 0; i < nsop; ++i)
	if (stricmp (sops[i].nick, ni->nick) == 0)
	{
	    if (i < nsop)
		bcopy (sops + i + 1, sops + i, sizeof (*sops) * (nsop - i));
	    --nsop;
	    break;
	}

    if (ni->next)
	ni->next->prev = ni->prev;
    if (ni->prev)
	ni->prev->next = ni->next;
    else
	nicklists[tolower (*ni->nick)] = ni->next;
    if (ni->email)
	free (ni->email);
    if (ni->url)
	free (ni->url);
    if (ni->last_usermask)
	free (ni->last_usermask);
    if (ni->last_realname)
	free (ni->last_realname);
    if (ni->access)
    {
	for (i = 0; i < ni->accesscount; ++i)
	    free (ni->access[i]);
	free (ni->access);
    }
    if(memos_on==TRUE)
    {
	if (ni->ignore)
	{
	    for (i = 0; i < ni->ignorecount; ++i)
		free (ni->ignore[i]);
	    free (ni->ignore);
	}
    }
    free (ni);
    ni = NULL;

    return cnt;
}

/*************************************************************************/

/* Collide a nick. */

static void
collide (NickInfo * ni)
{
    char newnick[NICKMAX];
    int success;

    del_timeout (ni, TO_COLLIDE);
#ifdef DAL_SERV
    for (success = 0, strscpy (newnick, ni->nick, NICKMAX);
	 strlen (newnick) < NICKMAX && success != 1;)
    {
	snprintf (newnick, sizeof (newnick), "%s_", newnick);
	if (!(finduser (newnick)))
	    success = 1;
    }
    if (success != 1)
	for (success = 0, strscpy (newnick, ni->nick, NICKMAX);
	     strlen (newnick) < NICKMAX && success != 1;)
	{
	    snprintf (newnick, sizeof (newnick), "%s-", newnick);
	    if (!(finduser (newnick)))
		success = 1;
	}
    if (success != 1)
	for (success = 0, strscpy (newnick, ni->nick, NICKMAX);
	     strlen (newnick) < NICKMAX && success != 1;)
	{
	    snprintf (newnick, sizeof (newnick), "%s~", newnick);
	    if (!(finduser (newnick)))
		success = 1;
	}

    if (success != 1)
	kill_user (s_NickServ, ni->nick, NS_FORCED_KILL);
    else
    {
	User *u;
	if (!(u = finduser (ni->nick)))
	{
	    write_log ("user: NICK KILL from nonexistent nick %s", ni->nick);
	    return;
	}
	send_cmd (s_NickServ, "SVSNICK %s %s :%lu",
		  ni->nick, newnick, time (NULL));
	notice (s_NickServ, newnick, NS_FORCED_CHANGE);
/*	change_user_nick (u, newnick);     /* SVSNICK does this for us */
    }
#else
    kill_user (s_NickServ, ni->nick, NS_FORCED_KILL);
#endif
    if (release_timeout > 0)
    {
	if (nickhold >= killlist_size) {
	    if (killlist_size < 8)
		killlist_size = 8;
	    else
		killlist_size *= 2;
	    killlist = srealloc (killlist, sizeof (*killlist) * killlist_size);
	}
	strcpy(killlist[nickhold].nick, ni->nick);
	killlist[nickhold].time = time(NULL);
	nickhold++;
    }
}

void time_to_die(void)
{
    int i, j;
    NickInfo *ni;
    char *av[2];

    if (!i_am_backup()) {
	for (i=0; i<nickhold; i++) {
	    if (killlist[i].time + wait_collide < time(NULL)) {
		ni = findnick (killlist[i].nick);
		send_cmd (NULL, "NICK %s %d 1 enforcer %s %s :%s Enforcement",
		ni->nick, time (NULL), services_host, server_name, s_NickServ);
		av[0] = ni->nick;
		av[1] = "nick kill";
		do_kill (s_NickServ, 1, av);
		ni->flags |= NI_KILL_HELD;
		add_timeout (ni, TO_RELEASE, release_timeout);

		nickhold--;
		for (j=i; j < nickhold; j++) {
		    strscpy (killlist[j].nick, killlist[j+1].nick, NICKMAX);
		    killlist[j].time = killlist[j+1].time;
		}
		i--;
	    }
	}
    }
}

/*************************************************************************/

/* Release hold on a nick. */

static void
release (NickInfo * ni)
{
    del_timeout (ni, TO_RELEASE);
    send_cmd (ni->nick, "QUIT");
    ni->flags &= ~NI_KILL_HELD;
}

/*************************************************************************/

/* Add a timeout to the timeout list. */

static void
add_timeout (NickInfo * ni, int type, time_t delay)
{
    Timeout *to;

    to = smalloc (sizeof (Timeout));
    to->next = timeouts;
    to->prev = NULL;
    if (timeouts)
	timeouts->prev = to;
    timeouts = to;
    to->ni = ni;
    to->type = type;
    to->settime = time (NULL);
    to->timeout = time (NULL) + delay;
}

/*************************************************************************/

/* Delete a timeout from the timeout list. */

static void
del_timeout (NickInfo * ni, int type)
{
    Timeout *to, *to2;

    to = timeouts;
    while (to)
	if (to->ni == ni && to->type == type)
	{
	    to2 = to->next;
	    if (to->next)
		to->next->prev = to->prev;
	    if (to->prev)
		to->prev->next = to->next;
	    else
		timeouts = to->next;
	    free (to);
	    to = to2;
	}
	else
	    to = to->next;
}

/*************************************************************************/
/*********************** NickServ command routines ***********************/
/*************************************************************************/

/* Return a help message. */

static void
do_help (const char *source)
{
    char *cmd = strtok (NULL, "");
    char buf[BUFSIZE];

    if (cmd)
    {
	Hash_HELP *command, hash_table[] =
	{
	    {"LISTOP*", H_OPER, listoper_help},
	    {"SLAVE*", H_OPER, slaves_help},
	    {"SET *OP*", H_OPER, set_ircop_help},
	    {"DROP", H_SOP, oper_drop_help},
	    {"GETPASS", H_SOP, getpass_help},
	    {"FORBID", H_SOP, forbid_help},
	    {"SUSPEND", H_SOP, suspend_help},
	    {"UNSUSPEND", H_SOP, unsuspend_help},
	    {"DEOP", H_SOP, deop_help},
	    {NULL}
	};

	if (command = get_help_hash (source, cmd, hash_table))
	    notice_list (s_NickServ, source, command->process);
	else
	{
	    snprintf (buf, sizeof (buf), "%s%s", s_NickServ, cmd ? " " : "");
	    strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	    helpserv (s_NickServ, source, buf);
	}
    }
    else
    {
	snprintf (buf, sizeof (buf), "%s%s", s_NickServ, cmd ? " " : "");
	strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	helpserv (s_NickServ, source, buf);
    }
}

/*************************************************************************/

/* Register a nick. */

static void
do_register (const char *source)
{
    NickInfo *ni;
    User *u;
    char *pass = strtok (NULL, " ");

    if (services_level != 1)
    {
	notice (s_NickServ, source, ERR_TEMP_DISABLED, "REGISTER");
	return;
    }

    if (!pass)
    {
	notice (s_NickServ, source, "Syntax: \2REGISTER \37password\37\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "REGISTER");

    }
    else if (!(u = finduser (source)))
    {
	write_log ("%s: Can't register nick %s: nick not online", s_NickServ, source);
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (is_services_nick (source))
    {
	write_log ("%s: %s@%s tried to register a SERVICES nick %s", s_NickServ,
		   u->username, u->host, source);
	notice (s_NickServ, source, NS_CANNOT_REGISTER, source);

    }
    else if (ni = findnick (source))
	if (ni->flags & NI_VERBOTEN)
	{
	    write_log ("%s: %s@%s tried to register FORBIDden nick %s", s_NickServ,
		       u->username, u->host, source);
	    notice (s_NickServ, source, NS_CANNOT_REGISTER, source);
	}
	else
	    notice (s_NickServ, source, NS_TAKEN, source);

    else if (stricmp (source, pass) == 0 || strlen (pass) < 5)
    {
	notice (s_NickServ, source,
		"Please try again with a more obscure password.");
	notice (s_NickServ, source,
		"Passwords should be at least five characters long, should");
	notice (s_NickServ, source,
	    "not be something easily guessed (e.g. your real name or your");
	notice (s_NickServ, source,
		"nick), and cannot contain the space character.");
	notice (s_NickServ, source,
		"\2/msg %s HELP REGISTER\2 for more information.");

    }
    else if (ni = makenick (source))
    {
	strscpy (ni->pass, pass, PASSMAX);
	ni->email = NULL;
	ni->url = NULL;
	ni->last_usermask = smalloc (strlen (u->username) + strlen (u->host) + 2);
	snprintf (ni->last_usermask, strlen (u->username) + strlen (u->host) + 2, 
					"%s@%s", u->username, u->host);
	ni->last_realname = sstrdup (u->realname);
	ni->time_registered = ni->last_seen = time (NULL);
	if (is_oper (source))
	    ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_KILLPROTECT | NI_IRCOP;
	else
	    ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_KILLPROTECT;
	ni->accesscount = 1;
	ni->access = smalloc (sizeof (char *));
	ni->access[0] = create_mask (u);
	S_nick_reg++;
	write_log ("%s: `%s' registered by %s@%s", s_NickServ,
		   source, u->username, u->host);
	notice (s_NickServ, source, NS_REGISTERED, ni->access[0]);
	notice (s_NickServ, source, NS_CHANGE_PASSWORD, pass);
	if (show_sync_on==TRUE)
	    notice(s_NickServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
    else
	notice (s_NickServ, source, ERR_FAILED, "REGISTER");
}

/*************************************************************************/

/* Link a nick to a host */

static void
do_link (const char *source)
{
    NickInfo *ni, *hni;
    User *u;
    char *hostnick = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");

    if (services_level != 1)
    {
	notice (s_NickServ, source, ERR_TEMP_DISABLED, "LINK");
	return;
    }

    if (!pass)
    {
	notice (s_NickServ, source, "Syntax: \2LINK \37nick\37\2 \37password\37");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "LINK");

    }
    else if (!(u = finduser (source)))
    {
	write_log ("%s: Can't link nick %s: nick not online", s_NickServ, source);
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (is_services_nick (source))
    {
	write_log ("%s: %s@%s tried to link a SERVICES nick %s", s_NickServ,
		   u->username, u->host, source);
	notice (s_NickServ, source, NS_CANNOT_LINK, source);

    }
    else if (ni = findnick (source))
	if (ni->flags & NI_VERBOTEN)
	{
	    write_log ("%s: %s@%s tried to link FORBIDden nick %s", s_NickServ,
		       u->username, u->host, source);
	    notice (s_NickServ, source, NS_CANNOT_LINK, source);
	}
	else
	    notice (s_NickServ, source, NS_TAKEN, source);

    else if (!(hni = findnick (hostnick)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, hostnick);

    else if (hni->flags & NI_SLAVE)
	notice (s_NickServ, source, ERR_CANT_BE_LINK, "LINK");

    else if (strcmp (hni->pass, pass) != 0)
	notice (s_NickServ, source, ERR_WRONG_PASSWORD);

    else if (ni = makenick (source))
    {
	strscpy (ni->pass, "", PASSMAX);
	ni->email = NULL;
	ni->url = NULL;
	if (hni->last_usermask)
	    free (hni->last_usermask);
	hni->last_usermask = smalloc (strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3);
	snprintf (hni->last_usermask, strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3, 
				"%s!%s@%s", u->nick, u->username, u->host);
	ni->last_usermask = sstrdup (hni->nick);
	ni->last_realname = sstrdup (u->realname);
	ni->time_registered = ni->last_seen = hni->last_seen = time (NULL);
	ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_SLAVE;
	S_nick_link++;
	write_log ("%s: `%s' linked to %s", s_NickServ,
		   source, hni->nick);
	notice (s_NickServ, source, NS_LINKED, hni->nick);
	if (show_sync_on==TRUE)
	    notice(s_NickServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
    else
	notice (s_NickServ, source, ERR_FAILED, "LINK");
}

/*************************************************************************/

static void
do_identify (const char *source)
{
    do_real_identify (s_NickServ, source);
}

void
do_real_identify (const char *whoami, const char *source)
{
    char *pass = strtok (NULL, " ");
    NickInfo *ni = findnick (source), *hni = host (ni);
    User *u;

    if (!pass)
    {
	notice (whoami, source, "Syntax: \2IDENTIFY \37password\37\2");
	notice (whoami, source, ERR_MORE_INFO, whoami, "IDENTIFY");

    }
    else if (!ni || !hni)
	notice (whoami, source, NS_YOU_NOT_REGISTERED);

    else if (!(u = finduser (source)))
    {
	write_log ("%s: IDENTIFY from nonexistent nick %s", whoami, source);
	notice (whoami, source, ERR_YOU_DONT_EXIST);

    }
    else if (hni->flags & NI_SUSPENDED)
	notice (whoami, source, ERR_SUSPENDED_NICK);

    else if (strcmp (pass, hni->pass) != 0)
    {
	write_log ("%s: Failed IDENTIFY for %s!%s@%s",
		   whoami, source, u->username, u->host);
	notice (whoami, source, ERR_WRONG_PASSWORD);
	u->passfail++;
	if (u->passfail >= passfail_max)
	    kill_user (s_NickServ, ni->nick, NS_FAILMAX_KILL);
    }
    else
    {
	ni->flags |= NI_IDENTIFIED;
	if (!(ni->flags & NI_RECOGNIZED))
	{
	    ni->last_seen = hni->last_seen = time (NULL);
	    if (!(hni->flags & NI_SUSPENDED))
	    {
		if (hni->last_usermask)
		    free (hni->last_usermask);
		hni->last_usermask = smalloc (strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3);
		snprintf (hni->last_usermask, strlen (u->nick) + strlen (u->username) + strlen (u->host) + 3,
				"%s!%s@%s", u->nick, u->username, u->host);
	    }
	    if (ni->last_realname)
		free (ni->last_realname);
	    ni->last_realname = sstrdup (u->realname);
	    write_log ("%s: %s!%s@%s identified for nick %s", whoami,
		       source, u->username, u->host, source);
	}
	notice (whoami, source, NS_IDENTIFIED);
	u->passfail = 0;
	S_nick_ident++;
	if (is_oper (source) && !(hni->flags & NI_IRCOP))
	{
	    notice (whoami, source,
	       "You have not set the \2IRC Operator\2 flag for your nick.");
	    notice (whoami, source,
	      "Please set this with \2/msg %s SET IRCOP ON\2.", s_NickServ);
	}
#ifdef DAL_SERV
	send_cmd (whoami, "SVSMODE %s +Rr", u->nick);
	strscpy(u->mode, changemode ("Rr", u->mode), sizeof(u->mode));
	if (is_services_op (source))
	{
	    send_cmd (whoami, "SVSMODE %s +a", u->nick);
	    strscpy(u->mode, changemode ("a", u->mode), sizeof(u->mode));
	}
#endif
	if(memos_on==TRUE)
	    if (!(ni->flags & NI_RECOGNIZED) && services_level == 1)
		check_memos (source);
    }
}

/*************************************************************************/

static void
do_slaves (const char *source)
{
    char *nick = strtok (NULL, " ");
    NickInfo *ni;
    User *u = finduser (source);

    if (!is_oper (source) && nick)
    {
	notice (s_NickServ, source, "Syntax: \2SLAVES\2%s", is_oper(source) ? " \37[nick]\37" : "");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SLAVES");

    }
    else if (!(ni = findnick (nick ? nick : source)))
	if (nick)
	    notice (s_NickServ, source, NS_NOT_REGISTERED, nick);
	else
	    notice (s_NickServ, source, NS_YOU_NOT_REGISTERED);

    else if (!u)
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    else if (!nick && !userisnick(source))
    {
	notice (s_NickServ, source, NS_NOT_YOURS, source);
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: SLAVES from nonexistent user %s", s_NickServ, source);

    }
    else
    {
	NickInfo *hni = host(ni);
	int i = slavecount (hni->nick), comma = 0;
	char buf[512];
	
	if (i < 1)
	    if (nick)
		notice (s_NickServ, source, NS_NO_SLAVES, nick);
	    else
		notice (s_NickServ, source, NS_YOU_NO_SLAVES);
	else
	{
	    snprintf (buf, sizeof(buf), "\2%s\2: ", hni->nick);
	    for (; i > 0; i--) {
		if (strlen(buf) + strlen (slave (hni->nick, i)->nick) >= sizeof(buf))
		{
		    notice (s_NickServ, source, "%s", buf);
		    snprintf (buf, sizeof(buf), "\2%s\2: ", hni->nick);
		    comma=0; i++;
		} else {
		    snprintf (buf, sizeof(buf), "%s%s%s", buf, comma ? ", " : "", slave (hni->nick, i)->nick);
		    comma=1;
		}
		if (i==1)
		    notice (s_NickServ, source, "%s", buf);
	    }
	}
    }
}

/*************************************************************************/

static void
do_drop (const char *source)
{
    char *nick = strtok (NULL, " ");
    NickInfo *ni;
    User *u = finduser (source);

    if (services_level != 1 && !is_services_op (source))
    {
	notice (s_NickServ, source, ERR_TEMP_DISABLED, "DROP");
	return;
    }

    if (!is_services_op (source) && nick)
    {
	notice (s_NickServ, source, "Syntax: \2DROP\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "DROP");

    }
    else if (!(ni = findnick (nick ? nick : source)))
	if (nick)
	    notice (s_NickServ, source, NS_NOT_REGISTERED, nick);
	else
	    notice (s_NickServ, source, NS_YOU_NOT_REGISTERED);

    else if (!is_services_op (source) && (host (ni)->flags & NI_SUSPENDED))
	notice (s_NickServ, source, ERR_SUSPENDED_NICK);

    else if (!nick && (!u || !(ni->flags & NI_IDENTIFIED)))
    {
	notice (s_NickServ, source, ERR_NEED_PASSWORD, "DROP");
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: DROP from nonexistent user %s", s_NickServ, source);

    }
    else if (nick && !u)
    {
	write_log ("%s: DROP %s from nonexistent oper %s", s_NickServ, nick, source);
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (nick && (host(ni)->flags & NI_IRCOP))
	notice (s_NickServ, source, ERR_NOT_ON_IRCOP, "DROP");

    else
    {
	int i;
	if (services_level != 1)
	    notice (s_NickServ, source, ERR_READ_ONLY);
	i = delnick (ni) - 1;
	S_nick_drop += i+1;
	write_log ("%s: %s!%s@%s dropped nickname %s (and %d slaves)", s_NickServ,
		   source, u->username, u->host, nick ? nick : source, i);
	if (nick)
	    notice (s_NickServ, source, NS_DROPPED, nick,
		i ? " (and " : "", i ? myitoa (i) : "", i ? " slaves)" : "");
	else
	    notice (s_NickServ, source, NS_DROPPED, source,
		i ? " (and " : "", i ? myitoa (i) : "", i ? " slaves)" : "");
    }
}

/*************************************************************************/

static void
do_slavedrop (const char *source)
{
    char *nick = strtok (NULL, " ");
    NickInfo *ni, *hni;
    User *u = finduser (source);

    if (services_level != 1)
    {
	notice (s_NickServ, source, ERR_TEMP_DISABLED, "DROPSLAVE");
	return;
    }

    if (!nick)
    {
	notice (s_NickServ, source, "Syntax: \2DROPSLAVE\2 \37nick\37");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "DROPSLAVE");

    }
    else if (stricmp (source, nick) == 0)
	notice (s_NickServ, source, ERR_NOT_ON_YOURSELF);

    else if (!(ni = findnick (nick)) || !(hni = host (ni)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    else if (stricmp (hni->nick, source) != 0)
    {
	if (issibling(findnick(source), nick))
	    notice (s_NickServ, source, ERR_MUST_BE_HOST, "DROPSLAVE");
	else
	    notice (s_NickServ, source, ERR_ACCESS_DENIED);

    }
    else if (hni->flags & NI_SUSPENDED)
	notice (s_NickServ, source, ERR_SUSPENDED_NICK);

    else if (!u || !(hni->flags & NI_IDENTIFIED))
    {
	notice (s_NickServ, source, ERR_NEED_PASSWORD, "DROPSLAVE");
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: DROPSLAVE from nonexistent user %s", s_NickServ, source);

    }
    else
    {
	if (services_level != 1)
	    notice (s_NickServ, source, ERR_READ_ONLY);
	delnick (ni);
	S_nick_drop++;
	write_log ("%s: %s!%s@%s dropped nickname %s (slave)", s_NickServ,
		   source, u->username, u->host, nick);
	notice (s_NickServ, source, NS_DROPPED, nick, "", "", "");
    }
}

/*************************************************************************/

static void
do_set (const char *source)
{
    char *cmd = strtok (NULL, " ");
    char *param = strtok (NULL, " ");
    NickInfo *ni = findnick (source);
    User *u;

    if (services_level != 1)
    {
	notice (s_NickServ, source, ERR_TEMP_DISABLED, "SET");
	return;
    }

    if (!param)
    {
	notice (s_NickServ, source,
		"Syntax: \2SET \37option\37 \37parameters\37\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET");

    }
    else if (!(ni = findnick (source)))
	notice (s_NickServ, source, NS_YOU_NOT_REGISTERED);

    else if (getflags (ni) & NI_SUSPENDED)
	notice (s_NickServ, source,
		ERR_SUSPENDED_NICK);

    else if (!(u = finduser (source)) || !(ni->flags & NI_IDENTIFIED))
    {
	notice (s_NickServ, source, ERR_NEED_PASSWORD, "SET");
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: SET from nonexistent user %s", s_NickServ, source);

    }
    else
    {
	Hash_NI *command, hash_table[] =
	{
	    {"PASS*", H_NONE, do_set_password},
	    {"KILL*", H_NONE, do_set_kill},
	    {"*MAIL", H_NONE, do_set_email},
	    {"CONTACT", H_NONE, do_set_email},
	    {"U*R*L", H_NONE, do_set_url},
	    {"W*W*W", H_NONE, do_set_url},
	    {"HOME*", H_NONE, do_set_url},
	    {"*PAGE", H_NONE, do_set_url},
	    {"*M*S*G*", H_NONE, do_set_privmsg},
	    {"PRIV*", H_NONE, do_set_private},
	    {"SEC*", H_NONE, do_set_secure},
	    {"*OP*", H_OPER, do_set_ircop},
	    {NULL}
	};

	if (command = get_ni_hash (source, cmd, hash_table))
	    (*command->process) (ni, param);
	else
	    notice (s_NickServ, source, ERR_UNKNOWN_OPTION,
		    strupper (cmd), s_NickServ, "SET");
    }
}

/*************************************************************************/

static void
do_set_password (NickInfo * ni, char *param)
{
    char *source = ni->nick;
    if (stricmp (ni->nick, param) == 0 || stricmp (host (ni)->nick, param) == 0 ||
	strlen (param) < 5)
    {
	notice (s_NickServ, source,
		"Please try again with a more obscure password.");
	notice (s_NickServ, source,
		"Passwords should be at least five characters long, should");
	notice (s_NickServ, source,
	    "not be something easily guessed (e.g. your real name or your");
	notice (s_NickServ, source,
		"nick), and cannot contain the space character.");
	notice (s_NickServ, source,
		"\2/msg %s HELP SET PASSWORD\2 for more information.");
    }
    else
    {
	strscpy (host (ni)->pass, param, PASSMAX);
	notice (s_NickServ, ni->nick, NS_CHANGE_PASSWORD, param);
    }
}

static void
do_set_email (NickInfo * ni, char *param)
{
    free (host (ni)->email);
    if (stricmp (param, "NONE") == 0)
    {
	host(ni)->email = NULL;
	notice (s_NickServ, ni->nick, "%s removed.", INFO_EMAIL);
    }
    else
    {
	host (ni)->email = sstrdup (param);
	notice (s_NickServ, ni->nick, "%s changed to \2%s\2.", INFO_EMAIL, param);
    }
}

static void
do_set_url (NickInfo * ni, char *param)
{
    free (host (ni)->url);
    if (stricmp (param, "NONE") == 0)
    {
	host(ni)->url = NULL;
	notice (s_NickServ, ni->nick, "%s removed.", INFO_URL);
    }
    else
    {
	host (ni)->url = sstrdup (param);
	notice (s_NickServ, ni->nick, "%s changed to \2%s\2.", INFO_URL, param);
    }
}

/*************************************************************************/

static void
do_set_kill (NickInfo * ni, char *param)
{
    char *source = ni->nick;

    if (stricmp (param, "ON") == 0)
    {
	host (ni)->flags |= NI_KILLPROTECT;
	notice (s_NickServ, source, "%s is now \2ON\2.", NS_FLAG_KILLPROTECT);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	host (ni)->flags &= ~NI_KILLPROTECT;
	notice (s_NickServ, source, "%s is now \2OFF\2.", NS_FLAG_KILLPROTECT);

    }
    else
    {
	notice (s_NickServ, source, "Syntax: \2SET KILL {ON|OFF}\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET KILL");
    }
}

static void
do_set_private (NickInfo * ni, char *param)
{
    char *source = ni->nick;

    if (stricmp (param, "ON") == 0)
    {
	host (ni)->flags |= NI_PRIVATE;
	notice (s_NickServ, source, "%s is now \2ON\2.", NS_FLAG_PRIVATE);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	host (ni)->flags &= ~NI_PRIVATE;
	notice (s_NickServ, source, "%s is now \2OFF\2.", NS_FLAG_PRIVATE);

    }
    else
    {
	notice (s_NickServ, source, "Syntax: \2SET PRIVATE {ON|OFF}\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET PRIVATE");
    }
}

/*************************************************************************/

static void
do_set_secure (NickInfo * ni, char *param)
{
    char *source = ni->nick;

    if (stricmp (param, "ON") == 0)
    {
	host (ni)->flags |= NI_SECURE;
	notice (s_NickServ, source, "%s is now \2ON\2.", NS_FLAG_SECURE);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	host (ni)->flags &= ~NI_SECURE;
	notice (s_NickServ, source, "%s is now \2OFF\2.", NS_FLAG_SECURE);

    }
    else
    {
	notice (s_NickServ, source, "Syntax: \2SET SECURE {ON|OFF}\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET SECURE");
    }
}

/*************************************************************************/

static void
do_set_privmsg (NickInfo * ni, char *param)
{
    char *source = ni->nick;

    if (stricmp (param, "ON") == 0)
    {
	host (ni)->flags |= NI_PRIVMSG;
	notice (s_NickServ, source, "Now using \2PRIVMSG\2 instead of NOTICE.");

    }
    else if (stricmp (param, "OFF") == 0)
    {
	host (ni)->flags &= ~NI_PRIVMSG;
	notice (s_NickServ, source, "Now using \2NOTICE\2 instead of PRIVMSG.");

    }
    else
    {
	notice (s_NickServ, source, "Syntax: \2SET PRIVMSG {ON|OFF}\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET PRIVMSG");
    }
}

/*************************************************************************/

static void
do_set_ircop (NickInfo * ni, char *param)
{
    char *source = ni->nick;

    if (stricmp (param, "ON") == 0)
    {
	host (ni)->flags |= NI_IRCOP;
	notice (s_NickServ, source, "%s is now \2ON\2.", NS_FLAG_IRCOP);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	host (ni)->flags &= ~NI_IRCOP;
	notice (s_NickServ, source, "%s is now \2OFF\2.", NS_FLAG_IRCOP);

    }
    else
    {
	notice (s_NickServ, source, "Syntax: \2SET IRCOP {ON|OFF}\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "SET IRCOP");
    }
}

/*************************************************************************/

static void
do_deop (const char *source)
{
    NickInfo *ni;
    char *s;

    if (!(s = strtok (NULL, " ")))
    {
	notice (s_NickServ, source, "Syntax: \2DEOP \37nickname\37\2");
	return;
    }
    if (!(ni = host (findnick (s))))
	notice (s_NickServ, source, NS_NOT_REGISTERED, s);
    else if (is_services_op (ni->nick))
	notice (s_NickServ, source, ERR_ACCESS_DENIED);
    else
    {
	host (ni)->flags &= ~NI_IRCOP;
	notice (s_NickServ, source, "%s for \2%s\2 is now \2OFF\2.",
		NS_FLAG_IRCOP, ni->nick);
    }
}

/*************************************************************************/

static void
do_access (const char *source)
{
    char *cmd = strtok (NULL, " ");
    NickInfo *ni = findnick (source);
    User *u;

    if (!cmd)
    {
	notice (s_NickServ, source,
		"Syntax: \2ACCESS {ADD|DEL|LIST} [\37mask\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "ACCESS");
    }
    else if (!(u = finduser (source)))
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    else if (!ni)
	notice (s_NickServ, source, NS_YOU_NOT_REGISTERED);

    else if (host (ni)->flags & NI_SUSPENDED)
	notice (s_NickServ, source, ERR_SUSPENDED_NICK);

    else if (!(ni->flags & NI_IDENTIFIED))
    {
	notice (s_NickServ, source, ERR_NEED_PASSWORD, "ACCESS");
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: SET from nonexistent user %s", s_NickServ, source);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_NONE, do_access_add},
	    {"CREATE", H_NONE, do_access_add},
	    {"MAKE", H_NONE, do_access_add},
	    {"*CUR*ENT", H_NONE, do_access_current},
	    {"ADDMASK", H_NONE, do_access_current},
	    {"MY*MASK", H_NONE, do_access_current},
	    {"DEL*", H_NONE, do_access_del},
	    {"ERASE", H_NONE, do_access_del},
	    {"TRASH", H_NONE, do_access_del},
	    {"LIST*", H_NONE, do_access_list},
	    {"VIEW", H_NONE, do_access_list},
	    {"DISP*", H_NONE, do_access_list},
	    {"SHOW*", H_NONE, do_access_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_NickServ, source,
		    "Syntax: \2ACCESS {ADD|DEL|LIST} [\37mask\37]\2");
	    notice (s_NickServ, source, ERR_UNKNOWN_COMMAND, cmd,
	       s_NickServ, "ACCESS");
	}
    }
}

static int
do_access_help (const char *source, char *cmd, char *mask)
{
    if (!cmd || ((stricmp (cmd, "LIST") == 0) ? !!mask : !mask))
    {
	notice (s_NickServ, source,
		"Syntax: \2ACCESS {ADD|DEL} [\37mask\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "ACCESS");
	return 1;
    }
    return 0;
}

static void
do_access_add (const char *source)
{
    char *mask = strtok (NULL, " ");
    NickInfo *ni = host (findnick (source));
    int i;
    char **access;

    if (!mask)
    {
	notice (s_NickServ, source,
		"Syntax: \2ACCESS ADD \37mask\37\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "ACCESS ADD");
    }
    else if (mask && !strchr (mask, '@'))
	notice (s_NickServ, source, ERR_FORMATTING, "user@host");

    else
    {
	for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
	    if (match_wild_nocase (*access, mask))
	    {
		notice (s_NickServ, source, LIST_THERE, mask, "your", "ACCESS");
		return;
	    }
	++ni->accesscount;
	ni->access = srealloc (ni->access, sizeof (char *) * ni->accesscount);
	ni->access[ni->accesscount - 1] = sstrdup (mask);
	notice (s_NickServ, source, LIST_ADDED, mask, "your", "ACCESS");
    }
}

static void
do_access_current (const char *source)
{
    NickInfo *ni = host (findnick (source));
    User *u = finduser(source);
    int i;
    char **access;

	char *mask = create_mask (u);
	for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
	    if (match_wild_nocase (*access, mask))
	    {
		notice (s_NickServ, source, LIST_THERE, mask, "your", "ACCESS");
		free (mask);
		return;
	    }
	++ni->accesscount;
	ni->access = srealloc (ni->access, sizeof (char *) * ni->accesscount);
	ni->access[ni->accesscount - 1] = sstrdup (mask);
	notice (s_NickServ, source, LIST_ADDED, mask, "your", "ACCESS");
	free (mask);
}

static void
do_access_del (const char *source)
{
    char *mask = strtok (NULL, " ");
    NickInfo *ni = host (findnick (source));
    int i;
    char **access;

    if (!mask)
    {
	notice (s_NickServ, source,
		"Syntax: \2ACCESS DEL \37mask|num\37\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "ACCESS DEL");
    }
    else
    {
	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn (mask, "1234567890") == strlen (mask) &&
	    (i = atoi (mask)) > 0 && i <= ni->accesscount)
	{
	    --i;
	    access = &ni->access[i];
	}
	else
	{
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
		if (strcmp (*access, mask) == 0)
		    break;
	    if (i == ni->accesscount)
		for (access = ni->access, i = 0; i < ni->accesscount;
		     ++access, ++i)
		    if (stricmp (*access, mask) == 0)
			break;
	    if (i == ni->accesscount)
	    {
		notice (s_NickServ, source, LIST_NOT_THERE, mask, "your", "ACCESS");
		return;
	    }
	}
	notice (s_NickServ, source, LIST_REMOVED, *access, "your", "ACCESS");
	if (*access)
	    free (*access);
	--ni->accesscount;
	if (i < ni->accesscount)	/* if it wasn't the last entry... */
	    bcopy (access + 1, access, (ni->accesscount - i) * sizeof (char *));
	if (ni->accesscount)	/* if there are any entries left... */
	    ni->access = srealloc (ni->access, ni->accesscount * sizeof (char *));
	else
	{
	    if (ni->access)
		free (ni->access);
	    ni->access = NULL;
	}
    }
}

static void
do_access_list (const char *source)
{
    char *mask = strtok (NULL, " ");
    NickInfo *ni = host (findnick (source));
    int i;
    char **access;

    notice (s_NickServ, source, "Access list:");
    for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
    {
	if (mask && !match_wild_nocase (mask, *access))
	    continue;
	notice (s_NickServ, source, "  %3d   %s", i + 1, *access);
    }
}

/*************************************************************************/

static void
do_ignore (const char *source)
{
    char *cmd = strtok (NULL, " ");
    NickInfo *ni = findnick (source);
    User *u;

    if (!cmd)
    {
	notice (s_NickServ, source,
		"Syntax: \2IGNORE {ADD|DEL|LIST} [\37nick\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "IGNORE");

    }
    else if (!(u = finduser (source)))
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    else if (!ni)
	notice (s_NickServ, source, NS_YOU_NOT_REGISTERED);

    else if (host (ni)->flags & NI_SUSPENDED)
	notice (s_NickServ, source,
		ERR_SUSPENDED_NICK);

    else if (!(ni->flags & NI_IDENTIFIED))
    {
	notice (s_NickServ, source, ERR_NEED_PASSWORD, "IGNORE");
	notice (s_NickServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);
	if (!u)
	    write_log ("%s: SET from nonexistent user %s", s_NickServ, source);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_NONE, do_ignore_add},
	    {"CREATE", H_NONE, do_ignore_add},
	    {"MAKE", H_NONE, do_ignore_add},
	    {"DEL*", H_NONE, do_ignore_del},
	    {"ERASE", H_NONE, do_ignore_del},
	    {"TRASH", H_NONE, do_ignore_del},
	    {"LIST*", H_NONE, do_ignore_list},
	    {"VIEW", H_NONE, do_ignore_list},
	    {"DISP*", H_NONE, do_ignore_list},
	    {"SHOW*", H_NONE, do_ignore_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_NickServ, source,
		    "Syntax: \2IGNORE {ADD|DEL|LIST} [\37nick\37]\2");
	    notice (s_NickServ, source, ERR_UNKNOWN_COMMAND, cmd,
	    				s_NickServ, "IGNORE");
	}
    }
}

static int
do_ignore_help (const char *source, char *cmd, char *nick)
{
    if (!cmd || ((stricmp (cmd, "LIST") == 0) ? !!nick : !nick))
    {
	notice (s_NickServ, source,
		"Syntax: \2IGNORE {ADD|DEL} [\37nick\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "IGNORE");
	return 1;
    }
    return 0;
}

static void
do_ignore_add (const char *source)
{
    char *nick = strtok (NULL, " ");

    if (!do_ignore_help (source, "ADD", nick))
    {
	NickInfo *ni = host (findnick (source)), *tni;
	int i;
	char **ignore;

	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
	    if (stricmp (*ignore, nick) == 0)
	    {
		notice (s_NickServ, source, LIST_THERE, *ignore, "your", "IGNORE");
		return;
	    }
	if (!(tni = host (findnick (nick))))
	{
	    notice (s_NickServ, source, NS_NOT_REGISTERED, nick);
	    return;
	}
	else if (stricmp (source, nick) == 0)
	{
	    notice (s_NickServ, source, ERR_NOT_ON_YOURSELF, "IGNORE ADD");
	    return;
	}
	else
	{
	    ++ni->ignorecount;
	    ni->ignore = srealloc (ni->ignore, sizeof (char *) * ni->ignorecount);
	    ni->ignore[ni->ignorecount - 1] = sstrdup (tni->nick);
	    notice (s_NickServ, source, LIST_ADDED, tni->nick, "your", "IGNORE");
	}
    }
}
static void
do_ignore_del (const char *source)
{
    char *nick = strtok (NULL, " ");

    if (!do_ignore_help (source, "DEL", nick))
    {
	NickInfo *ni = host (findnick (source));
	int i;
	char **ignore;

	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn (nick, "1234567890") == strlen (nick) &&
	    (i = atoi (nick)) > 0 && i <= ni->ignorecount)
	{
	    --i;
	    ignore = &ni->ignore[i];
	}
	else
	{
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
		if (strcmp (*ignore, nick) == 0)
		    break;
	    if (i == ni->ignorecount)
		for (ignore = ni->ignore, i = 0; i < ni->ignorecount;
		     ++ignore, ++i)
		    if (stricmp (*ignore, nick) == 0)
			break;
	    if (i == ni->ignorecount)
	    {
		notice (s_NickServ, source, LIST_NOT_THERE, nick, "your", "IGNORE");
		return;
	    }
	}
	notice (s_NickServ, source, LIST_REMOVED, *ignore, "your", "IGNORE");
	if (*ignore)
	    free (*ignore);
	--ni->ignorecount;
	if (i < ni->ignorecount)	/* if it wasn't the last entry... */
	    bcopy (ignore + 1, ignore, (ni->ignorecount - i) * sizeof (char *));
	if (ni->ignorecount)	/* if there are any entries left... */
	    ni->ignore = srealloc (ni->ignore, ni->ignorecount * sizeof (char *));
	else
	{
	    if (ni->ignore)
		free (ni->ignore);
	    ni->ignore = NULL;
	}
    }
}
static void
do_ignore_list (const char *source)
{
    char *nick = strtok (NULL, " ");

    if (!do_ignore_help (source, "LIST", nick))
    {
	char **ignore;
	NickInfo *ni = host (findnick (source)), *tni;
	int i;

	notice (s_NickServ, source, "Ignore list:");
	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
	{
	    if (nick && !match_wild_nocase (nick, *ignore))
		continue;

	    if ((tni = host(findnick(*ignore))) && (userisnick(tni->nick) ||
	    					strlen(tni->last_usermask)))
		notice (s_NickServ, source, "    %s (%s)", *ignore,
			userisnick(tni->nick) ? "ONLINE" : tni->last_usermask);
	    else
		notice (s_NickServ, source,
			"    %s", *ignore);
	}
    }
}

/*************************************************************************/

static void
do_info (const char *source)
{
    char *nick = strtok (NULL, " ");
    NickInfo *ni;

    if (!nick)
    {
	notice (s_NickServ, source, "Syntax: \2INFO \37nick\37\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "INFO");
    }
    else if (!(ni = findnick (nick)))
    {
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    }
    else if (ni->flags & NI_VERBOTEN)
	notice (s_NickServ, source, NS_CANNOT_REGISTER, ni->nick);

    else
    {
	char buf[BUFSIZE];
	long flags = getflags (ni);
	NickInfo *hni = host (ni);
	int i, onignore = is_on_ignore (source, nick);

	notice (s_NickServ, source, NS_INFO_INTRO, ni->nick, ni->last_realname);
	if (flags & NI_SLAVE)
	    notice (s_NickServ, source, NS_INFO_HOST, ni->last_usermask);
	if (!(flags & NI_SUSPENDED) && !onignore)
	{
	    if (host (ni)->email)
		notice (s_NickServ, source, NS_INFO_EMAIL, hni->email);
	    if (host (ni)->url)
		notice (s_NickServ, source, NS_INFO_URL, hni->url);
	}
	if (flags & NI_SUSPENDED)
	    notice (s_NickServ, source, NS_INFO_SUSPENDED, hni->last_usermask);
	else if (!(flags & NI_PRIVATE) && !userisnick (hni->nick) &&
				!slaveonline(hni->nick) && !onignore &&
				strlen(hni->last_usermask))
	    notice (s_NickServ, source, NS_INFO_USERMASK, hni->last_usermask);
	notice (s_NickServ, source, NS_INFO_REGISTERED,
					time_ago (ni->time_registered, 1));
	if (slaveonline (hni->nick) || userisnick (hni->nick))
	{
	    char *slaves;
	    NickInfo *sni;
	    int sc = slavecount (ni->nick);
	    slaves = smalloc (((sc + 1) * NICKMAX) + ((sc + 1) * 2));
	    if (stricmp (ni->nick, hni->nick) != 0 && userisnick (hni->nick))
		strcpy (slaves, finduser (hni->nick)->nick);
	    else
		*slaves = 0;
	    for (i = sc; i; i--)
	    {
		sni = slave (hni->nick, i);
		if (userisnick (sni->nick) && stricmp (sni->nick, ni->nick) != 0)
		{
		    if (*slaves)
			strcat (slaves, ", ");
		    strcat (slaves, sni->nick);
		}
	    }
	    if (*slaves && !onignore)
		notice (s_NickServ, source, userisnick (ni->nick) ?
			NS_INFO_AONLINE_AS : NS_INFO_ONLINE_AS, slaves);
	    free (slaves);
	}
	else
	{
	    if (!userisnick (ni->nick) && !onignore)
	    {
		notice (s_NickServ, source, NS_INFO_LAST_SEEN,
						time_ago (ni->last_seen, 1));
		if (!slaveonline (hni->nick) && !userisnick (hni->nick) &&
		    (ni->last_seen != hni->last_seen))
		    notice (s_NickServ, source, NS_INFO_LAST_ONLINE,
					    time_ago (hni->last_seen, 1));
	    }
	}
	*buf = 0;
	if (flags & NI_SUSPENDED)
	    strcat (buf, NS_FLAG_SUSPENDED);
	else
	{
	    if (flags & NI_KILLPROTECT)
		strcat (buf, NS_FLAG_KILLPROTECT);
	    if (flags & NI_SECURE)
	    {
		if (*buf)
		    strcat (buf, ", ");
		strcat (buf, NS_FLAG_SECURE);
	    }
	    if (flags & NI_PRIVATE)
	    {
		if (*buf)
		    strcat (buf, ", ");
		strcat (buf, NS_FLAG_PRIVATE);
	    }
	    if (flags & NI_IRCOP)
	    {
		if (*buf)
		    strcat (buf, ", ");
		if (is_justservices_op (nick))
		    strcat (buf, NS_FLAG_SOP);
		else
		    strcat (buf, NS_FLAG_IRCOP);
	    }
	    if (!*buf)
		strscpy (buf, NS_FLAG_NONE, BUFSIZE);
	}
	notice (s_NickServ, source, NS_INFO_OPTIONS, buf);
	if(memos_on==TRUE)
	{
	    if (flags & NI_SUSPENDED)
		notice (s_NickServ, source, NS_IS_SUSPENDED_MEMO, ni->nick);
	    else if (onignore)
		notice (s_NickServ, source, NS_AM_IGNORED, ni->nick);
	}
	if (userisnick (ni->nick) && !(flags & NI_SUSPENDED) && !onignore)
	    notice (s_NickServ, source, NS_INFO_ONLINE, finduser (nick)->nick);
	if (show_sync_on==TRUE)
	    notice(s_NickServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
}

/*************************************************************************/

static void
do_list (const char *source)
{
    char *pattern = strtok (NULL, " ");
    char *cmax = strtok (NULL, " ");
    NickInfo *ni;
    long flags;
    int nnicks, i, max = 50;
    char buf[BUFSIZE];

    if (!pattern)
    {
	notice (s_NickServ, source, "Syntax: \2LIST \37pattern\37\2 [\37max\37]");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "LIST");
    }
    else
    {
	if (cmax && atoi (cmax) > 0)
	    max = atoi (cmax);
	nnicks = 0;
	notice (s_NickServ, source, INFO_LIST_MATCH, pattern);
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next)
	    {
		flags = getflags(ni);
		if (ni->flags & NI_SLAVE)
		    continue;
		if (!(is_oper (source)))
		    if (flags & (NI_PRIVATE | NI_VERBOTEN | NI_SUSPENDED))
			continue;
		if (flags & (NI_VERBOTEN | NI_SUSPENDED))
		    if (strlen (ni->nick) > sizeof (buf))
			continue;
		    else if (strlen (ni->nick) + strlen (ni->last_usermask) > sizeof (buf))
			continue;
		if (flags & NI_VERBOTEN)
		    snprintf (buf, sizeof (buf), "%-20s  << FORBIDDEN >>", ni->nick);
		else if (flags & NI_SUSPENDED)
		    snprintf (buf, sizeof (buf), "%-20s  << SUSPENDED >>", ni->nick);
		else if (strlen (ni->last_usermask) > 0)
		    snprintf (buf, sizeof (buf), "%-20s  %s", ni->nick, userisnick(ni->nick) ? "<< ONLINE >>" : ni->last_usermask);
		else
		    snprintf (buf, sizeof (buf), "%-20s", ni->nick);
		if (match_wild_nocase (pattern, buf))
		    if (++nnicks <= max)
			notice (s_NickServ, source, "    %s", buf);
	    }
	notice (s_NickServ, source, INFO_END_OF_LIST,
					nnicks > max ? max : nnicks, nnicks);
	if (show_sync_on==TRUE)
	    notice(s_NickServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
}

static void
do_listoper (const char *source)
{
    char *pattern = strtok (NULL, " ");
    char *cmax = strtok (NULL, " ");
    NickInfo *ni;
    int nnicks, i, max = 50;
    char buf[BUFSIZE];

    if (!pattern)
    {
	notice (s_NickServ, source, "Syntax: \2LISTOPER \37pattern\37\2 [\37max\37]");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "LISTOPER");
    }
    else
    {
	if (cmax && atoi (cmax) > 0)
	    max = atoi (cmax);
	nnicks = 0;
	notice (s_NickServ, source, INFO_LIST_MATCH, pattern);
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next)
	    {
		if (ni->flags & NI_SLAVE)
		    continue;
		if (!(host (ni)->flags & NI_IRCOP))
		    continue;
		if (strlen (ni->nick) + strlen (ni->last_usermask) > sizeof (buf))
		    continue;
		if (strlen (ni->last_usermask) > 0)
		    snprintf (buf, sizeof (buf), "%-20s  %s", ni->nick, userisnick(ni->nick) ? "<< ONLINE >>" : ni->last_usermask);
		else
		    snprintf (buf, sizeof (buf), "%-20s", ni->nick);
		if (match_wild_nocase (pattern, buf))
		    if (++nnicks <= max)
			notice (s_NickServ, source, "    %s", buf);
	    }
	notice (s_NickServ, source, INFO_END_OF_LIST,
					nnicks > max ? max : nnicks, nnicks);
	if (show_sync_on==TRUE)
	    notice(s_NickServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
}

/*************************************************************************/

static void
do_recover (const char *source)
{
    char *nick = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");
    NickInfo *ni;
    User *u = finduser (source);

    if (!nick)
    {
	notice (s_NickServ, source,
		"Syntax: \2RECOVER \37nickname\37 [\37password\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "RECOVER");

    }
    else if (!(ni = findnick (nick)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    else if (!finduser (nick))
	notice (s_NickServ, source, NS_NOT_IN_USE, nick);

    else if (!u)
    {
	write_log ("%s: RECOVER: source user %s not found!", s_NickServ, source);
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (stricmp (nick, source) == 0)
	notice (s_NickServ, source, ERR_NOT_ON_YOURSELF, "RECOVER");

    else if (pass ? strcmp (pass, host (ni)->pass) == 0 : is_on_access (u, ni))
    {
	collide (ni);
	S_nick_recover++;
	notice (s_NickServ, source, NS_KILLED_IMPOSTER);
	notice (s_NickServ, source, NS_RELEASE, s_NickServ, nick);
    }
    else
    {
	notice (s_NickServ, source,
		pass ? ERR_WRONG_PASSWORD : ERR_ACCESS_DENIED);
	if (pass)
	    write_log ("%s: RECOVER: invalid password for %s by %s!%s@%s",
		       s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void
do_release (const char *source)
{
    char *nick = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");
    NickInfo *ni;
    User *u = finduser (source);

    if (!nick)
    {
	notice (s_NickServ, source,
		"Syntax: \2RELEASE \37nickname\37 [\37password\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "RELEASE");

    }
    else if (!(ni = findnick (nick)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    else if (!(ni->flags & NI_KILL_HELD))
	notice (s_NickServ, source, NS_NOT_IN_USE, nick);

    else if (!u)
    {
	write_log ("%s: RELEASE: source user %s not found!", s_NickServ, source);
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (pass ? strcmp (pass, host (ni)->pass) == 0 : is_on_access (u, ni))
    {
	release (ni);
	S_nick_release++;
	notice (s_NickServ, source, NS_KILLED_IMPOSTER);
    }
    else
    {
	notice (s_NickServ, source,
		pass ? ERR_WRONG_PASSWORD : ERR_ACCESS_DENIED);
	if (pass)
	    write_log ("%s: RELEASE: invalid password for %s by %s!%s@%s",
		       s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void
do_ghost (const char *source)
{
    char *nick = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");
    NickInfo *ni;
    User *u = finduser (source);

    if (!nick)
    {
	notice (s_NickServ, source,
		"Syntax: \2GHOST \37nickname\37 [\37password\37]\2");
	notice (s_NickServ, source, ERR_MORE_INFO, s_NickServ, "GHOST");

    }
    else if (!(ni = findnick (nick)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    else if (!finduser (nick))
	notice (s_NickServ, source, NS_NOT_IN_USE, nick);

    else if (!u)
    {
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);
	write_log ("%s: GHOST: source user %s not found!", s_NickServ, source);

    }
    else if (stricmp (nick, source) == 0)
	notice (s_NickServ, source, ERR_NOT_ON_YOURSELF, "GHOST");

    else if (pass ? strcmp (pass, host (ni)->pass) == 0 : is_on_access (u, ni))
    {
	char buf[NICKMAX + 25];
	snprintf (buf, sizeof (buf), "%s (%s)", source, NS_GHOST_KILL);
	kill_user (s_NickServ, ni->nick, buf);
	S_nick_ghost++;
	notice (s_NickServ, source, NS_KILLED_IMPOSTER);

    }
    else
    {
	notice (s_NickServ, source,
		pass ? ERR_WRONG_PASSWORD : ERR_ACCESS_DENIED);
	if (pass)
	    write_log ("%s: GHOST: invalid password for %s by %s!%s@%s",
		       s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void
do_getpass (const char *source)
{
    char *nick = strtok (NULL, " ");
    NickInfo *ni, *hni;
    User *u = finduser (source);

    if (!nick)
	notice (s_NickServ, source, "Syntax: \2GETPASS \37nickname\37\2");

    else if (!u)
	notice (s_NickServ, source, ERR_YOU_DONT_EXIST);

    else if (!(ni = findnick (nick)) || !(hni = host (ni)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);

    else
    {
	write_log ("%s: %s!%s@%s used GETPASS on %s (%s)",
	     s_NickServ, source, u->username, u->host, ni->nick, hni->nick);
	wallops (s_NickServ, MULTI_GETPASS_WALLOP, source, ni->nick, hni->nick);
	notice (s_NickServ, source, MULTI_GETPASS, ni->nick, hni->nick, hni->pass);
	S_nick_getpass++;
    }
}


/*************************************************************************/

static void
do_forbid (const char *source)
{
    NickInfo *ni;
    char *nick = strtok (NULL, " ");

    if (!nick)
    {
	notice (s_NickServ, source, "Syntax: \2FORBID \37nickname\37\2");
	return;
    }
    if (ni = findnick (nick))
	if (getflags (ni) & NI_IRCOP)
	{
	    notice (s_NickServ, source, ERR_NOT_ON_IRCOP, "FORBID");
	    return;
	}
	else
	{
	    int i = delnick (ni) - 1;
	    wallops (s_NickServ, NS_FORBID_WALLOP, source, nick, myitoa (i));
	}
    if (ni = makenick (nick))
    {
	ni->email = NULL;
	ni->url = NULL;
	ni->last_usermask = sstrdup ("");
	ni->last_realname = sstrdup ("");
	ni->flags |= NI_VERBOTEN;
	S_nick_forbid++;
	write_log ("%s: %s set FORBID for nick %s", s_NickServ, source, nick);
	notice (s_NickServ, source, MULTI_FORBID, nick);
	if (services_level != 1)
	    notice (s_NickServ, source, ERR_READ_ONLY);
    }
    else
    {
	write_log ("%s: Valid FORBID for %s by %s failed", s_NickServ,
		   nick, source);
	notice (s_NickServ, source, ERR_FAILED, "FORBID");
    }
}

/*************************************************************************/

static void
do_suspend (const char *source)
{
    NickInfo *ni, *hni;
    char *nick = strtok (NULL, " ");
    char *reason = strtok (NULL, "");

    if (!reason)
    {
	notice (s_NickServ, source, "Syntax: \2SUSPEND \37nickname reason\37\2");
	return;
    }
    if (!(ni = findnick (nick)) || !(hni = host (ni)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);
    else if (hni->flags & NI_IRCOP)
	notice (s_NickServ, source, ERR_NOT_ON_IRCOP, "SUSPEND");
    else if (hni->flags & NI_SUSPENDED)
	notice (s_NickServ, source, NS_IS_SUSPENDED, ni->nick);
    else
    {
	int i;
	hni->flags |= NI_SUSPENDED;
	hni->flags &= ~NI_IDENTIFIED;
	for (i = slavecount (hni->nick); i; i--)
	    slave (hni->nick, i)->flags &= ~NI_IDENTIFIED;
	if (hni->last_usermask)
	    free (hni->last_usermask);
	hni->last_usermask = smalloc (strlen (reason) + 2);
	snprintf (hni->last_usermask, strlen (reason) + 2, "%s", reason);
	S_nick_suspend++;
	write_log ("%s: %s set SUSPEND for nick %s (%s) because of %s", s_NickServ,
		   source, hni->nick, ni->nick, reason);
	notice (s_NickServ, source, MULTI_SUSPEND, ni->nick, hni->nick);
	if (services_level != 1)
	    notice (s_NickServ, source, ERR_READ_ONLY);
    }
}

static void
do_unsuspend (const char *source)
{
    NickInfo *ni, *hni;
    char *nick = strtok (NULL, " ");

    if (!nick)
    {
	notice (s_NickServ, source, "Syntax: \2UNSUSPEND \37nickname\37\2");
	return;
    }
    if (!(ni = findnick (nick)) || !(hni = host (ni)))
	notice (s_NickServ, source, NS_NOT_REGISTERED, nick);
    else if (!(hni->flags & NI_SUSPENDED))
	notice (s_NickServ, source, NS_IS_NOT_SUSPENDED, ni->nick);
    else
    {
	hni->flags &= ~NI_SUSPENDED;
	if (hni->last_usermask)
	    free (hni->last_usermask);
	hni->last_usermask = sstrdup("");
	S_nick_unsuspend++;
	write_log ("%s: %s removed SUSPEND for nick %s (%s)", s_NickServ, source,
		   hni->nick, ni->nick);
	notice (s_NickServ, source, MULTI_UNSUSPEND, ni->nick, hni->nick);
	if (services_level != 1)
	    notice (s_NickServ, source, ERR_READ_ONLY);
    }
}

static void
do_status (const char *source)
{
    char status[256];
    NickInfo *ni = findnick(source);
    User *u = finduser(source);

    if (ni)
    {
	strcpy(status, "Registered : ");
	if (ni->flags & NI_RECOGNIZED)
	    strcat(status, "Recognized : ");
	if (ni->flags & NI_IDENTIFIED)
	    strcat(status, "Identified : ");
    }
    else
	strcpy(status, "Unregistered : ");
    if (ni && host (ni)->flags & NI_IRCOP && !is_oper(source))
	strcat(status, "Just IRC Operator : ");
    else if (ni && host (ni)->flags & NI_IRCOP && is_oper(source))
	strcat(status, "IRC Operator : ");
    else if (is_oper(source))
	strcat(status, "IRC Operator (UNFLAGGED) : ");
    if (is_justservices_op(source))
	if (is_services_op(source))
	    strcat(status, "Services Operator : ");
	else
	    strcat(status, "Just Services Operator : ");
    if (is_justservices_admin(source))
	if (is_services_admin(source))
	    strcat(status, "Services Admin : ");
	else
	    strcat(status, "Just Services Admin : ");
    strcat (status, time_ago(u->signon, 0));

    notice(s_NickServ, source, "%s", status);
}

/*************************************************************************/
