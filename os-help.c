/* Help text for OperServ */

#ifdef DAL_SERV
static const char *os_help[] =
{
    "OperServ commands (IRC Operators):",
    "   \2MODE\2 {\37channel\37|\37nick\37} [\37modes\37] -- See/Change a channel/nick's modes",
    "   \2KICK\2 \37channel\37 \37nick\37 \37reason\37 -- Kick a user from a channel",
    "   \2AKILL\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37mask\37 [\37reason\37]] -- Manipulate the AKILL list",
    "   \2GLOBAL\2 \37message\37 -- Send a message to all users",
    "   \2STATS\2 -- status of Magick and network",
    "   \2SETTINGS\2 -- Show hardcoded Magick settings",
    "   \2BREAKDOWN\2 -- Show percentages of users per server",
    "   \2USERLIST\2 [\37pattern\37] -- Send list of users and their modes",
"   \2MASKLIST\2 [\37pattern\37] -- Send list of users (found by user@host)",
    "   \2CHANLIST\2 [\37pattern\37] -- Send list of channels and their modes/occupants",
    "   \2QLINE\2 \37nick\37 [\37reason\37] -- Quarentine a nick (disable its use)",
    "   \2UNQLINE\2 \37nick\37 -- Remove nick quarentine",
    "",
    NULL
};
#else
static const char *os_help[] =
{
    "OperServ commands (IRC Operators):",
    "   \2MODE\2 {\37channel\37|\37nick\37} [\37modes\37] -- See/Change a channel/nick's modes",
    "   \2KICK\2 \37channel\37 \37nick\37 \37reason\37 -- Kick a user from a channel",
    "   \2AKILL\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37mask\37 [\37reason\37]] -- Manipulate the AKILL list",
    "   \2GLOBAL\2 \37message\37 -- Send a message to all users",
    "   \2STATS\2 -- status of Magick and network",
    "   \2SETTINGS\2 -- Show hardcoded Magick settings",
    "   \2BREAKDOWN\2 -- Show percentages of users per server",
    "   \2USERLIST\2 [\37pattern\37] -- Send list of users and their modes",
"   \2MASKLIST\2 [\37pattern\37] -- Send list of users (found by user@host)",
    "   \2CHANLIST\2 [\37pattern\37] -- Send list of channels and their modes/occupants",
    "",
    NULL
};
#endif
#ifdef DAL_SERV
static const char *os_sop_help[] =
{
    "OperServ commands (Services Operators):",
    "   \2KILL\2 \37user\37 \37reason\37 -- Kill user with no indication of IrcOP",
    "   \2PAKILL\2 {\37ADD\37|\37DEL\37} [\37mask\37 [\37reason\37]] -- Manipulate the PAKILL list",
    "   \2CLONE\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37host\37] [37amount\37 [\37reason\37]] -- Manipulate the CLONE list",
    "   \2IGNORE\2 {\37ADD\37|\37DEL\37|\37LIST\37} -- Server auto-ignores.",
    "   \2JUPE\2 \37server\37 -- Make server appear linked",
    "   \2UPDATE\2 -- Update *Serv databases (before QUIT)",
    "   \2LOGONMSG\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37text|num\37] -- Manipulate the Logon Messages",
    "",
    NULL
};
#else
static const char *os_sop_help[] =
{
    "OperServ commands (Services Operators):",
    "   \2PAKILL\2 {\37ADD\37|\37DEL\37} [\37mask\37 [\37reason\37]] -- Manipulate the PAKILL list",
    "   \2CLONE\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37host\37] [37amount\37 [\37reason\37]] -- Manipulate the CLONE list",
    "   \2IGNORE\2 {\37ADD\37|\37DEL\37|\37LIST\37} -- Server auto-ignores.",
    "   \2JUPE\2 \37server\37 -- Make server appear linked",
    "   \2UPDATE\2 -- Update *Serv databases (before QUIT)",
    "   \2LOGONMSG\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37text|num\37] -- Manipulate the Logon Messages",
    "",
    NULL
};
#endif
#ifdef DAL_SERV
static const char *os_admin_help[] =
{
    "OperServ commands (Services Admins):",
    "   \2NOOP\2 \37server\37 {\37+\37|\37-\37} -- Restrict server's Operaters to local",
    "   \2SOP\2 -- Manipulate the Services Operator list",
    "   \2OPERMSG\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37text|num\37] -- Manipulate the Oper Messages",
    "   \2RELOAD\2 -- Reload databases/config immediately",
    "   \2QUIT\2 -- Terminate Magick without database save",
    "   \2SHUTDOWN\2 -- Same as UPDATE+QUIT",
    "   \2OFF\2/\2ON\2 [\37reason\37] -- Deactivate services without terminating",
    "",
    NULL
};
#else
static const char *os_admin_help[] =
{
    "OperServ commands (Services Admins):",
    "   \2SOP\2 -- Manipulate the Services Operator list",
    "   \2OPERMSG\2 {\37ADD\37|\37DEL\37|\37LIST\37} [\37text|num\37] -- Manipulate the Oper Messages",
    "   \2RELOAD\2 -- Reload databases/config immediately",
    "   \2QUIT\2 -- Terminate Magick without database save",
    "   \2SHUTDOWN\2 -- Same as UPDATE+QUIT",
    "   \2OFF\2/\2ON\2 [\37reason\37] -- Deactivate services without terminating",
    "",
    NULL
};
#endif
static const char *os_end_help[] =
{
    "   \2CHANSERV\2, \2NICKSERV\2, and \2MEMOSERV\2 will also tell you",
    "   the available Operator commands for the respective serviers.",
    "   Help for these commands is done with /msg *Serv HELP command",
    "",
    "\2Notice:\2 All commands sent to OperServ are logged!",
    NULL
};

