/* Miscellaneous routines.
 *
 * Magick is copyright (c) 1996-1997 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@DarkerNet
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#include "services.h"

/*************************************************************************/

/* toupper/tolower:  Like the ANSI functions, but make sure we return an
 *                   int instead of a (signed) char.
 */

int toupper(char c)
{
    if (islower(c))
	return (unsigned char)c - ('a'-'A');
    else
	return (unsigned char)c;
}

int tolower(char c)
{
    if (isupper(c))
	return (unsigned char)c + ('a'-'A');
    else
	return (unsigned char)c;
}

/*************************************************************************/

/* sgets2:  Read a line of text from a socket, and strip newline and
 *          carriage return characters from the end of the line.
 */

char *sgets2(char *buf, long size, int sock)
{
    char *s = sgets(buf, size, sock);
    if (!s || s == (char *)-1)
	return s;
    if (buf[strlen(buf)-1] == '\n')
	buf[strlen(buf)-1] = 0;
    if (buf[strlen(buf)-1] == '\r')
	buf[strlen(buf)-1] = 0;
    return buf;
}

/*************************************************************************/

/* strscpy:  Copy at most len-1 characters from a string to a buffer, and
 *           add a null terminator after the last character copied.
 */

char *strscpy(char *d, const char *s, size_t len)
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

int vsnprintf(char *buf, size_t len, const char *fmt, va_list args)
{
#if BAD_SNPRINTF
# undef vsnprintf
    int res = vsnprintf(buf, len, fmt, args);
# define vsnprintf my_vsnprintf
    if (res < 0)
	res = 0;
    return res;
#else
    /* DANGER WILL ROBINSON!  There's no real portable way to implement
     * vsnprintf(), so we cheat and call vsprintf.  Buffer overflows
     * galore! */
    return vsprintf(buf, fmt, args);
#endif
}

int snprintf(char *buf, size_t len, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    return vsnprintf(buf, len, fmt, args);
}

#endif /* HAVE_SNPRINTF */

/*************************************************************************/

#if !HAVE_STRICMP && !HAVE_STRCASECMP

/* stricmp, strnicmp:  Case-insensitive versions of strcmp() and
 *                     strncmp().
 */

int stricmp(const char *s1, const char *s2)
{
    register int c;

    while ((c = tolower(*s1)) == tolower(*s2)) {
	if (c == 0) return 0;
	s1++; s2++;
    }
    if (c < tolower(*s2))
	return -1;
    return 1;
}

int strnicmp(const char *s1, const char *s2, size_t len)
{
    register int c;

    if (!len) return 0;
    while ((c = tolower(*s1)) == tolower(*s2) && len > 0) {
	if (c == 0 || --len == 0) return 0;
	s1++; s2++;
    }
    if (c < tolower(*s2))
	return -1;
    return 1;
}
#endif

/*************************************************************************/

#if !HAVE_STRDUP
char *strdup(const char *s)
{
    char *new = malloc(strlen(s)+1);
    if (new)
	strcpy(new, s);
    return new;
}
#endif

/*************************************************************************/

#if !HAVE_STRSPN
size_t strspn(const char *s, const char *accept)
{
    size_t i = 0;

    while (*s && strchr(accept, *s))
	++i, ++s;
    return i;
}
#endif

/*************************************************************************/

/* stristr:  Search case-insensitively for string s2 within string s1,
 *           returning the first occurrence of s2 or NULL if s2 was not
 *           found.
 */

char *stristr(char *s1, char *s2)
{
    register char *s = s1, *d = s2;

    while (*s1)
	if (tolower(*s1) == tolower(*d)) {
	    s1++; d++;
	    if (*d == 0)
		return s;
	} else {
	    s = ++s1;
	    d = s2;
	}
    return NULL;
}

/*************************************************************************/

#if !HAVE_STRERROR
# if HAVE_SYS_ERRLIST
extern char *sys_errlist[];
# endif

char *strerror(int errnum)
{
# if HAVE_SYS_ERRLIST
    return sys_errlist[errnum];
# else
    static char buf[32];
    snprintf(buf, sizeof(buf), "Error %d", errnum);
    return buf;
# endif
}
#endif

