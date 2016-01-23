/* HelpServ functions.
 *
 * Magick is copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"
#include <sys/stat.h>

#ifdef HELPSERV
const char s_HelpServ[] = HELPSERV_NAME;
#endif
#ifdef IRCIIHELP
const char s_IrcIIHelp[] = IRCIIHELP_NAME;
#endif

static void do_help(const char *whoami, const char *source, char *topic);

/*************************************************************************/

/* helpserv:  Main HelpServ routine.  `whoami' is what nick we should send
 * messages as: this won't necessarily be s_HelpServ, because other
 * routines call this one to display help files. */

void helpserv(const char *whoami, const char *source, char *buf)
{
    char *cmd, *topic, *s;

    topic = buf ? sstrdup(buf) : NULL;
    cmd = strtok(buf, " ");
#ifdef HELPSERV
    if (cmd && stricmp(cmd, "\1PING")==0) {
	if (!(s = strtok(NULL, "")))
	    s = "\1";
	notice(s_HelpServ, source, "\1PING %s", s);
    } else
#endif
	do_help(whoami, source, topic);
    if (topic)
	free(topic);
}

/*************************************************************************/
/*********************** HelpServ command routines ***********************/
/*************************************************************************/

/* Return a help message. */

static void do_help(const char *whoami, const char *source, char *topic)
{
    FILE *f;
    struct stat st;
    char buf[256], *ptr, *s;
    char *old_topic;	/* an unclobbered (by strtok) copy */

    if (!topic || !*topic)
	topic = "help";
    old_topic = sstrdup(topic);

  if (stricmp(topic, "CREDITS")==0) {
	notice(whoami, source, "Magick is written by \2Preston A. Elder\2 \37<\37\37\2prez\2\37\37@\37\37\2antisocial.com\2\37\37>\37.");
	notice(whoami, source, "Yes, I decided to give myself a shameless plug.");
	notice(whoami, source, " ");
	notice(whoami, source, "Thanks to:");
	notice(whoami, source, "    \37Coca-Cola   \37 - Life Support *bleep, bleep*.");
	notice(whoami, source, "    \37Lord Striker\37 - for endless dumb looks.");
	notice(whoami, source, "    \37Unilynx     \37 - for giving me competition.");
	notice(whoami, source, "    \37Zephyr      \37 - for giving me the best");
	notice(whoami, source, "                   crash dummy I could ask for!");
	notice(whoami, source, "    \37Tschaicovski\37 - You wouldnt understand.");
	notice(whoami, source, "    \37Girls       \37 - For snobbing me, thus making");
	notice(whoami, source, "                   me lifeless nuff to do this.");
  } else {
    /* As we copy path parts, (1) lowercase everything and (2) make sure
     * we don't let any special characters through -- this includes '.'
     * (which could get parent dir) or '/' (which couldn't _really_ do
     * anything if we keep '.' out, but better to be on the safe side).
     * Special characters turn into '_'.
     */
    strscpy(buf, HELPSERV_DIR, sizeof(buf));
    ptr = buf + strlen(buf);
    for (s = strtok(topic, " "); s && ptr-buf < sizeof(buf)-1;
						s = strtok(NULL, " ")) {
	*ptr++ = '/';
	while (*s && ptr-buf < sizeof(buf)-1) {
	    if (*s == '.' || *s == '/')
		*ptr++ = '_';
	    else
		*ptr++ = tolower(*s);
	    ++s;
	}
	*ptr = 0;
    }

    /* If we end up at a directory, go for an "index" file/dir if
     * possible.
     */
    while (ptr-buf < sizeof(buf)-1
		&& stat(buf, &st) == 0 && S_ISDIR(st.st_mode)) {
	*ptr++ = '/';
	strscpy(ptr, "index", sizeof(buf) - (ptr-buf));
	ptr += strlen(ptr);
    }

    /* Send the file, if it exists.
     */
    if (!(f = fopen(buf, "r"))) {
	notice(whoami, source,
		"Sorry, no help available for \2%s\2.", old_topic);
	free(old_topic);
	return;
    }
    while (fgets(buf, sizeof(buf), f)) {
	s = strtok(buf, "\n");
	/* Use this odd construction to prevent any %'s in the text from
	 * doing weird stuff to the output.  Also replace blank lines by
	 * spaces (see send.c/notice_list() for an explanation of why).
	 */
	notice(whoami, source, "%s", s ? s : " ");
    }
    fclose(f);
  }
    free(old_topic);
}
