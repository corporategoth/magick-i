/* OperServ functions.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

typedef struct
{
    int what;
    char *name;
}
MessageName;

static MessageName messagename[] =
{
    {M_LOGON, "Logon"},
    {M_OPER, "Oper"},
    {-1}
};

char *override_level[] =
{
    "Disabled",
    "All IRC Operator",
    "\"low\" Levelled",
    "All Services Operator",
    "\"high\" Levelled",
    "All Services Admin",
    NULL
};

#include "os-help.c"

/* Nick for OperServ */
char s_OperServ[NICKMAX];
char akill_db[512];
char clone_db[512];
char sop_db[512];
char message_db[512];
/* Nick for sending global notices */
char s_GlobalNoticer[NICKMAX];
/* Perm nick for OperServ clone */
char s_Outlet[NICKMAX];
int akill_expire;
int clones_allowed;
char def_clone_reason[512];

int nsop = 0;
int sop_size = 0;
Sop *sops = NULL;

int nmessage = 0;
int message_size = 0;
Message *messages = NULL;

int nakill = 0;
int akill_size = 0;
Akill *akills = NULL;

int nclone = 0;
int clone_size = 0;
Allow *clones = NULL;

Clone *clonelist = NULL;

static void do_sop (const char *source);
static void do_sop_add (const char *source);
static void do_sop_del (const char *source);
static void do_sop_list (const char *source);
static void do_logonmsg (const char *source);
static void do_opermsg (const char *source);
static void do_logonmsg_add (const char *source);
static void do_opermsg_add (const char *source);
static void do_message_add (const char *source, short type);
static void do_logonmsg_del (const char *source);
static void do_opermsg_del (const char *source);
static void do_message_del (const char *source, short type);
static void do_logonmsg_list (const char *source);
static void do_opermsg_list (const char *source);
static void do_message_list (const char *source, short type);
static int add_message (const char *source, const char *text, short type);
static int del_message (int num, short type);
static void do_akill (const char *source);
static void do_pakill (const char *source);
static void do_akill_add (const char *source);
static void do_pakill_add (const char *source);
static void do_akill_addfunc (const char *source, int call);
static void do_akill_del (const char *source);
static void do_pakill_del (const char *source);
static void do_akill_delfunc (const char *source, int call);
static void do_akill_list (const char *source);
static int add_akill (const char *mask, const char *reason, const char *who, int call);
static int is_on_akill (const char *mask);
static void do_clone (const char *source);
static void do_clone_add (const char *source);
static void do_clone_del (const char *source);
static void do_clone_list (const char *source);
static void add_clone (const char *host, int amount, const char *reason, const char *who);
static Clone *findclone (const char *host);
static int is_on_clone (char *host);
static void do_mode (const char *source);
static void do_os_kick (const char *source);
static void do_help (const char *source);
static void do_global (const char *source);
static void do_settings (const char *source);
static void do_breakdown (const char *source);
static void do_sendpings (const char *source);
static void do_stats (const char *source);
#ifdef DAL_SERV
static void do_qline (const char *source);
static void do_unqline (const char *source);
static void do_noop (const char *source);
static void do_os_kill (const char *source);
#endif
static void do_ignore (const char *source);
static void do_update (const char *source);
static void do_os_quit (const char *source);
static void do_shutdown (const char *source);
static void do_reload (const char *source);
static void do_jupe (const char *source);
static void do_off (const char *source);
static void do_on (const char *source);
static void do_raw (const char *source);
static char *getmsgname (short type);

static void
do_identify (const char *source)
{
    do_real_identify (s_OperServ, source);
}

/*************************************************************************/

/* Main OperServ routine. */

void
operserv (const char *source, char *buf)
{
    char *cmd;
    char *s;

    write_log ("%s: %s: %s", s_OperServ, source, buf);
    cmd = strtok (buf, " ");

    if (!(runflags & RUN_MODE) && stricmp (cmd, "ON") != 0 && !match_wild_nocase ("ID*", cmd))
    {
	if (offreason)
	    notice (s_OperServ, source, SERVICES_OFF_REASON, offreason);
	else
	    notice (s_OperServ, source, SERVICES_OFF);
	return;
    }

    if (!cmd)
	return;

    if (stricmp (cmd, "\1PING") == 0)
    {

	if (!(s = strtok (NULL, "")))
	    s = "\1";
	notice (s_OperServ, source, "\1PING %s", s);

    }
    else if (match_wild_nocase ("USER*LIST", cmd))
    {
	s = strtok (NULL, " ");
	send_user_list (s_OperServ, source, s);

    }
    else if (match_wild_nocase ("*MASK*LIST", cmd))
    {
	s = strtok (NULL, " ");
	send_usermask_list (s_OperServ, source, s);

    }
    else if (match_wild_nocase ("U*MODE*LIST", cmd))
    {
	s = strtok (NULL, " ");
	send_usermode_list (s_OperServ, source, s);

    }
    else if (match_wild_nocase ("CHAN*LIST", cmd))
    {
	s = strtok (NULL, " ");
	send_channel_list (s_OperServ, source, s);

    }
    else if (match_wild_nocase ("C*MODE*LIST", cmd))
    {
	s = strtok (NULL, " ");
	send_chanmode_list (s_OperServ, source, s);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"HELP", H_OPER, do_help},
	    {"*MODE", H_OPER, do_mode},
	    {"KICK*", H_OPER, do_os_kick},
	    {"STAT*", H_OPER, do_stats},
	    {"SET*", H_OPER, do_settings},
	    {"BREAKDOWN", H_OPER, do_breakdown},
	    {"COUNT*", H_OPER, do_breakdown},
	    {"ID*", H_OPER, do_identify},
	    {"GLOB*", H_OPER, do_global},
	    {"A*KILL", H_OPER, do_akill},
	    {"P*A*KILL", H_SOP, do_pakill},
	    {"IGN*", H_SOP, do_ignore},
	    {"JUPE*", H_SOP, do_jupe},
	    {"FAKE*", H_SOP, do_jupe},
	    {"UPDATE*", H_SOP, do_update},
	    {"WRITE*", H_SOP, do_update},
	    {"SAVE*", H_SOP, do_update},
	    {"CLONE*", H_SOP, do_clone},
	    {"LOG*MESS*", H_SOP, do_logonmsg},
	    {"LOG*MSG*", H_SOP, do_logonmsg},
/*	    { "*PING*", H_SOP, do_sendpings }, */
	    {"OP*MESS*", H_ADMIN, do_opermsg},
	    {"OP*MSG*", H_ADMIN, do_opermsg},
	    {"OFF", H_ADMIN, do_off},
	    {"ON", H_ADMIN, do_on},
	    {"RAW", H_ADMIN, do_raw},
	    {"QUOTE", H_ADMIN, do_raw},
	    {"SHUTDOWN", H_ADMIN, do_shutdown},
	    {"DIE", H_ADMIN, do_shutdown},
	    {"END", H_ADMIN, do_shutdown},
	    {"RELOAD", H_ADMIN, do_reload},
	    {"*HUP", H_ADMIN, do_reload},
	    {"RESTART", H_ADMIN, do_reload},
	    {"QUIT", H_ADMIN, do_os_quit},
	    {"*MURDER", H_ADMIN, do_os_quit},
	    {"S*OP", H_ADMIN, do_sop},
	    {"OPER*", H_ADMIN, do_sop},
#ifdef DAL_SERV
	    {"QLINE", H_OPER, do_qline},
	    {"UNQLINE", H_OPER, do_unqline},
	    {"KILL*", H_SOP, do_os_kill},
	    {"N*OP*", H_ADMIN, do_noop},
#endif
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	{
	    if((nickserv_on==FALSE&&command->process==do_identify)
		||(devnull_on==FALSE&&command->process==do_ignore)
		||(globalnoticer_on==FALSE&&(command->process==do_global||command->process==do_logonmsg||command->process==do_opermsg))
		||(akill_on==FALSE&&(command->process==do_akill||command->process==do_pakill))
		||(clones_on==FALSE&&command->process==do_clone)
		)
		notice (s_OperServ, source, ERR_UNKNOWN_COMMAND, cmd, s_OperServ);
	    else
		(*command->process) (source);
	}
	else
	    notice (s_OperServ, source, ERR_UNKNOWN_COMMAND, cmd, s_OperServ);
    }
}

