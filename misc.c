/* Miscellaneous routines.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@magick.tm>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

/*************************************************************************/

/* toupper/tolower:  Like the ANSI functions, but make sure we return an
 *                   int instead of a (signed) char.
 */
int override_level_val;

int
toupper (char c)
{
    if (islower (c))
	return (unsigned char) c - ('a' - 'A');
    else
	return (unsigned char) c;
}

int
tolower (char c)
{
    if (isupper (c))
	return (unsigned char) c + ('a' - 'A');
    else
	return (unsigned char) c;
}

/*************************************************************************/

/* sgets2:  Read a line of text from a socket, and strip newline and
 *          carriage return characters from the end of the line.
 */

char *
sgets2 (char *buf, long size, int sock)
{
    char *s = sgets (buf, size, sock);
    if (!s || s == (char *) -1)
	return s;
    if (buf[strlen (buf) - 1] == '\n')
	buf[strlen (buf) - 1] = 0;
    if (buf[strlen (buf) - 1] == '\r')
	buf[strlen (buf) - 1] = 0;
    return buf;
}

/*************************************************************************/

/* strscpy:  Copy at most len-1 characters from a string to a buffer, and
 *           add a null terminator after the last character copied.
 */

char *
strscpy (char *d, const char *s, size_t len)
{
    char *d_orig = d;

    if (!len)
	return d;
    while (--len && (*d++ = *s++))
	;
    *d = 0;
    return d_orig;
}

/*************************************************************************/

#if !HAVE_SNPRINTF

/* [v]snprintf: Like [v]sprintf, but don't write more than len bytes
 *              (including null terminator).  Return the number of bytes
 *              written.
 */

int
vsnprintf (char *buf, size_t len, const char *fmt, va_list args)
{
#if BAD_SNPRINTF
#undef vsnprintf
    int res = vsnprintf (buf, len, fmt, args);
#define vsnprintf my_vsnprintf
    if (res < 0)
	res = 0;
    return res;
#else
    /* DANGER WILL ROBINSON!  There's no real portable way to implement
     * vsnprintf(), so we cheat and call vsprintf.  Buffer overflows
     * galore! */
    return vsprintf (buf, fmt, args);
#endif
}

int
snprintf (char *buf, size_t len, const char *fmt,...)
{
    va_list args;

    va_start (args, fmt);
    return vsnprintf (buf, len, fmt, args);
}

#endif /* HAVE_SNPRINTF */

/*************************************************************************/

#if !HAVE_STRICMP && !HAVE_STRCASECMP

/* stricmp, strnicmp:  Case-insensitive versions of strcmp() and
 *                     strncmp().
 */

int
stricmp (const char *s1, const char *s2)
{
    register int c;

    while ((c = tolower (*s1)) == tolower (*s2))
    {
	if (c == 0)
	    return 0;
	s1++;
	s2++;
    }
    if (c < tolower (*s2))
	return -1;
    return 1;
}

int
strnicmp (const char *s1, const char *s2, size_t len)
{
    register int c;

    if (!len)
	return 0;
    while ((c = tolower (*s1)) == tolower (*s2) && len > 0)
    {
	if (c == 0 || --len == 0)
	    return 0;
	s1++;
	s2++;
    }
    if (c < tolower (*s2))
	return -1;
    return 1;
}
#endif

/*************************************************************************/

#if !HAVE_STRDUP
char *
strdup (const char *s)
{
    char *new = malloc (strlen (s) + 1);
    if (new)
	strcpy (new, s);
    return new;
}
#endif

/*************************************************************************/

#if !HAVE_STRSPN
size_t
strspn (const char *s, const char *accept)
{
    size_t i = 0;

    while (*s && strchr (accept, *s))
	++i, ++s;
    return i;
}
#endif

/*************************************************************************/

/* stristr:  Search case-insensitively for string s2 within string s1,
 *           returning the first occurrence of s2 or NULL if s2 was not
 *           found.
 */

char *
stristr (char *s1, char *s2)
{
    register char *s = s1, *d = s2;

    while (*s1)
	if (tolower (*s1) == tolower (*d))
	{
	    s1++;
	    d++;
	    if (*d == 0)
		return s;
	}
	else
	{
	    s = ++s1;
	    d = s2;
	}
    return NULL;
}

