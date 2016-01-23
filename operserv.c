/* OperServ functions.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

/* Nick for sending global notices */
#ifdef GLOBALNOTICER
const char s_GlobalNoticer[] = GLOBALNOTICER_NAME;
#endif

#ifdef OPERSERV

#include "os-help.c"

/* Nick for OperServ */
const char s_OperServ[] = OPERSERV_NAME;

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

#ifdef AKILL
static void do_akill(const char *source, int call);
static int add_akill(const char *mask, const char *reason, const char *who, int call);
static int del_akill(const char *mask, int call);
static int is_on_akill(const char *mask);
#endif
#ifdef CLONES
static void do_clone(const char *source);
static void add_clone(const char *host, int amount, const char *reason, const char *who);
static int del_clone(const char *host);
static Clone *findclone(const char *host);
static int is_on_clone(char *host);
#endif
static void do_set(const char *source);

/*************************************************************************/

/* Main OperServ routine. */

void operserv(const char *source, char *buf)
{
    char *cmd;
    char *s, *chan, *nick, *serv;
    int i;

    log("%s: %s: %s", s_OperServ, source, buf);
    cmd = strtok(buf, " ");

    if(mode==0 && stricmp(cmd, "ON")!=0) {
	if (offreason)
	    notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2 (%s).", offreason);
	else
	    notice(s_OperServ, source, "Sorry, Services are curently \2OFF\2.");
	return;
    }

    if (!cmd) {

	return;

    } else if (stricmp(cmd, "\1PING") == 0) {

	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_OperServ, source, "\1PING %s", s);

    } else if (stricmp(cmd, "MODE") == 0) {

	Channel *c;
	User *u;
	char l[16];
	chan = strtok(NULL, " ");
	serv = strtok(NULL, " ");
	s = strtok(NULL, "");
	if (!chan)
	    return;

	if (chan[0]=='#') {
	    if (!(c = findchan(chan)))
		return;
	    else if (!serv) {
		sprintf(l, " %d", c->limit);
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
	    } else {
		if (s)
		    change_cmode(s_OperServ, chan, serv, s);
		else
		    change_cmode(s_OperServ, chan, serv, "");
	    }
	} else {
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

    } else if (stricmp(cmd, "KICK") == 0) {

	chan = strtok(NULL, " ");
	nick = strtok(NULL, " ");
	s = strtok(NULL, "");
	if (!chan || !nick || !s)
	    return;
	sprintf(serv, "%s (%s)", source, s);
	kick_user(s_OperServ, chan, nick, serv);

    } else if (stricmp(cmd, "HELP") == 0) {

	cmd = strtok(NULL, " ");

	if (!cmd) {
	    notice_list(s_OperServ, source, os_help);
	    if (is_services_op(source))
		notice_list(s_OperServ, source, os_sop_help);
	    notice_list(s_OperServ, source, os_end_help);
	} else if (stricmp(cmd, "MODE") == 0)
	    notice_list(s_OperServ, source, mode_help);
	else if (stricmp(cmd, "USERLIST") == 0)
	    notice_list(s_OperServ, source, userlist_help);
	else if (stricmp(cmd, "CHANLIST") == 0)
	    notice_list(s_OperServ, source, chanlist_help);
	else if (stricmp(cmd, "CHANUSERS") == 0)
	    notice_list(s_OperServ, source, chanusers_help);
	else if (stricmp(cmd, "KICK") == 0) {
	    notice_list(s_OperServ, source, kick_help);
#ifdef AKILL
	} else if (stricmp(cmd, "AKILL") == 0) {
	    notice_list(s_OperServ, source, akill_help);
#endif
#ifdef GLOBALNOTICER
	} else if (stricmp(cmd, "GLOBAL") == 0) {
	    /* Information varies, so we need to do it manually. */
	    notice(s_OperServ, source, "Syntax: GLOBAL \37message\37");
	    notice(s_OperServ, source, "");
	    notice(s_OperServ, source,
			"Allows IRCops to send messages to all users on the");
	    notice(s_OperServ, source,
			"network.  The message will be sent from the nick");
	    notice(s_OperServ, source, "\2%s\2.", s_GlobalNoticer);
#endif
	} else if (stricmp(cmd, "STATS") == 0)
	    notice_list(s_OperServ, source, stats_help);
	else if (stricmp(cmd, "SETTINGS") == 0)
	    notice_list(s_OperServ, source, settings_help);
	else if (!is_services_op(source))
	    notice(s_OperServ, source,
			"No help available for command \2%s\2.", cmd);
#ifdef DAL_SERV
	else if (stricmp(cmd, "QLINE") == 0)
	    notice_list(s_OperServ, source, qline_help);
	else if (stricmp(cmd, "UNQLINE") == 0)
	    notice_list(s_OperServ, source, unqline_help);
	else if (stricmp(cmd, "NOOP") == 0)
	    notice_list(s_OperServ, source, noop_help);
	else if (stricmp(cmd, "KILL") == 0)
	    notice_list(s_OperServ, source, kill_help);
#endif
	else if (stricmp(cmd, "JUPE") == 0)
	    notice_list(s_OperServ, source, jupe_help);
#ifdef AKILL
	else if (stricmp(cmd, "PAKILL") == 0)
	    notice_list(s_OperServ, source, pakill_help);
#endif
#ifdef CLONES
	else if (stricmp(cmd, "CLONE") == 0)
	    notice_list(s_OperServ, source, clone_help);
#endif
	else if (stricmp(cmd, "UPDATE") == 0)
	    notice_list(s_OperServ, source, update_help);
	else if (stricmp(cmd, "ON") == 0)
	    notice_list(s_OperServ, source, offon_help);
	else if (stricmp(cmd, "OFF") == 0)
	    notice_list(s_OperServ, source, offon_help);
	else if (stricmp(cmd, "QUIT") == 0)
	    notice_list(s_OperServ, source, quit_help);
	else if (stricmp(cmd, "SHUTDOWN") == 0)
	    notice_list(s_OperServ, source, shutdown_help);
	else
	    notice(s_OperServ, source,
			"No help available for command \2%s\2.", cmd);

#ifdef GLOBALNOTICER
    } else if (stricmp(cmd, "GLOBAL") == 0) {

	char *msg = strtok(NULL, "");

	if (!msg) {
	    notice(s_OperServ, source, "Syntax: \2GLOBAL \37msg\37\2");
	    notice(s_OperServ, source,
			"\2/msg %s HELP GLOBAL for more information.",
			s_OperServ);
	}
	noticeall(s_GlobalNoticer, "%s", msg);
#endif /* GLOBALNOTICER */
    } else if (stricmp(cmd, "SETTINGS") == 0) {

	notice(s_OperServ, source, "Expiries: ");
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
	notice(s_OperServ, source, "Services OPs: \2%s\2.", SERVICES_OPS);

    } else if (stricmp(cmd, "USERLIST") == 0) {

	s = strtok(NULL, " ");
	send_user_list(source, s);
	
    } else if (stricmp(cmd, "CHANLIST") == 0) {

	s = strtok(NULL, " ");
	send_channel_list(source, s);

    } else if (stricmp(cmd, "CHANUSERS") == 0) {

	chan = strtok(NULL, " ");
	if (chan)
	    send_channel_users(source, chan);

#ifdef AKILL
    } else if (stricmp(cmd, "AKILL") == 0) {

	do_akill(source, 0);
#endif

    } else if (stricmp(cmd, "STATS") == 0) {

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

    /* Services ops only below this point. */

    } else if (!is_services_op(source)) {

	notice(s_OperServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_OperServ);

#ifdef DAL_SERV
    } else if (stricmp(cmd, "QLINE") == 0) {

	nick = strtok(NULL, " ");
	s = strtok(NULL, "");
	if(!nick)
	    return;

	if(!s) {
	    send_cmd(NULL, "SQLINE %s", nick);
	    globops(s_OperServ, "\2%s\2 has been QUARENTINED by \2%s\2", nick, source);
	} else {
	    send_cmd(NULL, "SQLINE %s :%s", nick, s);
	    globops(s_OperServ, "\2%s\2 has been QUARENTINED by \2%s\2 for %s", nick, source, s);
	}

    } else if (stricmp(cmd, "UNQLINE") == 0) {

	nick = strtok(NULL, " ");
	if(!nick)
	    return;

	send_cmd(NULL, "UNSQLINE %s", nick);
	globops(s_OperServ, "\2%s\2 removed QUARENTINE for \2%s\2", source, nick);

    } else if (stricmp(cmd, "NOOP") == 0) {

	serv = strtok(NULL, " ");
	s = strtok(NULL, "");
	if (!s || (s[0]!='+' && s[0]!='-'))
	    return;

	send_cmd(s_OperServ, "SVSNOOP 1 :%s", serv, s);
	if(s[0]=='+')
	     globops(s_OperServ, "\2%s\2 QUARENTINED OPERS on \2%s\2", source, serv);
	else
	     globops(s_OperServ, "\2%s\2 removed QUARENTINE for OPERS on \2%s\2", source, serv);

    } else if (stricmp(cmd, "KILL") == 0) {

	nick = strtok(NULL, " ");
	s = strtok(NULL, "");
	if (!s || !finduser(nick))
	    return;

	send_cmd(s_OperServ, "SVSKILL %s :%s", nick, s);

#endif

    } else if (stricmp(cmd, "LISTIGNORE") == 0) {

	int sent_header = 0;
	IgnoreData *id;

	for (i = 0; i < 256; ++i) {
	    for (id = ignore[i]; id; id = id->next) {
		if (!sent_header) {
		    notice(s_OperServ, source, "Services ignorance list:");
		    sent_header = 1;
		}
		notice(s_OperServ, source, "%d %s", id->time, id->who);
	    }
	}
	if (!sent_header)
	    notice(s_OperServ, source, "Ignorance list is empty.");

    } else if (stricmp(cmd, "UPDATE") == 0) {

	notice(s_OperServ, source, "Updating databases.");
	save_data = 1;

#ifdef AKILL
    } else if (stricmp(cmd, "PAKILL") == 0) {

	do_akill(source, 1);
#endif

#ifdef CLONES
    } else if (stricmp(cmd, "CLONE") == 0) {

	do_clone(source);
#endif

    } else if (stricmp(cmd, "SET") == 0) {

	do_set(source);

    } else if (stricmp(cmd, "JUPE") == 0) {

	char *jserver = strtok(NULL, " ");
	if (!jserver) {
	    notice(s_OperServ, source, "Syntax: \2JUPE \37servername\37\2");
	} else {
	    globops(s_OperServ, "\2Juping\2 %s by request of \2%s\2.", jserver, source);
	    send_cmd(NULL, "SERVER %s 2 :Jupitered server", jserver);
	}

    } else if (stricmp(cmd, "OFF") == 0) {

	char *pass = strtok(NULL, " ");
	s = strtok(NULL, "");
	if (!pass)
	    return;
	if (stricmp(pass, SUPERPASS)==0) {
	    mode = 0;
	    notice(s_OperServ, source, "Services are now switched \2OFF\2.");
	    globops(s_OperServ, "Services switched \2OFF\2 by request of \2%s\2.", source);
	    if(s) {
		if (offreason = malloc(strlen(s)+1))
		    sprintf(offreason, "%s", s);
		else
		    offreason = s;
	    }
#ifdef GLOBALNOTICER
	    if (offreason)
		noticeall(s_GlobalNoticer, "Services are currently \2OFF\2 - %s", offreason);
	    else
		noticeall(s_GlobalNoticer, "Services are currently \2OFF\2 - Please do not attempt to use them!");
#endif
	} else
	    notice(s_OperServ, source, "Access Denied.");

    } else if (stricmp(cmd, "ON") == 0) {

	char *pass = strtok(NULL, " ");
	if (!pass)
	    return;
	if (stricmp(pass, SUPERPASS)==0) {
	    mode = 1;
	    notice(s_OperServ, source, "Services are now switched \2ON\2 again.");
	if (offreason)
	    offreason = NULL;

#ifdef GLOBALNOTICER
	    noticeall(s_GlobalNoticer, "Services are back \2ON\2 again - Please use them at will again.");
#endif
	} else
	    notice(s_OperServ, source, "Access Denied.");

    } else if (stricmp(cmd, "QUIT") == 0) {

	char *pass = strtok(NULL, " ");
	if (!pass)
	    return;
	if (stricmp(pass, SUPERPASS)==0) {
	    quitmsg = malloc(32 + strlen(source));
	    if (!quitmsg)
		quitmsg = "QUIT command received, but out of memory!";
	    else
		sprintf(quitmsg, "QUIT command received from %s", source);
	    quitting = 1;
	    terminating = 1;
	} else
	    notice(s_OperServ, source, "Access Denied.");

    } else if (stricmp(cmd, "SHUTDOWN") == 0) {

	char *pass = strtok(NULL, " ");
	if (!pass)
	    return;
	if (stricmp(pass, SUPERPASS)==0) {
	    quitmsg = malloc(32 + strlen(source));
	    save_data = 1;
	    if (!quitmsg)
		quitmsg = "SHUTDOWN command received, but out of memory!";
            else
		sprintf(quitmsg, "SHUTDOWN command received from %s", source);
	    quitting = 1;
	    terminating = 1;
	} else
	    notice(s_OperServ, source, "Access Denied.");

    } else if (stricmp(cmd, "RAW") == 0) {

	char *pass = strtok(NULL, " ");
	char *text = strtok(NULL, "");
	if (!pass)
	    return;
	if (stricmp(pass, SUPERPASS)==0) {
	    if (!text)
		notice(s_OperServ, source, "Syntax: \2RAW \37password command\37\2");
	    else
		send_cmd(NULL, text);
	} else
	    notice(s_OperServ, source, "Access Denied.");

    } else {

	notice(s_OperServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_OperServ);

    }
}

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

