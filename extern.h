/* Prototypes and external variable declarations.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifndef EXTERN_H
#define EXTERN_H
#include "cfgopts.h"

#define E extern

#ifdef WIN32
typedef unsigned long gid_t;
#define bcopy(src, dst, n )     memcpy( dst, src, n )
#endif


/* Names */
E char s_GlobalNoticer[NICKMAX];
E char s_NickServ[NICKMAX];
E char s_ChanServ[NICKMAX];
E char s_OperServ[NICKMAX];
E char s_MemoServ[NICKMAX];
E char s_HelpServ[NICKMAX];
E char s_IrcIIHelp[NICKMAX];
E char s_DevNull[NICKMAX];
E char s_Outlet[NICKMAX];


/**** cfgopts.c ****/

E int read_options(void);
E int check_config(void);

/**** channels.c ****/

E Channel *chanlist;

E void get_channel_stats(long *nrec, long *memuse);
E void send_channel_list(const char *who, const char *user, const char *s);
E void send_chanmode_list(const char *who, const char *user, const char *s);
E void send_channel_users(const char *who, const char *user, const char *chan);
E Channel *findchan(const char *chan);
E void chan_adduser(User *user, const char *chan);
E void chan_deluser(User *user, Channel *c);
E void do_cmode(const char *source, int ac, char **av);
E void do_topic(const char *source, int ac, char **av);
E int validchan(char *chan);
E int get_bantype(const char *mask);
E void change_cmode(const char *who, const char *chan, const char *mode, const char *pram);
E void kick_user(const char *who, const char *chan, const char *nick, const char *reason);

/**** chanserv.c ****/

E ChannelInfo *chanlists[256];
E char chanserv_db[512];
E int channel_expire;
E int akick_max;
E char def_akick_reason[512];

E void listchans(int count_only, const char *chan);
E void get_chanserv_stats(long *nrec, long *memuse);

E int get_access(User *user, ChannelInfo *ci);
E int check_access(User *user, ChannelInfo *ci, int what);
E int get_justaccess(const char *mask, ChannelInfo *ci);
E int check_justaccess(const char *mask, ChannelInfo *ci, int what);
E int is_founder(User *user, NickInfo *ni, ChannelInfo *ci);
E int is_justfounder(NickInfo *ni, ChannelInfo *ci);
E void chanserv(const char *source, char *buf);
E void load_cs_dbase(void);
E void save_cs_dbase(void);
E void check_modes(const char *chan);
E int delchan(ChannelInfo *ci);
E int check_valid_op(User *user, const char *chan, int newchan);
E int check_valid_voice(User *user, const char *chan, int newchan);
E int check_should_op(User *user, const char *chan);
E int check_kick(User *user, const char *chan);
E void do_cs_protect(const char *chan);
E void do_cs_unprotect(const char *chan);
E void do_cs_join(const char *chan);
E void do_cs_part(const char *chan);
E void do_cs_reop(const char *chan);
E void do_revenge(const char *chan, const char *actioner,
	const char *actionee, int action);
E int do_cs_revenge(const char *chan, const char *actioner,
	const char *actionee, int action);
E int get_revenge_level(ChannelInfo *ci);
E void record_topic(const char *chan);
E void restore_topic(const char *chan);
E int check_topiclock(const char *chan);
E void expire_chans(void);
E ChannelInfo *cs_findchan(const char *chan);

/**** helpserv.c ****/

E void helpserv(const char *whoami, const char *source, char *buf);
E char helpserv_dir[512];

/**** main.c ****/

E char remote_server[256], server_name[256], server_desc[128], services_user[512];
E char services_host[512], services_dir[512], log_filename[512], *offreason;
E char *quitmsg, inbuf[BUFSIZE];
E int remote_port, update_timeout, services_level, servsock, server_relink;
E int starthresh, file_version;
E float tz_offset;
E long runflags;
E gid_t file_gid;
E time_t start_time, reset_time, last_update;
E Boolean_T nickserv_on, chanserv_on, helpserv_on, irciihelp_on, memoserv_on;
E Boolean_T memos_on, news_on, devnull_on, operserv_on, outlet_on, akill_on;
E Boolean_T clones_on, globalnoticer_on, show_sync_on;