static void
do_mode (const char *source)
{
    Channel *c;
    User *u;
    char l[16];
    char *chan = strtok (NULL, " ");
    char *serv = strtok (NULL, " ");
    char *s = strtok (NULL, "");
    if (!chan)
	return;

    if (validchan(chan))
    {
	if (!(c = findchan (chan)))
	    return;
	else if (!serv)
	{
	    snprintf (l, sizeof (l), " %d", c->limit);
	    notice (s_OperServ, source, "%s +%s%s%s%s%s", c->name, c->mode,
		    (c->limit) ? " " : "",
		    (c->limit) ? l : "",
		    (c->key) ? " " : "",
		    (c->key) ? c->key : "");
	}
	else if (s)
	    change_cmode (s_OperServ, c->name, serv, s);
	else
	    change_cmode (s_OperServ, c->name, serv, "");
    }
    else if (!(u = finduser (chan)))
    {
	return;
    }
    else if (!serv)
	notice (s_OperServ, source, "%s +%s", u->nick, u->mode);
#ifdef DAL_SERV
    else if (is_services_op (source))
    {
	char *av[2];
	av[0] = sstrdup (u->nick);
	av[1] = sstrdup (serv);
	send_cmd (s_OperServ, "SVSMODE %s %s", u->nick, serv);
	do_svumode (source, 2, av);
	free (av[0]);
	free (av[1]);

    }
#endif
}

static void
do_os_kick (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *nick = strtok (NULL, " ");
    char *s = strtok (NULL, "");
    if (!s || !findchan (chan) || !finduser (nick))
	return;
    else
    {
	char *st;
	st=smalloc(strlen (s) + strlen (source) + 4);
	snprintf (st, strlen (s) + strlen (source) + 4, "%s (%s)", source, s);
	kick_user (s_OperServ, chan, finduser (nick)->nick, st);
	free(st);
    }
}

static void
do_help (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice_list (s_OperServ, source, os_help);
	if (is_services_op (source))
	    notice_list (s_OperServ, source, os_sop_help);
	if (is_services_admin (source))
	    notice_list (s_OperServ, source, os_admin_help);
	notice_list (s_OperServ, source, os_end_help);
    }
    else if (match_wild_nocase ("GLOB*", cmd) && globalnoticer_on==TRUE)
    {
	/* Information varies, so we need to do it manually. */
	notice (s_OperServ, source, "Syntax: GLOBAL \37message\37");
	notice (s_OperServ, source, "");
	notice (s_OperServ, source,
		    "Allows IRCops to send messages to all users on the");
	notice (s_OperServ, source,
		    "network.  The message will be sent from the nick");
	notice (s_OperServ, source, "\2%s\2.", s_GlobalNoticer);
    }
    else
    {

	Hash_HELP *command, hash_table[] =
	{
	    {"*MODE", H_OPER, mode_help},
	    {"USER*LIST", H_OPER, userlist_help},
	    {"*MASK*LIST", H_OPER, masklist_help},
	    {"CHAN*LIST", H_OPER, chanlist_help},
	    {"KICK", H_OPER, kick_help},
	    {"STAT*", H_OPER, stats_help},
	    {"SET*", H_OPER, settings_help},
	    {"BREAKDOWN", H_OPER, breakdown_help},
	    {"COUNT*", H_OPER, breakdown_help},
	    {"NICKS?RV", H_OPER, nickserv_help},
	    {"CHANS?RV", H_OPER, chanserv_help},
	    {"MEMOS?RV", H_OPER, memoserv_help},
	    {"JUPE*", H_SOP, jupe_help},
	    {"FAKE*", H_SOP, jupe_help},
	    {"IGNORE", H_SOP, ignore_help},
	    {"UPDATE*", H_SOP, update_help},
	    {"WRITE*", H_SOP, update_help},
	    {"SAVE*", H_SOP, update_help},
	    {"LOG*MESS*", H_SOP, logonmsg_help},
	    {"LOG*MSG*", H_SOP, logonmsg_help},
/*	    { "*PING*", H_SOP, sendpings_help }, */
	    {"OP*MESS*", H_ADMIN, opermsg_help},
	    {"OP*MSG*", H_ADMIN, opermsg_help},
	    {"ON", H_ADMIN, offon_help},
	    {"OFF", H_ADMIN, offon_help},
	    {"SHUTDOWN", H_ADMIN, shutdown_help},
	    {"DIE", H_ADMIN, shutdown_help},
	    {"END", H_ADMIN, shutdown_help},
	    {"RELOAD", H_ADMIN, reload_help},
	    {"*HUP", H_ADMIN, reload_help},
	    {"RESTART", H_ADMIN, reload_help},
	    {"QUIT", H_ADMIN, quit_help},
	    {"*MURDER", H_ADMIN, quit_help},
	    {"S*OP", H_ADMIN, sop_help},
	    {"OPER*", H_ADMIN, sop_help},
	    {"A*KILL", H_OPER, akill_help},
	    {"P*A*KILL", H_SOP, pakill_help},
#ifdef DAL_SERV
	    {"QLINE", H_OPER, qline_help},
	    {"UNQLINE", H_OPER, unqline_help},
	    {"KILL*", H_SOP, kill_help},
	    {"N*OP*", H_ADMIN, noop_help},
#endif
	    {"CLONE*", H_SOP, clone_help},
	    {NULL}
	};

	if (command = get_help_hash (source, cmd, hash_table))
	{
	    if((globalnoticer_on==FALSE&&(command->process==logonmsg_help||command->process==opermsg_help))
		||(akill_on==FALSE&&(command->process==akill_help||command->process==pakill_help))
		||(clones_on==FALSE&&command->process==clone_help)
		)
		notice (s_OperServ, source, ERR_NOHELP, cmd);
	    else
		notice_list (s_OperServ, source, command->process);
	}
	else
	    notice (s_OperServ, source, ERR_NOHELP, cmd);
    }
}

static void
do_global (const char *source)
{
    char *msg = strtok (NULL, "");
    if (!msg)
    {
	notice (s_OperServ, source, "Syntax: \2GLOBAL \37msg\37\2");
	notice (s_OperServ, source,
		"\2/msg %s HELP GLOBAL for more information.",
		s_OperServ);
    }
    noticeall (s_GlobalNoticer, "%s", msg);
    wallops (s_OperServ, OS_GLOBAL_WALLOP, source);
}

