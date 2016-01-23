/* ChanServ functions.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"


/* These 3 no longer static, as they are needed in memoserv.c for the
 * news functions.
 */
ChannelInfo *chanlists[256];
ChannelInfo *cs_findchan (const char *chan);
int channel_expire;
int akick_max;
char def_akick_reason[512];

static short def_access[] =
{
    -1,				/* CA_AUTODEOP  */
    5,				/* CA_AUTOVOICE */
    10,				/* CA_AUTOOP    */
    0,				/* CA_READMEMO  */
    15,				/* CA_WRITEMEMO */
    25,				/* CA_DELMEMO   */
    20,				/* CA_AKICK     */
    25,				/* CA_STARAKICK */
    10,				/* CA_UNBAN     */
    5,				/* CA_ACCESS    */
    25,				/* CA_SET       */
    5,				/* CA_CMDINVITE */
    10,				/* CA_CMDUNBAN  */
    5,				/* CA_CMDVOICE  */
    10,				/* CA_CMDOP     */
    20,				/* CA_CMDCLEAR  */
    -1,				/* CA_FLOOR     */
    30,				/* CA_CAP       */
    50,				/* CA_FOUNDER   */
    -1
};

typedef struct
{
    int what;
    char *cmd;
    char *desc;
}
LevelInfo;
static LevelInfo levelinfo[] =
{
    {CA_AUTODEOP, "AUTODEOP", "Automatic Mode -v-o (DeVoice and DeOp)"},
    {CA_AUTOVOICE, "AUTOVOICE", "Automatic Mode +v (Voice)"},
    {CA_AUTOOP, "AUTOOP", "Automatic Mode +o (Op)"},
    {CA_READMEMO, "NEWSREAD", "Read channel news"},
    {CA_WRITEMEMO, "NEWSPOST", "Post channel news"},
    {CA_DELMEMO, "NEWSDEL", "Erase ALL or non-owned channel news"},
    {CA_AKICK, "AKICK", "Modify AKICK list"},
    {CA_STARAKICK, "STARAKICK", "Add @* AKICK's"},
    {CA_UNBAN, "BOUNCE", "Bounce server bans"},
    {CA_ACCESS, "ACCESS", "Modify ACCESS list"},
    {CA_SET, "SET", "Modify channel SET's (except FOUNDER and PASSWORD)"},
    {CA_CMDINVITE, "INVITE", "Use /MSG ChanServ INVITE command"},
    {CA_CMDUNBAN, "UNBAN", "Use /MSG ChanServ UNBAN command"},
    {CA_CMDVOICE, "VOICE", "Use /MSG ChanServ VOICE command"},
    {CA_CMDOP, "OP", "Use /MSG ChanServ OP command"},
    {CA_CMDCLEAR, "CLEAR", "Use /MSG ChanServ CLEAR command"},
    {-1}
};

typedef struct
{
    int what;
    char *cmd;
    int rev1;
    int rev2;
    int rev3;
    char *desc;
}
RevengeInfo;
static RevengeInfo revengeinfo[] =
{
    {CR_NONE, "NONE", 0, 0, 0, "Disabled"},
    {CR_REVERSE, "REVERSE", 0, 0, 1, "Reverse Activity"},
    {CR_DEOP, "DEOP", 0, 1, 0, "Mode -o (DeOp)"},
    {CR_KICK, "KICK", 0, 1, 1, "Kick"},
    {CR_NICKBAN, "NICKBAN", 1, 0, 0, "Ban (user!*@*)"},
    {CR_USERBAN, "MASKBAN", 1, 0, 1, "Ban (*!*user@*.host)"},
    {CR_HOSTBAN, "HOSTBAN", 1, 1, 0, "Ban (*!*@host)"},
    {CR_MIRROR, "MIRROR", 1, 1, 1, "Mirror Activity"},
    {-1}
};

/* Note, the *-* symbol denotes cannot be changed by user. */

char s_ChanServ[NICKMAX];
char chanserv_db[512];
#include "cs-help.c"

static void alpha_insert_chan (ChannelInfo * ci);
static ChannelInfo *makechan (const char *chan);
static int is_identified (User * user, ChannelInfo * ci);

static char *oldmodeconv (short inmode);
static void do_help (const char *source);
static void do_register (const char *source);
static void do_identify (const char *source);
static void do_drop (const char *source);
static void do_set (const char *source);
static void do_set_founder (User * u, ChannelInfo * ci, char *param);
static void do_set_password (User * u, ChannelInfo * ci, char *param);
static void do_set_desc (User * u, ChannelInfo * ci, char *param);
static void do_set_topic (User * u, ChannelInfo * ci, char *param);
static void do_set_url (User * u, ChannelInfo * ci, char *param);
static void do_set_mlock (User * u, ChannelInfo * ci, char *param);
static void do_set_keeptopic (User * u, ChannelInfo * ci, char *param);
static void do_set_topiclock (User * u, ChannelInfo * ci, char *param);
static void do_set_private (User * u, ChannelInfo * ci, char *param);
static void do_set_secureops (User * u, ChannelInfo * ci, char *param);
static void do_set_restricted (User * u, ChannelInfo * ci, char *param);
static void do_set_secure (User * u, ChannelInfo * ci, char *param);
static void do_set_join (User * u, ChannelInfo * ci, char *param);
static void do_set_revenge (User * u, ChannelInfo * ci, char *param);
static void do_level (const char *source);
static void do_level_list (const char *source, char *chan);
static void do_level_set (const char *source, char *chan);
static void do_level_reset (const char *source, char *chan);
static void do_access (const char *source);
static void do_access_add (const char *source, char *chan);
static void do_access_del (const char *source, char *chan);
static void do_access_list (const char *source, char *chan);
static void do_akick (const char *source);
static void do_akick_add (const char *source, char *chan);
static void do_akick_del (const char *source, char *chan);
static void do_akick_list (const char *source, char *chan);
static int is_on_akick (const char *mask, ChannelInfo * ci);
static void do_info (const char *source);
static void do_list (const char *source);
static void do_users (const char *source);
static void do_invite (const char *source);
static void do_mode (const char *source);
static void do_voice (const char *source);
static void do_devoice (const char *source);
static void do_op (const char *source);
static void do_deop (const char *source);
static void do_unban (const char *source);
static void do_clear (const char *source);
static void do_resync (const char *source);
static void do_getpass (const char *source);
static void do_forbid (const char *source);
static void do_suspend (const char *source);
static void do_unsuspend (const char *source);


/*************************************************************************/
/*************************************************************************/

/* Display total number of registered channels and info about each; or, if
 * a specific channel is given, display information about that channel
 * (like /msg ChanServ INFO <channel>).  If count_only != 0, then only
 * display the number of registered channels (the channel parameter is
 * ignored).
 */

#define CR	printf ("\n");
void
listchans (int count_only, const char *chan)
{
    long count = 0;
    ChannelInfo *ci;
    int i;

    if (count_only)
    {

	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next)
		++count;
	printf (CS_INFO_COUNT, count); CR

    }
    else if (chan)
    {

	NickInfo *ni;
	char *t, buf[BUFSIZE], modes[BUFSIZE];

	if (!(ci = cs_findchan (chan)))
	{
	    printf (CS_NOT_REGISTERED, chan); CR
	    return;
	}
	if (ni = host (findnick (ci->founder)))
	    t = userisnick (ni->nick) ? "ONLINE" : ni->last_usermask;
	else
	    t = NULL;
	if (ci->flags & CI_VERBOTEN) {
	    printf (CS_CANNOT_REGISTER, chan); CR
	}
	else
	{
	    printf (CS_INFO_INTRO, ci->name); CR
	    printf (CS_INFO_FOUNDER, ci->founder,
	   			t ? " (" : "", t ? t : "", t ? ")" : ""); CR
	    printf (CS_INFO_DESC, ci->desc); CR
	    if (ci->url) {
		printf (CS_INFO_URL, ci->url); CR
	    }
	    printf (CS_INFO_REG_TIME, time_ago (ci->time_registered, 1)); CR
	    if (!findchan (ci->name)) {
		printf (CS_INFO_LAST_USED, time_ago (ci->last_used, 1)); CR
	    }
	    if (ci->last_topic)
	    {
		if (ci->flags & CI_SUSPENDED)
		{
		    printf (CS_INFO_SUSPENDED, ci->last_topic); CR
		    printf (CS_INFO_SUSPENDER, ci->last_topic_setter); CR
		}
		else
		{
		    printf (CS_INFO_TOPIC, ci->last_topic); CR
		    printf (CS_INFO_TOPIC_SET, ci->last_topic_setter); CR
		}
	    }
	    *buf = 0;
	    if (ci->flags & CI_SUSPENDED)
		strcpy (buf, CS_FLAG_SUSPENDED);
	    else
	    {
		if (ci->flags & CI_PRIVATE)
		    strcpy (buf, CS_FLAG_PRIVATE);
		if (ci->flags & CI_KEEPTOPIC)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_KEEPTOPIC);
		}
		if (ci->flags & CI_TOPICLOCK)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_TOPICLOCK);
		}
		if (ci->flags & CI_SECUREOPS)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_SECUREOPS);
		}
		if (ci->flags & CI_RESTRICTED)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_RESTRICTED);
		}
		if (ci->flags & CI_SECURE)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_SECURE);
		}
		if (!(*buf))
		    strcpy (buf, CS_FLAG_NONE);
	    }
	    printf(CS_INFO_OPTIONS, buf); CR
	    *buf = *modes = 0;
	    if (strlen(ci->mlock_on))
		sprintf (modes, "+%s", ci->mlock_on);
	    strcat(buf, modes); *modes = 0;
	    if (strlen(ci->mlock_off))
		sprintf (modes, "-%s", ci->mlock_off);
	    strcat(buf, modes); *modes = 0;
	    if (ci->mlock_key)
		sprintf (modes, " %s", ci->mlock_key);
	    strcat(buf, modes); *modes = 0;
	    if (ci->mlock_limit)
		sprintf (modes, " %ld", ci->mlock_limit);
	    strcat(buf, modes);
	    printf (CS_INFO_MLOCK, buf); CR
	    if (findchan (ci->name)) {
		printf (CS_IN_USE, ci->name); CR
	    }
	}

    }
    else
    {

	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next)
	    {
		printf ("%-20s  %s\n", ci->name,
			ci->flags & CI_VERBOTEN ? "Disallowed (FORBID)"
			: ci->flags & CI_SUSPENDED ? "Disallowed (SUSPEND)"
			: ci->desc);
		++count;
	    }
	printf (CS_INFO_COUNT, count);
    }
}

/*************************************************************************/

/* Return information on memory use.  Assumes pointers are valid. */

void
get_chanserv_stats (long *nrec, long *memuse)
{
    long count = 0, mem = 0;
    int i, j;
    ChannelInfo *ci;

    for (i = 0; i < 256; i++)
	for (ci = chanlists[i]; ci; ci = ci->next)
	{
	    count++;
	    mem += sizeof (*ci);
	    if (ci->desc)
		mem += strlen (ci->desc) + 1;
	    mem += ci->accesscount * sizeof (ChanAccess);
	    mem += ci->akickcount * sizeof (AutoKick);
	    for (j = 0; j < ci->akickcount; j++)
	    {
		if (ci->akick[j].name)
		    mem += strlen (ci->akick[j].name) + 1;
		if (ci->akick[j].reason)
		    mem += strlen (ci->akick[j].reason) + 1;
	    }
	    if (ci->mlock_key)
		mem += strlen (ci->mlock_key) + 1;
	    if (ci->last_topic)
		mem += strlen (ci->last_topic) + 1;
	    if (ci->cmd_access)
		mem += sizeof (*ci->cmd_access) * CA_SIZE;
	}
    *nrec = count;
    *memuse = mem;
}

/*************************************************************************/
/*************************************************************************/

/* Main ChanServ routine. */

void
chanserv (const char *source, char *buf)
{
    char *cmd, *s;
    cmd = strtok (buf, " ");

    if (!cmd)
	return;

    else if (stricmp (cmd, "\1PING") == 0)
    {
	if (!(s = strtok (NULL, "")))
	    s = "\1";
	notice (s_ChanServ, source, "\1PING %s", s);

    }
    else
    {
	Hash *command, hash_table[] =
	{
	    {"HELP", H_NONE, do_help},
	    {"REG*", H_NONE, do_register},
	    {"ID*", H_NONE, do_identify},
	    {"DROP", H_NONE, do_drop},
	    {"SET*", H_NONE, do_set},
	    {"ACC*", H_NONE, do_access},
	    {"A*KICK", H_NONE, do_akick},
	    {"INV*", H_NONE, do_invite},
	    {"*MODE", H_NONE, do_mode},
	    {"UNBAN", H_NONE, do_unban},
	    {"*INFO", H_NONE, do_info},
	    {"LIST", H_NONE, do_list},
	    {"USER*", H_NONE, do_users},
	    {"WHO*", H_NONE, do_users},
	    {"V*", H_NONE, do_voice},
	    {"DV*", H_NONE, do_devoice},
	    {"DEV*", H_NONE, do_devoice},
	    {"OP*", H_NONE, do_op},
	    {"DOP*", H_NONE, do_deop},
	    {"DEOP*", H_NONE, do_deop},
	    {"CLEAR", H_NONE, do_clear},
	    {"LEVEL*", H_NONE, do_level},
	    {"*SYNC*", H_NONE, do_resync},
	    {"GETPASS", H_SOP, do_getpass},
	    {"FORBID", H_SOP, do_forbid},
	    {"SUSPEND", H_SOP, do_suspend},
	    {"UNSUSPEND", H_SOP, do_unsuspend},
	    {NULL}
	};

	if (command = get_hash (source, cmd, hash_table))
	{
	    if(akick_max < 1 && command->process == do_akick)
		notice (s_ChanServ, source, ERR_UNKNOWN_COMMAND, cmd, s_ChanServ);
	    else
		(*command->process) (source);
	}
	else
	    notice (s_ChanServ, source, ERR_UNKNOWN_COMMAND, cmd, s_ChanServ);
    }
}

/*************************************************************************/


/* Load/save data files. */

