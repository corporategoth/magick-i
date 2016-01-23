/* Text outputs for services.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

/* WARNING:  I take no responsibility if Magick crashes after you start
 *           changing these values (results are unpredictable).
 *
 * Below is a list of the % values, if there are any in text you are
 * editing, make sure they are still there after editing, else you will
 * need to rewrite the calling functions to acomodate for it.
 *
 * %s == String			%c == Single Character
 * %d == Integer		%l == Numeric (%#.#l to adjust DP, etc)
 * 
 * Below is a list of the IRC Formatting helpers, add or remove at will.
 * Remember, you use the same code to start and end the formatting.
 * When you end ONE type of formatting, it ends ALL types active.  You
 * must use \15 when going strait from one format to another (no space).
 * You must make these 3-digit numbers if followed by a number.
 *
 * \2  == (^B) Bold		\37 == (^_) Underline
 * \22 == (^V) Reverse		\15 == (^O) Cancel Formatting
 *
 * If you want to use mIRC colour schemes, the syntax is listed below, but
 * it is NOT recommended.  Remember, you cannot have a number following a
 * colour code. X == FG Colour, Y == BG Colour (listed below).
 * These must be 3-digit numbers, as they are followed by a number.
 *
 * \003X == Change foreground	\003X,Y == Change foreground AND background
 * \003  == End colour (if not followed by any numbers)
 *
 * 0  == White			8  == Yellow
 * 1  == Black			9  == Bright Green
 * 2  == Blue			10 == Cyan
 * 3  == Green			11 == Bright Cyan
 * 4  == Red			12 == Bright Blue
 * 5  == Orange			13 == Bright Magenta
 * 6  == Magenta		14 == Grey
 * 7  == Bright Red		15 == Bright Grey
 */

#ifndef MESSAGES_H
#define MESSAGES_H

/****************************************************************************
 ********************************** ERRORS **********************************
 ****************************************************************************/

/* Log stuff (NO FORMATTING HERE!) */
#define ERR_READ_DB		"Error reading %s database."
#define ERR_WRITE_DB		"Error writing %s database."
#define ERR_UNKNOWN_SERVMSG	"Unknown message from server (%s)."

/* Incorrect Syntax */
#define ERR_UNKNOWN_COMMAND	"Unrecognized command \2%s\2.  Type \2/msg %s HELP\2 for help."
#define ERR_UNKNOWN_OPTION	"Unrecognized option \2%s\2.  Type \2/msg %s HELP %s\2 for help."
#define ERR_MORE_INFO		"Type \2/msg %s HELP %s\2 for more information."
#define ERR_NOHELP		"No help available for command \2%s\2."
#define ERR_STARTHRESH		"Entry must have at least %d non-*, ? or . characters."
#define ERR_REQ_PARAM		"%s requires a parameter.\n"
#define ERR_NEED_MLOCK_PARAM	"Paramater required for MLOCK +%c."
#define ERR_MLOCK_POSITIVE	"Paramater for MLOCK +%c must be positive."
#define ERR_MLOCK_UNKNOWN	"Unknown MLOCK character \2%c\2 ignored."
#define ERR_SECURE_NICKS	"SECURE access lists may only contain registered nicknames."
#define ERR_FORMATTING		"You must specify \2%s\2 only."
#define ERR_MAY_NOT_HAVE	"You may not use a %c character."
#define ERR_MUST_HAVE		"You must include a %c character."

/* Access Denied */
#define ERR_WRONG_PASSWORD	"Password incorrect."
#define ERR_ACCESS_DENIED	"Access denied."
#define ERR_SUSPENDED_NICK	"Access denied for SUSPENDED nicknames."
#define ERR_SUSPENDED_CHAN	"Access denied for SUSPENDED channels."
#define ERR_TEMP_DISABLED	"Sorry, \2%s\2 is temporarily disabled."
#define ERR_FAILED		"Sorry, \2%s\2 failed."
#define ERR_YOU_DONT_EXIST	"Sorry, could not obtain your user information."
#define ERR_CHAN_NOTVALID	"\2%s\2 is not a valid channel name."
#define ERR_READ_ONLY		"\2WARNING\2: Services are in read-only mode, changes will not be saved."