static void
do_settings (const char *source)
{
    notice (s_OperServ, source, OS_SET_EXP);
    if(chanserv_on==TRUE)
	notice (s_OperServ, source, OS_SET_EXP_CHAN, channel_expire);
    if(nickserv_on==TRUE)
	notice (s_OperServ, source, OS_SET_EXP_NICK, nick_expire);
    if(news_on==TRUE)
	notice (s_OperServ, source, OS_SET_EXP_NEWS, news_expire);
    if(akill_on==TRUE)
	notice (s_OperServ, source, OS_SET_EXP_AKILL, akill_expire);
    if(clones_on==TRUE)
	notice (s_OperServ, source, OS_SET_CLONES, clones_allowed);
    if(nickserv_on==TRUE&&release_timeout>0)
	notice (s_OperServ, source, OS_SET_RELEASE, release_timeout);
    if(chanserv_on==TRUE&&akick_max>0)
	notice (s_OperServ, source, OS_SET_AKICKS, akick_max);
    if(devnull_on==TRUE)
    {
	notice (s_OperServ, source, OS_SET_FLOOD, flood_messages, flood_time);
	notice (s_OperServ, source, OS_SET_IGNORE, ignore_time, ignore_offences);
    }
    if(server_relink>0)
	notice (s_OperServ, source, OS_SET_RELINK, server_relink);
    notice (s_OperServ, source, OS_SET_OVERRIDE, override_level[override_level_val]);
    notice (s_OperServ, source, OS_SET_UPDATE, update_timeout);
    if(show_sync_on==TRUE)
	notice(s_OperServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    notice (s_OperServ, source, OS_SET_ADMINS, services_admin);
}

static void
do_breakdown (const char *source)
{
    User *u;
    int i, cnt, totcnt = 0;

/*    notice(s_OperServ, source, "\2Servers                          Users   Lag  Percent\2");
 */ notice (s_OperServ, source, "\2Servers                          Users  Percent\2");
    for (i = 0; i < servcnt; ++i)
    {
	cnt = 0;
	for (u = userlist; u; u = u->next)
	    if (stricmp (servlist[i].server, u->server) == 0)
	    {
		cnt++;
		totcnt++;
	    }
/*      notice(s_OperServ, source, "%-32s  %4d  %3ds  %3.2f%%",
   servlist[i].server, cnt, servlist[i].lag, (float)(cnt*100)/usercnt);
 */ notice (s_OperServ, source, "%-32s  %4d  %3.2f%%",
	    servlist[i].server, cnt, (float) (cnt * 100) / usercnt);
    }
    if (totcnt != usercnt)
	notice (s_OperServ, source, 
	     "WARNING: I counted %d users, but have %d on status record.",
		totcnt, usercnt);
}

static void
do_sendpings (const char *source)
{
    notice (s_OperServ, source, "Sending Pings.");
    runflags |= RUN_SEND_PINGS;
}

static void
do_stats (const char *source)
{
    char *extra = strtok (NULL, "");

    notice (s_OperServ, source, "Current users : \2%d\2 (%d ops)", usercnt, opcnt);
    if(clones_on==TRUE)
    {
	Clone *clone;
	int cnt = 0, hosts = 0;
	for (clone = clonelist; clone; clone = clone->next)
	    if (clone->amount > 1)
	    {
		cnt += clone->amount - 1;
		hosts++;
	    }

	if (cnt)
	    notice (s_OperServ, source, "Current clones: \2%d\2 (%d hosts)", cnt, hosts);
    }
    notice (s_OperServ, source, "Maximum users : \2%d\2", maxusercnt);
    notice (s_OperServ, source, "Services up \2%s\2.", time_ago (start_time, 0));
    if (start_time != reset_time)
	notice (s_OperServ, source, "Services last reset \2%s\2.", time_ago (reset_time, 0));

    if (extra && is_services_op (source))
    if (stricmp (extra, "ALL") == 0)
    {
	long count, mem;

	get_user_stats (&count, &mem);
	notice (s_OperServ, source,
		"User    : \2%6d\2 records, \2%5d\2 kB",
		count, (mem + 1023) / 1024);
	get_channel_stats (&count, &mem);
	notice (s_OperServ, source,
		"Channel : \2%6d\2 records, \2%5d\2 kB",
		count, (mem + 1023) / 1024);
	if(nickserv_on==TRUE)
	{
	    get_nickserv_stats (&count, &mem);
	    notice (s_OperServ, source, "NickServ: \2%6d\2 records, \2%5d\2 kB", count, (mem + 1023) / 1024);
	}
	if(chanserv_on==TRUE)
	{
	    get_chanserv_stats (&count, &mem);
	    notice (s_OperServ, source, "ChanServ: \2%6d\2 records, \2%5d\2 kB", count, (mem + 1023) / 1024);
	}
	if (services_level == 1)
	{
	    if(memos_on==TRUE)
	    {
		get_memoserv_stats (&count, &mem);
		notice (s_OperServ, source, "MemoServ: \2%6d\2 records, \2%5d\2 kB", count, (mem + 1023) / 1024);
	    }
	    if(news_on==TRUE)
	    {
		get_newsserv_stats (&count, &mem);
		notice (s_OperServ, source, "NewsServ: \2%6d\2 records, \2%5d\2 kB", count, (mem + 1023) / 1024);
	    }
	}
	

	if(akill_on==TRUE)
	{
	    if(clones_on==TRUE)
		notice (s_OperServ, source, "OperServ: \2%6d\2 records, \2%5d\2 kB",nakill + nclone, ((akill_size * sizeof (*akills) + 1023) + (clone_size * sizeof (*clones) + 1023)) / 1024);
	    else
		notice (s_OperServ, source, "OperServ: \2%6d\2 records, \2%5d\2 kB",nakill, (akill_size * sizeof (*akills) + 1023) / 1024);
	}
	else
	    notice (s_OperServ, source, "OperServ: \2%6d\2 records, \2%5d\2 kB",nclone, (clone_size * sizeof (*clones) + 1023) / 1024);
    }
    else if (stricmp (extra, "NICK") == 0)
    {
	NickInfo *ni;
	int i, links = 0, regs = 0, forbid = 0, suspend = 0;
	for (i=33; i<256; i++)
	    for (ni = nicklists[i]; ni; ni = ni->next)
		if (ni->flags & NI_VERBOTEN)
		    forbid++;
		else if (host(ni)->flags & NI_SUSPENDED)
		    suspend++;
		else if (ni->flags & NI_SLAVE)
		    links++;
		else
		    regs++;
	notice (s_OperServ, source, "Registered / Linked    : \37\2%d\2\37 / \37\2%d\2\37", regs, links);
	notice (s_OperServ, source, "Forbidden  / Suspended : \37\2%d\2\37 / \37\2%d\2\37", forbid, suspend);
	notice (s_OperServ, source, "Registers  / Links     : \2%d\2 / \2%d\2", S_nick_reg, S_nick_link);
	notice (s_OperServ, source, "Identifies / Drops     : \2%d\2 / \2%d\2", S_nick_ident, S_nick_drop);
	notice (s_OperServ, source, "Recovers   / Releases  : \2%d\2 / \2%d\2", S_nick_recover, S_nick_release);
	notice (s_OperServ, source, "Ghosts     / Kills     : \2%d\2 / \2%d\2", S_nick_ghost, S_nick_kill);
	notice (s_OperServ, source, "Getpasses  / Forbids   : \2%d\2 / \2%d\2", S_nick_getpass, S_nick_forbid);
	notice (s_OperServ, source, "Suspends   / Unsuspends: \2%d\2 / \2%d\2", S_nick_suspend, S_nick_unsuspend);
    }
}

#ifdef DAL_SERV
static void
do_qline (const char *source)
{
    char *nick = strtok (NULL, " ");
    char *s = strtok (NULL, "");
    if (!nick)
	return;

    if (!s)
	send_cmd (NULL, "SQLINE %s", nick);
    else
	send_cmd (NULL, "SQLINE %s :%s", nick, s);
    wallops (s_OperServ, OS_QLINE, source, nick);
}

static void
do_unqline (const char *source)
{
    char *nick = strtok (NULL, " ");
    if (!nick)
	return;

    send_cmd (NULL, "UNSQLINE %s", nick);
    wallops (s_OperServ, OS_UNQLINE, source, nick);
}

static void
do_noop (const char *source)
{
    char *serv = strtok (NULL, " ");
    char *s = strtok (NULL, "");
    if (!s || (s[0] != '+' && s[0] != '-'))
	return;

    send_cmd (s_OperServ, "SVSNOOP 1 :%s", serv, s);
    if (s[0] == '+')
	wallops (s_OperServ, OS_SVSNOOP_ON, source, serv);
    else
	wallops (s_OperServ, OS_SVSNOOP_OFF, source, serv);
}

static void
do_os_kill (const char *source)
{
    char *nick = strtok (NULL, " ");
    char *s = strtok (NULL, "");
    if (!s || !finduser (nick))
	return;

    send_cmd (s_OperServ, "SVSKILL %s :%s", finduser (nick)->nick, s);
}
#endif

static void
do_ignore (const char *source)
{
    char *option = strtok (NULL, " ");
    char *param = strtok (NULL, "");
    int i;

    if (!option || (stricmp (option, "LIST") != 0 && !param))
	notice (s_OperServ, source,
		"Syntax: \2IGNORE\2 \37{ADD|DEL|LIST}\37 [\37nick\37]");
    else if (stricmp (option, "ADD") == 0)
    {
	if (ignorecnt >= ignore_size)
	{
	    if (ignore_size < 8)
		ignore_size = 8;
	    else
		ignore_size *= 2;
	    ignore = srealloc (ignore, sizeof (*ignore) * ignore_size);
	}
	strscpy (ignore[ignorecnt].nick, param, NICKMAX);
	ignore[ignorecnt].start = 0;
	ignorecnt++;
	notice (s_OperServ, source, LIST_ADDED, param, "services", "IGNORE");

    }
    else if (stricmp (option, "DEL") == 0)
    {
	int cnt = 0;
	if (strspn (param, "1234567890") == strlen (param) &&
	    (i = atoi (param)) > 0 && i <= ignorecnt)
	    strscpy (param, ignore[i - 1].nick, NICKMAX);
	for (i = 0; i < ignorecnt; ++i)
	    if (match_wild_nocase (param, ignore[i].nick))
	    {
		--ignorecnt;
		if (i < ignorecnt)
		    bcopy (ignore + i + 1, ignore + i, sizeof (*ignore) * (ignorecnt - i));
		i--;
		cnt++;
	    }
	notice (s_OperServ, source, LIST_REMOVED_MASK,
		cnt, cnt == 1 ? "y" : "ies", param, "services", "IGNORE");

    }
    else if (stricmp (option, "LIST") == 0)
    {
	notice (s_OperServ, source, "Services ignorance list:");
	for (i = 0; i < ignorecnt; i++)
	    if (!ignore[i].start && (!param ||
				 match_wild_nocase (param, ignore[i].nick)))
		notice (s_OperServ, source, "%d  %s%s", i + 1, ignore[i].nick,
			finduser (ignore[i].nick) ? " (online)" : "");
    }
    else
	notice (s_OperServ, source, ERR_UNKNOWN_OPTION, option, s_OperServ,
								"IGNORE");
}

static void
do_jupe (const char *source)
{
    char *serv = strtok (NULL, " ");
    char *reason = strtok (NULL, "");
    if (!serv)
	notice (s_OperServ, source, "Syntax: \2JUPE \37servername\37 [\37reason\37]\2");
    else
    {
	wallops (s_OperServ, OS_JUPE, serv, source);
	if (reason)
	    send_cmd (NULL, "SERVER %s 2 :JUPE: %s", serv, reason);
	else
	    send_cmd (NULL, "SERVER %s 2 :Jupitered server", serv);
    }
}

static void
do_off (const char *source)
{
    char *s = strtok (NULL, "");
    if (runflags & RUN_MODE)
    {
	runflags &= ~RUN_MODE;
	write_log ("Log closed with OFF command from %s", source);
	close_log ();
	wallops (s_OperServ, ONOFF_NOTIFY, "OFF", source);
	if (s)
	{
	    if (offreason = malloc (strlen (s) + 1))
		snprintf (offreason, strlen (s) + 1, "%s", s);
	    else
		offreason = s;
	}
	if(globalnoticer_on==TRUE)
	{
	    if (offreason)
		noticeall (s_GlobalNoticer, SERVICES_OFF_REASON, offreason);
	    else
		noticeall (s_GlobalNoticer, SERVICES_OFF);
	}
	else
	{
	    if (offreason)
		notice (s_OperServ, source, SERVICES_OFF_REASON, offreason);
	    else
		notice (s_OperServ, source, SERVICES_OFF);
	}
    }
}


static void
do_on (const char *source)
{
    if (!(runflags & RUN_MODE))
    {
	runflags |= RUN_MODE;
	open_log ();
	write_log ("Log opened with ON command from %s", source);
	wallops (s_OperServ, ONOFF_NOTIFY, "ON", source);
	if (offreason)
	    free(offreason);
	offreason = NULL;

	if(globalnoticer_on==TRUE)
	    noticeall (s_GlobalNoticer, SERVICES_ON);
	else
	    notice (s_OperServ, source, SERVICES_ON);
    }
}

static void
do_update (const char *source)
{
    notice (s_OperServ, source, OS_UPDATE);
    runflags |= RUN_SAVE_DATA;
}

static void
do_reload (const char *source)
{
    quitmsg = malloc (BUFSIZE);
    if (!quitmsg)
	quitmsg = "RELOAD command recieved, but out of memory!";
    else
	snprintf (quitmsg, BUFSIZE, "RELOAD command received from %s", source);
    runflags |= RUN_QUITTING;
    runflags |= RUN_NOSLEEP;
}

static void
do_os_quit (const char *source)
{
    quitmsg = malloc (BUFSIZE);
    if (!quitmsg)
	quitmsg = "QUIT command received, but out of memory!";
    else
	snprintf (quitmsg, BUFSIZE, "QUIT command received from %s", source);
    runflags |= RUN_QUITTING;
    runflags |= RUN_TERMINATING;
}

static void
do_shutdown (const char *source)
{
    do_update (source);
    do_os_quit (source);
}

static void
do_raw (const char *source)
{
    char *text = strtok (NULL, "");
    if (!text)
	notice (s_OperServ, source, "Syntax: \2RAW \37command\37\2");
    else
	send_cmd (NULL, text);
}


/*************************************************************************/
/************************** Services Operators ***************************/
/*************************************************************************/

void
load_sop ()
{
    FILE *f = fopen (sop_db, "r");
    int i;

    if (!f)
    {
	log_perror ("Can't read SOP database %s", sop_db);
	return;
    }
    switch (i = get_file_version (f, sop_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
	nsop = fgetc (f) * 256 + fgetc (f);
	if (nsop < 8)
	    sop_size = 16;
	else
	    sop_size = 2 * nsop;
	sops = srealloc (sops, sizeof (Sop) * sop_size);
	if (!nsop)
	{
	    fclose (f);
	    return;
	}
	if (nsop != fread (sops, sizeof (Sop), nsop, f))
	    fatal_perror ("Read error on %s", sop_db);
	break;
    default:
	fatal ("Unsupported version (%d) on %s", i, sop_db);
    }				/* switch (version) */
    fclose (f);
}

/*************************************************************************/

void
save_sop ()
{
    FILE *f;
    char sop_dbsave[2048];

    strcpy (sop_dbsave, sop_db);
    strcat (sop_dbsave, ".save");
    remove (sop_dbsave);
    if (rename (sop_db, sop_dbsave) < 0)
	log_perror ("Can't back up %s", sop_db);
    f = fopen (sop_db, "w");
    if (!f)
    {
	log_perror ("Can't write to SOP database %s", sop_db);
	if (rename (sop_dbsave, sop_db) < 0)
	    fatal_perror ("Can't restore backup copy of %s", sop_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't change group of %s to %d", sop_db, file_gid);
#endif
    write_file_version (f, sop_db);

    fputc (nsop / 256, f);
    fputc (nsop & 255, f);
    if (fwrite (sops, sizeof (Sop), nsop, f) != nsop)
	fatal_perror ("Write error on %s", sop_db);
    fclose (f);
    remove (sop_dbsave);
}

/*************************************************************************/

/* Handle an SOP command. */

static void
do_sop (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2SOP {ADD|DEL|LIST} \2[\37user\37]");
	notice (s_OperServ, source, ERR_MORE_INFO,
		s_OperServ, "SOP");
    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_ADMIN, do_sop_add},
	    {"ALLOW", H_ADMIN, do_sop_add},
	    {"GRANT", H_ADMIN, do_sop_add},
	    {"DEL*", H_ADMIN, do_sop_del},
	    {"ERASE", H_ADMIN, do_sop_del},
	    {"TRASH", H_ADMIN, do_sop_del},
	    {"LIST*", H_ADMIN, do_sop_list},
	    {"VIEW", H_ADMIN, do_sop_list},
	    {"DISP*", H_ADMIN, do_sop_list},
	    {"SHOW*", H_ADMIN, do_sop_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2SOP {ADD|DEL|LIST} [\37user\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION,
		    cmd, s_OperServ, "SOP");
	}
    }
}

void
do_sop_add (const char *source)
{
    char *nick;
    if (nick = strtok (NULL, " "))
    {
	if (is_justservices_op (nick))
	    notice (s_OperServ, source, LIST_THERE, nick, "services", "SOP");
	else
	{
	    if(nickserv_on==TRUE)
	    {
		NickInfo *ni = host (findnick (nick));
		if (!ni)
		    notice (s_OperServ, source, NS_NOT_REGISTERED, nick);
		else if (ni->flags & NI_IRCOP)
		{
		    strscpy (nick, ni->nick, NICKMAX);
		    if (nsop >= sop_size)
		    {
			if (sop_size < 8)
			    sop_size = 8;
			else
			    sop_size *= 2;
			sops = srealloc (sops, sizeof (*sops) * sop_size);
		    }
		    strscpy (sops[nsop].nick, nick, NICKMAX);
		    notice (s_OperServ, source, LIST_ADDED, nick, "services", "SOP");
		    ++nsop;
		    if (services_level != 1)
			notice (s_OperServ, source, ERR_READ_ONLY);
		}
		else 
		    notice (s_OperServ, source, ERR_ONLY_ON_IRCOP, "SOP ADD");
	    }
	    else
	    {
		if (nsop >= sop_size)
		{
		    if (sop_size < 8)
			sop_size = 8;
		    else
			sop_size *= 2;
		    sops = srealloc (sops, sizeof (*sops) * sop_size);
		}
		strscpy (sops[nsop].nick, nick, NICKMAX);
		notice (s_OperServ, source, "%s added to SOP list", nick);
		++nsop;
		if (services_level != 1)
		    notice (s_OperServ, source, ERR_READ_ONLY);
	    }
	}
    }
    else
	notice (s_OperServ, source,
		"Syntax: SOP ADD \37nick\37");
}

void
do_sop_del (const char *source)
{
    char *nick;
    int i;
    if (nick = strtok (NULL, " "))
    {
	if (strspn (nick, "1234567890") == strlen (nick) &&
	    (i = atoi (nick)) > 0 && i <= nsop)
	    strscpy (nick, sops[i - 1].nick, NICKMAX);

	for (i = 0; i < nsop; ++i)
	    if (stricmp (sops[i].nick, nick) == 0)
	    {
		strscpy (nick, sops[i].nick, NICKMAX);
		--nsop;
		if (i < nsop)
		    bcopy (sops + i + 1, sops + i, sizeof (*sops) * (nsop - i));
		i--;
		break;
	    }
	if (i < nsop) {
	    notice (s_OperServ, source, LIST_REMOVED, nick, "services", "SOP");
	    if (services_level != 1)
		notice (s_OperServ, source, ERR_READ_ONLY);
	} else
	    notice (s_OperServ, source, LIST_NOT_THERE, nick, "services", "SOP");
    }
    else
	notice (s_OperServ, source, "Syntax: SOP DEL \37nick\37");
}

void
do_sop_list (const char *source)
{
    char *s;
    int i;
    s = strtok (NULL, " ");
    if (s)
	strlower (s);
    notice (s_OperServ, source, "Current SOP list:");
    for (i = 0; i < nsop; ++i)
	if (!s || match_wild_nocase (s, sops[i].nick))
	    notice (s_OperServ, source, "  %3d %s",
		    i + 1, sops[i].nick);
}

int
is_services_op (const char *nick)
{
    int i;
    NickInfo *ni = findnick (nick);

    for (i = 0; i < nsop; ++i)
    {
	if(nickserv_on==TRUE)
	{
	    if (ni && (stricmp (sops[i].nick, host (ni)->nick) == 0 ||
			stricmp (sops[i].nick, ni->nick) == 0) &&
			(ni->flags & NI_IDENTIFIED) && is_oper(nick))
		return 1;
	}
	else if (stricmp (sops[i].nick, nick) == 0 && is_oper(nick))
	    return 1;
    }
    return 0;
}

int
is_justservices_op (const char *nick)
{
    int i;
    NickInfo *ni = findnick (nick);

    for (i = 0; i < nsop; ++i)
    {
	if(nickserv_on==TRUE)
	{
	    if (ni && (stricmp (sops[i].nick, host (ni)->nick) == 0 ||
			stricmp (sops[i].nick, ni->nick) == 0))
		return 1;
	}
	else if (stricmp (sops[i].nick, nick) == 0)
	    return 1;
    }
    return 0;
}


/*************************************************************************/
/************************** Logon/Oper MESSAGES **************************/
/*************************************************************************/

void
load_message ()
{
    FILE *f = fopen (message_db, "r");
    int i;

    if (!f)
    {
	log_perror ("Can't read MESSAGE database %s", message_db);
	return;
    }
    switch (i = get_file_version (f, message_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
	nmessage = fgetc (f) * 256 + fgetc (f);
	if (nmessage < 8)
	    message_size = 16;
	else
	    message_size = 2 * nmessage;
	messages = smalloc (sizeof (*messages) * message_size);
	if (!nmessage)
	{
	    fclose (f);
	    return;
	}
	if (nmessage != fread (messages, sizeof (*messages), nmessage, f))
	    fatal_perror ("Read error on %s", message_db);
	for (i = 0; i < nmessage; ++i)
	{
	    messages[i].text = read_string (f, message_db);
	}
	break;

    default:
	fatal ("Unsupported version (%d) on %s", i, message_db);
    }				/* switch (version) */
    fclose (f);
}

/*************************************************************************/

void
save_message ()
{
    FILE *f;
    int i;
    char message_dbsave[2048];

    strcpy (message_dbsave, message_db);
    strcat (message_dbsave, ".save");
    remove (message_dbsave);
    if (rename (message_db, message_dbsave) < 0)
	log_perror ("Can't back up %s", message_db);
    f = fopen (message_db, "w");
    if (!f)
    {
	log_perror ("Can't write to MESSAGE database %s", message_db);
	if (rename (message_dbsave, message_db) < 0)
	    fatal_perror ("Can't restore backup copy of %s", message_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't change group of %s to %d", message_db, file_gid);
#endif
    write_file_version (f, message_db);

    fputc (nmessage / 256, f);
    fputc (nmessage & 255, f);
    if (fwrite (messages, sizeof (*messages), nmessage, f) != nmessage)
	fatal_perror ("Write error on %s", message_db);
    for (i = 0; i < nmessage; ++i)
    {
	write_string (messages[i].text, f, message_db);
    }
    fclose (f);
    remove (message_dbsave);
}

/*************************************************************************/

static void
do_logonmsg (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2LOGONMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	notice (s_OperServ, source,
		ERR_MORE_INFO, s_OperServ, "LOGONMSG");

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_SOP, do_logonmsg_add},
	    {"WRITE", H_SOP, do_logonmsg_add},
	    {"COMPOSE", H_SOP, do_logonmsg_add},
	    {"DEL*", H_SOP, do_logonmsg_del},
	    {"ERASE", H_SOP, do_logonmsg_del},
	    {"TRASH", H_SOP, do_logonmsg_del},
	    {"LIST*", H_SOP, do_logonmsg_list},
	    {"VIEW", H_SOP, do_logonmsg_list},
	    {"DISP*", H_SOP, do_logonmsg_list},
	    {"SHOW*", H_SOP, do_logonmsg_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2LOGONMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION,
		    cmd, s_OperServ, "LOGONMSG");
	}
    }
}

static void
do_opermsg (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2OPERMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	notice (s_OperServ, source, ERR_MORE_INFO,
		s_OperServ, "OPERMSG");

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_SOP, do_opermsg_add},
	    {"WRITE", H_SOP, do_opermsg_add},
	    {"COMPOSE", H_SOP, do_opermsg_add},
	    {"DEL*", H_SOP, do_opermsg_del},
	    {"ERASE", H_SOP, do_opermsg_del},
	    {"TRASH", H_SOP, do_opermsg_del},
	    {"LIST*", H_SOP, do_opermsg_list},
	    {"VIEW", H_SOP, do_opermsg_list},
	    {"DISP*", H_SOP, do_opermsg_list},
	    {"SHOW*", H_SOP, do_opermsg_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2OPERMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION,
		    cmd, s_OperServ, "OPERMSG");
	}
    }
}

