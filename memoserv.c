/* MemoServ functions.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"


MemoList *memolists[256];	/* One for each initial character */
NewsList *newslists[256];	/* One for each initial character */

char s_MemoServ[NICKMAX];
char memoserv_db[512];
char newsserv_db[512];
int news_expire;

#include "ms-help.c"

MemoList *find_memolist (const char *nick);	/* Needed by NICKSERV */
static void alpha_insert_memolist (MemoList * ml);
static int candomemo (const char *source, const char *other);

NewsList *find_newslist (const char *chan);	/* Needed by CHANSERV */
static void alpha_insert_newslist (NewsList * nl);
static int candonews (const char *source, const char *chan, int action);

static void do_help (const char *source);
static void do_send (const char *source);
static void do_opersend (const char *source);
static void do_sopsend (const char *source);
static void do_forward (const char *source);
static void do_forward_send (const char *source, const char *origin, char *arg, const char *intext);
static void do_reply (const char *source);
static void do_reply_send (const char *source, const char *origin, const char *intext);
static void do_list (const char *source);
static void do_read (const char *source);
static void do_del (const char *source);

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

void
get_memoserv_stats (long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    MemoList *ml;

    for (i = 0; i < 256; i++)
	for (ml = memolists[i]; ml; ml = ml->next)
	{
	    count++;
	    mem += sizeof (*ml);
	    mem += sizeof (Memo) * ml->n_memos;
	    for (j = 0; j < ml->n_memos; j++)
		if (ml->memos[j].text)
		    mem += strlen (ml->memos[j].text) + 1;
	}
    *nrec = count;
    *memuse = mem;
}

void
get_newsserv_stats (long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    NewsList *nl;

    for (i = 0; i < 256; i++)
	for (nl = newslists[i]; nl; nl = nl->next)
	{
	    count++;
	    mem += sizeof (*nl);
	    mem += sizeof (Memo) * nl->n_newss;
	    for (j = 0; j < nl->n_newss; j++)
		if (nl->newss[j].text)
		    mem += strlen (nl->newss[j].text) + 1;
	}
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/
/*************************************************************************/

/* memoserv:  Main MemoServ routine. */
void
memoserv (const char *source, char *buf)
{
    char *cmd, *s;

    cmd = strtok (buf, " ");

    if (!cmd)
	return;

    else if (services_level != 1)
    {
	notice (s_MemoServ, source, MS_IS_BACKUP);
	return;

    }
    else if (stricmp (cmd, "\1PING") == 0)
    {
	if (!(s = strtok (NULL, "")))
	    s = "\1";
	notice (s_MemoServ, source, "\1PING %s", s);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"HELP", H_NONE, do_help},
	    {"SEND*", H_NONE, do_send},
	    {"WRITE", H_NONE, do_send},
	    {"*OPER*", H_OPER, do_opersend},
	    {"*S*OP*", H_SOP, do_sopsend},
	    {"LIST*", H_NONE, do_list},
	    {"VIEW*", H_NONE, do_list},
	    {"READ*", H_NONE, do_read},
	    {"RECIEVE", H_NONE, do_read},
	    {"CONTENTS", H_NONE, do_read},
	    {"GET", H_NONE, do_read},
	    {"DISP*", H_NONE, do_read},
	    {"SHOW*", H_NONE, do_read},
	    {"F*W*D*", H_NONE, do_forward},
	    {"RESEND", H_NONE, do_forward},
	    {"REPLY*", H_NONE, do_reply},
	    {"RESP*", H_NONE, do_reply},
	    {"DEL*", H_NONE, do_del},
	    {"ERASE", H_NONE, do_del},
	    {"TRASH", H_NONE, do_del},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	{
	    if(memos_on==FALSE&&(command->process==do_opersend||command->process==do_sopsend))
		notice (s_MemoServ, source, ERR_UNKNOWN_COMMAND, cmd, s_MemoServ);
	    else
		(*command->process) (source);
	}
	else
	    notice (s_MemoServ, source, ERR_UNKNOWN_COMMAND,
		    cmd, s_MemoServ);
    }
}

/*************************************************************************/

/* load_ms_dbase, save_ms_dbase:  Load/save memo data. */

void
load_ms_dbase (void)
{
    FILE *f = fopen (memoserv_db, "r");
    int i, j;
    MemoList *ml;
    Memo *memos;

    if (!f)
    {
	log_perror ("Can't read MemoServ database %s", memoserv_db);
	return;
    }
    switch (i = get_file_version (f, memoserv_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ml = smalloc (sizeof (MemoList));
		if (1 != fread (ml, sizeof (MemoList), 1, f))
		    fatal_perror ("Read error on %s", memoserv_db);
		alpha_insert_memolist (ml);
		ml->memos = memos = smalloc (sizeof (Memo) * ml->n_memos);
		fread (memos, sizeof (Memo), ml->n_memos, f);
		for (j = 0; j < ml->n_memos; ++j, ++memos)
		    memos->text = read_string (f, memoserv_db);
	    }
	break;
    default:
	fatal ("Unsupported version number (%d) on %s", i, memoserv_db);
    }				/* switch (version) */
    fclose (f);
}

