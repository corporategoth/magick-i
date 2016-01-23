/* OperServ functions.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

#ifdef OPERSERV

#include "os-help.c"

/* Nick for OperServ */
const char s_OperServ[] = OPERSERV_NAME;
/* Nick for sending global notices */
#ifdef GLOBALNOTICER
const char s_GlobalNoticer[] = GLOBALNOTICER_NAME;
#endif
/* Perm nick for OperServ clone */
#ifdef OUTLET
  char s_Outlet[NICKMAX];
#endif

int nsop = 0;
int sop_size = 0;
char sops[MAXSOPS][NICKMAX];

#ifdef GLOBALNOTICER
int nmessage = 0;
int message_size = 0;
Message *messages = NULL;
#endif

#ifdef AKILL
int nakill = 0;
int akill_size = 0;
Akill *akills = NULL;
#endif

#ifdef CLONES
int nclone = 0;
int clone_size = 0;
Allow *clones = NULL;

Clone *clonelist = NULL;
#endif

static void do_sop(const char *source);
static void do_sop_add(const char *source);
static void do_sop_del(const char *source);
static void do_sop_list(const char *source);
#ifdef GLOBALNOTICER
    static void do_logonmsg(const char *source);
    static void do_opermsg(const char *source);
    static void do_logonmsg_add(const char *source);
    static void do_opermsg_add(const char *source);
    static void do_message_add(const char *source, short type);
    static void do_logonmsg_del(const char *source);
    static void do_opermsg_del(const char *source);
    static void do_message_del(const char *source, short type);
    static void do_logonmsg_list(const char *source);
    static void do_opermsg_list(const char *source);
    static void do_message_list(const char *source, short type);
    static int add_message(const char *source, const char *text, short type);
    static int del_message(int num, short type);
#endif
#ifdef AKILL
    static void do_akill(const char *source);
    static void do_pakill(const char *source);
    static void do_akill_add(const char *source);
    static void do_pakill_add(const char *source);
    static void do_akill_addfunc(const char *source, int call);
    static void do_akill_del(const char *source);
    static void do_pakill_del(const char *source);
    static void do_akill_delfunc(const char *source, int call);
    static void do_akill_list(const char *source);
    static int add_akill(const char *mask, const char *reason, const char *who, int call);
    static int del_akill(const char *mask, int call);
    static int is_on_akill(const char *mask);
#endif
#ifdef CLONES
    static void do_clone(const char *source);
    static void do_clone_add(const char *source);
    static void do_clone_del(const char *source);
    static void do_clone_list(const char *source);
    static void add_clone(const char *host, int amount, const char *reason, const char *who);
    static int del_clone(const char *host);
    static Clone *findclone(const char *host);
    static int is_on_clone(char *host);
#endif
static void do_mode(const char *source);
static void do_os_kick(const char *source);
static void do_help(const char *source);
static void do_global(const char *source);
static void do_settings(const char *source);
static void do_breakdown(const char *source);
static void do_stats(const char *source);
#ifdef DAL_SERV
    static void do_qline(const char *source);
    static void do_unqline(const char *source);
    static void do_noop(const char *source);
    static void do_os_kill(const char *source);
#endif
static void do_ignore(const char *source);
static void do_update(const char *source);
static void do_os_quit(const char *source);
static void do_shutdown(const char *source);
static void do_jupe(const char *source);
static void do_off(const char *source);
static void do_on(const char *source);
static void do_raw(const char *source);

/*************************************************************************/

/* Main OperServ routine. */

void operserv(const char *source, char *buf)
{
    char *cmd;
    char *s;

    log("%s: %s: %s", s_OperServ, source, buf);
    cmd = strtok(buf, " ");

    if(mode==0 && stricmp(cmd, "ON")!=0) {
	if (offreason)
	    notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2 (%s).", offreason);
	else
	    notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2.");
	return;
    }

    if (!cmd)
	return;

    if (stricmp(cmd, "\1PING") == 0) {

	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_OperServ, source, "\1PING %s", s);

    } else if (match_wild_nocase("USER*LIST", cmd)) {
	s = strtok(NULL, " ");
	send_user_list(s_OperServ, source, s);

    } else if (match_wild_nocase("*MASK*LIST", cmd)) {
	s = strtok(NULL, " ");
	send_usermask_list(s_OperServ, source, s);

    } else if (match_wild_nocase("CHAN*LIST", cmd)) {
	s = strtok(NULL, " ");
	send_channel_list(s_OperServ, source, s);

    } else {
	Hash *command, hash_table[] = {
		{ "HELP",	H_OPER,		do_help },
		{ "*MODE",	H_OPER,		do_mode },
		{ "KICK*",	H_OPER,		do_os_kick },
		{ "STAT*",	H_OPER,		do_stats },
		{ "SET*",	H_OPER,		do_settings },
		{ "BREAKDOWN",	H_OPER,		do_breakdown },
		{ "COUNT*",	H_OPER,		do_breakdown },
		{ "IGN*",	H_SOP,		do_ignore },
		{ "JUPE*",	H_SOP,		do_jupe },
		{ "FAKE*",	H_SOP,		do_jupe },
		{ "UPDATE*",	H_SOP,		do_update },
		{ "WRITE*",	H_SOP,		do_update },
		{ "SAVE*",	H_SOP,		do_update },
#ifdef GLOBALNOTICER
		{ "GLOB*",	H_OPER,		do_global },
		{ "LOG*MESS*",	H_SOP,		do_logonmsg },
		{ "LOG*MSG*",	H_SOP,		do_logonmsg },
		{ "OP*MESS*",	H_ADMIN,	do_opermsg },
		{ "OP*MSG*",	H_ADMIN,	do_opermsg },
#endif
		{ "OFF",	H_ADMIN,	do_off },
		{ "ON",		H_ADMIN,	do_on },
		{ "RAW",	H_ADMIN,	do_raw },
		{ "QUOTE",	H_ADMIN,	do_raw },
		{ "SHUTDOWN",	H_ADMIN,	do_shutdown },
		{ "DIE",	H_ADMIN,	do_shutdown },
		{ "END",	H_ADMIN,	do_shutdown },
		{ "QUIT",	H_ADMIN,	do_os_quit },
		{ "*MURDER",	H_ADMIN,	do_os_quit },
		{ "S*OP",	H_ADMIN,	do_sop },
		{ "OPER*",	H_ADMIN,	do_sop },
#ifdef AKILL
		{ "A*KILL",	H_OPER,		do_akill },
		{ "P*A*KILL",	H_SOP,		do_pakill },
#endif
#ifdef DAL_SERV
		{ "QLINE",	H_OPER,		do_qline },
		{ "UNQLINE",	H_OPER,		do_unqline },
		{ "KILL*",	H_SOP,		do_os_kill },
		{ "N*OP*",	H_ADMIN,	do_noop },
#endif
#ifdef CLONES
		{ "CLONE*",	H_SOP,		do_clone },
#endif
		{ NULL }
	};

	if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
	else
	    notice(s_OperServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_OperServ);
    }
}