/*************************************************************************/

#if !HAVE_STRERROR
#if HAVE_SYS_ERRLIST
extern char *sys_errlist[];
#endif

char *
strerror (int errnum)
{
#if HAVE_SYS_ERRLIST
    return sys_errlist[errnum];
#else
    static char buf[32];
    snprintf (buf, sizeof (buf), "Error %d", errnum);
    return buf;
#endif
}
#endif

/*************************************************************************/

#if !HAVE_STRSIGNAL
char *
strsignal (int signum)
{
    static char buf[32];
    switch (signum)
    {
#ifdef SIGHUP
    case SIGHUP:
	strscpy (buf, "Hangup", sizeof (buf));
	break;
#endif
    case SIGINT:
	strscpy (buf, "Interrupt", sizeof (buf));
	break;
#ifdef SIGQUIT
    case SIGQUIT:
	strscpy (buf, "Quit", sizeof (buf));
	break;
#endif
#ifdef SIGILL
    case SIGILL:
	strscpy (buf, "Illegal instruction", sizeof (buf));
	break;
#endif
#ifdef SIGABRT
    case SIGABRT:
	strscpy (buf, "Abort", sizeof (buf));
	break;
#endif
#if defined(SIGIOT) && (!defined(SIGABRT) || SIGIOT != SIGABRT)
    case SIGIOT:
	strscpy (buf, "IOT trap", sizeof (buf));
	break;
#endif
#ifdef SIGBUS
    case SIGBUS:
	strscpy (buf, "Bus error", sizeof (buf));
	break;
#endif
    case SIGFPE:
	strscpy (buf, "Floating point exception", sizeof (buf));
	break;
#ifdef SIGKILL
    case SIGKILL:
	strscpy (buf, "Killed", sizeof (buf));
	break;
#endif
#ifdef SIGUSR1
    case SIGUSR1:
	strscpy (buf, "User signal 1", sizeof (buf));
	break;
#endif
    case SIGSEGV:
	strscpy (buf, "Segmentation fault", sizeof (buf));
	break;
#ifdef SIGUSR2
    case SIGUSR2:
	strscpy (buf, "User signal 2", sizeof (buf));
	break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
	strscpy (buf, "Broken pipe", sizeof (buf));
	break;
#endif
    case SIGALRM:
	strscpy (buf, "Alarm clock", sizeof (buf));
	break;
    case SIGTERM:
	strscpy (buf, "Terminated", sizeof (buf));
	break;
#ifdef SIGSTOP
    case SIGSTOP:
	strscpy (buf, "Suspended (signal)", sizeof (buf));
	break;
#endif
#ifdef SIGTSTP
    case SIGTSTP:
	strscpy (buf, "Suspended", sizeof (buf));
	break;
#endif
#ifdef SIGIO
    case SIGIO:
	strscpy (buf, "I/O error", sizeof (buf));
	break;
#endif
    default:
	snprintf (buf, sizeof (buf), "Signal %d\n", signum);
	break;
    }
    return buf;
}
#endif

/*************************************************************************/

/* smalloc, scalloc, srealloc, sstrdup:
 *    Versions of the memory allocation functions which will cause the
 *      program to terminate with an "Out of memory" error if the memory
 *      cannot be allocated.  (Hence, the return value from these functions
 *      is never NULL.)
 */