void
do_logonmsg_add (const char *source)
{
    do_message_add (source, M_LOGON);
}
void
do_opermsg_add (const char *source)
{
    do_message_add (source, M_OPER);
}

void
do_message_add (const char *source, short type)
{
    char *text = strtok (NULL, "");
    if (!text)
	notice (s_OperServ, source, "Syntax: %sMSG ADD \37text\37",
		getmsgname (type));
    else
    {
	notice (s_OperServ, source, LIST_ADDED,
		myitoa(add_message (source, text, type)),
		getmsgname (type), "messages");
	wallops (s_OperServ, OS_NEW_MESSAGE, source, getmsgname (type));
	if (services_level != 1)
	    notice (s_OperServ, source, ERR_READ_ONLY);
    }
}

void
do_logonmsg_del (const char *source)
{
    do_message_del (source, M_LOGON);
}
void
do_opermsg_del (const char *source)
{
    do_message_del (source, M_OPER);
}

void
do_message_del (const char *source, short type)
{
    char *text = strtok (NULL, " ");
    int num = 1;
    if (!text)
	notice (s_OperServ, source, "Syntax: %sMSG DEL \37num%s\37",
		getmsgname (type),
		is_services_admin (source) ? "|ALL" : "");
    else
    {
	if (stricmp (text, "ALL") != 0)
	{
	    num = atoi (text);
	    if (num > 0 && del_message (num, type)) {
		notice (s_OperServ, source, LIST_REMOVED_NUM,
			num, getmsgname (type), "messages");
		if (services_level != 1)
		    notice (s_OperServ, source, ERR_READ_ONLY);
	    } else
		notice (s_OperServ, source, LIST_NOT_FOUND,
			num, getmsgname (type), "messages");
	}
	else
	{
	    if (is_services_admin (source))
	    {
		while (del_message (num, type));
		notice (s_OperServ, source, LIST_REMOVED_ALL,
			getmsgname (type), "messages");
		if (services_level != 1)
		    notice (s_OperServ, source, ERR_READ_ONLY);
	    }
	    else
		notice (s_OperServ, source, ERR_ACCESS_DENIED);
	}
    }
}