void
save_ms_dbase (void)
{
    FILE *f;
    int i, j;
    MemoList *ml;
    Memo *memos;
    char memoservsave[2048];

    strcpy (memoservsave, memoserv_db);
    strcat (memoservsave, ".save");
    remove (memoservsave);
    if (rename (memoserv_db, memoservsave) < 0)
	log_perror ("Cannot back up %s", memoserv_db);
    f = fopen (memoserv_db, "w");
    if (!f)
    {
	log_perror ("Can't write to MemoServ database %s", memoserv_db);
	if (rename (memoservsave, memoserv_db) < 0)
	    fatal_perror ("Cannot restore backup copy of %s", memoserv_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't set group of %s to %d", memoserv_db, file_gid);
#endif
    write_file_version (f, memoserv_db);
    for (i = 33; i < 256; ++i)
    {
	for (ml = memolists[i]; ml; ml = ml->next)
	{
	    fputc (1, f);
	    if (1 != fwrite (ml, sizeof (MemoList), 1, f) ||
		ml->n_memos !=
		fwrite (ml->memos, sizeof (Memo), ml->n_memos, f))
		fatal_perror ("Write error on %s", memoserv_db);
	    for (memos = ml->memos, j = 0; j < ml->n_memos; ++memos, ++j)
		write_string (memos->text, f, memoserv_db);
	}
	fputc (0, f);
    }
    fclose (f);
    remove (memoservsave);
}

void
load_news_dbase (void)
{
    FILE *f = fopen (newsserv_db, "r");
    int i, j;
    NewsList *nl;
    Memo *newss;

    if (!f)
    {
	log_perror ("Can't read NewsServ database %s", newsserv_db);
	return;
    }
    switch (i = get_file_version (f, newsserv_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
	for (i = 33; i < 256; ++i)
	{
	    while (fgetc (f) == 1)
	    {
		nl = smalloc (sizeof (NewsList));
		if (1 != fread (nl, sizeof (NewsList), 1, f))
		    fatal_perror ("Read error on %s", newsserv_db);
		alpha_insert_newslist (nl);
		nl->newss = newss = smalloc (sizeof (Memo) * nl->n_newss);
		fread (newss, sizeof (Memo), nl->n_newss, f);
		for (j = 0; j < nl->n_newss; ++j, ++newss)
		    newss->text = read_string (f, newsserv_db);
	    }
	}
	break;
    default:
	fatal ("Unsupported version number (%d) on %s", i, newsserv_db);
    }				/* switch (version) */
    fclose (f);
}

void
save_news_dbase (void)
{
    FILE *f;
    int i, j;
    NewsList *nl;
    Memo *newss;
    char newsserv_dbsave[2048];

    strcpy (newsserv_dbsave, newsserv_db);
    strcat (newsserv_dbsave, ".save");
    remove (newsserv_dbsave);
    if (rename (newsserv_db, newsserv_dbsave) < 0)
	log_perror ("Cannot back up %s", newsserv_db);
    f = fopen (newsserv_db, "w");
    if (!f)
    {
	log_perror ("Can't write to NewsServ database %s", newsserv_db);
	if (rename (newsserv_dbsave, newsserv_db) < 0)
	    fatal_perror ("Cannot restore backup copy of %s", newsserv_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't set group of %s to %d", newsserv_db, file_gid);
#endif
    write_file_version (f, newsserv_db);
    for (i = 33; i < 256; ++i)
    {
	for (nl = newslists[i]; nl; nl = nl->next)
	{
	    fputc (1, f);
	    if (1 != fwrite (nl, sizeof (NewsList), 1, f) ||
		nl->n_newss !=
		fwrite (nl->newss, sizeof (Memo), nl->n_newss, f))
		fatal_perror ("Write error on %s", newsserv_db);
	    for (newss = nl->newss, j = 0; j < nl->n_newss; ++newss, ++j)
		write_string (newss->text, f, newsserv_db);
	}
	fputc (0, f);
    }
    fclose (f);
    remove (newsserv_dbsave);
}

/*************************************************************************/

/* check_memos:  See if the given nick has any waiting memos, and send a
 *               NOTICE to that nick if so.
 */

void
check_memos (const char *nick)
{
    MemoList *ml = find_memolist (nick);

    if (ml)
    {
	notice (s_MemoServ, nick, MS_YOU_HAVE, ml->n_memos,
		ml->n_memos == 1 ? "" : "s");
	notice (s_MemoServ, nick, "Type \2/msg %s %s%s\2 to %s.",
		s_MemoServ,
		ml->n_memos == 1 ? "READ " : "LIST",
		ml->n_memos == 1 ? myitoa (ml->memos->number) : "",
		ml->n_memos == 1 ? "read it" : "list them");
    }
}

/* check_newss:  See if the given chan has any waiting newss, and send a
 *               NOTICE to that chan if so.
 */

void
check_newss (const char *chan, const char *source)
{
    NewsList *nl = find_newslist (chan);
    ChannelInfo *ci = cs_findchan (chan);
    User *u = finduser (source);

    if (userisnick (source) && nl && ci)
	if (check_access (u, ci, CA_READMEMO))
	{
	    notice (s_MemoServ, source, NS_YOU_HAVE,
		    nl->n_newss == 1 ? "is" : "are",
		    nl->n_newss,
		    nl->n_newss == 1 ? "" : "s",
		    chan);
	    notice (s_MemoServ, source, "Type \2/msg %s %s %s %s%s\2to %s.",
		    s_MemoServ,
		    nl->n_newss == 1 ? "READ" : "LIST",
		    chan,
		    nl->n_newss == 1 ? myitoa (nl->newss->number) : "",
		    nl->n_newss == 1 ? " " : "",
		    nl->n_newss == 1 ? "read it" : "list them");
	}
}

/*************************************************************************/
/*********************** MemoServ private routines ***********************/
/*************************************************************************/

/* find_memolist:  Find the memo list for a given nick.  Return NULL if
 *                 none.
 */

MemoList *
find_memolist (const char *nick)
{
    MemoList *ml;
    int i = -1;
    NickInfo *ni = host (findnick (nick));

    if (ni)
    {
	for (ml = memolists[tolower (*(ni->nick))];
	   ml && (i = stricmp (ml->nick, ni->nick)) < 0;
	     ml = ml->next);
    }
    return i == 0 ? ml : NULL;
}

/* find_newslist:  Find the news list for a given nick.  Return NULL if
 *                 none.
 */

NewsList *
find_newslist (const char *chan)
{
    NewsList *nl;
    int i;

    for (nl = newslists[tolower (*chan)];
	 nl && (i = stricmp (nl->chan, chan)) < 0;
	 nl = nl->next)
	;
    return i == 0 ? nl : NULL;
}

/*************************************************************************/

/* alpha_insert_memolist:  Insert a memo list alphabetically into the
 *                         database.
 */

static void
alpha_insert_memolist (MemoList * ml)
{
    MemoList *ml2, *ml3;
    char *nick = ml->nick;

    for (ml3 = NULL, ml2 = memolists[tolower (*nick)];
	 ml2 && stricmp (ml2->nick, nick) < 0;
	 ml3 = ml2, ml2 = ml2->next)
	;
    ml->prev = ml3;
    ml->next = ml2;
    if (!ml3)
	memolists[tolower (*nick)] = ml;
    else
	ml3->next = ml;
    if (ml2)
	ml2->prev = ml;
}

/* alpha_insert_newslist:  Insert a news list alphabetically into the
 *                         database.
 */

static void
alpha_insert_newslist (NewsList * nl)
{
    NewsList *nl2, *nl3;
    char *chan = nl->chan;

    for (nl3 = NULL, nl2 = newslists[tolower (*chan)];
	 nl2 && stricmp (nl2->chan, chan) < 0;
	 nl3 = nl2, nl2 = nl2->next)
	;
    nl->prev = nl3;
    nl->next = nl2;
    if (!nl3)
	newslists[tolower (*chan)] = nl;
    else
	nl3->next = nl;
    if (nl2)
	nl2->prev = nl;
}

/*************************************************************************/

/* del_memolist:  Remove a nick's memo list from the database.  Assumes
 *                that the memo count for the nick is non-zero.
 */

void
del_memolist (MemoList * ml)
{
    if (!ml) return;
    if (ml->next)
	ml->next->prev = ml->prev;
    if (ml->prev)
	ml->prev->next = ml->next;
    else
	memolists[tolower (*ml->nick)] = ml->next;
    if (ml->memos)
	free (ml->memos);
    free (ml);
}

/* del_newslist:  Remove a nick's news list from the database.  Assumes
 *                that the news count for the nick is non-zero.
 */

void
del_newslist (NewsList * nl)
{
    if (!nl) return;
    if (nl->next)
	nl->next->prev = nl->prev;
    if (nl->prev)
	nl->prev->next = nl->next;
    else
	newslists[tolower (*nl->chan)] = nl->next;
    if (nl->newss)
	free (nl->newss);
    free (nl);
}

/*************************************************************************/
/*********************** MemoServ command routines ***********************/
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
	    {"*OPER*", H_OPER, opersend_help},
	    {"*S*OP*", H_SOP, sopsend_help},
	    {NULL}
	};

	if (command = get_help_hash (source, cmd, hash_table))
	    notice_list (s_MemoServ, source, command->process);
	else
	{
	    snprintf (buf, sizeof(buf), "%s%s", s_MemoServ, cmd ? " " : "");
	    strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	    helpserv (s_MemoServ, source, buf);
	}
    }
    else
    {
	snprintf (buf, sizeof(buf), "%s%s", s_MemoServ, cmd ? " " : "");
	strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	helpserv (s_MemoServ, source, buf);
    }
}

/*************************************************************************/

static int
candomemo (const char *source, const char *other)
{
    NickInfo *ni, *hni;

    if (!(ni = findnick (source)) || !(hni = host (ni)))
	notice (s_MemoServ, source, NS_YOU_NOT_REGISTERED);

    else if (!userisnick (source))
	notice (s_MemoServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);

    else if (other)
	if (!findnick (other))
	    notice (s_MemoServ, source, NS_NOT_REGISTERED, other);
	else if (is_on_ignore (source, other) && !(hni->flags & NI_IRCOP))
	    notice (s_MemoServ, source, NS_AM_IGNORED, other);
	else
	    return 1;

    else
	return 1;

    return 0;
}

static int
candonews (const char *source, const char *chan, int access)
{
    NickInfo *ni;
    ChannelInfo *ci;
    User *u;

    if (!(ni = findnick (source)))
	notice (s_MemoServ, source, NS_YOU_NOT_REGISTERED);

    else if (!userisnick (source))
	notice (s_MemoServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);

    else if (!(ci = cs_findchan (chan)))
	notice (s_MemoServ, source, CS_NOT_REGISTERED, chan);

    else if (!(u = finduser (source)) || !check_access (u, ci, access))
	notice (s_MemoServ, source, ERR_ACCESS_DENIED);

    else
	return 1;

    return 0;
}

/*************************************************************************/

/* Send a memo to a nick. */

static void
do_send (const char *source)
{
    Memo *m;
    char *arg = strtok (NULL, " ");
    char *text = strtok (NULL, "");

    if (!text)
    {
	if (news_on == TRUE)
	    notice (s_MemoServ, source, "Syntax: \2SEND \37nick|channel\37 \37memo-text\37\2");
	else
	    notice (s_MemoServ, source, "Syntax: \2SEND \37nick\37 \37memo-text\37\2");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "SEND");

    }
    else if (news_on == TRUE && validchan(arg))
    {
	    if (candonews (source, arg, CA_WRITEMEMO))
	    {
		Channel *c;

		NewsList *nl = find_newslist (arg);
		if (!nl)
		{
		    nl = scalloc (sizeof (NewsList), 1);
		    strscpy (nl->chan, arg, CHANMAX);
		    alpha_insert_newslist (nl);
		}
		++nl->n_newss;
		nl->newss = srealloc (nl->newss, sizeof (Memo) * nl->n_newss);
		m = &nl->newss[nl->n_newss - 1];
		strscpy (m->sender, source, CHANMAX);
		if (nl->n_newss > 1)
		{
		    m->number = m[-1].number + 1;
		    if (m->number < 1)
		    {
			int i;
			for (i = 0; i < nl->n_newss; ++i)
			    nl->newss[i].number = i + 1;
		    }
		}
		else
		    nl->newss[nl->n_newss - 1].number = 1;
		m->time = time (NULL);
		m->text = sstrdup (text);
		notice (s_MemoServ, source, NS_SEND, arg);
		if (c = findchan (arg))
		{
		    struct c_userlist *ul;
		    ChannelInfo *ci = cs_findchan (arg);
		    for (ul = c->users; ul; ul = ul->next)
			if (check_access (ul->user, ci, CA_READMEMO))
			{
			    notice (s_MemoServ, ul->user->nick, NS_NEW,
				    m->number, arg, source);
			    notice (s_MemoServ, ul->user->nick, NS_READ_NEW,
				    s_MemoServ, arg, m->number);
			}
		}
	    }
    }
    else if (memos_on == TRUE)
    {
	if (candomemo (source, arg))
	{
	    int i;
	    NickInfo *hni = host (findnick (arg));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
	    {
		ml = scalloc (sizeof (MemoList), 1);
		strscpy (ml->nick, hni->nick, NICKMAX);
		alpha_insert_memolist (ml);
	    }
	    ++ml->n_memos;
	    ml->memos = srealloc (ml->memos, sizeof (Memo) * ml->n_memos);
	    m = &ml->memos[ml->n_memos - 1];
	    strscpy (m->sender, source, NICKMAX);
	    if (ml->n_memos > 1)
	    {
		m->number = m[-1].number + 1;
		if (m->number < 1)
		{
		    int i;
		    for (i = 0; i < ml->n_memos; ++i)
			ml->memos[i].number = i + 1;
		}
	    }
	    else
		ml->memos[ml->n_memos - 1].number = 1;
	    m->time = time (NULL);
	    m->text = sstrdup (text);
	    notice (s_MemoServ, source, MS_SEND,
		    hni->nick, findnick (arg)->nick);
	    if (userisnick (hni->nick))
	    {
		notice (s_MemoServ, hni->nick, MS_NEW, "",
			m->number, source);
		notice (s_MemoServ, hni->nick, MS_READ_NEW,
			s_MemoServ, m->number);
	    }
	    i = slavecount (hni->nick);
	    for (i = slavecount (hni->nick); i; i--)
		if (userisnick (slave (hni->nick, i)->nick))
		{
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_NEW,
			    "", m->number, source);
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_READ_NEW,
			    s_MemoServ, m->number);
		}
	}
    }
}

