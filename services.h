/* Main header for Services.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifndef SERVICES_H
#define	SERVICES_H

/*************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include "win32util.h"
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define umask(x) _umask(x)
#define chdir(x) _chdir(x)
#define getpid() _getpid()
#define read(x,y,z) _read(x,y,z)
#define write(x,y,z) _write(x,y,z)
#define bzero(x,y) memset(x,0,y)
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <grp.h>
#endif

#include <ctype.h>

/* We have our own versions of toupper()/tolower(). */
#undef tolower
#undef toupper
#define	tolower tolower_
#define	toupper toupper_
extern int toupper(char), tolower(char);

#include "config.h"
#include "messages.h"

#if BUFSIZE < 64
#  error Cannot set BUFSIZE < 64 - edit the config.h
#endif
#if CHANMAX < 16
#  error Cannot set CHANMAX < 16 - edit the config.h
#endif
#if NICKMAX < 8
#  error Cannot set NICKMAX < 9 - edit the config.h
#endif
#if PASSMAX < 4
#  error Cannot set PASSMAX < 5 - edit the config.h
#endif
#if MODEMAX < 8
#  error Cannot set MODEMAX < 15 - edit the config.h
#endif
#if LASTMSGMAX < 4
#  error Cannot set LASTMSGMAX < 4 - edit the config.h
#endif

/*************************************************************************/

#define RUN_STARTED	0x00000001
#define	RUN_MODE	0x00000002
#define RUN_LOG_IS_OPEN	0x00000004
#define	RUN_DEBUG	0x00000008
#define RUN_SIGTERM	0x00000010
#define RUN_SAVE_DATA	0x00000020
#define RUN_SEND_PINGS	0x00000040
#define RUN_NOSEND	0x00000080
#define	RUN_QUITTING	0x00000100
#define	RUN_TERMINATING	0x00000200
#define RUN_NOSLEEP	0x00000400
#define RUN_LIVE	0x00000800

/*************************************************************************/

/* Nickname info structure.  Each nick structure is stored in one of 256
 * lists; the list is determined by the first character of the nick.  Nicks
 * are stored in alphabetical order within lists. */

/* OLD */
typedef struct nickinfo_v1 NickInfo_V1;
typedef struct nickinfo_v3 NickInfo_V3;

/* CURRENT */
typedef struct nickinfo_ NickInfo;

#define	NI_KILLPROTECT	0x00000001  /* Kill others who take this nick */
#define	NI_SECURE	0x00000002  /* Don't recognize unless IDENTIFY'd */
#define	NI_VERBOTEN	0x00000004  /* Nick may not be registered or used */
#define	NI_IRCOP	0x00000008  /* IrcOP - Nick will not expire */
#define	NI_PRIVATE	0x00000010  /* Private - Dont show up in list */
#define	NI_SUSPENDED	0x00000020  /* Suspended - May not IDENTIFY */
#define	NI_PRIVMSG	0x00000040  /* use PRIVMSG instead of NOTICE */
#define NI_SLAVE	0x00000080  /* Nick is just a 'linked' nick */

#define	NI_IDENTIFIED	0x80000000  /* User has IDENTIFY'd */
#define	NI_RECOGNIZED	0x40000000  /* User comes from a known addy */
#define	NI_KILL_HELD	0x20000000  /* Nick is being held after a kill */

/*************************************************************************/

/* Channel info structures.  Stored similarly to the nicks, except that
 * the second character of the channel name, not the first, is used to
 * determine the list.  (Hashing based on the first character of the name
 * wouldn't get very far. ;) ) */

/* OLD */
typedef struct chaninfo_v1 ChannelInfo_V1;
typedef struct chaninfo_v3 ChannelInfo_V3;

/* CURRENT */
typedef struct chanaccess_ ChanAccess;
typedef struct autokick_ AutoKick;
typedef struct chaninfo_ ChannelInfo;

/* Retain topic even after last person leaves channel */
#define	CI_KEEPTOPIC	0x00000001
/* Don't allow non-authorized users to be opped */
#define	CI_SECUREOPS	0x00000002
/* Hide channel from ChanServ LIST command */
#define	CI_PRIVATE	0x00000004
/* Topic can only be changed by SET TOPIC */
#define	CI_TOPICLOCK	0x00000008
/* Those not allowed ops are kickbanned */
#define	CI_RESTRICTED	0x00000010
/* Don't auto-deop anyone */
#define	CI_LEAVEOPS	0x00000020
/* Don't allow any privileges unless a user is IDENTIFY'd with NickServ */
#define	CI_SECURE	0x00000040
/* Don't allow the channel to be registered or used */
#define	CI_VERBOTEN	0x00000080
/* Dont honour channel access list or founder */
#define	CI_SUSPENDED	0x00000100
/* ChanServ joins channel when its established */
#define	CI_JOIN		0x00000200
/* Revenge flags */
#define	CI_REV1		0x80000000
#define	CI_REV2		0x40000000
#define	CI_REV3		0x20000000

/* Why ChanServ PARTED or JOINED a channel */
#define CJ_SET		0
#define CJ_NOUSERS	1
#define CJ_KICKED	2
#define CJ_KILLED	3