void
load_cs_dbase (void)
{
    FILE *f = fopen (chanserv_db, "r");
    int i, j;
    ChannelInfo *ci;

    if (!f)
    {
	log_perror ("Can't read ChanServ database %s", chanserv_db);
	return;
    }

    switch (i = get_file_version (f, chanserv_db))
    {
    case 5:
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ci = smalloc (sizeof (ChannelInfo));
		if (1 != fread (ci, sizeof (ChannelInfo), 1, f))
		    fatal_perror ("Read error on %s", chanserv_db);
		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan (ci);
		ci->desc = read_string (f, chanserv_db);
		if (ci->url)
		    ci->url = read_string (f, chanserv_db);
		if (ci->mlock_key)
		    ci->mlock_key = read_string (f, chanserv_db);
		if (ci->last_topic)
		    ci->last_topic = read_string (f, chanserv_db);

		if (ci->accesscount)
		{
		    ChanAccess *access;
		    access = smalloc (sizeof (ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread (access, sizeof (ChanAccess),
						  ci->accesscount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string (f, chanserv_db);
		    j = 0;
		    access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount)
		    {
			if (access->is_nick < 0)
			{
			    --ci->accesscount;
			    free (access->name);
			    if (j < ci->accesscount)
				bcopy (access + 1, access, sizeof (*access) *
				       (ci->accesscount - j));
			}
			else
			{
			    ++j;
			    ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc (ci->access,
				     sizeof (ChanAccess) * ci->accesscount);
		    else
		    {
			free (ci->access);
			ci->access = NULL;
		    }
		}		/* if (ci->accesscount) */

		if (ci->akickcount)
		{
		    AutoKick *akick;
		    akick = smalloc (sizeof (AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			fread (akick, sizeof (AutoKick), ci->akickcount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->akickcount; ++j, ++akick)
		    {
			akick->name = read_string (f, chanserv_db);
			if (akick->reason)
			    akick->reason = read_string (f, chanserv_db);
		    }
		    j = 0;
		    akick = ci->akick;
		    while (j < ci->akickcount)
		    {
			if (akick->is_nick < 0)
			{
			    --ci->akickcount;
			    free (akick->name);
			    if (akick->reason)
				free (akick->reason);
			    if (j < ci->akickcount)
				bcopy (akick + 1, akick, sizeof (*akick) *
				       (ci->akickcount - j));
			}
			else
			{
			    ++j;
			    ++akick;
			}
		    }
		    if (ci->akickcount)
		    {
			ci->akick = srealloc (ci->akick,
					sizeof (AutoKick) * ci->akickcount);
		    }
		    else
		    {
			free (ci->akick);
			ci->akick = NULL;
		    }
		}		/* if (ci->akickcount) */

		if (ci->cmd_access)
		{
		    int n_entries;
		    ci->cmd_access = smalloc (sizeof (short) * CA_SIZE);
		    n_entries = fgetc (f) << 8 | fgetc (f);
		    if (n_entries < 0)
			fatal_perror ("Read error on %s", chanserv_db);
		    if (n_entries <= CA_SIZE)
		    {
			fread (ci->cmd_access, sizeof (short), n_entries, f);
		    }
		    else
		    {
			fread (ci->cmd_access, sizeof (short), CA_SIZE, f);
			fseek (f, sizeof (short) * (n_entries - CA_SIZE),
			       SEEK_CUR);
		    }
		}
		if (!ci->cmd_access)
		    ci->cmd_access = def_access;
	    }			/* while (fgetc(f) == 1) */
	break;			/* case 5, etc. */

    case 4:
    case 3:
	{
	ChannelInfo_V3 *old_ci;
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ci = smalloc (sizeof (ChannelInfo));
		old_ci = smalloc (sizeof (ChannelInfo_V3));
		if (1 != fread (old_ci, sizeof (ChannelInfo_V3), 1, f))
		    fatal_perror ("Read error on %s", chanserv_db);
		/* Convert old dbase! */
		strscpy(ci->mlock_on, oldmodeconv(old_ci->mlock_on), sizeof(ci->mlock_on));
		strscpy(ci->mlock_off, oldmodeconv(old_ci->mlock_off), sizeof(ci->mlock_off));
		strscpy(ci->name, old_ci->name, sizeof(ci->name));
		strscpy(ci->founder, old_ci->founder, sizeof(ci->founder));
		strscpy(ci->founderpass, old_ci->founderpass,
						sizeof(ci->founderpass));
		strscpy(ci->last_topic_setter, old_ci->last_topic_setter,
					sizeof(ci->last_topic_setter));
		ci->time_registered = old_ci->time_registered;
		ci->last_used = old_ci->last_used;
		ci->accesscount = old_ci->accesscount;
		ci->akickcount = old_ci->akickcount;
		ci->mlock_limit = old_ci->mlock_limit;
		ci->last_topic_time = old_ci->last_topic_time;
		ci->flags = old_ci->flags;

		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan (ci);
		old_ci->desc = read_string (f, chanserv_db);
		ci->desc = sstrdup(old_ci->desc);
		free (old_ci->desc);
		if (old_ci->url)
		{
		    old_ci->url = read_string (f, chanserv_db);
		    if (strlen(old_ci->url) > 0)
			ci->url = sstrdup(old_ci->url);
		    else
			ci->url = NULL;
		    free (old_ci->url);
		}
		else
		    ci->url = NULL;
		if (old_ci->mlock_key)
		{
		    old_ci->mlock_key = read_string (f, chanserv_db);
		    ci->mlock_key = sstrdup(old_ci->mlock_key);
		    free (old_ci->mlock_key);
		}
		else
		    ci->mlock_key = NULL;
		if (old_ci->last_topic)
		{
		    old_ci->last_topic = read_string (f, chanserv_db);
		    ci->last_topic = sstrdup(old_ci->last_topic);
		    free (old_ci->last_topic);
		}
		else
		    ci->last_topic = NULL;

		if (ci->accesscount)
		{
		    ChanAccess *access;
		    access = smalloc (sizeof (ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread (access, sizeof (ChanAccess),
						  ci->accesscount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string (f, chanserv_db);
		    j = 0;
		    access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount)
		    {
			if (access->is_nick < 0)
			{
			    --ci->accesscount;
			    free (access->name);
			    if (j < ci->accesscount)
				bcopy (access + 1, access, sizeof (*access) *
				       (ci->accesscount - j));
			}
			else
			{
			    ++j;
			    ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc (ci->access,
				     sizeof (ChanAccess) * ci->accesscount);
		    else
		    {
			free (ci->access);
			ci->access = NULL;
		    }
		}		/* if (ci->accesscount) */

		if (ci->akickcount)
		{
		    AutoKick *akick;
		    akick = smalloc (sizeof (AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			fread (akick, sizeof (AutoKick), ci->akickcount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->akickcount; ++j, ++akick)
		    {
			akick->name = read_string (f, chanserv_db);
			if (akick->reason)
			    akick->reason = read_string (f, chanserv_db);
		    }
		    j = 0;
		    akick = ci->akick;
		    while (j < ci->akickcount)
		    {
			if (akick->is_nick < 0)
			{
			    --ci->akickcount;
			    free (akick->name);
			    if (akick->reason)
				free (akick->reason);
			    if (j < ci->akickcount)
				bcopy (akick + 1, akick, sizeof (*akick) *
				       (ci->akickcount - j));
			}
			else
			{
			    ++j;
			    ++akick;
			}
		    }
		    if (ci->akickcount)
		    {
			ci->akick = srealloc (ci->akick,
					sizeof (AutoKick) * ci->akickcount);
		    }
		    else
		    {
			free (ci->akick);
			ci->akick = NULL;
		    }
		}		/* if (ci->akickcount) */

		if (old_ci->cmd_access)
		{
		    int n_entries;
		    ci->cmd_access = smalloc (sizeof (short) * CA_SIZE);
		    n_entries = fgetc (f) << 8 | fgetc (f);
		    if (n_entries < 0)
			fatal_perror ("Read error on %s", chanserv_db);
		    if (n_entries <= CA_SIZE)
		    {
			fread (ci->cmd_access, sizeof (short), n_entries, f);
		    }
		    else
		    {
			fread (ci->cmd_access, sizeof (short), CA_SIZE, f);
			fseek (f, sizeof (short) * (n_entries - CA_SIZE),
			       SEEK_CUR);
		    }
		}
		if (!ci->cmd_access)
		    ci->cmd_access = def_access;
		free (old_ci);
	    }			/* while (fgetc(f) == 1) */
	break;			/* case 3, etc. */
	}
    case 2:
    case 1:
	{
	ChannelInfo_V1 *old_ci;
	for (i = 33; i < 256; ++i)
	    while (fgetc (f) == 1)
	    {
		ci = smalloc (sizeof (ChannelInfo));
		old_ci = smalloc (sizeof (ChannelInfo_V1));
		if (1 != fread (old_ci, sizeof (ChannelInfo_V1), 1, f))
		    fatal_perror ("Read error on %s", chanserv_db);
		/* Convert old dbase! */
		strscpy(ci->mlock_on, oldmodeconv(old_ci->mlock_on), sizeof(ci->mlock_on));
		strscpy(ci->mlock_off, oldmodeconv(old_ci->mlock_off), sizeof(ci->mlock_off));
		strscpy(ci->name, old_ci->name, sizeof(ci->name));
		strscpy(ci->founder, old_ci->founder, sizeof(ci->founder));
		strscpy(ci->founderpass, old_ci->founderpass,
						sizeof(ci->founderpass));
		strscpy(ci->last_topic_setter, old_ci->last_topic_setter,
					sizeof(ci->last_topic_setter));
		ci->time_registered = old_ci->time_registered;
		ci->last_used = old_ci->last_used;
		ci->accesscount = old_ci->accesscount;
		ci->akickcount = old_ci->akickcount;
		ci->mlock_limit = old_ci->mlock_limit;
		ci->last_topic_time = old_ci->last_topic_time;
		ci->flags = old_ci->flags;
		ci->url = NULL;

		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan (ci);
		old_ci->desc = read_string (f, chanserv_db);
		ci->desc = sstrdup(old_ci->desc);
		free (old_ci->desc);
		if (old_ci->mlock_key)
		{
		    old_ci->mlock_key = read_string (f, chanserv_db);
		    ci->mlock_key = sstrdup(old_ci->mlock_key);
		    free (old_ci->mlock_key);
		}
		else
		    ci->mlock_key = NULL;
		if (old_ci->last_topic)
		{
		    old_ci->last_topic = read_string (f, chanserv_db);
		    ci->last_topic = sstrdup(old_ci->last_topic);
		    free (old_ci->last_topic);
		}
		else
		    ci->last_topic = NULL;

		if (ci->accesscount)
		{
		    ChanAccess *access;
		    access = smalloc (sizeof (ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread (access, sizeof (ChanAccess),
						  ci->accesscount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string (f, chanserv_db);
		    j = 0;
		    access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount)
		    {
			if (access->is_nick < 0)
			{
			    --ci->accesscount;
			    free (access->name);
			    if (j < ci->accesscount)
				bcopy (access + 1, access, sizeof (*access) *
				       (ci->accesscount - j));
			}
			else
			{
			    ++j;
			    ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc (ci->access,
				     sizeof (ChanAccess) * ci->accesscount);
		    else
		    {
			free (ci->access);
			ci->access = NULL;
		    }
		}		/* if (ci->accesscount) */

		if (ci->akickcount)
		{
		    AutoKick *akick;
		    akick = smalloc (sizeof (AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			fread (akick, sizeof (AutoKick), ci->akickcount, f))
			fatal_perror ("Read error on %s", chanserv_db);
		    for (j = 0; j < ci->akickcount; ++j, ++akick)
		    {
			akick->name = read_string (f, chanserv_db);
			if (akick->reason)
			    akick->reason = read_string (f, chanserv_db);
		    }
		    j = 0;
		    akick = ci->akick;
		    while (j < ci->akickcount)
		    {
			if (akick->is_nick < 0)
			{
			    --ci->akickcount;
			    free (akick->name);
			    if (akick->reason)
				free (akick->reason);
			    if (j < ci->akickcount)
				bcopy (akick + 1, akick, sizeof (*akick) *
				       (ci->akickcount - j));
			}
			else
			{
			    ++j;
			    ++akick;
			}
		    }
		    if (ci->akickcount)
		    {
			ci->akick = srealloc (ci->akick,
					sizeof (AutoKick) * ci->akickcount);
		    }
		    else
		    {
			free (ci->akick);
			ci->akick = NULL;
		    }
		}		/* if (ci->akickcount) */

		if (old_ci->cmd_access)
		{
		    int n_entries;
		    ci->cmd_access = smalloc (sizeof (short) * CA_SIZE);
		    n_entries = fgetc (f) << 8 | fgetc (f);
		    if (n_entries < 0)
			fatal_perror ("Read error on %s", chanserv_db);
		    if (n_entries <= CA_SIZE)
		    {
			fread (ci->cmd_access, sizeof (short), n_entries, f);
		    }
		    else
		    {
			fread (ci->cmd_access, sizeof (short), CA_SIZE, f);
			fseek (f, sizeof (short) * (n_entries - CA_SIZE),
			       SEEK_CUR);
		    }
		}
		if (!ci->cmd_access)
		    ci->cmd_access = def_access;
		free (old_ci);
	    }
	break;			/* case 1, etc. */
	}
    default:
	fatal ("Unsupported version number (%d) on %s", i, chanserv_db);
    }				/* switch (version) */
    fclose (f);
}

static char *
oldmodeconv (short inmode)
{
    static char outmode[MODEMAX];
    strscpy (outmode, "", sizeof(outmode));
    if (inmode & 0x01) strcat(outmode, "i");
    if (inmode & 0x02) strcat(outmode, "m");
    if (inmode & 0x04) strcat(outmode, "n");
    if (inmode & 0x08) strcat(outmode, "p");
    if (inmode & 0x10) strcat(outmode, "s");
    if (inmode & 0x20) strcat(outmode, "t");
    if (inmode & 0x40) strcat(outmode, "k");
    if (inmode & 0x80) strcat(outmode, "l");
    return outmode;
}

/*************************************************************************/

void
save_cs_dbase (void)
{
    FILE *f;
    int i, j;
    ChannelInfo *ci;
    char chanservsave[2048];

    strcpy (chanservsave, chanserv_db);
    strcat (chanservsave, ".save");
    remove (chanservsave);
    if (rename (chanserv_db, chanservsave) < 0)
	log_perror ("Can't back up %s", chanserv_db);
    f = fopen (chanserv_db, "w");
    if (!f)
    {
	log_perror ("Can't write to ChanServ database %s", chanserv_db);
	if (rename (chanservsave, chanserv_db) < 0)
	    fatal_perror ("Can't restore backup copy of %s", chanserv_db);
	return;
    }
#ifndef WIN32
    if (fchown (fileno (f), -1, file_gid) < 0)
	log_perror ("Can't set group of %s to %d", chanserv_db, file_gid);
#endif
    write_file_version (f, chanserv_db);

    for (i = 33; i < 256; ++i)
    {
	for (ci = chanlists[i]; ci; ci = ci->next)
	{
	    fputc (1, f);
	    if (1 != fwrite (ci, sizeof (ChannelInfo), 1, f))
		fatal_perror ("Write error on %s", chanserv_db);
	    write_string (ci->desc ? ci->desc : "", f, chanserv_db);
	    if (ci->url)
		write_string (ci->url, f, chanserv_db);
	    if (ci->mlock_key)
		write_string (ci->mlock_key, f, chanserv_db);
	    if (ci->last_topic)
		write_string (ci->last_topic, f, chanserv_db);

	    if (ci->accesscount)
	    {
		ChanAccess *access = ci->access;
		if (ci->accesscount !=
		    fwrite (access, sizeof (ChanAccess), ci->accesscount, f))
		    fatal_perror ("Write error on %s", chanserv_db);
		for (j = 0; j < ci->accesscount; ++j, ++access)
		    write_string (access->name, f, chanserv_db);
	    }
	    if (ci->akickcount)
	    {
		AutoKick *akick = ci->akick;
		if (ci->akickcount !=
		    fwrite (akick, sizeof (AutoKick), ci->akickcount, f))
		    fatal_perror ("Write error on %s", chanserv_db);
		for (j = 0; j < ci->akickcount; ++j, ++akick)
		{
		    write_string (akick->name, f, chanserv_db);
		    if (akick->reason)
			write_string (akick->reason, f, chanserv_db);
		}
	    }
	    if (ci->cmd_access)
	    {
		fputc (CA_SIZE >> 8, f);
		fputc (CA_SIZE & 0xFF, f);
		fwrite (ci->cmd_access, sizeof (short), CA_SIZE, f);
	    }
	}			/* for (chanlists[i]) */
	fputc (0, f);
    }				/* for (i) */

    fclose (f);
    remove (chanservsave);
}

/*************************************************************************/

/* Check the current modes on a channel; if they conflict with a mode lock,
 * fix them. */

void
check_modes (const char *chan)
{
    Channel *c = findchan (chan);
    ChannelInfo *ci = cs_findchan (chan);
    char new_on[MODEMAX], new_off[MODEMAX], *newkey = NULL;
    long newlimit = 0;
    int set_limit = 0, set_key = 0;
    char buf[64];

    if (!c || !ci)
	return;

    if (ci->flags & CI_VERBOTEN)
	return;

    strscpy(new_on, strlen(ci->mlock_on) ? changemode(ci->mlock_on, c->mode) : "", sizeof(new_on));
    strscpy(new_off, strlen(ci->mlock_off) ? ci->mlock_off : "", sizeof(new_off));

    if (ci->mlock_limit && c->limit != ci->mlock_limit)
    {
	change_cmode (s_ChanServ, c->name, "-l", "");
	c->limit = newlimit;
	newlimit = ci->mlock_limit;
	set_limit = 1;
    }
    if (ci->mlock_key && (!c->key || strcmp(c->key, ci->mlock_key)!=0)) {
	if (c->key) {
	    change_cmode (s_ChanServ, c->name, "-k", c->key);
	    free (c->key);
	}
	newkey = ci->mlock_key;
	c->key = sstrdup (newkey);
	set_key = 1;
    }

    if (c->limit && hasmode("l", ci->mlock_off))
    {
	c->limit = 0;
    }
    if (c->key && hasmode("k", ci->mlock_off))
    {
	newkey = sstrdup (c->key);
	free (c->key);
	c->key = NULL;
	set_key = 1;
    }

    if (!i_am_backup () && (strlen(new_off) + strlen(new_on)))
    {
	strscpy(new_on, changemode("-lk", new_on), sizeof(new_on));
	strscpy(new_off, changemode("-k", new_off), sizeof(new_off));
	if (set_limit)
	{
	    if (set_key) {
		char *av[4];
		snprintf (buf, sizeof(buf), "+l%s%s%s%s%s",
			c->key ? "k" : "",
			strlen(new_on) ? new_on : "",
			(strlen(new_off) || !c->key) ? "-" : "",
			!c->key ? "k" : "",
			strlen(new_off) ? new_off : "");

		av[1] = sstrdup (c->name);
		av[2] = sstrdup (buf);
		av[3] = sstrdup (myitoa(newlimit));
		av[4] = sstrdup (newkey ? newkey : "");
		do_cmode (s_ChanServ, 4, av);
		free (av[1]);
		free (av[2]);
		free (av[3]);
		free (av[4]);

	    } else {
		snprintf (buf, sizeof(buf), "+l%s%s%s",
			strlen(new_on) ? new_on : "",
			strlen(new_off) ? "-" : "",
			strlen(new_off) ? new_off : "");
		change_cmode (s_ChanServ, c->name, buf, myitoa(newlimit));
	    }
	}
	else if (set_key)
	{
	    snprintf(buf, sizeof(buf), "%s%s%s%s%s%s",
		(strlen(new_on) || c->key) ? "+" : "",
		c->key ? "k" : "",
		strlen(new_on) ? new_on : "",
		(strlen(new_off) || !c->key) ? "-" : "",
		!c->key ? "k" : "",
		strlen(new_off) ? new_off : "");
	    change_cmode (s_ChanServ, c->name, buf, newkey ? newkey : "");
	}
	else
	{
	    snprintf (buf, sizeof (buf), "%s%s%s%s",
		strlen(new_on) ? "+" : "",
		strlen(new_on) ? new_on : "",
		strlen(new_off) ? "-" : "",
		strlen(new_off) ? new_off : "");
	    change_cmode (s_ChanServ, c->name, buf, "");
	}
    }
    if (newkey && !(c->key))
	free (newkey);
}

/*************************************************************************/

/* Check whether a user is allowed to be opped on a channel; if they
 * aren't, deop them.  If serverop is 1, the +o was done by a server.
 * Return 1 if the user is allowed to be opped, 0 otherwise. */

int
check_valid_op (User * user, const char *chan, int serverop)
{
    ChannelInfo *ci = cs_findchan (chan);

    if (!ci || i_am_backup () || (ci->flags & CI_LEAVEOPS))
	return 1;

    if (override (user->nick, CO_OPER))
	return 1;

    if (!is_chanop (user->nick, chan))
	return 0;

    if ((ci->flags & CI_SUSPENDED) && !override (user->nick, CO_SOP))
    {
	notice (s_ChanServ, user->nick, CS_ERR_SUSPENDED, chan);
	change_cmode (s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (ci->flags & CI_VERBOTEN)
    {
	if (is_oper (user->nick))
	    return 1;
	change_cmode (s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (serverop && !check_access (user, ci, CA_AUTOOP))
    {
	notice (s_ChanServ, user->nick, CS_ERR_REGISTERED, chan);
	change_cmode (s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    if (get_access (user, ci) <= ((ci->flags & CI_SECUREOPS) ? 0 : ci->cmd_access[CA_AUTODEOP]))
    {
	change_cmode (s_ChanServ, chan, "-o", user->nick);
	return 0;
    }

    return 1;
}

/*************************************************************************/

/* Check whether a user is allowed to be voiced on a channel; if they
 * aren't, devoice them.  If servervoice is 1, the +v was done by a server.
 * Return 1 if the user is allowed to be voiced, 0 otherwise. */

int
check_valid_voice (User * user, const char *chan, int servervoice)
{
    ChannelInfo *ci = cs_findchan (chan);

    if (!ci || i_am_backup () || (ci->flags & CI_LEAVEOPS))
	return 1;

    if (override (user->nick, CO_OPER))
	return 1;

    if (!is_voiced (user->nick, chan))
	return 0;

    if ((ci->flags & CI_SUSPENDED) && !override (user->nick, CO_OPER))
    {
	notice (s_ChanServ, user->nick, CS_ERR_SUSPENDED, chan);
	change_cmode (s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    if (servervoice && !check_access (user, ci, CA_AUTOVOICE))
    {
	notice (s_ChanServ, user->nick, CS_ERR_REGISTERED, chan);
	change_cmode (s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    if (get_access (user, ci) <= ((ci->flags & CI_SECUREOPS) ? 0 : ci->cmd_access[CA_AUTODEOP]))
    {
	change_cmode (s_ChanServ, chan, "-v", user->nick);
	return 0;
    }

    return 1;
}

/*************************************************************************/

/* Check whether a user should be opped on a channel, and if so, do it.
 * Return 1 if the user was opped, 0 otherwise.  (Updates the channel's
 * last used time if the user was opped.) */

int
check_should_op (User * user, const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    NickInfo *ni;

    if (!ci || i_am_backup () || ci->flags & CI_VERBOTEN || *chan == '+')
	return 0;

    if (ci->flags & CI_SECURE)
	if (!(ni = findnick (user->nick)) || !(ni->flags & NI_IDENTIFIED))
	    return 0;

    if (check_access (user, ci, CA_AUTOVOICE))
    {
	if (check_access (user, ci, CA_AUTOOP))
	    change_cmode (s_ChanServ, chan, "+o", user->nick);
	else
	    change_cmode (s_ChanServ, chan, "+v", user->nick);
	ci->last_used = time (NULL);
	return 1;
    }

    return 0;
}

/*************************************************************************/

/* Check whether a user is permitted to be on a channel.  If so, return 0;
 * else, kickban the user with an appropriate message (could be either
 * AKICK or restricted access) and return 1.
 */

int
check_kick (User * user, const char *chan)
{
    int Result;
    ChannelInfo *ci = cs_findchan (chan);
    AutoKick *akick;
    int i;
    NickInfo *ni;
    char *mask;

    if (!ci || i_am_backup ())
    {
	Result = 0;
	goto check_kick_END;
    }

    if (override (user->nick, CO_OPER))
    {
	Result = 0;
	goto check_kick_END;
    }

    if ((ci->flags & CI_VERBOTEN))
    {
	if (is_oper (user->nick))
	{
	    Result = 0;
	    goto check_kick_END;
	}
	mask = create_mask (user);
	change_cmode (s_ChanServ, chan, "+b", mask);
	free(mask);
	kick_user (s_ChanServ, chan, user->nick, CS_GET_OUT);
	Result = 1;
	goto check_kick_END;
    }

    if (ci->flags & CI_SUSPENDED)
    {
	Result = 0;
	goto check_kick_END;
    }
    ni = host (findnick (user->nick));

    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
	if ((akick->is_nick > 0 && userisnick (user->nick) &&
	     stricmp (ni->nick, akick->name) == 0) ||
	    (!akick->is_nick && match_usermask (akick->name, user)))
	{
	    mask = create_mask (user);
	    change_cmode (s_ChanServ, chan, "+b", akick->is_nick > 0 ?
			  mask : akick->name);
	    free(mask);
	    kick_user (s_ChanServ, chan, user->nick,
		       akick->reason ? akick->reason : def_akick_reason);
	    Result = 1;
	    goto check_kick_END;
	}

    if (!(ci->flags & CI_RESTRICTED))
    {
	Result = 0;
	goto check_kick_END;
    }

    if (get_access (user, ci) <= (ci->flags & CI_SECUREOPS ? 0 : ci->cmd_access[CA_AUTODEOP]))
    {
	mask = create_mask (user);
	change_cmode (s_ChanServ, chan, "+b", mask);
	free(mask);
	kick_user (s_ChanServ, chan, user->nick, CS_GET_OUT);
	Result = 1;
	goto check_kick_END;
    }

    Result = 0;
  check_kick_END:
    return Result;
}

/*************************************************************************/

/* Record the current channel topic in the ChannelInfo structure. */

void
record_topic (const char *chan)
{
    if (services_level == 1)
    {
	Channel *c = findchan (chan);
	ChannelInfo *ci = cs_findchan (chan);

	if (!c || !ci || (ci->flags & CI_SUSPENDED))
	    return;
	if (ci->last_topic)
	    free (ci->last_topic);
	if (c->topic)
	    ci->last_topic = sstrdup (c->topic);
	else
	    ci->last_topic = NULL;
	strscpy (ci->last_topic_setter, c->topic_setter, NICKMAX);
	ci->last_topic_time = c->topic_time;
    }
}

/*************************************************************************/

void
do_cs_protect (const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    if (ci && (ci->mlock_key || hasmode("i", ci->mlock_on)))
    {
	send_cmd (s_ChanServ, "JOIN %s", chan);
	send_cmd (s_ChanServ, "MODE %s +s%s%s :%s", chan,
		  hasmode("i", ci->mlock_on) ? "i" : "",
		  ci->mlock_key ? "k" : "",
		  ci->mlock_key ? ci->mlock_key : "");
    }
}
void
do_cs_unprotect (const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    if (ci && (ci->mlock_key || hasmode("i", ci->mlock_on)))
    {
	if (!hasmode("s", ci->mlock_on))
	    send_cmd (s_ChanServ, "MODE %s -s", chan);
	if (!(ci->flags & CI_JOIN))
	    send_cmd (s_ChanServ, "PART %s", chan);
    }
}


/* Join channel with JOIN on and op self */
void
do_cs_join (const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    if (ci && (ci->flags & CI_JOIN))
    {
	send_cmd (s_ChanServ, "JOIN %s", chan);
	send_cmd (s_ChanServ, "MODE %s +o %s", chan, s_ChanServ);
    }
}

/* Join channel with JOIN on and op self */
void
do_cs_part (const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    if (ci && (ci->flags & CI_JOIN))
	send_cmd (s_ChanServ, "PART %s", chan);
}

/* Join channel with JOIN on and op self */
void
do_cs_reop (const char *chan)
{
    ChannelInfo *ci = cs_findchan (chan);
    if (ci && (ci->flags & CI_JOIN))
	send_cmd (s_ChanServ, "MODE %s +o %s", chan, s_ChanServ);
}

int
do_cs_revenge (const char *chan, const char *actioner, const char *actionee,
	       int action)
{

    ChannelInfo *ci;
    User *user;

    if ((ci = cs_findchan (chan)) && (user = finduser (actioner)) &&
	!is_services_nick (actioner))
    {
	int setlev = 0, actlev = -1;

	/* Find out the level of the ACTIONER (setlev) */
	setlev = get_access (user, ci);

	/* Find out the level of the ACTIONEE (actlev) */
	user = finduser (actionee);
	actlev = get_access (user, ci);

	/* Unban if the ACTIONEE is HIGHER or EQUAL TO the ACTIONER */
	if (runflags & RUN_DEBUG)
	    write_log ("debug: actionee level is %d (%s), actioner is %d (%s)",
		       actlev, actionee, setlev, actioner);
	if (actlev > setlev)
	{
	    if (get_revenge_level (ci))
		do_revenge (chan, actioner, actionee, action);
	    return 1;
	}
    }
    return 0;
}

void
do_revenge (const char *chan, const char *actioner, const char *actionee,
	    int action)
{

    char buf[25 + NICKMAX], *mask;
    ChannelInfo *ci = cs_findchan (chan);
    User *user = finduser (actioner);

    if (!ci || !user || action == CR_NONE)
	return;
    if (override (actioner, CO_OPER))
	return;

    if (action == CR_DEOP)
	snprintf (buf, sizeof (buf), CS_REV_DEOP, actionee);
    else if (action == CR_KICK)
	snprintf (buf, sizeof (buf), CS_REV_KICK, actionee);
    else if (action == CR_NICKBAN || action == CR_USERBAN ||
	     action == CR_HOSTBAN)
	snprintf (buf, sizeof (buf), CS_REV_BAN, actionee);

    if (get_revenge_level (ci) == CR_DEOP)
	change_cmode (s_ChanServ, chan, "-o", actioner);

    else if (get_revenge_level (ci) == CR_KICK)
	kick_user (s_ChanServ, ci->name, actioner, buf);

    else if (get_revenge_level (ci) == CR_NICKBAN)
    {
	mask=smalloc(strlen (user->nick) + 5);
	snprintf (mask, strlen (user->nick) + 5, "%s!*@*", user->nick);
	change_cmode (s_ChanServ, chan, "+b", mask);
	kick_user (s_ChanServ, ci->name, actioner, buf);
	free(mask);
    }
    else if (get_revenge_level (ci) == CR_USERBAN)
    {
	mask = create_mask (user);
	change_cmode (s_ChanServ, chan, "+b", mask);
	kick_user (s_ChanServ, ci->name, actioner, buf);
	free(mask);

    }
    else if (get_revenge_level (ci) == CR_HOSTBAN)
    {
	mask=smalloc(strlen (user->host) + 5);
	snprintf (mask, strlen (user->host) + 5, "*!*@%s", user->host);
	change_cmode (s_ChanServ, chan, "+b", mask);
	kick_user (s_ChanServ, ci->name, actioner, buf);
	free(mask);
    }
    else if (get_revenge_level (ci) == CR_MIRROR)
    {

	if (action == CR_DEOP)
	    change_cmode (s_ChanServ, chan, "-o", actioner);

	else if (action == CR_KICK)
	    kick_user (s_ChanServ, ci->name, actioner, buf);

	else if (action == CR_NICKBAN)
	{
	    mask=smalloc(strlen(user->nick) + 5);
	    snprintf (mask, strlen (user->nick) + 5, "%s!*@*", user->nick);
	    change_cmode (s_ChanServ, chan, "+b", mask);
	    kick_user (s_ChanServ, ci->name, actioner, buf);
	    free(mask);
	}
	else if (action == CR_USERBAN)
	{
	    mask = create_mask (user);
	    change_cmode (s_ChanServ, chan, "+b", mask);
	    free(mask);
	    kick_user (s_ChanServ, ci->name, actioner, buf);

	}
	else if (action == CR_HOSTBAN)
	{
	    mask=smalloc(strlen (user->host) + 5);
	    snprintf (mask, strlen (user->host) + 5, "*!*@%s", user->host);
	    change_cmode (s_ChanServ, chan, "+b", mask);
	    kick_user (s_ChanServ, ci->name, actioner, buf);
	    free(mask);
	}
    }
}


/*************************************************************************/

/* Restore the topic in a channel when it's created, if we should. */

void
restore_topic (const char *chan)
{
    Channel *c = findchan (chan);
    ChannelInfo *ci = cs_findchan (chan);

    if (!c || !ci || i_am_backup () || !(ci->flags & (CI_KEEPTOPIC | CI_SUSPENDED)))
	return;
    if (c->topic)
	free (c->topic);
    if (ci->last_topic)
    {
	if (ci->flags & CI_SUSPENDED)
	{
	    char *tmp;
	    tmp=smalloc(strlen (ci->last_topic) + 30);
	    snprintf (tmp, strlen (ci->last_topic) + 30, CS_SUSPENDED_TOPIC, ci->last_topic);
	    c->topic = sstrdup (tmp);
	    free(tmp);
	}
	else
	    c->topic = sstrdup (ci->last_topic);
	strscpy (c->topic_setter, ci->last_topic_setter, NICKMAX);
	c->topic_time = ci->last_topic_time;
    }
    else
    {
	c->topic = NULL;
	strscpy (c->topic_setter, s_ChanServ, NICKMAX);
    }
#ifdef IRC_CLASSIC
    send_cmd (s_ChanServ, "TOPIC %s %lu :%s", chan,
	      c->topic_setter, c->topic ? c->topic : "");
#else
    send_cmd (s_ChanServ, "TOPIC %s %s %lu :%s", chan,
	      c->topic_setter, c->topic_time, c->topic ? c->topic : "");
#endif
}

/*************************************************************************/

/* See if the topic is locked on the given channel, and return 1 (and fix
 * the topic) if so. */

int
check_topiclock (const char *chan)
{
    Channel *c = findchan (chan);
    ChannelInfo *ci = cs_findchan (chan);

    if (!c || !ci || i_am_backup () || !(ci->flags & (CI_TOPICLOCK | CI_SUSPENDED)))
	return 0;
    if (c->topic)
	free (c->topic);
    if (ci->last_topic)
    {
	if (ci->flags & CI_SUSPENDED)
	{
	    char *tmp;
	    tmp=smalloc(strlen (ci->last_topic) + 30);
	    snprintf (tmp, strlen (ci->last_topic) + 30, CS_SUSPENDED_TOPIC, ci->last_topic);
	    c->topic = sstrdup (tmp);
	    free(tmp);
	}
	else
	    c->topic = sstrdup (ci->last_topic);
    }
    else
	c->topic = NULL;
    strscpy (c->topic_setter, ci->last_topic_setter, NICKMAX);
    c->topic_time = ci->last_topic_time;
    send_cmd (s_ChanServ, "TOPIC %s %s %lu :%s", chan,
	      c->topic_setter, c->topic_time, c->topic ? c->topic : "");
}

/*************************************************************************/

/* Remove all channels which have expired. */
void
expire_chans ()
{
    ChannelInfo *ci, *ci2;
    int i;
    const time_t expire_time = channel_expire * 24 * 60 * 60;
    time_t now = time (NULL);


    for (i = 33; i < 256; ++i)
    {
	ci = chanlists[i];
	while (ci)
	{
	    if (now - ci->last_used >= expire_time
		&& !(ci->flags & (CI_VERBOTEN | CI_SUSPENDED)))
	    {
		ci2 = ci->next;
		write_log ("Expiring channel %s", ci->name);
		delchan (ci);
		ci = ci2;
	    }
	    else
		ci = ci->next;
	}
    }
}

/*************************************************************************/
/*********************** ChanServ private routines ***********************/
/*************************************************************************/

/* Return the ChannelInfo structure for the given channel, or NULL if the
 * channel isn't registered. */

ChannelInfo *
cs_findchan (const char *chan)
{
    ChannelInfo *ci;

    for (ci = chanlists[tolower (chan[1])]; ci; ci = ci->next)
	if (stricmp (ci->name, chan) == 0)
	    return ci;
    return NULL;
}

/*************************************************************************/

/* Insert a channel alphabetically into the database. */

static void
alpha_insert_chan (ChannelInfo * ci)
{
    ChannelInfo *ci2, *ci3;
    char *chan = ci->name;

    for (ci3 = NULL, ci2 = chanlists[tolower (chan[1])];
	 ci2 && stricmp (ci2->name, chan) < 0;
	 ci3 = ci2, ci2 = ci2->next)
	;
    ci->prev = ci3;
    ci->next = ci2;
    if (!ci3)
	chanlists[tolower (chan[1])] = ci;
    else
	ci3->next = ci;
    if (ci2)
	ci2->prev = ci;
}

/*************************************************************************/

/* Add a channel to the database.  Returns a pointer to the new ChannelInfo
 * structure if the channel was successfully registered, NULL otherwise.
 * Assumes channel does not already exist. */

static ChannelInfo *
makechan (const char *chan)
{
    ChannelInfo *ci;

    ci = scalloc (sizeof (ChannelInfo), 1);
    strscpy (ci->name, chan, CHANMAX);
    ci->time_registered = time (NULL);
    ci->accesscount = ci->akickcount = 0;
    alpha_insert_chan (ci);
    return ci;
}

/*************************************************************************/

/* Remove a channel from the ChanServ database.  Return 1 on success, 0
 * otherwise. */

int
delchan (ChannelInfo * ci)
{
    int i;

    if(news_on==TRUE)
    {
	if (services_level == 1)
	{
	    NewsList *nl;
	    if (nl = find_newslist (ci->name))
		del_newslist (nl);
	}
    }

    do_cs_part (ci->name);
    if (ci->next)
	ci->next->prev = ci->prev;
    if (ci->prev)
	ci->prev->next = ci->next;
    else
	chanlists[tolower (ci->name[1])] = ci->next;
    if (ci->desc)
	free (ci->desc);
    if (ci->url)
	free (ci->url);
    if (ci->mlock_key)
	free (ci->mlock_key);
    if (ci->last_topic)
	free (ci->last_topic);
    for (i = 0; i < ci->accesscount; ++i)
	if (ci->access[i].name)
	    free (ci->access[i].name);
    if (ci->access)
	free (ci->access);
    for (i = 0; i < ci->akickcount; ++i)
    {
	if (ci->akick[i].name)
	    free (ci->akick[i].name);
	if (ci->akick[i].reason)
	    free (ci->akick[i].reason);
    }
    if (ci->akick)
	free (ci->akick);
    free (ci);
    ci = NULL;
    return 1;
}

/*************************************************************************/

/* Does the given user have founder access to the channel? */

int
is_founder (User * user, NickInfo * ni, ChannelInfo * ci)
{
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (ci->flags & CI_SUSPENDED)
	if (override (user->nick, CO_SOP))
	    return 1;
	else
	    return 0;

    if (ni && userisnick (user->nick) && !(ci->flags & CI_SECURE))
	if (issibling (ni, ci->founder))
	    return 1;
    return is_identified (user, ci);
}

int
is_justfounder (NickInfo * ni, ChannelInfo * ci)
{
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (ci->flags & CI_SUSPENDED)
	return 0;

    if (ni && issibling (ni, ci->founder))
	return 1;
    return 0;
}

/*************************************************************************/

/* Has the given user password-identified as founder for the channel? */

static int
is_identified (User * user, ChannelInfo * ci)
{
    struct u_chaninfolist *c;

    if (ci->flags & CI_SUSPENDED)
	if (override (user->nick, CO_SOP))
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

int
get_access (User * user, ChannelInfo * ci)
{
    ChanAccess *access;
    int i, highlev = ci->cmd_access[CA_FLOOR] - 1;
    NickInfo *ni;
    int identified = 0, isnick = 0;

    if (!ci)
	return 0;
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (ci->flags & CI_SUSPENDED)
	if (override (user->nick, CO_SOP))
	    return ci->cmd_access[CA_FOUNDER];
	else if (override (user->nick, CO_OPER))
	    return ci->cmd_access[CA_AUTOVOICE];
	else
	    return 0;

    if ((ni = findnick (user->nick)))	/* Find these first, save CPU */
	if (ni->flags & NI_IDENTIFIED)
	    identified = 1;
    if (userisnick (user->nick))
	isnick = 1;

    if (is_founder (user, findnick (user->nick), ci))
	return ci->cmd_access[CA_FOUNDER];
    for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
    {
	if ((
		access->is_nick > 0 &&
		issibling (findnick (user->nick), access->name) &&
		(
		    (
			(ci->flags & CI_SECURE) && identified
		    ) || (
			     !(ci->flags & CI_SECURE) && isnick
		    )
		)
	    ) || (
		     !access->is_nick &&
		     match_usermask (access->name, user)
	    ))
	    if (access->level > highlev)
		highlev = access->level;
    }
    return (highlev >= ci->cmd_access[CA_FLOOR]) ? highlev : 0;
}

int
check_access (User * user, ChannelInfo * ci, int what)
{
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (get_access (user, ci) >= ci->cmd_access[what])
	return 1;
    return 0;
}

/* Return the access level the given user has on the channel.  If the
 * channel doesn't exist, the user isn't on the access list, return 0 */

int
get_justaccess (const char *name, ChannelInfo * ci)
{
    ChanAccess *access;
    int i, highlev = ci->cmd_access[CA_FLOOR] - 1;

    if (!ci)
	return 0;
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (ci->flags & CI_SUSPENDED)
	return 0;

    if (is_justfounder (findnick (name), ci))
	return ci->cmd_access[CA_FOUNDER];
    for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	if ((
		access->is_nick > 0 &&
		issibling (findnick (name), access->name)
	    ) || (
		     !access->is_nick &&
		     match_wild_nocase (access->name, name)
	    ))
	    if (access->level > highlev)
		highlev = access->level;
    return (highlev >= ci->cmd_access[CA_FLOOR]) ? highlev : 0;
}


int
check_justaccess (const char *name, ChannelInfo * ci, int what)
{
    if (ci->flags & CI_VERBOTEN)
	return 0;
    if (get_justaccess (name, ci) >= ci->cmd_access[what])
	return 1;
    return 0;
}

/*************************************************************************/
/*********************** ChanServ command routines ***********************/
/*************************************************************************/

static void
do_help (const char *source)
{
    char *cmd = strtok (NULL, "");
    char buf[BUFSIZE];

    if (cmd && is_services_op (source))
    {
	Hash_HELP *command, hash_table[] =
	{
	    {"DROP", H_SOP, oper_drop_help},
	    {"GETPASS", H_SOP, getpass_help},
	    {"FORBID", H_SOP, forbid_help},
	    {"SUSPEND", H_SOP, suspend_help},
	    {"UNSUSPEND", H_SOP, unsuspend_help},
	    {NULL}
	};

	if (command = get_help_hash (source, cmd, hash_table))
	    notice_list (s_ChanServ, source, command->process);
	else
	{
	    snprintf (buf, sizeof (buf), "%s%s", s_ChanServ, cmd ? " " : "");
	    strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	    helpserv (s_ChanServ, source, buf);
	}
    }
    else
    {
	snprintf (buf, sizeof (buf), "%s%s", s_ChanServ, cmd ? " " : "");
	strscpy (buf + strlen (buf), cmd ? cmd : "", sizeof (buf) - strlen (buf));
	helpserv (s_ChanServ, source, buf);
    }
}

/*************************************************************************/

static void
do_register (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");
    char *desc = strtok (NULL, "");
    NickInfo *ni = findnick (source);
    ChannelInfo *ci;
    User *u = finduser (source);
    struct u_chaninfolist *uc;

    if (services_level != 1)
    {
	notice (s_ChanServ, source, ERR_TEMP_DISABLED, "REGISTER");
	return;
    }

    if (!desc)
    {
	notice (s_ChanServ, source,
		"Syntax: \2REGISTER \37channel\37 \37password\37 \37description\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "REGISTER");

    }
    else if (!validchan(chan))
	notice (s_ChanServ, source, ERR_CHAN_NOTVALID, chan);

    else if (!ni)
	notice (s_ChanServ, source, NS_YOU_NOT_REGISTERED);

    else if (!is_on_chan(source, chan))
	notice (s_ChanServ, source, CS_NOT_IN_CHAN, chan);

    else if (ci = cs_findchan (chan))
	if (ci->flags & CI_VERBOTEN)
	{
	    write_log ("%s: Attempt to register FORBIDden channel %s by %s!%s@%s",
		       s_ChanServ, chan, source, u->username, u->host);
	    notice (s_ChanServ, source, CS_CANNOT_REGISTER, chan);
	}
	else
	    notice (s_ChanServ, source, CS_TAKEN, chan);

    else if (!u)
    {
	write_log ("%s: Attempt to register channel %s from nonexistent nick %s",
		   s_ChanServ, chan, source);
	notice (s_ChanServ, source, ERR_YOU_DONT_EXIST);

    }
    else if (!is_chanop (source, chan))
	notice (s_ChanServ, source, ERR_NEED_OPS, "REGISTER");

    else if (!(ni->flags & (NI_IDENTIFIED | NI_RECOGNIZED))) {
	notice (s_ChanServ, source, ERR_NEED_PASSWORD, "REGISTER");
	notice (s_ChanServ, source, ERR_IDENT_NICK_FIRST, s_NickServ);

    } else
    {
	if (ci = makechan (chan))
	{
	    Channel *c;
	    if (!(c = findchan (chan)))
	    {
		write_log ("%s: Channel %s not found for REGISTER", s_ChanServ, chan);
		notice (s_ChanServ, source, CS_NOT_IN_USE, chan);
		return;
	    }
	    ci->last_used = ci->time_registered;
	    strscpy (ci->founder, source, NICKMAX);
	    strscpy (ci->founderpass, pass, PASSMAX);
	    ci->desc = sstrdup (desc);
	    ci->url = NULL;
	    if (c->topic)
	    {
		ci->last_topic = sstrdup (c->topic);
		strscpy (ci->last_topic_setter, c->topic_setter, NICKMAX);
		ci->last_topic_time = c->topic_time;
	    }
	    ci->flags = CI_KEEPTOPIC;
	    ci->cmd_access = def_access;
	    write_log ("%s: Channel %s registered by %s!%s@%s", s_ChanServ, chan,
		       source, u->username, u->host);
	    notice (s_ChanServ, source, CS_REGISTERED, chan, source);
	    notice (s_ChanServ, source, CS_CHANGE_PASSWORD, chan, ci->founderpass);
	    if (show_sync_on==TRUE)
		notice(s_ChanServ, source, INFO_SYNC_TIME,
			disect_time((last_update+update_timeout)-time(NULL), 0));
	}
	else
	    notice (s_ChanServ, source, ERR_FAILED, "REGISTER");
	uc = smalloc (sizeof (*uc));
	uc->next = u->founder_chans;
	uc->prev = NULL;
	if (u->founder_chans)
	    u->founder_chans->prev = uc;
	u->founder_chans = uc;
	uc->chan = ci;
    }
}

/*************************************************************************/

static void
do_identify (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *pass = strtok (NULL, " ");
    ChannelInfo *ci;
    struct u_chaninfolist *c;
    User *u = finduser (source);

    if (!pass)
    {
	notice (s_ChanServ, source,
		"Syntax: \2IDENTIFY %s \37password\37\2",
		chan ? chan : "\37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "IDENTIFY");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!u)
    {
	write_log ("%s: IDENTIFY from nonexistent nick %s for %s",
		   s_ChanServ, source, chan);
	notice (s_ChanServ, source, ERR_YOU_DONT_EXIST);

    }
    else
    {

	if (!(ci->flags & CI_SUSPENDED))
	{
	    if (strcmp (pass, ci->founderpass) == 0)
	    {
		if (!is_identified (u, ci))
		{
		    c = smalloc (sizeof (*c));
		    c->next = u->founder_chans;
		    c->prev = NULL;
		    if (u->founder_chans)
			u->founder_chans->prev = c;
		    u->founder_chans = c;
		    c->chan = ci;
		    write_log ("%s: %s!%s@%s identified for %s", s_ChanServ,
			       source, u->username, u->host, chan);
		}
		notice (s_ChanServ, source, CS_IDENTIFIED, chan);
	    }
	    else
	    {
		write_log ("%s: Failed IDENTIFY for %s by %s!%s@%s",
			   s_ChanServ, chan, source, u->username, u->host);
		notice (s_ChanServ, source, ERR_WRONG_PASSWORD);
	    }
	}
	else
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
    }
}

/*************************************************************************/

static void
do_drop (const char *source)
{
    char *chan = strtok (NULL, " ");
    ChannelInfo *ci;
    User *u = finduser (source);

    if (services_level != 1)
	if (!is_services_op (source))
	{
	    notice (s_ChanServ, source, ERR_TEMP_DISABLED, "DROP");
	    return;
	}

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2DROP \37channel\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "DROP");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (!is_services_op (source) && (!u || !is_identified (u, ci)))
    {
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	{
	    notice (s_ChanServ, source, ERR_NEED_PASSWORD, "DROP");
	    notice (s_ChanServ, source, ERR_IDENT_CHAN_FIRST,
		    s_ChanServ, chan);
	}

    }
    else if (is_services_op (source) && !u)
    {
	write_log ("%s: DROP %s from nonexistent oper %s", s_ChanServ, chan, source);
	notice (s_ChanServ, source, ERR_YOU_DONT_EXIST);

    }
    else
    {

	delchan (ci);
	write_log ("%s: Channel %s dropped by %s!%s@%s", s_ChanServ, chan,
		   source, u->username, u->host);
	notice (s_ChanServ, source, CS_DROPPED, chan);
	if (services_level != 1)
	    notice (s_ChanServ, source, ERR_READ_ONLY);
    }
}

/*************************************************************************/

/* Main SET routine.  Calls other routines as follows:
 *    do_set_command(User *command-sender, ChannelInfo *ci, char *param);
 * Additional parameters can be retrieved using strtok(NULL, toks).
 * (Exception is do_set_held(), which takes only source nick and channel
 * name.)
 */
static void
do_set (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *cmd = strtok (NULL, " ");
    char *param;
    ChannelInfo *ci;
    User *u;

    if (services_level != 1)
    {
	notice (s_ChanServ, source, ERR_TEMP_DISABLED);
	return;
    }

    if (cmd)
	if (stricmp (cmd, "DESC") == 0 || stricmp (cmd, "TOPIC") == 0)
	    param = strtok (NULL, "");
	else
	    param = strtok (NULL, " ");
    else
	param = NULL;

    if (!param)
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s \37option\37 \37parameters\37\2",
		chan ? chan : "\37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!(u = finduser (source)))
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (match_wild_nocase ("FOUND*", cmd))
	if ((is_founder (u, findnick (u->nick), ci) && is_identified (u, ci))
	    || override (u->nick, CO_ADMIN))
	    if (ci->flags & CI_SUSPENDED)
		notice (s_ChanServ, u->nick, ERR_SUSPENDED_CHAN);
	    else
		do_set_founder (u, ci, param);
	else if (is_founder (u, findnick (u->nick), ci))
	{
	    notice (s_ChanServ, source, ERR_NEED_PASSWORD, "SET");
	    notice (s_ChanServ, source, ERR_IDENT_CHAN_FIRST, s_ChanServ,
		    chan);
	}
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (match_wild_nocase ("*PASS*", cmd))
	if ((is_founder (u, findnick (u->nick), ci) && is_identified (u, ci))
	    || override (u->nick, CO_ADMIN))
	    if (ci->flags & CI_SUSPENDED)
		notice (s_ChanServ, u->nick, ERR_SUSPENDED_CHAN);
	    else
		do_set_password (u, ci, param);
	else if (is_founder (u, findnick (u->nick), ci))
	{
	    notice (s_ChanServ, source, ERR_NEED_PASSWORD, "SET");
	    notice (s_ChanServ, source, ERR_IDENT_CHAN_FIRST, s_ChanServ,
		    chan);
	}
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!check_access (u, ci, CA_SET))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
    {
	Hash_CI *command, hash_table[] =
	{
	    {"DESC*", H_NONE, do_set_desc},
	    {"TOPIC", H_NONE, do_set_topic},
	    {"M*LOCK", H_NONE, do_set_mlock},
	    {"KEEPTOPIC", H_NONE, do_set_keeptopic},
	    {"T*LOCK", H_NONE, do_set_topiclock},
	    {"PRIV*", H_NONE, do_set_private},
	    {"SEC*OPS", H_NONE, do_set_secureops},
	    {"RESTRICT*", H_NONE, do_set_restricted},
	    {"SEC*", H_NONE, do_set_secure},
	    {"JOIN", H_NONE, do_set_join},
	    {"REV*", H_NONE, do_set_revenge},
	    {"U*R*L", H_NONE, do_set_url},
	    {"W*W*W", H_NONE, do_set_url},
	    {"HOME*", H_NONE, do_set_url},
	    {"*PAGE", H_NONE, do_set_url},
	    {NULL}
	};

	if (command = get_ci_hash (source, cmd, hash_table))
	    (*command->process) (u, ci, param);
	else
	    notice (s_ChanServ, source, ERR_UNKNOWN_OPTION, cmd,
	    	s_ChanServ, "SET");
    }
}

/*************************************************************************/

static void
do_set_founder (User * u, ChannelInfo * ci, char *param)
{
    NickInfo *ni = findnick (param);

    if (!ni)
    {
	notice (s_ChanServ, u->nick, NS_NOT_REGISTERED, param);
	return;
    }
    strscpy (ci->founder, ni->nick, NICKMAX);
    notice (s_ChanServ, u->nick, "%s of %s changed to \2%s\2.",
    			INFO_FOUNDER, ci->name, ci->founder);
}

/*************************************************************************/

static void
do_set_password (User * u, ChannelInfo * ci, char *param)
{
    strscpy (ci->founderpass, param, PASSMAX);
    notice (s_ChanServ, u->nick,  CS_CHANGE_PASSWORD, ci->name,
    							ci->founderpass);
}

/*************************************************************************/

static void
do_set_desc (User * u, ChannelInfo * ci, char *param)
{
    free (ci->desc);
    ci->desc = sstrdup (param);
    notice (s_ChanServ, u->nick, "%s of %s changed to \2%s\2.",
	    INFO_DESC, ci->name, param);
}

/*************************************************************************/

static void
do_set_url (User * u, ChannelInfo * ci, char *param)
{
    if (ci->url)
	free (ci->url);
    if (stricmp (param, "NONE") == 0)
    {
	ci->url = NULL;
	notice (s_ChanServ, u->nick, "%s of %s removed.", INFO_URL, ci->name);
    }
    else
    {
	ci->url = sstrdup (param);
	notice (s_ChanServ, u->nick, "%s of %s changed to \2%s\2.",
			INFO_URL, ci->name, param);
    }
}

/*************************************************************************/

static void
do_set_topic (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;
    Channel *c = findchan (chan);

    if (!c)
    {
	write_log ("%s: SET TOPIC for %s from %s: channel not found!",
		   s_ChanServ, chan, source);
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);
	return;
    }
    if (ci->last_topic)
	free (ci->last_topic);
    if (*param)
	ci->last_topic = sstrdup (param);
    else
	ci->last_topic = NULL;
    if (c->topic)
    {
	free (c->topic);
	--c->topic_time;
    }
    else
	c->topic_time = time (NULL);
    if (*param)
	c->topic = sstrdup (param);
    else
	c->topic = NULL;
    strscpy (ci->last_topic_setter, source, NICKMAX);
    strscpy (c->topic_setter, source, NICKMAX);
    ci->last_topic_time = c->topic_time;
    send_cmd (s_ChanServ, "TOPIC %s %s %lu :%s",
	      chan, source, c->topic_time, param);
}

/*************************************************************************/

static void
do_set_mlock (User * u, ChannelInfo * ci, char *param)
{
    char *s, c, mlock_on[MODEMAX], mlock_off[MODEMAX];
    int add = -1;		/* 1 if adding, 0 if deleting, -1 if neither */

    strscpy(mlock_on, "", sizeof(mlock_on));
    strscpy(mlock_off, "", sizeof(mlock_off));
    ci->mlock_limit = 0;
    if (ci->mlock_key)
    {
	free (ci->mlock_key);
	ci->mlock_key = NULL;
    }

    while (*param)
    {
	if (*param != '+' && *param != '-' && add < 0)
	{
	    ++param;
	    continue;
	}
	switch ((c = *param++))
	{
	case '+':
	    add = 1;
	    break;
	case '-':
	    add = 0;
	    break;
	case 'k':
	    if (add)
	    {
		if (!(s = strtok (NULL, " ")))
		{
		    notice (s_ChanServ, u->nick, ERR_NEED_MLOCK_PARAM, 'k');
		    return;
		}
		ci->mlock_key = sstrdup (s);
		strscpy(mlock_on, changemode("k", mlock_on), sizeof(mlock_on));
	    }
	    else
	    {
		if (ci->mlock_key)
		{
		    free (ci->mlock_key);
		    ci->mlock_key = NULL;
		}
		strscpy(mlock_off, changemode("k", mlock_off), sizeof(mlock_off));
	    }
	    break;
	case 'l':
	    if (add)
	    {
		if (!(s = strtok (NULL, " ")))
		{
		    notice (s_ChanServ, u->nick, ERR_NEED_MLOCK_PARAM, 'l');
		    return;
		}
		if (atol (s) <= 0)
		{
		    notice (s_ChanServ, u->nick, ERR_MLOCK_POSITIVE, 'l');
		    return;
		}
		ci->mlock_limit = atol (s);
		strscpy(mlock_on, changemode("l", mlock_on), sizeof(mlock_on));
	    }
	    else
	    {
		strscpy(mlock_off, changemode("l", mlock_off), sizeof(mlock_off));
		ci->mlock_limit = 0;
	    }
	    break;
	default:
	    if (add)
		strscpy(mlock_on, changemode(myctoa(c), mlock_on), sizeof(mlock_on));
	    else
		strscpy(mlock_off, changemode(myctoa(c), mlock_off), sizeof(mlock_off));
	}			/* switch */
    }				/* while (*param) */
    strscpy(ci->mlock_on, changemode("-bvo", mlock_on), sizeof(ci->mlock_on));
    strscpy(ci->mlock_off, changemode("-bvo", mlock_off), sizeof(ci->mlock_off));


    if (strlen(ci->mlock_on) || strlen(ci->mlock_off))
    {
	notice (s_ChanServ, u->nick, "%s of %s changed to \2%s%s%s%s\2.",
		INFO_MLOCK, ci->name,
		strlen(ci->mlock_on) ? "+" : "",
		strlen(ci->mlock_on) ? ci->mlock_on : "",
		strlen(ci->mlock_off) ? "-" : "",
		strlen(ci->mlock_off) ? ci->mlock_off : "");
    }
    else
	notice (s_ChanServ, u->nick, "%s of %s removed.",
				INFO_MLOCK, ci->name);
    if (!findchan (ci->name))
	if (!ci->mlock_key && !hasmode("i", ci->mlock_on))
	    send_cmd (s_ChanServ, "PART %s", ci->name);
	else if (ci->mlock_key || hasmode("i", ci->mlock_on))
	    do_cs_protect (ci->name);
    check_modes (ci->name);
}

/*************************************************************************/

static void
do_set_keeptopic (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_KEEPTOPIC;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_KEEPTOPIC, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_KEEPTOPIC;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_KEEPTOPIC, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s KEEPTOPIC {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET KEEPTOPIC");
    }
}

/*************************************************************************/

static void
do_set_topiclock (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_TOPICLOCK;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_TOPICLOCK, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_TOPICLOCK;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_TOPICLOCK, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s TOPICLOCK {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET TOPICLOCK");
    }
}

/*************************************************************************/

static void
do_set_private (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_PRIVATE;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_PRIVATE, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_PRIVATE;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_PRIVATE, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s PRIVATE {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET PRIVATE");
    }
}

/*************************************************************************/

static void
do_set_secureops (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_SECUREOPS;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_SECUREOPS, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_SECUREOPS;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_SECUREOPS, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s SECUREOPS {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET SECUREOPS");
    }
}

/*************************************************************************/

static void
do_set_restricted (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_RESTRICTED;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_RESTRICTED, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_RESTRICTED;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_RESTRICTED, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s RESTRICTED {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET RESTRICTED");
    }
}

/*************************************************************************/

static void
do_set_secure (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ChanAccess *access;
	int i;

	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	    if (access->is_nick == 0)
	    {
		notice (s_ChanServ, source, ERR_SECURE_NICKS);
		return;
	    }
	ci->flags |= CI_SECURE;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			CS_FLAG_SECURE, ci->name);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	ci->flags &= ~CI_SECURE;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			CS_FLAG_SECURE, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s SECURE {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET SECURE");
    }
}

/*************************************************************************/

static void
do_set_join (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;

    if (stricmp (param, "ON") == 0)
    {
	ci->flags |= CI_JOIN;
	notice (s_ChanServ, source, "%s for %s is now \2ON\2.",
			INFO_JOIN, ci->name);
	if (findchan (chan) && !i_am_backup ())
	    do_cs_join (chan);

    }
    else if (stricmp (param, "OFF") == 0)
    {
	if (!i_am_backup ())
	    do_cs_part (chan);
	ci->flags &= ~CI_JOIN;
	notice (s_ChanServ, source, "%s for %s is now \2OFF\2.",
			INFO_JOIN, ci->name);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2SET %s JOIN {ON|OFF}\2", chan);
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET JOIN");
    }
}

/*************************************************************************/

static void
do_set_revenge (User * u, ChannelInfo * ci, char *param)
{
    char *source = u->nick, *chan = ci->name;
    int i;

    for (i = 0; revengeinfo[i].what >= 0; ++i)
	if (stricmp (param, revengeinfo[i].cmd) == 0)
	{
	    if (revengeinfo[i].rev1 == 1)
		ci->flags |= CI_REV1;
	    else
		ci->flags &= ~CI_REV1;
	    if (revengeinfo[i].rev2 == 1)
		ci->flags |= CI_REV2;
	    else
		ci->flags &= ~CI_REV2;
	    if (revengeinfo[i].rev3 == 1)
		ci->flags |= CI_REV3;
	    else
		ci->flags &= ~CI_REV3;
	    notice (s_ChanServ, source, CS_REV_SET, ci->name,
		    revengeinfo[i].cmd, revengeinfo[i].desc);
	    break;
	}

    if (revengeinfo[i].what < 0)
	if (stricmp (param, "CURRENT") == 0)
	{
	    for (i = 0; revengeinfo[i].what != get_revenge_level (ci) &&
		 revengeinfo[i].what >= 0; i++);
	    if (revengeinfo[i].what >= 0)
		notice (s_ChanServ, source, CS_REV_LEVEL, ci->name,
			revengeinfo[i].cmd, revengeinfo[i].desc);
	}
	else
	{
	    notice (s_ChanServ, source,
		    "Syntax: \2SET %s REVENGE {CURRENT|level}\2", chan);
	    notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "SET REVENGE");
	}
}

/* This is one UUUUUUUGLY function. */

int
get_revenge_level (ChannelInfo * ci)
{
    int i;

    for (i = 0; revengeinfo[i].what >= 0; i++)
	if (revengeinfo[i].rev1 == 1 && (ci->flags & CI_REV1))
	{
	    if (revengeinfo[i].rev2 == 1 && (ci->flags & CI_REV2))
	    {
		if (revengeinfo[i].rev3 == 1 && (ci->flags & CI_REV3))
/*111 */ return revengeinfo[i].what;
		else if (revengeinfo[i].rev3 == 0 && !(ci->flags & CI_REV3))
/*110 */ return revengeinfo[i].what;
	    }
	    else if (revengeinfo[i].rev2 == 0 && !(ci->flags & CI_REV2))
	    {
		if (revengeinfo[i].rev3 == 1 && (ci->flags & CI_REV3))
/*101 */ return revengeinfo[i].what;
		else if (revengeinfo[i].rev3 == 0 && !(ci->flags & CI_REV3))
/*100 */ return revengeinfo[i].what;
	    }
	}
	else if (revengeinfo[i].rev1 == 0 && !(ci->flags & CI_REV1))
	{
	    if (revengeinfo[i].rev2 == 1 && (ci->flags & CI_REV2))
	    {
		if (revengeinfo[i].rev3 == 1 && (ci->flags & CI_REV3))
/*011 */ return revengeinfo[i].what;
		else if (revengeinfo[i].rev3 == 0 && !(ci->flags & CI_REV3))
/*010 */ return revengeinfo[i].what;
	    }
	    else if (revengeinfo[i].rev2 == 0 && !(ci->flags & CI_REV2))
	    {
		if (revengeinfo[i].rev3 == 1 && (ci->flags & CI_REV3))
/*001 */ return revengeinfo[i].what;
		else if (revengeinfo[i].rev3 == 0 && !(ci->flags & CI_REV3))
/*000 */ return revengeinfo[i].what;
	    }
	}
    return 0;
}

/*************************************************************************/

static void
do_level (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *cmd = strtok (NULL, " ");
    ChannelInfo *ci;
    User *u;

    if (!cmd)
    {
	notice (s_ChanServ, source,
		"Syntax: \2LEVEL %s {SET|RESET|LIST} \2[\37\2command\2\37|\37all\37 [\37level\37]]\2",
		chan ? chan : "\37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "LEVEL");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!(u = finduser (source)) || (!override (source, CO_SOP) &&
					  get_access (u, ci) <= 0))
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
    {
	Hash_CHAN *command, hash_table[] =
	{
	    {"LIST*", H_NONE, do_level_list},
	    {"VIEW", H_NONE, do_level_list},
	    {"DISP*", H_NONE, do_level_list},
	    {"SHOW*", H_NONE, do_level_list},
	    {"SET*", H_NONE, do_level_set},
	    {"ASS*", H_NONE, do_level_set},
	    {"RESET*", H_NONE, do_level_reset},
	    {"REASS*", H_NONE, do_level_reset},
	    {NULL}
	};

	if (command = get_chan_hash (source, cmd, hash_table))
	    (*command->process) (source, chan);
	else
	{
	    notice (s_ChanServ, source,
		    "Syntax: \2LEVEL %s {SET|RESET|LIST} \2[\37\2command\2\37|\37all\37 [\37level\37]]\2",
		    chan);
	    notice (s_ChanServ, source, ERR_UNKNOWN_OPTION, cmd,
		s_ChanServ, "LEVEL");
	}
    }
}

static void
do_level_list (const char *source, char *chan)
{
    char *acc = strtok (NULL, " ");
    ChannelInfo *ci = cs_findchan (chan);
    User *u = finduser (source);
    int i;

    if (check_access (u, ci, CA_SET) || override (source, CO_SOP))
    {
	notice (s_ChanServ, source, CS_LEVEL_LIST, chan);
	notice (s_ChanServ, source, "Level   Type");
	for (i = 0; levelinfo[i].what >= 0; i++)
	    if (!acc || match_wild_nocase (acc, levelinfo[i].cmd))
		notice (s_ChanServ, source, "%5d   %s",
		      ci->cmd_access[levelinfo[i].what], levelinfo[i].desc);
    }
    else
	for (i = 0; levelinfo[i].what >= 0; i++)
	    if (!acc || match_wild_nocase (acc, levelinfo[i].cmd))
		if (check_access (u, ci, levelinfo[i].what) &&
		    ci->cmd_access[levelinfo[i].what] >= 0)
		    notice (s_ChanServ, source, CS_LEVEL_YOU,
			    levelinfo[i].desc);
}


static void
do_level_set (const char *source, char *chan)
{
    char *acc = strtok (NULL, " ");
    char *s = strtok (NULL, " ");
    ChannelInfo *ci = cs_findchan (chan);
    User *u = finduser (source);
    NickInfo *ni = findnick (source);
    short level, chlev = -1;
    int i;

    if (!is_founder (u, ni, ci))
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!s)
	notice (s_ChanServ, source,
		"Syntax: \2LEVEL %s SET \37command\2\37 \37level\37",
		chan ? chan : "\37channel\37");

    else
    {
	for (i = 0; levelinfo[i].what >= 0; i++)
	    if (stricmp (acc, levelinfo[i].cmd) == 0)
		chlev = levelinfo[i].what;

	if (chlev >= 0)
	{
	    level = atoi (s);
	    if (level < ci->cmd_access[CA_FLOOR])
		notice (s_ChanServ, source, CS_LEVEL_LOW,
			ci->cmd_access[CA_FLOOR]);
	    else if (level > ci->cmd_access[CA_CAP])
		notice (s_ChanServ, source, CS_LEVEL_HIGH,
			ci->cmd_access[CA_CAP]);
	    else if (ci->cmd_access[chlev] == level)
		notice (s_ChanServ, source, CS_LEVEL_NO_CHANGE,
			acc, chan, level);
	    else
	    {
		ci->cmd_access[chlev] = level;
		notice (s_ChanServ, source, CS_LEVEL_CHANGE,
			acc, chan, level);
	    }
	}
	else
	    notice (s_ChanServ, source, CS_LEVEL_NONE, acc);
    }
}

static void
do_level_reset (const char *source, char *chan)
{
    char *acc = strtok (NULL, " ");
    ChannelInfo *ci = cs_findchan (chan);
    User *u = finduser (source);
    NickInfo *ni = findnick (source);
    short chlev = -2;
    int i;

    if (!is_founder (u, ni, ci))
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!acc)
	notice (s_ChanServ, source,
	     "Syntax: \2LEVEL %s RESET \2{\37\2command\2\37|\37\2ALL\2\37}",
		chan ? chan : "\37channel\37");

    else
    {
	for (i = 0; levelinfo[i].what >= 0; i++)
	    if (stricmp (acc, levelinfo[i].cmd) == 0)
		chlev = levelinfo[i].what;

	if (chlev < 0 && match_wild_nocase ("ALL*", acc))
	    chlev = -1;
	if (chlev >= 0)
	{
	    if (ci->cmd_access[chlev] == def_access[chlev])
		notice (s_ChanServ, source, CS_LEVEL_NO_CHANGE,
		    acc, chan, def_access[chlev]);
	    else {
		ci->cmd_access[chlev] = def_access[chlev];
		notice (s_ChanServ, source, CS_LEVEL_CHANGE,
		    acc, chan, def_access[chlev]);
	    }
	}
	else if (chlev == -1)
	{
	    ci->cmd_access = def_access;
	    notice (s_ChanServ, source, CS_LEVEL_RESET, chan);
	}
	else
	    notice (s_ChanServ, source, CS_LEVEL_NONE, acc);
    }
}

/*************************************************************************/

static void
do_access (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *cmd = strtok (NULL, " ");
    ChannelInfo *ci;
    User *u;

    /* If LIST, we don't *require* any parameters, but we can take any.
     * If DEL, we require a mask and no level.
     * Else (ADD), we require a level (which implies a mask). */
    if (!cmd)
    {
	notice (s_ChanServ, source,
	  "Syntax: \2ACCESS %s {ADD|DEL|LIST} [\37user\37 [\37level\37]]\2",
		chan ? chan : "\37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "ACCESS");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!(u = finduser (source)) || !(check_access (u, ci, CA_ACCESS)
					   || override (source, CO_SOP)))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
    {
	Hash_CHAN *command, hash_table[] =
	{
	    {"ADD*", H_NONE, do_access_add},
	    {"CREATE", H_NONE, do_access_add},
	    {"MAKE", H_NONE, do_access_add},
	    {"DEL*", H_NONE, do_access_del},
	    {"ERASE*", H_NONE, do_access_del},
	    {"TRASH", H_NONE, do_access_del},
	    {"LIST*", H_NONE, do_access_list},
	    {"VIEW", H_NONE, do_access_list},
	    {"DISP*", H_NONE, do_access_list},
	    {"SHOW*", H_NONE, do_access_list},
	    {NULL}
	};

	if (command = get_chan_hash (source, cmd, hash_table))
	    (*command->process) (source, chan);
	else
	{
	    notice (s_ChanServ, source,
		    "Syntax: \2ACCESS %s {ADD|DEL|LIST} [\37mask\37 [\37level\37]]\2", chan);
	    notice (s_ChanServ, source, ERR_UNKNOWN_OPTION, cmd,
	       s_ChanServ, "ACCESS");

	}
    }
}

void
do_access_add (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    char *s = strtok (NULL, " ");
    ChannelInfo *ci = cs_findchan (chan);	/* MUST exist */
    NickInfo *ni;
    User *u = finduser (source);	/* MUST exist */
    short level = 0, ulev = get_access (u, ci);		/* MUST be ok || SOP */
    int i;
    ChanAccess *access;

    if (!s)
    {
	notice (s_ChanServ, source,
		"Syntax: \2ACCESS %s ADD \37user\37 \37level\37\2",
		chan ? chan : "\37channel\37");
	return;
    }
    else if ((level = atoi (s)) >= ulev)
    {
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);
	return;
    }
    if (ulev > ci->cmd_access[CA_ACCESS])
    {
	if (services_level != 1)
	{
	    notice (s_ChanServ, source, ERR_TEMP_DISABLED, "ACCESS ADD");
	    return;
	}

	if (level == 0)
	{
	    notice (s_ChanServ, source, CS_ACCESS_ZERO);
	    return;
	}
	else if (level < ci->cmd_access[CA_FLOOR])
	{
	    notice (s_ChanServ, source, CS_ACCESS_LOW,
	    					ci->cmd_access[CA_FLOOR]);
	    return;
	}
	else if (level > ci->cmd_access[CA_CAP])
	{
	    notice (s_ChanServ, source, CS_ACCESS_HIGH,
						ci->cmd_access[CA_CAP]);
	    return;
	}

	ni = host (findnick (mask));
	if (!ni)
	{
	    char *nick, *user, *host;
	    if (ci->flags & CI_SECURE)
	    {
		notice (s_ChanServ, source, ERR_SECURE_NICKS);
		notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ,
								"SET SECURE");
		return;
	    }
	    split_usermask (mask, &nick, &user, &host);
	    s = smalloc (strlen (nick) + strlen (user) + strlen (host) + 3);
	    snprintf (s, strlen (nick) + strlen (user) + strlen (host) + 3,
	    					"%s!%s@%s", nick, user, host);
	    free (nick);
	    free (user);
	    free (host);
	}
	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i) {
	    if (access->is_nick >= 0 && (access->is_nick ?
	    		issibling (ni, access->name) :
	    		match_wild_nocase (access->name, mask)))
	    {
		if (access->level >= ulev && !is_founder (u, ni, ci))
		{
		    notice (s_ChanServ, source, ERR_ACCESS_DENIED);
		    return;
		}
		if (access->level == level)
		{
		    notice (s_ChanServ, source, LIST_UNCHANGED,
			    access->name, chan, "access", myitoa(level));
		    return;
		}
		access->level = level;
		notice (s_ChanServ, source, LIST_CHANGED,
			access->name, chan, "access", myitoa(level));
		return;
	    }
	}
	for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
	    if (access->is_nick < 0)
		break;
	if (i == ci->accesscount)
	{
	    ++ci->accesscount;
	    ci->access = srealloc (ci->access, sizeof (ChanAccess) * ci->accesscount);
	    access = &ci->access[ci->accesscount - 1];
	}
	access->name = ni ? sstrdup (ni->nick) : s;
	access->is_nick = ni ? 1 : 0;
	access->level = level;
	notice (s_ChanServ, source, LIST_ADDED_AT, 
		access->name, chan, "access", myitoa(level));
    }
    else
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);
}

void
do_access_del (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    ChannelInfo *ci = cs_findchan (chan);	/* MUST exist */
    NickInfo *ni;
    User *u = finduser (source);	/* MUST exist */
    short ulev = get_access (u, ci);	/* MUST be ok || SOP */
    int i;
    ChanAccess *access;

    if (!mask)
    {
	notice (s_ChanServ, source,
		"Syntax: \2ACCESS %s DEL \37user\37\2",
		chan ? chan : "\37channel\37");
	return;
    }
    if (ulev > ci->cmd_access[CA_ACCESS])
    {
	if (services_level != 1)
	{
	    notice (s_ChanServ, source, ERR_READ_ONLY);
	    return;
	}

	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn (mask, "1234567890") == strlen (mask) &&
	    (i = atoi (mask)) > 0 && i <= ci->accesscount)
	{
	    --i;
	    access = &ci->access[i];
	    if (access->is_nick < 0)
	    {
		notice (s_ChanServ, source, LIST_NOT_FOUND, i+1, chan, "access");
		return;
	    }
	}
	else
	{
	    ni = host (findnick (mask));
	    if (!ni)
	    {
		char *nick, *user, *host;
		split_usermask (mask, &nick, &user, &host);
		mask = smalloc (strlen (nick) + strlen (user) + strlen (host) + 3);
		snprintf (mask, strlen (nick) + strlen (user) + strlen (host) + 3,
						"%s!%s@%s", nick, user, host);
		free (nick);
		free (user);
		free (host);
	    }
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (access = ci->access, i = 0; i < ci->accesscount;
		 ++access, ++i)
		if (access->is_nick >= 0 && stricmp (access->name, mask) == 0)
		    break;
	    if (i == ci->accesscount)
		for (access = ci->access, i = 0; i < ci->accesscount;
		     ++access, ++i)
		    if (access->is_nick >= 0 && stricmp (access->name, mask) == 0)
			break;
	    if (i == ci->accesscount)
	    {
		notice (s_ChanServ, source, LIST_NOT_THERE, mask, chan, "access");
		if (!ni)
		    free (mask);
		return;
	    }
	    if (!ni)
		free (mask);
	}
	ni = findnick (source);
	if (ulev <= access->level && !is_founder (u, ni, ci))
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);
	else
	{
	    notice (s_ChanServ, source, LIST_REMOVED, access->name, chan,
	    							"access");
	    if (access->name)
		free (access->name);
	    access->is_nick = -1;
	}
    }
    else
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);
}

void
do_access_list (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    char *s;
    ChannelInfo *ci = cs_findchan (chan);	/* MUST exist */
    NickInfo *ni;
    short level = 0;
    int i;
    ChanAccess *access;

    if (mask)
	level = atoi (mask);
    notice (s_ChanServ, source, "Access list for %s:", chan);
    notice (s_ChanServ, source, "  Num   Lev  Mask");
    for (access = ci->access, i = 0; i < ci->accesscount; ++access, ++i)
    {
	if (access->is_nick < 0)
	    continue;
	if ((mask && !match_wild_nocase (mask, access->name)) ||
	    (level && level != access->level))
	    continue;
	if (ni = host (findnick (access->name)))
	    s = userisnick(ni->nick) ? "ONLINE" : ni->last_usermask;
	else
	    s = NULL;
	notice (s_ChanServ, source, "  %3d %6d  %s%s%s%s",
		i + 1, access->level, access->name,
		s ? " (" : "", s ? s : "", s ? ")" : "");
    }
}

/*************************************************************************/

static void
do_akick (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *cmd = strtok (NULL, " ");
    ChannelInfo *ci;
    User *u;

    if (!cmd)
    {
	notice (s_ChanServ, source,
	     "Syntax: \2AKICK %s {ADD|DEL|LIST} [\37nick-or-usermask\37]\2",
		chan ? chan : "\37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "AKICK");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!(u = finduser (source)) || !(check_access (u, ci, CA_AKICK)
					   || override (source, CO_SOP)))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
    {
	Hash_CHAN *command, hash_table[] =
	{
	    {"ADD*", H_NONE, do_akick_add},
	    {"CREATE", H_NONE, do_akick_add},
	    {"MAKE", H_NONE, do_akick_add},
	    {"DEL*", H_NONE, do_akick_del},
	    {"ERASE*", H_NONE, do_akick_del},
	    {"TRASH", H_NONE, do_akick_del},
	    {"LIST*", H_NONE, do_akick_list},
	    {"VIEW", H_NONE, do_akick_list},
	    {"DISP*", H_NONE, do_akick_list},
	    {"SHOW*", H_NONE, do_akick_list},
	    {NULL}
	};

	if (command = get_chan_hash (source, cmd, hash_table))
	    (*command->process) (source, chan);
	else
	{
	    notice (s_ChanServ, source,
	     "Syntax: \2AKICK %s {ADD|DEL|LIST} [\37nick-or-usermask\37]\2",
		    chan);
	    notice (s_ChanServ, source, ERR_UNKNOWN_OPTION, cmd,
						s_ChanServ, "AKICK");
	}
    }
}

void
do_akick_add (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    char *reason = strtok (NULL, "");
    ChannelInfo *ci = cs_findchan (chan);
    NickInfo *ni;
    User *u = finduser (source);
    int i;
    AutoKick *akick;

    if (!mask)
    {
	notice (s_ChanServ, source,
	   "Syntax: \2AKICK %s ADD \37nick-or-usermask\37 [\37reason\37]\2",
		chan ? chan : "\37channel\37");
	return;
    }
    if (check_access (u, ci, CA_AKICK))
    {
	if (services_level != 1)
	{
	    notice (s_ChanServ, source, ERR_TEMP_DISABLED, "AKICK ADD");
	    return;
	}

	if (ci->akickcount > akick_max)
	{
	    notice (s_ChanServ, source, LIST_LIMIT, akick_max, "channel" "AKICK");
	    return;
	}
	ni = host (findnick (mask));
	if (!ni)
	{
	    int nonchr = 0;
	    char *nick, *user, *host;
	    split_usermask (mask, &nick, &user, &host);
	    mask = smalloc (strlen (nick) + strlen (user) + strlen (host) + 3);
	    snprintf (mask, strlen (nick) + strlen (user) + strlen (host) + 3,
	    					"%s!%s@%s", nick, user, host);
	    free (nick);
	    free (user);
	    free (host);
	    for (i = strlen (mask); mask[i] != '@' && i >= 0; i--)
		if (!(mask[i] == '*' || mask[i] == '?' || mask[i] == '.'))
		    nonchr++;
	    if (nonchr < starthresh && !check_access (u, ci, CA_STARAKICK))
	    {
		notice (s_ChanServ, source, ERR_STARTHRESH, starthresh);
		return;
	    }
	    else if (nonchr < starthresh)
	    {
		for (i--; i > 0 && (mask[i] == '*' || mask[i] == '?'); i--);
		if (!i)
		{
		    notice (s_ChanServ, source, ERR_STARTHRESH, 1);
		    return;
		}
	    }
	}
	if (get_justaccess (mask, ci) >= get_access (u, ci))
	{
	    notice (s_ChanServ, source, CS_ACCESS_HIGHER,
		    ni ? ni->nick : mask);
	    return;
	}
	else if (!ni && (i = countusermask (mask)))
	{
	    User *user2;
	    while (i)
	    {
		user2 = findusermask (mask, i);
		if (get_access (user2, ci) >= get_access (u, ci))
		{
		    notice (s_ChanServ, source, CS_ACCESS_HIGHER_MATCH,
			    mask, user2->nick);
		    return;
		}
		i--;
	    }
	}
	if (is_on_akick (mask, ci))
	    notice (s_ChanServ, source, LIST_THERE, mask, chan, "AKICK");
	else
	{
	    Channel *c;
	    ++ci->akickcount;
	    ci->akick = srealloc (ci->akick, sizeof (AutoKick) * ci->akickcount);
	    akick = &ci->akick[ci->akickcount - 1];
	    akick->name = ni ? sstrdup (ni->nick) : mask;
	    akick->is_nick = ni ? 1 : 0;
	    akick->pad = 0;	/* unused */
	    if (reason)
		akick->reason = sstrdup (reason);
	    else
		akick->reason = NULL;
	    notice (s_ChanServ, source, LIST_ADDED, akick->name, chan, "AKICK");

	    /* This call should not fail, but check just in case... */
	    if (c = findchan (chan))
	    {
		struct c_userlist *u2, *u3;
		NickInfo *ni2;

		if (!reason)
		    reason = def_akick_reason;
		u2 = c->users;
		while (u2)
		{
		    u3 = u2->next;
		    ni2 = findnick (akick->name);
		    if (ni ? issibling (ni2, u2->user->nick) :
			match_usermask (akick->name, u2->user))
		    {
			kick_user (s_ChanServ, chan, u2->user->nick, reason);
		    }
		    u2 = u3;
		}
	    }
	}
    }
    else
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);
}

void
do_akick_del (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    User *u = finduser (source);
    ChannelInfo *ci = cs_findchan (chan);
    int i;
    AutoKick *akick;

    if (!mask)
    {
	notice (s_ChanServ, source,
		"Syntax: \2AKICK %s DEL \37nick-or-usermask\37\2",
		chan ? chan : "\37channel\37");
	return;
    }
    if (check_access (u, ci, CA_AKICK))
    {
	if (services_level != 1)
	{
	    notice (s_ChanServ, source, ERR_TEMP_DISABLED, "AKICK DEL");
	    return;
	}
	/* Special case: is it a number?  Only do search if it isn't. */
	if (strspn (mask, "1234567890") == strlen (mask) &&
	    (i = atoi (mask)) > 0 && i <= ci->akickcount)
	{
	    --i;
	    akick = &ci->akick[i];
	}
	else
	{
	    /* First try for an exact match; then, a case-insensitive one. */
	    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
		if (stricmp (akick->name, mask) == 0)
		    break;
	    if (i == ci->akickcount)
		for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
		    if (stricmp (akick->name, mask) == 0)
			break;
	    if (i == ci->akickcount)
	    {
		notice (s_ChanServ, source, LIST_NOT_THERE, mask, chan, "AKICK");
		return;
	    }
	}
	notice (s_ChanServ, source, LIST_REMOVED, akick->name, chan, "AKICK");
	if (akick->name)
	    free (akick->name);
	if (akick->reason)
	    free (akick->reason);
	--ci->akickcount;
	if (i < ci->akickcount)
	    bcopy (akick + 1, akick, sizeof (AutoKick) * (ci->akickcount - i));
	if (ci->akickcount)
	    ci->akick = srealloc (ci->akick, sizeof (AutoKick) * ci->akickcount);
	else
	{
	    if (ci->akick)
		free (ci->akick);
	    ci->akick = NULL;
	}
    }
    else
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);
}

void
do_akick_list (const char *source, char *chan)
{
    char *mask = strtok (NULL, " ");
    char *t;
    ChannelInfo *ci = cs_findchan (chan);
    NickInfo *ni;
    int i;
    AutoKick *akick;

    notice (s_ChanServ, source, "AKICK list for %s:", chan);
    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
    {
	if (mask && !match_wild_nocase (mask, akick->name))
	    continue;
	if (ni = host (findnick (akick->name)))
	    t = userisnick(ni->nick) ? "ONLINE" : ni->last_usermask;
	else
	    t = NULL;
	notice (s_ChanServ, source, "  %3d %s%s%s%s%s%s%s",
		i + 1, akick->name,
		t ? " (" : "", t ? t : "", t ? ")" : "",
		akick->reason ? " (" : "",
		akick->reason ? akick->reason : "",
		akick->reason ? ")" : "");
    }
}

static int
is_on_akick (const char *mask, ChannelInfo * ci)
{
    int i, is_nick = 0;
    AutoKick *akick;
    char aknick[NICKMAX];

    if (findnick (mask))
    {
	is_nick = 1;
	strscpy (aknick, host (findnick (mask))->nick, NICKMAX);
    }

    for (akick = ci->akick, i = 0; i < ci->akickcount; ++akick, ++i)
	if (is_nick)
	{
	    if (akick->is_nick > 0 && stricmp (aknick, akick->name) == 0)
		return 1;
	}
	else if (akick->is_nick == 0 && match_wild_nocase (akick->name, mask))
	    return 2;
    return 0;
}

/*************************************************************************/

static void
do_info (const char *source)
{
    char *chan = strtok (NULL, " ");
    ChannelInfo *ci;
    NickInfo *ni;
    int i;
    char buf[BUFSIZE], modes[MODEMAX], *t;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2INFO \37channel\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "INFO");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_CANNOT_REGISTER, chan);

    else
    {
	notice (s_ChanServ, source, CS_INFO_INTRO, chan);
	if (ni = host (findnick (ci->founder)))
	    t = userisnick(ni->nick) ? "ONLINE" : ni->last_usermask;
	else
	    t = NULL;
	notice (s_ChanServ, source, CS_INFO_FOUNDER,
		ci->founder, t ? " (" : "", t ? t : "", t ? ")" : "");
	notice (s_ChanServ, source, CS_INFO_DESC, ci->desc);
	if (ci->url)
	    notice (s_ChanServ, source, CS_INFO_URL, ci->url);
	notice (s_ChanServ, source, CS_INFO_REG_TIME,
					time_ago (ci->time_registered, 1));
	if (!findchan (chan))
	    notice (s_ChanServ, source, CS_INFO_LAST_USED,
					    time_ago (ci->last_used, 1));
	if (ci->last_topic)
	    if (ci->flags & CI_SUSPENDED)
	    {
		notice (s_ChanServ, source, CS_INFO_SUSPENDED, ci->last_topic);
		notice (s_ChanServ, source, CS_INFO_SUSPENDER,
							ci->last_topic_setter);
	    }
	    else
	    {
		notice (s_ChanServ, source, CS_INFO_TOPIC, ci->last_topic);
		notice (s_ChanServ, source, CS_INFO_TOPIC_SET,
							ci->last_topic_setter);
	    }
	for (i = 0; revengeinfo[i].what != get_revenge_level (ci) &&
	     revengeinfo[i].what >= 0; i++);
	if (revengeinfo[i].what >= 0)
	    notice (s_ChanServ, source, CS_INFO_REVENGE, revengeinfo[i].desc);

	if (!(ci->flags & CI_PRIVATE) || override (source, CO_OPER))
	{
	    Channel *c = findchan (chan);
	    if (c)
	    {
		int ops, users, voices;
		struct c_userlist *data;
		for (data = c->users, users = 0; data; data = data->next)
		    users++;
		for (data = c->voices, voices = 0; data; data = data->next)
		    voices++;
		for (data = c->chanops, ops = 0; data; data = data->next)
		    ops++;
		notice (s_ChanServ, source, CS_INFO_CHAN_STAT,
			users, users == 1 ? "" : "s",
			voices, voices == 1 ? "" : "s",
			ops, ops == 1 ? "" : "s");
	    }
	}
	    *buf = 0;
	    if (ci->flags & CI_SUSPENDED)
		strcpy (buf, CS_FLAG_SUSPENDED);
	    else
	    {
		if (ci->flags & CI_PRIVATE)
		    strcpy (buf, CS_FLAG_PRIVATE);
		if (ci->flags & CI_KEEPTOPIC)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_KEEPTOPIC);
		}
		if (ci->flags & CI_TOPICLOCK)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_TOPICLOCK);
		}
		if (ci->flags & CI_SECUREOPS)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_SECUREOPS);
		}
		if (ci->flags & CI_RESTRICTED)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_RESTRICTED);
		}
		if (ci->flags & CI_SECURE)
		{
		    if (*buf)
			strcat (buf, ", ");
		    strcat (buf, CS_FLAG_SECURE);
		}
		if (!(*buf))
		    strcpy (buf, CS_FLAG_NONE);
	    }
	notice (s_ChanServ, source, CS_INFO_OPTIONS, buf);
	*buf = *modes = 0;
	if (strlen(ci->mlock_on))
	    snprintf (modes, sizeof (modes), "+%s", ci->mlock_on);
	strcat(buf, modes); *modes = 0;
	if (strlen(ci->mlock_off))
	    snprintf (modes, sizeof (modes), "-%s", ci->mlock_off);
	strcat(buf, modes); *modes = 0;
	if (ci->mlock_limit)
	    sprintf (modes, " %ld", ci->mlock_limit);
	strcat(buf, modes);
	notice (s_ChanServ, source, CS_INFO_MLOCK, buf);
	if (show_sync_on==TRUE)
	    notice(s_ChanServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
}

/*************************************************************************/

static void
do_list (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *cmax = strtok(NULL, " ");
    ChannelInfo *ci;
    int nchans, i, max = 50;
    char buf[BUFSIZE];

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2LIST \37pattern\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "LIST");

    }
    else
    {
	if (cmax && atoi(cmax)>0)
	    max=atoi(cmax);
	nchans = 0;
	notice (s_ChanServ, source, INFO_LIST_MATCH, chan);
	for (i = 33; i < 256; ++i)
	    for (ci = chanlists[i]; ci; ci = ci->next)
	    {
		if (!(is_oper (source)))
		    if (ci->flags & (CI_PRIVATE | CI_VERBOTEN | CI_SUSPENDED))
			continue;
		if (ci->flags & (CI_VERBOTEN | CI_SUSPENDED))
		    if (strlen (ci->name) > sizeof (buf))
			continue;
		    else if (strlen (ci->name) + strlen (ci->desc) > sizeof (buf))
			continue;
		if (ci->flags & CI_VERBOTEN)
		    snprintf (buf, sizeof (buf), "%-20s  << FORBIDDEN >>", ci->name);
		else if (ci->flags & CI_SUSPENDED)
		    snprintf (buf, sizeof (buf), "%-20s  << SUSPENDED >>", ci->name);
		else if (strlen (ci->desc) > 0)
		    snprintf (buf, sizeof (buf), "%-20s  %s", ci->name, ci->desc);
		else
		    snprintf (buf, sizeof (buf), "%-20s", ci->name);
		if (match_wild_nocase (chan, buf))
		    if (++nchans <= max)
			notice (s_ChanServ, source, "    %s", buf);
	    }
	notice (s_ChanServ, source, INFO_END_OF_LIST,
					(nchans > max) ? max : nchans, nchans);
	if (show_sync_on==TRUE)
	    notice(s_ChanServ, source, INFO_SYNC_TIME,
		disect_time((last_update+update_timeout)-time(NULL), 0));
    }
}

static void
do_users (const char *source)
{
    char *chan = strtok (NULL, " ");
    User *u = finduser (source);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source,
		"Syntax: \2USERS\2 \037channel\037");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "USERS");

    }
    else if (!findchan (chan))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if ((!u || !check_access (u, ci, CA_CMDINVITE)) &&
	     !override (source, CO_OPER))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
	send_channel_users (s_ChanServ, source, chan);

}
/*************************************************************************/