/*************************************************************************/

#ifdef DAL_SERV
static const char *mode_help[] =
{
    "Syntax: \2MODE\2 {\37channel\37|\37nick\37} [\37modes\37]",
    "",
    "Allows IRCops to see channel modes for any channel",
    "or nick, and set them for any channel.",
    "Service Operators may also set a nick's modes.",
    "Parameters are the same as for the standard /MODE",
    "command.",
    NULL
};
#else
static const char *mode_help[] =
{
    "Syntax: \2MODE\2 {\37channel\37|\37nick\37} [\37modes\37]",
    "",
    "Allows IRCops to see channel modes for any channel",
    "or nick, and set them for any channel.",
    "Parameters are the same as for the standard /MODE",
    "command.",
    NULL
};
#endif
/*************************************************************************/

static const char *kick_help[] =
{
    "Syntax: \2KICK\2 \37channel\37 \37user\37 \37reason\37",
    " ",
    "Allows IRCops to kick a user from any channel.",
    "Parameters are the same as for the standard /KICK",
    "command.  The kick message will have the nickname of the",
    "IRCop sending the KICK command prepended; for example:",
    "",
    "*** SpamMan has been kicked off channel #my_channel by OperServ (PreZ (Flood))",
    NULL
};

/*************************************************************************/

static const char *sop_help[] =
{
    "Syntax: \2SOP\2 ADD \37nick\37",
    "        \2SOP\2 DEL \37nick\37",
    "        \2SOP\2 LIST [\37nick\37]",
    "",
    "Allows Operators to manipulate the SOP list.  Any oper who",
    "is on this list will be given special privilages such as",
    "the ability to GETPASS, etc.  Any function marked as a",
    "Services Operator function is available to them",
    " ",
    "SOP ADD adds the given nick to the SOP list.",
    "SOP DEL removes the given nick from the SOP list if it",
    "is present.  SOP LIST shows all current SOPs; if the",
    "optional nick is given, the list is limited to those",
    "SOPs matching the nick (can include wildcards).",
    "Limited to \2Services Admin\2.",
    NULL
};

/*************************************************************************/