static void do_akill(const char *source, int call)
{
    char *cmd, *mask, *reason, *who, *s;
    int i;

    cmd = strtok(NULL, " ");
    if (!cmd)
	cmd = "";

    if (stricmp(cmd, "ADD") == 0) {
	if ((mask = strtok(NULL, " ")) && (reason = strtok(NULL, ""))) {
	    strlower(mask);
	    if (!(s = strchr(mask, '@'))) {
		notice(s_OperServ, source, "Hostmask must contain an `@'
			character.");
		return;
	    }
		/* Find @*, @*.*, @*.*.*, etc. and dissalow if !SOP */
	    for(i=strlen(mask)-1;(mask[i]=='*' || mask[i]=='?' || mask[i]=='.') && i>=0;i--) ;
	    if(mask[i]=='@' && !is_services_op(source))
		notice(s_OperServ, source, "@* AKILL's are not allowed!!");
	    else {
		int nonchr = 0;
		for(;mask[i]!='@';i--)
		    if(!(mask[i]=='*' || mask[i]=='?' || mask[i]=='.'))
			nonchr++;
		if(nonchr<4 && !is_services_op(source))
		    notice(s_OperServ, source, "Must have at least 3 non- *, ? or . characters.");
		else if(is_on_akill(mask))
		    notice(s_OperServ, source, "AKILL already exists (or inclusive)");
		else {		
		    notice(s_OperServ, source, "%s added to AKILL list (%d users killed)", mask, add_akill(mask, reason, source, call));
		}
	    }
	} else {
	    notice(s_OperServ, source,
			"Syntax: AKILL ADD \37mask\37 \37reason\37");
	}
  if(services_level!=1) {
	notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
  }

    } else if (stricmp(cmd, "DEL") == 0) {
	if (mask = strtok(NULL, " ")) {
	    if (strspn(mask, "1234567890") == strlen(mask) &&
					(i = atoi(mask)) > 0 && i <= nakill)
		strcpy(mask, akills[i-1].mask);
	    strlower(mask);
	    s = strchr(mask, '@');
	    if (del_akill(mask, call)) {
		notice(s_OperServ, source, "%s removed from AKILL list.", mask);
		if (s) {
		    *s++ = 0;
		    send_cmd(server_name, "RAKILL %s %s", s, mask);
		} else {
		    /* We lose... can't figure out what's a username and what's
		     * a hostname.  Ah well.
		     */
		}
	    } else {
		if(call==1)
		    notice(s_OperServ, source, "%s not found on AKILL list.", mask);
		else
		    notice(s_OperServ, source, "%s not found on AKILL list or is PERMINANT.", mask);
	    }
	} else {
	    notice(s_OperServ, source, "Syntax: AKILL DEL \37mask\37");
	}
  if(services_level!=1) {
	notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
  }

    } else if (stricmp(cmd, "LIST") == 0) {
	s = strtok(NULL, " ");
	if (s) {
	    strlower(s);
	    strchr(s, '@');
	}
	notice(s_OperServ, source, "Current AKILL list:");
	for (i = 0; i < nakill; ++i) {
	    if (!s || match_wild_nocase(s, akills[i].mask)) {
		notice(s_OperServ, source, "  %3d %-32s  %s",
					i+1, akills[i].mask, akills[i].reason);
	    }
	}

    } else if (stricmp(cmd, "VIEW") == 0) {
	s = strtok(NULL, " ");
	if (s) {
	    strlower(s);
	    strchr(s, '@');
	}
	notice(s_OperServ, source, "Current AKILL list:");
	for (i = 0; i < nakill; ++i) {
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

    } else {
	notice(s_OperServ, source,
		"Syntax: \2AKILL {ADD|DEL|LIST|view} [\37mask\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP AKILL\2", s_OperServ);
    }
}

static int is_on_akill(const char *mask)
{
    int i;

    for (i = 0; i < nakill; ++i) {
	if (match_wild_nocase(akills[i].mask, mask))
	    return 1;
    }
    return 0;
}

/*************************************************************************/

void expire_akill () {
    int i;
    const time_t expire_time = AKILL_EXPIRE*24*60*60;
    time_t tm;
    for (i = 0; i < nakill; ++i) {
	if (akills[i].time > 0) {
	    tm = time(NULL) - akills[i].time;
	    if (tm > expire_time) {
		free(akills[i].mask);
		free(akills[i].reason);
		--nakill;
		if (i < nakill)
		    bcopy(akills+i+1, akills+i, sizeof(*akills) * (nakill-i));
	    }
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
    for (i = 0; i < nakill; ++i) {
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
	    send_cmd(server_name,
			"AKILL %s %s :You are Banned (%s)",
			myhost, myuser, akills[i].reason);
	    return 1;
	}
    }
    return 0;
}

/*************************************************************************/

static int add_akill(const char *mask, const char *reason, const char *who, int call)
{
    User *user;
    int cnt, killd;
    char buf[strlen(reason)+20];
    sprintf(buf, "You are banned (%s)", reason);

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
    int i;

    for (i = 0; i < nakill && stricmp(akills[i].mask, mask) != 0; ++i) ;
    if (i < nakill) {
	if (akills[i].time==0 && call!=1)
	    return 0;
	free(akills[i].mask);
	free(akills[i].reason);
	--nakill;
	if (i < nakill)
	    bcopy(akills+i+1, akills+i, sizeof(*akills) * (nakill-i));
	return 1;
    } else {
	return 0;
    }
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
    char *cmd, *host, *amt, *reason;
    int i, amount;

    cmd = strtok(NULL, " ");
    if (!cmd)
	cmd = "";

    if (stricmp(cmd, "ADD") == 0) {
	host   = strtok(NULL, " ");
	amt    = strtok(NULL, " ");
	reason = strtok(NULL, "");
	if (reason) {
	    strlower(host);
	    amount = atoi(amt);
		/* Find @*, @*.*, @*.*.*, etc. and dissalow */
	    for(i=strlen(host)-1;(host[i]=='*' || host[i]=='?' || host[i]=='.') && i>=0;i--) ;
	    if(i==-1)
		notice(s_OperServ, source, "* CLONE HOST's are not allowed!!");
	    else if(is_on_clone(host))
		notice(s_OperServ, source, "CLONE HOST already exists (or inclusive)");
	    else if (amount<1)
		notice(s_OperServ, source, "CLONE AMOUNT must be greater than 0");
	    else if (amount>999)
		notice(s_OperServ, source, "CLONE AMOUNT must be less than 1000");
	    else {
		add_clone(host, amount, reason, source);
		notice(s_OperServ, source, "%s added to CLONE list.", host);
	    }

	} else {

	    notice(s_OperServ, source,
			"Syntax: CLONE ADD \37host\37 \37amount\37 \37reason\37");
	}
  if(services_level!=1) {
	notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
  }

    } else if (stricmp(cmd, "DEL") == 0) {
	host   = strtok(NULL, " ");
	if (host) {
	    if (strspn(host, "1234567890") == strlen(host) &&
					(i = atoi(host)) > 0 && i <= nclone)
		strcpy(host, clones[i-1].host);
	    strlower(host);
	    if (del_clone(host)) {
		notice(s_OperServ, source, "%s removed from CLONE list.", host);
	    } else {
		notice(s_OperServ, source, "%s not found on CLONE list.", host);
	    }
	} else {
	    notice(s_OperServ, source, "Syntax: CLONE DEL \37host\37");
	}
  if(services_level!=1) {
	notice(s_OperServ, source,
		"\2Notice:\2 Changes will not be saved!  Services is in read-only mode.");
  }

    } else if (stricmp(cmd, "LIST") == 0) {
	host   = strtok(NULL, " ");
	if (host)
	    strlower(host);
	notice(s_OperServ, source, "Current CLONE list:");
	for (i = 0; i < nclone; ++i) {
	    if (!host || match_wild_nocase(host, clones[i].host)) {
		notice(s_OperServ, source, "  %3d %-32s  [%3d] %s",
			i+1, clones[i].host, clones[i].amount, clones[i].reason);
	    }
	}

    } else if (stricmp(cmd, "VIEW") == 0) {
	host   = strtok(NULL, " ");
	if (host)
	    strlower(host);
	notice(s_OperServ, source, "Current CLONE list:");
	for (i = 0; i < nclone; ++i) {
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

    } else {
	notice(s_OperServ, source,
		"Syntax: \2CLONE {ADD|DEL|LIST|VIEW} [\37host\37]\2");
	notice(s_OperServ, source,
		"For help: \2/msg %s HELP CLONE\2", s_OperServ);
    }
}

static int is_on_clone(char *host)
{
    int i;

    strlower(host);
    for (i = 0; i < nclone; ++i) {
	if (match_wild_nocase(clones[i].host, host)) {
	    return 1;
	}
    }
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
    } else {
	return 0;
    }
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
	char buf[512];
	int i;

	clone->amount += 1;

	strscpy(buf, host, sizeof(buf)-1);
	strlower(buf);
	for (i = 0; i < nclone; ++i) {
	    if (match_wild(clones[i].host, buf)) {
		if (clone->amount>clones[i].amount)
		    kill_user(s_OperServ, nick, DEF_CLONE_REASON);
		return;
	    }
	}
	if (clone->amount>CLONES_ALLOWED)
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

    if (!option || !setting) {
	notice(s_OperServ, source,
			"Syntax: \2SET \37option\37 \37setting\37\2");
    } else if (stricmp(option, "IGNORE") == 0) {
	if (stricmp(setting, "on") == 0) {
	    allow_ignore = 1;
	    notice(s_OperServ, source, "Ignore code \2will\2 be used.");
	} else if (stricmp(setting, "off") == 0) {
	    allow_ignore = 0;
	    notice(s_OperServ, source, "Ignore code \2will not\2 be used.");
	} else {
	    notice(s_OperServ, source,
			"Setting for \2IGNORE\2 must be \2ON\2 or \2OFF\2.");
	}
    } else {
	notice(s_OperServ, source, "Unknown option \2%s\2.", option);
    }
}

/*************************************************************************/
#endif /* OPERSERV */
