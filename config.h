/* Magick configuration.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifndef CONFIG_H
#define	CONFIG_H


/******* General configuration *******/

/* The remote server and port to use, and the password for the link. */
#define	REMOTE_SERVER	"srealm.darker.net"
#define	REMOTE_PORT	9666
#define	PASSWORD	"LinkPassword"

/* Information about us as a server. */
#define	SERVER_NAME	"hell.darker.net"
#define	SERVER_DESC	"DarkerNet's IRC services"
#define	SERVICES_USER	"reaper"
#define	SERVICES_HOST	"darker.net"

/* Services level means the priority this version of services has over
 * other services on the net (the lower number, the higher priority).
 * This is mainly for networks with backup services and want the backups
 * to kick in should the primaries die.  If this is set >1 then services
 * are in READ ONLY mode - no database modification will be allowed.
 * Cannot be set below 1.
 */
#define	SERVICES_LEVEL	1 

/* This is for backup services (sharing the same set of databases as the
 * primary versions) to keep up to date with the "X Time Ago" displays.
 * Essentially what needs to go here how many HOURS difference there is
 * between you and the primary services.  (Can be .5 if applicable)
 *
 * Eg. If main services are EST (us) and you are CST (us), this would be 1.
 *     If main services are EST (us) and you are EST (au), this would be 15.
 *         But, depending on Daylight Savings, can goto 14 or 16.
 *     If main services are EST (au) and you are CST (au), this would be 1.5.
 *
 * I'm sure you get the drift -- if you ARE the mains (SERVICES_LEVEL 1), this
 * value is ignored.
 */
#define	TZ_OFFSET	0	/* 0 == Same TimeZone */

/* PICK AND CHOOSE:
 *    To select a module, #define it, to exclude it, #undef it.
 *    a "relies on" means it will be undef if the specified module is not
 *    defined (eg. CHANSERV is disabled if NICKSERV is undef)
 *    "and" denotes both must be enabled, "or" denotes at least one.
 */

#define	NICKSERV	/* */
#define	CHANSERV	/* relies on NICKSERV */
#define	HELPSERV	/* */
#define	IRCIIHELP	/* */
#define	MEMOSERV	/* relies on NICKSERV and (MEMOS or NEWS) */
#define	MEMOS		/* relies on MEMOSERV */
#define	NEWS		/* relies on MEMOSERV and CHANSERV */
#define	DEVNULL		/* */
#define	OPERSERV	/* */
#define OUTLET		/* relies on OPERSERV */
#define	AKILL		/* relies on OPERSERV */
#define	CLONES		/* relies on OPERSERV */
#define	GLOBALNOTICER	/* relies on OPERSERV */

/* NOTES ON MODULES:
 *     - NICKSERV, CHANSERV, HELPSERV, IRCIIHELP, MEMOSERV, DEVNULL
 *       OPERSERV and GLOBALNOTICER just activate the nicks (and
 *       their associated functions).
 *     - MEMOS activates USER memos.
 *     - NEWS activates CHANNEL memos.
 *     - OUTLET activates an OperServ clone (see below).
 *     - AKILL activates the Auto KILL list.
 *     - CLONES activates the internal clone detection/noticing.
 */

/* Be original ... */

#define	NICKSERV_NAME		"NickServ"
#define	CHANSERV_NAME		"ChanServ"
#define	OPERSERV_NAME		"OperServ"
#define	MEMOSERV_NAME		"MemoServ"
#define	HELPSERV_NAME		"HelpServ"
#define	GLOBALNOTICER_NAME	"Death"
#define	DEVNULL_NAME		"DevNull"
#define	IRCIIHELP_NAME		"IrcIIHelp"

/* This is an outlet equivilant to operserv, that is always
 * there, and is named SERVICES_PREFIX-SERVICES_LEVEL, ie.
 * if this is "Magick-" and your services_level is 1, the nick
 * would be Magick-1, for services_level 2, it would be Magick-2
 * and so on.  This is so you can still access all versions of
 * Services at once (no matter which is currently 'OperServ').
 */
#define SERVICES_PREFIX		"Magick-"

/* This replaces old IRCOP_OVERRIDE.  This allows Irc OP's to use
 * certain ChanServ functions without being on the Access List.
 *
 * 0 = No Override
 * 1 = All IrcOP
 * 2 = INVITE, OP, VOICE, etc are IrcOP Functions
 *     CLEAR, etc are Services OP Functions
 *     SET FOUNDER, etc are Services Admin Functions
 * 3 = All Services OP
 * 4 = INVITE, OP, VOICE, etc are Services OP Functions
 *     CLEAR, etc are Services Admin Functions
 *     SET FOUNDER, etc are Services Admin Functions
 * 5 = All Services Admin
 */
#define	OVERRIDE_LEVEL	2

/* Log filename in services directory */
#define	LOG_FILENAME	"magick.log"

/* File for Message of the Dat (/motd) */
#define	MOTD_FILENAME	"magick.motd"

