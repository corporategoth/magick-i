/* NickServ functions.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#ifdef NICKSERV
NickInfo *nicklists[256];	/* One for each initial character */

const char s_NickServ[] = NICKSERV_NAME;
#include "ns-help.c"
Timeout *timeouts = NULL;

static int is_on_access(User *u, NickInfo *ni);
static void alpha_insert_nick(NickInfo *ni);
static NickInfo *makenick(const char *nick);
static void collide(NickInfo *ni);
static void release(NickInfo *ni);
static void add_timeout(NickInfo *ni, int type, time_t delay);
static void del_timeout(NickInfo *ni, int type);
static void do_help(const char *source);
static void do_register(const char *source);
static void do_link(const char *source);
static void do_identify(const char *source);
static void do_drop(const char *source);
static void do_slavedrop(const char *source);
static void do_set(const char *source);
static void do_set_password(NickInfo *ni, char *param);
#if FILE_VERSION > 2
    static void do_set_email(NickInfo *ni, char *param);
    static void do_set_url(NickInfo *ni, char *param);
#endif
static void do_set_kill(NickInfo *ni, char *param);
static void do_set_private(NickInfo *ni, char *param);
static void do_set_secure(NickInfo *ni, char *param);
static void do_set_ircop(NickInfo *ni, char *param);
static void do_set_privmsg(NickInfo *ni, char *param);
static void do_access(const char *source);
static int do_access_help (const char *source, char *cmd, char *nick);
static void do_access_add(const char *source);
static void do_access_del(const char *source);
static void do_access_list(const char *source);
#if (FILE_VERSION > 3) && defined(MEMOS)
    static void do_ignore(const char *source);
    static int do_ignore_help (const char *source, char *cmd, char *nick);
    static void do_ignore_add(const char *source);
    static void do_ignore_del(const char *source);
    static void do_ignore_list(const char *source);
    int is_on_ignore(const char *source, char *target);
#endif
static void do_deop(const char *source);
static void do_info(const char *source);
static void do_list(const char *source);
static void do_listoper(const char *source);
static void do_recover(const char *source);
static void do_release(const char *source);
static void do_ghost(const char *source);
static void do_getpass(const char *source);
static void do_forbid(const char *source);
static void do_suspend(const char *source);
static void do_unsuspend(const char *source);

/*************************************************************************/

/* Display total number of registered nicks and info about each; or, if
 * a specific nick is given, display information about that nick (like
 * /msg NickServ INFO <nick>).  If count_only != 0, then only display the
 * number of registered nicks (the nick parameter is ignored).
 */

void listnicks(int count_only, const char *nick)
{
    long count = 0;
    NickInfo *ni;
    int i;

    if (count_only) {
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next)
		++count;
	printf("%d nicknames registered.\n", count);

    } else if (nick) {
	char buf[BUFSIZE];
	long flags;

	if (!(ni = findnick(nick))) {
	    printf("%s not registered.\n", nick);
	    return;
	} else if (ni->flags & NI_VERBOTEN) {
	    printf("%s is FORBIDden.\n", ni->nick);
	    return;
	}
	flags = getflags(ni);
	printf("%s is %s\n", ni->nick, ni->last_realname);
	if(flags & NI_SLAVE)
		printf("        Host Nick: %s\n", ni->last_usermask);
#if FILE_VERSION > 2
	if(strlen(host(ni)->email)>0)
		printf("   E-Mail address: %s\n", host(ni)->email);
	if(strlen(host(ni)->url)>0)
		printf("   WWW Page (URL): %s\n", host(ni)->url);
#endif
        if (flags & NI_SUSPENDED)
		printf("    Suspended For: %s\n", host(ni)->last_usermask);
	else
		printf("Last seen address: %s\n", host(ni)->last_usermask);
	printf("       Registered: %s ago\n", time_ago(ni->time_registered, 1));
	if (!userisnick(ni->nick))
	    printf("        Last seen: %s ago\n", time_ago(ni->last_seen, 1));
	*buf = 0;
        if (flags & NI_SUSPENDED)
	    strcat(buf, "\2SUSPENDED USER\2");
	else {
	    if (flags & NI_KILLPROTECT)
		strcat(buf, "Kill protection");
	    if (flags & NI_SECURE) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "Security");
	    }
	    if (flags & NI_PRIVATE) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "Private");
	    }
	    if (flags & NI_IRCOP) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "IRC ");
		if (is_justservices_op(nick))
		    strcat(buf, "S-");
		strcat(buf, "Operator");
	    }
	    if (!*buf)
		strscpy(buf, "None", BUFSIZE);
	}
	printf("          Options: %s\n", buf);
#if (FILE_VERSION > 3) && defined(MEMOS)
        if (flags & NI_SUSPENDED)
	    printf("NOTE: Cannot send memos to a suspended user.\n");
#endif
	if (userisnick(ni->nick))
	    printf("User is currently online.\n");

    } else {
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next) {
		printf("    %-20s  %s\n", ni->nick,
			  ni->flags & NI_VERBOTEN  ? "Disallowed (FORBID)"
			: getflags(ni) & NI_SUSPENDED ? "Disallowed (SUSPEND)"
			: host(ni)->last_usermask);
		++count;
	    }
	printf("%d nicknames registered.\n", count);
    }
}

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

void get_nickserv_stats(long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    NickInfo *ni;
    char **accptr;

    for (i = 0; i < 256; i++)
	for (ni = nicklists[i]; ni; ni = ni->next) {
	    count++;
	    mem += sizeof(*ni);
	    if (ni->last_usermask)
		mem += strlen(ni->last_usermask)+1;
	    if (ni->last_realname)
		mem += strlen(ni->last_realname)+1;
	    mem += sizeof(char *) * ni->accesscount;
	    for (accptr=ni->access, j=0; j < ni->accesscount; accptr++, j++)
		if (*accptr)
		    mem += strlen(*accptr)+1;
	}
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/
/*************************************************************************/

/* Main NickServ routine. */

void nickserv(const char *source, char *buf)
{
    char *cmd, *s;

    cmd = strtok(buf, " ");

    if (!cmd)
	return;

    else if (stricmp(cmd, "\1PING")==0) {
	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_NickServ, source, "\1PING %s", s);

    } else {
	Hash *command, hash_table[] = {
		{ "HELP",	H_NONE,	do_help },
		{ "REG*",	H_NONE,	do_register },
		{ "*LINK*",	H_NONE,	do_link },
		{ "ID*",	H_NONE,	do_identify },
		{ "S*DROP",	H_NONE,	do_slavedrop },
		{ "DROP*S",	H_NONE,	do_slavedrop },
		{ "DROP*",	H_NONE,	do_drop },
		{ "SET*",	H_NONE,	do_set },
		{ "ACC*",	H_NONE,	do_access },
		{ "INFO",	H_NONE,	do_info },
		{ "CHANINFO",	H_NONE,	do_info },
		{ "LIST",	H_NONE,	do_list },
		{ "U*LIST",	H_NONE,	do_list },
		{ "LISTU*",	H_NONE,	do_list },
		{ "REC*",	H_NONE,	do_recover },
		{ "REL*",	H_NONE,	do_release },
		{ "GHOST",	H_NONE,	do_ghost },
		{ "LISTO*",	H_OPER,	do_listoper },
		{ "O*LIST",	H_OPER,	do_listoper },
		{ "GETPASS",	H_SOP,	do_getpass },
		{ "FORBID",	H_SOP,	do_forbid },
		{ "SUSPEND",	H_SOP,	do_suspend },
		{ "UNSUSPEND",	H_SOP,	do_unsuspend },
		{ "DEOP*",	H_SOP,	do_deop },
#if (FILE_VERSION > 3) && defined(MEMOS)
		{ "IGN*",	H_NONE,	do_ignore },
#endif
		{ NULL }
	};

	if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
	else
	    notice(s_NickServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_NickServ);
    }
}

/*************************************************************************/

/* Load/save data files. */

void load_ns_dbase(void)
{
    FILE *f = fopen(NICKSERV_DB, "r");
    int i, j, len;
    NickInfo *ni;

    if (!f) {
	log_perror("Can't read NickServ database " NICKSERV_DB);
	return;
    }
    switch (i = get_file_version(f, NICKSERV_DB)) {
      case 4:
#if FILE_VERSION > 3
	for (i = 33; i < 256; ++i)
	    while (fgetc(f) == 1) {
		ni = smalloc(sizeof(NickInfo));
		if (1 != fread(ni, sizeof(NickInfo), 1, f))
		    fatal_perror("Read error on " NICKSERV_DB);
		ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
		alpha_insert_nick(ni);
		ni->email = read_string(f, NICKSERV_DB);
		ni->url = read_string(f, NICKSERV_DB);
		ni->last_usermask = read_string(f, NICKSERV_DB);
		ni->last_realname = read_string(f, NICKSERV_DB);
		if (ni->accesscount) {
		    char **access;
		    access = smalloc(sizeof(char *) * ni->accesscount);
		    ni->access = access;
		    for (j = 0; j < ni->accesscount; ++j, ++access)
			*access = read_string(f, NICKSERV_DB);
		}
		if (ni->ignorecount) {
		    char **ignore;
		    ignore = smalloc(sizeof(char *) * ni->ignorecount);
		    ni->ignore = ignore;
		    for (j = 0; j < ni->ignorecount; ++j, ++ignore)
			*ignore = read_string(f, NICKSERV_DB);
		}
	    }
	break;
#endif
      case 3:
#if FILE_VERSION > 2
	for (i = 33; i < 256; ++i)
	    while (fgetc(f) == 1) {
		ni = smalloc(sizeof(NickInfo));
		if (1 != fread(ni, sizeof(NickInfo), 1, f))
		    fatal_perror("Read error on " NICKSERV_DB);
		ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
		alpha_insert_nick(ni);
		ni->email = read_string(f, NICKSERV_DB);
		ni->url = read_string(f, NICKSERV_DB);
		ni->last_usermask = read_string(f, NICKSERV_DB);
		ni->last_realname = read_string(f, NICKSERV_DB);
		if (ni->accesscount) {
		    char **access;
		    access = smalloc(sizeof(char *) * ni->accesscount);
		    ni->access = access;
		    for (j = 0; j < ni->accesscount; ++j, ++access)
			*access = read_string(f, NICKSERV_DB);
		}
	    }
	break;
#endif
      case 2:
      case 1:
	for (i = 33; i < 256; ++i)
	    while (fgetc(f) == 1) {
		ni = smalloc(sizeof(NickInfo));
		if (1 != fread(ni, sizeof(NickInfo), 1, f))
		    fatal_perror("Read error on " NICKSERV_DB);
		ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
		alpha_insert_nick(ni);
#if FILE_VERSION > 2
            ni->email = sstrdup("");
            ni->url = sstrdup("");
#endif
		ni->last_usermask = read_string(f, NICKSERV_DB);
		ni->last_realname = read_string(f, NICKSERV_DB);
		if (ni->accesscount) {
		    char **access;
		    access = smalloc(sizeof(char *) * ni->accesscount);
		    ni->access = access;
		    for (j = 0; j < ni->accesscount; ++j, ++access)
			*access = read_string(f, NICKSERV_DB);
		}
	    }
	break;
      default:
        fatal("Unsupported version number (%d) on %s", i, NICKSERV_DB);
    } /* switch (version) */
    fclose(f);
}

