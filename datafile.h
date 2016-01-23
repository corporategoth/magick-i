/* Data Files header for Services.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

/* struct		ver
 * ------------------------
 * nickinfo_v1		1-2
 * nickinfo_v3		3
 * nickinfo_		4-
 * chanaccess_		1-
 * autokick_		1-
 * chaninfo_v1		1-2
 * chaninfo_v3		3-4
 * chaninfo_		5-
 * memo_		1-
 * memolist_		1-
 * newslist_		1-
 * message_		1-
 * akill_v1		1
 * akill_		2-
 * allow_		1-
 */
 
/****************************************************************************
 * nick.db
 ****************************************************************************/

struct nickinfo_v1 {
    NickInfo_V1 *next, *prev;
    char nick[NICKMAX];
    char pass[PASSMAX];
    char *last_usermask;
    char *last_realname;
    time_t time_registered;
    time_t last_seen;
    long accesscount;	/* # of entries */
    char **access;	/* Array of strings */
    long flags;		/* See below */
    long reserved[4];	/* For future expansion -- set to 0 */
};

struct nickinfo_v3 {
    NickInfo_V3 *next, *prev;
    char nick[NICKMAX];
    char pass[PASSMAX];
    char *email;
    char *url;
    char *last_usermask;
    char *last_realname;
    time_t time_registered;
    time_t last_seen;
    long accesscount;	/* # of entries */
    char **access;	/* Array of strings */
    long ignorecount;   /* # of entries */
    char **ignore;	/* Array of strings */
    long flags;		/* See below */
    long reserved[4];	/* For future expansion -- set to 0 */
};

struct nickinfo_ {
    NickInfo *next, *prev;
    char nick[NICKMAX];
    char pass[PASSMAX];
    char *email;
    char *url;
    char *last_usermask;
    char *last_realname;
    time_t time_registered;
    time_t last_seen;
    long accesscount;	/* # of entries */
    char **access;	/* Array of strings */
    long ignorecount;   /* # of entries */
    char **ignore;	/* Array of strings */
    long flags;		/* See below */
    long reserved[4];	/* For future expansion -- set to 0 */
};

/****************************************************************************
 * chan.db
 ****************************************************************************/

struct chanaccess_ {
    short level;
    short is_nick;	/* 1 if this is a nick, 0 if a user@host mask.  If
			 * -1, then this entry is currently unused (a hack
			 * to get numbered lists to have consistent
			 * numbering). */
    char *name;
};

struct autokick_ {
    short is_nick;
    short pad;
    char *name;
    char *reason;
};

struct chaninfo_v1 {
    ChannelInfo_V1 *next, *prev;
    char name[CHANMAX];
    char founder[NICKMAX];		/* Always a reg'd nick */
    char founderpass[PASSMAX];
    char *desc;
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

struct chaninfo_v3 {
    ChannelInfo_V3 *next, *prev;
    char name[CHANMAX];
    char founder[NICKMAX];		/* Always a reg'd nick */
    char founderpass[PASSMAX];
    char *desc;
    char *url;
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

struct chaninfo_ {
    ChannelInfo *next, *prev;
    char name[CHANMAX];
    char founder[NICKMAX];		/* Always a reg'd nick */
    char founderpass[PASSMAX];
    char *desc;
    char *url;
    time_t time_registered;
    time_t last_used;
    long accesscount;
    ChanAccess *access;			/* List of authorized users */
    long akickcount;
    AutoKick *akick;
    char mlock_on[64], mlock_off[64];	/* See channel modes below */
    long mlock_limit;			/* 0 if no limit */
    char *mlock_key;			/* NULL if no key */
    char *last_topic;			/* Last topic on the channel */
    char last_topic_setter[NICKMAX];	/* Who set the last topic */
    time_t last_topic_time;		/* When the last topic was set */
    long flags;				/* See below */
    short *cmd_access;			/* Access levels for commands */
    long reserved[3];			/* For future expansion -- set to 0 */
};

/****************************************************************************
 * memo.db and news.db
 ****************************************************************************/

struct memo_ {
    char sender[NICKMAX];
    long number;	/* Index number -- not necessarily array position! */
    time_t time;	/* When it was sent */
    char *text;
    long reserved[4];	/* For future expansion -- set to 0 */
};

struct memolist_ {
    MemoList *next, *prev;
    char nick[NICKMAX];	/* Owner of the memos */
    long n_memos;	/* Number of memos */
    Memo *memos;	/* The memos themselves */
    long reserved[4];	/* For future expansion -- set to 0 */
};

struct newslist_ {
    NewsList *next, *prev;
    char chan[CHANMAX];	/* Owner of the memos */
    long n_newss;	/* Number of memos */
    Memo *newss;	/* The memos themselves */
    long reserved[4];	/* For future expansion -- set to 0 */
};

/****************************************************************************
 * others
 ****************************************************************************/

struct message_ {
    char *text;
    short type;
    char who[NICKMAX];
    time_t time;
};

struct akill_v1 {
    char *mask;
    char *reason;
    time_t time;
};

struct akill_ {
    char *mask;
    char *reason;
    char who[NICKMAX];
    time_t time;
};

struct allow_ {
    char *host;
    int amount;
    char *reason;
    char who[NICKMAX];
    time_t time;
};

struct sop_ {
    char nick[NICKMAX];
};