/* Database filenames */
#define	NICKSERV_DB	"nick.db"
#define	CHANSERV_DB	"chan.db"
#define	MEMOSERV_DB	"memo.db"
#define	NEWSSERV_DB	"news.db"
#define	AKILL_DB	"akill.db"
#define	CLONE_DB	"clone.db"
#define SOP_DB		"sop.db"
#define MESSAGE_DB	"message.db"

/* File containing process ID */
#define	PID_FILE	"magick.pid"

/* Subdirectory for help files */
#define	HELPSERV_DIR	"helpfiles"

/* Delay (or if) between attempting to reconnect to parent server if
 * server is SQUIT or parent server dies.  Undef or ser -1 to disable.
 * This means services will NOT die upon their parent server doing so
 * if defined, but it also means it will create 2 log entries for every
 * (specified) seconds that the parent server is offline.
 */
#define	SERVER_RELINK	5

/* Delay (in seconds) between database updates.  (Incidentally, this is
 * also how often we check for nick/channel expiration.)
 */
#define	UPDATE_TIMEOUT	300

/* Delay (in seconds) between server pings.  Services ping all servers
 * to try to keep an up to date lag check.  This is how often to do it.
 */
#define	PING_FREQUENCY	30

/* Delay (in seconds) before we time out on a read and do other stuff,
 * like checking NickServ timeouts.
 */
#define	READ_TIMEOUT	10



/******* ChanServ configuration *******/

/* Number of days before a channel expires */
#define	CHANNEL_EXPIRE	14

/* Maximum number of AKICKs on a single channel. */
#define	AKICK_MAX	32

/* Default reason for AKICK if none is given. */
#define	DEF_AKICK_REASON "You have been banned from the channel"



/******* NickServ configuration *******/

/* Number of days before a nick registration expires */
#define	NICK_EXPIRE	28

/* Delay (in seconds) before a NickServ-collided nick is released. */
#define	RELEASE_TIMEOUT	60



/******* MemoServ configuration *******/

/* Number of days before news items expire */
#define	NEWS_EXPIRE	21



/******* OperServ configuration *******/

/* Who are the Services Admins? (space-separated list of NICKNAMES ONLY)
 * Note that to modify the Services OP List the user must:
 *	- Have a nickname in the list below
 *      - Be an IRC Operator (have mode +o enabled)
 *	- Identify with NickServ (therefore, the nick must be registered)
 */
#define	SERVICES_ADMIN		"PreZ Lord_Striker"

/* Maximum number of Services Operators allowed */
#define MAXSOPS			64

/* Number of days before erasing akills not set with PAKILL */
#define	AKILL_EXPIRE		7

/* How many CLONES are allowed by default? */
#define	CLONES_ALLOWED		2

/* Default reason for AKICK if none is given. */
#define	DEF_CLONE_REASON "Exceeded maximum amount of connections from one host."



/******* DevNull configuration *******/

/* The below are read together.  Flood Protection will be triggered against
 * a user if FLOOD_MESSAGES messages are recieved in FLOOD_TIME seconds.
 */
#define	FLOOD_MESSAGES	5
#define	FLOOD_TIME	10

/* How long to ignore user when flood protection is triggered */
#define	IGNORE_TIME	20

/* How many offences will cause user to be added to perm ignore list */
#define	IGNORE_OFFENCES	5



/******* Miscellaneous - it should be save to leave these untouched *******/

/* Non-Star chars needed for AKILL, CLONE and AKICK
 *    3 means *.com will work, but *.au wont.
 *    4 means *.com wont work, need *a.com
 */
#define	STARTHRESH	4

/* Extra warning: if you change these, your data files will be unusable! */

/* Size of input buffer */
#define	BUFSIZE		1024

/* Maximum length of a channel name */
#define	CHANMAX		64

/* Maximum length of a nickname */
#define	NICKMAX		32

/* Maximum length of a password */
#define	PASSMAX		32



/**************************************************************************/

/* System-specific defines */

/* IF your Nick collide works where NEWER NICK takes presidense, then
 * define this (some do - or at least, it works out that way - test it
 * undef'd - if you find your BACKUP services (if any) are getting the
 * services nick's even when your REAL ones are online - define this).
 *
 * OK - I created this when my services had this problem - I later found
 * out the cause - each backup of services needs to have a different set
 * of user@host's for its users (either user or host or both can be
 * different, doesnt matter) - TRY CHANGING THAT before defining this -
 * I'm leaving it in just incase there ARE some wierd ircd's out there.
 */
#undef	WIERD_COLLIDE

/* IF you cant READ or FORWARD (from channel) memos, define this
 * as some systems are REALLY stupid about it *shrug* (ONLY define
 * this if you are having problems!!
 */
#undef	STUPID

#undef	DAL_SERV		/* This should be handled by sysconf.h */
#include "sysconf.h"

#if !HAVE_STRICMP && HAVE_STRCASECMP
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

#endif	/* CONFIG_H */