/* Revenge levels */
#define	CR_NONE		0
#define	CR_REVERSE	1
#define	CR_DEOP		2
#define	CR_KICK		3
#define	CR_NICKBAN	4
#define	CR_USERBAN	5
#define	CR_HOSTBAN	6
#define	CR_MIRROR	7

/* Indices for cmd_access[]: */
#define	CA_AUTODEOP	0
#define	CA_AUTOVOICE	1
#define	CA_AUTOOP	2
#define	CA_READMEMO	3
#define	CA_WRITEMEMO	4
#define	CA_DELMEMO	5
#define	CA_AKICK	6
#define	CA_STARAKICK	7
#define	CA_UNBAN	8
#define	CA_ACCESS	9
#define	CA_SET		10	/* NOT FOUNDER and PASSWORD */

#define	CA_CMDINVITE	11
#define	CA_CMDUNBAN	12
#define	CA_CMDVOICE	13
#define	CA_CMDOP	14
#define	CA_CMDCLEAR	15

#define	CA_FLOOR	16
#define	CA_CAP		17
#define	CA_FOUNDER	18

#define	CA_SIZE		19	/* <--- DO NOT DELETE */

/* OVERRIDE_LEVEL levels (Based on level 2) */
#define	CO_OPER		0
#define CO_SOP		1
#define CO_ADMIN	2

/*************************************************************************/

/* MemoServ data.  Only nicks that actually have memos get records in
 * MemoServ's lists, which are stored the same way NickServ's are. */

typedef struct memo_ Memo;
typedef struct memolist_ MemoList;
typedef struct newslist_ NewsList;

#define	MS_READ		0x00000001	/* Yes, I had planned to do this */
#define MS_DELETED	0x00000002

/*************************************************************************/

/* Online user and channel data. */

typedef struct user_ User;
typedef struct channel_ Channel;
typedef struct clone_ Clone;

struct user_ {
    User *next, *prev;
    char nick[NICKMAX];
    char *username;
    char *host;				/* user's hostname */
    char *realname;
    char *server;			/* name of server user is on */
    time_t signon;
    time_t my_signon;			/* when did _we_ see the user? */
    char mode[MODEMAX];			/* see below */
    int messages;			/* How many messages in dbase */
    time_t msg_times[LASTMSGMAX];	/* Time of last X messages */
    int flood_offence;			/* Times has triggered flood prot */
    int passfail;			/* Times has failed password */
    struct u_chanlist {
	struct u_chanlist *next, *prev;
	Channel *chan;
    } *chans;				/* channels user has joined */
    struct u_chaninfolist {
	struct u_chaninfolist *next, *prev;
	ChannelInfo *chan;
    } *founder_chans;			/* channels user has identified for */
};

struct clone_ {
    Clone *next, *prev;
    char *host;
    int amount;
};

struct channel_ {
    Channel *next, *prev;
    char name[CHANMAX];
    time_t creation_time;		/* when channel was created */
    char *topic;
    char topic_setter[NICKMAX];		/* who set the topic */
    time_t topic_time;			/* when topic was set */
    char mode[MODEMAX];			/* binary modes only */
    int limit;				/* 0 if none */
    char *key;				/* NULL if none */
    int bancount, bansize;
    char **bans;
    struct c_userlist {
	struct c_userlist *next, *prev;
	User *user;
    } *users, *chanops, *voices;
};

/*************************************************************************/

/* OLD */
typedef struct akill_v1 Akill_V1;

/* CURRENT */
typedef struct ignore_ Ignore;
typedef struct servers_ Servers;
typedef struct message_ Message;
typedef struct akill_ Akill;
typedef struct allow_ Allow;
typedef struct timeout_ Timeout;
typedef struct timer_ Timer;
typedef struct sop_ Sop;

struct ignore_ {
    char nick[NICKMAX];
    time_t start;
};

struct servers_ {
    char *server;
    char *desc;
    int hops;
    int lag;
};

struct timeout_ {
    Timeout *next, *prev;
    NickInfo *ni;
    time_t settime, timeout;
    int type;
};
#define	TO_COLLIDE	0	/* Collide the user with this nick */
#define	TO_RELEASE	1	/* Release a collided nick */

struct timer_ {
    Timer *next, *prev;
    char *label;
    time_t start;
};

#define	M_LOGON	0
#define	M_OPER	1

/*************************************************************************/

/* For the Hash Tables used to accept paramaters */

typedef struct hash_ Hash;
typedef struct hash_ni_ Hash_NI;
typedef struct hash_ci_ Hash_CI;
typedef struct hash_help_ Hash_HELP;
typedef struct hash_chan_ Hash_CHAN;

struct hash_ {
    char *accept;
    short access;
    void (*process)(const char *source);
};

struct hash_ni_ {
    char *accept;
    short access;
    void (*process)(NickInfo *ni, char *param);
};

struct hash_ci_ {
    char *accept;
    short access;
    void (*process)(User *u, ChannelInfo *ci, char *param);
};

struct hash_help_ {
    char *accept;
    short access;
    const char *(*process);
};

struct hash_chan_ {
    char *accept;
    short access;
    void (*process)(const char *source, char *chan);
};

#define	H_NONE	0
#define	H_OPER	1
#define	H_SOP	2
#define	H_ADMIN	3

/*************************************************************************/

#include "datafile.h"
#include "extern.h"

/*************************************************************************/

#endif	/* SERVICES_H */