/* Need verification */
#define ERR_NEED_PASSWORD	"Password identification required for \2%s\2."
#define ERR_NEED_OPS		"You must be a channel operator for \2%s\2."
#define ERR_IDENT_NICK_FIRST	"Please retry after typing \2/MSG %s IDENTIFY \37password\37."
#define ERR_IDENT_CHAN_FIRST	"Please retry after typing \2/MSG %s IDENTIFY \37%s password\37."
#define ERR_NICK_FORBIDDEN	"This nickname may not be used.  Please choose another."
#define ERR_NICK_OWNED		"This nickname is owned by someone else.  Please choose another."
#define ERR_NICK_SECURE		"This nickname is registered and protected."
#define ERR_NICK_IDENTIFY	"If this is your nick, type \2/MSG %s IDENTIFY \37password\37"
#define ERR_WILL_KILL_YOU	"If you do not change within one minute, you will be disconnected."

/* Done as or to wrong person */
#define ERR_MUST_BE_HOST	"\2%s\2 must be done as the HOST nickname."
#define ERR_CANT_BE_HOST	"\2%s\2 may not be done as the HOST nickname."
#define ERR_MUST_BE_LINK	"\2%s\2 must be done as a LINKED nickname."
#define ERR_CANT_BE_LINK	"\2%s\2 may not be done on a LINKED nickname."
#define ERR_MUST_BETO_HOST	"\2%s\2 must be performed on a HOST nickname."
#define ERR_CANT_BETO_HOST	"\2%s\2 may not be performed on a HOST nickname."
#define ERR_MUST_BETO_LINK	"\2%s\2 must be performed on a LINKED nickname."
#define ERR_CANT_BETO_LINK	"\2%s\2 may not be performed on a LINKED nickname."
#define ERR_NOT_ON_YOURSELF	"\2%s\2 may not be performed on yourself."
#define ERR_NOT_ON_IRCOP	"\2%s\2 may not be performed on an IRC Operator."
#define ERR_ONLY_ON_IRCOP	"\2%s\2 may only be performed on an IRC Operator."
#define ERR_MUST_BE_IRCOP	"You must have the IRC Operator flag set to \2%s\2."


/****************************************************************************
 *********************************** MISC ***********************************
 ****************************************************************************/

#define INFO_SYNC_TIME		"Databases will sync in %s."
#define INFO_LIST_MATCH		"List of entries matching \2%s\2:"
#define INFO_END_OF_LIST	"End of list - %d/%d matches shown."
#define INFO_EMAIL		"E-Mail"
#define INFO_URL		"WWW Page (URL)"
#define INFO_FOUNDER		"Founder"
#define INFO_DESC		"Description"
#define INFO_MLOCK		"Mode Lock"
#define INFO_JOIN		"ChanServ Join"

/* Different lists maintinance */
#define LIST_THERE		"\2%s\2 is already present (or inclusive) on %s %s list."
#define LIST_NOT_THERE		"\2%s\2 not found on %s %s list."
#define LIST_NOT_FOUND		"No such entry \2#%d\2 on %s %s list."
#define LIST_ADDED		"\2%s\2 has been added to %s %s list."
#define LIST_ADDED_AT		"\2%s\2 has been added to %s %s at \2%s\2."
#define LIST_REMOVED		"\2%s\2 has been removed from %s %s list."
#define LIST_REMOVED_NUM	"Entry \2#%d\2 has been removed from %s %s list."
#define LIST_REMOVED_MASK	"%d entr%s matching \2%s\2 removed from %s %s list."
#define LIST_REMOVED_ALL	"All entries on %s %s list removed."
#define LIST_UNCHANGED		"\2%s\2 unchanged on %s %s list at \2%s\2."
#define LIST_CHANGED		"\2%s\2 changed on %s %s list to \2%s\2."
#define LIST_LIMIT		"Sorry, you may only have %d entries on %s %s list."