/*************************************************************************/

static void
do_opersend (const char *source)
{
    char *text = strtok (NULL, "");

    if (!text)
    {
	notice (s_MemoServ, source, "Syntax: \2OPERSEND \37memo-text\37\2");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "OPERSEND");
    }
    else
    {
	if (candomemo (source, NULL))
	{
	    int i, j;
	    MemoList *ml;
	    Memo *m;
	    NickInfo *hni, *ni = host (findnick (source));
	    char sender[NICKMAX];
	    char *text2;
	    text2=smalloc(strlen(text) + 16);
	    if (!ni)
	    {
		notice (s_MemoServ, source, NS_YOU_NOT_REGISTERED);
		free(text2);
		return;
	    }
	    else if (!(ni->flags & NI_IRCOP))
	    {
		notice (s_MemoServ, source, ERR_MUST_BE_IRCOP, "OPERSEND");
		free(text2);
		return;
	    }
	    strscpy (sender, ni->nick, NICKMAX);
	    for (i = 33; i < 256; ++i)
	    {
		for (ni = nicklists[i]; ni; ni = ni->next)
		{
		    if ((ni->flags & NI_IRCOP) && stricmp (ni->nick, sender) != 0)
		    {
			ml = find_memolist (ni->nick);
			if (!ml)
			{
			    ml = scalloc (sizeof (MemoList), 1);
			    strscpy (ml->nick, ni->nick, NICKMAX);
			    alpha_insert_memolist (ml);
			}
			++ml->n_memos;
			ml->memos = srealloc (ml->memos, sizeof (Memo) * ml->n_memos);
			m = &ml->memos[ml->n_memos - 1];
			strscpy (m->sender, source, NICKMAX);
			if (ml->n_memos > 1)
			{
			    m->number = m[-1].number + 1;
			    if (m->number < 1)
			    {
				for (j = 0; j < ml->n_memos; ++j)
				    ml->memos[j].number = j + 1;
			    }
			}
			else
			    ml->memos[ml->n_memos - 1].number = 1;
			snprintf (text2, strlen(text) + 16, "\37[\37\2OPER\2\37]\37 %s", text);
			m->time = time (NULL);
			m->text = sstrdup (text2);
			hni = host (ni);
			if (userisnick (hni->nick))
			{
			    notice (s_MemoServ, hni->nick, MS_NEW, "OPER", m->number, source);
			    notice (s_MemoServ, hni->nick, MS_READ_NEW, s_MemoServ, m->number);
			}
			for (j = slavecount (hni->nick); j; j--)
			{
			    if (userisnick (slave (hni->nick, j)->nick))
			    {
				notice (s_MemoServ, slave (hni->nick, j)->nick, MS_NEW, "OPER", m->number, source);
				notice (s_MemoServ, slave (hni->nick, j)->nick, MS_READ_NEW, s_MemoServ, m->number);
			    }
			}
		    }
		}
	    }
	    notice (s_MemoServ, source, MS_MASS_SEND, "IRC Operators");
	    free(text2);
	}
    }
}

