/* MemoServ functions.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

#ifdef MEMOSERV

#ifdef MEMOS
MemoList *memolists[256];	/* One for each initial character */
#endif
#ifdef NEWS
NewsList *newslists[256];	/* One for each initial character */
#endif

const char s_MemoServ[] = MEMOSERV_NAME;

#include "ms-help.c"

#ifdef MEMOS
MemoList *find_memolist(const char *nick);	/* Needed by NICKSERV */
static void alpha_insert_memolist(MemoList *ml);
static void del_memolist(MemoList *ml);
#endif /* MEMOS */

#ifdef NEWS
NewsList *find_newslist(const char *chan);	/* Needed by CHANSERV */
static void alpha_insert_newslist(NewsList *nl);
static void del_newslist(NewsList *nl);
#endif /* NEWS */

static void do_help(const char *source);
static void do_send(const char *source);
static void do_forward(const char *source);
static void do_fwd2(const char *source, const char *origin, char *arg, const char *intext);
static void do_list(const char *source);
static void do_read(const char *source);
static void do_del(const char *source);

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

#ifdef MEMOS
void get_memoserv_stats(long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    MemoList *ml;

    for (i = 0; i < 256; i++) {
	for (ml = memolists[i]; ml; ml = ml->next) {
	    count++;
	    mem += sizeof(*ml);
	    mem += sizeof(Memo) * ml->n_memos;
	    for (j = 0; j < ml->n_memos; j++) {
		if (ml->memos[j].text)
		    mem += strlen(ml->memos[j].text)+1;
	    }
	}
    }
    *nrec = count;
    *memuse = mem;
}
#endif /* MEMOS */

#ifdef NEWS
void get_newsserv_stats(long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    NewsList *nl;

    for (i = 0; i < 256; i++) {
	for (nl = newslists[i]; nl; nl = nl->next) {
	    count++;
	    mem += sizeof(*nl);
	    mem += sizeof(Memo) * nl->n_newss;
	    for (j = 0; j < nl->n_newss; j++) {
		if (nl->newss[j].text)
		    mem += strlen(nl->newss[j].text)+1;
	    }
	}
    }
    *nrec = count;
    *memuse = mem;
}
#endif /* NEWS */

/*************************************************************************/
/*************************************************************************/

/* memoserv:  Main MemoServ routine. */

void memoserv(const char *source, char *buf)
{
    char *cmd, *s;
    NickInfo *ni;

    cmd = strtok(buf, " ");

    if (!cmd) {
	return;

    } else if (stricmp(cmd, "\1PING") == 0) {
	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_MemoServ, source, "\1PING %s", s);

    } else if (stricmp(cmd, "HELP") == 0) {
	do_help(source);

    } else if (stricmp(cmd, "SEND") == 0) {
	do_send(source);

    } else if (stricmp(cmd, "LIST") == 0) {
	do_list(source);

    } else if (stricmp(cmd, "READ") == 0) {
	do_read(source);

    } else if (stricmp(cmd, "FORWARD") == 0) {
	do_forward(source);

    } else if (stricmp(cmd, "DEL") == 0) {
	do_del(source);

/*
    } else if (!is_services_op(source)) {
	notice(s_MemoServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_MemoServ);
*/

    } else {
	notice(s_MemoServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_MemoServ);

    }
}

/*************************************************************************/

/* load_ms_dbase, save_ms_dbase:  Load/save memo data. */

#ifdef MEMOS
void load_ms_dbase(void)
{
    FILE *f = fopen(MEMOSERV_DB, "r");
    int i, j, len;
    MemoList *ml;
    Memo *memos;

    if (!f) {
	log_perror("Can't read MemoServ database " MEMOSERV_DB);
	return;
    }
    switch (i = get_file_version(f, MEMOSERV_DB)) {
      case 4:
      case 3:
      case 2:
      case 1:
	for (i = 33; i < 256; ++i) {
	    while (fgetc(f) == 1) {
		ml = smalloc(sizeof(MemoList));
		if (1 != fread(ml, sizeof(MemoList), 1, f))
		    fatal_perror("Read error on " MEMOSERV_DB);
		alpha_insert_memolist(ml);
		ml->memos = memos = smalloc(sizeof(Memo) * ml->n_memos);
		fread(memos, sizeof(Memo), ml->n_memos, f);
		for (j = 0; j < ml->n_memos; ++j, ++memos)
		    memos->text = read_string(f, MEMOSERV_DB);
	    }
	}
	break;
      default:
	fatal("Unsupported version number (%d) on %s", i, MEMOSERV_DB);
    } /* switch (version) */
    fclose(f);
}

