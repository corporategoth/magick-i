/* Main header for Services.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
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
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <grp.h>

#include <ctype.h>

/* We have our own versions of toupper()/tolower(). */
#undef tolower
#undef toupper
#define	tolower tolower_
#define	toupper toupper_
extern int toupper(char), tolower(char);

/* These should be read by config.h */
#undef	NICKSERV
#undef	CHANSERV
#undef	HELPSERV
#undef	IRCIIHELP
#undef	MEMOSERV
#undef	MEMOS
#undef	NEWS
#undef	DEVNULL
#undef	OPERSERV
#undef	AKILL
#undef	CLONES
#undef	GLOBALNOTICER

#include "config.h"

/* Satisfy dependancies */
#ifndef NICKSERV
#  undef CHANSERV
#  undef MEMOSERV
#endif
#ifndef CHANSERV
#  undef NEWS
#endif
#ifndef MEMOSERV
#  undef NEWS
#  undef MEMOS
#endif
#if !defined(MEMOS) && !defined(NEWS)
#  undef MEMOSERV
#endif
#ifndef OPERSERV
#  undef GLOBALNOTICER
#  undef OUTLET
#  undef AKILL
#  undef CLONES
#endif
#if CLONES_ALLOWED < 1
#  undef CLONES
#endif

/* Stupid catchers! */
#if SERVICES_LEVEL < 1
#  error Cannot set SERVICES_LEVEL < 1 - edit the config.h
#endif
#if (TZ_OFFSET >= 24) || (TZ_OFFSET <= -24)
#  error TZ_OFFSET must fall between -24 and 24 - edit the config.h
#endif
#if UPDATE_TIMEOUT < 30
#  error Cannot set UPDATE_TIMEOUT < 30 - edit the config.h
#endif
#if READ_TIMEOUT < 1
#  error Cannot set READ_TIMEOUT < 1 - edit the config.h
#endif
#if BUFSIZE < 64
#  error Cannot set BUFSIZE < 64 - edit the config.h
#endif
#if CHANMAX < 16
#  error Cannot set CHANMAX < 16 - edit the config.h
#endif
#if NICKMAX < 9
#  error Cannot set NICKMAX < 9 - edit the config.h
#endif
#if PASSMAX < 5
#  error Cannot set PASSMAX < 5 - edit the config.h
#endif

/*************************************************************************/

/* Version number for data files; if structures below change, increment
 * this.  (Otherwise -very- bad things will happen!)
 * TO DATE: 1  - Original
 *          2  - Sreamlined Auto Kill Database
 *          3  - Added URL/E-Mail to Nick Database
 *          4  - Added IGNORE (memo reject)
 */

/* If your not using MEMOS, its no use being on FILE_VERSION 4.  Change
   Below if you want to override (but why would you?). */

#ifdef MEMOS
# define FILE_VERSION	4
#else
# define FILE_VERSION	3
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

/*************************************************************************/

/* Nickname info structure.  Each nick structure is stored in one of 256
 * lists; the list is determined by the first character of the nick.  Nicks
 * are stored in alphabetical order within lists. */

#ifdef NICKSERV
typedef struct nickinfo_ NickInfo;
struct nickinfo_ {
    NickInfo *next, *prev;
    char nick[NICKMAX];
    char pass[PASSMAX];
#if FILE_VERSION > 2
    char *email;
    char *url;
#endif
    char *last_usermask;
    char *last_realname;
    time_t time_registered;
    time_t last_seen;
    long accesscount;	/* # of entries */
    char **access;	/* Array of strings */
#if FILE_VERSION > 3
    long ignorecount;   /* # of entries */
    char **ignore;	/* Array of strings */
#endif
    long flags;		/* See below */
    long reserved[4];	/* For future expansion -- set to 0 */
};

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
#endif /* NICKSERV */

/*************************************************************************/

/* Channel info structures.  Stored similarly to the nicks, except that
 * the second character of the channel name, not the first, is used to
 * determine the list.  (Hashing based on the first character of the name
 * wouldn't get very far. ;) ) */

#ifdef CHANSERV
/* Access levels for users. */
typedef struct {
    short level;
    short is_nick;	/* 1 if this is a nick, 0 if a user@host mask.  If
			 * -1, then this entry is currently unused (a hack
			 * to get numbered lists to have consistent
			 * numbering). */
    char *name;
} ChanAccess;

/* AutoKick data. */
typedef struct {
    short is_nick;
    short pad;
    char *name;
    char *reason;
} AutoKick;