static void
do_sopsend (const char *source)
{
    char *text = strtok (NULL, "");

    if (!text)
    {
	notice (s_MemoServ, source, "Syntax: \2SOPSEND \37memo-text\37\2");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "SOPSEND");

    }
    else
    {
	if (candomemo (source, NULL))
	{
	    int i, j;
	    MemoList *ml;
	    Memo *m;
	    NickInfo *hni, *ni = host (findnick (source));
	    char sender[NICKMAX];
	    char *text2;
	    text2=smalloc(strlen (text) + 16);
	    snprintf (text2, strlen (text) + 16, "\37[\37\2SOP\2\37]\37 %s", text);
	    if (!ni)
	    {
		notice (s_MemoServ, source, NS_YOU_NOT_REGISTERED);
		free(text2);
		return;
	    }
	    strscpy (sender, ni->nick, NICKMAX);
	    for (i = 0; i < nsop; ++i)
	    {
		if (stricmp (sender, sops[i].nick) != 0)
		{
		    ml = find_memolist (sops[i].nick);
		    if (!ml)
		    {
			ml = scalloc (sizeof (MemoList), 1);
			strscpy (ml->nick, sops[i].nick, NICKMAX);
			alpha_insert_memolist (ml);
		    }
		    ++ml->n_memos;
		    ml->memos = srealloc (ml->memos, sizeof (Memo) * ml->n_memos);
		    m = &ml->memos[ml->n_memos - 1];
		    strscpy (m->sender, source, NICKMAX);
		    if (ml->n_memos > 1)
		    {
			m->number = m[-1].number + 1;
			if (m->number < 1)
			{
			    for (j = 0; j < ml->n_memos; ++j)
				ml->memos[j].number = j + 1;
			}
		    }
		    else
			ml->memos[ml->n_memos - 1].number = 1;
		    m->time = time (NULL);
		    m->text = sstrdup (text2);
		    hni = host (findnick (sops[i].nick));
		    if (userisnick (hni->nick))
		    {
			notice (s_MemoServ, hni->nick, MS_NEW, "SOP", m->number, source);
			notice (s_MemoServ, hni->nick, MS_READ_NEW, s_MemoServ, m->number);
		    }
		    for (j = slavecount (hni->nick); j; j--)
		    {
			if (userisnick (slave (hni->nick, j)->nick))
			{
			    notice (s_MemoServ, slave (hni->nick, j)->nick, MS_NEW, "SOP", m->number, source);
			    notice (s_MemoServ, slave (hni->nick, j)->nick, MS_READ_NEW, s_MemoServ, m->number);
			}
		    }
		}
	    }
	    notice (s_MemoServ, source, MS_MASS_SEND, "Services Operators");
	    free(text2);
	}
    }
}