static void
do_invite (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *inv_params = strtok (NULL, " ");
    User *u = finduser (source);
    int over = override(source, CO_OPER);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2INVITE\2 \37channel\37%s",
					over ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "INVITE");

    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (!findchan (chan) && !hasmode("i", ci->mlock_on))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (ci->flags & CI_SUSPENDED)
	send_cmd (s_ChanServ, "INVITE %s %s", source, chan);

    else if ((!u || !check_access (u, ci, CA_CMDINVITE)) && !over)
	notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!inv_params || !over)
	send_cmd (s_ChanServ, "INVITE %s %s", source, chan);
    else if (finduser (inv_params))
	send_cmd (s_ChanServ, "INVITE %s %s", finduser (inv_params)->nick, chan);
    else
	notice (s_ChanServ, source, NS_NOT_IN_USE, inv_params);
}

/*************************************************************************/

static void
do_mode (const char *source)
{
    char *chan = strtok (NULL, " ");
    User *u = finduser (source);
    ChannelInfo *ci;
    Channel *c;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2MODE\2 \37channel\37");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "MODE");
    }
    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (!(c = findchan (chan)) && !hasmode("i", ci->mlock_on) &&
    							!ci->mlock_key)
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if ((!u || !check_access (u, ci, CA_CMDINVITE)) &&
	     !override (source, CO_OPER))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (c)
    {
	char l[16];
	char modes[64];
	strscpy (modes, changemode ("-kl", c->mode), sizeof(modes));
	snprintf (l, sizeof (l), " %d", c->limit);
	notice (s_ChanServ, source, "%s +%s%s%s %s %s", c->name, modes,
		c->key   ? "k"    : "", c->limit ? "l"    : "",
		c->key   ? c->key : "", c->limit ? l      : "");
    }
    else
	notice (s_ChanServ, source, "%s modes are +s%s%s %s", ci->name,
		hasmode("i", ci->mlock_on) ? "i" : "",
		ci->mlock_key ? "k" : "",
		ci->mlock_key ? ci->mlock_key : "");
}