/* Output in multi files */
#define MULTI_GETPASS		"Password for %s (%s) is \2%s\2."
#define MULTI_GETPASS_WALLOP	"\2%s\2 used GETPASS on \2%s\2 (%s)."
#define MULTI_FORBID		"\2%s\2 has been FORBIDDEN."
#define MULTI_SUSPEND		"\2%s\2 (%s) has been SUSPENDED."
#define MULTI_UNSUSPEND		"\2%s\2 (%s) has been UNSUSPENDED."

/* process.c stuff */
#define FLOODING		"%s is FLOODING services (placed on %s ignore)."
#define TEMP_FLOOD		"Services FLOOD triggered (>%d messages in %d seconds).  You are being ignored for %d seconds."
#define PERM_FLOOD		"Services FLOOD triggered >%d times in one connection.  You are now on perminant ignore."
#define IS_IGNORED		"You have triggered PERM ignore on a previous connection.  You will not be answered by services."
#define SERVICES_OFF_REASON	"Services are currently \2OFF\2 (%s)."
#define SERVICES_OFF		"Services are currently \2OFF\2."
#define SERVICES_ON		"Services have now been switched back \2ON\2 - Please use them at will."
#define ONOFF_NOTIFY		"Services switched \2%s\2 by request of \2%s\2."


/****************************************************************************
 ********************************* NickServ *********************************
 ****************************************************************************/

/* INFO displays */
#define NS_INFO_INTRO		"%s is %s"
#define NS_INFO_HOST		"        Host Nick: \2%s\2"
#define NS_INFO_EMAIL		"   E-Mail Address: %s"
#define NS_INFO_URL		"   WWW Page (URL): %s"
#define NS_INFO_SUSPENDED	"    Suspended For: %s"
#define NS_INFO_USERMASK	"Last Seen Address: %s"
#define NS_INFO_REGISTERED	"       Registered: %s ago"
#define NS_INFO_ONLINE_AS	"        Online As: %s"
#define NS_INFO_AONLINE_AS	"   Also Online As: %s"
#define NS_INFO_LAST_SEEN	"        Last Seen: %s ago"
#define NS_INFO_LAST_ONLINE	"      Last Online: %s ago"
#define NS_INFO_OPTIONS		"          Options: %s"
#define NS_INFO_COUNT		"%d nicknames currently registered."
#define NS_INFO_ONLINE		"This user is currently online, type \2/whois %s\2 for more information."

/* FLAG names */
#define NS_FLAG_SUSPENDED	"\2SUSPENDED USER\2"
#define NS_FLAG_KILLPROTECT	"Kill Protection"
#define NS_FLAG_SECURE		"Security"
#define NS_FLAG_PRIVATE		"Private"
#define NS_FLAG_IRCOP		"IRC Operator"
#define NS_FLAG_SOP		"Services Operator"
#define NS_FLAG_NONE		"None"

/* General returns */
#define NS_REGISTERED		"Your nickname is now registered under host \2%s\2."
#define NS_LINKED		"Your nickname has now been linked to \2%s\2."
#define NS_DROPPED		"Nickname %s%s%s%s has been dropped."
#define NS_CHANGE_PASSWORD	"Your password is now \2%s\2 - Please remember this for later use."
#define NS_KILLED_IMPOSTER	"User claiming your nickname has been killed."
#define NS_RELEASE		"Type \2/MSG %s RELEASE %s \37password\37 to use it before the one-minute timeout."
#define NS_FORCED_CHANGE	"Your nickname has been forcibly changed."
#define NS_FORCED_KILL		"Nickname protection enforced"
#define NS_FAILMAX_KILL		"Repeated password failure"
#define NS_GHOST_KILL		"Removing GHOST user"
#define NS_IDENTIFIED		"Password accepted - you are now recognized."
#define NS_FORBID_WALLOP	"\2%s\2 used FORBID on nickname \2%s\2 (%d slave nicks dropped)."