static const char *logonmsg_help[] =
{
    "Syntax: \2LOGONMSG\2 ADD \37text\37",
    "        \2LOGONMSG\2 DEL \37num\37",
    "        \2LOGONMSG\2 LIST",
    "",
    "Allows Operators to manipulate the Logon Messages.  These",
    "messages are displayed to every user when they logon to",
    "IRC, or when they set back (unaway).",
    " ",
    "LOGONMSG ADD adds the given text to the Logon Messages.",
    "LOGONMSG DEL removes the numbered message from list if it",
    "is present.  LOGONMSG LIST shows all current Logon Messages,"
    "as well as who set them and when they were set.",
    "Limited to \2Services Operators\2.",
    NULL
};

static const char *opermsg_help[] =
{
    "Syntax: \2OPERMSG\2 ADD \37text\37",
    "        \2OPERMSG\2 DEL \37num\37",
    "        \2OPERMSG\2 LIST",
    "",
    "Allows Operators to manipulate the Oper Messages.  These",
    "messages are displayed to a user when they set mode +o",
    "(/oper), or when they set back (unaway) and are oper'd.",
    " ",
    "OPERMSG ADD adds the given text to the Oper Messages.",
    "OPERMSG DEL removes the numbered message from list if it",
    "is present.  OPERMSG LIST shows all current Oper Messages,"
    "as well as who set them and when they were set.",
    "Limited to \2Services Admin\2.",
    NULL
};

/*************************************************************************/

static const char *akill_help[] =
{
    "Syntax: \2AKILL\2 ADD \37mask\37 \37reason\37",
    "        \2AKILL\2 DEL \37mask\37",
    "        \2AKILL\2 LIST [\37mask\37]",
    "",
    "Allows IRCops to manipulate the AKILL list.  If a user",
    "matching an AKILL mask attempts to connect, Services will",
    "issue a KILL for that user.  AKILL's expire after 7 days.",
    " ",
    "AKILL ADD adds the given user@host mask to the AKILL",
    "list for the given reason (which \2must\2 be given).",
    "Only services admins may give a @* mask.",
    "AKILL DEL removes the given mask from the AKILL list if it",
    "is present.  AKILL LIST shows all current AKILLs; if the",
    "optional mask is given, the list is limited to those",
    "AKILLs matching the mask.",
    NULL
};