void save_ms_dbase(void)
{
    FILE *f;
    int i, j, len;
    MemoList *ml;
    Memo *memos;

    remove(MEMOSERV_DB ".save");
    if (rename(MEMOSERV_DB, MEMOSERV_DB ".save") < 0)
	log_perror("Cannot back up " MEMOSERV_DB);
    f = fopen(MEMOSERV_DB, "w");
    if (!f) {
	log_perror("Can't write to MemoServ database " MEMOSERV_DB);
	if (rename(MEMOSERV_DB ".save", MEMOSERV_DB) < 0)
	    fatal_perror("Cannot restore backup copy of " MEMOSERV_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't set group of %s to %d", MEMOSERV_DB, file_gid);
    write_file_version(f, MEMOSERV_DB);
    for (i = 33; i < 256; ++i) {
	for (ml = memolists[i]; ml; ml = ml->next) {
	    fputc(1, f);
	    if (1 != fwrite(ml, sizeof(MemoList), 1, f) ||
		    ml->n_memos !=
			fwrite(ml->memos, sizeof(Memo), ml->n_memos, f))
		fatal_perror("Write error on " MEMOSERV_DB);
	    for (memos = ml->memos, j = 0; j < ml->n_memos; ++memos, ++j)
		write_string(memos->text, f, MEMOSERV_DB);
	}
	fputc(0, f);
    }
    fclose(f);
    remove(MEMOSERV_DB ".save");
}
#endif /* MEMOS */

#ifdef NEWS
void load_news_dbase(void)
{
    FILE *f = fopen(NEWSSERV_DB, "r");
    int i, j, len;
    NewsList *nl;
    Memo *newss;

    if (!f) {
	log_perror("Can't read NewsServ database " NEWSSERV_DB);
	return;
    }
    switch (i = get_file_version(f, NEWSSERV_DB)) {
      case 4:
      case 3:
      case 2:
      case 1:
	for (i = 33; i < 256; ++i) {
	    while (fgetc(f) == 1) {
		nl = smalloc(sizeof(NewsList));
		if (1 != fread(nl, sizeof(NewsList), 1, f))
		    fatal_perror("Read error on " NEWSSERV_DB);
		alpha_insert_newslist(nl);
		nl->newss = newss = smalloc(sizeof(Memo) * nl->n_newss);
		fread(newss, sizeof(Memo), nl->n_newss, f);
		for (j = 0; j < nl->n_newss; ++j, ++newss)
		    newss->text = read_string(f, NEWSSERV_DB);
	    }
	}
	break;
      default:
	fatal("Unsupported version number (%d) on %s", i, NEWSSERV_DB);
    } /* switch (version) */
    fclose(f);
}

void save_news_dbase(void)
{
    FILE *f;
    int i, j, len;
    NewsList *nl;
    Memo *newss;

    remove(NEWSSERV_DB ".save");
    if (rename(NEWSSERV_DB, NEWSSERV_DB ".save") < 0)
	log_perror("Cannot back up " NEWSSERV_DB);
    f = fopen(NEWSSERV_DB, "w");
    if (!f) {
	log_perror("Can't write to NewsServ database " NEWSSERV_DB);
	if (rename(NEWSSERV_DB ".save", NEWSSERV_DB) < 0)
	    fatal_perror("Cannot restore backup copy of " NEWSSERV_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't set group of %s to %d", NEWSSERV_DB, file_gid);
    write_file_version(f, NEWSSERV_DB);
    for (i = 33; i < 256; ++i) {
	for (nl = newslists[i]; nl; nl = nl->next) {
	    fputc(1, f);
	    if (1 != fwrite(nl, sizeof(NewsList), 1, f) ||
		    nl->n_newss !=
			fwrite(nl->newss, sizeof(Memo), nl->n_newss, f))
		fatal_perror("Write error on " NEWSSERV_DB);
	    for (newss = nl->newss, j = 0; j < nl->n_newss; ++newss, ++j)
		write_string(newss->text, f, NEWSSERV_DB);
	}
	fputc(0, f);
    }
    fclose(f);
    remove(NEWSSERV_DB ".save");
}
#endif /* NEWS */

/*************************************************************************/

/* check_memos:  See if the given nick has any waiting memos, and send a
 *               NOTICE to that nick if so.
 */

#ifdef MEMOS
void check_memos(const char *nick)
{
    MemoList *ml;

    if (ml = find_memolist(nick)) {
	notice(s_MemoServ, nick, "You have %d memo%s.", ml->n_memos,
		ml->n_memos == 1 ? "" : "s");
	notice(s_MemoServ, nick, "Type \2/msg %s %s\2 to %s.",
		s_MemoServ,
		ml->n_memos == 1 ? "READ 1" : "LIST",
		ml->n_memos == 1 ? "read it" : "list them");
    }
}
#endif /* MEMOS */

/* check_newss:  See if the given chan has any waiting newss, and send a
 *               NOTICE to that chan if so.
 */

#ifdef NEWS
void check_newss(const char *chan, const char *source)
{
    NewsList *nl;
    ChannelInfo *ci = cs_findchan(chan);
    User *u = finduser(source);

    if (ci && check_access(u, ci, CA_READMEMO)) {
	if (nl = find_newslist(chan)) {
	    notice(s_MemoServ, source, "There %s %d news article%s for %s.",
		nl->n_newss == 1 ? "is" : "are",
		nl->n_newss,
		nl->n_newss == 1 ? "" : "s",
		chan);
	    notice(s_MemoServ, source, "Type \2/msg %s %s %s%s\2 to %s.",
		s_MemoServ,
		nl->n_newss == 1 ? "READ" : "LIST",
		chan,
		nl->n_newss == 1 ? " 1"    : "",
		nl->n_newss == 1 ? "read it" : "list them");
	}
    }
}
#endif /* NEWS */

/*************************************************************************/
/*********************** MemoServ private routines ***********************/
/*************************************************************************/

/* find_memolist:  Find the memo list for a given nick.  Return NULL if
 *                 none.
 */

#ifdef MEMOS
MemoList *find_memolist(const char *nick)
{
    MemoList *ml;
    int i;

    for (ml = memolists[tolower(*nick)];
			ml && (i = stricmp(ml->nick, nick)) < 0;
			ml = ml->next)
	;
    return i==0 ? ml : NULL;
}

/* find_newslist:  Find the news list for a given nick.  Return NULL if
 *                 none.
 */
#endif /* MEMOS */

#ifdef NEWS
NewsList *find_newslist(const char *chan)
{
    NewsList *nl;
    int i;

    for (nl = newslists[tolower(*chan)];
			nl && (i = stricmp(nl->chan, chan)) < 0;
			nl = nl->next)
	;
    return i==0 ? nl : NULL;
}
#endif /* NEWS */

/*************************************************************************/

/* alpha_insert_memolist:  Insert a memo list alphabetically into the
 *                         database.
 */

#ifdef MEMOS
static void alpha_insert_memolist(MemoList *ml)
{
    MemoList *ml2, *ml3;
    char *nick = ml->nick;

    for (ml3 = NULL, ml2 = memolists[tolower(*nick)];
			ml2 && stricmp(ml2->nick, nick) < 0;
			ml3 = ml2, ml2 = ml2->next)
	;
    ml->prev = ml3;
    ml->next = ml2;
    if (!ml3)
	memolists[tolower(*nick)] = ml;
    else
	ml3->next = ml;
    if (ml2)
	ml2->prev = ml;
}
#endif /* MEMOS */

/* alpha_insert_newslist:  Insert a news list alphabetically into the
 *                         database.
 */

#ifdef NEWS
static void alpha_insert_newslist(NewsList *nl)
{
    NewsList *nl2, *nl3;
    char *chan = nl->chan;

    for (nl3 = NULL, nl2 = newslists[tolower(*chan)];
			nl2 && stricmp(nl2->chan, chan) < 0;
			nl3 = nl2, nl2 = nl2->next)
	;
    nl->prev = nl3;
    nl->next = nl2;
    if (!nl3)
	newslists[tolower(*chan)] = nl;
    else
	nl3->next = nl;
    if (nl2)
	nl2->prev = nl;
}
#endif /* NEWS */

/*************************************************************************/

/* del_memolist:  Remove a nick's memo list from the database.  Assumes
 *                that the memo count for the nick is non-zero.
 */

#ifdef MEMOS
static void del_memolist(MemoList *ml)
{
    int i;

    if (ml->next)
	ml->next->prev = ml->prev;
    if (ml->prev)
	ml->prev->next = ml->next;
    else
	memolists[tolower(*ml->nick)] = ml->next;
    free(ml->memos);
    free(ml);
}
#endif

/* del_newslist:  Remove a nick's news list from the database.  Assumes
 *                that the news count for the nick is non-zero.
 */

#ifdef NEWS
static void del_newslist(NewsList *nl)
{
    int i;

    if (nl->next)
	nl->next->prev = nl->prev;
    if (nl->prev)
	nl->prev->next = nl->next;
    else
	newslists[tolower(*nl->chan)] = nl->next;
    free(nl->newss);
    free(nl);
}
#endif /* NEWS */

/*************************************************************************/
/*********************** MemoServ command routines ***********************/
/*************************************************************************/

/* Return a help message. */

static void do_help(const char *source)
{
    char *cmd = strtok(NULL, "");
    char buf[256];

    sprintf(buf, "%s%s", s_MemoServ, cmd ? " " : "");
    strscpy(buf+strlen(buf), cmd ? cmd : "", sizeof(buf)-strlen(buf));
    helpserv(s_MemoServ, source, buf);
}

/*************************************************************************/

/* Send a memo to a nick. */

static void do_send(const char *source)
{
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif
    NickInfo *ni;
    Memo *m;
    char *arg = strtok(NULL, " ");
    char *text = strtok(NULL, "");

    if (!text) {
#ifdef NEWS
	notice(s_MemoServ, source,
		"Syntax: \2SEND \37nick|channel\37 \37memo-text\37\2");
#else
	notice(s_MemoServ, source,
		"Syntax: \2SEND \37nick\37 \37memo-text\37\2");
#endif /* NEWS */
	notice(s_MemoServ, source,
		"\2/msg %s HELP SEND\2 for more information.", s_MemoServ);

#ifdef NEWS
    } else if (arg[0]=='#') {

      ChannelInfo *ci;
      Channel *c;
      User *u;

      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	notice(s_MemoServ, source, "Permission denied.");

      else if (!(ci = cs_findchan(arg)))
	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      else if (!(u = finduser(source)) || !check_access(u, ci, CA_WRITEMEMO))
	notice(s_MemoServ, source, "Access denied.");

      else {

	nl = find_newslist(arg);
	if (!nl) {
	    nl = scalloc(sizeof(NewsList), 1);
	    strscpy(nl->chan, arg, CHANMAX);
	    alpha_insert_newslist(nl);
	}
	++nl->n_newss;
	nl->newss = srealloc(nl->newss, sizeof(Memo) * nl->n_newss);
	m = &nl->newss[nl->n_newss-1];
	strscpy(m->sender, source, CHANMAX);
	if (nl->n_newss > 1) {
	    m->number = m[-1].number + 1;
	    if (m->number < 1) {
		int i;
		for (i = 0; i < nl->n_newss; ++i)
		    nl->newss[i].number = i+1;
	    }
	} else
	    nl->newss[nl->n_newss-1].number = 1;
	m->time = time(NULL);
	m->text = sstrdup(text);
	notice(s_MemoServ, source, "News sent to %s.", arg);
	if (c = findchan(arg)) {
	    struct c_userlist *u;
	    for (u = c->users; u; u = u->next) {
		if (check_access(u->user, ci, CA_READMEMO)) {
		    notice(s_MemoServ, u->user->nick, "There is new news for %s (#%d) from %s.",
			arg, m->number, source);
		    notice(s_MemoServ, u->user->nick, "Type \2/msg %s READ %s %d\2 to read it.",
			s_MemoServ, arg, m->number);
		}
	    }
	}
     }

#endif /* NEWS */
#ifdef MEMOS
    } else {

      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	notice(s_MemoServ, source, "Permission denied.");

      else if (!findnick(arg))
	notice(s_MemoServ, source, "Nick %s isn't registered.", arg);

#if FILE_VERSION > 3
      else if (is_on_ignore(source,arg) && !(ni->flags & NI_IRCOP))
	notice(s_MemoServ, source, "Nick %s is ignoring your memos.", arg);
#endif
      
      else {
	ml = find_memolist(arg);
	if (!ml) {
	    ml = scalloc(sizeof(MemoList), 1);
	    strscpy(ml->nick, arg, NICKMAX);
	    alpha_insert_memolist(ml);
	}
	++ml->n_memos;
	ml->memos = srealloc(ml->memos, sizeof(Memo) * ml->n_memos);
	m = &ml->memos[ml->n_memos-1];
	strscpy(m->sender, source, NICKMAX);
	if (ml->n_memos > 1) {
	    m->number = m[-1].number + 1;
	    if (m->number < 1) {
		int i;
		for (i = 0; i < ml->n_memos; ++i)
		    ml->memos[i].number = i+1;
	    }
	} else
	    ml->memos[ml->n_memos-1].number = 1;
	m->time = time(NULL);
	m->text = sstrdup(text);
	notice(s_MemoServ, source, "Memo sent to %s.", arg);
	if (finduser(arg)) {
	    notice(s_MemoServ, arg, "You have a new memo (#%d) from %s.",
			m->number, source);
	    notice(s_MemoServ, arg, "Type \2/msg %s READ %d\2 to read it.",
			s_MemoServ, m->number);
	}
      }
#endif
    }
}

/*************************************************************************/

/* List the memos (if any) for the source nick. */

static void do_list(const char *source)
{
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif
    NickInfo *ni;
    Memo *m;
    int i;
    char timebuf[64];
    char *arg = strtok(NULL, "");
    struct tm tm;

#ifdef NEWS
    if(arg && arg[0]=='#') {
      ChannelInfo *ci;
      User *u;


      if (!(ci = cs_findchan(arg))) {

	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      } else if (!(u = finduser(source)) || !check_access(u, ci, CA_READMEMO)) {

	notice(s_MemoServ, source, "Access denied.");

      } else if (!(nl = find_newslist(arg))) {
	notice(s_MemoServ, source, "There is no news for %s.", arg);

      } else {
	notice(s_MemoServ, source,
		"News articles for %s.  To read, type \2/msg %s READ %s \37num\37\2",
		arg, s_MemoServ, arg);
	notice(s_MemoServ, source, "Num  Sender            Date/Time");
	for (i = 0, m = nl->newss; i < nl->n_newss; ++i, ++m) {
	    tm = *localtime(&m->time);
	    strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", &tm);
	    timebuf[sizeof(timebuf)-1] = 0;	/* just in case */
	    notice(s_MemoServ, source, "%3d  %-16s  %s %s",
			m->number, m->sender, timebuf, time_zone);
	}
      }
#ifdef MEMOS
    } else {
#endif
#endif /* NEWS */
#ifdef MEMOS
      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED))) {
	notice(s_MemoServ, source, "Permission denied.");

      } else if (!(ml = find_memolist(source))) {
	notice(s_MemoServ, source, "You have no memos.");

      } else {
	notice(s_MemoServ, source,
		"Memos for %s.  To read, type \2/msg %s READ \37num\37\2",
		source, s_MemoServ);
	notice(s_MemoServ, source, "Num  Sender            Date/Time");
	for (i = 0, m = ml->memos; i < ml->n_memos; ++i, ++m) {
	    tm = *localtime(&m->time);
	    strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", &tm);
	    timebuf[sizeof(timebuf)-1] = 0;	/* just in case */
	    notice(s_MemoServ, source, "%3d  %-16s  %s %s",
			m->number, m->sender, timebuf, time_zone);
	}
      }
#endif
#ifdef NEWS
    }
#endif /* NEWS */
}

/*************************************************************************/

/* Read a memo. */

#ifdef STUPID
static void do_read(const char *whoto)
#else
static void do_read(const char *source)
#endif
{
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif
    NickInfo *ni;
    Memo *m;
#ifdef NEWS
    char *arg = strtok(NULL, " ");
    char *arg2 = strtok(NULL, "");
#else
    char *arg = strtok(NULL, "");
#endif /* NEWS */
    char *numstr;
    int num;
#ifdef STUPID
    char *source = sstrdup(whoto);
#endif

    if (arg) {
#ifdef NEWS
	    if (!arg2)
#endif /* NEWS */
		strcpy(numstr,arg);
#ifdef NEWS
	    else
		strcpy(numstr,arg2);
#endif /* NEWS */
    }
    if (!arg) {
	notice(s_MemoServ, source, "Syntax: \2READ \37num|all\37");
#ifdef NEWS
	notice(s_MemoServ, source, "Syntax: \2READ channel \37num|all\37");
#endif
      notice(s_MemoServ, source,
		"\2/msg %s HELP READ\2 for more information.", s_MemoServ);
    
    } else if (!numstr || ((num = atoi(numstr)) <= 0 && stricmp(numstr,"ALL") != 0)) {
#ifdef NEWS
      if(arg[0]!='#')
#endif /* NEWS */
	notice(s_MemoServ, source, "Syntax: \2READ \37num|all\37");
#ifdef NEWS
      else
	notice(s_MemoServ, source, "Syntax: \2READ channel \37num|all\37");
#endif /* NEWS */
      notice(s_MemoServ, source,
		"\2/msg %s HELP READ\2 for more information.", s_MemoServ);

#ifdef NEWS
    } else if (arg[0]=='#') {

    ChannelInfo *ci;
    User *u;

      if (!(ci = cs_findchan(arg))) {

	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      } else if (!(u = finduser(source)) || !check_access(u, ci, CA_READMEMO)) {

	notice(s_MemoServ, source, "Access denied.");

      } else if (!(nl = find_newslist(arg))) {
	notice(s_MemoServ, source, "There is no news for %s.", arg);

      } else {

	if (num>0) {
	    int i;
	    for (i = 0; i < nl->n_newss; ++i) {
		if (nl->newss[i].number == num)
		    break;
	    }
	    if (i >= nl->n_newss) {
	        notice(s_MemoServ, source, "News article %d does not exist for %s!", num, arg);
	    } else {
		m = &nl->newss[i];
		notice(s_MemoServ, source,
		    "News %d for %s from %s.",
		    m->number, arg, m->sender);
		notice(s_MemoServ, source, "%s", m->text);
	    }
	} else {
	    int i;
	    for (i = 0; i < nl->n_newss; ++i) {
		m = &nl->newss[i];
		notice(s_MemoServ, source,
		    "News %d for %s from %s.",
		    m->number, arg, m->sender);
		notice(s_MemoServ, source, "%s", m->text);
	    }
	}

      }
#endif /* NEWS */
#ifdef MEMOS
    } else {
      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED))) {
	notice(s_MemoServ, source, "Permission denied.");

      } else if (!(ni->flags & NI_IDENTIFIED)) {
	notice(s_MemoServ, source, "Identification required for that command."
			"  Type \2/msg %s IDENTIFY \37password\37\2 to "
			"access your memos.", s_NickServ);

      } else if (!(ml = find_memolist(source))) {
	notice(s_MemoServ, source, "You have no memos.");

      } else {

	if (num>0) {
	    int i;
	    for (i = 0; i < ml->n_memos; ++i) {
		if (ml->memos[i].number == num)
		    break;
	    }
	    if (i >= ml->n_memos) {
		notice(s_MemoServ, source, "Memo %d does not exist!", num);
	    } else {
		m = &ml->memos[i];
		notice(s_MemoServ, source,
		    "Memo %d from %s.  To delete, type: \2/msg %s DEL %d\2",
		    m->number, m->sender, s_MemoServ, m->number);
		notice(s_MemoServ, source, "%s", m->text);
	    }
	} else {
	    int i;
	    for (i = 0; i < ml->n_memos; ++i) {
		m = &ml->memos[i];
		notice(s_MemoServ, source,
		    "Memo %d from %s.", m->number, m->sender);
		notice(s_MemoServ, source, "%s", m->text);
	    }
	    notice(s_MemoServ, source,
		"To delete, type: \2/msg %s DEL ALL\2", s_MemoServ);
	}
      }