E void open_log();
E void close_log();
E void write_log(const char *fmt,...);
E void log_perror(const char *fmt,...);
E void fatal(const char *fmt,...);
E void fatal_perror(const char *fmt,...);
E void check_file_version(FILE *f, const char *filename);
E void write_file_version(FILE *f, const char *filename);
E const char *any_service();
E int i_am_backup();
E int is_services_nick(const char *nick);
E int is_justservices_nick(const char *nick);
E void introduce_user(const char *user);
E int is_server(const char *nick);
E int get_file_version(FILE *f, const char *filename);

/**** memoserv.c ****/

E void get_memoserv_stats(long *nrec, long *memuse);
E char memoserv_db[512];
E char newsserv_db[512];
E int news_expire;

E void memoserv(const char *source, char *buf);
E MemoList *memolists[256];
E void load_ms_dbase(void);
E void save_ms_dbase(void);
E void check_memos(const char *nick);
E MemoList *find_memolist(const char *nick);
E void del_memolist(MemoList *ml);
E NewsList *newslists[256];
E void load_news_dbase(void);
E void save_news_dbase(void);
E void check_newss(const char *chan, const char *source);
E void expire_news(void);
E NewsList *find_newslist(const char *chan);
E void del_newslist(NewsList *nl);
E void get_newsserv_stats(long *nrec, long *memuse);

/**** misc.c ****/

#if !HAVE_SNPRINTF
# if BAD_SNPRINTF
#  define vsnprintf my_vsnprintf
#  define snprintf my_snprintf
# endif
E int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
E int snprintf(char *buf, size_t size, const char *fmt, ...);
#endif
#if !HAVE_STRICMP && !HAVE_STRCASECMP
E int stricmp(const char *s1, const char *s2);
E int strnicmp(const char *s1, const char *s2, size_t len);
#endif
#if !HAVE_STRDUP
E char *strdup(const char *s);
#endif
#if !HAVE_STRSPN
E size_t strspn(const char *s, const char *accept);
#endif
E char *stristr(char *s1, char *s2);
#if !HAVE_STRERROR
E char *strerror(int errnum);
#endif
#if !HAVE_STRSIGNAL
char *strsignal(int signum);
#endif
E char *sgets2(char *buf, long size, int sock);
E char *strscpy(char *d, const char *s, size_t len);
E void *smalloc(long size);
E void *scalloc(long elsize, long els);
E void *srealloc(void *oldptr, long newsize);
E char *sstrdup(const char *s);
E char *merge_args(int argc, char **argv);
E int match_wild(const char *pattern, const char *str);
E int match_wild_nocase(const char *pattern, const char *str);
E char *month_name(int month);
E char *strupper(char *s);
E char *strlower(char *s);
E char *read_string(FILE *f, const char *filename);
E void write_string(const char *s, FILE *f, const char *filename);
E char *myctoa(char c);
E char *myitoa(int num);
E char *time_ago(time_t time, int call);
E char *disect_time(time_t time, int call);
E Hash *get_hash(const char *source, const char *cmd, Hash *hash_table);
E Hash_NI *get_ni_hash(const char *source, const char *cmd, Hash_NI *hash_table);
E Hash_CI *get_ci_hash(const char *source, const char *cmd, Hash_CI *hash_table);
E Hash_HELP *get_help_hash(const char *source, const char *cmd, Hash_HELP *hash_table);
E Hash_CHAN *get_chan_hash(const char *source, const char *cmd, Hash_CHAN *hash_table);
E int override(const char *source, int level);
E int override_level_val;
E int hasmode (const char *mode, const char *modestr);
E char *changemode (const char *mode, const char *modestr);

/**** nickserv.c ****/

E NickInfo *nicklists[256];
E Timeout *timeouts;
E char nickserv_db[512];
E int nick_expire;
E int release_timeout;
E int wait_collide;
E int passfail_max;
E int S_nick_reg, S_nick_drop, S_nick_ghost, S_nick_kill, S_nick_ident;
E int S_nick_recover, S_nick_release, S_nick_link;
E int S_nick_getpass, S_nick_forbid, S_nick_suspend, S_nick_unsuspend;

E void listnicks(int count_only, const char *nick);
E void get_nickserv_stats(long *nrec, long *memuse);

