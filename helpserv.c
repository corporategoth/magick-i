/* HelpServ functions.

 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include <sys/stat.h>

char s_HelpServ[NICKMAX];
char s_IrcIIHelp[NICKMAX];
char helpserv_dir[512];

static void do_help (const char *whoami, const char *source, char *topic);

/*************************************************************************/

/* helpserv:  Main HelpServ routine.  `whoami' is what nick we should send
 * messages as: this won't necessarily be s_HelpServ, because other
 * routines call this one to display help files. */

void
helpserv (const char *whoami, const char *source, char *buf)
{
    char *cmd, *topic, *s;

    topic = buf ? sstrdup (buf) : NULL;
    cmd = strtok (buf, " ");
    if(helpserv_on==TRUE)
    {
	if (cmd && stricmp (cmd, "\1PING") == 0)
	{
	    if (!(s = strtok (NULL, "")))
		s = "\1";
	    notice (s_HelpServ, source, "\1PING %s", s);
	}
	else
	    do_help (whoami, source, topic);
    }
    else
	do_help (whoami, source, topic);
    if (topic)
	free (topic);
}

/*************************************************************************/
/*********************** HelpServ command routines ***********************/
/*************************************************************************/

/* Return a help message. */

static void
do_help (const char *whoami, const char *source, char *topic)
{
    FILE *f;
    struct stat st;
    char buf[256], *ptr, *s;
    char *old_topic;		/* an unclobbered (by strtok) copy */

    if (!topic || !*topic)
	topic = "help";
    old_topic = sstrdup (topic);

    if (stricmp (topic, "CREDITS") == 0)
    {
	notice (whoami, source, "Magick is written by \2Preston A. Elder\2 \37<\37\37\2prez\2\37\37@\37\37\2magick.tm\2\37\37>\37.");
	notice (whoami, source, " ");
	notice (whoami, source, "Magick is dedicated to the memory of my my beloved");
	notice (whoami, source, "sister, Stacey Louise Elder (Jan 1975 - Feb 1998).");
	notice (whoami, source, " ");
	notice (whoami, source, "The Magick home page is found at:");
	notice (whoami, source, "       \2HTTP://www.magick.tm\2");
	notice (whoami, source, " ");
	notice (whoami, source, "Magick is available freely from:");
	notice (whoami, source, "       \2FTP://ftp.magick.tm/pub/Magick\2");
	notice (whoami, source, "    or \2HTTP://www.magick.tm/download\2");
	notice (whoami, source, " ");
	notice (whoami, source, "Please type \2/msg %s CONTRIBUTORS\2 for full credits (flood).", s_HelpServ);
    }
    else if (stricmp (topic, "CONTRIBUTORS") == 0)
    {
	notice (whoami, source, "Programmers:");
	notice (whoami, source, "    \2PreZ\2  <prez@magick.tm>");
	notice (whoami, source, "        - Main Magick source + Head of development.");
	notice (whoami, source, "    \2Ungod\2 <ungod@magick.tm>");
	notice (whoami, source, "        - Win32 Development (and ongoing maintinance of it).");
	notice (whoami, source, " ");
	notice (whoami, source, "Thanks to (from PreZ):");
	notice (whoami, source, "    \37Coca-Cola       \37 - Life Support *bleep, bleep*.");
	notice (whoami, source, "    \37Lord Striker    \37 - for endless dumb looks.");
	notice (whoami, source, "    \37Unilynx         \37 - for giving me competition and inspiration.");
	notice (whoami, source, "    \37Zephyr          \37 - for giving me the best crash dummy I could ask for!");
	notice (whoami, source, "    \37Tschaicovski    \37 - If you dont understand, you never will.");
	notice (whoami, source, "    \37Girls           \37 - For snobbing me, thus making me lifeless nuff to do this.");
	notice (whoami, source, "Thanks to (from Ungod):");
	notice (whoami, source, "    \37PreZ		\37 - for not being a lamer and asking intelligent questions (albeit sometimes impossible ones).");
	notice (whoami, source, "    \37Mloe		\37 - for not being a lamer and asking intelligent questions and having the faith to believe my answers (even when i do lose my mind and go off into some other plane of existence)");
	notice (whoami, source, "    \37Nescafe Blend 43\37 - Life Support *bleep, bleep*.");
	notice (whoami, source, "    \37Lord Striker    \37 - for the same endless dumb looks as above :) (he's an expert)");
	notice (whoami, source, "    \37Girls           \37 - see above");
	notice (whoami, source, " ");
	notice (whoami, source, "Also thanks to anyone who reported bugs, and offered suggestions that put Magick on top.");
    }
    else
    {
	/* As we copy path parts, (1) lowercase everything and (2) make sure
	 * we don't let any special characters through -- this includes '.'
	 * (which could get parent dir) or '/' (which couldn't _really_ do
	 * anything if we keep '.' out, but better to be on the safe side).
	 * Special characters turn into '_'.
	 */
	strscpy (buf, helpserv_dir, sizeof (buf));
	ptr = buf + strlen (buf);
	for (s = strtok (topic, " "); s && ptr - buf < sizeof (buf) - 1;
	     s = strtok (NULL, " "))
	{
	    *ptr++ = '/';
	    while (*s && ptr - buf < sizeof (buf) - 1)
	    {
		if (*s == '.' || *s == '/')
		    *ptr++ = '_';
		else
		    *ptr++ = tolower (*s);
		++s;
	    }
	    *ptr = 0;
	}

	/* If we end up at a directory, go for an "index" file/dir if
	 * possible.
	 */
	while (ptr - buf < sizeof (buf) - 1
	       && stat (buf, &st) == 0 && S_ISDIR (st.st_mode))
	{
	    *ptr++ = '/';
	    strscpy (ptr, "index", sizeof (buf) - (ptr - buf));
	    ptr += strlen (ptr);
	}

	/* Send the file, if it exists.
	 */
	if (!(f = fopen (buf, "r")))
	{
	    notice (whoami, source, ERR_NOHELP, old_topic);
	    free (old_topic);
	    return;
	}
	while (fgets (buf, sizeof (buf), f))
	{
	    s = strtok (buf, "\n");
	    /* Use this odd construction to prevent any %'s in the text from
	     * doing weird stuff to the output.  Also replace blank lines by
	     * spaces (see send.c/notice_list() for an explanation of why).
	     */
	    notice (whoami, source, "%s", s ? s : " ");
	}
	fclose (f);
    }
    free (old_topic);
}