/*************************************************************************/

/* List the memos (if any) for the source nick. */

static void
do_list (const char *source)
{
    Memo *m;
    int i;
    char *arg = strtok (NULL, "");

    if (news_on == TRUE && arg && validchan(arg))
    {
	    if (candonews (source, arg, CA_READMEMO))
	    {
		NewsList *nl = find_newslist(arg);
		if (!nl)
		    notice (s_MemoServ, source, NS_YOU_DONT_HAVE, arg);
		else
		{
		    notice (s_MemoServ, source, NS_LIST, s_MemoServ, arg);
		    notice (s_MemoServ, source, "Num  Sender            Waiting for");
		    for (i = 0, m = nl->newss; i < nl->n_newss; ++i, ++m)
			notice (s_MemoServ, source, "%3d  %-16s  %s",
				m->number, m->sender, time_ago (m->time, 1));
		}
	    }
    }
    else if (memos_on == TRUE)
    {
	if (candomemo (source, NULL))
	{
	    NickInfo *hni = host(findnick(source));
	    MemoList *ml = find_memolist(hni->nick);
	    if (!ml)
		notice (s_MemoServ, source, MS_YOU_DONT_HAVE);
	    else
	    {
		notice (s_MemoServ, source, MS_LIST, s_MemoServ);
		notice (s_MemoServ, source, "Num  Sender            Waiting for");
		for (i = 0, m = ml->memos; i < ml->n_memos; ++i, ++m)
		{
		    notice (s_MemoServ, source, "%3d  %-16s  %s",
			    m->number, m->sender, time_ago (m->time, 1));
		}
	    }
	}
    }
}

/*************************************************************************/

/* Read a memo. */
/* took out #ifdef STUPID, ie, always assume stupid, just to be on the safe side */

static void
do_read (const char *source)
{
    Memo *m;
    char numstr[8];
    int num;
    char *arg = strtok (NULL, " ");
    char *arg2 = strtok (NULL, "");

    if (arg)
    {
	if (news_on == TRUE)
	{
	    if (!arg2)
	    {
		strscpy (numstr, arg, sizeof(numstr));
	    }
	    else
		strscpy (numstr, arg2, sizeof(numstr));
	}
	else
	    strscpy (numstr, arg, sizeof(numstr));
    }
	if (!arg)
	{
	    notice (s_MemoServ, source, "Syntax: \2READ \37num|all\37");
	    if (news_on == TRUE)
		notice (s_MemoServ, source, "Syntax: \2READ channel \37num|all\37");
		notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "READ");
	}
	else if (!numstr || ((num = atoi (numstr)) <= 0 && stricmp (numstr, "ALL") != 0))
	{
	    if (news_on==TRUE)
	    {
		if (!validchan(arg))
		{
		    notice (s_MemoServ, source, "Syntax: \2READ \37num|all\37");
		}
		else
		    notice (s_MemoServ, source, "Syntax: \2READ channel \37num|all\37");
	    }
	    else
		notice (s_MemoServ, source, "Syntax: \2READ \37num|all\37");
	    notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "READ");
	}
	else if (news_on == TRUE && validchan(arg))
	{
		if (candonews (source, arg, CA_READMEMO))
		{
		    NewsList *nl = find_newslist(arg);
		    if (!nl)
			notice (s_MemoServ, source, NS_YOU_DONT_HAVE, arg);
		    else if (num > 0)
		    {
			int i;
			for (i = 0; i < nl->n_newss; ++i)
			    if (nl->newss[i].number == num)
				break;
			if (i >= nl->n_newss)
			    notice (s_MemoServ, source, NS_DOESNT_EXIST, num, arg);
			else
			{
			    m = &nl->newss[i];
			    notice (s_MemoServ, source, NS_MEMO,
				    m->number, arg, m->sender, time_ago (m->time, 1));
			    notice (s_MemoServ, source, "%s", m->text);
			}
		    }
		    else
		    {
			int i;
			for (i = 0; i < nl->n_newss; ++i)
			{
			    m = &nl->newss[i];
			    notice (s_MemoServ, source, NS_MEMO,
				    m->number, arg, m->sender, time_ago (m->time, 1));
			    notice (s_MemoServ, source, "%s", m->text);
			}
		    }
		}
	}
	else if (memos_on == TRUE)
	{
	    if (candomemo (source, NULL))
	    {
		NickInfo *hni= host(findnick(source));
		MemoList *ml = find_memolist (hni->nick);
		if (!ml)
		    notice (s_MemoServ, source, MS_YOU_DONT_HAVE);
		else if (num > 0)
		{
		    int i;
		    for (i = 0; i < ml->n_memos; ++i)
			if (ml->memos[i].number == num)
			    break;
		    if (i >= ml->n_memos)
			notice (s_MemoServ, source, MS_DOESNT_EXIST, num);
		    else
		    {
			m = &ml->memos[i];
			notice (s_MemoServ, source, MS_MEMO,
				m->number, m->sender, time_ago (m->time, 1));
			notice (s_MemoServ, source, "%s", m->text);
			notice (s_MemoServ, source, MS_TODEL,
				s_MemoServ, m->number);
		    }
		}
		else
		{
		    int i;
		    for (i = 0; i < ml->n_memos; ++i)
		    {
			m = &ml->memos[i];
			notice (s_MemoServ, source, MS_MEMO,
				m->number, m->sender, time_ago (m->time, 1));
			notice (s_MemoServ, source, "%s", m->text);
		    }
		    notice (s_MemoServ, source, MS_TODEL_ALL, s_MemoServ);
		}
	    }
	}
}