E void time_to_die(void);
E int delnick(NickInfo *ni);
E void nickserv(const char *source, char *buf);
E void load_ns_dbase(void);
E void save_ns_dbase(void);
E int validate_user(User *u);
E void cancel_user(User *u);
E void check_timeouts(void);
E void expire_nicks(void);
E void do_real_identify (const char *whoami, const char *source);
E NickInfo *findnick(const char *nick);
E NickInfo *host(NickInfo *ni);
E NickInfo *slave(const char *nick, int num);
E int countslave(const char *nick);
E int issibling(NickInfo *ni, const char *target);
E int slavecount(const char *nick);
E int userisnick(const char *nick);
E long getflags(NickInfo *ni);
E int is_on_ignore(const char *source, const char *target);

/**** operserv.c ****/

E char akill_db[512];
E char clone_db[512];
E char sop_db[512];
E char message_db[512];
E int akill_expire;
E int clones_allowed;
E char def_clone_reason[512];

E void operserv(const char *source, char *buf);
/* dummy val, someone wanna make an upper level #define for it?*/
E Sop *sops;
E int nsop, sop_size;
E void load_sop(void);
E void save_sop(void);
E int is_services_op(const char *nick);
E int is_justservices_op(const char *nick);
E Message *messages;
E int nmessage, message_size;
E void load_message(void);
E void save_message(void);
E Akill *akills;
E int nakill, akill_size;
E void load_akill(void);
E void save_akill(void);
E int check_akill(const char *nick, const char *username, const char *host);
E void expire_akill(void);
E int del_akill(const char *mask, int call);
E Clone *clonelist;
E Allow *clones;
E int nclone, clone_size;
E void clone_add(const char *nick, const char *host);
E void clone_del(const char *host);
E int del_clone(const char *host);
E void clones_add(const char *nick, const char *host);
E void clones_del(const char *host);
E void load_clone(void);
E void save_clone(void);

/**** process.c ****/

E int ignorecnt, ignore_size;
E Ignore *ignore;
E int servcnt, serv_size;
E Servers *servlist;
E Timer *pings;
E char motd_filename[512];
E int flood_messages;
E int flood_time;
E int ignore_time;
E int ignore_offences;

E void addtimer (const char *label);
E void deltimer (const char *label);
E time_t gettimer (const char *label);
E int is_ignored (const char *nick);
E int split_buf(char *buf, char ***argv, int colon_special);
E void process(void);


/**** send.c ****/

E void send_cmd(const char *source, const char *fmt, ...);
E void vsend_cmd(const char *source, const char *fmt, va_list args);
E void wallops(const char *whoami, const char *fmt, ...);
E void notice(const char *source, const char *dest, const char *fmt, ...);
E void notice_list(const char *source, const char *dest, const char **text);
E void noticeall(const char *source, const char *fmt, ...);

/**** sockutil.c ****/

E int sgetc(int s);
E char *sgets(char *buf, unsigned int len, int s);
E int sputs(char *str, int s);
E int sockprintf(int s, char *fmt,...);
E int conn(char *host, int port);
E void disconn(int s);
E int read_timeout;


/**** users.c ****/

E int usercnt, opcnt, maxusercnt;
E User *userlist;
E char services_admin[BUFSIZE];

E void send_user_list(const char *who, const char *user, const char *x);
E void send_usermask_list(const char *who, const char *user, const char *x);
E void send_usermode_list(const char *who, const char *user, const char *x);
E void get_user_stats(long *nusers, long *memuse);
E User *finduser(const char *nick);
E User *findusermask(const char *mask, int matchno);
E int countusermask(const char *mask);

E void change_user_nick(User *u, const char *nick);
E void do_nick(const char *source, int ac, char **av);
E void do_join(const char *source, int ac, char **av);
E void do_part(const char *source, int ac, char **av);
E void do_kick(const char *source, int ac, char **av);
E void do_svumode(const char *source, int ac, char **av);
E void do_umode(const char *source, int ac, char **av);
E void do_quit(const char *source, int ac, char **av);
E void do_kill(const char *source, int ac, char **av);
E void delete_user(User *user);

E int is_services_admin(const char *nick);
E int is_justservices_admin(const char *nick);
E int is_oper(const char *nick);
E int is_on_chan(const char *nick, const char *chan);
E int is_chanop(const char *nick, const char *chan);
E int is_voiced(const char *nick, const char *chan);

E void kill_user(const char *who, const char *nick, const char *reason);
E int match_usermask(const char *mask, User *user);
E void split_usermask(const char *mask, char **nick, char **user, char **host);
E char *create_mask(User *u);

#endif	/* EXTERN_H */
