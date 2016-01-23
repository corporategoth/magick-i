/* MemoServ special help texts.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

/************************************************************************/

#ifdef MEMOSERV

static const char *opersend_help[] = {
"Syntax: OPERSEND \37text\37",
"",
"Will send a memo to all users with the IRC Operator",
"flag set (see \2/msg NickServ HELP SET IRCOP\2).",
"Apart from the lack of recipiant, this command works",
"exactly the same as the SEND command.",
NULL
};

#endif /* MEMOSERV */