void save_ns_dbase(void)
{
    FILE *f;
    int i, j, len;
    NickInfo *ni;
    char **access;
    char **ignore;

    remove(NICKSERV_DB ".save");
    if (rename(NICKSERV_DB, NICKSERV_DB ".save") < 0)
	log_perror("Can't back up " NICKSERV_DB);
    f = fopen(NICKSERV_DB, "w");
    if (!f) {
	log_perror("Can't write to NickServ database " NICKSERV_DB);
	if (rename(NICKSERV_DB ".save", NICKSERV_DB) < 0)
	    fatal_perror("Can't restore backup of " NICKSERV_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't change group of %s to %d", NICKSERV_DB, file_gid);
    write_file_version(f, NICKSERV_DB);

    for (i = 33; i < 256; ++i) {
	for (ni = nicklists[i]; ni; ni = ni->next) {
	    fputc(1, f);
	    if (1 != fwrite(ni, sizeof(NickInfo), 1, f))
		fatal_perror("Write error on " NICKSERV_DB);
#if FILE_VERSION > 2
	    write_string(ni->email ? ni->email : "",
							f, NICKSERV_DB);
	    write_string(ni->url ? ni->url : "",
							f, NICKSERV_DB);
#endif
	    write_string(ni->last_usermask ? ni->last_usermask : "",
							f, NICKSERV_DB);
	    write_string(ni->last_realname ? ni->last_realname : "",
							f, NICKSERV_DB);
	    for (access = ni->access, j = 0; j < ni->accesscount; ++access, ++j)
		write_string(*access, f, NICKSERV_DB);
#if FILE_VERSION > 3
	    for (ignore = ni->ignore, j = 0; j < ni->ignorecount; ++ignore, ++j)
		write_string(*ignore, f, NICKSERV_DB);
#endif
	}
	fputc(0, f);
    }
    fclose(f);
    remove(NICKSERV_DB ".save");
}

/*************************************************************************/

/* Check whether a user is on the access list of the nick they're using.
 * If not, send warnings as appropriate.  If so (and not NI_SECURE), update
 * last seen info.  Return 1 if the user is valid and recognized, 0
 * otherwise (note that this means an NI_SECURE nick will always return 0
 * from here). */

int validate_user(User *u)
{
    NickInfo *ni, *hni;
    int on_access;

    if (!(ni = findnick(u->nick)) || !(hni = host(ni)))
	return;

    if (ni->flags & NI_VERBOTEN) {
	notice(s_NickServ, u->nick,
		"This nickname may not be used.  Please choose another one.");
	notice(s_NickServ, u->nick,
		"If you do not change within one minute, you will be disconnected.");
	add_timeout(ni, TO_COLLIDE, 60);
	return 0;
    }

    on_access = is_on_access(u, ni);

    if (!(hni->flags & NI_SECURE) && on_access) {
	ni->flags |= NI_RECOGNIZED;
	ni->last_seen = hni->last_seen = time(NULL);
        if (!(hni->flags & NI_SUSPENDED)) {
	    if (hni->last_usermask)
		free(hni->last_usermask);
	    hni->last_usermask = smalloc(strlen(u->nick)+strlen(u->username)+strlen(u->host)+2);
	    sprintf(hni->last_usermask, "%s!%s@%s", u->nick, u->username, u->host);
	}
	if (ni->last_realname)
	    free(ni->last_realname);
	ni->last_realname = sstrdup(u->realname);
#ifdef DAL_SERV
	send_cmd(s_NickServ, "SVSMODE %s +r", u->nick);
#endif
	return 1;
    }

    if (hni->flags & NI_SECURE) {
	notice(s_NickServ, u->nick,
		"This nickname is registered and protected.  If it is your");
	notice(s_NickServ, u->nick,
		"nick, type \2/msg %s IDENTIFY \37password\37\2.  Otherwise,",
		s_NickServ);
	notice(s_NickServ, u->nick,
		"please choose a different nick.");
    } else {
	notice(s_NickServ, u->nick,
		"This nick is owned by someone else.  Please choose another.");
	notice(s_NickServ, u->nick,
		"(If this is your nick, type \2/msg %s IDENTIFY \37password\37\2.)",
		s_NickServ);
    }
    if ((hni->flags & NI_KILLPROTECT)
		&& !((hni->flags & NI_SECURE) && on_access)) {
	notice(s_NickServ, u->nick,
		"If you do not change within one minute, you will be disconnected.");
	add_timeout(ni, TO_COLLIDE, 60);
    }

    return 0;
}

/*************************************************************************/

/* Cancel validation flags for a nick (i.e. when the user with that nick
 * signs off or changes nicks).  Also cancels any impending collide. */

void cancel_user(User *u)
{
    NickInfo *ni;

    if (ni = findnick(u->nick)) {
	ni->flags &= ~(NI_IDENTIFIED | NI_RECOGNIZED);
	del_timeout(ni, TO_COLLIDE);
	ni->last_seen = host(ni)->last_seen = time(NULL);        
    }
}

/*************************************************************************/

/* Check the timeout list for any pending actions. */

void check_timeouts()
{
    Timeout *to, *to2;
    time_t t = time(NULL);
    User *u;

    to = timeouts;
    while (to) {
	if (t < to->timeout) {
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
	switch (to->type) {
	  case TO_COLLIDE:
	    /* If they identified or don't exist anymore, don't kill them. */
	    if (to->ni->flags & NI_IDENTIFIED
			|| !(u = finduser(to->ni->nick))
			|| u->my_signon > to->settime)
		break;
	    /* The RELEASE timeout will always add to the beginning of the
	     * list, so we won't see it.  Which is fine because it can't be
	     * triggered yet anyway. */
	    collide(to->ni);
	    break;
	  case TO_RELEASE:
	    release(to->ni);
	    break;
	  default:
	    log("%s: Unknown timeout type %d for nick %s", s_NickServ,
						to->type, to->ni->nick);
	}
	free(to);
	to = to2;
    }
}

/*************************************************************************/

/* Remove all nicks which have expired. */

void expire_nicks()
{
    NickInfo *ni, *ni2;
    int i;
    const time_t expire_time = NICK_EXPIRE*24*60*60;
    time_t now = time(NULL);

    for (i = 33; i < 256; ++i) {
	ni = nicklists[i];
	while (ni) {
	    if (now - ni->last_seen >= expire_time && !slavecount(ni->nick)
				&& !(ni->flags & (NI_VERBOTEN | NI_IRCOP))) {
		ni2 = ni->next;
		log("Expiring nickname %s", ni->nick);
		delnick(ni);
		ni = ni2;
	    } else
		ni = ni->next;
	}
    }
}

/*************************************************************************/

/* Return the NickInfo structure for the given nick, or NULL if the nick
 * isn't registered. */

NickInfo *findnick(const char *nick)
{
    NickInfo *ni;

    for (ni = nicklists[tolower(*nick)]; ni; ni = ni->next)
	if (stricmp(ni->nick, nick)==0)
	    return ni;
    return NULL;
}

NickInfo *host(NickInfo *ni)
{
    if (!ni) return NULL;
    if (ni->flags & NI_SLAVE) return findnick(ni->last_usermask);
    else return ni;
}

NickInfo *slave(const char *nick, int num)
{
    NickInfo *ni;
    int i, cnt = 0;

    for (i = 33; i < 256; ++i)
	for (ni = nicklists[i]; ni; ni = ni->next) {
	    if (stricmp(ni->last_usermask, nick)==0) cnt++;
	    if (cnt==num) return ni;
	}
    return NULL;
}

int issibling(NickInfo *ni, const char *target)
{
    if (ni) {
	NickInfo *hni = host(ni);
	int i;
	if (stricmp(ni->nick, target)==0) return 1;
	if (stricmp(hni->nick, target)==0) return 1;
	for (i=slavecount(hni->nick); i; i--)
	    if (stricmp(target, slave(hni->nick, i)->nick)==0) return 1;
    }
    return 0;
}

int slavecount(const char *nick)
{
    NickInfo *ni;
    int i, cnt = 0;

    for (i = 33; i < 256; ++i)
	for (ni = nicklists[i]; ni; ni = ni->next)
	    if (stricmp(ni->last_usermask, nick)==0) cnt++;
    return cnt;
}

int userisnick (const char *nick)
{
    NickInfo *ni = findnick(nick);
    if (!finduser(nick) || !ni) return 0;
    if ((getflags(ni) & NI_SECURE) ? (ni->flags & NI_IDENTIFIED)
		: (ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	return 1;
    return 0;
}

int slaveonline (const char *nick)
{
    int i = slavecount(nick);

    for (i=slavecount(nick); i; i--)
	if (userisnick(slave(nick, i)->nick))
	    return 1;
    return 0;
}

/* Find flags (assume host's flags if there is one) */
long getflags (NickInfo *ni)
{
    NickInfo *hni = host(ni);
    long flags;

    if (hni) {
	flags = hni->flags;
	flags &= ~NI_RECOGNIZED;
	flags &= ~NI_IDENTIFIED;
	flags &= ~NI_KILL_HELD;
	if (ni->flags & NI_RECOGNIZED)	flags |= NI_RECOGNIZED;
	if (ni->flags & NI_IDENTIFIED)	flags |= NI_IDENTIFIED;
	if (ni->flags & NI_KILL_HELD)	flags |= NI_KILL_HELD;
	if (ni->flags & NI_SLAVE)	flags |= NI_SLAVE;
	return flags;
    }
    return 0;
}

/*************************************************************************/
/*********************** NickServ private routines ***********************/
/*************************************************************************/

/* Is the given user's address on the given nick's access list?  Return 1
 * if so, 0 if not. */

static int is_on_access(User *u, NickInfo *ni)
{
    int i;
    char *buf;
    NickInfo *hni = host(ni);

    i = strlen(u->username);
    buf = smalloc(i + strlen(u->host) + 2);
    sprintf(buf, "%s@%s", u->username, u->host);
    strlower(buf+i+1);
    for (i = 0; i < hni->accesscount; ++i)
	if (match_wild_nocase(hni->access[i], buf)) {
	    free(buf);
	    return 1;
	}
    free(buf);
    return 0;
}

/* Is the given user's nick on the given nick's ignore list?  Return 1
 * if so, 0 if not. */

#if (FILE_VERSION > 3) && defined(MEMOS)
int is_on_ignore(const char *source, char *target)
{
    int i;
    NickInfo *ni, *hni = host(findnick(source));
    char **ignore, mynick[NICKMAX];
    
    if (hni)
	strscpy(mynick, hni->nick, NICKMAX);
    else
	strscpy(mynick, source, NICKMAX);
    
    if (ni = host(findnick(target))) {
        if (ni->flags & NI_SUSPENDED)
        	return 1;
	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
	    if (stricmp(*ignore, mynick)==0 || stricmp(*ignore, source)==0)
		return 1;
    }
    return 0;
}
#endif

/*************************************************************************/

/* Insert a nick alphabetically into the database. */

static void alpha_insert_nick(NickInfo *ni)
{
    NickInfo *ni2, *ni3;
    char *nick = ni->nick;

    for (ni3 = NULL, ni2 = nicklists[tolower(*nick)];
			ni2 && stricmp(ni2->nick, nick) < 0;
			ni3 = ni2, ni2 = ni2->next)
	;
    ni->prev = ni3;
    ni->next = ni2;
    if (!ni3)
	nicklists[tolower(*nick)] = ni;
    else
	ni3->next = ni;
    if (ni2)
	ni2->prev = ni;
}

/*************************************************************************/

/* Add a nick to the database.  Returns a pointer to the new NickInfo
 * structure if the nick was successfully registered, NULL otherwise.
 * Assumes nick does not already exist. */

static NickInfo *makenick(const char *nick)
{
    NickInfo *ni;
    User *u;

    ni = scalloc(sizeof(NickInfo), 1);
    strscpy(ni->nick, nick, NICKMAX);
    alpha_insert_nick(ni);
    return ni;
}

/*************************************************************************/

/* Remove a nick from the NickServ database.  Return 1 on success, 0
 * otherwise. */

int delnick(NickInfo *ni)
{
    int i, sc, cnt=1;
    NickInfo *hni = host(ni);
#ifdef CHANSERV
    ChannelInfo *ci, *ci2;
    ChanAccess *access;
    int j;
#endif

/* For slave nicks (they have to be del'd last) */
    for (i = slavecount(ni->nick); i; --i)
	cnt += delnick(slave(ni->nick, 1));

/* Delete the user's MEMOS  */
#ifdef MEMOS
    if (services_level==1) {
	MemoList *ml;
	if (ml = find_memolist(ni->nick))
	    del_memolist(ml);
    }
#endif /* MEMOS */

/* Delete the user from channel access lists
 * and delete any channels this user owns */
#ifdef CHANSERV
    for (i = 33; i < 256; ++i) {
	ci = chanlists[i];
	while (ci)
	    if (stricmp(ci->founder, ni->nick)==0)
		if (stricmp(ni->nick, hni->nick)==0) {
		    ci2 = ci->next;
		    delchan(ci);
		    ci = ci2;
		} else {
		    strscpy(ci->founder, hni->nick, NICKMAX);
		    ci = ci->next;
		}
	    else {
		for (access = ci->access, j = 0; access && j < ci->accesscount;
							access++, j++)
		    if (access->is_nick>0 && stricmp(access->name, ni->nick)==0)
			if (stricmp(ni->nick, hni->nick)==0) {
			    free(access->name);
			    access->is_nick = -1;
			} else {
			    free(access->name);
			    access->name = sstrdup(hni->nick);
			}
		ci = ci->next;
	    }
    }
#endif

/* Delete from SOP list if on it */
     for (i = 0; i < nsop; ++i)
	if (stricmp(sops[i], ni->nick)==0) {
	    if (i < nsop)
		bcopy(sops+i+1, sops+i, sizeof(*sops) * (nsop-i));
	    --nsop;
	    break;
	}

    if (ni->next)
	ni->next->prev = ni->prev;
    if (ni->prev)
	ni->prev->next = ni->next;
    else
	nicklists[tolower(*ni->nick)] = ni->next;
#if FILE_VERSION > 2
    if (ni->email)
	free(ni->email);
    if (ni->url)
	free(ni->url);
#endif
    if (ni->last_usermask)
	free(ni->last_usermask);
    if (ni->last_realname)
	free(ni->last_realname);
    if (ni->access) {
	for (i = 0; i < ni->accesscount; ++i)
	    free(ni->access[i]);
	free(ni->access);
    }
#if (FILE_VERSION > 3) && defined(MEMOS)
    if (ni->ignore) {
	for (i = 0; i < ni->ignorecount; ++i)
	    free(ni->ignore[i]);
	free(ni->ignore);
    }
#endif
    free(ni);

    return cnt;
}

/*************************************************************************/

/* Collide a nick. */

static void collide(NickInfo *ni)
{
    char *av[2];
    char newnick[NICKMAX];
    int success;

    del_timeout(ni, TO_COLLIDE);
#ifdef DAL_SERV
    for(success=0, strscpy(newnick, ni->nick, NICKMAX);
		strlen(newnick)<NICKMAX && success!=1;) {
	snprintf(newnick, NICKMAX, "%s_", newnick);
	if(!(finduser(newnick)))
	    success=1;
    }
    if(success!=1)
	for(success=0, strscpy(newnick, ni->nick, NICKMAX);
		    strlen(newnick)<NICKMAX && success!=1;) {
	    snprintf(newnick, NICKMAX, "%s-", newnick);
	    if(!(finduser(newnick)))
		success=1;
	}
    if(success!=1)
	for(success=0, strscpy(newnick, ni->nick, NICKMAX);
		    strlen(newnick)<NICKMAX && success!=1;) {
	    snprintf(newnick, NICKMAX, "%s~", newnick);
	    if(!(finduser(newnick)))
		success=1;
	}

    if(success!=1)
	kill_user(s_NickServ, ni->nick, "Nick kill enforced");
    else {
	User *u;
	if (!(u = finduser(ni->nick))) {
	    log("user: NICK KILL from nonexistent nick %s", ni->nick);
	    return;
	}
	send_cmd(s_NickServ, "SVSNICK %s %s :%lu",
    		ni->nick, newnick, time(NULL));
	notice(s_NickServ, newnick,
		"Your nick has been forcibly changed (Nick protection enforced)");
	change_user_nick(u, newnick);
    }
#else
    kill_user(s_NickServ, ni->nick, "Nick kill enforced");
#endif
#if RELEASE_TIMEOUT > 0
    if (!finduser(any_service())) {
	send_cmd(NULL, "NICK %s %d 1 enforcer %s %s :%s Enforcement",
		ni->nick, time(NULL), services_host, server_name, s_NickServ);
	av[0] = ni->nick;
	av[1] = "nick kill";
	do_kill(s_NickServ, 1, av);
	ni->flags |= NI_KILL_HELD;
	add_timeout(ni, TO_RELEASE, RELEASE_TIMEOUT);
    }
#endif
}

/*************************************************************************/

/* Release hold on a nick. */

static void release(NickInfo *ni)
{
    del_timeout(ni, TO_RELEASE);
    send_cmd(ni->nick, "QUIT");
    ni->flags &= ~NI_KILL_HELD;
}

/*************************************************************************/

/* Add a timeout to the timeout list. */

static void add_timeout(NickInfo *ni, int type, time_t delay)
{
    Timeout *to;

    to = smalloc(sizeof(Timeout));
    to->next = timeouts;
    to->prev = NULL;
    if (timeouts)
	timeouts->prev = to;
    timeouts = to;
    to->ni = ni;
    to->type = type;
    to->settime = time(NULL);
    to->timeout = time(NULL) + delay;
}

/*************************************************************************/

/* Delete a timeout from the timeout list. */

static void del_timeout(NickInfo *ni, int type)
{
    Timeout *to, *to2;

    to = timeouts;
    while (to)
	if (to->ni == ni && to->type == type) {
	    to2 = to->next;
	    if (to->next)
		to->next->prev = to->prev;
	    if (to->prev)
		to->prev->next = to->next;
	    else
		timeouts = to->next;
	    free(to);
	    to = to2;
	} else
	    to = to->next;
}

/*************************************************************************/
/*********************** NickServ command routines ***********************/
/*************************************************************************/

/* Return a help message. */

static void do_help(const char *source)
{
    char *cmd = strtok(NULL, "");
    char buf[BUFSIZE];

    if (cmd) {
	Hash_HELP *command, hash_table[] = {
		{ "LISTOP*",	H_OPER,	listoper_help },
		{ "SET *OP*",	H_OPER,	set_ircop_help },
		{ "DROP",	H_SOP,	oper_drop_help },
		{ "GETPASS",	H_SOP,	getpass_help },
		{ "FORBID",	H_SOP,	forbid_help },
		{ "SUSPEND",	H_SOP,	suspend_help },
		{ "UNSUSPEND",	H_SOP,	unsuspend_help },
		{ "DEOP",	H_SOP,	deop_help },
		{ NULL }
	};

	if (command = get_help_hash(source, cmd, hash_table))
	    notice_list(s_NickServ, source, command->process);
	else {
		snprintf(buf, BUFSIZE, "%s%s", s_NickServ, cmd ? " " : "");
		strscpy(buf+strlen(buf), cmd ? cmd : "", sizeof(buf)-strlen(buf));
		helpserv(s_NickServ, source, buf);
	}
    } else {
	snprintf(buf, BUFSIZE, "%s%s", s_NickServ, cmd ? " " : "");
	strscpy(buf+strlen(buf), cmd ? cmd : "", sizeof(buf)-strlen(buf));
	helpserv(s_NickServ, source, buf);
    }
}

/*************************************************************************/

/* Register a nick. */

static void do_register(const char *source)
{
    NickInfo *ni;
    User *u;
    char *pass = strtok(NULL, " ");

    if(services_level!=1) {
	notice(s_NickServ, source,
		"Sorry, nickname registration is temporarily disabled.");
	return;
    }

    if (!pass) {
	notice(s_NickServ, source, "Syntax: \2REGISTER \37password\37\2");
	notice(s_NickServ, source,
			"\2/msg %s HELP REGISTER\2 for more information.",
			s_NickServ);

    } else if (!(u = finduser(source))) {
	log("%s: Can't register nick %s: nick not online", s_NickServ, source);
	notice(s_NickServ, source, "Sorry, registration failed.");

    } else if (is_services_nick(source)) {
	log("%s: %s@%s tried to register a SERVICES nick %s", s_NickServ,
			u->username, u->host, source);
	notice(s_NickServ, source,
			"Nickname \2%s\2 may not be registered.", source);

    } else if (ni = findnick(source))
	if (ni->flags & NI_VERBOTEN) {
	    log("%s: %s@%s tried to register FORBIDden nick %s", s_NickServ,
			u->username, u->host, source);
	    notice(s_NickServ, source,
			"Nickname \2%s\2 may not be registered.", source);
	} else
	    notice(s_NickServ, source,
			"Nickname \2%s\2 is already taken!", source);

    else if (stricmp(source, pass)==0 || strlen(pass) < 5) {
	notice(s_NickServ, source,
		"Please try again with a more obscure password.");
	notice(s_NickServ, source,
		"Passwords should be at least five characters long, should");
	notice(s_NickServ, source,
		"not be something easily guessed (e.g. your real name or your");
	notice(s_NickServ, source,
		"nick), and cannot contain the space character.");
	notice(s_NickServ, source,
		"\2/msg %s HELP REGISTER\2 for more information.");

    } else
	if (ni = makenick(source)) {
	    strscpy(ni->pass, pass, PASSMAX);
#if FILE_VERSION > 2
            ni->email = sstrdup("");
            ni->url = sstrdup("");
#endif
	    ni->last_usermask = smalloc(strlen(u->username)+strlen(u->host)+2);
	    sprintf(ni->last_usermask, "%s@%s", u->username, u->host);
	    ni->last_realname = sstrdup(u->realname);
	    ni->time_registered = ni->last_seen = time(NULL);
	    if (is_oper(source))
		ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_KILLPROTECT | NI_IRCOP;
	    else
		ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_KILLPROTECT;
	    ni->accesscount = 1;
	    ni->access = smalloc(sizeof(char *));
	    ni->access[0] = create_mask(u);
	    log("%s: `%s' registered by %s@%s", s_NickServ,
		    source, u->username, u->host);
	    notice(s_NickServ, source,
		    "Nickname %s registered under your account: %s",
		    source, ni->access[0]);
	    notice(s_NickServ, source,
		    "Your password is \2%s\2 - remember this for later use.",
		    pass);
	} else
	    notice(s_NickServ, source,
		"Sorry, couldn't register your nickname.");
}

/*************************************************************************/

/* Link a nick to a host */

static void do_link(const char *source)
{
    NickInfo *ni, *hni;
    User *u;
    char *hostnick = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");

    if(services_level!=1) {
	notice(s_NickServ, source,
		"Sorry, nickname linking is temporarily disabled.");
	return;
    }

    if (!pass) {
	notice(s_NickServ, source, "Syntax: \2LINK \37nick\37\2 \37password\37");
	notice(s_NickServ, source,
			"\2/msg %s HELP LINK\2 for more information.",
			s_NickServ);

    } else if (!(u = finduser(source))) {
	log("%s: Can't link nick %s: nick not online", s_NickServ, source);
	notice(s_NickServ, source, "Sorry, linking failed.");

    } else if (is_services_nick(source)) {
	log("%s: %s@%s tried to link a SERVICES nick %s", s_NickServ,
			u->username, u->host, source);
	notice(s_NickServ, source,
			"Nickname \2%s\2 may not be linked.", source);

    } else if (ni = findnick(source))
	if (ni->flags & NI_VERBOTEN) {
	    log("%s: %s@%s tried to link FORBIDden nick %s", s_NickServ,
			u->username, u->host, source);
	    notice(s_NickServ, source,
			"Nickname \2%s\2 may not be linked.", source);
	} else
	    notice(s_NickServ, source,
			"Nickname \2%s\2 is already taken!", source);

    else if (!(hni = findnick(hostnick)))
	notice(s_NickServ, source, "Nickname \2%s\2 is not registered",
								hostnick);

    else if (hni->flags & NI_SLAVE)
	notice(s_NickServ, source, "Cannot link to another link.");

    else if (strcmp(hni->pass, pass)!=0)
	notice(s_NickServ, source, "Password incorrect.");

    else
	if (ni = makenick(source)) {
	    strscpy(ni->pass, "", PASSMAX);
#if FILE_VERSION > 2
            ni->email = sstrdup("");
            ni->url = sstrdup("");
#endif
	    if (hni->last_usermask)
		free(hni->last_usermask);
	    hni->last_usermask = smalloc(strlen(u->nick)+strlen(u->username)+strlen(u->host)+3);
	    sprintf(hni->last_usermask, "%s!%s@%s", u->nick, u->username, u->host);
	    ni->last_usermask = sstrdup(hni->nick);
	    ni->last_realname = sstrdup(u->realname);
	    ni->time_registered = ni->last_seen = hni->last_seen = time(NULL);
	    ni->flags = NI_IDENTIFIED | NI_RECOGNIZED | NI_SLAVE;

	    log("%s: `%s' linked to %s", s_NickServ,
		    source, hni->nick);
	    notice(s_NickServ, source,
		    "Nickname %s is now linked to %s",
		    source, hni->nick);
	} else
	    notice(s_NickServ, source,
		"Sorry, couldn't link your nickname.");
}

/*************************************************************************/

static void do_identify(const char *source)
    { do_real_identify(s_NickServ, source); }

void do_real_identify(const char *whoami, const char *source)
{
    char *pass = strtok(NULL, " ");
    NickInfo *ni = findnick(source), *hni = host(ni);
    User *u;

    if (!pass) {
	notice(whoami, source, "Syntax: \2IDENTIFY \37password\37\2");
	notice(whoami, source,
		"\2/msg %s HELP IDENTIFY\2 for more information.",
		whoami);

    } else if (!ni)
	notice(whoami, source, "Your nick isn't registered.");

    else if (!(u = finduser(source))) {
	log("%s: IDENTIFY from nonexistent nick %s", whoami, source);
	notice(whoami, source, "Sorry, identification failed.");

    } else if (hni->flags & NI_SUSPENDED)
	notice(whoami, source,
		"Access Denied for SUSPENDED users.");

    else if (strcmp(pass, hni->pass) != 0) {
	log("%s: Failed IDENTIFY for %s!%s@%s",
		whoami, source, u->username, u->host);
	notice(whoami, source, "Password incorrect.");

    } else {
	ni->flags |= NI_IDENTIFIED;
	if (!(ni->flags & NI_RECOGNIZED)) {
	    ni->last_seen = hni->last_seen = time(NULL);
	    if (!(hni->flags & NI_SUSPENDED)) {
		if (hni->last_usermask)
		    free(hni->last_usermask);
		hni->last_usermask = smalloc(strlen(u->nick)+strlen(u->username)+strlen(u->host)+2);
		sprintf(hni->last_usermask, "%s!%s@%s", u->nick, u->username, u->host);
	    }
	    if (ni->last_realname)
		free(ni->last_realname);
	    ni->last_realname = sstrdup(u->realname);
	    log("%s: %s!%s@%s identified for nick %s", whoami,
			source, u->username, u->host, source);
	}
	notice(whoami, source,
		"Password accepted - you are now recognized.");
	if (is_oper(source) && !(hni->flags & NI_IRCOP)) {
	    notice(whoami, source,
		"You have not set the \2IRC Operator\2 flag for your nick.");
	    notice(whoami, source,
		"Please set this with \2/msg %s SET IRCOP ON\2.", s_NickServ);
	}
#ifdef DAL_SERV
	send_cmd(whoami, "SVSMODE %s +Rr", u->nick);
	if (is_services_op(source))
	    send_cmd(whoami, "SVSMODE %s +a", u->nick);
#endif
#ifdef MEMOS
	if (!(ni->flags & NI_RECOGNIZED) && services_level==1)
	    check_memos(source);
#endif
    }
}

/*************************************************************************/

static void do_drop(const char *source)
{
    char *nick = strtok(NULL, " ");
    NickInfo *ni;
    User *u = finduser(source);

    if (services_level!=1 && !is_services_op(source)) {
	notice(s_NickServ, source,
		"Sorry, nickname de-registration is temporarily disabled.");
	return;
    }

    if (!is_services_op(source) && nick) {
	notice(s_NickServ, source, "Syntax: \2DROP\2");
	notice(s_NickServ, source,
		"\2/msg %s DROP\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(nick ? nick : source)))
	if (nick)
	    notice(s_NickServ, source, "Nick %s isn't registered.", nick);
	else
	    notice(s_NickServ, source, "Your nick isn't registered.");

    else if (!is_services_op(source) && (host(ni)->flags & NI_SUSPENDED))
	notice(s_NickServ, source,
		"Access Denied for SUSPENDED users.");

    else if (!nick && (!u || !(ni->flags & NI_IDENTIFIED))) {
	notice(s_NickServ, source,
		"Password authentication required for that command.");
	notice(s_NickServ, source,
		"Retry after typing \2/msg %s IDENTIFY \37password\37.",
		s_NickServ);
	if (!u)
	    log("%s: DROP from nonexistent user %s", s_NickServ, source);

    } else if (nick && !u) {
	log("%s: DROP %s from nonexistent oper %s", s_NickServ, nick, source);
	notice(s_NickServ, source, "Can't find your user record!");

    } else if (nick && (ni->flags & NI_IRCOP))
	notice(s_NickServ, source, "Can't drop a nick with \2IRC Operator\2 flag set");

    else {
	int i;
	if(services_level!=1)
	    notice(s_NickServ, source,
		"Warning: Services is in read-only mode.  Changes will not be saved.");
	i=delnick(ni)-1;
	log("%s: %s!%s@%s dropped nickname %s (and %d slaves)", s_NickServ,
		source, u->username, u->host, nick ? nick : source, i);
	if (nick)
	    notice(s_NickServ, source, "Nickname %s%s%s%s has been dropped.",
		nick, i ? " (and " : "", i ? itoa(i) : "", i ? " slaves)" : "");
	else
	    notice(s_NickServ, source, "Your nickname%s%s%s has been dropped.",
		i ? " (and " : "", i ? itoa(i) : "", i ? " slaves)" : "");
    }
}

/*************************************************************************/

static void do_slavedrop(const char *source)
{
    char *nick = strtok(NULL, " ");
    NickInfo *ni, *hni;
    User *u = finduser(source);

    if (services_level!=1) {
	notice(s_NickServ, source,
		"Sorry, nickname de-registration is temporarily disabled.");
	return;
    }

    if (!nick) {
	notice(s_NickServ, source, "Syntax: \2DROPSLAVE\2 \37nick\37");
	notice(s_NickServ, source,
		"\2/msg %s DROPSLAVE\2 for more information.", s_NickServ);

    } else if (stricmp(source, nick)==0)
	notice(s_NickServ, source, "Use \2/MSG %s DROP\2 to drop your nickname.",
		s_NickServ);

    else if (!(ni = findnick(nick)) || !(hni = host(ni)))
	notice(s_NickServ, source, "Nick %s isn't registered.", nick);

    else if (hni->flags & NI_SUSPENDED)
	notice(s_NickServ, source,
		"Access Denied for SUSPENDED users.");

    else if (stricmp(hni->nick, source)!=0) {
	notice(s_NickServ, source, "Nick %s does not belong to you.", ni->nick);
	notice(s_NickServ, source, "If this is your nick, you MUST be on the 'host' nick.");

    } else if (!u || !(hni->flags & NI_IDENTIFIED)) {
	notice(s_NickServ, source,
		"Password authentication required for that command.");
	notice(s_NickServ, source,
		"Retry after typing \2/msg %s IDENTIFY \37password\37.",
		s_NickServ);
	if (!u)
	    log("%s: DROPSLAVE from nonexistent user %s", s_NickServ, source);

    } else {
	if(services_level!=1)
	    notice(s_NickServ, source,
		"Warning: Services is in read-only mode.  Changes will not be saved.");
	delnick(ni);
	log("%s: %s!%s@%s dropped nickname %s (slave)", s_NickServ,
		source, u->username, u->host, nick);
	notice(s_NickServ, source, "Nickname %s has been dropped.",
		nick);
    }
}

/*************************************************************************/

static void do_set(const char *source)
{
    char *cmd    = strtok(NULL, " ");
    char *param  = strtok(NULL, " ");
    NickInfo *ni = findnick(source);
    User *u;

    if(services_level!=1) {
	notice(s_NickServ, source,
		"Sorry, nickname option setting is temporarily disabled.");
	return;
    }

    if (!param) {
	notice(s_NickServ, source,
		"Syntax: \2SET \37option\37 \37parameters\37\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(source)))
	notice(s_NickServ, source, "Your nickname is not registered.");

    else if (getflags(ni) & NI_SUSPENDED)
	notice(s_NickServ, source,
		"Access Denied for SUSPENDED users.");

    else if (!(u = finduser(source)) || !(ni->flags & NI_IDENTIFIED)) {
	notice(s_NickServ, source,
		"Password authentication required for that command.");
	notice(s_NickServ, source,
		"Retry after typing \2/msg %s IDENTIFY \37password\37.",
		s_NickServ);
	if (!u)
	    log("%s: SET from nonexistent user %s", s_NickServ, source);

    } else {
	Hash_NI *command, hash_table[] = {
		{ "PASS*",	H_NONE,	do_set_password },
		{ "KILL*",	H_NONE,	do_set_kill },
#if FILE_VERSION > 2
		{ "*MAIL",	H_NONE,	do_set_email },
		{ "CONTACT",	H_NONE,	do_set_email },
		{ "U*R*L",	H_NONE,	do_set_url },
		{ "W*W*W",	H_NONE,	do_set_url },
		{ "HOME*",	H_NONE,	do_set_url },
		{ "*PAGE",	H_NONE,	do_set_url },
#endif
		{ "*M*S*G*",	H_NONE,	do_set_privmsg },
		{ "PRIV*",	H_NONE,	do_set_private },
		{ "SEC*",	H_NONE,	do_set_secure },
		{ "*OP*",	H_OPER,	do_set_ircop },
		{ NULL }
	};

	if (command = get_ni_hash(source, cmd, hash_table))
	    (*command->process)(ni,  param);
	else
	    notice(s_NickServ, source,
		"Unknown SET option \2%s\2.", strupper(cmd));
    }
}

/*************************************************************************/

static void do_set_password(NickInfo *ni, char *param)
{
    char *source = ni->nick;
    if (stricmp(ni->nick, param)==0 || stricmp(host(ni)->nick, param)==0 ||
    							strlen(param) < 5) {
	notice(s_NickServ, source,
		"Please try again with a more obscure password.");
	notice(s_NickServ, source,
		"Passwords should be at least five characters long, should");
	notice(s_NickServ, source,
		"not be something easily guessed (e.g. your real name or your");
	notice(s_NickServ, source,
		"nick), and cannot contain the space character.");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET PASSWORD\2 for more information.");
    } else {
	strscpy(host(ni)->pass, param, PASSMAX);
	notice(s_NickServ, ni->nick, "Password changed to \2%s\2.", param);
    }
}

#if FILE_VERSION > 2
static void do_set_email(NickInfo *ni, char *param)
{
    free(host(ni)->email);
    if(stricmp(param, "NONE")==0) {
	host(ni)->email = sstrdup("");
	notice(s_NickServ, ni->nick, "E-Mail removed.");
    } else {
	host(ni)->email = sstrdup(param);
	notice(s_NickServ, ni->nick, "E-Mail changed to \2%s\2.", param);
    }
}

static void do_set_url(NickInfo *ni, char *param)
{
    free(host(ni)->url);
    if(stricmp(param, "NONE")==0) {
	host(ni)->url = sstrdup("");
	notice(s_NickServ, ni->nick, "World Wide Web Page (URL) removed.");
    } else {
	host(ni)->url = sstrdup(param);
	notice(s_NickServ, ni->nick, "World Wide Web Page (URL) changed to \2%s\2.", param);
    }
}
#endif

/*************************************************************************/

static void do_set_kill(NickInfo *ni, char *param)
{
    char *source = ni->nick;

    if (stricmp(param, "ON")==0) {
	host(ni)->flags |= NI_KILLPROTECT;
	notice(s_NickServ, source, "Kill protection is now \2ON\2.");

    } else if (stricmp(param, "OFF")==0) {
	host(ni)->flags &= ~NI_KILLPROTECT;
	notice(s_NickServ, source, "Kill protection is now \2OFF\2.");

    } else {
	notice(s_NickServ, source, "Syntax: \2SET KILL {ON|OFF}\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET KILL\2 for more information.", s_NickServ);
    }
}

static void do_set_private(NickInfo *ni, char *param)
{
    char *source = ni->nick;

    if (stricmp(param, "ON")==0) {
	host(ni)->flags |= NI_PRIVATE;
	notice(s_NickServ, source, "Privacy mode is now \2ON\2.");

    } else if (stricmp(param, "OFF")==0) {
	host(ni)->flags &= ~NI_PRIVATE;
	notice(s_NickServ, source, "Privacy mode is now \2OFF\2.");

    } else {
	notice(s_NickServ, source, "Syntax: \2SET PRIVATE {ON|OFF}\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET PRIVATE\2 for more information.", s_NickServ);
    }
}

/*************************************************************************/

static void do_set_secure(NickInfo *ni, char *param)
{
    char *source = ni->nick;

    if (stricmp(param, "ON")==0) {
	host(ni)->flags |= NI_SECURE;
	notice(s_NickServ, source, "Secure option is now \2ON\2.");

    } else if (stricmp(param, "OFF")==0) {
	host(ni)->flags &= ~NI_SECURE;
	notice(s_NickServ, source, "Secure option is now \2OFF\2.");

    } else {
	notice(s_NickServ, source, "Syntax: \2SET SECURE {ON|OFF}\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET SECURE\2 for more information.",
		s_NickServ);
    }
}

/*************************************************************************/

static void do_set_privmsg(NickInfo *ni, char *param)
{
    char *source = ni->nick;

    if (stricmp(param, "ON")==0) {
	host(ni)->flags |= NI_PRIVMSG;
	notice(s_NickServ, source, "Now using \2PRIVMSG\2 instead of NOTICE.");

    } else if (stricmp(param, "OFF")==0) {
	host(ni)->flags &= ~NI_PRIVMSG;
	notice(s_NickServ, source, "Now using \2NOTICE\2 instead of PRIVMSG.");

    } else {
	notice(s_NickServ, source, "Syntax: \2SET PRIVMSG {ON|OFF}\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET PRIVMSG\2 for more information.",
		s_NickServ);
    }
}

/*************************************************************************/

static void do_set_ircop(NickInfo *ni, char *param)
{
    char *source = ni->nick;

    if (stricmp(param, "ON")==0) {
	host(ni)->flags |= NI_IRCOP;
	notice(s_NickServ, source, "IRC Operator is now \2ON\2.");

    } else if (stricmp(param, "OFF")==0) {
	host(ni)->flags &= ~NI_IRCOP;
	notice(s_NickServ, source, "IRC Operator is now \2OFF\2.");

    } else {
	notice(s_NickServ, source, "Syntax: \2SET IRCOP {ON|OFF}\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP SET IRCOP\2 for more information.",
		s_NickServ);
    }
}

/*************************************************************************/

static void do_deop(const char *source)
{
	NickInfo *ni;
	char *s;

	if (!(s = strtok(NULL, " "))) {
	    notice(s_NickServ, source, "Syntax: \2DEOP \37nickname\37\2");
	    return;
	}
	if (!(ni = host(findnick(s))))
	    notice(s_NickServ, source, "Nick %s does not exist.", s);
	else if (is_services_op(ni->nick))
	    notice(s_NickServ, source, "Access Denied.");
	else {
	    host(ni)->flags &= ~NI_IRCOP;
	    notice(s_NickServ, source, "IRC Operator for \2%s\2 is now \2OFF\2.",
		ni->nick);
	}
}

/*************************************************************************/

static void do_access(const char *source)
{
    char *cmd = strtok(NULL, " ");
    NickInfo *ni = findnick(source);
    User *u;

    if (!cmd) {
	notice(s_NickServ, source,
		"Syntax: \2ACCESS {ADD|DEL|LIST} [\37mask\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);

    } else if (!ni)
	notice(s_NickServ, source, "Your nick isn't registered.");

    else if (host(ni)->flags & NI_SUSPENDED)
	notice(s_NickServ, source,
		"Access Denied for SUSPENDED users.");

    else if (!(u = finduser(source)) || !(ni->flags & NI_IDENTIFIED)) {
	notice(s_NickServ, source,
		"Password authentication required for that command.");
	notice(s_NickServ, source,
		"Retry after typing \2/msg %s IDENTIFY \37password\37.",
		s_NickServ);
	if (!u)
	    log("%s: SET from nonexistent user %s", s_NickServ, source);

    } else {
	Hash *command, hash_table[] = {
		{ "ADD",	H_NONE,	do_access_add },
		{ "CREATE",	H_NONE,	do_access_add },
		{ "MAKE",	H_NONE,	do_access_add },
		{ "DEL*",	H_NONE,	do_access_del },
		{ "ERASE",	H_NONE,	do_access_del },
		{ "TRASH",	H_NONE,	do_access_del },
		{ "LIST*",	H_NONE,	do_access_list },
		{ "VIEW",	H_NONE,	do_access_list },
		{ "DISP*",	H_NONE,	do_access_list },
		{ "SHOW*",	H_NONE,	do_access_list },
		{ NULL }
	};

	if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
	else {
	    notice(s_NickServ, source,
		"Syntax: \2ACCESS {ADD|DEL|LIST} [\37mask\37]\2");
	    notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);
	}
    }
}

static int do_access_help (const char *source, char *cmd, char *mask)
{
    if (!cmd || ((stricmp(cmd,"LIST")==0) ? !!mask : !mask)) {
	notice(s_NickServ, source,
		"Syntax: \2ACCESS {ADD|DEL} [\37mask\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);
	return 1;
    }
    return 0;
}

static void do_access_add(const char *source)
{
    char *mask = strtok(NULL, " ");
    NickInfo *ni = host(findnick(source));
    int i;
    char **access;

    if (!mask) {
	notice(s_NickServ, source,
		"Syntax: \2ACCESS ADD \37mask\37\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);
    } else if (mask && !strchr(mask, '@')) {
	notice(s_NickServ, source,
		"Mask must be in the form \37user\37@\37host\37.");
	notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);
    } else {
	for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
	    if (strcmp(*access, mask) == 0) {
		notice(s_NickServ, source,
			"Mask \2%s\2 already present on your access list.",
			*access);
		return;
	    }
	++ni->accesscount;
	ni->access = srealloc(ni->access, sizeof(char *) * ni->accesscount);
	ni->access[ni->accesscount-1] = sstrdup(mask);
	notice(s_NickServ, source, "\2%s\2 added to your access list.", mask);
    }
}

static void do_access_del(const char *source)
{
    char *mask = strtok(NULL, " ");
    NickInfo *ni = host(findnick(source));
    int i;
    char **access;

    if (!mask) {
	notice(s_NickServ, source,
		"Syntax: \2ACCESS DEL \37mask|num\37\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_NickServ);
    } else {
	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn(mask, "1234567890") == strlen(mask) &&
				(i = atoi(mask)) > 0 && i <= ni->accesscount) {
	    --i;
	    access = &ni->access[i];
	} else {
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i)
		if (strcmp(*access, mask) == 0)
		    break;
	    if (i == ni->accesscount)
		for (access = ni->access, i = 0; i < ni->accesscount;
							++access, ++i)
		    if (stricmp(*access, mask)==0)
			break;
	    if (i == ni->accesscount) {
		notice(s_NickServ, source,
			"\2%s\2 not found on your access list.", mask);
		return;
	    }
	}
	notice(s_NickServ, source,
		"\2%s\2 deleted from your access list.", *access);
	free(*access);
	--ni->accesscount;
	if (i < ni->accesscount)	/* if it wasn't the last entry... */
	    bcopy(access+1, access, (ni->accesscount-i) * sizeof(char *));
	if (ni->accesscount)		/* if there are any entries left... */
	    ni->access = srealloc(ni->access, ni->accesscount * sizeof(char *));
	else {
	    free(ni->access);
	    ni->access = NULL;
	}
    }
}

static void do_access_list(const char *source)
{
    char *mask = strtok(NULL, " ");
    NickInfo *ni = host(findnick(source));
    int i;
    char **access;

	notice(s_NickServ, source, "Access list:");
	for (access = ni->access, i = 0; i < ni->accesscount; ++access, ++i) {
	    if (mask && !match_wild_nocase(mask, *access))
		continue;
	    notice(s_NickServ, source, "  %3d   %s", i+1, *access);
	}
}

/*************************************************************************/

#if (FILE_VERSION > 3) && defined(MEMOS)
static void do_ignore(const char *source)
{
    char *cmd = strtok(NULL, " ");
    NickInfo *ni = findnick(source), *tni;
    User *u;

    if (!cmd) {
	    notice(s_NickServ, source,
		"Syntax: \2IGNORE {ADD|DEL|LIST} [\37nick\37]\2");
	    notice(s_NickServ, source,
		"\2/msg %s HELP IGNORE\2 for more information.", s_NickServ);

    } else if (!ni)
	notice(s_NickServ, source, "Your nick isn't registered.");

    else if (host(ni)->flags & NI_SUSPENDED)
	notice(s_NickServ, source,
		"Access Denied for SUSPENDED users.");

    else if (!(u = finduser(source)) || !(ni->flags & NI_IDENTIFIED)) {
	notice(s_NickServ, source,
		"Password authentication required for that command.");
	notice(s_NickServ, source,
		"Retry after typing \2/msg %s IDENTIFY \37password\37.",
		s_NickServ);
	if (!u)
	    log("%s: SET from nonexistent user %s", s_NickServ, source);

    } else {
	Hash *command, hash_table[] = {
		{ "ADD",	H_NONE,	do_ignore_add },
		{ "CREATE",	H_NONE,	do_ignore_add },
		{ "MAKE",	H_NONE,	do_ignore_add },
		{ "DEL*",	H_NONE,	do_ignore_del },
		{ "ERASE",	H_NONE,	do_ignore_del },
		{ "TRASH",	H_NONE,	do_ignore_del },
		{ "LIST*",	H_NONE,	do_ignore_list },
		{ "VIEW",	H_NONE,	do_ignore_list },
		{ "DISP*",	H_NONE,	do_ignore_list },
		{ "SHOW*",	H_NONE,	do_ignore_list },
		{ NULL }
	};

	if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
	else {
	    notice(s_NickServ, source,
		"Syntax: \2IGNORE {ADD|DEL|LIST} [\37nick\37]\2");
	    notice(s_NickServ, source,
		"\2/msg %s HELP IGNORE\2 for more information.", s_NickServ);
	}
    }
}

static int do_ignore_help (const char *source, char *cmd, char *nick)
{
    if (!cmd || ((stricmp(cmd,"LIST")==0) ? !!nick : !nick)) {
	notice(s_NickServ, source,
		"Syntax: \2IGNORE {ADD|DEL} [\37nick\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP IGNORE\2 for more information.", s_NickServ);
    return 1;
    }
    return 0;
}

static void do_ignore_add(const char *source)
{
    char *nick = strtok(NULL, " ");

    if (!do_ignore_help(source, "ADD", nick)) {
	NickInfo *ni = host(findnick(source)), *tni;
	int i;
	char **ignore;

	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
	    if (strcmp(*ignore, nick) == 0) {
		notice(s_NickServ, source,
			"Nick \2%s\2 already present on your ignore list.",
			*ignore);
		return;
	    }
	if (!(tni = host(findnick(nick)))) {
	    notice(s_NickServ, source, "\2%s\2 is not registered.", nick);
	    return;
	} else if (stricmp(source, nick)==0) {
	    notice(s_NickServ, source, "You know, I should do it, just to spite you.");
	    return;
	} else {
	    ++ni->ignorecount;
	    ni->ignore = srealloc(ni->ignore, sizeof(char *) * ni->ignorecount);
	    ni->ignore[ni->ignorecount-1] = sstrdup(tni->nick);
	    notice(s_NickServ, source, "\2%s\2 added to your ignore list%s%s%s.",
		tni->nick,
		stricmp(tni->nick, nick)!=0 ? " (HOST of " : "",
		stricmp(tni->nick, nick)!=0 ? nick : "",
		stricmp(tni->nick, nick)!=0 ? ")" : "");
	}
    }
}
static void do_ignore_del(const char *source)
{
    char *nick = strtok(NULL, " ");

    if (!do_ignore_help(source, "DEL", nick)) {
	NickInfo *ni = host(findnick(source));
	int i;
	char **ignore;

	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn(nick, "1234567890") == strlen(nick) &&
				(i = atoi(nick)) > 0 && i <= ni->ignorecount) {
	    --i;
	    ignore = &ni->ignore[i];
	} else {
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i)
		if (strcmp(*ignore, nick) == 0)
		    break;
	    if (i == ni->ignorecount)
		for (ignore = ni->ignore, i = 0; i < ni->ignorecount;
							++ignore, ++i)
		    if (stricmp(*ignore, nick)==0)
			break;
	    if (i == ni->ignorecount) {
		notice(s_NickServ, source,
			"\2%s\2 not found on your ignore list.", nick);
		return;
	    }
	}
	notice(s_NickServ, source,
		"\2%s\2 deleted from your ignore list.", *ignore);
	free(*ignore);
	--ni->ignorecount;
	if (i < ni->ignorecount)	/* if it wasn't the last entry... */
	    bcopy(ignore+1, ignore, (ni->ignorecount-i) * sizeof(char *));
	if (ni->ignorecount)		/* if there are any entries left... */
	    ni->ignore = srealloc(ni->ignore, ni->ignorecount * sizeof(char *));
	else {
	    free(ni->ignore);
	    ni->ignore = NULL;
	}
    }
}
static void do_ignore_list(const char *source)
{
    char *nick = strtok(NULL, " ");

    if (!do_ignore_help(source, "LIST", nick)) {
	char *buf, **ignore;
	NickInfo *ni = host(findnick(source)), *tni;
	int i;
	
	notice(s_NickServ, source, "Ignore list:");
	for (ignore = ni->ignore, i = 0; i < ni->ignorecount; ++ignore, ++i) {
	    if (nick && !match_wild_nocase(nick, *ignore))
		continue;

	    if (!(tni = host(findnick(*ignore))))
		notice(s_NickServ, source, "    %s", *ignore);
	    else
		notice(s_NickServ, source,
			"    %s (%s)", *ignore, tni->last_usermask);
	}
    }
}
#endif /* (FILE_VERSION > 3) && defined(MEMOS) */

/*************************************************************************/

static void do_info(const char *source)
{
    char *nick = strtok(NULL, " ");
    NickInfo *ni;

    if (!nick) {
	notice(s_NickServ, source, "Syntax: \2INFO \37nick\37\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP IDENTIFY\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(nick))) {
	notice(s_NickServ, source, "Nick \2%s\2 isn't registered.", nick);

    } else if (ni->flags & NI_VERBOTEN)
	notice(s_NickServ, source,
		"Nick \2%s\2 may not be registered or used.", ni->nick);

    else {
	char buf[BUFSIZE];
	long flags = getflags(ni);
	NickInfo *hni = host(ni);
	int i, onignore = is_on_ignore(source, nick);

	notice(s_NickServ, source,
		"%s is %s\n", ni->nick, ni->last_realname);
	if (flags & NI_SLAVE)
	    notice(s_NickServ, source,
			"        Host Nick: \2%s\2\n", ni->last_usermask);
#if FILE_VERSION > 2
        if (!(flags & NI_SUSPENDED) && !onignore) {
	    if(strlen(host(ni)->email)>0)
		notice(s_NickServ, source,
			"   E-Mail address: %s\n", hni->email);
	    if(strlen(host(ni)->url)>0)
		notice(s_NickServ, source,
			"   WWW Page (URL): %s\n", hni->url);
	}
#endif
        if (flags & NI_SUSPENDED)
	    notice(s_NickServ, source,
		"    Suspended For: %s\n", hni->last_usermask);
	else
	    if (!(flags & NI_PRIVATE) && !userisnick(hni->nick) &&
				    !slaveonline(hni->nick) && !onignore)
		notice(s_NickServ, source,
		    "Last seen address: %s\n", hni->last_usermask);
	notice(s_NickServ, source,     "       Registered: %s ago\n",
		time_ago(ni->time_registered, 1));
	if (slaveonline(hni->nick) || userisnick(hni->nick)) {
	    int sc=slavecount(ni->nick);
	    char slaves[((sc+1)*NICKMAX)+((sc+1)*2)];
	    NickInfo *sni;
	    if (stricmp(ni->nick, hni->nick)!=0 && userisnick(hni->nick))
		strcpy(slaves, finduser(hni->nick)->nick);
	    else
		*slaves=0;
	    for (i=sc; i; i--) {
		sni=slave(hni->nick, i);
		if (userisnick(sni->nick) && stricmp(sni->nick, ni->nick)!=0) {
		    if (*slaves) strcat(slaves, ", ");
		    strcat(slaves, sni->nick);
		}
	    }
	    if (*slaves && !onignore)
		notice(s_NickServ, source,
			"   %snline as: %s", userisnick(ni->nick) ? "Also o" :
			"     O", slaves);
	} else {
	    if (!userisnick(ni->nick) && !onignore) {
		notice(s_NickServ, source, "        Last seen: %s ago\n",
			time_ago(ni->last_seen, 1));
		if (!slaveonline(hni->nick) && !userisnick(hni->nick) &&
					(ni->last_seen != hni->last_seen))
		    notice(s_NickServ, source, "      Last online: %s ago\n",
			time_ago(hni->last_seen, 1));
	    }
	}
	*buf = 0;
        if (flags & NI_SUSPENDED)
	    strcat(buf, "\2SUSPENDED USER\2");
	else {
	    if (flags & NI_KILLPROTECT)
		strcat(buf, "Kill protection");
	    if (flags & NI_SECURE) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "Security");
	    }
	    if (flags & NI_PRIVATE) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "Private");
	    }
	    if (flags & NI_IRCOP) {
		if (*buf)
		    strcat(buf, ", ");
		strcat(buf, "IRC ");
		if (is_justservices_op(nick))
		    strcat(buf, "S-");
		strcat(buf, "Operator");
	    }
	    if (!*buf)
		strscpy(buf, "None", BUFSIZE);
	}
	notice(s_NickServ, source, "          Options: %s", buf);
#if (FILE_VERSION > 3) && defined(MEMOS)
        if (flags & NI_SUSPENDED)
	    notice(s_NickServ, source, "NOTE: Cannot send memos to a suspended user.");
	else
	    if (onignore)
		notice(s_NickServ, source, "NOTE: This user is ignoring your memos.");
#endif
        if (userisnick(ni->nick) && !(flags & NI_SUSPENDED) && !onignore)
	    notice(s_NickServ, source, "This user is online, type \2/whois %s\2 for more information.", finduser(nick)->nick);
    }
}

/*************************************************************************/

static void do_list(const char *source)
{
    char *pattern = strtok(NULL, " ");
    char *cmax = strtok(NULL, " ");
    NickInfo *ni;
    int nnicks, i, max = 50;
    long flags;
    char buf[BUFSIZE];

    if (!pattern) {
	notice(s_NickServ, source, "Syntax: \2LIST \37pattern\37\2 [\37max\37]");
	notice(s_NickServ, source,
		"\2/msg %s HELP LIST\2 for more information.", s_NickServ);

    } else {
	if (cmax && atoi(cmax)>0)
	    max=atoi(cmax);
	nnicks = 0;
	notice(s_NickServ, source, "List of entries matching \2%s\2:",
		pattern);
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next) {
		if (ni->flags & NI_SLAVE)
		    continue;
		if (!(is_oper(source)))
		    if (ni->flags & (NI_PRIVATE | NI_VERBOTEN | NI_SUSPENDED))
			continue;
		if (ni->flags & (NI_VERBOTEN | NI_SUSPENDED))
		    if (strlen(ni->nick) > sizeof(buf))
			continue;
		else
		    if (strlen(ni->nick)+strlen(ni->last_usermask) > sizeof(buf))
			continue;
		if (ni->flags & NI_VERBOTEN)
		    snprintf(buf, BUFSIZE, "%-20s  << FORBIDDEN >>", ni->nick);
	        else if (ni->flags & NI_SUSPENDED)
		    snprintf(buf, BUFSIZE, "%-20s  << SUSPENDED >>", ni->nick);
		else
		    if (strlen(ni->last_usermask)>0)
			snprintf(buf, BUFSIZE, "%-20s  %s", ni->nick, ni->last_usermask);
		    else
			snprintf(buf, BUFSIZE, "%-20s", ni->nick);
		if (match_wild_nocase(pattern, buf)) {
		    if (++nnicks <= max)
			notice(s_NickServ, source, "    %s", buf);
		}
	    }
	notice(s_NickServ, source, "End of list - %d/%d matches shown.",
					nnicks>max ? max : nnicks, nnicks);
    }
}

static void do_listoper(const char *source)
{
    char *pattern = strtok(NULL, " ");
    char *cmax = strtok(NULL, " ");
    NickInfo *ni;
    int nnicks, i, max = 50;
    char buf[BUFSIZE];

    if (!pattern) {
	notice(s_NickServ, source, "Syntax: \2LISTOPER \37pattern\37\2 [\37max\37]");
	notice(s_NickServ, source,
		"\2/msg %s HELP LISTOPER\2 for more information.", s_NickServ);

    } else {
	if (cmax && atoi(cmax)>0)
	    max=atoi(cmax);
	nnicks = 0;
	notice(s_NickServ, source, "List of oper entries matching \2%s\2:",
		pattern);
	for (i = 33; i < 256; ++i)
	    for (ni = nicklists[i]; ni; ni = ni->next) {
		if (ni->flags & NI_SLAVE)
		    continue;
		if (!(host(ni)->flags & NI_IRCOP))
		    continue;
		if (strlen(ni->nick)+strlen(ni->last_usermask) > sizeof(buf))
		    continue;
		if (strlen(ni->last_usermask)>0)
		    snprintf(buf, BUFSIZE, "%-20s  %s", ni->nick, ni->last_usermask);
		else
		    snprintf(buf, BUFSIZE, "%-20s", ni->nick);
		if (match_wild_nocase(pattern, buf)) {
		    if (++nnicks <= max)
			notice(s_NickServ, source, "    %s", buf);
		}
	    }
	notice(s_NickServ, source, "End of list - %d/%d matches shown.",
					nnicks>max ? max : nnicks, nnicks);
    }
}

/*************************************************************************/

static void do_recover(const char *source)
{
    char *nick = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");
    NickInfo *ni;
    User *u = finduser(source);

    if (!nick) {
	notice(s_NickServ, source,
		"Syntax: \2RECOVER \37nickname\37 [\37password\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP RECOVER\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(nick)))
	notice(s_NickServ, source, "Nick %s isn't registered.", nick);

    else if (!finduser(nick))
	notice(s_NickServ, source, "Nick %s isn't currently in use.");

    else if (!u) {
	log("%s: RECOVER: source user %s not found!", s_NickServ, source);
	notice(s_NickServ, source, "Recover failed.");

    } else if (stricmp(nick, source)==0)
        notice(s_NickServ, source, "You can't recover yourself.");

    else if (pass ? strcmp(pass, host(ni)->pass)==0 : is_on_access(u, ni)) {
	collide(ni);
	notice(s_NickServ, source, "User claiming your nick has been killed.");
	notice(s_NickServ, source,
		"\2/msg %s RELEASE\2 to get it back before the one-minute timeout.",
		s_NickServ);

    } else {
	notice(s_NickServ, source,
		pass ? "Password incorrect." : "Access denied.");
	if (pass)
	    log("%s: RECOVER: invalid password for %s by %s!%s@%s",
			s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void do_release(const char *source)
{
    char *nick = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");
    NickInfo *ni;
    User *u = finduser(source);

    if (!nick) {
	notice(s_NickServ, source,
		"Syntax: \2RELEASE \37nickname\37 [\37password\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP RELEASE\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(nick)))
	notice(s_NickServ, source, "Nick %s isn't registered.", nick);

    else if (!(ni->flags & NI_KILL_HELD))
	notice(s_NickServ, source, "Nick %s isn't being held.", nick);

    else if (!u) {
	log("%s: RELEASE: source user %s not found!", s_NickServ, source);
	notice(s_NickServ, source, "Release failed.");

    } else if (pass ? strcmp(pass, host(ni)->pass)==0 : is_on_access(u, ni)) {
	release(ni);
	notice(s_NickServ, source,
		"Services' hold on your nick has been released.");

    } else {
	notice(s_NickServ, source,
		pass ? "Password incorrect." : "Access denied.");
	if (pass)
	    log("%s: RELEASE: invalid password for %s by %s!%s@%s",
			s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void do_ghost(const char *source)
{
    char *nick = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");
    NickInfo *ni;
    User *u = finduser(source);

    if (!nick) {
	notice(s_NickServ, source,
		"Syntax: \2GHOST \37nickname\37 [\37password\37]\2");
	notice(s_NickServ, source,
		"\2/msg %s HELP GHOST\2 for more information.", s_NickServ);

    } else if (!(ni = findnick(nick)))
	notice(s_NickServ, source, "Nick %s isn't registered.", nick);

    else if (!finduser(nick))
	notice(s_NickServ, source, "Nick %s isn't currently in use.");

    else if (!u) {
	notice(s_NickServ, source, "Ghost failed.");
	log("%s: GHOST: source user %s not found!", s_NickServ, source);

    } else if (stricmp(nick, source)==0)
    	notice(s_NickServ, source, "You can't ghost yourself.");

    else if (pass ? strcmp(pass, host(ni)->pass)==0 : is_on_access(u, ni)) {
	char buf[NICKMAX+25];
	snprintf(buf, sizeof(buf), "%s (Removing GHOST user)", source);
	kill_user(s_NickServ, ni->nick, buf);
	notice(s_NickServ, source, "Ghost with your nick has been killed.");

    } else {
	notice(s_NickServ, source,
		pass ? "Password incorrect." : "Access denied.");
	if (pass)
	    log("%s: GHOST: invalid password for %s by %s!%s@%s",
			s_NickServ, source, u->username, u->host);
    }
}

/*************************************************************************/

static void do_getpass(const char *source)
{
    char *nick = strtok(NULL, " ");
    NickInfo *ni, *rni;
    User *u = finduser(source);

    if (!nick)
	notice(s_NickServ, source, "Syntax: \2GETPASS \37nickname\37\2");

    else if (!u)
	notice(s_NickServ, source, "Couldn't get your user info!");

    else if (!(rni = findnick(nick)) || !(ni = host(rni)))
	notice(s_NickServ, source, "Nick %s isn't registered.", nick);

    else {
	log("%s: %s!%s@%s used GETPASS on %s (%s)",
		s_NickServ, source, u->username, u->host, ni->nick, rni->nick);
	wallops(s_NickServ, "\2%s\2 used GETPASS on \2%s\2 (%s)", source,
		ni->nick, rni->nick);
	notice(s_NickServ, source, "Password for %s (%s) is \2%s\2.",
		ni->nick, rni->nick, ni->pass);
    }
}


/*************************************************************************/

static void do_forbid(const char *source)
{
    NickInfo *ni;
    char *nick = strtok(NULL, " ");

    if (!nick) {
	notice(s_NickServ, source, "Syntax: \2FORBID \37nickname\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_NickServ, source,
	    "Warning: Services is in read-only mode; changes will not be saved.");
    if (ni = findnick(nick))
	if (getflags(ni) & NI_IRCOP) {
	    notice(s_NickServ, source,
		"Cannot forbid an \2IRC Operator\2.");
	    return;
	} else {
	    int i = delnick(ni)-1;
	    wallops(s_NickServ, "\2%s\2 used FORBID on \2%s\2%s%s%s.", source,
	    nick, i ? " (" : "",
	    i ? itoa(i) : "",
	    i ? " slave nicks dropped)" : "");
	}
    if (ni = makenick(nick)) {
	ni->flags |= NI_VERBOTEN;
	log("%s: %s set FORBID for nick %s", s_NickServ, source, nick);
	notice(s_NickServ, source, "Nick \2%s\2 is now FORBIDden.", nick);
    } else {
	log("%s: Valid FORBID for %s by %s failed", s_NickServ,
		nick, source);
	notice(s_NickServ, source, "Couldn't FORBID nick \2%s\2!", nick);
    }
}

/*************************************************************************/

static void do_suspend(const char *source)
{
    NickInfo *ni, *hni;
    char *nick = strtok(NULL, " ");
    char *reason = strtok(NULL, "");

    if (!reason) {
	notice(s_NickServ, source, "Syntax: \2SUSPEND \37nickname reason\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_NickServ, source,
	    "Warning: Services is in read-only mode; changes will not be saved.");
    if (!(ni = findnick(nick)) || !(hni = host(ni)))
	notice(s_NickServ, source,
	    "Nick %s does not exist", nick);
    else if (hni->flags & NI_IRCOP)
	notice(s_NickServ, source,
	    "Cannot suspend an \2IRC Operator\2.");
    else if (hni->flags & NI_SUSPENDED)
	notice(s_NickServ, source,
	    "Nick %s (%s) is already suspended.", hni->nick, ni->nick);
    else {
	int i;
	hni->flags |= NI_SUSPENDED;
	hni->flags &= ~NI_IDENTIFIED;
	for (i=slavecount(hni->nick); i; i--)
	    slave(hni->nick, i)->flags &= ~NI_IDENTIFIED;
	if (hni->last_usermask)
	    free(hni->last_usermask);
	hni->last_usermask = smalloc(strlen(reason)+2);
	sprintf(hni->last_usermask, "%s", reason);
	log("%s: %s set SUSPEND for nick %s (%s) because of %s", s_NickServ,
		source, hni->nick, ni->nick, reason);
	notice(s_NickServ, source, "Nick \2%s\2 (%s) is now SUSPENDED.",
		hni->nick, ni->nick);
    }
}

static void do_unsuspend(const char *source)
{
    NickInfo *ni, *hni;
    char *nick = strtok(NULL, " ");

    if (!nick) {
	notice(s_NickServ, source, "Syntax: \2UNSUSPEND \37nickname\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_NickServ, source,
	    "Warning: Services is in read-only mode; changes will not be saved.");
    if (!(ni = findnick(nick)) || !(hni = host(ni)))
	notice(s_NickServ, source,
	    "Nick %s does not exist", nick);
    else if (!(hni->flags & NI_SUSPENDED))
	notice(s_NickServ, source,
	    "Nick %s (%s) is not suspended.", hni->nick, ni->nick);
    else {
	hni->flags &= ~NI_SUSPENDED;
	if (hni->last_usermask)
	    free(hni->last_usermask);
	hni->last_usermask = NULL;
	log("%s: %s removed SUSPEND for nick %s (%s)", s_NickServ, source,
		hni->nick, ni->nick);
	notice(s_NickServ, source, "Nick \2%s\2 (%s) is now UNSUSPENDED.",
		hni->nick, ni->nick);
    }
}

/*************************************************************************/
#endif  /* NICKSERV */