#endif
    }
#ifdef STUPID
    free(source);
#endif
}

/* Forward a memo. */

#ifdef STUPID
static void do_forward(const char *whoto)
#else
static void do_forward(const char *source)
#endif
{
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif
    NickInfo *ni;
    Memo *m;
    char *arg = strtok(NULL, " ");
#ifdef NEWS
    char *arg2 = strtok(NULL, " ");
    char *arg3 = strtok(NULL, "");
#else
    char *arg2 = strtok(NULL, "");
#endif /* NEWS */
    char *numstr;
    int num;
#ifdef STUPID
    char *source = sstrdup(whoto);
#endif

    if (arg) {
#ifdef NEWS
	if (arg2) {
	    if (!arg3)
#endif /* NEWS */
		strcpy(numstr,arg);
#ifdef NEWS
	    else
		strcpy(numstr,arg2);
	}
#endif /* NEWS */
    }
    if (!arg2) {
#ifdef NEWS
	notice(s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user|channel");
	notice(s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
#else
	notice(s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user");
#endif /* NEWS */
      notice(s_MemoServ, source,
		"\2/msg %s HELP FORWARD\2 for more information.", s_MemoServ);
    
    } else if (!numstr || (num = atoi(numstr)) <= 0) {
#ifdef NEWS
      if(arg[0]!='#')
	notice(s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user|channel");
      else
	notice(s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
#else
	notice(s_MemoServ, source, "Syntax: \2FORWARD \37num\37 user");
#endif /* NEWS */
      notice(s_MemoServ, source,
		"\2/msg %s HELP FORWARD\2 for more information.", s_MemoServ);

#ifdef NEWS
    } else if (arg[0]=='#') {

    ChannelInfo *ci;
    User *u;

      if (!arg3) {

	notice(s_MemoServ, source, "Syntax: \2FORWARD channel \37num\37 user|channel");
	notice(s_MemoServ, source,
		"\2/msg %s HELP FORWARD\2 for more information.", s_MemoServ);

      } else if (!(ci = cs_findchan(arg))) {

	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      } else if (!(u = finduser(source)) || !check_access(u, ci, CA_READMEMO)) {

	notice(s_MemoServ, source, "Access denied.");

      } else if (!(nl = find_newslist(arg))) {
	notice(s_MemoServ, source, "There is no news for %s.", arg);

      } else {

	    int i;
	    for (i = 0; i < nl->n_newss; ++i) {
		if (nl->newss[i].number == num)
		    break;
	    }
	    if (i >= nl->n_newss) {
	        notice(s_MemoServ, source, "News article %d does not exist for %s!", num, arg);
	    } else {
		char s[NICKMAX+CHANMAX+2];
		char whofrom[NICKMAX];
		m = &nl->newss[i];
		strcpy(s, m->sender);
		strcat(s, "/");
		strcat(s, ci->name);
		strcpy(whofrom, source);
		do_fwd2(whofrom, s, arg3, m->text);
	    }

      }
#endif /* NEWS */
#ifdef MEMOS
    } else {
      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED))) {
	notice(s_MemoServ, source, "Permission denied.");

      } else if (!(ni->flags & NI_IDENTIFIED)) {
	notice(s_MemoServ, source, "Identification required for that command."
			"  Type \2/msg %s IDENTIFY \37password\37\2 to "
			"access your memos.", s_NickServ);

      } else if (!(ml = find_memolist(source))) {
	notice(s_MemoServ, source, "You have no memos.");

      } else {

	    int i;
	    for (i = 0; i < ml->n_memos; ++i) {
		if (ml->memos[i].number == num)
		    break;
	    }
	    if (i >= ml->n_memos) {
		notice(s_MemoServ, source, "Memo %d does not exist!", num);
	    } else {
		char whofrom[NICKMAX];
		strcpy(whofrom, source);
		m = &ml->memos[i];
#ifdef NEWS
		if (arg3)
		    do_fwd2(whofrom, m->sender, arg3, m->text);
		else
#endif
		    do_fwd2(whofrom, m->sender, arg2, m->text);
	    }
      }
#endif
    }
#ifdef STUPID
    free(source);
#endif
}

/*************************************************************************/

/* Delete a memo. */

static void do_del(const char *source)
{
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif /* NEWS */
    NickInfo *ni;
#ifdef NEWS
    char *arg = strtok(NULL, " ");
    char *arg2 = strtok(NULL, "");
#else
    char *arg = strtok(NULL, "");
#endif /* NEWS */
    char *numstr;
    int num, i;

    if (arg) {
#ifdef NEWS
	    if (!arg2)
#endif /* NEWS */
		strcpy(numstr,arg);
#ifdef NEWS
	    else
		strcpy(numstr,arg2);
#endif /* NEWS */
    }
    	
    
    if (!arg) {
	notice(s_MemoServ, source, "Syntax: \2DEL {\37num\37 | ALL}");
#ifdef NEWS
	notice(s_MemoServ, source, "Syntax: \2DEL channel {\37num\37 | ALL}");
#endif /* NEWS */
      notice(s_MemoServ, source,
		"\2/msg %s HELP DEL\2 for more information.", s_MemoServ);

    } else if (!numstr ||
	    ((num = atoi(numstr)) <= 0 && stricmp(numstr, "ALL") != 0)) {
#ifdef NEWS
      if(arg[0]!='#')
#endif /* NEWS */
	notice(s_MemoServ, source, "Syntax: \2DEL {\37num\37 | ALL}");
#ifdef NEWS
      else
	notice(s_MemoServ, source, "Syntax: \2DEL channel {\37num\37 | ALL}");
#endif
      notice(s_MemoServ, source,
		"\2/msg %s HELP DEL\2 for more information.", s_MemoServ);

#ifdef NEWS
    } else if (arg[0]=='#') {

      ChannelInfo *ci;
      User *u;

      if (!(ci = cs_findchan(arg))) {

	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      } else if (!(u = finduser(source)) || !check_access(u, ci, CA_WRITEMEMO)) {

	notice(s_MemoServ, source, "Access denied.");

      } else if (!(nl = find_newslist(arg))) {
	notice(s_MemoServ, source, "There is no news for %s.", arg);

      } else {
	if (num > 0) {
	    /* Delete a specific news. */
	    for (i = 0; i < nl->n_newss; ++i) {
		if (nl->newss[i].number == num)
		    break;
	    }
	    if (i < nl->n_newss) {
		if((stricmp(nl->newss[i].sender, source) == 0) || (check_access(u, ci, CA_DELMEMO))) {
		    free(nl->newss[i].text); /* Deallocate news text newsry */
		    --nl->n_newss;		 /* One less news now */
		    if (i < nl->n_newss)	 /* Move remaining newss down a slot */
		        bcopy(nl->newss + i+1, nl->newss + i,
					sizeof(Memo) * (nl->n_newss - i));
		    notice(s_MemoServ, source, "News article %d for %s has been deleted.", num, arg);
		} else {
		    notice(s_MemoServ, source, "Access denied (not sender).");
		}
	    } else {
		notice(s_MemoServ, source, "News article %d for %s does not exist!", num, arg);
	    }
	} else {
	    /* Delete all newss.  This requires freeing the newsry holding
	     * the text of each news and flagging that there are no newss
	     * left. */
	     if (!check_access(u, ci, CA_DELMEMO))
	         notice(s_MemoServ, source, "Access denied.");
	     else {
	       for (i = 0; i < nl->n_newss; ++i)
		free(nl->newss[i].text);
	       nl->n_newss = 0;
	       notice(s_MemoServ, source, "All of news articles for %s have been deleted.", arg);
	     }
	}

	/* Did we delete the last news?  If so, delete this NewsList. */
	if (nl->n_newss == 0)
	    del_newslist(nl);
      }

#endif
#ifdef MEMOS
    } else {
      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED))) {
	notice(s_MemoServ, source, "Permission denied.");

      } else if (!(ni->flags & NI_IDENTIFIED)) {
	notice(s_MemoServ, source, "Identification required for that command."
			"  Type \2/msg %s IDENTIFY \37password\37\2 to "
			"access your memos.", s_NickServ);

      } else if (!(ml = find_memolist(source))) {
	notice(s_MemoServ, source, "You have no memos.");

      } else {
	if (num > 0) {
	    /* Delete a specific memo. */
	    for (i = 0; i < ml->n_memos; ++i) {
		if (ml->memos[i].number == num)
		    break;
	    }
	    if (i < ml->n_memos) {
		free(ml->memos[i].text); /* Deallocate memo text memory */
		--ml->n_memos;		 /* One less memo now */
		if (i < ml->n_memos)	 /* Move remaining memos down a slot */
		    bcopy(ml->memos + i+1, ml->memos + i,
					sizeof(Memo) * (ml->n_memos - i));
		notice(s_MemoServ, source, "Memo %d has been deleted.", num);
	    } else {
		notice(s_MemoServ, source, "Memo %d does not exist!", num);
	    }
	} else {
	    /* Delete all memos.  This requires freeing the memory holding
	     * the text of each memo and flagging that there are no memos
	     * left. */
	    for (i = 0; i < ml->n_memos; ++i)
		free(ml->memos[i].text);
	    ml->n_memos = 0;
	    notice(s_MemoServ, source, "All of your memos have been deleted.");
	}

	/* Did we delete the last memo?  If so, delete this MemoList. */
	if (ml->n_memos == 0)
	    del_memolist(ml);
      }
#endif /* MEMOS */
    }
}

