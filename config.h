/* Magick configuration.
 *
 * Magick IRC Services are copyright (c) 1996-1998 Preston A. Elder.
 *     E-mail: <prez@antisocial.com>   IRC: PreZ@RelicNet
 * Originally based on EsperNet services (c) 1996-1998 Andy Church
 * This program is free but copyrighted software; see the file COPYING for
 * details.
 */

#ifndef CONFIG_H
#define	CONFIG_H

/* Live config file */
#define CONFIG_FILE	"magick.ini"

/* Extra warning: if you change these, your data files will be unusable! */

/* Size of input buffer */
#define	BUFSIZE		1024

/* Maximum length of a channel name */
#define	CHANMAX		64

/* Maximum length of a nickname */
#define	NICKMAX		32

/* Maximum length of a password */
#define	PASSMAX		32

/* Maximum number of valid +/- Modes */
#define MODEMAX		64

/* Maximum LastMsg value */
#define LASTMSGMAX	256

/**************************************************************************/

/* System-specific defines */

/* IF your Nick collide works where NEWER NICK takes presidense, then
 * define this (some do - or at least, it works out that way - test it
 * undef'd - if you find your BACKUP services (if any) are getting the
 * services nick's even when your REAL ones are online - define this).
 *
 * OK - I created this when my services had this problem - I later found
 * out the cause - each backup of services needs to have a different set
 * of user@host's for its users (either user or host or both can be
 * different, doesnt matter) - TRY CHANGING THAT before defining this -
 * I'm leaving it in just incase there ARE some wierd ircd's out there.
 */
#undef	WIERD_COLLIDE

#undef	DAL_SERV		/* This should be handled by sysconf.h */
#include "sysconf.h"

#if !HAVE_STRICMP && HAVE_STRCASECMP
# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif

#endif	/* CONFIG_H */
