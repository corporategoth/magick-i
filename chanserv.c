/* ChanServ functions.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

#ifdef CHANSERV

/* These 3 no longer static, as they are needed in memoserv.c for the
 * news functions.
 */
ChannelInfo *chanlists[256];
ChannelInfo *cs_findchan(const char *chan);

static short def_access[] = {
	-1,	/* CA_AUTODEOP	*/
	5,	/* CA_AUTOVOICE	*/
	10,	/* CA_AUTOOP	*/
	0,	/* CA_READMEMO	*/
	15,	/* CA_WRITEMEMO	*/
	25,	/* CA_DELMEMO	*/
	20,	/* CA_AKICK	*/
	25,	/* CA_STARAKICK	*/
	10,	/* CA_UNBAN	*/
	5,	/* CA_ACCESS	*/
	25,	/* CA_SET	*/
	5,	/* CA_CMDINVITE	*/
	10,	/* CA_CMDUNBAN	*/
	5,	/* CA_CMDVOICE	*/
	10,	/* CA_CMDOP	*/
	20,	/* CA_CMDCLEAR	*/
	-1,	/* CA_FLOOR	*/
	30,	/* CA_CAP	*/
	50,	/* CA_FOUNDER	*/
	-1
};

typedef struct {
	int what;
	char *cmd;
	char *desc;
} LevelInfo;
static LevelInfo levelinfo[] = {
    { CA_AUTODEOP,	"AUTODEOP",	"Automatic Mode -v-o (DeVoice and DeOp)" },
    { CA_AUTOVOICE,	"AUTOVOICE",	"Automatic Mode +v (Voice)" },
    { CA_AUTOOP,	"AUTOOP",	"Automatic Mode +o (Op)" },
    { CA_READMEMO,	"NEWSREAD",	"Read channel news" },
    { CA_WRITEMEMO,	"NEWSPOST",	"Post channel news" },
    { CA_DELMEMO,	"NEWSDEL",	"Erase ALL or non-owned channel news" },
    { CA_AKICK,		"AKICK",	"Modify AKICK list" },
    { CA_STARAKICK,	"STARAKICK",	"Add @* AKICK's" },
    { CA_UNBAN,		"BOUNCE",	"Bounce server bans" },
    { CA_ACCESS,	"ACCESS",	"Modify ACCESS list" },
    { CA_SET,		"SET",		"Modify channel SET's (except FOUNDER and PASSWORD)" },
    { CA_CMDINVITE,	"INVITE",	"Use /MSG ChanServ INVITE command" },
    { CA_CMDUNBAN,	"UNBAN",	"Use /MSG ChanServ UNBAN command" },
    { CA_CMDVOICE,	"VOICE",	"Use /MSG ChanServ VOICE command" },
    { CA_CMDOP,		"OP",		"Use /MSG ChanServ OP command" },
    { CA_CMDCLEAR,	"CLEAR",	"Use /MSG ChanServ CLEAR command" },
    { -1 }
};

typedef struct {
	int what;
	char *cmd;
	int rev1;
	int rev2;
	int rev3;
	char *desc;
} RevengeInfo;
static RevengeInfo revengeinfo[] = {
    { CR_NONE,		"NONE",		0, 0, 0,	"Disabled" },
    { CR_REVERSE,	"REVERSE",	0, 0, 1,	"Reverse Activity" },
    { CR_DEOP,		"DEOP",		0, 1, 0,	"Mode -o (DeOp)" },
    { CR_KICK,		"KICK",		0, 1, 1,	"Kick" },
    { CR_NICKBAN,	"NICKBAN",	1, 0, 0,	"Ban (user!*@*)" },
    { CR_USERBAN,	"MASKBAN",	1, 0, 1,	"Ban (*!*user@*.host)" },
    { CR_HOSTBAN,	"HOSTBAN",	1, 1, 0,	"Ban (*!*@host)" },
    { CR_MIRROR,	"MIRROR",	1, 1, 1,	"Mirror Activity" },
    { -1 }
};

/* Note, the *-* symbol denotes cannot be changed by user. */

const char s_ChanServ[] = CHANSERV_NAME;
#include "cs-help.c"

static void alpha_insert_chan(ChannelInfo *ci);
static ChannelInfo *makechan(const char *chan);
static int is_identified(User *user, ChannelInfo *ci);

static void do_help(const char *source);
static void do_register(const char *source);
static void do_identify(const char *source);
static void do_drop(const char *source);
static void do_set(const char *source);
static void do_set_founder(User *u, ChannelInfo *ci, char *param);
static void do_set_password(User *u, ChannelInfo *ci, char *param);
static void do_set_desc(User *u, ChannelInfo *ci, char *param);
static void do_set_topic(User *u, ChannelInfo *ci, char *param);
#if FILE_VERSION > 2
    static void do_set_url(User *u, ChannelInfo *ci, char *param);
#endif
static void do_set_mlock(User *u, ChannelInfo *ci, char *param);
static void do_set_keeptopic(User *u, ChannelInfo *ci, char *param);
static void do_set_topiclock(User *u, ChannelInfo *ci, char *param);
static void do_set_private(User *u, ChannelInfo *ci, char *param);
static void do_set_secureops(User *u, ChannelInfo *ci, char *param);
static void do_set_restricted(User *u, ChannelInfo *ci, char *param);
static void do_set_secure(User *u, ChannelInfo *ci, char *param);
static void do_set_join(User *u, ChannelInfo *ci, char *param);
static void do_set_revenge(User *u, ChannelInfo *ci, char *param);
static void do_level(const char *source);
static void do_level_list(const char *source, char *chan);
static void do_level_set(const char *source, char *chan);
static void do_level_reset(const char *source, char *chan);
static void do_access(const char *source);
static void do_access_add(const char *source, char *chan);
static void do_access_del(const char *source, char *chan);
static void do_access_list(const char *source, char *chan);
#if AKICK_MAX > 0
    static void do_akick(const char *source);
    static void do_akick_add(const char *source, char *chan);
    static void do_akick_del(const char *source, char *chan);
    static void do_akick_list(const char *source, char *chan);
    static int is_on_akick(const char *mask, ChannelInfo *ci);
#endif
static void do_info(const char *source);
static void do_list(const char *source);
static void do_users(const char *source);
static void do_invite(const char *source);
static void do_voice(const char *source);
static void do_devoice(const char *source);
static void do_op(const char *source);
static void do_deop(const char *source);
static void do_unban(const char *source);
static void do_clear(const char *source);
static void do_getpass(const char *source);
static void do_forbid(const char *source);
static void do_suspend(const char *source);
static void do_unsuspend(const char *source);


/*************************************************************************/
/*************************************************************************/

/* Display total number of registered channels and info about each; or, if
 * a specific channel is given, display information about that channel
 * (like /msg ChanServ INFO <channel>).  If count_only != 0, then only
 * display the number of registered channels (the channel parameter is
 * ignored).
 */