/* Ownership and status */
#define NS_NOT_YOURS		"Nickname %s does not belong to you."
#define NS_IN_USE		"Nickname %s is currently in use."
#define NS_NOT_IN_USE		"Nickname %s is not currently in use."
#define NS_YOU_NO_SLAVES	"You have no slave nicks."
#define NS_NO_SLAVES		"Nickname %s has no slave nicks."
#define NS_YOU_NOT_REGISTERED	"Your nickname is not registered."
#define NS_NOT_REGISTERED	"Nickname %s is not registered."
#define NS_TAKEN		"Nickname %s is already registered."
#define NS_CANNOT_REGISTER	"Nickname %s may not be registered."
#define NS_CANNOT_LINK		"Nickname %s may not be linked."
#define NS_AM_IGNORED		"%s is ignoring your memos."
#define NS_IS_SUSPENDED_MEMO	"%s is SUSPENDED, therefore may not retrieve memos."
#define NS_IS_SUSPENDED		"Nickname %s is SUSPENDED."
#define NS_IS_NOT_SUSPENDED	"Nickname %s is not SUSPENDED."


/****************************************************************************
 ********************************* ChanServ *********************************
 ****************************************************************************/

/* INFO Displays */
#define CS_INFO_INTRO		"Information on channel %s"
#define CS_INFO_FOUNDER		"       Founder: %s%s%s%s"
#define CS_INFO_DESC		"   Description: %s"
#define CS_INFO_URL		"WWW Page (URL): %s"
#define CS_INFO_REG_TIME	"    Registered: %s ago"
#define CS_INFO_LAST_USED	"     Last Used: %s ago"
#define CS_INFO_SUSPENDED	" Suspended For: %s"
#define CS_INFO_SUSPENDER	"  Suspended By: %s"
#define CS_INFO_TOPIC		"    Last Topic: %s"
#define CS_INFO_TOPIC_SET	"  Topic Set By: %s"
#define CS_INFO_REVENGE		" Revenge Level: %s"
#define CS_INFO_CHAN_STAT	" Channel Stats: %d user%s, %d voice%s, %d op%s."
#define CS_INFO_OPTIONS		"       Options: %s"
#define CS_INFO_MLOCK		"     Mode Lock: %s"
#define CS_INFO_COUNT		"%d channels currently registered."

/* FLAG Names */
#define CS_FLAG_SUSPENDED	"\2SUSPENDED CHANNEL\2"
#define CS_FLAG_PRIVATE		"Private"
#define CS_FLAG_KEEPTOPIC	"Topic Retention"
#define CS_FLAG_TOPICLOCK	"Topic Lock"
#define CS_FLAG_SECUREOPS	"Secure Ops"
#define CS_FLAG_SECURE		"Secure"
#define CS_FLAG_RESTRICTED	"Restricted"
#define CS_FLAG_NONE		"None"

/* Bitchy Stuff */
#define CS_REV_LEVEL		"Revenge level for %s currently at \2%s\2 (%s)."
#define CS_REV_SET		"Revenge level for %s set to \2%s\2 (%s)."
#define CS_REV_DEOP		"\2REVENGE\2 - Do not deop %s."
#define CS_REV_KICK		"\2REVENGE\2 - Do not kick %s."
#define CS_REV_BAN		"\2REVENGE\2 - Do not ban %s."
#define CS_SUSPENDED_TOPIC	"[\2SUSPENDED\2] %s [\2SUSPENDED\2]"
#define CS_FORBID_WALLOP	"\2%s\2 used FORBID on channel \2%s\2."