/*************************************************************************/

#if !HAVE_STRSIGNAL
char *strsignal(int signum)
{
    static char buf[32];
    switch (signum) {
	case SIGHUP:	strcpy(buf, "Hangup"); break;
	case SIGINT:	strcpy(buf, "Interrupt"); break;
	case SIGQUIT:	strcpy(buf, "Quit"); break;
#ifdef SIGILL
	case SIGILL:	strcpy(buf, "Illegal instruction"); break;
#endif
#ifdef SIGABRT
	case SIGABRT:	strcpy(buf, "Abort"); break;
#endif
#if defined(SIGIOT) && (!defined(SIGABRT) || SIGIOT != SIGABRT)
	case SIGIOT:	strcpy(buf, "IOT trap"); break;
#endif
#ifdef SIGBUS
	case SIGBUS:	strcpy(buf, "Bus error"); break;
#endif
	case SIGFPE:	strcpy(buf, "Floating point exception"); break;
	case SIGKILL:	strcpy(buf, "Killed"); break;
	case SIGUSR1:	strcpy(buf, "User signal 1"); break;
	case SIGSEGV:	strcpy(buf, "Segmentation fault"); break;
	case SIGUSR2:	strcpy(buf, "User signal 2"); break;
	case SIGPIPE:	strcpy(buf, "Broken pipe"); break;
	case SIGALRM:	strcpy(buf, "Alarm clock"); break;
	case SIGTERM:	strcpy(buf, "Terminated"); break;
	case SIGSTOP:	strcpy(buf, "Suspended (signal)"); break;
	case SIGTSTP:	strcpy(buf, "Suspended"); break;
	case SIGIO:	strcpy(buf, "I/O error"); break;
	default:	snprintf(buf, sizeof(buf), "Signal %d\n", signum); break;
    }
    return buf;
}
#endif

/*************************************************************************/

/* smalloc, scalloc, srealloc, sstrdup:
 *	Versions of the memory allocation functions which will cause the
 *	program to terminate with an "Out of memory" error if the memory
 *	cannot be allocated.  (Hence, the return value from these functions
 *	is never NULL.)
 */