void
do_logonmsg_list (const char *source)
{
    do_message_list (source, M_LOGON);
}
void
do_opermsg_list (const char *source)
{
    do_message_list (source, M_OPER);
}

void
do_message_list (const char *source, short type)
{
    int i, num;
    notice (s_OperServ, source, "Current %s Message list:",
	    getmsgname (type));
    for (i = 0, num = 1; i < nmessage; ++i)
	if (messages[i].type == type)
	{
	    notice (s_OperServ, source, "  %3d by %s set %s ago",
		    num,
		    *messages[i].who ? messages[i].who : "<unknown>",
		    time_ago (messages[i].time, 1));
	    notice (s_OperServ, source, "      %s", messages[i].text);
	    num++;
	}
}

/*************************************************************************/

static int
add_message (const char *source, const char *text, short type)
{
    int i, num;
    if (nmessage >= message_size)
    {
	if (message_size < 8)
	    message_size = 8;
	else
	    message_size *= 2;
	messages = srealloc (messages, sizeof (*messages) * message_size);
    }
    messages[nmessage].text = sstrdup (text);
    messages[nmessage].type = type;
    messages[nmessage].time = time (NULL);
    strscpy (messages[nmessage].who, source, NICKMAX);
    ++nmessage;
    for (i = 0, num = 0; i < nmessage; ++i)
	if (messages[i].type == type)
	    num++;
    return num;
}