/* Levels and Access */
#define CS_LEVEL_YOU		"You can %s."
#define CS_LEVEL_LIST		"Level required for commands on %s:"
#define CS_LEVEL_LOW		"Cannot change levels below %d."
#define CS_LEVEL_HIGH		"Cannot change levels above %d."
#define CS_LEVEL_CHANGE		"Level for %s on %s changed to \2%d\2."
#define CS_LEVEL_NO_CHANGE	"Level for %s on %s unchanged at \2%d\2."
#define CS_LEVEL_RESET		"All levels for %s reset."
#define CS_LEVEL_NONE		"Unknown level type \2%s\2."
#define CS_ACCESS_ZERO		"Access levels must be non-zero."
#define CS_ACCESS_LOW		"Cannot add or change to access levels below %d."
#define CS_ACCESS_HIGH		"Cannot add or change to access levels above %d."
#define CS_ACCESS_HIGHER	"Sorry, %s is higher or equal on access list."
#define CS_ACCESS_HIGHER_MATCH	"Sorry, %s matches %s (higher or equal on access list)."

/* General Outputs */
#define CS_REGISTERED		"Channel %s has been registered under your nickname \2%s\2."
#define CS_DROPPED		"Channel %s has been dropped."
#define CS_CHANGE_PASSWORD	"Channel %s password is now \2%s\2 - Please remember this for later use."
#define CS_IDENTIFIED		"Password accepted - you now have founder access to %s."
#define CS_YOU_UNBANNED		"You have been unbanned from %s."
#define CS_UNBANNED		"%s has been unbanned from %s."
#define CS_CLEARED		"\2%s\2 have been cleared from channel %s."
#define CS_CLEAR_KICK		"CLEAR USERS command from %s"

/* Ownerships */
#define CS_ERR_SUSPENDED	"Channel %s is currently SUSPENDED, no ops/voices will be allowed."
#define CS_ERR_REGISTERED	"Channel %s is a registered channel, access privilages apply."
#define CS_IN_USE		"Channel %s is currently in use."
#define CS_NOT_IN_USE		"Channel %s is not currently in use."
#define CS_NOT_REGISTERED	"Channel %s is not registered."
#define CS_TAKEN		"Channel %s is already registered."
#define CS_CANNOT_REGISTER	"Channel %s may not be registered."
#define CS_FORBIDDEN		"Channel %s may not be used."
#define CS_GET_OUT		"You are not permitted in this channel"
#define CS_IS_SUSPENDED		"Channel %s is SUSPENDED."
#define CS_IS_NOT_SUSPENDED	"Channel %s is not SUSPENDED."
#define CS_YOU_NOT_IN_CHAN	"You are not in channel %s."
#define CS_YOU_IN_CHAN		"You are already in channel %s."
#define CS_NOT_IN_CHAN		"%s is not in channel %s."
#define CS_IN_CHAN		"%s is already in channel %s."
#define CS_YOU_NOT_GOT		"You are not %s in channel %s."
#define CS_NOT_GOT		"%s is not %s in channel %s."
#define CS_YOU_ALREADY_GOT	"You are already %s in channel %s."
#define CS_ALREADY_GOT		"%s is already %s in channel %s."


/****************************************************************************
 ********************************* MemoServ *********************************
 ****************************************************************************/

#define MS_IS_BACKUP		"Sorry, backup services currently in use, MEMOS and NEWS are disabled."

#define MS_YOU_DONT_HAVE	"You have no memos."
#define NS_YOU_DONT_HAVE	"There are no news articles for %s."
#define MS_YOU_HAVE		"You have %d memo%s."
#define NS_YOU_HAVE		"There %s %d news article%s for %s."
#define MS_DOESNT_EXIST		"Memo %d does not exist!"
#define NS_DOESNT_EXIST		"News article %d does not exist for %s!"
#define NS_MAY_NOT		"You may not %s a news article."