typedef struct chaninfo_ ChannelInfo;
struct chaninfo_ {
    ChannelInfo *next, *prev;
    char name[CHANMAX];
    char founder[NICKMAX];		/* Always a reg'd nick */
    char founderpass[PASSMAX];
    char *desc;
#if FILE_VERSION > 2
    char *url;
#endif
    time_t time_registered;
    time_t last_used;
    long accesscount;
    ChanAccess *access;			/* List of authorized users */
    long akickcount;
    AutoKick *akick;
    short mlock_on, mlock_off;		/* See channel modes below */
    long mlock_limit;			/* 0 if no limit */
    char *mlock_key;			/* NULL if no key */
    char *last_topic;			/* Last topic on the channel */
    char last_topic_setter[NICKMAX];	/* Who set the last topic */
    time_t last_topic_time;		/* When the last topic was set */
    long flags;				/* See below */
    short *cmd_access;			/* Access levels for commands */
    long reserved[3];			/* For future expansion -- set to 0 */
};

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
#endif

/*************************************************************************/

/* MemoServ data.  Only nicks that actually have memos get records in
 * MemoServ's lists, which are stored the same way NickServ's are. */

#ifdef MEMOSERV
typedef struct memo_ Memo;

struct memo_ {
    char sender[NICKMAX];
    long number;	/* Index number -- not necessarily array position! */
    time_t time;	/* When it was sent */
    char *text;
    long reserved[4];	/* For future expansion -- set to 0 */
};


#ifdef MEMOS
typedef struct memolist_ MemoList;
#endif
#ifdef NEWS
typedef struct newslist_ NewsList;
#endif

#ifdef MEMOS
struct memolist_ {
    MemoList *next, *prev;
    char nick[NICKMAX];	/* Owner of the memos */
    long n_memos;	/* Number of memos */
    Memo *memos;	/* The memos themselves */
    long reserved[4];	/* For future expansion -- set to 0 */
};
#endif

#ifdef NEWS
struct newslist_ {
    NewsList *next, *prev;
    char chan[CHANMAX];	/* Owner of the memos */
    long n_newss;	/* Number of memos */
    Memo *newss;	/* The memos themselves */
    long reserved[4];	/* For future expansion -- set to 0 */
};
#endif 
#endif /* MEMOSERV && NICKSERV */

/*************************************************************************/

/* Online user and channel data. */

typedef struct user_ User;
typedef struct channel_ Channel;
#ifdef CLONES
typedef struct clone_ Clone;
#endif

struct user_ {
    User *next, *prev;
    char nick[NICKMAX];
    char *username;
    char *host;				/* user's hostname */
    char *realname;
    char *server;			/* name of server user is on */
    time_t signon;
    time_t my_signon;			/* when did _we_ see the user? */
    short mode;				/* see below */
    int messages;			/* How many messages in dbase */
    time_t msg_times[FLOOD_MESSAGES];	/* Time of last X messages */
    int flood_offence;			/* Times has triggered flood prot */
    struct u_chanlist {
	struct u_chanlist *next, *prev;
	Channel *chan;
    } *chans;				/* channels user has joined */
#ifdef CHANSERV
    struct u_chaninfolist {
	struct u_chaninfolist *next, *prev;
	ChannelInfo *chan;
    } *founder_chans;			/* channels user has identified for */
#endif
};

#define	UMODE_O	0x0001
#define	UMODE_I	0x0002
#define	UMODE_S	0x0004
#define	UMODE_W	0x0008
#define	UMODE_G	0x0010

#ifdef CLONES
struct clone_ {
    Clone *next, *prev;
    char *host;
    int amount;
};
#endif

struct channel_ {
    Channel *next, *prev;
    char name[CHANMAX];
    time_t creation_time;		/* when channel was created */
    char *topic;
    char topic_setter[NICKMAX];		/* who set the topic */
    time_t topic_time;			/* when topic was set */
    int mode;				/* binary modes only */
    int limit;				/* 0 if none */
    char *key;				/* NULL if none */
    int bancount, bansize;
    char **bans;
    struct c_userlist {
	struct c_userlist *next, *prev;
	User *user;
    } *users, *chanops, *voices;
};

#define	CMODE_I	0x01
#define	CMODE_M	0x02
#define	CMODE_N	0x04
#define	CMODE_P	0x08
#define	CMODE_S	0x10
#define	CMODE_T	0x20
#define	CMODE_K	0x40			/* These two used only by ChanServ */
#define	CMODE_L	0x80

/*************************************************************************/

typedef struct ignore_ Ignore;
typedef struct servers_ Servers;
typedef struct message_ Message;
#ifdef AKILL
typedef struct akill_ Akill;
#endif
#ifdef CLONES
typedef struct allow_ Allow;
#endif
typedef struct timeout_ Timeout;
typedef struct timer_ Timer;

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

struct message_ {
    char *text;
    short type;
    char who[NICKMAX];
    time_t time;
};
#define	M_LOGON	0
#define	M_OPER	1

#ifdef AKILL
struct akill_ {
    char *mask;
    char *reason;
    char who[NICKMAX];
    time_t time;
};
#endif

#ifdef CLONES
struct allow_ {
    char *host;
    int amount;
    char *reason;
    char who[NICKMAX];
    time_t time;
};
#endif

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

#include "extern.h"

/*************************************************************************/

#endif	/* SERVICES_H */