/* Forward a memo. */

static void
do_forward (const char *source)
{
    Memo *m;
    char numstr[8];
    int num;
    char *arg = strtok (NULL, " ");
    char *arg2 = strtok (NULL, " ");
    char *arg3 = strtok (NULL, "");


    if (arg)
    {
	if (news_on==TRUE)
	{
	    if (arg2)
	    {
		if (!arg3)
		    strscpy (numstr, arg, 8);
		else
		    strscpy (numstr, arg2, 8);
	    }
	}
	else
	    strscpy (numstr, arg, 8);
    }

    if (!arg2)
    {
	if (news_on == TRUE)
	{
	    notice (s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user|channel");
	    notice (s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
	}
	else
	    notice (s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "FORWARD");

    }
    else if (!numstr || (num = atoi (numstr)) <= 0)
    {
	if (news_on == TRUE)
	{
	    if (!validchan(arg))
		notice (s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user|channel");
	    else
		notice (s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
	}
	else
	    notice (s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "FORWARD");

    }
    else if (news_on == TRUE && validchan(arg))
    {
	    if (!arg3)
	    {
		notice (s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
		notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "FORWARD");

	    }
	    else if (candonews (source, arg, CA_READMEMO))
	    {
		NewsList *nl = find_newslist (arg);
		if (!nl)
		    notice (s_MemoServ, source, NS_YOU_DONT_HAVE, arg);
		else
		{
		    int i;
		    for (i = 0; i < nl->n_newss; ++i)
			if (nl->newss[i].number == num)
			    break;
		    if (i >= nl->n_newss)
			notice (s_MemoServ, source, NS_DOESNT_EXIST, num, arg);
		    else
		    {
			ChannelInfo *ci = cs_findchan (arg);
			char s[NICKMAX + CHANMAX + 2];
			m = &nl->newss[i];
			snprintf (s, sizeof (s), "%s/%s", m->sender, ci->name);
			do_forward_send (source, s, arg3, m->text);
		    }
		}
	    }
    }
    else if (memos_on == TRUE)
    {
	if (candomemo (source, NULL))
	{
	    NickInfo *hni = host(findnick(source));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
		notice (s_MemoServ, source, MS_YOU_DONT_HAVE);
	    else
	    {
		int i;
		for (i = 0; i < ml->n_memos; ++i)
		    if (ml->memos[i].number == num)
			break;
		if (i >= ml->n_memos)
		    notice (s_MemoServ, source, MS_DOESNT_EXIST, num);
		else
		{
		    m = &ml->memos[i];
		    if (news_on == TRUE)
		    {
			if (arg3)
			    do_forward_send (source, m->sender, arg3, m->text);
			else
			    do_forward_send (source, m->sender, arg2, m->text);
		    }
		    else
			do_forward_send (source, m->sender, arg2, m->text);
		}
	    }
	}
    }
}

static void
do_forward_send (const char *source, const char *origin, char *arg, const char *intext)
{
    Memo *m;
    char *text;
    text=smalloc(strlen (origin) + strlen (intext) + 10);
    snprintf (text, strlen (origin) + strlen (intext) + 10, "[FWD: %s] %s", origin, intext);

    if (news_on == TRUE && validchan(arg))
    {
	    if (candonews (source, arg, CA_WRITEMEMO))
	    {
		Channel *c;
		NewsList *nl = find_newslist (arg);
		if (!nl)
		{
		    nl = scalloc (sizeof (NewsList), 1);
		    strscpy (nl->chan, arg, CHANMAX);
		    alpha_insert_newslist (nl);
		}
		++nl->n_newss;
		nl->newss = srealloc (nl->newss, sizeof (Memo) * nl->n_newss);
		m = &nl->newss[nl->n_newss - 1];
		strscpy (m->sender, source, CHANMAX);
		if (nl->n_newss > 1)
		{
		    m->number = m[-1].number + 1;
		    if (m->number < 1)
		    {
			int i;
			for (i = 0; i < nl->n_newss; ++i)
			    nl->newss[i].number = i + 1;
		    }
		}
		else
		    nl->newss[nl->n_newss - 1].number = 1;
		m->time = time (NULL);
		m->text = sstrdup (text);
		notice (s_MemoServ, source, NS_SEND, arg);
		if (c = findchan (arg))
		{
		    struct c_userlist *ul;
		    ChannelInfo *ci = cs_findchan (arg);
		    for (ul = c->users; ul; ul = ul->next)
			if (check_access (ul->user, ci, CA_READMEMO))
			{
			    notice (s_MemoServ, ul->user->nick, NS_NEW,
				    m->number, arg, source);
			    notice (s_MemoServ, ul->user->nick, NS_READ_NEW,
				    s_MemoServ, arg, m->number);
			}
		}
	    }
    }
    else if (memos_on == TRUE)
    {
	if (candomemo (source, arg))
	{
	    int i;
	    NickInfo *hni = host(findnick(arg));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
	    {
		ml = scalloc (sizeof (MemoList), 1);
		strscpy (ml->nick, hni->nick, NICKMAX);
		alpha_insert_memolist (ml);
	    }
	    ++ml->n_memos;
	    ml->memos = srealloc (ml->memos, sizeof (Memo) * ml->n_memos);
	    m = &ml->memos[ml->n_memos - 1];
	    strscpy (m->sender, source, NICKMAX);
	    if (ml->n_memos > 1)
	    {
		m->number = m[-1].number + 1;
		if (m->number < 1)
		{
		    int i;
		    for (i = 0; i < ml->n_memos; ++i)
			ml->memos[i].number = i + 1;
		}
	    }
	    else
		ml->memos[ml->n_memos - 1].number = 1;
	    m->time = time (NULL);
	    m->text = sstrdup (text);
	    notice (s_MemoServ, source, MS_SEND,
		    hni->nick, findnick (arg)->nick);
	    if (userisnick (hni->nick))
	    {
		notice (s_MemoServ, hni->nick, MS_NEW,
			"", m->number, source);
		notice (s_MemoServ, hni->nick, MS_READ_NEW,
			s_MemoServ, m->number);
	    }
	    i = slavecount (hni->nick);
	    for (i = slavecount (hni->nick); i; i--)
		if (userisnick (slave (hni->nick, i)->nick))
		{
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_NEW,
			    "", m->number, source);
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_READ_NEW,
			    s_MemoServ, m->number);
		}
	}
    }
    free(text);
}

/* Reply to a memo. */

static void
do_reply (const char *source)
{
    Memo *m;
    char numstr[8];
    int num;
    char *arg = strtok (NULL, " ");
    char *text = strtok (NULL, "");


    if (arg)
	strscpy (numstr, arg, 8);

    if (!text || !numstr || (num = atoi (numstr)) <= 0)
    {
	notice (s_MemoServ, source, "Syntax: \2REPLY \37num memo\37");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "REPLY");

    }
    else if (news_on == TRUE && validchan(arg))
	    notice(s_MemoServ, source, NS_MAY_NOT, "reply to");
    else if (memos_on == TRUE)
    {
	if (candomemo (source, NULL))
	{
	    NickInfo *hni = host(findnick(source));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
		notice (s_MemoServ, source, MS_YOU_DONT_HAVE);
	    else
	    {
		int i;
		for (i = 0; i < ml->n_memos; ++i)
		    if (ml->memos[i].number == num)
			break;
		if (i >= ml->n_memos)
		    notice (s_MemoServ, source, MS_DOESNT_EXIST, num);
		else
		{
		    char sender[NICKMAX];
		    m = &ml->memos[i];
		    strscpy(sender, m->sender, NICKMAX);
		    do_reply_send (source, sender, text);
		}
	    }
	}
    }
}

static void
do_reply_send (const char *source, const char *origin, const char *intext)
{
    Memo *m;
    char *text;
    text=smalloc(strlen (intext) + 10);
    snprintf (text, strlen (intext) + 10, "[REPLY] %s", intext);

	if (candomemo (source, origin))
	{
	    int i;
	    NickInfo *hni = host(findnick(origin));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
	    {
		ml = scalloc (sizeof (MemoList), 1);
		strscpy (ml->nick, hni->nick, NICKMAX);
		alpha_insert_memolist (ml);
	    }
	    ++ml->n_memos;
	    ml->memos = srealloc (ml->memos, sizeof (Memo) * ml->n_memos);
	    m = &ml->memos[ml->n_memos - 1];
	    strscpy (m->sender, source, NICKMAX);
	    if (ml->n_memos > 1)
	    {
		m->number = m[-1].number + 1;
		if (m->number < 1)
		{
		    int i;
		    for (i = 0; i < ml->n_memos; ++i)
			ml->memos[i].number = i + 1;
		}
	    }
	    else
		ml->memos[ml->n_memos - 1].number = 1;
	    m->time = time (NULL);
	    m->text = sstrdup (text);
	    notice (s_MemoServ, source, MS_SEND,
		    hni->nick, findnick (origin)->nick);
	    if (userisnick (hni->nick))
	    {
		notice (s_MemoServ, hni->nick, MS_NEW,
			"", m->number, source);
		notice (s_MemoServ, hni->nick, MS_READ_NEW,
			s_MemoServ, m->number);
	    }
	    i = slavecount (hni->nick);
	    for (i = slavecount (hni->nick); i; i--)
		if (userisnick (slave (hni->nick, i)->nick))
		{
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_NEW,
			    "", m->number, source);
		    notice (s_MemoServ, slave (hni->nick, i)->nick, MS_READ_NEW,
			    s_MemoServ, m->number);
		}
	}
    free(text);
}