static const char *pakill_help[] =
{
    "Syntax: \2PAKILL\2 ADD \37mask\37 \37reason\37",
    "        \2PAKILL\2 DEL \37mask\37",
    "",
    "Allows IRCops to manipulate the PAKILL list.  If a user",
    "matching an PAKILL mask attempts to connect, Services will",
    "issue a KILL for that user. PAKILL's do not expire.",
    "See help on \37AKILL\37 for further information.",
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *clone_help[] =
{
    "Syntax: \2CLONE\2 ADD \37host\37 \37amount\37 \37reason\37",
    "        \2CLONE\2 DEL \37host\37",
    "        \2CLONE\2 LIST [\37pattern\37]",
    "",
    "If a user matching a CLONE host attempts to connect, will",
    "Services will grant them \37amount\37 connections at once,",
    "instead of the standard 2, if more than this are attempted",
    "hoowever, services will issue a KILL for that user.",
    " ",
    "CLONE ADD adds the given host to the CLONE list with",
    "the given connections and for list for the given reason",
    "(which \2must\2 be given).",
    "CLONE DEL removes the given host from the CLONE list if it",
    "is present.  CLONE LIST shows all current CLONEs; if the",
    "optional host is given, the list is limited to those",
    "CLONEs matching the host.",
    NULL
};

/*************************************************************************/

static const char *stats_help[] =
{
    "Syntax: \2STATS\2 [\37ALL\37]",
    "",
    "Shows the current number of users and IRCops online",
    "(excluding Services), the highest number of users online",
    "since Services was started, and the length of time",
    "Services has been running.",
    "",
    "The \2ALL\2 option is available only to Services admins, and",
    "displays information on Services' memory usage.  Using this",
    "option can freeze Services for a short period of time on",
    "large networks, so don't overuse it!",
    NULL
};

/*************************************************************************/

static const char *settings_help[] =
{
    "Syntax: \2SETTINGS\2",
    "",
    "Shows the current Service Operators, expiry times,",
    "limitations, etc etc hardcoded into Magick.",
    NULL
};

/*************************************************************************/

static const char *update_help[] =
{
    "Syntax: \2UPDATE\2",
    "",
    "Forces Magick to update the channel, nick, and memo",
    "databases as soon as you send the command.  Useful",
    "before shutdowns.",
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *reload_help[] =
{
    "Syntax: \2RELOAD\2",
    "",
    "Forces Magick to reload the channel, nick, and memo"
    "databases and configuration files as soon as you send",
    "the command.  Useful for configuration changes or",
    "database stuffups.  This WILL squit for <1 second.",
    "Limited to \2Services Admin\2.",
    NULL
};

/*************************************************************************/

static const char *sendpings_help[] =
{
    "Syntax: \2SENDPINGS\2",
    "",
    "Forces Magick to send a ping to ALL servers linked"
    "to update the lag meter in the \2BREAKDOWN\2.",
    "Limited to \2Services Operator\2.",
    NULL
};

/************************************************************************/

static const char *quit_help[] =
{
    "Syntax: \2QUIT\2",
    "",
    "Causes Magick to do an immediate shutdown.  It is",
    "a good idea to update the databases using the",
    "\2UPDATE\2 command beforehand.",
    "\2NOTE:\2 This command is NOT to be toyed with--using",
    "it without good reason can disrupt network operations,",
    "especially if a person with access to restart Services",
    "is not around.",
    "Limited to \2Services Admin\2.",
    NULL
};

/************************************************************************/

static const char *shutdown_help[] =
{
    "Syntax: \2SHUTDOWN\2",
    "",
    "Tells Magick to shut down, but save databases.  It is",
    "a \"clean\" alternative to \2QUIT\2.",
    "\2NOTE:\2 This command is NOT to be toyed with--using",
    "it without good reason can disrupt network operations,",
    "especially if a person with access to restart Services",
    "is not around.",
    "Limited to \2Services Admin\2.",
    NULL
};

/************************************************************************/

static const char *jupe_help[] =
{
    "Syntax: \2JUPE\2 \37server\37 [\37reason\37]",
    "",
    "Tells Services to jupiter a server -- that is, to create",
    "a fake \"server\" connected to Services which prevents",
    "the real server of that name from connecting.  The jupe",
    "may be removed using a standard \2SQUIT\2."
    "To be used \2only\2 in a situation where a server is",
    "disrupting the network and must be juped.",
    "If a reason is specified, the server description becomes",
    "\"JUPE: reason\", else its \"Jupitered Server\".",
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

#ifdef DAL_SERV
static const char *qline_help[] =
{
    "Syntax: \2QLINE\2 \37nick\37 [\37reason\37]",
    "",
    "Sends a nick QUARENTINE line to all servers that",
    "forbids anyone but IrcOP's from using the nick.",
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *unqline_help[] =
{
    "Syntax: \2UNQLINE\2 \37nick\37",
    "",
    "Removes a nick QUARENTINE set by QLINE, thus",
    "re-enabling the nick to be used by all users."
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *noop_help[] =
{
    "Syntax: \2NOOP\2 \37server\37 {\37+\37|\37-\37}",
    "",
    "Forces \2server\2's opers to all act as LOCAL OPERS",
    "essentially the equivilant of a server Q: line in the",
    "ircd.conf.  This should ONLY be used in EXTREME cases,",
    "as this will not only render the operators on the server"
    "powerless outside of it, it will also remove their access"
    "to OperServ and any other special operator functions."
    "\2+\2 will set this (ie. limit the server), and \2-\2 remove it."
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *kill_help[] =
{
    "Syntax: \2KILL\2 \37user\37 \37reason\37",
    "",
    "This kills a message with any message you wish, without",
    "the SignOff: user (Killed (IrcOP (Reason))) or even a",
    "\"You have been killed by IrcOP\" message on the users",
    "status screeen.  It is just as if the server has dumped",
    "the user for something, eg. Ping Timeout."
    "Limited to \2Services Operator\2.",
    NULL
};
#endif /* DAL_SERV */

/*************************************************************************/

static const char *breakdown_help[] =
{
    "Syntax: \2BREAKDOWN\2",
    "",
    "Shows a listing of servers, with the amount of users",
    "on each server, and the percentage of the network this",
    "tally equates.",
    NULL
};

/*************************************************************************/

static const char *userlist_help[] =
{
    "Syntax: \2USERLIST\2 [\37PATTERN\37]",
    "",
    "Sends a list of users, including what their current modes",
    "are, their server, logon time, hostmask, real name, and the",
    "channels they are in, and channels they are founder of.  If",
    "pattern is specified, it will only show those matching it.",
    "(Searches by nickname).",
    NULL
};

/*************************************************************************/

static const char *masklist_help[] =
{
    "Syntax: \2MASKLIST\2 [\37PATTERN\37]",
    "",
    "Sends a list of users, including what their current modes",
    "are, their server, logon time, hostmask, real name, and the",
    "channels they are in, and channels they are founder of.  If",
    "pattern is specified, it will only show those matching it.",
    "(Searches by user@host).",
    NULL
};

/*************************************************************************/

static const char *chanlist_help[] =
{
    "Syntax: \2CHANLIST\2 [\37PATTERN\37]",
    "",
    "Sends a list of channels, including what its modes are,",
    "and who its occupants are (and their status).  If mask",
    "is specified, it will only show those matching it.",
    NULL
};

/*************************************************************************/

static const char *offon_help[] =
{
    "Syntax: \2OFF\2 [\37reason\37]",
    "        \2ON\2",
    "",
    "Turns services OFF or ON without terminating them.",
    "When in OFF mode the only command services will",
    "accept is the ON command.  All other commands will",
    "get a message to the effect of services being off.",
    "Limited to \2Services Admin\2.",
    NULL
};

/*************************************************************************/

static const char *ignore_help[] =
{
    "Syntax: \2IGNORE\2 ADD \37nick\37",
    "        \2IGNORE\2 DEL \37nick\37",
    "        \2IGNORE\2 LIST [\37pattern\37]",
    "",
    "Maintinance on Services internal ignorance list.  If a",
    "user triggers Flood Protection more than a certain amount",
    "of times (seen with \2SETTINGS\2), it will put them on",
    "IGNORE list.  You may also add and remove people youserlf",
    "to and from this list.  Temporary ignores dont show up on",
    "this list.",
    "Limited to \2Services Operator\2.",
    NULL
};

/*************************************************************************/

static const char *chanserv_help[] =
{
    "ChanServ commands (Services Operators):",
    "   \2DROP\2 \37channel\37 - De-registeres a channel",
    "   \2GETPASS\2 \37channel\37 - Gets the founder password of channel",
    "   \2FORBID\2 \37channel\37 - Stops a channel from being used at all",
    "   \2SUSPEND\2 \37channel reason\37 - Immobilizes a channel",
    "   \2UNSUSPEND\2 \37channel\37 - Reverses previous command",
    NULL
};
static const char *nickserv_help[] =
{
    "NickServ commands (IRC Operators):",
    "   \2SET IRCOP\2 {\37ON\37|\37OFF\37} - Set IRC Operator flag",
    "   \2LISTOPER\2 \37pattern\37 - List users with IRC Operator flag",
    "",
    "NickServ commands (Services Operators):",
    "   \2DROP\2 \37nick\37 - De-registeres a nick",
    "   \2GETPASS\2 \37nick\37 - Gets the password of a nick",
    "   \2FORBID\2 \37nick\37 - Stops a nick from being used at all",
    "   \2SUSPEND\2 \37nick reason\37 - Immobilizes a user",
    "   \2UNSUSPEND\2 \37nick\37 - Reverses previous command",
    "   \2DEOPER\2 \37nick\37 - Removes IRC Operator flag",
    NULL
};
static const char *memoserv_help[] =
{
    "MemoServ commands (IRC Operators):",
    "   \2OPERSEND\2 \37text\37 - Send a memo to all IRC Operators",
    "",
    "MemoServ commands (Services Operators):",
    "   \2SOPSEND\2 \37text\37 - Send a memo to all Services Operators",
    NULL
};