/*************************************************************************/

static char *
getmsgname (short type)
{
    static char ret[16];	/* Ewww .. I hade hard-coded lengths */
    int i;
    strscpy(ret, "UNKNOWN", 16);
    for (i = 0; messagename[i].what >= 0; i++)
	if (type == messagename[i].what)
	    strscpy(ret, messagename[i].name, 16);
    return strupper (ret);
}

/*************************************************************************/

/* Return whether the text was found in the MESSAGE list. */

static int
del_message (int num, short type)
{
    int i, find = 0;

    for (i = 0, find = 0; i < nmessage; ++i)
	if (messages[i].type == type)
	{
	    find++;
	    if (num == find)
	    {
		if (messages[i].text)
		    free (messages[i].text);
		--nmessage;
		if (i < nmessage)
		    bcopy (messages + i + 1, messages + i, sizeof (*messages) * (nmessage - i));
		return 1;
	    }
	}
    return 0;
}

/*************************************************************************/
/****************************** AKILL stuff ******************************/
/*************************************************************************/

void
load_akill ()
{
    FILE *f = fopen (akill_db, "r");
    int i;

    if (!f)
    {
	log_perror ("Can't read AKILL database %s", akill_db);
	return;
    }
    switch (i = get_file_version (f, akill_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
	nakill = fgetc (f) * 256 + fgetc (f);
	if (nakill < 8)
	    akill_size = 16;
	else
	    akill_size = 2 * nakill;
	akills = smalloc (sizeof (*akills) * akill_size);
	if (!nakill)
	{
	    fclose (f);
	    return;
	}
	if (nakill != fread (akills, sizeof (*akills), nakill, f))
	    fatal_perror ("Read error on %s", akill_db);
	for (i = 0; i < nakill; ++i)
	{
	    akills[i].mask = read_string (f, akill_db);
	    akills[i].reason = read_string (f, akill_db);
	}
	break;

    case 1:
	{
	    Akill_V1 old_akill;
	    nakill = fgetc (f) * 256 + fgetc (f);
	    if (nakill < 8)
		akill_size = 16;
	    else
		akill_size = 2 * nakill;
	    akills = smalloc (sizeof (*akills) * akill_size);
	    if (!nakill)
	    {
		fclose (f);
		return;
	    }
	    for (i = 0; i < nakill; ++i)
	    {
		if (1 != fread (&old_akill, sizeof (old_akill), 1, f))
		    fatal_perror ("Read error on %s", akill_db);
		akills[i].time = old_akill.time;
		akills[i].who[0] = 0;
	    }
	    for (i = 0; i < nakill; ++i)
	    {
		akills[i].mask = read_string (f, akill_db);
		akills[i].reason = read_string (f, akill_db);
	    }
	    break;
	}			/* case 1 */

    default:
	fatal ("Unsupported version (%d) on %s", i, akill_db);
    }				/* switch (version) */
    fclose (f);
}

/*************************************************************************/

void
save_akill ()
{
    FILE *f;
    int i;
    char akill_dbsave[2048];

    strcpy (akill_dbsave, akill_db);
    strcat (akill_dbsave, ".save");
    remove (akill_dbsave);
    if (rename (akill_db, akill_dbsave) < 0)
	log_perror ("Can't back up %s", akill_db);
    f = fopen (akill_db, "w");
    if (!f)
    {
	log_perror ("Can't write to AKILL database %s", akill_db);
	if (rename (akill_dbsave, akill_db) < 0)
	    fatal_perror ("Can't restore backup copy of %s", akill_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't change group of %s to %d", akill_db, file_gid);
#endif
    write_file_version (f, akill_db);

    fputc (nakill / 256, f);
    fputc (nakill & 255, f);
    if (fwrite (akills, sizeof (*akills), nakill, f) != nakill)
	fatal_perror ("Write error on %s", akill_db);
    for (i = 0; i < nakill; ++i)
    {
	write_string (akills[i].mask, f, akill_db);
	write_string (akills[i].reason, f, akill_db);
    }
    fclose (f);
    remove (akill_dbsave);
}

/*************************************************************************/

/* Handle an AKILL command. */

static void
do_akill (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2AKILL {ADD|DEL|LIST} [\37hostmask\37]\2");
	notice (s_OperServ, source, ERR_MORE_INFO, s_OperServ, "AKILL");

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_OPER, do_akill_add},
	    {"DISALLOW", H_OPER, do_akill_add},
	    {"DENY*", H_OPER, do_akill_add},
	    {"DEL*", H_OPER, do_akill_del},
	    {"ERASE", H_OPER, do_akill_del},
	    {"TRASH", H_OPER, do_akill_del},
	    {"LIST*", H_OPER, do_akill_list},
	    {"VIEW", H_OPER, do_akill_list},
	    {"DISP*", H_OPER, do_akill_list},
	    {"SHOW*", H_OPER, do_akill_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2AKILL {ADD|DEL|LIST} [\37host\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION,
		    cmd, s_OperServ, "AKILL");
	}
    }
}

static void
do_pakill (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2PAKILL {ADD|DEL|LIST} [\37hostmask\37]\2");
	notice (s_OperServ, source, ERR_MORE_INFO, s_OperServ, "AKILL");
    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_SOP, do_pakill_add},
	    {"DISALLOW", H_SOP, do_pakill_add},
	    {"DENY*", H_SOP, do_pakill_add},
	    {"DEL*", H_SOP, do_pakill_del},
	    {"ERASE", H_SOP, do_pakill_del},
	    {"TRASH", H_SOP, do_pakill_del},
	    {"LIST*", H_OPER, do_akill_list},
	    {"VIEW", H_OPER, do_akill_list},
	    {"DISP*", H_OPER, do_akill_list},
	    {"SHOW*", H_OPER, do_akill_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2PAKILL {ADD|DEL|LIST} [\37host\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION, cmd,
			s_OperServ, "AKILL");
	}
    }
}

void
do_akill_add (const char *source)
{
    do_akill_addfunc (source, 0);
}
void
do_pakill_add (const char *source)
{
    do_akill_addfunc (source, 1);
}

void
do_akill_addfunc (const char *source, int call)
{
    char *mask, *reason, *s;
    int i;
    if ((mask = strtok (NULL, " ")) && (reason = strtok (NULL, "")))
    {
	int nonchr = 0;
	for (i = 0; mask[i] != '!' && i < strlen (mask); i++);
	if (i < strlen (mask))
	{
	    notice (s_OperServ, source, ERR_MAY_NOT_HAVE, '!');
	    return;
	}
	strlower (mask);
	if (!(s = strchr (mask, '@')))
	{
	    notice (s_OperServ, source, ERR_MUST_HAVE, '@');
	    return;
	}
	if (mask[0] == '@')
	{
	    char *mymask;
	    mymask = smalloc (strlen (mask) + 2);
	    strcpy (mymask, "*");
	    strcat (mymask, mask);
	    strcpy (mask, mymask);
	    free (mymask);
	}
	/* Find @*, @*.*, @*.*.*, etc. and dissalow if !SOP */
	for (i = strlen (mask) - 1; i > 0 && mask[i] != '@'; i--)
	    if (!(mask[i] == '*' || mask[i] == '?' || mask[i] == '.'))
		nonchr++;
	if (nonchr < starthresh && !is_services_op (source))
	{
	    notice (s_OperServ, source, ERR_STARTHRESH, starthresh);
	    return;
	}
	else if (nonchr < starthresh)
	{
	    for (i--; i > 0 && (mask[i] == '*' || mask[i] == '?'); i--);
	    if (!i)
	    {
		notice (s_OperServ, source, ERR_STARTHRESH, 1);
		return;
	    }
	}
	if (is_on_akill (mask))
	    notice (s_OperServ, source, LIST_THERE, mask, "services", "AKILL");
	else {
	    notice (s_OperServ, source, OS_AKILL_ADDED, mask,
	    	call ? "P" : "",add_akill (mask, reason, source, call));
	    if (services_level != 1)
		notice (s_OperServ, source, ERR_READ_ONLY);
	}
    }
    else
	notice (s_OperServ, source, "Syntax: AKILL ADD \37mask\37 \37reason\37");
}

