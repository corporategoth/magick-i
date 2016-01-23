/* NickServ special help texts.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifdef NICKSERV

/*************************************************************************/

static const char *oper_drop_help[] = {
"Syntax: \2DROP [\37nickname\37]\2",
"",
"Without a parameter, drops your nickname from the",
"NickServ database.",
"",
"With a parameter, drops the named nick from the database.",
"This use limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *getpass_help[] = {
"Syntax: \2GETPASS \37nickname\37\2",
"",
"Returns the password for the given nickname.  \2Note\2 that",
"whenever this command is used, a message including the",
"person who issued the command and the nickname it was used",
"on will be logged and sent out as a GLOBOPS.",
"Limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *forbid_help[] = {
"Syntax: \2FORBID \37nickname\37\2",
"",
"Disallows a nickname from being registered or used by",
"anyone.  May be cancelled by dropping the nick.",
"Limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *suspend_help[] = {
"Syntax: \2SUSPEND \37nickname reason\37\2",
"",
"This stops a user from being able to IDENTIFY to nickserv",
"By implecation, this also forbids reading MEMOS, DROPing",
"changing their ACCESS list, changing any of their SET's,",
"or getting ops on a SECUREOPS channel."
"This should be used for BRIEF periods of time when a",
"user needs repremanding,  but not quite a DROP.",
"Limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *unsuspend_help[] = {
"Syntax: \2UNSUSPEND \37nickname reason\37\2",
"",
"This reverses the SUSPEND command, giving back full control",
"to the user.  Everything previously imposed by SUSPEND will",
" be reversed.  One drawback however is that their last-seen",
"hostmask will be lost - this is regained next signon tho."
"Limited to \2Services Admin\2.",
NULL
};

/*************************************************************************/

static const char *set_ircop_help[] = {
"Syntax: \2SET IRCOP {ON|OFF}\2",
"",
"Disallows your nickname from being expired, and adds",
"\37IRC Operator\37 to your \2INFO\2 display..",
"Limited to \2IRC Operators\2.",
NULL
};

/*************************************************************************/

static const char *listoper_help[] = {
"Syntax: \2SET LISTOPER \37pattern\37\2",
"",
"Lists nicknames that have the IRC Operator flag set.",
"Works exactly as the \2/MSG NickServ LIST\2 command",
"does."
"Limited to \2IRC Operators\2.",
NULL
};

/*************************************************************************/

static const char *deop_help[] = {
"Syntax: \2SET DEOP \37nick\37\2",
"",
"Takes away the \37IRC Operator\37 flag from someone",
"who is no longer an IRC Operator",
"Limited to \2IRC Operators\2.",
NULL
};

/*************************************************************************/

#endif /* NICKSERV */