static void do_mode (const char *source)
{
	Channel *c;
	User *u;
	char l[16];
	char *chan = strtok(NULL, " ");
	char *serv = strtok(NULL, " ");
	char *s = strtok(NULL, "");
	if (!chan)
	    return;

	if (chan[0]=='#')
	    if (!(c = findchan(chan)))
		return;
	    else if (!serv) {
		snprintf(l, sizeof(l), " %d", c->limit);
		notice(s_OperServ, source, "%s +%s%s%s%s%s%s%s%s%s%s%s", c->name,
				(c->mode&CMODE_I) ? "i" : "",
				(c->mode&CMODE_M) ? "m" : "",
				(c->mode&CMODE_N) ? "n" : "",
				(c->mode&CMODE_P) ? "p" : "",
				(c->mode&CMODE_S) ? "s" : "",
				(c->mode&CMODE_T) ? "t" : "",
				(c->limit)        ? "l" : "",
				(c->key)          ? "k" : "",
				(c->limit)        ?  l  : "",
				(c->key)          ? " " : "",
				(c->key)          ? c->key : "");
	    } else
		if (s)
		    change_cmode(s_OperServ, chan, serv, s);
		else
		    change_cmode(s_OperServ, chan, serv, "");
	else
	    if (!(u = finduser(chan)))
		return;
	    else if (!serv)
		notice(s_OperServ, source, "%s +%s%s%s%s%s", u->nick,
				(u->mode&UMODE_O) ? "o" : "",
				(u->mode&UMODE_I) ? "i" : "",
				(u->mode&UMODE_S) ? "s" : "",
				(u->mode&UMODE_W) ? "w" : "",
				(u->mode&UMODE_G) ? "g" : "");
#ifdef DAL_SERV
	    else if (is_services_op(source)) {
		char *av[2];
		av[0] = sstrdup(chan);
		av[1] = sstrdup(serv);
		send_cmd(s_OperServ, "SVSMODE %s %s", chan, serv);
		do_umode(source, 2, av);
		free(av[0]);
		free(av[1]);
		
	    }
#endif
}

static void do_os_kick (const char *source)
{
	char *chan = strtok(NULL, " ");
	char *nick = strtok(NULL, " ");
	char *s = strtok(NULL, "");
	char st[strlen(s)+strlen(source)+4];
	if (!s)
	    return;
	snprintf(st, sizeof(st), "%s (%s)", source, s);
	kick_user(s_OperServ, chan, nick, st);
}

static void do_help (const char *source)
{
	char *cmd = strtok(NULL, " ");

	if (!cmd) {
	    notice_list(s_OperServ, source, os_help);
	    if (is_services_op(source))
		notice_list(s_OperServ, source, os_sop_help);
	    if (is_services_admin(source))
		notice_list(s_OperServ, source, os_admin_help);
	    notice_list(s_OperServ, source, os_end_help);
#ifdef GLOBALNOTICER
	} else if (match_wild_nocase("GLOB*", cmd)) {
	    /* Information varies, so we need to do it manually. */
	    notice(s_OperServ, source, "Syntax: GLOBAL \37message\37");
	    notice(s_OperServ, source, "");
	    notice(s_OperServ, source,
			"Allows IRCops to send messages to all users on the");
	    notice(s_OperServ, source,
			"network.  The message will be sent from the nick");
	    notice(s_OperServ, source, "\2%s\2.", s_GlobalNoticer);
#endif
	} else {

	    Hash_HELP *command, hash_table[] = {
		{ "*MODE",	H_OPER,		mode_help },
		{ "USER*LIST",	H_OPER,		userlist_help },
		{ "*MASK*LIST",	H_OPER,		masklist_help },
		{ "CHAN*LIST",	H_OPER,		chanlist_help },
		{ "KICK",	H_OPER,		kick_help },
		{ "STAT*",	H_OPER,		stats_help },
		{ "SET*",	H_OPER,		settings_help },
		{ "BREAKDOWN",	H_OPER,		breakdown_help },
		{ "COUNT*",	H_OPER,		breakdown_help },
		{ "NICKS?RV",	H_OPER,		nickserv_help },
		{ "CHANS?RV",	H_OPER,		chanserv_help },
		{ "MEMOS?RV",	H_OPER,		memoserv_help },
		{ "JUPE*",	H_SOP,		jupe_help },
		{ "FAKE*",	H_SOP,		jupe_help },
		{ "IGNORE",	H_SOP,		ignore_help },
		{ "UPDATE*",	H_SOP,		update_help },
		{ "WRITE*",	H_SOP,		update_help },
		{ "SAVE*",	H_SOP,		update_help },
#ifdef GLOBALNOTICER
		{ "LOG*MESS*",	H_SOP,		logonmsg_help },
		{ "LOG*MSG*",	H_SOP,		logonmsg_help },
		{ "OP*MESS*",	H_ADMIN,	opermsg_help },
		{ "OP*MSG*",	H_ADMIN,	opermsg_help },
#endif
		{ "ON",		H_ADMIN,	offon_help },
		{ "OFF",	H_ADMIN,	offon_help },
		{ "SHUTDOWN",	H_ADMIN,	shutdown_help },
		{ "DIE",	H_ADMIN,	shutdown_help },
		{ "END",	H_ADMIN,	shutdown_help },
		{ "QUIT",	H_ADMIN,	quit_help },
		{ "*MURDER",	H_ADMIN,	quit_help },
		{ "S*OP",	H_ADMIN,	sop_help },
		{ "OPER*",	H_ADMIN,	sop_help },
#ifdef AKILL
		{ "A*KILL",	H_OPER,		akill_help },
		{ "P*A*KILL",	H_SOP,		pakill_help },
#endif
#ifdef DAL_SERV
		{ "QLINE",	H_OPER,		qline_help },
		{ "UNQLINE",	H_OPER,		unqline_help },
		{ "KILL*",	H_SOP,		kill_help },
		{ "N*OP*",	H_ADMIN,	noop_help },
#endif
#ifdef CLONES
		{ "CLONE*",	H_SOP,		clone_help },
#endif
		{ NULL }
	    };

	    if (command = get_help_hash(source, cmd, hash_table))
		notice_list(s_OperServ, source, command->process);
	    else
		notice(s_OperServ, source,
			"No help available for command \2%s\2.", cmd);
	}
}

static void do_global (const char *source)
{
	char *msg = strtok(NULL, "");

	if (!msg) {
	    notice(s_OperServ, source, "Syntax: \2GLOBAL \37msg\37\2");
	    notice(s_OperServ, source,
			"\2/msg %s HELP GLOBAL for more information.",
			s_OperServ);
	}
	noticeall(s_GlobalNoticer, "%s", msg);
}

static void do_settings (const char *source)
{
	notice(s_OperServ, source, "Expiries (days): ");
#ifdef CHANSERV
	notice(s_OperServ, source, "    Channels : \2%d\2", CHANNEL_EXPIRE);
#endif
#ifdef NICKSERV
	notice(s_OperServ, source, "    Nicknames: \2%d\2", NICK_EXPIRE);
#endif
#ifdef NEWS
	notice(s_OperServ, source, "    News     : \2%d\2", NEWS_EXPIRE);
#endif
#ifdef AKILL
	notice(s_OperServ, source, "    AKILL's  : \2%d\2", AKILL_EXPIRE);
#endif
	notice(s_OperServ, source, "Maximum of \2%d\2 (%d currently) Services Operators.",
		MAXSOPS, nsop);
#ifdef CLONES
	notice(s_OperServ, source, "Each user may have \2%d\2 connections.",
		CLONES_ALLOWED);
#endif
#if defined(NICKSERV) && RELEASE_TIMEOUT > 0
	notice(s_OperServ, source, "Nicknames held for \2%d\2 seconds.",
		RELEASE_TIMEOUT);
#endif
#if defined(CHANSERV) && AKICK_MAX > 0
	notice(s_OperServ, source, "Maximum of \2%d\2 AKICK's per channel.",
		AKICK_MAX);
#endif
#if SERVER_RELINK > 0
	notice(s_OperServ, source, "Services relink in \2%d\2 seconds upon SQUIT.",
		SERVER_RELINK);
#endif
	notice(s_OperServ, source, "Databases saved every \2%d\2 seconds.",
		UPDATE_TIMEOUT);
	notice(s_OperServ, source, "Services Admins: \2%s\2.", SERVICES_ADMIN);
}