void *smalloc(long size)
{
    void *buf;

    buf = malloc(size);
    if (!size)
	log("smalloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise(SIGUSR1);
    return buf;
}

void *scalloc(long elsize, long els)
{
    void *buf = calloc(elsize, els);
    if (!elsize || !els)
	log("scalloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise(SIGUSR1);
    return buf;
}

void *srealloc(void *oldptr, long newsize)
{
    void *buf = realloc(oldptr, newsize);
    if (!newsize)
	log("srealloc: Illegal attempt to allocate 0 bytes");
    if (!buf)
	raise(SIGUSR1);
    return buf;
}

char *sstrdup(const char *s)
{
    char *t = strdup(s);
    if (!t)
	raise(SIGUSR1);
    return t;
}

/*************************************************************************/

/* merge_args:  Take an argument count and argument vector and merge them
 *              into a single string in which each argument is separated by
 *              a space.
 */

char *merge_args(int argc, char **argv)
{
    int i;
    static char s[4096];
    char *t;

    t = s;
    for (i = 0; i < argc; ++i)
	t += snprintf(t, sizeof(s)-(t-s), "%s%s", *argv++, (i<argc-1) ? " " : "");
    return s;
}

/*************************************************************************/

/* match_wild:  Attempt to match a string to a pattern which might contain
 *              '*' or '?' wildcards.  Return 1 if the string matches the
 *              pattern, 0 if not.
 */

int do_match_wild(const char *pattern, const char *str, int docase)
{
    char c;
    const char *s;

    /* This WILL eventually terminate: either by *pattern == 0, or by a
     * trailing '*'. */

    for (;;) {
	switch (c = *pattern++) {
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
	    while (*s) {
		if ((docase ? *s == *pattern : (tolower(*s)==tolower(*pattern)))
				&& (docase ? match_wild(pattern, s) :
				match_wild_nocase(pattern, s)))
		    return 1;
		++s;
	    }
	    break;
	  default:
	    if ((docase ? (*str++ != c) : (tolower(*str++) != tolower(c))))
		return 0;
	    break;
	} /* switch */
    }
}

int match_wild(const char *pattern, const char *str)
	{ return do_match_wild(pattern, str, 1); }
int match_wild_nocase(const char *pattern, const char *str)
	{ return do_match_wild(pattern, str, 0); }


/*************************************************************************/

/* month_name:  Return the three-character month name corresponding to the
 *              given month number, 1-12.
 */

static char months[12][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *month_name(int month)
{
    return months[month-1];
}

/*************************************************************************/

/* strupper, strlower:  Convert a string to upper or lower case.
 */

char *strupper(char *s)
{
    char *t = s;
    while (*t)
	*t++ = toupper(*t);
    return s;
}

char *strlower(char *s)
{
    char *t = s;
    while (*t)
	*t++ = tolower(*t);
    return s;
}

/*************************************************************************/

/* read_string, write_string:
 *	Read a string from a file, or write a string to a file, with the
 *	string length prefixed as a two-byte big-endian integer.  The
 *	filename is passed in so that it can be reported in the log file
 *	(and possibly with wallops) if an error occurs.
 */

char *read_string(FILE *f, const char *filename)
{
    char *s;
    int len;

    len = fgetc(f) * 256 + fgetc(f);
    s = smalloc(len);
    if (len != fread(s, 1, len, f))
	fatal_perror("Read error on %s", filename);
    return s;
}

char *write_string(const char *s, FILE *f, const char *filename)
{
    int len;

    len = strlen(s) + 1;	/* Include trailing null */
    fputc(len / 256, f); fputc(len & 255, f);
    if (len != fwrite(s, 1, len, f))
	fatal_perror("Write error on %s", filename);
}

/*************************************************************************/

/* Functions for processing the hash tables */
Hash *get_hash(const char *source, const char *cmd, Hash *hash_table)
{
    for(;hash_table->accept;++hash_table)
	if (match_wild_nocase(hash_table->accept, cmd))
	    if ((hash_table->access==H_OPER && is_oper(source)) ||
		(hash_table->access==H_SOP  && is_services_op(source)) ||
		(hash_table->access==H_NONE))
		return hash_table;
    return NULL;
}

Hash_NI *get_ni_hash(const char *source, const char *cmd, Hash_NI *hash_table)
{
    for(;hash_table->accept;++hash_table)
	if (match_wild_nocase(hash_table->accept, cmd))
	    if ((hash_table->access==H_OPER && is_oper(source)) ||
		(hash_table->access==H_SOP  && is_services_op(source)) ||
		(hash_table->access==H_NONE))
		return hash_table;
    return NULL;
}

Hash_CI *get_ci_hash(const char *source, const char *cmd, Hash_CI *hash_table)
{
    for(;hash_table->accept;++hash_table)
	if (match_wild_nocase(hash_table->accept, cmd))
	    if ((hash_table->access==H_OPER && is_oper(source)) ||
		(hash_table->access==H_SOP  && is_services_op(source)) ||
		(hash_table->access==H_NONE))
		return hash_table;
    return NULL;
}

Hash_HELP *get_help_hash(const char *source, const char *cmd, Hash_HELP *hash_table)
{
    for(;hash_table->accept;++hash_table)
	if (match_wild_nocase(hash_table->accept, cmd))
	    if ((hash_table->access==H_OPER && is_oper(source)) ||
		(hash_table->access==H_SOP  && is_services_op(source)) ||
		(hash_table->access==H_NONE))
		return hash_table;
    return NULL;
}

Hash_CHAN *get_chan_hash(const char *source, const char *cmd, Hash_CHAN *hash_table)
{
    for(;hash_table->accept;++hash_table)
	if (match_wild_nocase(hash_table->accept, cmd))
	    if ((hash_table->access==H_OPER && is_oper(source)) ||
		(hash_table->access==H_SOP  && is_services_op(source)) ||
		(hash_table->access==H_NONE))
		return hash_table;
    return NULL;
}