void
do_akill_del (const char *source)
{
    do_akill_delfunc (source, 0);
}

void
do_pakill_del (const char *source)
{
    do_akill_delfunc (source, 1);
}

void
do_akill_delfunc (const char *source, int call)
{
    char *mask;
    int i;
    if (mask = strtok (NULL, " "))
    {
	if (strspn (mask, "1234567890") == strlen (mask) &&
	    (i = atoi (mask)) > 0 && i <= nakill)
	    strcpy (mask, akills[i - 1].mask);
	if (i = del_akill (mask, call)) {
	    notice (s_OperServ, source, LIST_REMOVED_MASK,
		    i, i == 1 ? "y" : "ies", mask, "services", "AKILL");
	    if (services_level != 1)
		notice (s_OperServ, source, ERR_READ_ONLY);
	} else if (call == 1)
	    notice (s_OperServ, source, LIST_NOT_THERE, mask, "services", "AKILL");
	else
	    notice (s_OperServ, source, OS_AKILL_NOT_THERE, mask);
    }
    else
	notice (s_OperServ, source, "Syntax: AKILL DEL \37mask\37");
    if (services_level != 1)
	notice (s_OperServ, source, ERR_READ_ONLY);
}

void
do_akill_list (const char *source)
{
    char *s;
    int i;
    s = strtok (NULL, " ");
    if (s)
    {
	strlower (s);
	strchr (s, '@');
    }
    notice (s_OperServ, source, "Current AKILL list:");
    for (i = 0; i < nakill; ++i)
	if (!s || match_wild_nocase (s, akills[i].mask))
	{
	    notice (s_OperServ, source, "  %3d %s (by %s %s %s%s)",
		    i + 1, akills[i].mask,
		    *akills[i].who ? akills[i].who : "<unknown>",
		    akills[i].time ? "set" : "is",
		    akills[i].time ? time_ago (akills[i].time, 1)
		    : "PERMINANT", akills[i].time ? " ago" : "");
	    notice (s_OperServ, source, "      %s", akills[i].reason);
	}
}

static int
is_on_akill (const char *mask)
{
    int i;

    for (i = 0; i < nakill; ++i)
	if (match_wild_nocase (akills[i].mask, mask))
	    return 1;
    return 0;
}

/*************************************************************************/

void
expire_akill ()
{
    int i;
    const time_t expire_time = akill_expire * 24 * 60 * 60;
    time_t tm;
    for (i = 0; i < nakill; ++i)
	if (akills[i].time > 0)
	{
	    tm = time (NULL) - akills[i].time;
	    if (tm > expire_time)
	    {
#ifdef IRC_DALNET
		char *nick, *user, *host;
		split_usermask (akills[i].mask, &nick, &user, &host);
		send_cmd (server_name, "RAKILL %s %s", host, user);
		free(nick);
		free(user);
		free(host);
#else
		send_cmd (server_name, "UNGLINE * %s", akills[i].mask);
#endif
		wallops (s_OperServ, OS_AKILL_EXPIRE, akills[i].mask,
				akills[i].reason, akills[i].who);
		if (akills[i].mask)
		    free (akills[i].mask);
		if (akills[i].reason)
		    free (akills[i].reason);
		--nakill;
		if (i < nakill)
		    bcopy (akills + i + 1, akills + i, sizeof (*akills) * (nakill - i));
	    }
	}
}

/*************************************************************************/

/* Does the user match any AKILLs? */

int
check_akill (const char *nick, const char *username, const char *host)
{
    char buf[512];
    int i;

    strscpy (buf, username, sizeof (buf) - 2);
    i = strlen (buf);
    buf[i++] = '@';
    strlower (strscpy (buf + i, host, sizeof (buf) - i));
    for (i = 0; i < nakill; ++i)
	if (match_wild_nocase (akills[i].mask, buf))
	{
	    char *av[2], *mynick, *myuser, *myhost;
	    av[0] = sstrdup (nick);
	    av[1] = sstrdup (akills[i].reason);
	    if (!i_am_backup ())
	    {
		send_cmd (s_OperServ, "KILL %s :" OS_AKILL_BANNED, nick,
						akills[i].reason);
		do_kill (s_OperServ, 2, av);
	    }
	    free (av[1]);
	    free (av[0]);

#ifdef IRC_DALNET
	    split_usermask (akills[i].mask, &mynick, &myuser, &myhost);
	    send_cmd (server_name,
		      "AKILL %s %s :" OS_AKILL_BANNED,
		      myhost, myuser, akills[i].reason);
	    free (mynick);
	    free (myuser);
	    free (myhost);
#else
	    send_cmd (server_name,
		      "GLINE * +99999999 %s :" OS_AKILL_BANNED,
		      akills[i].mask, akills[i].reason);
#endif
	    return 1;
	}
    return 0;
}

/*************************************************************************/

static int
add_akill (const char *mask, const char *reason, const char *who, int call)
{
    User *user;
    int cnt, killd;
    char *buf;
    buf=smalloc(strlen (reason) + 20);
    snprintf (buf, strlen (reason) + 20, OS_AKILL_BANNED, reason);

    if (nakill >= akill_size)
    {
	if (akill_size < 8)
	    akill_size = 8;
	else
	    akill_size *= 2;
	akills = srealloc (akills, sizeof (*akills) * akill_size);
    }
    akills[nakill].mask = sstrdup (mask);
    akills[nakill].reason = sstrdup (reason);
    if (call == 1)
	akills[nakill].time = 0;
    else
	akills[nakill].time = time (NULL);
    strscpy (akills[nakill].who, who, NICKMAX);
    ++nakill;

    cnt = killd = countusermask (mask);
    for (; cnt; cnt--)
    {
	user = findusermask (mask, cnt);
	kill_user (s_OperServ, user->nick, buf);
    }
    free (buf);
    return killd;
}

/*************************************************************************/

/* Return whether the mask was found in the AKILL list. */

int
del_akill (const char *mask, int call)
{
    int i, ret = 0;

    for (i = 0; i < nakill; ++i)
    {
	if (match_wild_nocase (akills[i].mask, mask))
	{
	    if (!(akills[i].time == 0 && call != 1))
	    {
#ifdef IRC_DALNET
		char *nick, *user, *host;
		split_usermask (akills[i].mask, &nick, &user, &host);
		send_cmd (server_name, "RAKILL %s %s", host, user);
		free(nick);
		free(user);
		free(host);
#else
		send_cmd (server_name, "UNGLINE * %s", akills[i].mask);
#endif
		if (akills[i].mask)
		    free (akills[i].mask);
		if (akills[i].reason)
		    free (akills[i].reason);
		--nakill;
		if (i < nakill)
		    bcopy (akills + i + 1, akills + i, sizeof (*akills) * (nakill - i));
		ret++;
	    }
	}
    }
    return ret;
}

/*************************************************************************/
/**************************** Clone detection ****************************/
/*************************************************************************/

void
load_clone ()
{
    FILE *f = fopen (clone_db, "r");
    int i;

    if (!f)
    {
	log_perror ("Can't read CLONE database %s", clone_db);
	return;
    }
    switch (i = get_file_version (f, clone_db))
    {
    case 5:
    case 4:
    case 3:
    case 2:
    case 1:
	nclone = fgetc (f) * 256 + fgetc (f);
	if (nclone < 8)
	    clone_size = 16;
	else
	    clone_size = 2 * nclone;
	clones = smalloc (sizeof (*clones) * clone_size);
	if (!nclone)
	{
	    fclose (f);
	    return;
	}
	if (nclone != fread (clones, sizeof (*clones), nclone, f))
	    fatal_perror ("Read error on %s", clone_db);
	for (i = 0; i < nclone; ++i)
	{
	    clones[i].host = read_string (f, clone_db);
	    clones[i].reason = read_string (f, clone_db);
	}
	break;
    default:
	fatal ("Unsupported version (%d) on %s", i, clone_db);
    }				/* switch (version) */
    fclose (f);
}

/*************************************************************************/