/*************************************************************************/

/* Delete a memo. */

static void
do_del (const char *source)
{
    char *arg = strtok (NULL, " ");
    char *arg2 = strtok (NULL, "");
    char numstr[8];
    int num, i;

    if (arg)
    {
	if (news_on == TRUE)
	{
	    if (!arg2)
	    {
		strscpy (numstr, arg, sizeof(numstr));
	    }
	    else
		strscpy (numstr, arg2, sizeof(numstr));
	}
	else
	    strscpy (numstr, arg, sizeof(numstr));
    }

    if (!arg)
    {
	notice (s_MemoServ, source, "Syntax: \2DEL {\37num\37 | ALL}");
	if (news_on == TRUE)
	    notice (s_MemoServ, source, "Syntax: \2DEL channel {\37num\37 | ALL}");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "DEL");
    }
    else if (!numstr ||
	     ((num = atoi (numstr)) <= 0 && stricmp (numstr, "ALL") != 0))
    {
	if (news_on == TRUE)
	{
	    if (!validchan(arg))
		notice (s_MemoServ, source, "Syntax: \2DEL {\37num\37 | ALL}");
	    else
		notice (s_MemoServ, source, "Syntax: \2DEL channel {\37num\37 | ALL}");
	}
	else
	    notice (s_MemoServ, source, "Syntax: \2DEL {\37num\37 | ALL}");
	notice (s_MemoServ, source, ERR_MORE_INFO, s_MemoServ, "DEL");
    }
    else if (news_on == TRUE && validchan(arg))
    {
	    if (candonews (source, arg, CA_WRITEMEMO))
	    {
		NewsList *nl = find_newslist (arg);
		if (!nl)
		    notice (s_MemoServ, source, NS_YOU_DONT_HAVE, arg);
		else
		{
		    User *u = finduser (source);
		    ChannelInfo *ci = cs_findchan (arg);
		    if (num > 0)
		    {
			/* Delete a specific news. */
			for (i = 0; i < nl->n_newss; ++i)
			    if (nl->newss[i].number == num)
				break;
			if (i < nl->n_newss)
			{
			    if ((stricmp (nl->newss[i].sender, source) == 0) || (check_access (u, ci, CA_DELMEMO)))
			    {
				if (nl->newss[i].text)
				    free (nl->newss[i].text);	/* Deallocate news text newsry */
				--nl->n_newss;	/* One less news now */
				if (i < nl->n_newss)	/* Move remaining newss down a slot */
				    bcopy (nl->newss + i + 1, nl->newss + i,
					 sizeof (Memo) * (nl->n_newss - i));
				notice (s_MemoServ, source, NS_DELETE, num, arg);
			    }
			    else
				notice (s_MemoServ, source, ERR_ACCESS_DENIED);
			}
			else
			    notice (s_MemoServ, source, NS_DOESNT_EXIST, num, arg);
		    }
		    else
		    {
			/* Delete all newss.  This requires freeing the newsry holding
			   * the text of each news and flagging that there are no newss
			   * left. */
			if (!check_access (u, ci, CA_DELMEMO))
			    notice (s_MemoServ, source, ERR_ACCESS_DENIED);
			else
			{
			    for (i = 0; i < nl->n_newss; ++i)
				if (nl->newss[i].text)
				    free (nl->newss[i].text);
			    nl->n_newss = 0;
			    notice (s_MemoServ, source, NS_DELETE_ALL, arg);
			}
		    }
		    /* Did we delete the last news?  If so, delete this NewsList. */
		    if (nl->n_newss == 0)
			del_newslist (nl);
		}
	    }
    }
    else if (memos_on)
    {
	if (candomemo (source, NULL))
	{
	    NickInfo *hni = host(findnick(source));
	    MemoList *ml = find_memolist (hni->nick);
	    if (!ml)
		notice (s_MemoServ, source, MS_YOU_DONT_HAVE);
	    else
	    {
		if (num > 0)
		{
		    /* Delete a specific memo. */
		    for (i = 0; i < ml->n_memos; ++i)
			if (ml->memos[i].number == num)
			    break;
		    if (i < ml->n_memos)
		    {
			if (ml->memos[i].text)
			    free (ml->memos[i].text);	/* Deallocate memo text memory */
			--ml->n_memos;	/* One less memo now */
			if (i < ml->n_memos)	/* Move remaining memos down a slot */
			    bcopy (ml->memos + i + 1, ml->memos + i,
				   sizeof (Memo) * (ml->n_memos - i));
			notice (s_MemoServ, source, MS_DELETE, num);
		    }
		    else
			notice (s_MemoServ, source, MS_DOESNT_EXIST, num);
		}
		else
		{
		    /* Delete all memos.  This requires freeing the memory holding
		       * the text of each memo and flagging that there are no memos
		       * left. */
		    for (i = 0; i < ml->n_memos; ++i)
			if (ml->memos[i].text)
			    free (ml->memos[i].text);
		    ml->n_memos = 0;
		    notice (s_MemoServ, source, MS_DELETE_ALL);
		}
		/* Did we delete the last memo?  If so, delete this MemoList. */
		if (ml->n_memos == 0)
		    del_memolist (ml);
	    }
	}
    }
}

void
expire_news ()
{
    NewsList *nl;
    Memo *m;
    ChannelInfo *ci;
    int i, j;
    const time_t expire_time = news_expire * 24 * 60 * 60;
    time_t tm;

    for (i = 33; i < 256; ++i)
    {
	for (ci = chanlists[i]; ci; ci = ci->next)
	{
	    nl = find_newslist (ci->name);
	    if (nl) {
		for (j = 0, m = nl->newss; j < nl->n_newss; ++j, ++m)
		{
		    tm = time (NULL) - m->time;
		    if (tm > expire_time)
		    {
			if (nl->newss[j].text)
			    free (nl->newss[j].text);	/* Deallocate news text newsry */
			--nl->n_newss;	/* One less news now */
			if (j < nl->n_newss)	/* Move remaining newss down a slot */
			    bcopy (nl->newss + j + 1, nl->newss + j,
				   sizeof (Memo) * (nl->n_newss - j));
			write_log ("Expiring news article %d for channel %s", j+1, ci->name);
			j--;
		    }
		}
		if (nl->n_newss == 0)
		    del_newslist (nl);
	    }
	}
    }
}
