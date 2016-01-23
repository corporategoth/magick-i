/* ChanServ special help texts.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifdef CHANSERV

/*************************************************************************/

static const char *oper_drop_help[] = {
"Syntax: \2DROP \37channel\37\2",
"",
"Unregisters the named channel.  Only \2Services admins\2",
"can drop a channel for which they have not identified.",
NULL
};

/*************************************************************************/

static const char *getpass_help[] = {
"Syntax: \2GETPASS \37channel\37\2",
"",
"Returns the password for the given channel.  \2Note that\2",
"\2whenever this command is used, a message including the\2",
"\2person who issued the command and the channel it was used\2",
"\2on will be logged and sent out as a GLOBOPS.\2",
"Limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *forbid_help[] = {
"Syntax: \2FORBID \37channel\37\2",
"",
"Disallows anyone from registering or using the given",
"channel.  May be cancelled by dropping the channel.",
"Limited to \2Services Admin\2.",
NULL
};
/*************************************************************************/

static const char *suspend_help[] = {
"Syntax: \2SUSPEND \37channel reason\37\2",
"",
"Makes ACCESS and AKICK lists (and FOUNDER) meaningless.",
"SOP's will get autoops and founder, everyone else",
"(regardless of previous level on access list, even if",
"they were founder) will be just a 0-level user.  AKICK",
"list is disabled, and INVITE is publicly accessable.",
"By its nature, MEMOs are only postable by SOPS.",
"This should be used for BRIEF periods of time when a",
"channel needs repremanding,  but not quite a DROP.",
"Limited to \2Services Admin\2.",
NULL
};
/*************************************************************************/

static const char *unsuspend_help[] = {
"Syntax: \2UNSUSPEND \37channel\37\2",
"",
"This reverses the SUSPEND command, giving back full control",
"to the previos founder/access list.  Everything previously",
"imposed by SUSPEND will be reversed.  One drawback however",
"is that TOPICLOCK will be set OFF and KEEPTOPIC on."
"Limited to \2Services Admin\2.",
NULL
};
/*************************************************************************/

#endif /* CHANSERV */