void listchans(int count_only, const char *chan)
{
    long count = 0;
    ChannelInfo *ci;
    int i;
    time_t curtime = time(NULL);

    if (count_only) {

	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next)
		++count;
	printf("%d channels registered.\n", count);

    } else if (chan) {

	struct tm tm;
	NickInfo *ni;
	char *t;

	if (!(ci = cs_findchan(chan))) {
	    printf("Channel %s not registered.\n", chan);
	    return;
	}
	if (ni = findnick(ci->founder))
	    t = ni->last_usermask;
	else
	    t = NULL;
	if (ci->flags & CI_VERBOTEN)
	    printf("Channel %s is FORBIDden.\n");
	else {
	    printf("Information about channel %s:\n", ci->name);
	    printf("        Founder: %s%s%s%s\n",
			ci->founder, t ? " (" : "", t ? t : "", t ? ")" : "");
	    printf("    Description: %s\n", ci->desc);
	if(strlen(ci->url)>0)
	    printf(" WWW Page (URL): %s", ci->url);
	    tm = *localtime(&ci->time_registered);
	    printf("     Registered: %s %2d %02d:%02d:%02d %d\n",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
	    tm = *localtime(&ci->last_used);
	    printf("      Last used: %s %2d %02d:%02d:%02d %d\n",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
	    tm = *localtime(&curtime);
	    printf("   Current time: %s %2d %02d:%02d:%02d %d\n",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
	    if (ci->last_topic) {
		if (ci->flags & CI_SUSPENDED) {
		    printf("  Suspended For: %s\n", ci->last_topic);
		    printf("   Suspended By: %s\n", ci->last_topic_setter);
		} else {
		    printf("     Last topic: %s\n", ci->last_topic);
		    printf("   Topic set by: %s\n", ci->last_topic_setter);
		}
	    }
	    printf("        Options: ");
	    if (!ci->flags) 
		printf("None\n");
	    else {
		int need_comma = 0;
		static const char commastr[] = ", ";
		if (ci->flags & CI_SUSPENDED)
		    printf("SUSPENDED CHANNEL");
		else {
		    if (ci->flags & CI_PRIVATE) {
			printf("Private");
			need_comma = 1;
		    }
		    if (ci->flags & CI_KEEPTOPIC) {
			printf("%sTopic Retention", need_comma ? commastr : "");
			need_comma = 1;
		    }
		    if (ci->flags & CI_TOPICLOCK) {
			printf("%sTopic Lock", need_comma ? commastr : "");
			need_comma = 1;
		    }
		    if (ci->flags & CI_SECUREOPS) {
			printf("%sSecure Ops", need_comma ? commastr : "");
			need_comma = 1;
		    }
		    if (ci->flags & CI_RESTRICTED) {
			printf("%sRestricted Access", need_comma ? commastr : "");
			need_comma = 1;
		    }
		    if (ci->flags & CI_SECURE) {
			printf("%sSecure", need_comma ? commastr : "");
			need_comma = 1;
		    }
		}
		printf("\n");
	    }
	    printf("      Mode lock: ");
	    if (ci->mlock_on || ci->mlock_key || ci->mlock_limit)
		printf("+%s%s%s%s%s%s%s%s",
			(ci->mlock_on & CMODE_I) ? "i" : "",
			(ci->mlock_key         ) ? "k" : "",
			(ci->mlock_limit       ) ? "l" : "",
			(ci->mlock_on & CMODE_M) ? "m" : "",
			(ci->mlock_on & CMODE_N) ? "n" : "",
			(ci->mlock_on & CMODE_P) ? "p" : "",
			(ci->mlock_on & CMODE_S) ? "s" : "",
			(ci->mlock_on & CMODE_T) ? "t" : "");
	    if (ci->mlock_off)
		printf("-%s%s%s%s%s%s%s%s",
			(ci->mlock_off & CMODE_I) ? "i" : "",
			(ci->mlock_off & CMODE_K) ? "k" : "",
			(ci->mlock_off & CMODE_L) ? "l" : "",
			(ci->mlock_off & CMODE_M) ? "m" : "",
			(ci->mlock_off & CMODE_N) ? "n" : "",
			(ci->mlock_off & CMODE_P) ? "p" : "",
			(ci->mlock_off & CMODE_S) ? "s" : "",
			(ci->mlock_off & CMODE_T) ? "t" : "");
	    if (ci->mlock_key)
		printf(" %s", ci->mlock_key);
	    if (ci->mlock_limit)
		printf(" %ld", ci->mlock_limit);
	    printf("\n");
	}

    } else {

	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next) {
		printf("%-20s  %s\n", ci->name,
			  ci->flags & CI_VERBOTEN  ? "Disallowed (FORBID)"
			: ci->flags & CI_SUSPENDED ? "Disallowed (SUSPEND)"
						: ci->desc);
		++count;
	    }
	printf("%d channels registered.\n", count);
    }
}

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

void get_chanserv_stats(long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    ChannelInfo *ci;

    for (i = 0; i < 256; i++)
	for (ci = chanlists[i]; ci; ci = ci->next) {
	    count++;
	    mem += sizeof(*ci);
	    if (ci->desc)
		mem += strlen(ci->desc)+1;
	    mem += ci->accesscount * sizeof(ChanAccess);
	    mem += ci->akickcount * sizeof(AutoKick);
	    for (j = 0; j < ci->akickcount; j++) {
		if (ci->akick[j].name)
		    mem += strlen(ci->akick[j].name)+1;
		if (ci->akick[j].reason)
		    mem += strlen(ci->akick[j].reason)+1;
	    }
	    if (ci->mlock_key)
		mem += strlen(ci->mlock_key)+1;
	    if (ci->last_topic)
		mem += strlen(ci->last_topic)+1;
	    if (ci->cmd_access)
		mem += sizeof(*ci->cmd_access) * CA_SIZE;
	}
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/
/*************************************************************************/

/* Main ChanServ routine. */

void chanserv(const char *source, char *buf)
{
    char *cmd, *s;
    cmd = strtok(buf, " ");

    if (!cmd)
	return;

    else if (stricmp(cmd, "\1PING") == 0) {
	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_ChanServ, source, "\1PING %s", s);

    } else {
	Hash *command, hash_table[] = {
		{ "HELP",	H_NONE,	do_help },
		{ "REG*",	H_NONE,	do_register },
		{ "ID*",	H_NONE,	do_identify },
		{ "DROP",	H_NONE,	do_drop },
		{ "SET*",	H_NONE,	do_set },
		{ "ACC*",	H_NONE,	do_access },
#if AKICK_MAX > 0
		{ "A*KICK",	H_NONE,	do_akick },
#endif
		{ "INVITE",	H_NONE,	do_invite },
		{ "UNBAN",	H_NONE,	do_unban },
		{ "*INFO",	H_NONE,	do_info },
		{ "LIST",	H_NONE,	do_list },
		{ "USER*",	H_NONE,	do_users },
		{ "WHO*",	H_NONE,	do_users },
		{ "V*",		H_NONE,	do_voice },
		{ "DV*",	H_NONE,	do_devoice },
		{ "DEV*",	H_NONE,	do_devoice },
		{ "OP*",	H_NONE,	do_op },
		{ "DOP*",	H_NONE,	do_deop },
		{ "DEOP*",	H_NONE,	do_deop },
		{ "CLEAR",	H_NONE,	do_clear },
		{ "LEVEL*",	H_NONE,	do_level },
		{ "GETPASS",	H_SOP,	do_getpass },
		{ "FORBID",	H_SOP,	do_forbid },
		{ "SUSPEND",	H_SOP,	do_suspend },
		{ "UNSUSPEND",	H_SOP,	do_unsuspend },
		{ NULL }
	};

	if (command = get_hash(source, cmd, hash_table))
	    (*command->process)(source);
	else
	    notice(s_ChanServ, source,
		"Unrecognized command \2%s\2.  Type \"/msg %s HELP\" for help.",
		cmd, s_ChanServ);
    }
}

/*************************************************************************/

/* Load/save data files. */

void load_cs_dbase(void)
{
    FILE *f = fopen(CHANSERV_DB, "r");
    int i, j, len;
    ChannelInfo *ci;

    if (!f) {
	log_perror("Can't read ChanServ database " CHANSERV_DB);
	return;
    }

    switch (i = get_file_version(f, CHANSERV_DB)) {

      case 4:
      case 3:
#if FILE_VERSION > 2

	for (i = 33; i < 256; ++i)
	    while (fgetc(f) == 1) {
		ci = smalloc(sizeof(ChannelInfo));
		if (1 != fread(ci, sizeof(ChannelInfo), 1, f))
		    fatal_perror("Read error on " CHANSERV_DB);
		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan(ci);
		ci->desc = read_string(f, CHANSERV_DB);
		ci->url = read_string(f, CHANSERV_DB);
		if (ci->mlock_key)
		    ci->mlock_key = read_string(f, CHANSERV_DB);
		if (ci->last_topic)
		    ci->last_topic = read_string(f, CHANSERV_DB);

		if (ci->accesscount) {
		    ChanAccess *access;
		    access = smalloc(sizeof(ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread(access, sizeof(ChanAccess),
							ci->accesscount, f))
			fatal_perror("Read error on " CHANSERV_DB);
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string(f, CHANSERV_DB);
		    j = 0; access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount) {
			if (access->is_nick < 0) {
			    --ci->accesscount;
			    free(access->name);
			    if (j < ci->accesscount)
				bcopy(access+1, access,	sizeof(*access) *
							(ci->accesscount - j));
			} else {
			    ++j; ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc(ci->access,
					sizeof(ChanAccess) * ci->accesscount);
		    else {
			free(ci->access);
			ci->access = NULL;
		    }
		} /* if (ci->accesscount) */

		if (ci->akickcount) {
		    AutoKick *akick;
		    akick = smalloc(sizeof(AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			    fread(akick, sizeof(AutoKick), ci->akickcount, f))
			fatal_perror("Read error on " CHANSERV_DB);
		    for (j = 0; j < ci->akickcount; ++j, ++akick) {
			akick->name = read_string(f, CHANSERV_DB);
			if (akick->reason)
			    akick->reason = read_string(f, CHANSERV_DB);
		    }
		    j = 0; akick = ci->akick;
		    while (j < ci->akickcount) {
			if (akick->is_nick < 0) {
			    --ci->akickcount;
			    free(akick->name);
			    if (akick->reason)
				free(akick->reason);
			    if (j < ci->akickcount)
				bcopy(akick+1, akick, sizeof(*akick) *
							(ci->akickcount - j));
			} else {
			    ++j; ++akick;
			}
		    }
		    if (ci->akickcount) {
			ci->akick = srealloc(ci->akick,
					sizeof(AutoKick) * ci->akickcount);
		    } else {
			free(ci->akick);
			ci->akick = NULL;
		    }
		} /* if (ci->akickcount) */

		if (ci->cmd_access) {
		    int n_entries;
		    ci->cmd_access = smalloc(sizeof(short) * CA_SIZE);
		    n_entries = fgetc(f)<<8 | fgetc(f);
		    if (n_entries < 0)
			fatal_perror("Read error on " CHANSERV_DB);
		    if (n_entries <= CA_SIZE) {
			fread(ci->cmd_access, sizeof(short), n_entries, f);
		    } else {
			fread(ci->cmd_access, sizeof(short), CA_SIZE, f);
			fseek(f, sizeof(short) * (n_entries - CA_SIZE),
								SEEK_CUR);
		    }
		}
		if (!ci->cmd_access)
		    ci->cmd_access = def_access;
	    } /* while (fgetc(f) == 1) */
	break; /* case 3, etc. */

#endif
      case 2:
      case 1:

	for (i = 33; i < 256; ++i)
	    while (fgetc(f) == 1) {
		ci = smalloc(sizeof(ChannelInfo));
		if (1 != fread(ci, sizeof(ChannelInfo), 1, f))
		    fatal_perror("Read error on " CHANSERV_DB);
		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan(ci);
		ci->desc = read_string(f, CHANSERV_DB);
#if FILE_VERSION > 2
		ci->url = sstrdup("");
#endif
		if (ci->mlock_key)
		    ci->mlock_key = read_string(f, CHANSERV_DB);
		if (ci->last_topic)
		    ci->last_topic = read_string(f, CHANSERV_DB);

		if (ci->accesscount) {
		    ChanAccess *access;
		    access = smalloc(sizeof(ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread(access, sizeof(ChanAccess),
							ci->accesscount, f))
			fatal_perror("Read error on " CHANSERV_DB);
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string(f, CHANSERV_DB);
		    j = 0; access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount) {
			if (access->is_nick < 0) {
			    --ci->accesscount;
			    free(access->name);
			    if (j < ci->accesscount)
				bcopy(access+1, access,	sizeof(*access) *
							(ci->accesscount - j));
			} else {
			    ++j; ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc(ci->access,
					sizeof(ChanAccess) * ci->accesscount);
		    else {
			free(ci->access);
			ci->access = NULL;
		    }
		} /* if (ci->accesscount) */

		if (ci->akickcount) {
		    AutoKick *akick;
		    akick = smalloc(sizeof(AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			    fread(akick, sizeof(AutoKick), ci->akickcount, f))
			fatal_perror("Read error on " CHANSERV_DB);
		    for (j = 0; j < ci->akickcount; ++j, ++akick) {
			akick->name = read_string(f, CHANSERV_DB);
			if (akick->reason)
			    akick->reason = read_string(f, CHANSERV_DB);
		    }
		    j = 0; akick = ci->akick;
		    while (j < ci->akickcount) {
			if (akick->is_nick < 0) {
			    --ci->akickcount;
			    free(akick->name);
			    if (akick->reason)
				free(akick->reason);
			    if (j < ci->akickcount)
				bcopy(akick+1, akick, sizeof(*akick) *
							(ci->akickcount - j));
			} else {
			    ++j; ++akick;
			}
		    }
		    if (ci->akickcount) {
			ci->akick = srealloc(ci->akick,
					sizeof(AutoKick) * ci->akickcount);
		    } else {
			free(ci->akick);
			ci->akick = NULL;
		    }
		} /* if (ci->akickcount) */

		if (ci->cmd_access) {
		    int n_entries;
		    ci->cmd_access = smalloc(sizeof(short) * CA_SIZE);
		    n_entries = fgetc(f)<<8 | fgetc(f);
		    if (n_entries < 0)
			fatal_perror("Read error on " CHANSERV_DB);
		    if (n_entries <= CA_SIZE) {
			fread(ci->cmd_access, sizeof(short), n_entries, f);
		    } else {
			fread(ci->cmd_access, sizeof(short), CA_SIZE, f);
			fseek(f, sizeof(short) * (n_entries - CA_SIZE),
								SEEK_CUR);
		    }
		}
		if (!ci->cmd_access)
		    ci->cmd_access = def_access;
	    }
	break; /* case 1, etc. */
      default:
	fatal("Unsupported version number (%d) on %s", i, CHANSERV_DB);
    } /* switch (version) */
    fclose(f);
}

/*************************************************************************/

void save_cs_dbase(void)
{
    FILE *f;
    int i, j, len;
    ChannelInfo *ci;

    remove(CHANSERV_DB ".save");
    if (rename(CHANSERV_DB, CHANSERV_DB ".save") < 0)
	log_perror("Can't back up " CHANSERV_DB);
    f = fopen(CHANSERV_DB, "w");
    if (!f) {
	log_perror("Can't write to ChanServ database " CHANSERV_DB);
	if (rename(CHANSERV_DB ".save", CHANSERV_DB) < 0)
	    fatal_perror("Can't restore backup copy of " CHANSERV_DB);
	return;
    }
    if (fchown(fileno(f), -1, file_gid) < 0)
	log_perror("Can't set group of %s to %d", CHANSERV_DB, file_gid);
    write_file_version(f, CHANSERV_DB);

    for (i = 33; i < 256; ++i) {
	for (ci = chanlists[i]; ci; ci = ci->next) {
	    fputc(1, f);
	    if (1 != fwrite(ci, sizeof(ChannelInfo), 1, f))
		fatal_perror("Write error on " CHANSERV_DB);
	    write_string(ci->desc ? ci->desc : "", f, CHANSERV_DB);
#if FILE_VERSION > 2
		write_string(ci->desc ? ci->url : "", f, CHANSERV_DB);
#endif
	    if (ci->mlock_key)
		write_string(ci->mlock_key, f, CHANSERV_DB);
	    if (ci->last_topic)
		write_string(ci->last_topic, f, CHANSERV_DB);

	    if (ci->accesscount) {
		ChanAccess *access = ci->access;
		if (ci->accesscount !=
			fwrite(access, sizeof(ChanAccess), ci->accesscount, f))
		    fatal_perror("Write error on " CHANSERV_DB);
		for (j = 0; j < ci->accesscount; ++j, ++access)
		    write_string(access->name, f, CHANSERV_DB);
	    }
	    if (ci->akickcount) {
		AutoKick *akick = ci->akick;
		if (ci->akickcount !=
			fwrite(akick, sizeof(AutoKick), ci->akickcount, f))
		    fatal_perror("Write error on " CHANSERV_DB);
		for (j = 0; j < ci->akickcount; ++j, ++akick) {
		    write_string(akick->name, f, CHANSERV_DB);
		    if (akick->reason)
			write_string(akick->reason, f, CHANSERV_DB);
		}
	    }
	    if (ci->cmd_access) {
		fputc(CA_SIZE >> 8, f);
		fputc(CA_SIZE & 0xFF, f);
		fwrite(ci->cmd_access, sizeof(short), CA_SIZE, f);
	    }
	} /* for (chanlists[i]) */
	fputc(0, f);
    } /* for (i) */

    fclose(f);
    remove(CHANSERV_DB ".save");
}

/*************************************************************************/

/* Check the current modes on a channel; if they conflict with a mode lock,
 * fix them. */

void check_modes(const char *chan)
{
    Channel *c = findchan(chan);
    ChannelInfo *ci = cs_findchan(chan);
    char newmodes[32], *newkey = NULL;
    long newlimit = 0;
    char *end = newmodes;
    int modes;
    int set_limit = 0, set_key = 0;

    if (!c || !ci)
	return;

    *end++ = '+';
    modes = ~c->mode & ci->mlock_on;
    if (modes & CMODE_I) {
	*end++ = 'i';
	c->mode |= CMODE_I;
    }
    if (modes & CMODE_M) {
	*end++ = 'm';
	c->mode |= CMODE_M;
    }
    if (modes & CMODE_N) {
	*end++ = 'n';
	c->mode |= CMODE_N;
    }
    if (modes & CMODE_P) {
	*end++ = 'p';
	c->mode |= CMODE_P;
    }
    if (modes & CMODE_S) {
	*end++ = 's';
	c->mode |= CMODE_S;
    }
    if (modes & CMODE_T) {
	*end++ = 't';
	c->mode |= CMODE_T;
    }

    if (!c->limit && ci->mlock_limit) {
	*end++ = 'l';
	newlimit = ci->mlock_limit;
	c->limit = newlimit;
	set_limit = 1;
    }
    if (!c->key && ci->mlock_key) {
	*end++ = 'k';
	newkey = ci->mlock_key;
	c->key = sstrdup(newkey);
	set_key = 1;
    }

    if (end[-1] == '+')
	--end;

    *end++ = '-';
    modes = c->mode & ci->mlock_off;
    if (modes & CMODE_I) {
	*end++ = 'i';
	c->mode &= ~CMODE_I;
    }
    if (modes & CMODE_M) {
	*end++ = 'm';
	c->mode &= ~CMODE_M;
    }
    if (modes & CMODE_N) {
	*end++ = 'n';
	c->mode &= ~CMODE_N;
    }
    if (modes & CMODE_P) {
	*end++ = 'p';
	c->mode &= ~CMODE_P;
    }
    if (modes & CMODE_S) {
	*end++ = 's';
	c->mode &= ~CMODE_S;
    }
    if (modes & CMODE_T) {
	*end++ = 't';
	c->mode &= ~CMODE_T;
    }

    if (c->key && (ci->mlock_off & CMODE_K)) {
	*end++ = 'k';
	newkey = sstrdup(c->key);
	free(c->key);
	c->key = NULL;
	set_key = 1;
    }
    if (c->limit && (ci->mlock_off & CMODE_L)) {
	*end++ = 'l';
	c->limit = 0;
    }

    if (end[-1] == '-')
	--end;

    if (end == newmodes)
	return;
    *end = 0;
    if (set_limit) {
	if (set_key)
	    send_cmd(s_ChanServ, "MODE %s %s %d :%s", c->name,
				newmodes, newlimit, newkey ? newkey : "");
	else
	    send_cmd(s_ChanServ, "MODE %s %s %d", c->name, newmodes, newlimit);
    } else if (set_key)
	send_cmd(s_ChanServ, "MODE %s %s :%s", c->name,
				newmodes, newkey ? newkey : "");
    else
	send_cmd(s_ChanServ, "MODE %s %s", c->name, newmodes);

    if (newkey && !c->key)
	free(newkey);
}

/*************************************************************************/

/* Check whether a user is allowed to be opped on a channel; if they
 * aren't, deop them.  If serverop is 1, the +o was done by a server.
 * Return 1 if the user is allowed to be opped, 0 otherwise. */

int check_valid_op(User *user, const char *chan, int serverop)
{
    ChannelInfo *ci = cs_findchan(chan);

    if (!ci || (ci->flags & CI_LEAVEOPS))
	return 1;

#ifdef IRCOP_OVERRIDE
    if (is_oper(user->nick))
	return 1;
#endif

    if (!is_chanop(user->nick, chan))
        return 0;

    if (ci->flags & CI_SUSPENDED && !is_services_op(user->nick)) {
	notice(s_ChanServ, user->nick,
		"This channel has been suspended, no ops will be allowed.");
	change_cmode(s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (ci->flags & CI_VERBOTEN && !is_oper(user->nick)) {
	/* check_kick() will get them out; we needn't explain. */
	change_cmode(s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (serverop && get_access(user, ci) <= 0) {
	notice(s_ChanServ, user->nick,
		"This channel has been registered with %s.", s_ChanServ);
	change_cmode(s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (get_access(user, ci) <= (ci->flags & CI_SECUREOPS ? 0 : ci->cmd_access[CA_AUTODEOP])) {
	change_cmode(s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    return 1;
}

/*************************************************************************/

/* Check whether a user is allowed to be voiced on a channel; if they
 * aren't, devoice them.  If serverop is 1, the +v was done by a server.
 * Return 1 if the user is allowed to be voiced, 0 otherwise. */

int check_valid_voice(User *user, const char *chan, int serverop)
{
    ChannelInfo *ci = cs_findchan(chan);

    if (!ci || (ci->flags & CI_LEAVEOPS))
	return 1;

#ifdef IRCOP_OVERRIDE
    if (is_oper(user->nick))
	return 1;
#endif

    if (!is_voiced(user->nick, chan))
        return 0;

    if (ci->flags & CI_SUSPENDED && !is_oper(user->nick)) {
	notice(s_ChanServ, user->nick,
		"This channel has been suspended, no voices will be allowed.");
	change_cmode(s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    if (serverop && get_access(user, ci) <= 0) {
	notice(s_ChanServ, user->nick,
		"This channel has been registered with %s.", s_ChanServ);
	change_cmode(s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    if (get_access(user, ci) <= (ci->flags & CI_SECUREOPS ? 0 : ci->cmd_access[CA_AUTODEOP])) {
	change_cmode(s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    return 1;
}

/*************************************************************************/

/* Check whether a user should be opped on a channel, and if so, do it.
 * Return 1 if the user was opped, 0 otherwise.  (Updates the channel's
 * last used time if the user was opped.) */

int check_should_op(User *user, const char *chan)
{
    ChannelInfo *ci = cs_findchan(chan);
    NickInfo *ni;

    if (!ci || ci->flags & CI_VERBOTEN || *chan == '+')
	return 0;

    if (ci->flags & CI_SECURE)
	if (!(ni = findnick(user->nick)) || !(ni->flags & NI_IDENTIFIED))
	    return 0;

    if (check_access(user, ci, CA_AUTOVOICE)) {
	if (check_access(user, ci, CA_AUTOOP))
	    change_cmode(s_ChanServ, chan, "+o", user->nick);
	else
	    change_cmode(s_ChanServ, chan, "+v", user->nick);
	ci->last_used = time(NULL);
	return 1;
    }

    return 0;
}

/*************************************************************************/

/* Check whether a user is permitted to be on a channel.  If so, return 0;
 * else, kickban the user with an appropriate message (could be either
 * AKICK or restricted access) and return 1.
 */

int check_kick(User *user, const char *chan)
{
    ChannelInfo *ci = cs_findchan(chan);
    AutoKick *akick;
    int i, bad;
    NickInfo *ni;
    char *av[3];

    if (!ci)
	return 0;

#ifdef IRCOP_OVERRIDE
    if (is_oper(user->nick))
	return 0;
#endif

    if (ci->flags & CI_VERBOTEN) {
	change_cmode(s_ChanServ, chan, "+b", create_mask(user));
	kick_user(s_ChanServ, chan, user->nick, "This channel may not be used.");
	return 1;
    }

    if (ci->flags & CI_SUSPENDED)
	return 0;

    ni = findnick(user->nick);
    if ((ni == NULL) || !(ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED)))
	ni = NULL;

    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
	if ((akick->is_nick > 0 && ni && stricmp(akick->name, user->nick) == 0) ||
		(!akick->is_nick && match_usermask(akick->name, user))) {
	    change_cmode(s_ChanServ, chan, "+b", akick->is_nick ? create_mask(user) : akick->name);
	    kick_user(s_ChanServ, chan, user->nick,
			akick->reason ? akick->reason : DEF_AKICK_REASON);
	    return 1;
	}

    if (!(ci->flags & CI_RESTRICTED))
	return 0;

    if (get_access(user, ci) <= (ci->flags&CI_SECUREOPS ? 0 : ci->cmd_access[CA_AUTODEOP])) {
	change_cmode(s_ChanServ, chan, "+b", create_mask(user));
	kick_user(s_ChanServ, chan, user->nick,
		"You are not permitted to be on this channel.");
	return 1;
    }

    return 0;
}

/*************************************************************************/

/* Record the current channel topic in the ChannelInfo structure. */

void record_topic(const char *chan)
{
    if(services_level==1) {
	Channel *c = findchan(chan);
	ChannelInfo *ci = cs_findchan(chan);

	if (!c || !ci || (ci->flags & CI_SUSPENDED))
	    return;
	if (ci->last_topic)
	    free(ci->last_topic);
	if (c->topic)
	    ci->last_topic = sstrdup(c->topic);
	else
	    ci->last_topic = NULL;
	strscpy(ci->last_topic_setter, c->topic_setter, NICKMAX);
	ci->last_topic_time = c->topic_time;
    }
}

/*************************************************************************/

/* Join channel with JOIN on and op self */
void do_cs_join(const char *chan)
{
    ChannelInfo *ci = cs_findchan(chan);
    if (ci && (ci->flags & CI_JOIN)) {
	send_cmd(s_ChanServ, "JOIN %s", chan);
	send_cmd(s_ChanServ, "MODE %s +o %s", chan, s_ChanServ);
    }
}

/* Join channel with JOIN on and op self */
void do_cs_part(const char *chan)
{
    ChannelInfo *ci = cs_findchan(chan);
    if (ci && (ci->flags & CI_JOIN))
	send_cmd(s_ChanServ, "PART %s", chan);
}

/* Join channel with JOIN on and op self */
void do_cs_reop(const char *chan)
{
    ChannelInfo *ci = cs_findchan(chan);
    if (ci && (ci->flags & CI_JOIN))
	send_cmd(s_ChanServ, "MODE %s +o %s", chan, s_ChanServ);
}

int do_cs_revenge(const char *chan, const char *actioner, const char *actionee,
	int action) {

    ChannelInfo *ci;
    User *user;
    int i;

    if ((ci = cs_findchan(chan)) && (user = finduser(actioner)) &&
					!is_services_nick(actioner)) {
	int setlev = 0, actlev = -1;

	/* Find out the level of the ACTIONER (setlev) */
	setlev = get_access(user, ci);

	/* Find out the level of the ACTIONEE (actlev) */
	user = finduser(actionee);
	actlev = get_access(user, ci);

	/* Unban if the ACTIONEE is HIGHER or EQUAL TO the ACTIONER */
	if(debug)
	    log("debug: actionee level is %d (%s), actioner is %d (%s)",
					actlev, actionee, setlev, actioner);
	if (actlev > setlev) {
	    if (get_revenge_level(ci))
		do_revenge(chan, actioner, actionee, action);
	    return 1;
	}
    }
    return 0;
}
void do_revenge(const char *chan, const char *actioner, const char *actionee,
	int action) {

    char buf[25+NICKMAX];
    ChannelInfo *ci = cs_findchan(chan);
    User *user = finduser(actioner);

    if (!ci || !user || action==CR_NONE) return;

#ifdef IRCOP_OVERRIDE
    if (is_oper(actioner)) return;
#endif

    if (action == CR_DEOP)
	snprintf(buf, sizeof(buf), "\2REVENGE\2 - Do not deop %s", actionee);
    else if (action == CR_KICK)
	snprintf(buf, sizeof(buf), "\2REVENGE\2 - Do not kick %s", actionee);
    else if (action == CR_NICKBAN || action == CR_USERBAN ||
  		  					action == CR_HOSTBAN)
	snprintf(buf, sizeof(buf), "\2REVENGE\2 - Do not ban %s", actionee);

    if (get_revenge_level(ci)==CR_DEOP)
	change_cmode(s_ChanServ, chan, "-o", actioner);

    else if (get_revenge_level(ci)==CR_KICK)
	kick_user(s_ChanServ, ci->name, actioner, buf);

    else if (get_revenge_level(ci)==CR_NICKBAN) {
	char mask[strlen(user->nick)+5];
	snprintf(mask, sizeof(mask), "%s!*@*", user->nick);
	change_cmode(s_ChanServ, chan, "+b", mask);
	kick_user(s_ChanServ, ci->name, actioner, buf);

    } else if (get_revenge_level(ci)==CR_USERBAN) {
	change_cmode(s_ChanServ, chan, "+b", create_mask(user));
	kick_user(s_ChanServ, ci->name, actioner, buf);

    } else if (get_revenge_level(ci)==CR_HOSTBAN) {
	char mask[strlen(user->host)+5];
	snprintf(mask, sizeof(mask), "*!*@%s", user->host);
	change_cmode(s_ChanServ, chan, "+b", mask);
	kick_user(s_ChanServ, ci->name, actioner, buf);

    } else if (get_revenge_level(ci)==CR_MIRROR) {

	if (action == CR_DEOP)
	    change_cmode(s_ChanServ, chan, "-o", actioner);

	else if (action == CR_KICK)
	    kick_user(s_ChanServ, ci->name, actioner, buf);

	else if (action == CR_NICKBAN) {
	    char mask[strlen(user->nick)+5];
	    snprintf(mask, sizeof(mask), "%s!*@*", user->nick);
	    change_cmode(s_ChanServ, chan, "+b", mask);
	    kick_user(s_ChanServ, ci->name, actioner, buf);

	} else if (action == CR_USERBAN) {
	    change_cmode(s_ChanServ, chan, "+b", create_mask(user));
	    kick_user(s_ChanServ, ci->name, actioner, buf);

	} else if (action == CR_HOSTBAN) {
	    char mask[strlen(user->host)+5];
	    snprintf(mask, sizeof(mask), "*!*@%s", user->host);
	    change_cmode(s_ChanServ, chan, "+b", mask);
	    kick_user(s_ChanServ, ci->name, actioner, buf);
	}
    }
}


/*************************************************************************/

/* Restore the topic in a channel when it's created, if we should. */

void restore_topic(const char *chan)
{
    Channel *c = findchan(chan);
    ChannelInfo *ci = cs_findchan(chan);

    if (!c || !ci || !(ci->flags & (CI_KEEPTOPIC | CI_SUSPENDED)))
	return;
    if (c->topic)
	free(c->topic);
    if (ci->last_topic) {
	if (ci->flags & CI_SUSPENDED) {
	    char tmp[strlen(ci->last_topic)+30];
	    snprintf(tmp, sizeof(tmp), "[\2SUSPENDED\2] %s [\2SUSPENDED\2]", ci->last_topic);
	    c->topic = sstrdup(tmp);
	} else
	    c->topic = sstrdup(ci->last_topic);
	strscpy(c->topic_setter, ci->last_topic_setter, NICKMAX);
	c->topic_time = ci->last_topic_time;
    } else {
	c->topic = NULL;
	strscpy(c->topic_setter, s_ChanServ, NICKMAX);
    }
#ifdef IRC_CLASSIC
    send_cmd(s_ChanServ, "TOPIC %s %lu :%s", chan,
		c->topic_setter,  c->topic ? c->topic : "");
#else
    send_cmd(s_ChanServ, "TOPIC %s %s %lu :%s", chan,
		c->topic_setter, c->topic_time, c->topic ? c->topic : "");
#endif
}

/*************************************************************************/

/* See if the topic is locked on the given channel, and return 1 (and fix
 * the topic) if so. */

int check_topiclock(const char *chan)
{
    Channel *c = findchan(chan);
    ChannelInfo *ci = cs_findchan(chan);

    if (!c || !ci || !(ci->flags & (CI_TOPICLOCK | CI_SUSPENDED)))
	return 0;
    if (c->topic)
	free(c->topic);
    if (ci->last_topic) {
	if (ci->flags & CI_SUSPENDED) {
	    char tmp[strlen(ci->last_topic)+30];
	    snprintf(tmp, sizeof(tmp), "[\2SUSPENDED\2] %s [\2SUSPENDED\2]", ci->last_topic);
	    c->topic = sstrdup(tmp);
	} else
	    c->topic = sstrdup(ci->last_topic);
    } else
	c->topic = NULL;
    strscpy(c->topic_setter, ci->last_topic_setter, NICKMAX);
    c->topic_time = ci->last_topic_time;
    send_cmd(s_ChanServ, "TOPIC %s %s %lu :%s", chan,
		c->topic_setter, c->topic_time, c->topic ? c->topic : "");
}

/*************************************************************************/

/* Remove all channels which have expired. */
void expire_chans()
{
    ChannelInfo *ci;
    int i;
    const time_t expire_time = CHANNEL_EXPIRE*24*60*60;
    time_t now = time(NULL);

    for (i = 33; i < 256; ++i) {
	ci = chanlists[i];
	while (ci) {
	    if (now - ci->last_used >= expire_time
			&& !(ci->flags & (CI_VERBOTEN | CI_SUSPENDED))) {
		ChannelInfo *ci2;
		if (ci->prev) {
		    ci2 = ci->prev;
		    if (ci->next)
			ci2->next = ci->next;
		    else
			ci2->next = NULL;
		} else
		    ci2 = NULL;
		log("Expiring channel %s", ci->name);
		delchan(ci);
		ci = ci2;
	    }
	    if (ci) ci = ci->next;
	}
    }
}

/*************************************************************************/
/*********************** ChanServ private routines ***********************/
/*************************************************************************/

/* Return the ChannelInfo structure for the given channel, or NULL if the
 * channel isn't registered. */

ChannelInfo *cs_findchan(const char *chan)
{
    ChannelInfo *ci;

    for (ci = chanlists[tolower(chan[1])]; ci; ci = ci->next)
	if (stricmp(ci->name, chan) == 0)
	    return ci;
    return NULL;
}

/*************************************************************************/

/* Insert a channel alphabetically into the database. */

static void alpha_insert_chan(ChannelInfo *ci)
{
    ChannelInfo *ci2, *ci3;
    char *chan = ci->name;

    for (ci3 = NULL, ci2 = chanlists[tolower(chan[1])];
			ci2 && stricmp(ci2->name, chan) < 0;
			ci3 = ci2, ci2 = ci2->next)
	;
    ci->prev = ci3;
    ci->next = ci2;
    if (!ci3)
	chanlists[tolower(chan[1])] = ci;
    else
	ci3->next = ci;
    if (ci2)
	ci2->prev = ci;
}

/*************************************************************************/

/* Add a channel to the database.  Returns a pointer to the new ChannelInfo
 * structure if the channel was successfully registered, NULL otherwise.
 * Assumes channel does not already exist. */

static ChannelInfo *makechan(const char *chan)
{
    ChannelInfo *ci;
    Channel *c;
    char *s, *end;

    ci = scalloc(sizeof(ChannelInfo), 1);
    strscpy(ci->name, chan, CHANMAX);
    ci->time_registered = time(NULL);
    alpha_insert_chan(ci);
    return ci;
}

/*************************************************************************/

/* Remove a channel from the ChanServ database.  Return 1 on success, 0
 * otherwise. */

int delchan(ChannelInfo *ci)
{
    int i;

#ifdef NEWS
    if (services_level==1) {
	NewsList *nl;
	if (nl = find_newslist(ci->name))
	    del_newslist(nl);
    }
#endif /* NEWS */

    do_cs_part(ci->name);
    if (ci->next)
	ci->next->prev = ci->prev;
    if (ci->prev)
	ci->prev->next = ci->next;
    else
	chanlists[tolower(ci->name[1])] = ci->next;
    if (ci->desc)
	free(ci->desc);
#if FILE_VERSION > 2
    if (ci->url)
	free(ci->url);
#endif
    if (ci->mlock_key)
	free(ci->mlock_key);
    if (ci->last_topic)
	free(ci->last_topic);
    for (i = 0; i < ci->accesscount; ++i)
	free(ci->access[i].name);
    if (ci->access)
	free(ci->access);
    for (i = 0; i < ci->akickcount; ++i) {
	free(ci->akick[i].name);
	if (ci->akick[i].reason)
	    free(ci->akick[i].reason);
    }
    if (ci->akick)
	free(ci->akick);
    if (ci->cmd_access)
	free(ci->cmd_access);
    free(ci);
    return 1;
}

/*************************************************************************/

/* Does the given user have founder access to the channel? */

int is_founder(User *user, NickInfo *ni, ChannelInfo *ci)
{
    struct u_chaninfolist *c;

    if(ci->flags & CI_SUSPENDED)
	if(is_services_op(user->nick))
	    return 1;
	else
	    return 0;
	
    if (ni && ((ni->flags & NI_IDENTIFIED) ||
		((ni->flags & NI_RECOGNIZED) && !(ci->flags & CI_SECURE)))
	   && stricmp(user->nick, ci->founder) == 0)
	return 1;
    return is_identified(user, ci);
}

int is_justfounder(NickInfo *ni, ChannelInfo *ci)
{
    struct u_chaninfolist *c;

    if(ci->flags & CI_SUSPENDED)
	    return 0;
	
    if (ni && stricmp(ni->nick, ci->founder) == 0)
	return 1;
    return 0;
}

/*************************************************************************/

/* Has the given user password-identified as founder for the channel? */

static int is_identified(User *user, ChannelInfo *ci)
{
    struct u_chaninfolist *c;

    if(ci->flags & CI_SUSPENDED)
	if(is_services_op(user->nick))
	    return 1;
	else
	    return 0;
    for (c = user->founder_chans; c; c = c->next)
	if (c->chan == ci)
	    return 1;
    return 0;
}

/*************************************************************************/

/* Return the access level the given user has on the channel.  If the
 * channel doesn't exist, the user isn't on the access list, or the channel
 * is CS_SECURE and the user hasn't IDENTIFY'd with NickServ, return 0. */

int get_access(User *user, ChannelInfo *ci)
{
    NickInfo *ni = findnick(user->nick);
    ChanAccess *access;
    int i, highlev = ci->cmd_access[CA_FLOOR]-1;

    if (!ci)
	return 0;
    if(ci->flags & CI_SUSPENDED)
    	if(is_services_op(user->nick))
	    return ci->cmd_access[CA_FOUNDER];
	else if(is_oper(user->nick))
	    return ci->cmd_access[CA_AUTOVOICE];
	else
	    return 0;

    if (is_founder(user, ni, ci))
	return ci->cmd_access[CA_FOUNDER];
    for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	if (	(access->is_nick > 0 && ni &&
		((ni->flags & NI_IDENTIFIED) ||
		((ni->flags & NI_RECOGNIZED)  &&
		!(ci->flags & CI_SECURE))) &&
		stricmp(user->nick, access->name) == 0) ||
		(!access->is_nick && !ni &&
		match_usermask(access->name, user)))
	    if (access->level > highlev) highlev = access->level;
    if (highlev > ci->cmd_access[CA_FLOOR]-1) return highlev;
    return 0;
}

int check_access (User *user, ChannelInfo *ci, int what)
{
    if (get_access(user, ci) >= ci->cmd_access[what])
	return 1;
    return 0;
}

/* Return the access level the given user has on the channel.  If the
 * channel doesn't exist, the user isn't on the access list, return 0 */

int get_justaccess(const char *name, ChannelInfo *ci)
{
    NickInfo *ni = findnick(name);
    ChanAccess *access;
    int i, highlev = ci->cmd_access[CA_FLOOR]-1;

    if (!ci)
	return 0;
    if(ci->flags & CI_SUSPENDED)
	return 0;
    if (is_justfounder(ni, ci))
	return ci->cmd_access[CA_FOUNDER];
    for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	if (	(access->is_nick > 0 && ni &&
		stricmp(name, access->name) == 0) ||
	    	(!access->is_nick && !ni &&
	    	match_wild_nocase(access->name, name)))
	    if (access->level > highlev) highlev = access->level;
    if (highlev > ci->cmd_access[CA_FLOOR]-1) return highlev;
    return 0;
}


int check_justaccess (const char *name, ChannelInfo *ci, int what)
{
    if (get_justaccess(name, ci) >= ci->cmd_access[what])
	return 1;
    return 0;
}

/*************************************************************************/
/*********************** ChanServ command routines ***********************/
/*************************************************************************/

static void do_help(const char *source)
{
    char *cmd = strtok(NULL, "");
    char buf[BUFSIZE];

    if (cmd && is_services_op(source)) {
	Hash_HELP *command, hash_table[] = {
		{ "DROP",	H_SOP,	oper_drop_help },
		{ "GETPASS",	H_SOP,	getpass_help },
		{ "FORBID",	H_SOP,	forbid_help },
		{ "SUSPEND",	H_SOP,	suspend_help },
		{ "UNSUSPEND",	H_SOP,	unsuspend_help },
		{ NULL }
	};

	if (command = get_help_hash(source, cmd, hash_table))
	    notice_list(s_ChanServ, source, command->process);
	else {
	    snprintf(buf, BUFSIZE, "%s%s", s_ChanServ, cmd ? " " : "");
	    strscpy(buf+strlen(buf), cmd ? cmd : "", sizeof(buf)-strlen(buf));
	    helpserv(s_ChanServ, source, buf);
	}
    } else {
	snprintf(buf, BUFSIZE, "%s%s", s_ChanServ, cmd ? " " : "");
	strscpy(buf+strlen(buf), cmd ? cmd : "", sizeof(buf)-strlen(buf));
	helpserv(s_ChanServ, source, buf);
    }
}

/*************************************************************************/

static void do_register(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");
    char *desc = strtok(NULL, "");
    NickInfo *ni = findnick(source);
    ChannelInfo *ci;
    User *u = finduser(source);
    struct u_chaninfolist *c;

  if(services_level!=1) {
    notice(s_ChanServ, source,
		"Sorry, channel registration is temporarily disabled.");
    return;
  }

    if (!desc) {
	notice(s_ChanServ, source,
		"Syntax: \2REGISTER \37channel\37 \37password\37 \37description\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP REGISTER\2 for more information.", s_ChanServ);

    } else if (*chan == '&')
	notice(s_ChanServ, source, "Local channels cannot be registered.");

    else if (chan[1] == 0)
	notice(s_ChanServ, source, "\2%s\2 is not a valid channel name.", chan);

    else if (!ni) {
	notice(s_ChanServ, source, "Your must register your nickname first.");
	notice(s_ChanServ, source,
		"\2/msg %s HELP\2 for information on registering nicknames.",
		s_NickServ);

    } else if (ci = cs_findchan(chan))
	if (ci->flags & CI_VERBOTEN) {
	    log("%s: Attempt to register FORBIDden channel %s by %s!%s@%s",
			s_ChanServ, chan, source, u->username, u->host);
	    notice(s_ChanServ, source,
			"Channel \2%s\2 may not be registered.", chan);
	} else
	    notice(s_ChanServ, source,
			"Channel \2%s\2 already registered!", chan);

    else if (!u) {
	log("%s: Attempt to register channel %s from nonexistent nick %s",
						s_ChanServ, chan, source);
	notice(s_ChanServ, source, "Sorry, registration failed.");

    } else if (!is_chanop(source, chan))
	notice(s_ChanServ, source,
		"You must be a channel operator to register the channel.");

    else if (!(ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED)))
	notice(s_ChanServ, source,
		"You must identify to %s first", s_NickServ);

    else {
	if (ci = makechan(chan)) {
	    Channel *c;
	    if (!(c = findchan(chan))) {
		log("%s: Channel %s not found for REGISTER", s_ChanServ, chan);
		notice(s_ChanServ, source, "Sorry, registration failed.");
		return;
	    }
	    ci->last_used = ci->time_registered;
	    strscpy(ci->founder, source, NICKMAX);
	    strscpy(ci->founderpass, pass, PASSMAX);
	    ci->desc = sstrdup(desc);
#if FILE_VERSION > 2
	    ci->url = sstrdup("");
#endif
	    if (c->topic) {
		ci->last_topic = sstrdup(c->topic);
		strscpy(ci->last_topic_setter, c->topic_setter, NICKMAX);
		ci->last_topic_time = c->topic_time;
	    }
	    ci->flags = CI_KEEPTOPIC;
	    ci->cmd_access = def_access;
	    log("%s: Channel %s registered by %s!%s@%s", s_ChanServ, chan,
			source, u->username, u->host);
	    notice(s_ChanServ, source,
			"Channel %s registered under your nickname: %s",
			chan, source);
	    notice(s_ChanServ, source,
			"Your channel password is \2%s\2 - remember it for "
			"later use.", ci->founderpass);
	} else
	    notice(s_ChanServ, source, "Sorry, registration failed.");
	c = smalloc(sizeof(*c));
	c->next = u->founder_chans;
	c->prev = NULL;
	if (u->founder_chans)
	    u->founder_chans->prev = c;
	u->founder_chans = c;
	c->chan = ci;
    }
}

/*************************************************************************/

static void do_identify(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *pass = strtok(NULL, " ");
    ChannelInfo *ci;
    struct u_chaninfolist *c;
    User *u = finduser(source);

    if (!pass) {
	notice(s_ChanServ, source,
		"Syntax: \2IDENTIFY %s \37password\37\2",
		chan ? chan : "\37channel\37");
	notice(s_ChanServ, source,
		"\2/msg %s HELP IDENTIFY\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!u) {
	log("%s: IDENTIFY from nonexistent nick %s for %s",
						s_ChanServ, source, chan);
	notice(s_ChanServ, source, "Sorry, identification failed.");

    } else {

        if(!(ci->flags & CI_SUSPENDED)) {
	    if (strcmp(pass, ci->founderpass) == 0) {
		if (!is_identified(u, ci)) {
		    c = smalloc(sizeof(*c));
		    c->next = u->founder_chans;
		    c->prev = NULL;
		    if (u->founder_chans)
			u->founder_chans->prev = c;
		    u->founder_chans = c;
		    c->chan = ci;
		    log("%s: %s!%s@%s identified for %s", s_ChanServ,
			source, u->username, u->host, chan);
		}
		notice(s_ChanServ, source,
		    "Password accepted - you now have founder-level access to %s.",
		    chan);
	    } else {
		log("%s: Failed IDENTIFY for %s by %s!%s@%s",
			s_ChanServ, chan, source, u->username, u->host);
		notice(s_ChanServ, source, "Password incorrect.");
	    }
	} else
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
    }
}

/*************************************************************************/

static void do_drop(const char *source)
{
    char *chan = strtok(NULL, " ");
    ChannelInfo *ci;
    User *u = finduser(source);
    int is_servop = is_services_op(source);

  if(services_level!=1)
    if (!is_servop) {
	notice(s_ChanServ, source,
		"Sorry, channel de-registration is temporarily disabled.");
	return;
    }

    if (!chan) {
	notice(s_ChanServ, source, "Syntax: \2DROP \37channel\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP DROP\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!is_servop && (!u || !is_identified(u, ci))) {
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else {
	    notice(s_ChanServ, source,
		"Password authentication required for that command.");
	    notice(s_ChanServ, source,
		"Retry after typing \2/msg %s IDENTIFY %s <password>.",
		s_ChanServ, chan);
	}

    } else if (is_servop && !u) {
	log("%s: DROP %s from nonexistent oper %s", s_ChanServ, chan, source);
	notice(s_ChanServ, source, "Can't find your user record!");

    } else {

  if(services_level!=1)
	notice(s_ChanServ, source,
		"Warning: Services is in read-only mode; changes will not be saved.");
	delchan(ci);
	log("%s: Channel %s dropped by %s!%s@%s", s_ChanServ, chan,
			source, u->username, u->host);
	notice(s_ChanServ, source, "Channel %s has been dropped.", chan);

    }
}

/*************************************************************************/

/* Main SET routine.  Calls other routines as follows:
 *	do_set_command(User *command-sender, ChannelInfo *ci, char *param);
 * Additional parameters can be retrieved using strtok(NULL, toks).
 * (Exception is do_set_held(), which takes only source nick and channel
 * name.)
 */
static void do_set(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *cmd  = strtok(NULL, " ");
    char *param;
    ChannelInfo *ci;
    User *u;
    int i;

  if(services_level!=1) {
    notice(s_ChanServ, source,
		"Sorry, channel option setting is temporarily disabled.");
    return;
  }

    if (cmd)
	if (stricmp(cmd, "DESC") == 0 || stricmp(cmd, "TOPIC") == 0)
	    param = strtok(NULL, "");
	else
	    param = strtok(NULL, " ");
    else
	param = NULL;

    if (!param) {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s \37option\37 \37parameters\37\2",
		chan ? chan : "\37channel\37");
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!(u = finduser(source)) || !check_access(u, ci, CA_SET))
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else if (stricmp(cmd, "FOUNDER") == 0)
	if (is_founder(u, findnick(u->nick), ci))
	     if(is_identified(u, ci))
		    do_set_founder(u, ci, param);
	     else {
		notice(s_ChanServ, source,
		    "Password authentication required for that command.");
		notice(s_ChanServ, source,
		    "Retry after typing \2/msg ChanServ IDENTIFY %s \37password\37.",
		    chan);
	     }
	else
	    notice(s_ChanServ, source, "Access denied.");

    else if (stricmp(cmd, "PASSWORD") == 0)
	if (is_founder(u, findnick(u->nick), ci))
	     if(is_identified(u, ci))
		do_set_password(u, ci, param);
	     else {
		notice(s_ChanServ, source,
		    "Password authentication required for that command.");
		notice(s_ChanServ, source,
		    "Retry after typing \2/msg ChanServ IDENTIFY %s \37password\37.",
		    chan);
	     }
	else
	    notice(s_ChanServ, source, "Access denied.");

    else {
	Hash_CI *command, hash_table[] = {
		{ "DESC*",	H_NONE,	do_set_desc },
		{ "TOPIC",	H_NONE,	do_set_topic },
		{ "M*LOCK",	H_NONE,	do_set_mlock },
		{ "KEEPTOPIC",	H_NONE,	do_set_keeptopic },
		{ "T*LOCK",	H_NONE,	do_set_topiclock },
		{ "PRIV*",	H_NONE,	do_set_private },
		{ "SEC*OPS",	H_NONE,	do_set_secureops },
		{ "RESTRICT*",	H_NONE,	do_set_restricted },
		{ "SEC*",	H_NONE,	do_set_secure },
#if FILE_VERSION > 2
		{ "U*R*L",	H_NONE,	do_set_url },
		{ "W*W*W",	H_NONE,	do_set_url },
		{ "HOME*",	H_NONE,	do_set_url },
		{ "*PAGE",	H_NONE,	do_set_url },
#endif
		{ "JOIN",	H_NONE,	do_set_join },
		{ "REV*",	H_NONE,	do_set_revenge },
		{ NULL }
	};

	if (command = get_ci_hash(source, cmd, hash_table))
	    (*command->process)(u, ci, param);
	else
	    notice(s_ChanServ, source,
			"Unknown SET option \2%s\2.", strupper(cmd));
    }
}

/*************************************************************************/

static void do_set_founder(User *u, ChannelInfo *ci, char *param)
{
    NickInfo *ni = findnick(param);

    if (!ni) {
	notice(s_ChanServ, u->nick, "%s isn't a registered nickname.", param);
	return;
    }
    strscpy(ci->founder, param, NICKMAX);
    notice(s_ChanServ, u->nick,
		"Founder of %s changed to \2%s\2.", ci->name, ci->founder);
}

/*************************************************************************/

static void do_set_password(User *u, ChannelInfo *ci, char *param)
{
    strscpy(ci->founderpass, param, PASSMAX);
    notice(s_ChanServ, u->nick,
		"%s password changed to \2%s\2.", ci->name, ci->founderpass);
}

/*************************************************************************/

static void do_set_desc(User *u, ChannelInfo *ci, char *param)
{
    free(ci->desc);
    ci->desc = sstrdup(param);
    notice(s_ChanServ, u->nick,
		"Description of %s changed to \2%s\2.", ci->name, param);
}

/*************************************************************************/

#if FILE_VERSION > 2
static void do_set_url(User *u, ChannelInfo *ci, char *param)
{
    free(ci->url);
    if(stricmp(param, "NONE") == 0) {
	ci->url = sstrdup("");
	notice(s_ChanServ, u->nick,
		"World Wide Web Page (URL) of %s removed.", ci->name);
    } else {
	ci->url = sstrdup(param);
	notice(s_ChanServ, u->nick,
		"World Wide Web Page (URL) of %s changed to \2%s\2.", ci->name, param);
    }
}
#endif

/*************************************************************************/

static void do_set_topic(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;
    Channel *c = findchan(chan);

    if (!c) {
	log("%s: SET TOPIC for %s from %s: channel not found!",
		s_ChanServ, chan, source);
	notice(s_ChanServ, source, "Sorry, can't set topic.");
	return;
    }
    if (ci->last_topic)
	free(ci->last_topic);
    if (*param)
	ci->last_topic = sstrdup(param);
    else
	ci->last_topic = NULL;
    if (c->topic) {
	free(c->topic);
	--c->topic_time;
    } else
	c->topic_time = time(NULL);
    if (*param)
	c->topic = sstrdup(param);
    else
	c->topic = NULL;
    strscpy(ci->last_topic_setter, source, NICKMAX);
    strscpy(c->topic_setter, source, NICKMAX);
    ci->last_topic_time = c->topic_time;
    send_cmd(s_ChanServ, "TOPIC %s %s %lu :%s",
		chan, source, c->topic_time, param);
}

/*************************************************************************/

static void do_set_mlock(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;
    char *s, modebuf[32], *end, c;
    int add = -1;	/* 1 if adding, 0 if deleting, -1 if neither */

    ci->mlock_on = ci->mlock_off = ci->mlock_limit = 0;
    if (ci->mlock_key) {
	free(ci->mlock_key);
	ci->mlock_key = NULL;
    }

    while (*param) {
	if (*param != '+' && *param != '-' && add < 0) {
	    ++param;
	    continue;
	}
	switch (tolower((c = *param++))) {
	case '+':
	    add = 1; break;
	case '-':
	    add = 0; break;
	case 'i':
	    if (add)
		ci->mlock_on |= CMODE_I;
	    else
		ci->mlock_off |= CMODE_I;
	    break;
	case 'k':
	    if (add) {
		if (!(s = strtok(NULL, " "))) {
		    notice(s_ChanServ, source,
				"Parameter required for MLOCK +k.");
		    break;
		}
		ci->mlock_key = sstrdup(s);
	    } else {
		if (ci->mlock_key) {
		    free(ci->mlock_key);
		    ci->mlock_key = NULL;
		}
		ci->mlock_off |= CMODE_K;
	    }
	    break;
	case 'l':
	    if (add) {
		if (!(s = strtok(NULL, " "))) {
		    notice(s_ChanServ, source,
				"Parameter required for MLOCK +l.");
		    break;
		}
		if (atol(s) <= 0) {
		    notice(s_ChanServ, source,
			"Parameter for MLOCK +l must be a positive number.");
		    break;
		}
		ci->mlock_limit = atol(s);
	    } else
		ci->mlock_off |= CMODE_L;
	    break;
	case 'm':
	    if (add)
		ci->mlock_on |= CMODE_M;
	    else
		ci->mlock_off |= CMODE_M;
	    break;
	case 'n':
	    if (add)
		ci->mlock_on |= CMODE_N;
	    else
		ci->mlock_off |= CMODE_N;
	    break;
	case 'p':
	    if (add)
		ci->mlock_on |= CMODE_P;
	    else
		ci->mlock_off |= CMODE_P;
	    break;
	case 's':
	    if (add)
		ci->mlock_on |= CMODE_S;
	    else
		ci->mlock_off |= CMODE_S;
	    break;
	case 't':
	    if (add)
		ci->mlock_on |= CMODE_T;
	    else
		ci->mlock_off |= CMODE_T;
	    break;
	default:
	    notice(s_ChanServ, source,
			"Unknown mode character %c ignored.", c);
	} /* switch */
    } /* while (*param) */

    end = modebuf;
    *end = 0;
    if (ci->mlock_on)
	end += snprintf(end, sizeof(modebuf), "+%s%s%s%s%s%s%s%s",
				(ci->mlock_on & CMODE_I) ? "i" : "",
				(ci->mlock_key         ) ? "k" : "",
				(ci->mlock_limit       ) ? "l" : "",
				(ci->mlock_on & CMODE_M) ? "m" : "",
				(ci->mlock_on & CMODE_N) ? "n" : "",
				(ci->mlock_on & CMODE_P) ? "p" : "",
				(ci->mlock_on & CMODE_S) ? "s" : "",
				(ci->mlock_on & CMODE_T) ? "t" : "");
    if (ci->mlock_off)
	end += snprintf(end, sizeof(modebuf), "-%s%s%s%s%s%s%s%s",
				(ci->mlock_off & CMODE_I) ? "i" : "",
				(ci->mlock_off & CMODE_K) ? "k" : "",
				(ci->mlock_off & CMODE_L) ? "l" : "",
				(ci->mlock_off & CMODE_M) ? "m" : "",
				(ci->mlock_off & CMODE_N) ? "n" : "",
				(ci->mlock_off & CMODE_P) ? "p" : "",
				(ci->mlock_off & CMODE_S) ? "s" : "",
				(ci->mlock_off & CMODE_T) ? "t" : "");
    if (*modebuf)
	notice(s_ChanServ, source,
		"Mode lock on channel %s changed to \2%s\2.", chan, modebuf);
    else
	notice(s_ChanServ, source,
		"Mode lock on channel %s removed.", chan);
    check_modes(chan);
}

/*************************************************************************/

static void do_set_keeptopic(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_KEEPTOPIC;
	notice(s_ChanServ, source,
		"Topic retention option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_KEEPTOPIC;
	notice(s_ChanServ, source,
		"Topic retention option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s KEEPTOPIC {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET KEEPTOPIC\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_topiclock(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_TOPICLOCK;
	notice(s_ChanServ, source,
		"Topic lock option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_TOPICLOCK;
	notice(s_ChanServ, source,
		"Topic lock option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s TOPICLOCK {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET TOPICLOCK\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_private(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_PRIVATE;
	notice(s_ChanServ, source,
		"Private option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_PRIVATE;
	notice(s_ChanServ, source,
		"Private option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s PRIVATE {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET PRIVATE\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_secureops(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_SECUREOPS;
	notice(s_ChanServ, source,
		"Secure ops option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_SECUREOPS;
	notice(s_ChanServ, source,
		"Secure ops option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s SECUREOPS {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET SECUREOPS\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_restricted(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_RESTRICTED;
	notice(s_ChanServ, source,
		"Restricted access option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_RESTRICTED;
	notice(s_ChanServ, source,
		"Restricted access option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s RESTRICTED {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET RESTRICTED\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_secure(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ChanAccess *access;
	int i;

	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	    if (access->is_nick == 0) {
		notice(s_ChanServ, source,
			"SECURE channel access lists may only contain registered");
		notice(s_ChanServ, source,
			"nicknames, not nick!user@host masks.");
		notice(s_ChanServ, source,
			"\2/msg %s HELP SET SECURE\2 for more information.",
			s_ChanServ);
		return;
	    }
	ci->flags |= CI_SECURE;
	notice(s_ChanServ, source,
		"Secure option is now \2ON\2.");

    } else if (stricmp(param, "OFF") == 0) {
	ci->flags &= ~CI_SECURE;
	notice(s_ChanServ, source,
		"Secure option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s SECURE {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET SECURE\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_join(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp(param, "ON") == 0) {
	ci->flags |= CI_JOIN;
	notice(s_ChanServ, source,
		"Join option is now \2ON\2.");
	do_cs_join(ci->name);

    } else if (stricmp(param, "OFF") == 0) {
	do_cs_part(ci->name);
	ci->flags &= ~CI_JOIN;
	notice(s_ChanServ, source,
		"Join option is now \2OFF\2.");

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2SET %s JOIN {ON|OFF}\2", chan);
	notice(s_ChanServ, source,
		"\2/msg %s HELP SET JOIN\2 for more information.",
		s_ChanServ);
    }
}

/*************************************************************************/

static void do_set_revenge(User *u, ChannelInfo *ci, char *param)
{
    char *source = u->nick, *chan = ci->name;
    int i;

    for (i=0;revengeinfo[i].what>=0;++i)
        if (stricmp(param, revengeinfo[i].cmd)==0) {
	    if (revengeinfo[i].rev1==1)	ci->flags |= CI_REV1;
	    else			ci->flags &= ~CI_REV1;
	    if (revengeinfo[i].rev2==1)	ci->flags |= CI_REV2;
	    else			ci->flags &= ~CI_REV2;
	    if (revengeinfo[i].rev3==1)	ci->flags |= CI_REV3;
	    else			ci->flags &= ~CI_REV3;
	    notice(s_ChanServ, source,
		"Revenge set to \2%s\2 (%s).", revengeinfo[i].cmd,
			revengeinfo[i].desc);
	    break;
	}

    if (revengeinfo[i].what<0) 
	if (stricmp(param, "CURRENT")==0) {
	    for (i=0;revengeinfo[i].what!=get_revenge_level(ci) &&
					revengeinfo[i].what>=0;i++) ;
	    if (revengeinfo[i].what>=0)
		notice(s_ChanServ, source, "Revenge currently at \2%s\2 (%s).",
			revengeinfo[i].cmd, revengeinfo[i].desc);
	} else {
	    notice(s_ChanServ, source,
		"Syntax: \2SET %s REVENGE {CURRENT|level}\2", chan);
	    notice(s_ChanServ, source,
		"\2/msg %s HELP SET REVENGE\2 for more information.",
		s_ChanServ);
	}
}

/* This is one UUUUUUUGLY function. */

int get_revenge_level(ChannelInfo *ci)
{
    int i;

    for (i=0;revengeinfo[i].what>=0;i++)
	if (revengeinfo[i].rev1==1 && (ci->flags & CI_REV1)) {
	    if (revengeinfo[i].rev2==1 && (ci->flags & CI_REV2)) {
		if (revengeinfo[i].rev3==1 && (ci->flags & CI_REV3))
/*111*/		    return revengeinfo[i].what;
		else if (revengeinfo[i].rev3==0 && !(ci->flags & CI_REV3))
/*110*/		    return revengeinfo[i].what;
	    } else if (revengeinfo[i].rev2==0 && !(ci->flags & CI_REV2)) {
		if (revengeinfo[i].rev3==1 && (ci->flags & CI_REV3))
/*101*/		    return revengeinfo[i].what;
		else if (revengeinfo[i].rev3==0 && !(ci->flags & CI_REV3))
/*100*/		    return revengeinfo[i].what;
	    }
	} else if (revengeinfo[i].rev1==0 && !(ci->flags & CI_REV1)) {
	    if (revengeinfo[i].rev2==1 && (ci->flags & CI_REV2)) {
		if (revengeinfo[i].rev3==1 && (ci->flags & CI_REV3))
/*011*/		    return revengeinfo[i].what;
		else if (revengeinfo[i].rev3==0 && !(ci->flags & CI_REV3))
/*010*/		    return revengeinfo[i].what;
	    } else if (revengeinfo[i].rev2==0 && !(ci->flags & CI_REV2)) {
		if (revengeinfo[i].rev3==1 && (ci->flags & CI_REV3))
/*001*/		    return revengeinfo[i].what;
		else if (revengeinfo[i].rev3==0 && !(ci->flags & CI_REV3))
/*000*/		    return revengeinfo[i].what;
	    }
	}
    return 0;
}

/*************************************************************************/

static void do_level(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *cmd  = strtok(NULL, " ");
    ChannelInfo *ci;
    User *u;

    if (!cmd) {
	notice(s_ChanServ, source,
		"Syntax: \2LEVEL %s {SET|RESET|LIST} \2[\37\2command\2\37|\37all\37 [\37level\37]]\2",
		chan ? chan : "\37channel\37");
	notice(s_ChanServ, source,
		"\2/msg %s HELP LEVEL\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

#ifdef IRCOP_OVERRIDE
    else if (!(u = finduser(source)) || (!is_services_op(source) &&
#else
    else if (!(u = finduser(source)) || (
#endif
			get_access(u, ci) <= 0))
	notice(s_ChanServ, source, "Access denied.");

    else {
	Hash_CHAN *command, hash_table[] = {
		{ "LIST*",	H_NONE,	do_level_list },
		{ "VIEW",	H_NONE,	do_level_list },
		{ "DISP*",	H_NONE,	do_level_list },
		{ "SHOW*",	H_NONE,	do_level_list },
		{ "SET*",	H_NONE,	do_level_set },
		{ "ASS*",	H_NONE,	do_level_set },
		{ "RESET*",	H_NONE,	do_level_reset },
		{ "REASS*",	H_NONE,	do_level_reset },
		{ NULL }
	};

	if (command = get_chan_hash(source, cmd, hash_table))
	    (*command->process)(source, chan);
	else {
	    notice(s_ChanServ, source,
			"Syntax: \2LEVEL %s {SET|RESET|LIST} \2[\37\2command\2\37|\37all\37 [\37level\37]]\2",
			chan);
	    notice(s_ChanServ, source,
			"\2/msg %s HELP LEVEL\2 for more information.", s_ChanServ);
	}
    }
}

static void do_level_list (const char *source, char *chan)
{
    char *acc  = strtok(NULL, " ");
    ChannelInfo *ci = cs_findchan(chan);
    User *u = finduser(source);
    int i;

#ifdef IRCOP_OVERRIDE
	if (check_access(u, ci, CA_SET) || is_services_op(source)) {
#else
	if (check_access(u, ci, CA_SET)) {
#endif
	    notice(s_ChanServ, source, "Level required for comands on %s:", chan);
	    notice(s_ChanServ, source, "Level   Type");
	    for (i=0; levelinfo[i].what >= 0; i++)
		if(!acc || match_wild_nocase(acc, levelinfo[i].cmd))
		    notice(s_ChanServ, source, "%5d   %s",
			ci->cmd_access[levelinfo[i].what], levelinfo[i].desc);
	} else 
	    for (i=0; levelinfo[i].what >= 0; i++)
		if(!acc || match_wild_nocase(acc, levelinfo[i].cmd))
		    if (check_access(u, ci, levelinfo[i].what) &&
	    			ci->cmd_access[levelinfo[i].what] >= 0)
			notice(s_ChanServ, source, "You can %s",
							levelinfo[i].desc);
}


static void do_level_set (const char *source, char *chan)
{
    char *acc  = strtok(NULL, " ");
    char *s    = strtok(NULL, " ");
    ChannelInfo *ci = cs_findchan(chan);
    User *u = finduser(source);
    NickInfo *ni = findnick(source);
    short level, chlev = -1;
    int i;

    if (!is_founder(u, ni, ci))
	notice(s_ChanServ, source, "Access denied.");

    else if (!s)
	notice(s_ChanServ, source,
		"Syntax: \2LEVEL %s SET \37command\2\37 \37level\37",
		chan ? chan : "\37channel\37");

    else {
	for(i=0;levelinfo[i].what >= 0; i++)
	    if (stricmp(acc, levelinfo[i].cmd)==0)
		chlev=levelinfo[i].what;

	if (chlev>=0) {
	    level=atoi(s);
	    if (level < ci->cmd_access[CA_FLOOR])
		notice(s_ChanServ, source,
			"Cannot change to levels below %d",
			ci->cmd_access[CA_FLOOR]);
	    else if (level > ci->cmd_access[CA_CAP])
		notice(s_ChanServ, source,
			"Cannot change to levels above %d",
			ci->cmd_access[CA_CAP]);
	    else if(ci->cmd_access[chlev]==level)
		notice(s_ChanServ, source,
			"Level for \2%s\2 on %s unchanged from \2%d\2.",
			acc, chan, level);
	    else {
		ci->cmd_access[chlev] = level;
		notice(s_ChanServ, source,
			"Level for \2%s\2 on %s changed to \2%d\2.",
			acc, chan, level);
	    }	
	} else
	    notice(s_ChanServ, source,
		"Unknown level type \2%s\2", acc);
    }
}

static void do_level_reset (const char *source, char *chan)
{
    char *acc  = strtok(NULL, " ");
    ChannelInfo *ci = cs_findchan(chan);
    User *u = finduser(source);
    NickInfo *ni = findnick(source);
    short chlev = -2;
    int i;

    if (!is_founder(u, ni, ci))
	notice(s_ChanServ, source, "Access denied.");

    else if (!acc)
	notice(s_ChanServ, source,
		"Syntax: \2LEVEL %s RESET \2{\37\2command\2\37|\37\2ALL\2\37}",
		chan ? chan : "\37channel\37");

    else {
	for(i=0;levelinfo[i].what >= 0; i++)
	    if (stricmp(acc, levelinfo[i].cmd)==0)
		chlev=levelinfo[i].what;

	if (chlev<0 && match_wild_nocase("ALL*", acc))
		chlev=-1;
	if (chlev>=0) {
	    ci->cmd_access[chlev] = def_access[chlev];
	    notice(s_ChanServ, source,
		"Level for \2%s\2 on %s reset to \2%d\2.",
		acc, chan, def_access[chlev]);
	} else if (chlev==-1) {
	    ci->cmd_access = def_access;
	    notice(s_ChanServ, source,
		"All Levels for %s reset.",
		chan);
	} else
	    notice(s_ChanServ, source,
		"Unknown level type \2%s\2", acc);
    }
}

/*************************************************************************/

static void do_access(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *cmd  = strtok(NULL, " ");
    ChannelInfo *ci;
    User *u;

    /* If LIST, we don't *require* any parameters, but we can take any.
     * If DEL, we require a mask and no level.
     * Else (ADD), we require a level (which implies a mask). */
    if (!cmd) {
	notice(s_ChanServ, source,
		"Syntax: \2ACCESS %s {ADD|DEL|LIST} [\37user\37 [\37level\37]]\2",
		chan ? chan : "\37channel\37");
	notice(s_ChanServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!(u = finduser(source)) || !(check_access(u, ci, CA_ACCESS)
#ifdef IRCOP_OVERRIDE
					|| is_services_op(source)))
#else
					))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else {
	Hash_CHAN *command, hash_table[] = {
		{ "ADD*",	H_NONE,	do_access_add },
		{ "CREATE",	H_NONE,	do_access_add },
		{ "MAKE",	H_NONE,	do_access_add },
		{ "DEL*",	H_NONE,	do_access_del },
		{ "ERASE*",	H_NONE,	do_access_del },
		{ "TRASH",	H_NONE,	do_access_del },
		{ "LIST*",	H_NONE,	do_access_list },
		{ "VIEW",	H_NONE,	do_access_list },
		{ "DISP*",	H_NONE,	do_access_list },
		{ "SHOW*",	H_NONE,	do_access_list },
		{ NULL }
	};

	if (command = get_chan_hash(source, cmd, hash_table))
	    (*command->process)(source, chan);
	else {
	    notice(s_ChanServ, source,
		"Syntax: \2ACCESS %s {ADD|DEL|LIST} [\37mask\37 [\37level\37]]\2", chan);
	    notice(s_ChanServ, source,
		"\2/msg %s HELP ACCESS\2 for more information.", s_ChanServ);

	}
    }
}

void do_access_add (const char *source, char *chan)
{
    char *mask = strtok(NULL, " ");
    char *s    = strtok(NULL, " ");
    ChannelInfo *ci = cs_findchan(chan);	/* MUST exist */
    NickInfo *ni;
    User *u = finduser(source);			/* MUST exist */
    short level = 0, ulev = get_access(u, ci);	/* MUST be ok || SOP */
    int i;
    ChanAccess *access;

    if (!s) {
	notice(s_ChanServ, source,
		"Syntax: \2ACCESS %s ADD \37user\37 \37level\37\2",
		chan ? chan : "\37channel\37");
	return;
    } else if ((level = atoi(s)) >= ulev) {
	notice(s_ChanServ, source, "Access denied.");
	return;
    }
#ifdef IRCOP_OVERRIDE
    if (ulev > ci->cmd_access[CA_ACCESS]) {
#endif
	if(services_level!=1) {
	    notice(s_ChanServ, source,
		    "Sorry, channel access list modification is temporarily disabled.");
	    return;
	}

	if (level == 0) {
	    notice(s_ChanServ, source, "Access level must be non-%d.", 0);
	    return;
	} else if (level < ci->cmd_access[CA_FLOOR]) {
	    notice(s_ChanServ, source,
		"Cannot add or change to access levels below %d", ci->cmd_access[CA_FLOOR]);
	    return;
	} else if (level > ci->cmd_access[CA_CAP]) {
	    notice(s_ChanServ, source,
		"Cannot add or change to access levels above %d", ci->cmd_access[CA_CAP]);
	    return;
	}

	ni = findnick(mask);
	if (!ni) {
	    char *nick, *user, *host;
	    if (ci->flags & CI_SECURE) {
		notice(s_ChanServ, source,
			"SECURE channel access lists may only contain registered");
		notice(s_ChanServ, source,
			"nicknames, not nick!user@host masks.");
		notice(s_ChanServ, source,
			"\2/msg %s HELP SET SECURE\2 for more information.",
			s_ChanServ);
		return;
	    }
	    split_usermask(mask, &nick, &user, &host);
	    s = smalloc(strlen(nick)+strlen(user)+strlen(host)+3);
	    sprintf(s, "%s!%s@%s", nick, user, host);
	    free(nick);
	    free(user);
	    free(host);
	}
	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	    if ((access->is_nick ? stricmp(access->name,mask) :
					stricmp(access->name,mask)) == 0) {
		if (access->level >= ulev && !is_founder(u, ni, ci)) {
		    notice(s_ChanServ, source, "Access denied.");
		    return;
		}
		if (access->level == level) {
		    notice(s_ChanServ, source,
			"Access level for \2%s\2 on %s unchanged from \2%d\2.",
			access->name, chan, level);
		    return;
		}
		access->level = level;
		notice(s_ChanServ, source,
			"Access level for \2%s\2 on %s changed to \2%d\2.",
			access->name, chan, level);
		return;
	    }
	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	    if (access->is_nick < 0)
		break;
	if (i == ci->accesscount) {
	    ++ci->accesscount;
	    ci->access =
		srealloc(ci->access, sizeof(ChanAccess) * ci->accesscount);
	    access = &ci->access[ci->accesscount-1];
	}
	access->name = ni ? sstrdup(ni->nick) : s;
	access->is_nick = (ni != NULL);
	access->level = level;
	notice(s_ChanServ, source,
		"\2%s\2 added to %s access list at level \2%d\2.",
		access->name, chan, level);
#ifdef IRCOP_OVERRIDE
    } else
	notice(s_ChanServ, source, "Access denied.");
#endif
}

void do_access_del (const char *source, char *chan)
{
    char *mask = strtok(NULL, " ");
    ChannelInfo *ci = cs_findchan(chan);	/* MUST exist */
    NickInfo *ni;
    User *u = finduser(source);			/* MUST exist */
    short ulev = get_access(u, ci);		/* MUST be ok || SOP */
    int i;
    ChanAccess *access;

    if (!mask) {
	notice(s_ChanServ, source,
		"Syntax: \2ACCESS %s DEL \37user\37\2",
		chan ? chan : "\37channel\37");
	return;
    }
#ifdef IRCOP_OVERRIDE
    if (ulev > ci->cmd_access[CA_ACCESS]) {
#endif
	if(services_level!=1) {
	    notice(s_ChanServ, source,
		"Sorry, channel access list modification is temporarily disabled.");
	    return;
	}

	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn(mask, "1234567890") == strlen(mask) &&
				(i = atoi(mask)) > 0 && i <= ci->accesscount) {
	    --i;
	    access = &ci->access[i];
	    if (access->is_nick < 0) {
		notice(s_ChanServ, source,
			"No such entry (#%d) on %s access list.", i+1, chan);
		return;
	    }
	} else {
	    ni = findnick(mask);
	    if (!ni) {
		char *nick, *user, *host;
		split_usermask(mask, &nick, &user, &host);
		mask = smalloc(strlen(nick)+strlen(user)+strlen(host)+3);
		sprintf(mask, "%s!%s@%s", nick, user, host);
		free(nick);
		free(user);
		free(host);
	    }
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (access = ci->access, i = 0; i < ci->accesscount;
							++access, ++i)
		if (access->is_nick >= 0 && stricmp(access->name, mask) == 0)
		    break;
	    if (i == ci->accesscount)
		for (access = ci->access, i = 0; i < ci->accesscount;
							++access, ++i)
		    if (access->is_nick >= 0 && stricmp(access->name, mask) == 0)
			break;
	    if (i == ci->accesscount) {
		notice(s_ChanServ, source,
			"\2%s\2 not found on %s access list.", mask, chan);
		if (!ni)
		    free(mask);
		return;
	    }
	    if (!ni)
		free(mask);
	}
	ni = findnick(source);
	if (ulev <= access->level && !is_founder(u, ni, ci))
	    notice(s_ChanServ, source, "Permission denied.");
	else {
	    notice(s_ChanServ, source,
		"\2%s\2 deleted from %s access list.", access->name, chan);
	    free(access->name);
	    access->is_nick = -1;
	}
#ifdef IRCOP_OVERRIDE
    } else
	notice(s_ChanServ, source, "Access denied.");
#endif
}

void do_access_list (const char *source, char *chan)
{
    char *mask = strtok(NULL, " ");
    char *s;
    ChannelInfo *ci = cs_findchan(chan);	/* MUST exist */
    NickInfo *ni;
    short level = 0;
    int i;
    ChanAccess *access;

	if (mask)
	    level = atoi(mask);
	notice(s_ChanServ, source, "Access list for %s:", chan);
	notice(s_ChanServ, source, "  Num   Lev  Mask");
	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i) {
	    if (access->is_nick < 0)
		continue;
	    if ((mask && !match_wild_nocase(mask, access->name)) ||
					(level && level != access->level))
		continue;
	    if (ni = findnick(access->name))
		s = ni->last_usermask;
	    else
		s = NULL;
	    notice(s_ChanServ, source, "  %3d %6d  %s%s%s%s",
			i+1, access->level, access->name,
			s ? " (" : "", s ? s : "", s ? ")" : "");
	}
}

/*************************************************************************/

#if AKICK_MAX > 0
static void do_akick(const char *source)
{
    char *chan   = strtok(NULL, " ");
    char *cmd    = strtok(NULL, " ");
    ChannelInfo *ci;
    User *u;

    if (!cmd) {
	notice(s_ChanServ, source,
		"Syntax: \2AKICK %s {ADD|DEL|LIST} [\37nick-or-usermask\37]\2",
		chan ? chan : "\37channel\37");
	notice(s_ChanServ, source,
		"\2/msg %s HELP AKICK\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!(u = finduser(source)) || !(check_access(u, ci, CA_AKICK)
#ifdef IRCOP_OVERRIDE
					|| is_services_op(source)))
#else
					))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else {
	Hash_CHAN *command, hash_table[] = {
		{ "ADD*",	H_NONE,	do_akick_add },
		{ "CREATE",	H_NONE,	do_akick_add },
		{ "MAKE",	H_NONE,	do_akick_add },
		{ "DEL*",	H_NONE,	do_akick_del },
		{ "ERASE*",	H_NONE,	do_akick_del },
		{ "TRASH",	H_NONE,	do_akick_del },
		{ "LIST*",	H_NONE,	do_akick_list },
		{ "VIEW",	H_NONE,	do_akick_list },
		{ "DISP*",	H_NONE,	do_akick_list },
		{ "SHOW*",	H_NONE,	do_akick_list },
		{ NULL }
	};

	if (command = get_chan_hash(source, cmd, hash_table))
	    (*command->process)(source, chan);
	else {
	    notice(s_ChanServ, source,
		"Syntax: \2AKICK %s {ADD|DEL|LIST} [\37nick-or-usermask\37]\2",
		chan);
	    notice(s_ChanServ, source,
		"\2/msg %s HELP AKICK\2 for more information.", s_ChanServ);
	}
    }
}

void do_akick_add (const char *source, char *chan)
{
    char *mask   = strtok(NULL, " ");
    char *reason = strtok(NULL, "");
    ChannelInfo *ci = cs_findchan(chan);
    NickInfo *ni;
    User *u = finduser(source);
    int i;
    AutoKick *akick;

    if (!mask) {
	notice(s_ChanServ, source,
		"Syntax: \2AKICK %s ADD \37nick-or-usermask\37 [\37reason\37]\2",
		chan ? chan : "\37channel\37");
	return;
    }
#ifdef IRCOP_OVERRIDE
    if (check_access(u, ci, CA_AKICK)) {
#endif
	if(services_level!=1) {
	    notice(s_ChanServ, source,
		"Sorry, channel AutoKick list modification is temporarily disabled.");
	    return;
	}

	if (ci->akickcount > AKICK_MAX) {
	    notice(s_ChanServ, source,
		    "Sorry, you can only have %d AutoKick masks on a channel.",
		    AKICK_MAX);
	    return;
	}
	ni = findnick(mask);
	if (!ni) {
	    char *nick, *user, *host;
	    int nonchr = 0;
	    split_usermask(mask, &nick, &user, &host);
	    mask = smalloc(strlen(nick)+strlen(user)+strlen(host)+3);
	    sprintf(mask, "%s!%s@%s", nick, user, host);
	    free(nick);
	    free(user);
	    free(host);
	    for(i=strlen(mask);mask[i]!='@' && i>=0;i--)
		if(!(mask[i]=='*' || mask[i]=='?' || mask[i]=='.'))
		    nonchr++;
	    if(nonchr<STARTHRESH && !check_access(u, ci, CA_STARAKICK)) {
		notice(s_ChanServ, source, "Must have at least %d non- *, ? or . characters in host.", STARTHRESH);
		return;
	    } else if (nonchr<STARTHRESH) {
		for(i--;i>0 && (mask[i]=='*' || mask[i]=='?');i--) ;
		if (!i) {
		    notice(s_ChanServ, source, "Must have at least 1 non- *, ? or . characters.", STARTHRESH);
		    return;
		}
	    }
	}
	if (get_justaccess(mask, ci)>=get_access(u,ci)) {
	    notice(s_ChanServ, source,
		"Sorry, \2%s\2 is higher or equal on access list.",
		ni ? ni->nick : mask);
	    return;
	} else if (!ni && (i = countusermask(mask))) {
	    User *user2;
	    while (i) {
		user2 = findusermask(mask, i);
		if (get_access(user2, ci)>=get_access(u,ci)) {
		    notice(s_ChanServ, source,
			"Sorry, \2%s\2 matches \2%s\2 (higher or equal on access list).",
			mask, user2->nick);
		    return;
		}
		i--;
	    }
	}
	if(is_on_akick(mask, ci))
	    notice(s_ChanServ, source, "AKICK already exists (or inclusive)");
	else {		
	    Channel *c;
	    ++ci->akickcount;
	    ci->akick = srealloc(ci->akick, sizeof(AutoKick) * ci->akickcount);
	    akick = &ci->akick[ci->akickcount-1];
	    akick->name = ni ? sstrdup(ni->nick) : mask;
	    akick->is_nick = (ni != NULL);
	    akick->pad = 0;
	    if (reason)
		akick->reason = sstrdup(reason);
	    else
		akick->reason = NULL;
	    notice(s_ChanServ, source,
		"\2%s\2 added to %s AKICK list.", akick->name, chan);

	    /* This call should not fail, but check just in case... */
	    if (c = findchan(chan)) {
		struct c_userlist *u2;
		if (!reason)
		    reason = DEF_AKICK_REASON;
		for (u2 = c->users; u2; u2 = u2->next)
		    if (ni ? stricmp(u2->user->nick, akick->name)==0 :
				match_usermask(akick->name, u2->user))
			kick_user(s_ChanServ, chan, u2->user->nick, reason);
	    }
	}
#ifdef IRCOP_OVERRIDE
    } else
	notice(s_ChanServ, source, "Access denied.");
#endif
}

void do_akick_del (const char *source, char *chan)
{
    char *mask   = strtok(NULL, " ");
#ifdef IRCOP_OVERRIDE
    User *u = finduser(source);
#endif
    ChannelInfo *ci = cs_findchan(chan);
    int i;
    AutoKick *akick;

    if (!mask) {
	notice(s_ChanServ, source,
		"Syntax: \2AKICK %s DEL \37nick-or-usermask\37\2",
		chan ? chan : "\37channel\37");
	return;
    }
#ifdef IRCOP_OVERRIDE
    if (check_access(u, ci, CA_AKICK)) {
#endif
	if(services_level!=1) {
	    notice(s_ChanServ, source,
		"Sorry, channel AutoKick list modification is temporarily disabled.");
	    return;
	}
	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn(mask, "1234567890") == strlen(mask) &&
				(i = atoi(mask)) > 0 && i <= ci->akickcount) {
	    --i;
	    akick = &ci->akick[i];
	} else {
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
		if (stricmp(akick->name, mask) == 0)
		    break;
	    if (i == ci->akickcount)
		for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
		    if (stricmp(akick->name, mask) == 0)
			break;
	    if (i == ci->akickcount) {
		notice(s_ChanServ, source,
			"\2%s\2 not found on %s AKICK list.", mask, chan);
		return;
	    }
	}
	notice(s_ChanServ, source,
		"\2%s\2 deleted from %s AKICK list.", akick->name, chan);
	free(akick->name);
	if (akick->reason)
	    free(akick->reason);
	--ci->akickcount;
	if (i < ci->akickcount)
	    bcopy(akick+1, akick, sizeof(AutoKick) * (ci->akickcount-i));
	if (ci->akickcount)
	    ci->akick = srealloc(ci->akick, sizeof(AutoKick) * ci->akickcount);
	else {
	    free(ci->akick);
	    ci->akick = NULL;
	}
#ifdef IRCOP_OVERRIDE
    } else
	notice(s_ChanServ, source, "Access denied.");
#endif
}

void do_akick_list (const char *source, char *chan)
{
    char *mask   = strtok(NULL, " ");
    char *t;
    ChannelInfo *ci = cs_findchan(chan);
    NickInfo *ni;
    int i;
    AutoKick *akick;

	notice(s_ChanServ, source, "AKICK list for %s:", chan);
	for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i) {
	    if (mask && !match_wild_nocase(mask, akick->name))
		continue;
	    if (ni = findnick(akick->name))
		t = ni->last_usermask;
	    else
		t = NULL;
	    notice(s_ChanServ, source, "  %3d %s%s%s%s%s%s%s",
			i+1, akick->name,
			t ? " (" : "", t ? t : "", t ? ")" : "",
			akick->reason ? " (" : "",
			akick->reason ? akick->reason : "",
			akick->reason ? ")" : "");
	}
}

static int is_on_akick(const char *mask, ChannelInfo *ci)
{
    int i, is_nick = 0;
    AutoKick *akick;

    if (findnick(mask)) is_nick = 1;
    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
	if (is_nick)
	    if (akick->is_nick>0 && stricmp(mask, akick->name)==0)
	        return 1;
	else
	    if (akick->is_nick==0 && match_wild_nocase(akick->name, mask))
		return 1;
    return 0;
}

#endif

/*************************************************************************/

static void do_info(const char *source)
{
    char *chan = strtok(NULL, " ");
    ChannelInfo *ci;
    NickInfo *ni;
    struct tm tm;
    int i;
    char s[BUFSIZE], *t, *end;
    time_t curtime = time(NULL);

    if (!chan) {
	notice(s_ChanServ, source, "Syntax: \2INFO \37channel\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP INFO\2 for more information.", s_ChanServ);

    } else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel \2%s\2 is not registered.", chan);

    else if (ci->flags & CI_VERBOTEN)
    	notice(s_ChanServ, source,
    		"Channel \2%s\2 may not be registered or used.", chan);

    else {
	notice(s_ChanServ, source, "Information for channel \2%s\2:", chan);
	if (ni = findnick(ci->founder))
	    t = ni->last_usermask;
	else
	    t = NULL;
	notice(s_ChanServ, source,
		"        Founder: %s%s%s%s",
			ci->founder, t ? " (" : "", t ? t : "", t ? ")" : "");
	notice(s_ChanServ, source,
		"    Description: %s", ci->desc);
#if FILE_VERSION > 2
	if(strlen(ci->url)>0)
	    notice(s_ChanServ, source,
		    " WWW Page (URL): %s", ci->url);
#endif
	tm = *localtime(&ci->time_registered);
	notice(s_ChanServ, source,
		"     Registered: %s %2d %02d:%02d:%02d %d",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
	tm = *localtime(&ci->last_used);
	notice(s_ChanServ, source,
		"      Last used: %s %2d %02d:%02d:%02d %d",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
        tm = *localtime(&curtime);
	notice(s_ChanServ, source,
		"   Current time: %s %2d %02d:%02d:%02d %d\n",
			month_name(tm.tm_mon+1), tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec, tm.tm_year+1900);
	if (ci->last_topic)
	    if (ci->flags & CI_SUSPENDED) {
		notice(s_ChanServ, source,
		    "  Suspended For: %s", ci->last_topic);
		notice(s_ChanServ, source,
		    "   Suspended By: %s\n", ci->last_topic_setter);
	    } else {
		notice(s_ChanServ, source,
		    "     Last topic: %s", ci->last_topic);
		notice(s_ChanServ, source,
		    "   Topic set by: %s\n", ci->last_topic_setter);
	    }
	for (i=0;revengeinfo[i].what!=get_revenge_level(ci) &&
					revengeinfo[i].what>=0;i++) ;
	if (revengeinfo[i].what>=0)
	    notice(s_ChanServ, source, "  Revenge Level: %s.",
					revengeinfo[i].desc);

#ifdef IRCOP_OVERRIDE
	if (!(ci->flags & CI_PRIVATE) || is_oper(source)) {
#else
	if (!(ci->flags & CI_PRIVATE)) {
#endif
	    Channel *c = findchan(chan);
	    if (c) {
		int ops, users, voices;
		struct c_userlist *data;
		for (data = c->users, users = 0; data; data = data->next) users++;
		for (data = c->voices, voices = 0; data; data = data->next) voices++;
		for (data = c->chanops, ops = 0; data; data = data->next) ops++;
		notice(s_ChanServ, source, "  Channel Stats: %d user%s, %d voice%s, %d op%s.",
					users,  users  == 1 ? "" : "s",
					voices, voices == 1 ? "" : "s",
					ops,    ops    == 1 ? "" : "s");
	    }
	}
	if (!ci->flags)
	    snprintf(s, 6, "None\n");
	else {
	    int need_comma = 0;
	    static const char commastr[] = ", ";
	    end = s;
	    if (ci->flags & CI_SUSPENDED)
		end += snprintf(end, sizeof(s)-(end-s), "\2SUSPENDED CHANNEL\2");
	    else {
	    if (ci->flags & CI_PRIVATE) {
		end += snprintf(end, sizeof(s)-(end-s), "Private");
		need_comma = 1;
	    }
	    if (ci->flags & CI_KEEPTOPIC) {
		end += snprintf(end, sizeof(s)-(end-s), "%sTopic Retention", need_comma ? commastr : "");
		need_comma = 1;
	    }
	    if (ci->flags & CI_TOPICLOCK) {
		end += snprintf(end, sizeof(s)-(end-s), "%sTopic Lock", need_comma ? commastr : "");
		need_comma = 1;
	    }
	    if (ci->flags & CI_SECUREOPS) {
		end += snprintf(end, sizeof(s)-(end-s), "%sSecure Ops", need_comma ? commastr : "");
		need_comma = 1;
	    }
	    if (ci->flags & CI_RESTRICTED) {
		end += snprintf(end, sizeof(s)-(end-s), "%sRestricted Access", need_comma ? commastr : "");
		need_comma = 1;
	    }
	    if (ci->flags & CI_SECURE) {
		end += snprintf(end, sizeof(s)-(end-s), "%sSecure", need_comma ? commastr : "");
		need_comma = 1;
	    }
	    }
	}
	notice(s_ChanServ, source, "        Options: %s", s);
	end = s;
	*end = 0;
	if (ci->mlock_on || ci->mlock_key || ci->mlock_limit)
	    end += snprintf(end, BUFSIZE, "+%s%s%s%s%s%s%s%s",
				(ci->mlock_on & CMODE_I) ? "i" : "",
				(ci->mlock_key         ) ? "k" : "",
				(ci->mlock_limit       ) ? "l" : "",
				(ci->mlock_on & CMODE_M) ? "m" : "",
				(ci->mlock_on & CMODE_N) ? "n" : "",
				(ci->mlock_on & CMODE_P) ? "p" : "",
				(ci->mlock_on & CMODE_S) ? "s" : "",
				(ci->mlock_on & CMODE_T) ? "t" : "");
	if (ci->mlock_off)
	    end += snprintf(end, BUFSIZE, "-%s%s%s%s%s%s%s%s",
				(ci->mlock_off & CMODE_I) ? "i" : "",
				(ci->mlock_off & CMODE_K) ? "k" : "",
				(ci->mlock_off & CMODE_L) ? "l" : "",
				(ci->mlock_off & CMODE_M) ? "m" : "",
				(ci->mlock_off & CMODE_N) ? "n" : "",
				(ci->mlock_off & CMODE_P) ? "p" : "",
				(ci->mlock_off & CMODE_S) ? "s" : "",
				(ci->mlock_off & CMODE_T) ? "t" : "");
	notice(s_ChanServ, source,
		"      Mode lock: %s", s);
    }
}

/*************************************************************************/

static void do_list(const char *source)
{
    char *chan = strtok(NULL, " ");
    ChannelInfo *ci;
    int nchans, i;
    char buf[BUFSIZE];

    if (!chan) {
	notice(s_ChanServ, source, "Syntax: \2LIST \37pattern\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP LIST\2 for more information.", s_ChanServ);

    } else {
	nchans = 0;
	notice(s_ChanServ, source, "List of entries matching \2%s\2:", chan);
	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next) {
		if (!(is_oper(source)))
		    if (ci->flags & (CI_PRIVATE | CI_VERBOTEN | CI_SUSPENDED))
			continue;
		if (ci->flags & (CI_VERBOTEN | CI_SUSPENDED))
		    if (strlen(ci->name) > sizeof(buf))
			continue;
		else
		    if (strlen(ci->name)+strlen(ci->desc) > sizeof(buf))
			continue;
		if (ci->flags & CI_VERBOTEN)
		    snprintf(buf, sizeof(buf), "%-20s  << FORBIDDEN >>", ci->name);
		else if (ci->flags & CI_SUSPENDED)
		    snprintf(buf, sizeof(buf), "%-20s  << SUSPENDED >>", ci->name);
		else
		    if(strlen(ci->desc)>0)
			snprintf(buf, sizeof(buf), "%-20s  %s", ci->name, ci->desc);
		    else
			snprintf(buf, sizeof(buf), "%-20s", ci->name);
		if (match_wild_nocase(chan, buf))
		    if (++nchans <= 50)
			notice(s_ChanServ, source, "    %s", buf);
	    }
	notice(s_ChanServ, source, "End of list - %d/%d matches shown.",
					nchans>50 ? 50 : nchans, nchans);
    }

}

static void do_users(const char *source)
{
    char *chan = strtok(NULL, " ");
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan) {
	notice(s_ChanServ, source,
		"Syntax: \2UsERS\2 \037channel\037");
	notice(s_ChanServ, source,
		"\2/msg ChanServ HELP VOICE\2 for more information.");

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDINVITE)) && !is_oper(source))
#else
    else if ((!u || !check_access(u, ci, CA_CMDINVITE)))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else
	send_channel_users(s_ChanServ, source, chan);

}
/*************************************************************************/

static void do_invite(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *inv_params = strtok(NULL, " ");  
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan) {

#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source, "Syntax: \2INVITE\2 \37channel\37 %s", is_oper(source) ? "\037nick\037" : "");
#else
	notice(s_ChanServ, source, "Syntax: \2INVITE\2 \37channel\37");
#endif
	notice(s_ChanServ, source,
		"\2/msg %s HELP INVITE\2 for more information.", s_ChanServ);

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (ci->flags & CI_SUSPENDED)
	send_cmd(s_ChanServ, "INVITE %s %s", source, chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDINVITE)) && !is_oper(source))
#else
    else if ((!u || !check_access(u, ci, CA_CMDINVITE)))
#endif
	    notice(s_ChanServ, source, "Access denied.");

    else
#ifdef IRCOP_OVERRIDE
	if (!inv_params || !is_oper(source))
#endif
	    send_cmd(s_ChanServ, "INVITE %s %s", source, chan);
#ifdef IRCOP_OVERRIDE
	else
	    send_cmd(s_ChanServ, "INVITE %s %s", inv_params, chan);
#endif
}

/*************************************************************************/

static void do_voice(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *voice_params = strtok(NULL, " ");  
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan) {
#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source,
		"Syntax: \2VOICE\2 \037channel\037 %s", is_oper(source) ? "\037nick\037" : "");
#else
	notice(s_ChanServ, source,
		"Syntax: \2VOICE\2 \037channel\037");
#endif
	notice(s_ChanServ, source,
		"\2/msg ChanServ HELP VOICE\2 for more information.");

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!is_on_chan(source, chan))
	notice(s_ChanServ, source, "You are not in channel %s.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (is_voiced(source, chan))
	notice(s_ChanServ, source, "You are already voiced in %s", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDVOICE)) && !is_oper(source))
#else
    else if (!u || !check_access(u, ci, CA_CMDVOICE))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else
#ifdef IRCOP_OVERRIDE
	if (!voice_params || !is_oper(source))
#endif
	    change_cmode(s_ChanServ, chan, "+v", source);
#ifdef IRCOP_OVERRIDE
	else
	    change_cmode(s_ChanServ, chan, "+v", voice_params);
#endif
}

/*************************************************************************/

static void do_devoice(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *devoice_params = strtok(NULL, " ");
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan) {
#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source,
		"Syntax: \2DEVOICE\2 \037channel\037 %s", is_oper(source) ? "\037nick\037" : "");
#else
	notice(s_ChanServ, source,
		"Syntax: \2DEVOICE\2 \037channel\037");
#endif
	notice(s_ChanServ, source,
		"\2/msg ChanServ HELP DEVOICE\2 for more information.");

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!is_on_chan(source, chan))
	notice(s_ChanServ, source, "You are not in channel %s.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!is_voiced(source, chan))
	notice(s_ChanServ, source, "You are not voiced in %s", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDVOICE)) && !is_oper(source))
#else
    else if (!u || !check_access(u, ci, CA_CMDVOICE))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else
#ifdef IRCOP_OVERRIDE
	if (!devoice_params || !is_oper(source))
#endif
	    change_cmode(s_ChanServ, chan, "-v", source);
#ifdef IRCOP_OVERRIDE
	else
	    change_cmode(s_ChanServ, chan, "-v", devoice_params);
#endif
}
/*************************************************************************/

static void do_op(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *op_params = strtok(NULL, " ");  
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan) {
#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source,
		"Syntax: \2OP\2 \037channel\037 %s", is_oper(source) ? "\037nick\037" : "");
#else
	notice(s_ChanServ, source,
		"Syntax: \2OP\2 \037channel\037");
#endif
	notice(s_ChanServ, source,
		"\2/msg ChanServ HELP OP\2 for more information.");

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!is_on_chan(source, chan))
	notice(s_ChanServ, source, "You are not in channel %s.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (is_chanop(source, chan))
	notice(s_ChanServ, source, "You are already an op in %s", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDOP)) && !is_oper(source))
#else
    else if (!u || !check_access(u, ci, CA_CMDOP))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else
#ifdef IRCOP_OVERRIDE
	if (!op_params || !is_oper(source))
#endif
	    change_cmode(s_ChanServ, chan, "+o", source);
#ifdef IRCOP_OVERRIDE
	else
	    change_cmode(s_ChanServ, chan, "+o", op_params);
#endif
}

/*************************************************************************/

static void do_deop(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *deop_params = strtok(NULL, " ");
    User *u = finduser(source);
    ChannelInfo *ci;
    int ulev;

    if (!chan) {
#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source,
		"Syntax: \2DEOP\2 \037channel\037 %s", is_oper(source) ? "\037nick\037" : "");
#else
	notice(s_ChanServ, source,
		"Syntax: \2DEOP\2 \037channel\037");
#endif
	notice(s_ChanServ, source,
		"\2/msg ChanServ HELP DEOP\2 for more information.");

    } else if (!findchan(chan))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!is_on_chan(source, chan))
	notice(s_ChanServ, source, "You are not in channel %s.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!is_chanop(source, chan))
	notice(s_ChanServ, source, "You are not an op in %s", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDOP)) && !is_oper(source))
#else
    else if (!u || !check_access(u, ci, CA_CMDOP))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else
#ifdef IRCOP_OVERRIDE
	if (!deop_params || !is_oper(source))
#endif
	    change_cmode(s_ChanServ, chan, "-o", source);
#ifdef IRCOP_OVERRIDE
	else
	    change_cmode(s_ChanServ, chan, "-o", deop_params);
#endif
}

/*************************************************************************/

static void do_unban(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *unban_params = strtok(NULL, " ");
    User *u;
    ChannelInfo *ci;
    Channel *c;
    int i;

#ifdef IRCOP_OVERRIDE
    if (!is_oper(source) || !unban_params)
#endif
	u = finduser(source);
#ifdef IRCOP_OVERRIDE
    else
	u = finduser(unban_params);
#endif

    if (!chan) {
#ifdef IRCOP_OVERRIDE
	notice(s_ChanServ, source, "Syntax: \2UNBAN\2 \37channel\37 %s", is_oper(source) ? "\37nick\37" : "");
#else
	notice(s_ChanServ, source, "Syntax: \2UNBAN\2 \37channel\37");
#endif
	notice(s_ChanServ, source,
		"\2/msg %s HELP UNBAN\2 for more information.", s_ChanServ);

    } else if (!(c = findchan(chan)))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !(check_access(u, ci, CA_CMDUNBAN)) || is_oper(source))) {
#else
    else if (!u || !check_access(u, ci, CA_CMDUNBAN)) {
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
    	    notice(s_ChanServ, source, "Access denied.");

    } else {
	for (i = 0; i < c->bancount; i++)
	    if (match_usermask(c->bans[i], u)) {
		change_cmode(s_ChanServ, c->name, "-b", c->bans[i]);
		i--;
	    }

#ifdef IRCOP_OVERRIDE
	if (!unban_params || !is_oper(source))
#endif
	    notice(s_ChanServ, source, "You have been unbanned from %s.", chan);
#ifdef IRCOP_OVERRIDE
	else
	    notice(s_ChanServ, source, "%s has been unbanned from %s.", unban_params, chan);
#endif
    }
}

/*************************************************************************/

static void do_clear(const char *source)
{
    char *chan = strtok(NULL, " ");
    char *what = strtok(NULL, " ");
    User *u = finduser(source);
    Channel *c;
    ChannelInfo *ci;

    if (!what) {

	notice(s_ChanServ, source,
		"Syntax: \2CLEAR \37channel\37 \37what\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP CLEAR\2 for more information.", s_ChanServ);

    } else if (!(c = findchan(chan)))
	notice(s_ChanServ, source, "Channel %s does not exist.", chan);

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

#ifdef IRCOP_OVERRIDE
    else if ((!u || !check_access(u, ci, CA_CMDCLEAR)) && !is_services_op(source))
#else
    else if (!u || !check_access(u, ci, CA_CMDCLEAR))
#endif
        if (ci->flags & CI_SUSPENDED)
	    notice(s_ChanServ, source, "Access Denied for SUSPENDED channels.");
	else
	    notice(s_ChanServ, source, "Access denied.");

    else if (stricmp(what, "bans") == 0) {
	char *av[3];
	int i;

	for (i = 0; i < c->bancount; ++i)
	    change_cmode(s_ChanServ, chan, "-b", c->bans[i]);
	notice(s_ChanServ, source, "All bans on %s have been removed.", chan);

    } else if (stricmp(what, "modes") == 0) {
	char *av[3];

	if (c->key)
	    change_cmode(s_ChanServ, chan, "-mintpslk", c->key);
	else
	    change_cmode(s_ChanServ, chan, "-mintpslk", "");
	notice(s_ChanServ, source, "All modes on %s have been reset.", chan);

    } else if (stricmp(what, "ops") == 0) {
	char *av[3];
	struct c_userlist *cu, *next;

	for (cu = c->chanops; cu; cu = next) {
	    next = cu->next;
	    change_cmode(s_ChanServ, chan, "-o", cu->user->nick);
	}
	notice(s_ChanServ, source, "Mode +o cleared from %s.", chan);

    } else if (stricmp(what, "voices") == 0) {
	char *av[3];
	struct c_userlist *cu, *next;

	for (cu = c->voices; cu; cu = next) {
	    next = cu->next;
	    change_cmode(s_ChanServ, chan, "-v", cu->user->nick);
	}
	notice(s_ChanServ, source, "Mode +v cleared from %s.", chan);

    } else if (stricmp(what, "users") == 0) {
	char *av[3];
	struct c_userlist *cu, *next_cu;
	char buf[BUFSIZE];

	snprintf(buf, sizeof(buf), "CLEAR USERS command from %s", source);

	for (cu = c->users; cu; cu = next_cu) {
	    next_cu = cu->next;
	    kick_user(s_ChanServ, chan, cu->user->nick, buf);
	}
	notice(s_ChanServ, source, "All users kicked from %s.", chan);

    } else {
	notice(s_ChanServ, source,
		"Syntax: \2CLEAR \37channel\37 \37what\37\2");
	notice(s_ChanServ, source,
		"\2/msg %s HELP CLEAR\2 for more information.", s_ChanServ);
    }
}

/*************************************************************************/

static void do_getpass(const char *source)
{
    char *chan = strtok(NULL, " ");
    User *u = finduser(source);
    ChannelInfo *ci;

    if (!chan)
	notice(s_ChanServ, source, "Syntax: \2GETPASS \37channel\37\2");

    else if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source, "Channel %s is not registered.", chan);

    else if (!u)
	notice(s_ChanServ, source, "Could not find your user record!");

    else {
	log("%s: %s!%s@%s used GETPASS on %s",
		s_ChanServ, source, u->username, u->host, chan);
	wallops(s_ChanServ, "\2%s\2 used GETPASS on channel \2%s\2", source, chan);
	notice(s_ChanServ, source, "Password for %s is: \2%s\2",
		chan, ci->founderpass);
    }
}

/*************************************************************************/

static void do_forbid(const char *source)
{
    ChannelInfo *ci;
    char *chan = strtok(NULL, " ");

    if (!chan) {
	notice(s_ChanServ, source, "Syntax: \2FORBID \37channel\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_ChanServ, source,
	    "Warning: Services is in read-only mode.  Changes will not be saved.");
    if (ci = cs_findchan(chan)) {
	delchan(ci);
	wallops(s_ChanServ, "\2%s\2 used FORBID on channel \2%s\2", source, chan);
    }
    if (ci = makechan(chan)) {
    	log("%s: %s set FORBID for channel %s", s_ChanServ, source, chan);
	ci->flags |= CI_VERBOTEN;
	notice(s_ChanServ, source,
		"Channel \2%s\2 has been marked FORBIDden.", chan);
    } else {
    	log("%s: Valid FORBID for %s by %s failed", s_ChanServ,
    		chan, source);
	notice(s_ChanServ, source,
		"Couldn't FORBID channel \2%s\2!", chan);
    }
}

/*************************************************************************/

static void do_suspend(const char *source)
{
    Channel *c;
    ChannelInfo *ci;
    char *chan = strtok(NULL, " ");
    char *reason = strtok(NULL, "");

    if (!reason) {
	notice(s_ChanServ, source, "Syntax: \2SUSPEND \37channel reason\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_ChanServ, source,
	    "Warning: Services is in read-only mode.  Changes will not be saved.");
    if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source,
		"Channel \2%s\2 does not exist", chan);
    else if (ci->flags & CI_SUSPENDED)
	notice(s_ChanServ, source,
	    "Channel %s is already suspended.", chan);
    else {
	ci->flags |= CI_SUSPENDED;

	if (ci->last_topic)
	    free(ci->last_topic);
	ci->last_topic = sstrdup(reason);
	strscpy(ci->last_topic_setter, source, NICKMAX);
	ci->last_topic_time = time(NULL);

	if ((c = findchan(chan))) {
	    int i;
	    char *av[3];
	    struct c_userlist *cu, *next;

	/* CLEAR BANS */
	    for (i = 0; i < c->bancount; ++i)
		change_cmode(s_ChanServ, chan, "-b", c->bans[i]);

	/* CLEAR MODES */
	    if (c->key)
		change_cmode(s_ChanServ, chan, "-mintpslk", c->key);
	    else
		change_cmode(s_ChanServ, chan, "-mintpslk", "");

	/* CLEAR OPS */
	    for (cu = c->chanops; cu; cu = next) {
		next = cu->next;
		change_cmode(s_ChanServ, chan, "-o", cu->user->nick);
	    }

	/* CLEAR VOICES */
	    for (cu = c->voices; cu; cu = next) {
		next = cu->next;
		change_cmode(s_ChanServ, chan, "-v", cu->user->nick);
	    }
	    restore_topic(chan);
	}

    	log("%s: %s set SUSPEND for channel %s because of %s", s_ChanServ,
    		source, chan, reason);
	notice(s_ChanServ, source,
		"Channel \2%s\2 has been marked SUSPENDED.", chan);
    }
}

static void do_unsuspend(const char *source)
{
    Channel *c;
    ChannelInfo *ci;
    char *chan = strtok(NULL, " ");
    int i;
    char *av[3];

    if (!chan) {
	notice(s_ChanServ, source, "Syntax: \2UNSUSPEND \37channel\37\2");
	return;
    }
    if(services_level!=1)
	notice(s_ChanServ, source,
	    "Warning: Services is in read-only mode.  Changes will not be saved.");
    if (!(ci = cs_findchan(chan)))
	notice(s_ChanServ, source,
		"Channel \2%s\2 does not exist", chan);
    else if (!(ci->flags & CI_SUSPENDED))
	notice(s_ChanServ, source,
	    "Channel %s is not suspended.", chan);
    else {
	ci->flags &= ~CI_SUSPENDED;

	if (ci->last_topic)
	    free(ci->last_topic);
	ci->last_topic = NULL;
	strscpy(ci->last_topic_setter, source, NICKMAX);
	ci->last_topic_time = time(NULL);

	if ((c = findchan(chan))) {
	    if (c->topic)
		free(c->topic);
	    c->topic = NULL;
	    strscpy(c->topic_setter, ci->last_topic_setter, NICKMAX);
	    c->topic_time = ci->last_topic_time;

	    send_cmd(s_ChanServ, "TOPIC %s %s %lu :", chan,
		c->topic_setter, c->topic_time);
	}
    	log("%s: %s removed SUSPEND for channel %s", s_ChanServ, source, chan);
	notice(s_ChanServ, source,
		"Channel \2%s\2 has been marked UNSUSPENDED.", chan);
    }
}

/*************************************************************************/
#endif  /* CHANSERV */