#define MS_LIST			"To read, type \2/msg %s READ \37num\37."
#define NS_LIST			"To read, type \2/msg %s READ %s \37num\37."
#define MS_NEW			"You have a new %smemo (#%d) from %s."
#define MS_READ_NEW		"Type \2/msg %s READ %d\2 to read it."
#define NS_NEW			"There is a new news article (#%d) for %s from %s."
#define NS_READ_NEW		"Type \2/msg %s READ %s %d\2 to read it."
#define MS_MEMO			"Memo %d from %s (sent %s ago)."
#define NS_MEMO			"News article %d for %s from %s (sent %s ago)"
#define MS_TODEL		"To delete, type \2/msg %s DEL %d\2."
#define MS_TODEL_ALL		"To delete, type \2/msg %s DEL ALL\2."

#define MS_SEND			"Memo sent to %s (%s)."
#define NS_SEND			"News article posted for %s."
#define MS_MASS_SEND		"Memo sent to all %s."
#define MS_DELETE		"Memo %d has been deleted."
#define NS_DELETE		"News article %d for %s has been deleted."
#define MS_DELETE_ALL		"All of your memos have been deleted."
#define NS_DELETE_ALL		"All news articles for %s have been deleted."

/****************************************************************************
 ********************************* OperServ *********************************
 ****************************************************************************/


#define OS_GLOBAL_WALLOP	"\2%s\2 just sent a message to ALL users."

#define OS_SET_EXP		"Expiries (days):"
#define OS_SET_EXP_CHAN		"    Channels : \2%d\2"
#define OS_SET_EXP_NICK		"    Nicknames: \2%d\2"
#define OS_SET_EXP_NEWS		"    News     : \2%d\2"
#define OS_SET_EXP_AKILL	"    AutoKills: \2%d\2"
#define OS_SET_SOPS		"Maximum of \2%d\2 Services Operators (%d currently)."
#define OS_SET_CLONES		"Each user may have \2%d\2 connections."
#define OS_SET_RELEASE		"Nicknames held for \2%d\2 seconds."
#define OS_SET_AKICKS		"Maximum of \2%d\2 AKICK's per channel."
#define OS_SET_FLOOD		"Flood triggered on \2%d\2 messages in \2%d\2 seconds."
#define OS_SET_IGNORE		"Ignore lasts for \2%d\2 seconds, \2%d\2 ignores makes it perminant."
#define OS_SET_RELINK		"Services relink in \2%d\2 seconds upon SQUIT."
#define OS_SET_OVERRIDE		"ChanServ Override is \2%s\2"
#define OS_SET_UPDATE		"Databases saved every \2%d\2 seconds."
#define OS_SET_ADMINS		"Services Admins: \2%s\2"

#define OS_QLINE		"\2%s\2 activated QUARENTINE on nick \2%s\2."
#define OS_UNQLINE		"\2%s\2 removed QUARENTINE on nick \2%s\2."
#define OS_SVSNOOP_ON		"\2%s\2 activated OPER QUARENTINE on \2%s\2."
#define OS_SVSNOOP_OFF		"\2%s\2 removed OPER QUARENTINE on \2%s\2."
#define OS_JUPE			"JUPING \2%s\2 by request of \2%s\2."
#define OS_UPDATE		"Updating databases."
#define OS_NEW_MESSAGE		"\2%s\2 created a new %s message."
#define OS_AKILL_ADDED		"\2%s\2 has been added to services %sAKILL list (%d users killed)."
#define OS_AKILL_NOT_THERE	"\2%s\2 not found on services AKILL list or is PERMINANT."
#define OS_AKILL_EXPIRE		"Expring AKILL on %s (%s) by %s."
#define OS_AKILL_BANNED		"You are banned (%s)"
#define OS_CLONE_HIGH		"CLONE AMOUNT must be less than %d."
#define OS_CLONE_LOW		"CLONE AMOUNT must be greater than %d."

#endif /* MESSAGES_H */