void *
smalloc (long size)
{
    void *buf;

    buf = malloc (size);
    if (!size)
	write_log ("smalloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise (SIGUSR1);
    return buf;
}

void *
scalloc (long elsize, long els)
{
    void *buf = calloc (elsize, els);
    if (!elsize || !els)
	write_log ("scalloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise (SIGUSR1);
    return buf;
}

void *
srealloc (void *oldptr, long newsize)
{
    void *buf = realloc (oldptr, newsize);
    if (!newsize)
	write_log ("srealloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise (SIGUSR1);
    return buf;
}

char *
sstrdup (const char *s)
{
    char *t = strdup (s);
    if (!t)
	raise (SIGUSR1);
    return t;
}

/*************************************************************************/

/* merge_args:  Take an argument count and argument vector and merge them
 *              into a single string in which each argument is separated by
 *              a space.
 */

char *
merge_args (int argc, char **argv)
{
    int i;
    static char s[4096];
    char *t;

    t = s;
    for (i = 0; i < argc; ++i)
	t += snprintf (t, sizeof (s) - (t - s), "%s%s", *argv++, (i < argc - 1) ? " " : "");
    return s;
}

/*************************************************************************/

/* match_wild:  Attempt to match a string to a pattern which might contain
 *              '*' or '?' wildcards.  Return 1 if the string matches the
 *              pattern, 0 if not.
 */

int
do_match_wild (const char *pattern, const char *str, int docase)
{
    char c;
    const char *s;

    /* This WILL eventually terminate: either by *pattern == 0, or by a
     * trailing '*'. */

    for (;;)
    {
	switch (c = *pattern++)
	{
	case 0:
	    if (!*str)
		return 1;
	    return 0;
	case '?':
	    if (!*str)
		return 0;
	    ++str;
	    break;
	case '*':
	    if (!*pattern)
		return 1;	/* trailing '*' matches everything else */
	    s = str;
	    while (*s)
	    {
		if ((docase ? *s == *pattern : (tolower (*s) == tolower (*pattern)))
		    && (docase ? match_wild (pattern, s) :
			match_wild_nocase (pattern, s)))
		    return 1;
		++s;
	    }
	    break;
	default:
	    if ((docase ? (*str++ != c) : (tolower (*str++) != tolower (c))))
		return 0;
	    break;
	}			/* switch */
    }
}

int
match_wild (const char *pattern, const char *str)
{
    return do_match_wild (pattern, str, 1);
}
int
match_wild_nocase (const char *pattern, const char *str)
{
    return do_match_wild (pattern, str, 0);
}


/*************************************************************************/

/* month_name:  Return the three-character month name corresponding to the
 *              given month number, 1-12.
 */

static char months[12][4] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *
month_name (int month)
{
    return months[month - 1];
}

/*************************************************************************/

/* strupper, strlower:  Convert a string to upper or lower case.
 */

char *
strupper (char *s)
{
    char *t = s;
    while (*t)
	*t++ = toupper (*t);
    return s;
}

char *
strlower (char *s)
{
    char *t = s;
    while (*t)
	*t++ = tolower (*t);
    return s;
}

/*************************************************************************/

/* Convert a number to a string (reverse of atoi). */

char *
myctoa (char c)
{
    static char ret[2];
    ret[0] = c;
    ret[1] = 0;
    return ret;
}

char *
myitoa (int num)
{
    static char ret[7] = "";
    int i = 0, j, k;
    float x;

    if (num < 0)
    {
	ret[0] = '-';
	i++;
    }

    /* Find size of integer! */
    for (j = 0, x = 1.0; j < sizeof (num); j++)
	x *= 256.0;
    for (k = 0; x >= 10.0; x /= 10.0, k++);
    for (j = 0, x = 1.0; j < k; j++)
	x *= 10.0;

    for (j = abs (num); x > 1.0; x /= 10.0)
	if (j >= x)
	{
	    for (k = 0; j >= x; j -= x, k++);
	    ret[i] = k + 48;
	    ret[i + 1] = 0;
	    i++;
	}
    ret[i] = j + 48;
    ret[i + 1] = 0;

    return ret;
}

/* call:
 * 0 == Do not effect with TimeZone
 * 1 == Add/Remove if TimeZone != 0
 */
char *
time_ago (time_t mytime, int call)
{
    if (services_level > 1 && !!tz_offset && call)
	mytime += tz_offset * 60 * 60;
    return disect_time (time (NULL) - mytime, 0);
}
char *
disect_time (time_t time, int call)
{
    static char ret[32];
    int years, days, hours, minutes, seconds;
    years = days = hours = minutes = seconds = 0;

    if (services_level > 1 && !!tz_offset && call)
	time += tz_offset * 60 * 60;
    while (time >= 60 * 60 * 24 * 365)
    {
	time -= 60 * 60 * 24 * 365;
	years++;
    }
    while (time >= 60 * 60 * 24)
    {
	time -= 60 * 60 * 24;
	days++;
    }
    while (time >= 60 * 60)
    {
	time -= 60 * 60;
	hours++;
    }
    while (time >= 60)
    {
	time -= 60;
	minutes++;
    }
    seconds = time;

    if (years)
	snprintf (ret, sizeof (ret), "%d year%s, %d day%s, %02d:%02d:%02d",
		  years, years == 1 ? "" : "s",
		  days, days == 1 ? "" : "s",
		  hours, minutes, seconds);
    else if (days)
	snprintf (ret, sizeof (ret), "%d day%s, %02d:%02d:%02d",
		  days, days == 1 ? "" : "s",
		  hours, minutes, seconds);
    else if (hours)
	snprintf (ret, sizeof (ret), "%d hour%s, %d minute%s, %d second%s",
		  hours, hours == 1 ? "" : "s",
		  minutes, minutes == 1 ? "" : "s",
		  seconds, seconds == 1 ? "" : "s");
    else if (minutes)
	snprintf (ret, sizeof (ret), "%d minute%s, %d second%s",
		  minutes, minutes == 1 ? "" : "s",
		  seconds, seconds == 1 ? "" : "s");
    else
	snprintf (ret, sizeof (ret), "%d second%s",
		  seconds, seconds == 1 ? "" : "s");
    return ret;
}

/*************************************************************************/

/* read_string, write_string:
 *    Read a string from a file, or write a string to a file, with the
 *      string length prefixed as a two-byte big-endian integer.  The
 *      filename is passed in so that it can be reported in the log file
 *      (and possibly with wallops) if an error occurs.
 */

char *
read_string (FILE * f, const char *filename)
{
    char *s;
    int len;

    len = fgetc (f) * 256 + fgetc (f);
    s = smalloc (len);
    if (len != fread (s, 1, len, f))
	fatal_perror ("Read error on %s", filename);
    return s;
}

void
write_string (const char *s, FILE * f, const char *filename)
{
    int len;

    len = strlen (s) + 1;	/* Include trailing null */
    fputc (len / 256, f);
    fputc (len & 255, f);
    if (len != fwrite (s, 1, len, f))
	fatal_perror ("Write error on %s", filename);
}

/*************************************************************************/

/* Functions for processing the hash tables */

/* Standard [void command(source)] */
Hash *
get_hash (const char *source, const char *cmd, Hash * hash_table)
{
    for (; hash_table->accept; ++hash_table)
	if (match_wild_nocase (hash_table->accept, cmd))
	    if ((hash_table->access == H_OPER && is_oper (source)) ||
		(hash_table->access == H_SOP && is_services_op (source)) ||
	    (hash_table->access == H_ADMIN && is_services_admin (source)) ||
		(hash_table->access == H_NONE))
		return hash_table;
    return NULL;
}

/* NickServ SET [void command(ni, param)] */
Hash_NI *
get_ni_hash (const char *source, const char *cmd, Hash_NI * hash_table)
{
    for (; hash_table->accept; ++hash_table)
	if (match_wild_nocase (hash_table->accept, cmd))
	    if ((hash_table->access == H_OPER && is_oper (source)) ||
		(hash_table->access == H_SOP && is_services_op (source)) ||
	    (hash_table->access == H_ADMIN && is_services_admin (source)) ||
		(hash_table->access == H_NONE))
		return hash_table;
    return NULL;
}

/* ChanServ SET [void command(u, ci, param)] */
Hash_CI *
get_ci_hash (const char *source, const char *cmd, Hash_CI * hash_table)
{
    for (; hash_table->accept; ++hash_table)
	if (match_wild_nocase (hash_table->accept, cmd))
	    if ((hash_table->access == H_OPER && is_oper (source)) ||
		(hash_table->access == H_SOP && is_services_op (source)) ||
	    (hash_table->access == H_ADMIN && is_services_admin (source)) ||
		(hash_table->access == H_NONE))
		return hash_table;
    return NULL;
}

/* ALL Help files [const char *(*command)] */
Hash_HELP *
get_help_hash (const char *source, const char *cmd, Hash_HELP * hash_table)
{
    for (; hash_table->accept; ++hash_table)
	if (match_wild_nocase (hash_table->accept, cmd))
	    if ((hash_table->access == H_OPER && is_oper (source)) ||
		(hash_table->access == H_SOP && is_services_op (source)) ||
	    (hash_table->access == H_ADMIN && is_services_admin (source)) ||
		(hash_table->access == H_NONE))
		return hash_table;
    return NULL;
}

/* ChanServ commands [void command(source, chan)] */
Hash_CHAN *
get_chan_hash (const char *source, const char *cmd, Hash_CHAN * hash_table)
{
    for (; hash_table->accept; ++hash_table)
	if (match_wild_nocase (hash_table->accept, cmd))
	    if ((hash_table->access == H_OPER && is_oper (source)) ||
		(hash_table->access == H_SOP && is_services_op (source)) ||
	    (hash_table->access == H_ADMIN && is_services_admin (source)) ||
		(hash_table->access == H_NONE))
		return hash_table;
    return NULL;
}

/*************************************************************************/

/* Does the function fit the override? */

int
override (const char *source, int level)
{
    switch (override_level_val)
    {
    case 1:
	if (is_oper (source))
	    return 1;
	break;
    case 2:
	if (level == CO_OPER && is_oper (source))
	    return 1;
	else if (level == CO_SOP && is_services_op (source))
	    return 1;
	else if (level == CO_ADMIN && is_services_admin (source))
	    return 1;
	break;
    case 3:
	if (is_services_op (source))
	    return 1;
	break;
    case 4:
	if (level == CO_OPER && is_services_op (source))
	    return 1;
	else if (level >= CO_SOP && is_services_admin (source))
	    return 1;
	break;
    case 5:
	if (is_services_admin (source))
	    return 1;
	break;
    }
    return 0;
}

int
hasmode (const char *mode, const char *modestr)
{
    int i, j, k, add = 1;
    char mode_on[64], mode_off[64];

    /* Modes we want, or dont want */
    for (i=0, j=0, k=0; mode[i]; i++)
    {
	if (mode[i]=='+') add = 1;
	else if (mode[i]=='-') add = 0;
	else if ((mode[i] >= 65 && mode[i] <= 90) ||   /* A-Z */
		 (mode[i] >= 97 && mode[i] <= 122))    /* a-z */
	{
	    add ? (mode_on[j] = mode[i]) : (mode_off[k] = mode[i]);
	    add ? j++ : k++;
	}
	mode_on[j] = mode_off[k] = 0;
    }

    /* What we DO need is there ... */
    for (i=0; mode_on[i]; i++)
    {
	for (j=0; modestr[j]; j++)
	    if (mode_on[i]==modestr[j])
		break;
	if (!(modestr[j]))
	    return 0;
    }

    /* What we DONT want isnt there ... */
    for (i=0; mode_off[i]; i++)
    {
	for (j=0; modestr[j]; j++)
	    if (mode_off[i]==modestr[j])
		break;
	if (modestr[j])
	    return 0;
    }

    return 1;
}

char *
changemode (const char *mode, const char *modestr)
{

    int i, j, k, add = 1;
    char mode_on[64], mode_off[64];
    static char newmodestr[64];
    strscpy (newmodestr, "", sizeof(newmodestr));

    /* Modes we want, or dont want */
    for (i=0, j=0, k=0; mode[i]; i++)
    {
	if (mode[i]=='+') add = 1;
	else if (mode[i]=='-') add = 0;
	else if ((mode[i] >= 65 && mode[i] <= 90) ||   /* A-Z */
		 (mode[i] >= 97 && mode[i] <= 122))    /* a-z */
	{
	    add ? (mode_on[j] = mode[i]) : (mode_off[k] = mode[i]);
	    add ? j++ : k++;
	}
	mode_on[j] = mode_off[k] = 0;
    }

    k=0;
    for (i=0; modestr[i]; i++)
    {
	for (j=0; mode_off[j]; j++)
	    if (mode_off[j]==modestr[i])
		break;
	if (!(mode_off[j]))
	{
	    newmodestr[k] = modestr[i];
	    k++; newmodestr[k] = 0;
	}
    }

    for (i=0; mode_on[i]; i++)
    {
	for (j=0; newmodestr[j]; j++)
	    if (mode_on[i]==newmodestr[j])
		break;
	if (!(newmodestr[j]))
	{
	    newmodestr[k] = mode_on[i];
	    k++; newmodestr[k] = 0;
	}
    }
    return newmodestr;
}