#ifdef NEWS
void expire_news()
{
    NewsList *nl;
    Memo *m;
    ChannelInfo *ci;
    int i, j;
    const time_t expire_time = NEWS_EXPIRE*24*60*60;
    time_t tm;

    for (i = 33; i < 256; ++i) {
	for (ci = chanlists[i]; ci; ci = ci->next) {
          if ((nl = find_newslist(ci->name))) {
	    for (j = 0, m = nl->newss; j < nl->n_newss; ++j, ++m) {
		tm = time(NULL) - m->time;
		if (tm > expire_time) {
		    free(nl->newss[j].text); /* Deallocate news text newsry */
		    --nl->n_newss;		 /* One less news now */
		    if (j < nl->n_newss)	 /* Move remaining newss down a slot */
			bcopy(nl->newss + j+1, nl->newss + j,
					sizeof(Memo) * (nl->n_newss - j));
		    log("Expiring news article %d for channel %s", j, ci->name);
		}
	    }
	  }
	}
    }
}
#endif /* NEWS */

static void do_fwd2(const char *source, const char *origin, char *arg, const char *intext) {
#ifdef MEMOS
    MemoList *ml;
#endif
#ifdef NEWS
    NewsList *nl;
#endif /* NEWS */
    NickInfo *ni;
    Memo *m;
    char *text;

    char *Torigin = sstrdup(origin);
    char *Tintext = sstrdup(intext);
    sprintf(text, "[FWD: %s] %s\0", Torigin, Tintext);
    free(Torigin);
    free(Tintext);

#ifdef NEWS
    if (arg[0]=='#') {

      ChannelInfo *ci;
      User *u;

      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	notice(s_MemoServ, source, "Permission denied.");

      else if (!(ci = cs_findchan(arg)))
	notice(s_MemoServ, source, "Channel %s is not registered.", arg);

      else if (!(u = finduser(source)) || !check_access(u, ci, CA_WRITEMEMO))
	notice(s_MemoServ, source, "Access denied.");

      else {

	nl = find_newslist(arg);
	if (!nl) {
	    nl = scalloc(sizeof(NewsList), 1);
	    strscpy(nl->chan, arg, CHANMAX);
	    alpha_insert_newslist(nl);
	}
	++nl->n_newss;
	nl->newss = srealloc(nl->newss, sizeof(Memo) * nl->n_newss);
	m = &nl->newss[nl->n_newss-1];
	strscpy(m->sender, source, CHANMAX);
	if (nl->n_newss > 1) {
	    m->number = m[-1].number + 1;
	    if (m->number < 1) {
		int i;
		for (i = 0; i < nl->n_newss; ++i)
		    nl->newss[i].number = i+1;
	    }
	} else
	    nl->newss[nl->n_newss-1].number = 1;
	m->time = time(NULL);
	m->text = sstrdup(text);
	notice(s_MemoServ, source, "News sent to %s.", arg);
	if (findchan(arg)) {
	    notice(s_MemoServ, arg, "There is new news for %s (#%d) from %s.",
			arg, m->number, source);
	    notice(s_MemoServ, arg, "Type \2/msg %s READ %s %d\2 to read it.",
			s_MemoServ, arg, m->number);
	}
     }
#ifdef MEMOS
    } else {
#endif
#endif /* NEWS */
#ifdef MEMOS
      if (!(ni = findnick(source))) {
	notice(s_MemoServ, source, "Your nick is not registered.  Type"
			"\2/msg %s HELP\2 for information on registering"
			"your nickname.", s_NickServ);

      } else if (!(ni->flags & (NI_RECOGNIZED | NI_IDENTIFIED)))
	notice(s_MemoServ, source, "Permission denied.");

      else if (!findnick(arg))
	notice(s_MemoServ, source, "Nick %s isn't registered.", arg);

#if FILE_VERSION > 3
      else if (is_on_ignore(source,arg) && !(ni->flags & NI_IRCOP))
	notice(s_MemoServ, source, "Nick %s is ignoring your memos.", arg);
#endif
      
      else {
	ml = find_memolist(arg);
	if (!ml) {
	    ml = scalloc(sizeof(MemoList), 1);
	    strscpy(ml->nick, arg, NICKMAX);
	    alpha_insert_memolist(ml);
	}
	++ml->n_memos;
	ml->memos = srealloc(ml->memos, sizeof(Memo) * ml->n_memos);
	m = &ml->memos[ml->n_memos-1];
	strscpy(m->sender, source, NICKMAX);
	if (ml->n_memos > 1) {
	    m->number = m[-1].number + 1;
	    if (m->number < 1) {
		int i;
		for (i = 0; i < ml->n_memos; ++i)
		    ml->memos[i].number = i+1;
	    }
	} else
	    ml->memos[ml->n_memos-1].number = 1;
	m->time = time(NULL);
	m->text = sstrdup(text);
	notice(s_MemoServ, source, "Memo sent to %s.", arg);
	if (finduser(arg)) {
	    notice(s_MemoServ, arg, "You have a new memo (#%d) from %s.",
			m->number, source);
	    notice(s_MemoServ, arg, "Type \2/msg %s READ %d\2 to read it.",
			s_MemoServ, m->number);
	}
      }
#endif
#ifdef NEWS
    }
#endif
}

#endif  /* MEMOSERV */