/*************************************************************************/

static void
do_voice (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *voice_params = strtok (NULL, " ");
    User *u = finduser (source);
    int over = override(source, CO_OPER);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2VOICE\2 \037channel\037%s",
			over ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "VOICE");

    }
    else if (!findchan (chan))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!is_on_chan((over && voice_params) ? voice_params : source, chan))
	if (over && voice_params)
	    notice (s_ChanServ, source, CS_NOT_IN_CHAN, voice_params, chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_IN_CHAN, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (is_voiced((over && voice_params) ? voice_params : source, chan))
	if (over && voice_params)
	    notice (s_ChanServ, source, CS_ALREADY_GOT, voice_params,
			    				"voiced", chan);
	else
	    notice (s_ChanServ, source, CS_YOU_ALREADY_GOT, "voiced", chan);

    else if ((!u || !check_access(u, ci, CA_CMDVOICE)) && !over)
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!voice_params || !over)
	change_cmode (s_ChanServ, chan, "+v", source);
    else if (finduser (voice_params))
	change_cmode (s_ChanServ, chan, "+v", finduser (voice_params)->nick);
    else
	notice (s_ChanServ, source, NS_NOT_IN_USE, voice_params);
}

/*************************************************************************/

static void
do_devoice (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *devoice_params = strtok (NULL, " ");
    User *u = finduser (source);
    int over = override(source, CO_OPER);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2DEVOICE\2 \037channel\037%s",
			over ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "DEVOICE");

    }
    else if (!findchan (chan))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!is_on_chan((over && devoice_params) ? devoice_params : source, chan))
	if (over && devoice_params)
	    notice (s_ChanServ, source, CS_NOT_IN_CHAN, devoice_params, chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_IN_CHAN, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!is_voiced((over && devoice_params) ? devoice_params : source, chan))
	if (over && devoice_params)
	    notice (s_ChanServ, source, CS_NOT_GOT, devoice_params,
			    				"voiced", chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_GOT, "voiced", chan);

    else if ((!u || !check_access(u, ci, CA_CMDVOICE)) && !over)
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!devoice_params || !over)
	change_cmode (s_ChanServ, chan, "-v", source);
    else if (finduser (devoice_params))
	change_cmode (s_ChanServ, chan, "-v", finduser (devoice_params)->nick);
    else
	notice (s_ChanServ, source, NS_NOT_IN_USE, devoice_params);
}
/*************************************************************************/