void
save_clone ()
{
    FILE *f;
    int i;
    char clone_dbsave[2048];

    strcpy (clone_dbsave, clone_db);
    strcat (clone_dbsave, ".save");
    remove (clone_dbsave);
    if (rename (clone_db, clone_dbsave) < 0)
	log_perror ("Can't back up %s", clone_db);
    f = fopen (clone_db, "w");
    if (!f)
    {
	log_perror ("Can't write to CLONE database %s", clone_db);
	if (rename (clone_dbsave, clone_db) < 0)
	    fatal_perror ("Can't restore backup copy of %s", clone_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't change group of %s to %d", clone_db, file_gid);
#endif
    write_file_version (f, clone_db);

    fputc (nclone / 256, f);
    fputc (nclone & 255, f);
    if (fwrite (clones, sizeof (*clones), nclone, f) != nclone)
	fatal_perror ("Write error on %s", clone_db);
    for (i = 0; i < nclone; ++i)
    {
	write_string (clones[i].host, f, clone_db);
	write_string (clones[i].reason, f, clone_db);
    }
    fclose (f);
    remove (clone_dbsave);
}

/*************************************************************************/

/* Handle an CLONE command. */

static void
do_clone (const char *source)
{
    char *cmd = strtok (NULL, " ");

    if (!cmd)
    {
	notice (s_OperServ, source,
		"Syntax: \2CLONE {ADD|DEL|LIST} [\37host\37]\2");
	notice (s_OperServ, source, ERR_MORE_INFO, s_OperServ, "CLONE");
    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"ADD", H_SOP, do_clone_add},
	    {"ALLOW", H_SOP, do_clone_add},
	    {"GRANT", H_SOP, do_clone_add},
	    {"DEL*", H_SOP, do_clone_del},
	    {"ERASE", H_SOP, do_clone_del},
	    {"TRASH", H_SOP, do_clone_del},
	    {"LIST*", H_SOP, do_clone_list},
	    {"VIEW", H_SOP, do_clone_list},
	    {"DISP*", H_SOP, do_clone_list},
	    {"SHOW*", H_SOP, do_clone_list},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	    (*command->process) (source);
	else
	{
	    notice (s_OperServ, source,
		    "Syntax: \2CLONE {ADD|DEL|LIST} [\37host\37]\2");
	    notice (s_OperServ, source, ERR_UNKNOWN_OPTION, cmd,
		    s_OperServ, "CLONE");
	}
    }
}

static void
do_clone_add (const char *source)
{
    char *host, *amt, *reason;
    host = strtok (NULL, " ");
    amt = strtok (NULL, " ");
    reason = strtok (NULL, "");
    if (reason)
    {
	int i, amount, nonchr = 0;
	for (i = 0; host[i] != '!' && host[i] != '@' && i < strlen (host); i++);
	if (i < strlen (host))
	{
	    notice (s_OperServ, source, ERR_MAY_NOT_HAVE, host[i]);
	    return;
	}
	strlower (host);
	amount = atoi (amt);

	/* Find @*, @*.*, @*.*.*, etc. and dissalow */
	for (i = strlen (host) - 1; i >= 0; i--)
	    if (!(host[i] == '*' || host[i] == '?' || host[i] == '.'))
		nonchr++;
	if (nonchr < starthresh)
	    notice (s_OperServ, source, ERR_STARTHRESH, starthresh);
	else if (is_on_clone (host))
	    notice (s_OperServ, source, LIST_THERE, host, "services", "CLONE");
	else if (amount < 1)
	    notice (s_OperServ, source, OS_CLONE_LOW, 0);
	else if (amount > 999)
	    notice (s_OperServ, source, OS_CLONE_HIGH, 1000);
	else
	{
	    add_clone (host, amount, reason, source);
	    notice (s_OperServ, source, LIST_ADDED, host, "services", "CLONE");
	    if (services_level != 1)
		notice (s_OperServ, source, ERR_READ_ONLY);
	}
    }
    else
	notice (s_OperServ, source,
		"Syntax: CLONE ADD \37host\37 \37amount\37 \37reason\37");
}

static void
do_clone_del (const char *source)
{
    char *host;
    int i;

    host = strtok (NULL, " ");
    if (host)
    {
	if (strspn (host, "1234567890") == strlen (host) &&
	    (i = atoi (host)) > 0 && i <= nclone)
	    strcpy (host, clones[i - 1].host);
	strlower (host);
	if (del_clone (host)) {
	    notice (s_OperServ, source, LIST_REMOVED, host, "services", "CLONE");
	    if (services_level != 1)
		notice (s_OperServ, source, ERR_READ_ONLY);
	} else
	    notice (s_OperServ, source, LIST_NOT_THERE, host, "services", "CLONE");
    }
    else
	notice (s_OperServ, source, "Syntax: CLONE DEL \37host\37");
}

static void
do_clone_list (const char *source)
{
    char *host;
    int i;

    host = strtok (NULL, " ");
    if (host)
	strlower (host);
    notice (s_OperServ, source, "Current CLONE list:");
    for (i = 0; i < nclone; ++i)
	if (!host || match_wild_nocase (host, clones[i].host))
	{
	    notice (s_OperServ, source, "  %3d %s (by %s set %s ago)",
		    i + 1, clones[i].host,
		    *clones[i].who ? clones[i].who : "<unknown>",
		    time_ago (clones[i].time, 1));
	    notice (s_OperServ, source, "      [%3d] %s",
		    clones[i].amount, clones[i].reason);
	}
}

static int
is_on_clone (char *host)
{
    int i;

    strlower (host);
    for (i = 0; i < nclone; ++i)
	if (match_wild_nocase (clones[i].host, host))
	    return 1;
    return 0;
}

/*************************************************************************/

static void
add_clone (const char *host, int amount, const char *reason, const char *who)
{
    if (nclone >= clone_size)
    {
	if (clone_size < 8)
	    clone_size = 8;
	else
	    clone_size *= 2;
	clones = srealloc (clones, sizeof (*clones) * clone_size);
    }
    clones[nclone].host = sstrdup (host);
    clones[nclone].amount = amount;
    clones[nclone].reason = sstrdup (reason);
    clones[nclone].time = time (NULL);
    strscpy (clones[nclone].who, who, NICKMAX);
    ++nclone;
}

/*************************************************************************/

/* Return whether the host was found in the CLONE list. */

int
del_clone (const char *host)
{
    int i;

    if (strspn (host, "1234567890") == strlen (host) &&
	(i = atoi (host)) > 0 && i <= nclone)
	--i;
    else
	for (i = 0; i < nclone && stricmp (clones[i].host, host) != 0; ++i);
    if (i < nclone)
    {
	if (clones[i].host)
	    free (clones[i].host);
	if (clones[i].reason)
	    free (clones[i].reason);
	--nclone;
	if (i < nclone)
	    bcopy (clones + i + 1, clones + i, sizeof (*clones) * (nclone - i));
	return 1;
    }
    else
	return 0;
}

/* We just got a new user; does it look like a clone?  If so, kill the
 * user if not under or at the CLONE threshold.
 */
void
clones_add (const char *nick, const char *host)
{
    Clone *clone;

/* ignore services_host, backup services dont like primary enforcer :P */
    if (stricmp (host, services_host) != 0 && !is_services_nick (nick))
	if (!(clone = findclone (host)))
	{
	    clone = scalloc (sizeof (Clone), 1);
	    if (!host)
		host = "";
	    clone->host = sstrdup (host);
	    clone->amount = 1;
	    clone->next = clonelist;
	    if (clonelist)
		clonelist->prev = clone;
	    clonelist = clone;
	}
	else
	{
	    int i;
	    clone->amount += 1;

	    for (i = 0; i < nclone; ++i)
		if (match_wild_nocase (clones[i].host, host))
		{
		    if (clone->amount > clones[i].amount)
			kill_user (s_OperServ, nick, def_clone_reason);
		    return;
		}
	    if (clone->amount > clones_allowed)
		kill_user (s_OperServ, nick, def_clone_reason);
	}
}

void
clones_del (const char *host)
{
    Clone *clone;

    if (!(clone = findclone (host)))
	return;

    if (clone->amount > 1)
	clone->amount -= 1;
    else
    {
	if (clone->host)
	    free (clone->host);
	if (clone->prev)
	    clone->prev->next = clone->next;
	else
	    clonelist = clone->next;
	if (clone->next)
	    clone->next->prev = clone->prev;
	free (clone);
    }
}

static Clone *
findclone (const char *host)
{
    Clone *clone;
    for (clone = clonelist; clone && stricmp (clone->host, host) != 0;
	 clone = clone->next);
    return clone;
}

/*************************************************************************/
/*************************************************************************/
