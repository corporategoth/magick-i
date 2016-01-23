/* MemoServ special help texts.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

/************************************************************************/


static const char *opersend_help[] =
{
    "Syntax: OPERSEND \37text\37",
    "",
    "Will send a memo to all users with the IRC Operator",
    "flag set (see \2/msg NickServ HELP SET IRCOP\2).",
    "Apart from the lack of recipiant, this command works",
    "exactly the same as the SEND command.",
    NULL
};

static const char *sopsend_help[] =
{
    "Syntax: SOPSEND \37text\37",
    "",
    "Will send a memo to all users who are set as Services",
    "Operators (see \2/msg OperServ HELP SOP\2).",
    "Apart from the lack of recipiant, this command works",
    "exactly the same as the SEND command.",
    NULL
};