static void
do_op (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *op_params = strtok (NULL, " ");
    User *u = finduser (source);
    int over = override(source, CO_OPER);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2OP\2 \037channel\037%s",
			over ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "OP");

    }
    else if (!findchan (chan))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!is_on_chan((over && op_params) ? op_params : source, chan))
	if (over && op_params)
	    notice (s_ChanServ, source, CS_NOT_IN_CHAN, op_params, chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_IN_CHAN, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (is_chanop((over && op_params) ? op_params : source, chan))
	if (over && op_params)
	    notice (s_ChanServ, source, CS_ALREADY_GOT, op_params,
			    				"oped", chan);
	else
	    notice (s_ChanServ, source, CS_YOU_ALREADY_GOT, "oped", chan);

    else if ((!u || !check_access(u, ci, CA_CMDOP)) && !over)
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!op_params || !over)
	change_cmode (s_ChanServ, chan, "+o", source);
    else if (finduser (op_params))
	change_cmode (s_ChanServ, chan, "+o", finduser (op_params)->nick);
    else
	notice (s_ChanServ, source, NS_NOT_IN_USE, op_params);
}

/*************************************************************************/

static void
do_deop (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *deop_params = strtok (NULL, " ");
    User *u = finduser (source);
    int over = override(source, CO_OPER);
    ChannelInfo *ci;

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2DEOP\2 \037channel\037%s",
			over ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "DEOP");

    }
    else if (!findchan (chan))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!is_on_chan((over && deop_params) ? deop_params : source, chan))
	if (over && deop_params)
	    notice (s_ChanServ, source, CS_NOT_IN_CHAN, deop_params, chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_IN_CHAN, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!is_chanop((over && deop_params) ? deop_params : source, chan))
	if (over && deop_params)
	    notice (s_ChanServ, source, CS_NOT_GOT, deop_params,
			    				"oped", chan);
	else
	    notice (s_ChanServ, source, CS_YOU_NOT_GOT, "oped", chan);

    else if ((!u || !check_access(u, ci, CA_CMDOP)) && !over)
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (!deop_params || !over)
	change_cmode (s_ChanServ, chan, "-o", source);
    else if (finduser (deop_params))
	change_cmode (s_ChanServ, chan, "-o", finduser (deop_params)->nick);
    else
	notice (s_ChanServ, source, NS_NOT_IN_USE, deop_params);
}