static void do_breakdown (const char *source)
{
    User *u;
    int i, cnt, totcnt = 0;

    notice(s_OperServ, source, "Servers                          Users  Percent");
    for (i=0;i<servcnt;++i) {
	cnt=0;
	for (u = userlist; u; u = u->next)
	    if (stricmp(servlist[i].server,u->server)==0) {
		cnt++; totcnt++; }
	notice(s_OperServ, source, "%-32s  %4d  %3.2f%%",
		servlist[i].server, cnt, (float)(cnt*100)/usercnt);
    }
    if (totcnt!=usercnt) 
	notice(s_OperServ, source,
		"WARNING: I counted %d users, but have %d on status record.",
		totcnt, usercnt);
}

static void do_stats (const char *source)
{
	time_t uptime = time(NULL) - start_time;
	char *extra = strtok(NULL, "");

	notice(s_OperServ, source, "Current users: \2%d\2 (%d ops)",
			usercnt, opcnt);
	notice(s_OperServ, source, "Maximum users: \2%d\2", maxusercnt);
	if (uptime > 86400)
	    notice(s_OperServ, source,
	    		"Services up \2%d\2 day%s, \2%02d:%02d\2",
			uptime/86400, (uptime/86400 == 1) ? "" : "s",
			(uptime/3600) % 24, (uptime/60) % 60);
	else if (uptime > 3600)
	    notice(s_OperServ, source,
	    		"Services up \2%d hour%s, %d minute%s\2",
			uptime/3600, uptime/3600==1 ? "" : "s",
			(uptime/60) % 60, (uptime/60)%60==1 ? "" : "s");
	else
	    notice(s_OperServ, source,
	    		"Services up \2%d minute%s, %d second%s\2",
			uptime/60, uptime/60==1 ? "" : "s",
			uptime%60, uptime%60==1 ? "" : "s");

	if (extra && stricmp(extra, "ALL") == 0 && is_services_op(source)) {
	    long count, mem;
	    int i;

	    get_user_stats(&count, &mem);
	    notice(s_OperServ, source,
			"User    : \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
	    get_channel_stats(&count, &mem);
	    notice(s_OperServ, source,
			"Channel : \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
#ifdef NICKSERV
	    get_nickserv_stats(&count, &mem);
	    notice(s_OperServ, source,
			"NickServ: \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
#endif
#ifdef CHANSERV
	    get_chanserv_stats(&count, &mem);
	    notice(s_OperServ, source,
			"ChanServ: \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
#endif
	    if (services_level==1) {
#ifdef MEMOS
		get_memoserv_stats(&count, &mem);
		notice(s_OperServ, source,
			"MemoServ: \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
#endif
#ifdef NEWS
		get_newsserv_stats(&count, &mem);
		notice(s_OperServ, source,
			"NewsServ: \2%6d\2 records, \2%5d\2 kB",
			count, (mem+1023) / 1024);
#endif
	    }
	    notice(s_OperServ, source,
			"OperServ: \2%6d\2 records, \2%5d\2 kB",
#ifdef AKILL
#ifdef CLONES
			nakill + nclone,
			((akill_size * sizeof(*akills) + 1023) +
			(clone_size * sizeof(*clones) + 1023)) / 1024);
#else
			nakill, (akill_size * sizeof(*akills) + 1023) / 1024);
#endif /* CLONES */
#else
			nclone, (clone_size * sizeof(*clones) + 1023) / 1024);
#endif /*AKILL */
	}
}

#ifdef DAL_SERV
static void do_qline (const char *source)
{
	char *nick = strtok(NULL, " ");
	char *s = strtok(NULL, "");
	if(!nick)
	    return;

	if(!s) {
	    send_cmd(NULL, "SQLINE %s", nick);
	    wallops(s_OperServ, "\2%s\2 has been QUARENTINED by \2%s\2", nick, source);
	} else {
	    send_cmd(NULL, "SQLINE %s :%s", nick, s);
	    wallops(s_OperServ, "\2%s\2 has been QUARENTINED by \2%s\2 for %s", nick, source, s);
	}
}

static void do_unqline (const char *source)
{
	char *nick = strtok(NULL, " ");
	if(!nick)
	    return;

	send_cmd(NULL, "UNSQLINE %s", nick);
	wallops(s_OperServ, "\2%s\2 removed QUARENTINE for \2%s\2", source, nick);
}

static void do_noop (const char *source)
{
	char *serv = strtok(NULL, " ");
	char *s = strtok(NULL, "");
	if (!s || (s[0]!='+' && s[0]!='-'))
	    return;

	send_cmd(s_OperServ, "SVSNOOP 1 :%s", serv, s);
	if(s[0]=='+')
	     wallops(s_OperServ, "\2%s\2 QUARENTINED OPERS on \2%s\2", source, serv);
	else
	     wallops(s_OperServ, "\2%s\2 removed QUARENTINE for OPERS on \2%s\2", source, serv);
}

static void do_os_kill (const char *source)
{
	char *nick = strtok(NULL, " ");
	char *s = strtok(NULL, "");
	if (!s || !finduser(nick))
	    return;

	send_cmd(s_OperServ, "SVSKILL %s :%s", nick, s);
}
#endif

static void do_ignore (const char *source)
{
	char *option = strtok(NULL, " ");
	char *setting = strtok(NULL, "");

	if (!option)
	    notice(s_OperServ, source,
			"Syntax: \2IGNORE \37{ON|OFF|LIST}\37 [\37match\37]\2");
	else if (stricmp(option, "ON") == 0) {
	    allow_ignore = 1;
	    notice(s_OperServ, source, "Ignore code \2will\2 be used.");
	} else if (stricmp(option, "OFF") == 0) {
	    allow_ignore = 0;
	    notice(s_OperServ, source, "Ignore code \2will not\2 be used.");
	} else if (stricmp(option, "LIST") == 0) {
	    int i;
	    int sent_header = 0;
	    IgnoreData *id;
	    for (i = 33; i < 256; ++i) 
		for (id = ignore[i]; id; id = id->next) {
		    if (!sent_header) {
			notice(s_OperServ, source, "Services ignorance list:");
			sent_header = 1;
		    }
		    if (!setting || match_wild_nocase(setting, id->who))
			notice(s_OperServ, source, "%d %s", id->time, id->who);
		}
	    if (!sent_header)
		notice(s_OperServ, source, "Ignorance list is empty.");
	} else
	    notice(s_OperServ, source, "Unknown option \2%s\2.", option);
}

static void do_jupe (const char *source)
{
	char *serv = strtok(NULL, " ");
	if (!serv)
	    notice(s_OperServ, source, "Syntax: \2JUPE \37servername\37\2");
	else {
	    wallops(s_OperServ, "\2Juping\2 %s by request of \2%s\2.", serv, source);
	    send_cmd(NULL, "SERVER %s 2 :Jupitered server", serv);
	}
}

static void do_off (const char *source)
{
	char *s = strtok(NULL, "");
	mode = 0;
	close_log();
	notice(s_OperServ, source, "Services are now switched \2OFF\2.");
	wallops(s_OperServ, "Services switched \2OFF\2 by request of \2%s\2.", source);
	if(s)
	    if (offreason = malloc(strlen(s)+1))
		snprintf(offreason, sizeof(offreason), "%s", s);
	    else
		offreason = s;
#ifdef GLOBALNOTICER
	if (offreason)
	    noticeall(s_GlobalNoticer, "Services are currently \2OFF\2 - %s", offreason);
	else
	    noticeall(s_GlobalNoticer, "Services are currently \2OFF\2 - Please do not attempt to use them!");
#endif
}


static void do_on (const char *source)
{
	mode = 1;
	open_log();
	notice(s_OperServ, source, "Services are now switched \2ON\2 again.");
	if (offreason)
	    offreason = NULL;

#ifdef GLOBALNOTICER
	    noticeall(s_GlobalNoticer, "Services are back \2ON\2 again - Please use them at will again.");
#endif
}

static void do_update (const char *source)
{
	notice(s_OperServ, source, "Updating databases.");
	save_data = 1;
}

static void do_os_quit (const char *source)
{
	quitmsg = malloc(32 + strlen(source));
	if (!quitmsg)
	    quitmsg = "QUIT command received, but out of memory!";
	else
	    snprintf(quitmsg, sizeof(quitmsg), "QUIT command received from %s", source);
	quitting = 1;
	terminating = 1;
}

static void do_shutdown (const char *source)
{
	do_update(source);
	do_os_quit(source);
}

static void do_raw (const char *source)
{
	char *text = strtok(NULL, "");
	if (!text)
	    notice(s_OperServ, source, "Syntax: \2RAW \37password command\37\2");
	else
	    send_cmd(NULL, text);
}


/*************************************************************************/
/************************** Services Operators ***************************/
/*************************************************************************/

void load_sop()
{
    FILE *f = fopen(SOP_DB, "r");
    int i;

    if (!f) {
	log_perror("Can't read SOP database " SOP_DB);
	return;
    }
    switch (i = get_file_version(f, SOP_DB)) {
      case 4:
      case 3:
      case 2:
      case 1:
	nsop = fgetc(f) * 256 + fgetc(f);
	if (nsop < 8)
	    sop_size = 16;
	else
	    sop_size = 2*nsop;
	if (!nsop) {
	    fclose(f);
	    return;
	}
	if (nsop != fread(sops, NICKMAX, nsop, f))
	    fatal_perror("Read error on " SOP_DB);
	break;
      default:
	fatal("Unsupported version (%d) on %s", i, SOP_DB);
    } /* switch (version) */
    fclose(f);
}

/*************************************************************************/

void save_sop()
{
    FILE *f;
    int i;

    remove(SOP_DB ".save");
    if (rename(SOP_DB, SOP_DB ".save") < 0)
	log_perror("Can't back up " SOP_DB);
    f = fopen(SOP_DB, "w");
    if (!f) {
	log_perror("Can't write to SOP database " SOP_DB);
	if (rename(SOP_DB ".save", SOP_DB) < 0)
	    fatal_perror("Can't restore backup copy of " SOP_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't change group of %s to %d", SOP_DB, file_gid);
    write_file_version(f, SOP_DB);

    fputc(nsop/256, f); fputc(nsop & 255, f);
    if (fwrite(sops, NICKMAX, nsop, f) != nsop)
	fatal_perror("Write error on " SOP_DB);
    fclose(f);
    remove(SOP_DB ".save");
}

/*************************************************************************/

/* Handle an SOP command. */

static void do_sop(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_ADMIN,	do_sop_add },
		{ "ALLOW",	H_ADMIN,	do_sop_add },
		{ "GRANT",	H_ADMIN,	do_sop_add },
		{ "DEL*",	H_ADMIN,	do_sop_del },
		{ "ERASE",	H_ADMIN,	do_sop_del },
		{ "TRASH",	H_ADMIN,	do_sop_del },
		{ "LIST*",	H_ADMIN,	do_sop_list },
		{ "VIEW",	H_ADMIN,	do_sop_list },
		{ "DISP*",	H_ADMIN,	do_sop_list },
		{ "SHOW*",	H_ADMIN,	do_sop_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2SOP {ADD|DEL|LIST} [\37host\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP SOP\2", s_OperServ);
    }
}

void do_sop_add(const char *source)
{
	char *nick;
	if (nick = strtok(NULL, " ")) {
	    if(is_justservices_op(nick))
		notice(s_OperServ, source, "SOP already exists");
	    else if (nsop+1==MAXSOPS)
		notice(s_OperServ, source,
			"Maximum Services Operators limit reached.");
	    else {
#ifdef NICKSERV
		NickInfo *ni = findnick(nick);
		if (ni && (ni->flags & NI_IRCOP)) {
		    strscpy(nick, ni->nick, NICKMAX);
#endif
		    if (nsop >= sop_size) {
			if (sop_size < 8)
			    sop_size = 8;
			else
			    sop_size *= 2;
		    }
		    strscpy(sops[nsop], nick, NICKMAX);
		    notice(s_OperServ, source, "%s added to SOP list", nick);
		    ++nsop;
#ifdef NICKMAX
		} else
		    if (ni)
			notice(s_OperServ, source, "%s is not an IRC Operator.", nick);
		    else
			notice(s_OperServ, source, "%s is not registered.", nick);
#endif
	    }
	} else
	    notice(s_OperServ, source,
			"Syntax: SOP ADD \37nick\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_sop_del(const char *source)
{
    char *nick;
    int i;
	if (nick = strtok(NULL, " ")) {
	    if (strspn(nick, "1234567890") == strlen(nick) &&
					(i = atoi(nick)) > 0 && i <= nsop)
		strscpy(nick, sops[i-1], NICKMAX);

	    for (i = 0; i < nsop; ++i)
		if (stricmp(sops[i], nick)==0) {
		    strscpy(nick, sops[i], NICKMAX);
		    --nsop;
		    if (i < nsop)
			bcopy(sops+i+1, sops+i, NICKMAX * (nsop-i));
		    i--;
		    break;
		}
	    if (i<nsop)
		notice(s_OperServ, source, "%s removed from SOP list.", nick);
	    else
		notice(s_OperServ, source, "%s not found on SOP list.", nick);
	} else
	    notice(s_OperServ, source, "Syntax: SOP DEL \37nick\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_sop_list(const char *source)
{
    char *s;
    int i;
	s = strtok(NULL, " ");
	if (s)
	    strlower(s);
	notice(s_OperServ, source, "Current SOP list:");
	for (i = 0; i < nsop; ++i)
	    if (!s || match_wild_nocase(s, sops[i]))
		notice(s_OperServ, source, "  %3d %s",
				i+1, sops[i]);
}

int is_services_op(const char *nick)
{
    int i;
    NickInfo *ni;

    for (i = 0; i < nsop; ++i)
	if (stricmp(sops[i], nick)==0)
#ifdef NICKSERV
	    if ((ni = findnick(nick)) && (ni->flags & NI_IDENTIFIED)
							&& is_oper(nick))
#endif
		return 1;
    return 0;
}

int is_justservices_op(const char *nick)
{
    int i;

    for (i = 0; i < nsop; ++i)
	if (stricmp(sops[i], nick)==0)
	    return 1;
    return 0;
}


/*************************************************************************/
/************************** Logon/Oper MESSAGES **************************/
/*************************************************************************/

#ifdef GLOBALNOTICER
void load_message()
{
    FILE *f = fopen(MESSAGE_DB, "r");
    int i;

    if (!f) {
	log_perror("Can't read MESSAGE database " MESSAGE_DB);
	return;
    }
    switch (i = get_file_version(f, MESSAGE_DB)) {
      case 4:
      case 3:
      case 2:
      case 1:
	nmessage = fgetc(f) * 256 + fgetc(f);
	if (nmessage < 8)
	    message_size = 16;
	else
	    message_size = 2*nmessage;
	messages = smalloc(sizeof(*messages) * message_size);
	if (!nmessage) {
	    fclose(f);
	    return;
	}
	if (nmessage != fread(messages, sizeof(*messages), nmessage, f))
	    fatal_perror("Read error on " MESSAGE_DB);
	for (i = 0; i < nmessage; ++i) {
	    messages[i].text  = read_string(f, MESSAGE_DB);
	}
	break;

      default:
	fatal("Unsupported version (%d) on %s", i, MESSAGE_DB);
    } /* switch (version) */
    fclose(f);
}

/*************************************************************************/

void save_message()
{
    FILE *f;
    int i;

    remove(MESSAGE_DB ".save");
    if (rename(MESSAGE_DB, MESSAGE_DB ".save") < 0)
	log_perror("Can't back up " MESSAGE_DB);
    f = fopen(MESSAGE_DB, "w");
    if (!f) {
	log_perror("Can't write to MESSAGE database " MESSAGE_DB);
	if (rename(MESSAGE_DB ".save", MESSAGE_DB) < 0)
	    fatal_perror("Can't restore backup copy of " MESSAGE_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't change group of %s to %d", MESSAGE_DB, file_gid);
    write_file_version(f, MESSAGE_DB);

    fputc(nmessage/256, f); fputc(nmessage & 255, f);
    if (fwrite(messages, sizeof(*messages), nmessage, f) != nmessage)
	fatal_perror("Write error on " MESSAGE_DB);
    for (i = 0; i < nmessage; ++i) {
	write_string(messages[i].text, f, MESSAGE_DB);
    }
    fclose(f);
    remove(MESSAGE_DB ".save");
}

/*************************************************************************/

static void do_logonmsg(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_SOP,	do_logonmsg_add },
		{ "WRITE",	H_SOP,	do_logonmsg_add },
		{ "COMPOSE",	H_SOP,	do_logonmsg_add },
		{ "DEL*",	H_SOP,	do_logonmsg_del },
		{ "ERASE",	H_SOP,	do_logonmsg_del },
		{ "TRASH",	H_SOP,	do_logonmsg_del },
		{ "LIST*",	H_SOP,	do_logonmsg_list },
		{ "VIEW",	H_SOP,	do_logonmsg_list },
		{ "DISP*",	H_SOP,	do_logonmsg_list },
		{ "SHOW*",	H_SOP,	do_logonmsg_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2LOGONMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP LOGONMSG\2", s_OperServ);
    }
}

static void do_opermsg(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_ADMIN,	do_opermsg_add },
		{ "WRITE",	H_ADMIN,	do_opermsg_add },
		{ "COMPOSE",	H_ADMIN,	do_opermsg_add },
		{ "DEL*",	H_ADMIN,	do_opermsg_del },
		{ "ERASE",	H_ADMIN,	do_opermsg_del },
		{ "TRASH",	H_ADMIN,	do_opermsg_del },
		{ "LIST*",	H_ADMIN,	do_opermsg_list },
		{ "VIEW",	H_ADMIN,	do_opermsg_list },
		{ "DISP*",	H_ADMIN,	do_opermsg_list },
		{ "SHOW*",	H_ADMIN,	do_opermsg_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2OPERMSG {ADD|DEL|LIST} [\37text|num\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP OPERMSG\2", s_OperServ);
    }
}

void do_logonmsg_add(const char *source)
{ do_message_add(source, M_LOGON); }
void do_opermsg_add(const char *source)
{ do_message_add(source, M_OPER); }

void do_message_add(const char *source, short type)
{
    char *text;
	if ((text = strtok(NULL, "")))
	    notice(s_OperServ, source, "Added %s Message (#%d).",
				type == M_LOGON ? "Logon" :
				type == M_OPER  ? "Oper" :
				"Unknown", add_message(source, text, type));
	else
	    notice(s_OperServ, source, "Syntax: %sMSG ADD \37text\37",
				type == M_LOGON ? "LOGON" :
				type == M_OPER  ? "OPER" :
				"");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_logonmsg_del(const char *source)
{ do_message_del(source, M_LOGON); }
void do_opermsg_del(const char *source)
{ do_message_del(source, M_OPER); }

void do_message_del(const char *source, short type)
{
    char *text;
    int num = 1;
	if (text = strtok(NULL, " ")) {
	    if (stricmp(text, "ALL")!=0) {
		num=atoi(text);
		if (num>0 && del_message(num, type))
		    notice(s_OperServ, source, "%s Message (#%d) Removed.",
				type == M_LOGON ? "Logon" :
				type == M_OPER  ? "Oper" :
				"Unknown", num);
		else
		    notice(s_OperServ, source, "No such %s Mesage (#%d).",
				type == M_LOGON ? "Logon" :
				type == M_OPER  ? "Oper" :
				"Unknown", num);
	    } else
		if (is_services_admin(source)) {
		    while (del_message(num, type)) ;
		    notice(s_OperServ, source, "All %s Messages Removed.",
				type == M_LOGON ? "Logon" :
				type == M_OPER  ? "Oper" :
				"Unknown");
		} else
		    notice(s_OperServ, source, "Access Denied.");
	} else
	    notice(s_OperServ, source, "Syntax: %sMSG DEL \37num%s\37",
				type == M_LOGON ? "LOGON" :
				type == M_OPER  ? "OPER" :
				"", is_services_admin(source) ? "|ALL" : "");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_logonmsg_list(const char *source)
{ do_message_list(source, M_LOGON); }
void do_opermsg_list(const char *source)
{ do_message_list(source, M_OPER); }

void do_message_list(const char *source, short type)
{
	int i, num;
	notice(s_OperServ, source, "Current %s Message list:",
				type == M_LOGON ? "Logon" :
				type == M_OPER  ? "Oper" :
				"Unknown");
	for (i = 0, num = 1; i < nmessage; ++i)
	    if (messages[i].type == type) {
	    	char timebuf[32];
	    	time_t t;
	    	struct tm tm;

	    	time(&t);
	    	tm = *localtime(&t);
	    	strftime(timebuf, sizeof(timebuf), "%d %b %Y %H:%M:%S %Z", &tm);
		notice(s_OperServ, source, "  %3d by %s on %s",
				num,
				*messages[i].who ? messages[i].who : "<unknown>",
				timebuf);
		notice(s_OperServ, source, "      %s", messages[i].text);
		num++;
	    }
}

/*************************************************************************/

static int add_message(const char *source, const char *text, short type)
{
    int i, num;
    if (nmessage >= message_size) {
	if (message_size < 8)
	    message_size = 8;
	else
	    message_size *= 2;
	messages = srealloc(messages, sizeof(*messages) * message_size);
    }
    messages[nmessage].text = sstrdup(text);
    messages[nmessage].type = type;
    messages[nmessage].time = time(NULL);
    strscpy(messages[nmessage].who, source, NICKMAX);
    ++nmessage;
    for (i=0, num=0; i < nmessage; ++i)
	if (messages[i].type == type)
	    num++;
    return num;
}

/*************************************************************************/

/* Return whether the text was found in the MESSAGE list. */

static int del_message(int num, short type)
{
    int i, find = 0;

    for (i = 0, find = 0; i < nmessage; ++i)
	if (messages[i].type == type) {
	    find++;
	    if (num == find) {
		free(messages[i].text);
		--nmessage;
		if (i < nmessage)
		    bcopy(messages+i+1, messages+i, sizeof(*messages) * (nmessage-i));
		return 1;
	    }
	}
    return 0;
}
#endif

/*************************************************************************/
/****************************** AKILL stuff ******************************/
/*************************************************************************/

#ifdef AKILL
void load_akill()
{
    FILE *f = fopen(AKILL_DB, "r");
    int i;

    if (!f) {
	log_perror("Can't read AKILL database " AKILL_DB);
	return;
    }
    switch (i = get_file_version(f, AKILL_DB)) {
      case 4:
      case 3:
      case 2:
	nakill = fgetc(f) * 256 + fgetc(f);
	if (nakill < 8)
	    akill_size = 16;
	else
	    akill_size = 2*nakill;
	akills = smalloc(sizeof(*akills) * akill_size);
	if (!nakill) {
	    fclose(f);
	    return;
	}
	if (nakill != fread(akills, sizeof(*akills), nakill, f))
	    fatal_perror("Read error on " AKILL_DB);
	for (i = 0; i < nakill; ++i) {
	    akills[i].mask = read_string(f, AKILL_DB);
	    akills[i].reason = read_string(f, AKILL_DB);
	}
	break;

      case 1: {
	struct {
	    char *mask;
	    char *reason;
	    time_t time;
	} old_akill;
	nakill = fgetc(f) * 256 + fgetc(f);
	if (nakill < 8)
	    akill_size = 16;
	else
	    akill_size = 2*nakill;
	akills = smalloc(sizeof(*akills) * akill_size);
	if (!nakill) {
	    fclose(f);
	    return;
	}
	for (i = 0; i < nakill; ++i) {
	    if (1 !=fread(&old_akill, sizeof(old_akill), 1, f))
		fatal_perror("Read error on " AKILL_DB);
	    akills[i].time = old_akill.time;
	    akills[i].who[0] = 0;
	}
	for (i = 0; i < nakill; ++i) {
	    akills[i].mask = read_string(f, AKILL_DB);
	    akills[i].reason = read_string(f, AKILL_DB);
	}
	break;
      } /* case 1 */

      default:
	fatal("Unsupported version (%d) on %s", i, AKILL_DB);
    } /* switch (version) */
    fclose(f);
}

/*************************************************************************/

void save_akill()
{
    FILE *f;
    int i;

    remove(AKILL_DB ".save");
    if (rename(AKILL_DB, AKILL_DB ".save") < 0)
	log_perror("Can't back up " AKILL_DB);
    f = fopen(AKILL_DB, "w");
    if (!f) {
	log_perror("Can't write to AKILL database " AKILL_DB);
	if (rename(AKILL_DB ".save", AKILL_DB) < 0)
	    fatal_perror("Can't restore backup copy of " AKILL_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't change group of %s to %d", AKILL_DB, file_gid);
    write_file_version(f, AKILL_DB);

    fputc(nakill/256, f); fputc(nakill & 255, f);
    if (fwrite(akills, sizeof(*akills), nakill, f) != nakill)
	fatal_perror("Write error on " AKILL_DB);
    for (i = 0; i < nakill; ++i) {
	write_string(akills[i].mask, f, AKILL_DB);
	write_string(akills[i].reason, f, AKILL_DB);
    }
    fclose(f);
    remove(AKILL_DB ".save");
}

/*************************************************************************/

/* Handle an AKILL command. */

static void do_akill(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_OPER,	do_akill_add },
		{ "DISALLOW",	H_OPER,	do_akill_add },
		{ "DENY*",	H_OPER,	do_akill_add },
		{ "DEL*",	H_OPER,	do_akill_del },
		{ "ERASE",	H_OPER,	do_akill_del },
		{ "TRASH",	H_OPER,	do_akill_del },
		{ "LIST*",	H_OPER,	do_akill_list },
		{ "VIEW",	H_OPER,	do_akill_list },
		{ "DISP*",	H_OPER,	do_akill_list },
		{ "SHOW*",	H_OPER,	do_akill_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2AKILL {ADD|DEL|LIST} [\37host\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP AKILL\2", s_OperServ);
    }
}

static void do_pakill(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_SOP,	do_pakill_add },
		{ "DISALLOW",	H_SOP,	do_pakill_add },
		{ "DENY*",	H_SOP,	do_pakill_add },
		{ "DEL*",	H_SOP,	do_pakill_del },
		{ "ERASE",	H_SOP,	do_pakill_del },
		{ "TRASH",	H_SOP,	do_pakill_del },
		{ "LIST*",	H_OPER,	do_akill_list },
		{ "VIEW",	H_OPER,	do_akill_list },
		{ "DISP*",	H_OPER,	do_akill_list },
		{ "SHOW*",	H_OPER,	do_akill_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2PAKILL {ADD|DEL|LIST} [\37host\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP PAKILL\2", s_OperServ);
    }
}

void do_akill_add(const char *source)
{ do_akill_addfunc(source, 0); }
void do_pakill_add(const char *source)
{ do_akill_addfunc(source, 1); }

void do_akill_addfunc(const char *source, int call)
{
    char *mask, *reason, *s;
    int i;
	if ((mask = strtok(NULL, " ")) && (reason = strtok(NULL, ""))) {
	    int nonchr = 0;
	    for (i=0;mask[i]!='!' && i<strlen(mask);i++) ;
	    if (i<strlen(mask)) {
		notice(s_OperServ, source, "Hostmask may not contain a `!' character.");
		return;
	    }
	    strlower(mask);
	    if (!(s = strchr(mask, '@'))) {
		notice(s_OperServ, source, "Hostmask must contain an `@' character.");
		return;
	    }
		/* Find @*, @*.*, @*.*.*, etc. and dissalow if !SOP */
	    for(i=strlen(mask)-1;mask[i]!='@';i--)
		if(!(mask[i]=='*' || mask[i]=='?' || mask[i]=='.'))
		    nonchr++;
	    if(nonchr<STARTHRESH && !is_services_op(source)) {
		notice(s_OperServ, source, "Must have at least %d non- *, ? or . characters in host.", STARTHRESH);
		return;
	    } else if (nonchr<STARTHRESH) {
		for(i--;i>0 && (mask[i]=='*' || mask[i]=='?');i--) ;
		if (!i) {
		    notice(s_OperServ, source, "Must have at least 1 non- *, ? or . characters.", STARTHRESH);
		    return;
		}
	    }
	    if(is_on_akill(mask))
		notice(s_OperServ, source, "AKILL already exists (or inclusive)");
	    else
		notice(s_OperServ, source, "%s added to AKILL list (%d users killed)", mask, add_akill(mask, reason, source, call));
	} else
	    notice(s_OperServ, source,
			"Syntax: AKILL ADD \37mask\37 \37reason\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_akill_del(const char *source)
{ do_akill_delfunc(source, 0); }
void do_pakill_del(const char *source)
{ do_akill_delfunc(source, 1); }

void do_akill_delfunc(const char *source, int call)
{
    char *mask;
    int i;
	if (mask = strtok(NULL, " ")) {
	    if (strspn(mask, "1234567890") == strlen(mask) &&
					(i = atoi(mask)) > 0 && i <= nakill)
		strcpy(mask, akills[i-1].mask);
	    if (i = del_akill(mask, call))
		notice(s_OperServ, source, "%d AKILL%s matching %s removed from AKILL list.",
			i, i==1 ? "" : "s", mask);
	    else
		if(call==1)
		    notice(s_OperServ, source, "%s not found on AKILL list.", mask);
		else
		    notice(s_OperServ, source, "%s not found on AKILL list or is PERMINANT.", mask);
	} else
	    notice(s_OperServ, source, "Syntax: AKILL DEL \37mask\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

void do_akill_list(const char *source)
{
    char *s;
    int i;
	s = strtok(NULL, " ");
	if (s) {
	    strlower(s);
	    strchr(s, '@');
	}
	notice(s_OperServ, source, "Current AKILL list:");
	for (i = 0; i < nakill; ++i)
	    if (!s || match_wild_nocase(s, akills[i].mask)) {
	    	char timebuf[32];
	    	time_t t;
	    	struct tm tm;

	    	time(&t);
	    	tm = *localtime(&t);
	    	strftime(timebuf, sizeof(timebuf), "%d %b %Y %H:%M:%S %Z", &tm);
		notice(s_OperServ, source, "  %3d %s (by %s %s %s)",
				i+1, akills[i].mask,
				*akills[i].who ? akills[i].who : "<unknown>",
				akills[i].time ? "on" : "is",
				akills[i].time ? timebuf : "PERMINANT");
		notice(s_OperServ, source, "      %s", akills[i].reason);
	    }
}

static int is_on_akill(const char *mask)
{
    int i;

    for (i = 0; i < nakill; ++i)
	if (match_wild_nocase(akills[i].mask, mask))
	    return 1;
    return 0;
}

/*************************************************************************/

void expire_akill () {
    int i;
    const time_t expire_time = AKILL_EXPIRE*24*60*60;
    time_t tm;
    for (i = 0; i < nakill; ++i)
	if (akills[i].time > 0) {
	    tm = time(NULL) - akills[i].time;
	    if (tm > expire_time) {
#ifdef IRC_DALNET
		char *s = strchr(akills[i].mask, '@');
		if (s) {
		    *s++ = 0;
		    send_cmd(server_name, "RAKILL %s %s", s, akills[i].mask);
		}
#else
		send_cmd(server_name, "UNGLINE * %s", akills[i].mask);
#endif
		wallops(s_OperServ, "Expiring AKILL on %s (%s), by %s.",
			akills[i].mask, akills[i].reason, akills[i].who);
		free(akills[i].mask);
		free(akills[i].reason);
		--nakill;
		if (i < nakill)
		    bcopy(akills+i+1, akills+i, sizeof(*akills) * (nakill-i));
	    }
	}
}

/*************************************************************************/

/* Does the user match any AKILLs? */

int check_akill(const char *nick, const char *username, const char *host)
{
    char buf[512];
    int i;

    strscpy(buf, username, sizeof(buf)-2);
    i = strlen(buf);
    buf[i++] = '@';
    strlower(strscpy(buf+i, host, sizeof(buf)-i));
    for (i = 0; i < nakill; ++i)
	if (match_wild_nocase(akills[i].mask, buf)) {
	    char *av[2], nickbuf[NICKMAX], myuser[512] = "", myhost[512] = "";
	    int j, k;
	    kill_user(s_OperServ, nick, akills[i].reason);

	    /* so we dont get 1,000,001 akill's */
	    for (j=0, k=0;akills[i].mask[j]!='@';j++, k++)
		myuser[k]=akills[i].mask[j];
	    myuser[k]=0; j++;
	    for (k=0;j<strlen(akills[i].mask);j++, k++)
		myhost[k]=akills[i].mask[j];
	    myhost[k]=0;
#ifdef IRC_DALNET
	    send_cmd(server_name,
			"AKILL %s %s :You are Banned (%s)",
			myhost, myuser, akills[i].reason);
#else
	    send_cmd(server_name,
			"GLINE * +99999999 %s@%s :You are Banned (%s)",
			myuser, myhost, akills[i].reason);
#endif
	    return 1;
	}
    return 0;
}

/*************************************************************************/

static int add_akill(const char *mask, const char *reason, const char *who, int call)
{
    User *user;
    int cnt, killd;
    char buf[strlen(reason)+20];
    snprintf(buf, sizeof(buf), "You are banned (%s)", reason);

    if (nakill >= akill_size) {
	if (akill_size < 8)
	    akill_size = 8;
	else
	    akill_size *= 2;
	akills = srealloc(akills, sizeof(*akills) * akill_size);
    }
    akills[nakill].mask = sstrdup(mask);
    akills[nakill].reason = sstrdup(reason);
    if (call==1)
	akills[nakill].time = 0;
    else
	akills[nakill].time = time(NULL);
    strscpy(akills[nakill].who, who, NICKMAX);
    ++nakill;

    cnt = killd = countusermask(mask);
    for (; cnt; cnt--) {
	user = findusermask(mask, cnt);
	kill_user(s_OperServ, user->nick, buf);
    }
    return killd;
}

/*************************************************************************/

/* Return whether the mask was found in the AKILL list. */

static int del_akill(const char *mask, int call)
{
    int i, ret = 0;

    for (i = 0; i < nakill; ++i)
	if (match_wild_nocase(akills[i].mask, mask)) {
	    if (!(akills[i].time==0 && call!=1)) {
#ifdef IRC_DALNET
		char mymask[strlen(akills[i].mask)], *s;
		strscpy(mymask, akills[i].mask, strlen(akills[i].mask));
		s = strchr(mymask, '@');
		if (s) {
		    *s++ = 0;
		    send_cmd(server_name, "RAKILL %s %s", s, akills[i].mask);
		}
#else
		send_cmd(server_name, "UNGLINE * %s", akills[i].mask);
#endif
		free(akills[i].mask);
		free(akills[i].reason);
		--nakill;
		if (i < nakill)
		    bcopy(akills+i+1, akills+i, sizeof(*akills) * (nakill-i));
		ret++;
	    }
	}
    return ret;
}
#endif /* AKILL */

/*************************************************************************/
/**************************** Clone detection ****************************/
/*************************************************************************/

#ifdef CLONES
void load_clone()
{
    FILE *f = fopen(CLONE_DB, "r");
    int i;

    if (!f) {
	log_perror("Can't read CLONE database " CLONE_DB);
	return;
    }
    switch (i = get_file_version(f, CLONE_DB)) {
      case 4:
      case 3:
      case 2:
      case 1:
	nclone = fgetc(f) * 256 + fgetc(f);
	if (nclone < 8)
	    clone_size = 16;
	else
	    clone_size = 2*nclone;
	clones = smalloc(sizeof(*clones) * clone_size);
	if (!nclone) {
	    fclose(f);
	    return;
	}
	if (nclone != fread(clones, sizeof(*clones), nclone, f))
	    fatal_perror("Read error on " CLONE_DB);
	for (i = 0; i < nclone; ++i) {
	    clones[i].host = read_string(f, CLONE_DB);
	    clones[i].reason = read_string(f, CLONE_DB);
	}
	break;
      default:
	fatal("Unsupported version (%d) on %s", i, CLONE_DB);
    } /* switch (version) */
    fclose(f);
}

/*************************************************************************/

void save_clone()
{
    FILE *f;
    int i;

    remove(CLONE_DB ".save");
    if (rename(CLONE_DB, CLONE_DB ".save") < 0)
	log_perror("Can't back up " CLONE_DB);
    f = fopen(CLONE_DB, "w");
    if (!f) {
	log_perror("Can't write to CLONE database " CLONE_DB);
	if (rename(CLONE_DB ".save", CLONE_DB) < 0)
	    fatal_perror("Can't restore backup copy of " CLONE_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't change group of %s to %d", CLONE_DB, file_gid);
    write_file_version(f, CLONE_DB);

    fputc(nclone/256, f); fputc(nclone & 255, f);
    if (fwrite(clones, sizeof(*clones), nclone, f) != nclone)
	fatal_perror("Write error on " CLONE_DB);
    for (i = 0; i < nclone; ++i) {
	write_string(clones[i].host, f, CLONE_DB);
	write_string(clones[i].reason, f, CLONE_DB);
    }
    fclose(f);
    remove(CLONE_DB ".save");
}

/*************************************************************************/

/* Handle an CLONE command. */

static void do_clone(const char *source)
{
    char *cmd = strtok(NULL, " ");
    Hash *command, hash_table[] = {
		{ "ADD",	H_SOP,	do_clone_add },
		{ "ALLOW",	H_SOP,	do_clone_add },
		{ "GRANT",	H_SOP,	do_clone_add },
		{ "DEL*",	H_SOP,	do_clone_del },
		{ "ERASE",	H_SOP,	do_clone_del },
		{ "TRASH",	H_SOP,	do_clone_del },
		{ "LIST*",	H_SOP,	do_clone_list },
		{ "VIEW",	H_SOP,	do_clone_list },
		{ "DISP*",	H_SOP,	do_clone_list },
		{ "SHOW*",	H_SOP,	do_clone_list },
		{ NULL }
    };

    if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
    else {
	notice(s_OperServ, source,
		"Syntax: \2CLONE {ADD|DEL|LIST} [\37host\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP CLONE\2", s_OperServ);
    }
}

static void do_clone_add(const char *source)
{
    char *host, *amt, *reason;
	host   = strtok(NULL, " ");
	amt    = strtok(NULL, " ");
	reason = strtok(NULL, "");
	if (reason) {
	    int i, amount, nonchr = 0;
	    for (i=0;host[i]!='!' && host[i]!='@' && i<strlen(host);i++) ;
	    if (i<strlen(host)) {
		notice(s_OperServ, source, "Host may not contain a `!' or `@' character.");
		return;
	    }
	    strlower(host);
	    amount = atoi(amt);

		/* Find @*, @*.*, @*.*.*, etc. and dissalow */
	    for(i=strlen(host)-1;i>=0;i--)
		if(!(host[i]=='*' || host[i]=='?' || host[i]=='.'))
		    nonchr++;
	    if(nonchr<STARTHRESH)
		notice(s_OperServ, source, "Must have at least %d non- *, ? or . characters.", STARTHRESH);
	    else if(is_on_clone(host))
		notice(s_OperServ, source, "CLONE already exists (or inclusive)");
	    else if (amount<1)
		notice(s_OperServ, source, "CLONE AMOUNT must be greater than 0");
	    else if (amount>999)
		notice(s_OperServ, source, "CLONE AMOUNT must be less than 1000");
	    else {
		add_clone(host, amount, reason, source);
		notice(s_OperServ, source, "%s added to CLONE list.", host);
	    }
	} else
	    notice(s_OperServ, source,
			"Syntax: CLONE ADD \37host\37 \37amount\37 \37reason\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

static void do_clone_del(const char *source)
{
    char *host;
    int i;

	host   = strtok(NULL, " ");
	if (host) {
	    if (strspn(host, "1234567890") == strlen(host) &&
					(i = atoi(host)) > 0 && i <= nclone)
		strcpy(host, clones[i-1].host);
	    strlower(host);
	    if (del_clone(host))
		notice(s_OperServ, source, "%s removed from CLONE list.", host);
	    else
		notice(s_OperServ, source, "%s not found on CLONE list.", host);
	} else
	    notice(s_OperServ, source, "Syntax: CLONE DEL \37host\37");
	if(services_level!=1)
	    notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
}

static void do_clone_list(const char *source)
{
    char *host;
    int i;

	host   = strtok(NULL, " ");
	if (host)
	    strlower(host);
	notice(s_OperServ, source, "Current CLONE list:");
	for (i = 0; i < nclone; ++i)
	    if (!host || match_wild_nocase(host, clones[i].host)) {
	    	char timebuf[32];
	    	time_t t;
	    	struct tm tm;

	    	time(&t);
	    	tm = *localtime(&t);
	    	strftime(timebuf, sizeof(timebuf), "%d %b %Y %H:%M:%S %Z", &tm);
		notice(s_OperServ, source, "  %3d %s (by %s on %s)",
				i+1, clones[i].host,
				*clones[i].who ? clones[i].who : "<unknown>",
				timebuf);
		notice(s_OperServ, source, "      [%3d] %s",
				clones[i].amount, clones[i].reason);
	    }
}

static int is_on_clone(char *host)
{
    int i;

    strlower(host);
    for (i = 0; i < nclone; ++i)
	if (match_wild_nocase(clones[i].host, host))
	    return 1;
    return 0;
}

/*************************************************************************/

static void add_clone(const char *host, int amount, const char *reason, const char *who)
{
    if (nclone >= clone_size) {
	if (clone_size < 8)
	    clone_size = 8;
	else
	    clone_size *= 2;
	clones = srealloc(clones, sizeof(*clones) * clone_size);
    }
    clones[nclone].host = sstrdup(host);
    clones[nclone].amount = amount;
    clones[nclone].reason = sstrdup(reason);
    clones[nclone].time = time(NULL);
    strscpy(clones[nclone].who, who, NICKMAX);
    ++nclone;
}

/*************************************************************************/

/* Return whether the host was found in the CLONE list. */

static int del_clone(const char *host)
{
    int i;

    if (strspn(host, "1234567890") == strlen(host) &&
				(i = atoi(host)) > 0 && i <= nclone)
	--i;
    else
	for (i = 0; i < nclone && stricmp(clones[i].host, host) != 0; ++i) ;
    if (i < nclone) {
	free(clones[i].host);
	free(clones[i].reason);
	--nclone;
	if (i < nclone)
	    bcopy(clones+i+1, clones+i, sizeof(*clones) * (nclone-i));
	return 1;
    } else
	return 0;
}

/* We just got a new user; does it look like a clone?  If so, kill the
 * user if not under or at the CLONE threshold.
 */
void clones_add(const char *nick, const char *host)
{
    Clone *clone;

    if (!(clone = findclone(host))) {
	clone = scalloc(sizeof(Clone), 1);
	if (!host)
	    host = "";
	clone->host = sstrdup(host);
	clone->amount = 1;
	clone->next = clonelist;
	if (clonelist)
	    clonelist->prev = clone;
	clonelist = clone;
    } else {
	int i;
	clone->amount += 1;

	for (i = 0; i < nclone; ++i)
	    if (match_wild_nocase(clones[i].host, host)) {
		if (clone->amount > clones[i].amount)
		    kill_user(s_OperServ, nick, DEF_CLONE_REASON);
		return;
	    }
	if (clone->amount > CLONES_ALLOWED)
	    kill_user(s_OperServ, nick, DEF_CLONE_REASON);
    }
}

void clones_del(const char *host)
{
    Clone *clone;

    if (!(clone = findclone(host)))
	return;

    if (clone->amount>1)
	clone->amount -= 1;
    else {
	free(clone->host);
	if (clone->prev)
	    clone->prev->next = clone->next;
	else
	    clonelist = clone->next;
	if (clone->next)
	    clone->next->prev = clone->prev;
	free(clone);
    }
}

static Clone *findclone(const char *host)
{
    Clone *clone;
    for (clone = clonelist; clone && stricmp(clone->host, host) != 0;
							clone = clone->next) ;
    return clone;
}

#endif /* CLONES */

/*************************************************************************/
/*************************************************************************/

/* Set various Services runtime options. */

static void do_set(const char *source)
{
    char *option = strtok(NULL, " ");
    char *setting = strtok(NULL, " ");

    if (!option || !setting)
	notice(s_OperServ, source,
			"Syntax: \2SET \37option\37 \37setting\37\2");
    else if (stricmp(option, "IGNORE") == 0)
	if (stricmp(setting, "on") == 0) {
	    allow_ignore = 1;
	    notice(s_OperServ, source, "Ignore code \2will\2 be used.");
	} else if (stricmp(setting, "off") == 0) {
	    allow_ignore = 0;
	    notice(s_OperServ, source, "Ignore code \2will not\2 be used.");
	} else
	    notice(s_OperServ, source,
			"Setting for \2IGNORE\2 must be \2ON\2 or \2OFF\2.");
    else
	notice(s_OperServ, source, "Unknown option \2%s\2.", option);
}

/*************************************************************************/
#endif /* OPERSERV */