/*************************************************************************/

static void
do_unban (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *unban_params = strtok (NULL, " ");
    User *u;
    ChannelInfo *ci;
    Channel *c;
    int i;

    if (!override (source, CO_OPER) || !unban_params)
	u = finduser (source);
    else
	u = finduser (unban_params);

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2UNBAN\2 \37channel\37%s",
		override (source, CO_OPER) ? " \037nick\037" : "");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "UNBAN");

    }
    else if (!(c = findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!u || !(check_access(u, ci, CA_CMDUNBAN) ||
	      override (source, CO_OPER)))
    {
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    }
    else
    {
	for (i = 0; i < c->bancount; i++)
	    if (match_usermask (c->bans[i], u))
	    {
		change_cmode (s_ChanServ, c->name, "-b", c->bans[i]);
		i--;
	    }

	if (!unban_params || !override (source, CO_OPER))
	    notice (s_ChanServ, source, CS_YOU_UNBANNED, chan);
	else
	    notice (s_ChanServ, source, CS_UNBANNED, u->nick, chan);
    }
}

/*************************************************************************/

static void
do_clear (const char *source)
{
    char *chan = strtok (NULL, " ");
    char *what = strtok (NULL, " ");
    User *u = finduser (source);
    Channel *c;
    ChannelInfo *ci;

    if (!what)
    {

	notice (s_ChanServ, source,
		"Syntax: \2CLEAR \37channel\37 \37what\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "CLEAR");

    }
    else if (!(c = findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if ((!u || !check_access (u, ci, CA_CMDCLEAR)) &&
	     !override (source, CO_SOP))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else if (stricmp (what, "bans") == 0)
    {
	int i;

	for (i = 0; i < c->bancount; )
	    change_cmode (s_ChanServ, chan, "-b", c->bans[i]);
	notice (s_ChanServ, source, CS_CLEARED, "BANS", chan);

    }
    else if (stricmp (what, "modes") == 0)
    {

	if (c->key)
	    change_cmode (s_ChanServ, chan, c->mode, c->key);
	else
	    change_cmode (s_ChanServ, chan, c->mode, "");
	notice (s_ChanServ, source, CS_CLEARED, "MODES", chan);

    }
    else if (stricmp (what, "ops") == 0)
    {
	struct c_userlist *cu, *next;

	for (cu = c->chanops; cu; cu = next)
	{
	    next = cu->next;
	    change_cmode (s_ChanServ, chan, "-o", cu->user->nick);
	}
	notice (s_ChanServ, source, CS_CLEARED, "OPS", chan);

    }
    else if (stricmp (what, "voices") == 0)
    {
	struct c_userlist *cu, *next;

	for (cu = c->voices; cu; cu = next)
	{
	    next = cu->next;
	    change_cmode (s_ChanServ, chan, "-v", cu->user->nick);
	}
	notice (s_ChanServ, source, CS_CLEARED, "VOICES", chan);

    }
    else if (stricmp (what, "users") == 0)
    {
	struct c_userlist *cu, *next_cu;
	char buf[BUFSIZE];

	snprintf (buf, sizeof (buf), CS_CLEAR_KICK, source);

	for (cu = c->users; cu; cu = next_cu)
	{
	    next_cu = cu->next;
	    kick_user (s_ChanServ, chan, cu->user->nick, buf);
	}
	notice (s_ChanServ, source, CS_CLEARED, "USERS", chan);

    }
    else
    {
	notice (s_ChanServ, source,
		"Syntax: \2CLEAR \37channel\37 \37what\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "CLEAR");
    }
}

/*************************************************************************/

static void
do_resync (const char *source)
{
    char *chan = strtok (NULL, " ");
    User *u = finduser (source);
    Channel *c;
    ChannelInfo *ci;
    int i;
    struct c_userlist *cu;

    if (!chan)
    {

	notice (s_ChanServ, source,
		"Syntax: \2CLEAR \37channel\37\2");
	notice (s_ChanServ, source, ERR_MORE_INFO, s_ChanServ, "CLEAR");

    }
    else if (!(c = findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_IN_USE, chan);

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if ((!u || !check_access (u, ci, CA_CMDCLEAR)) &&
	     !override (source, CO_SOP))
	if (ci->flags & CI_SUSPENDED)
	    notice (s_ChanServ, source, ERR_SUSPENDED_CHAN);
	else
	    notice (s_ChanServ, source, ERR_ACCESS_DENIED);

    else
    {
	for (i = 0; i < c->bancount; ++i)
	    send_cmd (s_ChanServ, "MODE %s -%s+%s :%s %s", chan, "b", "b", c->bans[i], c->bans[i]);
	if (c->key)
	{
	    send_cmd (s_ChanServ, "MODE %s -%s %s", chan, c->mode, c->key);
	    if (c->limit)
		send_cmd (s_ChanServ, "MODE %s +%s %s %d", chan, c->mode, c->key, c->limit);
	    else
		send_cmd (s_ChanServ, "MODE %s +%s %s", chan, c->mode, c->key);
	}
	else if (c->limit)
	{
	    send_cmd (s_ChanServ, "MODE %s -%s", chan, c->mode);
	    send_cmd (s_ChanServ, "MODE %s +%s %d", chan, c->mode, c->limit);
	}
	else
	{
	    send_cmd (s_ChanServ, "MODE %s -%s", chan, c->mode);
	    send_cmd (s_ChanServ, "MODE %s +%s", chan, c->mode);
	}
	for (cu = c->chanops; cu; cu = cu->next)
	    send_cmd (s_ChanServ, "MODE %s -%s+%s %s %s", chan, "o", "o", cu->user->nick, cu->user->nick);
	for (cu = c->voices; cu; cu = cu->next)
	    send_cmd (s_ChanServ, "MODE %s -%s+%s %s %s", chan, "v", "v", cu->user->nick, cu->user->nick);

    }
}

/*************************************************************************/

static void
do_getpass (const char *source)
{
    char *chan = strtok (NULL, " ");
    User *u = finduser (source);
    ChannelInfo *ci;

    if (!chan)
	notice (s_ChanServ, source, "Syntax: \2GETPASS \37channel\37\2");

    else if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!u)
	notice (s_ChanServ, source, ERR_YOU_DONT_EXIST);

    else
    {
	write_log ("%s: %s!%s@%s used GETPASS on %s",
		   s_ChanServ, source, u->username, u->host, chan);
	wallops (s_ChanServ, MULTI_GETPASS_WALLOP, source, chan, ci->founder);
	notice (s_ChanServ, source, MULTI_GETPASS, chan, ci->founder,
						ci->founderpass);
    }
}

/*************************************************************************/

static void
do_forbid (const char *source)
{
    ChannelInfo *ci;
    char *chan = strtok (NULL, " ");

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2FORBID \37channel\37\2");
	return;
    }
    if (ci = cs_findchan (chan))
    {
	delchan (ci);
	wallops (s_ChanServ, CS_FORBID_WALLOP, source, chan);
    }
    if (ci = makechan (chan))
    {
	ci->desc = sstrdup ("");
	ci->url = NULL;
	write_log ("%s: %s set FORBID for channel %s", s_ChanServ, source, chan);
	ci->flags |= CI_VERBOTEN;
	notice (s_ChanServ, source, MULTI_FORBID, chan);
	if (services_level != 1)
	    notice (s_ChanServ, source, ERR_READ_ONLY);
    }
    else
    {
	write_log ("%s: Valid FORBID for %s by %s failed", s_ChanServ,
		   chan, source);
	notice (s_ChanServ, source, ERR_FAILED, "FORBID");
    }
}

/*************************************************************************/

static void
do_suspend (const char *source)
{
    Channel *c;
    ChannelInfo *ci;
    char *chan = strtok (NULL, " ");
    char *reason = strtok (NULL, "");

    if (!reason)
    {
	notice (s_ChanServ, source, "Syntax: \2SUSPEND \37channel reason\37\2");
	return;
    }
    if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (ci->flags & CI_SUSPENDED)
	notice (s_ChanServ, source, CS_IS_SUSPENDED, chan);

    else
    {
	ci->flags |= CI_SUSPENDED;

	if (ci->last_topic)
	    free (ci->last_topic);
	ci->last_topic = sstrdup (reason);
	strscpy (ci->last_topic_setter, source, NICKMAX);
	ci->last_topic_time = time (NULL);

	if ((c = findchan (chan)))
	{
	    int i;
	    struct c_userlist *cu, *next;

	    /* CLEAR BANS */
	    for (i = 0; i < c->bancount; ++i)
		change_cmode (s_ChanServ, chan, "-b", c->bans[i]);

	    /* CLEAR MODES */
	    if (c->key)
		change_cmode (s_ChanServ, chan, "-mintpslk", c->key);
	    else
		change_cmode (s_ChanServ, chan, "-mintpslk", "");

	    /* CLEAR OPS */
	    for (cu = c->chanops; cu; cu = next)
	    {
		next = cu->next;
		change_cmode (s_ChanServ, chan, "-o", cu->user->nick);
	    }

	    /* CLEAR VOICES */
	    for (cu = c->voices; cu; cu = next)
	    {
		next = cu->next;
		change_cmode (s_ChanServ, chan, "-v", cu->user->nick);
	    }
	    restore_topic (chan);
	}

	write_log ("%s: %s set SUSPEND for channel %s because of %s", s_ChanServ,
		   source, chan, reason);
	notice (s_ChanServ, source, MULTI_SUSPEND, chan, ci->founder);
	if (services_level != 1)
	    notice (s_ChanServ, source, ERR_READ_ONLY);
    }
}

static void
do_unsuspend (const char *source)
{
    Channel *c;
    ChannelInfo *ci;
    char *chan = strtok (NULL, " ");

    if (!chan)
    {
	notice (s_ChanServ, source, "Syntax: \2UNSUSPEND \37channel\37\2");
	return;
    }
    if (!(ci = cs_findchan (chan)))
	notice (s_ChanServ, source, CS_NOT_REGISTERED, chan);

    else if (ci->flags & CI_VERBOTEN)
	notice (s_ChanServ, source, CS_FORBIDDEN, chan);

    else if (!(ci->flags & CI_SUSPENDED))
	notice (s_ChanServ, source, CS_IS_NOT_SUSPENDED, chan);
    else
    {
	ci->flags &= ~CI_SUSPENDED;

	if (ci->last_topic)
	    free (ci->last_topic);
	ci->last_topic = NULL;
	strscpy (ci->last_topic_setter, source, NICKMAX);
	ci->last_topic_time = time (NULL);

	if ((c = findchan (chan)))
	{
	    if (c->topic)
		free (c->topic);
	    c->topic = NULL;
	    strscpy (c->topic_setter, ci->last_topic_setter, NICKMAX);
	    c->topic_time = ci->last_topic_time;

	    send_cmd (s_ChanServ, "TOPIC %s %s %lu :", chan,
		      c->topic_setter, c->topic_time);
	}
	write_log ("%s: %s removed SUSPEND for channel %s", s_ChanServ, source, chan);
	notice (s_ChanServ, source, MULTI_UNSUSPEND, chan, ci->founder);
	if (services_level != 1)
	    notice (s_ChanServ, source, ERR_READ_ONLY);
    }
}

/*************************************************************************/
