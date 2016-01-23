/* WHAT IS THIS PROGRAM:
 *     Just Little program to add 5 to all access levels
 *     >= 5 to accomodate for the new +v on level 5
 *     instead of +o (implemented in 3.0.4).
 *
 * WARNING: BACK UP YOUR DATA FILES!!
 *          ALL of the fatal error checking in the normal
 *          routines contained in this file have been
 *          REMOVED, and therefore, it *WILL NOT* halt,
 *          etc as it normally would.
 */
 
#include "services.h"

#ifdef CHANSERV

ChannelInfo *chanlists[256];
ChannelInfo *cs_findchan(const char *chan);

void load_cs_dbase(void);
void save_cs_dbase(void);
static void alpha_insert_chan(ChannelInfo *ci);


int tolower(char c)
{
    if (isupper(c))
	return (unsigned char)c + ('a'-'A');
    else
	return (unsigned char)c;
}

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

#if !HAVE_STRDUP
char *strdup(const char *s)
{
    char *new = malloc(strlen(s)+1);
    if (new)
	strcpy(new, s);
    return new;
}
#endif

char *sstrdup(const char *s)
{
    char *t = strdup(s);
    if (!t)
	raise(SIGUSR1);
    return t;
}

void *srealloc(void *oldptr, long newsize)
{
    void *buf = realloc(oldptr, newsize);
    if (!newsize)
	;
    if (!buf)
	raise(SIGUSR1);
    return buf;
}

void *smalloc(long size)
{
    void *buf;

    buf = malloc(size);
    if (!size) ;
    if (!buf)
	raise(SIGUSR1);
    return buf;
}

char *read_string(FILE *f, const char *filename)
{
    char *s;
    int len;

    len = fgetc(f) * 256 + fgetc(f);
    s = smalloc(len);
    if (len != fread(s, 1, len, f))
	;
    return s;
}

char *write_string(const char *s, FILE *f, const char *filename)
{
    int len;

    len = strlen(s) + 1;	/* Include trailing null */
    fputc(len / 256, f); fputc(len & 255, f);
    if (len != fwrite(s, 1, len, f))
	;
}

int get_file_version(FILE *f, const char *filename)
{
    int version = fgetc(f)<<24 | fgetc(f)<<16 | fgetc(f)<<8 | fgetc(f);
    if (ferror(f) || feof(f))
	;
    else if (version > FILE_VERSION || version < 1)
	;
    return version;
}

void write_file_version(FILE *f, const char *filename)
{
    if (
	fputc(FILE_VERSION>>24 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION>>16 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION>> 8 & 0xFF, f) < 0 ||
	fputc(FILE_VERSION     & 0xFF, f) < 0
    )
	;
}


void load_cs_dbase(void)
{
    FILE *f = fopen(CHANSERV_DB, "r");
    int i, j, len;
    ChannelInfo *ci;

    if (!f) {
	return;
    }

    switch (i = get_file_version(f, CHANSERV_DB)) {

      case 4:
      case 3:
#if FILE_VERSION > 2

	for (i = 33; i < 256; ++i) {

	    while (fgetc(f) == 1) {

		ci = smalloc(sizeof(ChannelInfo));
		if (1 != fread(ci, sizeof(ChannelInfo), 1, f))
			;
		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan(ci);
		ci->desc = read_string(f, CHANSERV_DB);
		ci->url = read_string(f, CHANSERV_DB);
		if (ci->mlock_key)
		    ci->mlock_key = read_string(f, CHANSERV_DB);
		if (ci->last_topic)
		    ci->last_topic = read_string(f, CHANSERV_DB);

		if (ci->accesscount) {
		    ChanAccess *access;
		    access = smalloc(sizeof(ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread(access, sizeof(ChanAccess),
							ci->accesscount, f))
			;
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string(f, CHANSERV_DB);
		    j = 0; access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount) {
			if (access->is_nick < 0) {
			    --ci->accesscount;
			    free(access->name);
			    if (j < ci->accesscount)
				bcopy(access+1, access,	sizeof(*access) *
							(ci->accesscount - j));
			} else {
			    ++j; ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc(ci->access,
					sizeof(ChanAccess) * ci->accesscount);
		    else {
			free(ci->access);
			ci->access = NULL;
		    }
		} /* if (ci->accesscount) */

		if (ci->akickcount) {
		    AutoKick *akick;
		    akick = smalloc(sizeof(AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			    fread(akick, sizeof(AutoKick), ci->akickcount, f))
				;
		    for (j = 0; j < ci->akickcount; ++j, ++akick) {
			akick->name = read_string(f, CHANSERV_DB);
			if (akick->reason)
			    akick->reason = read_string(f, CHANSERV_DB);
		    }
		    j = 0; akick = ci->akick;
		    while (j < ci->akickcount) {
			if (akick->is_nick < 0) {
			    --ci->akickcount;
			    free(akick->name);
			    if (akick->reason)
				free(akick->reason);
			    if (j < ci->akickcount)
				bcopy(akick+1, akick, sizeof(*akick) *
							(ci->akickcount - j));
			} else {
			    ++j; ++akick;
			}
		    }
		    if (ci->akickcount) {
			ci->akick = srealloc(ci->akick,
					sizeof(AutoKick) * ci->akickcount);
		    } else {
			free(ci->akick);
			ci->akick = NULL;
		    }
		} /* if (ci->akickcount) */

		if (ci->cmd_access) {
		    int n_entries;
		    ci->cmd_access = smalloc(sizeof(short) * CA_SIZE);
		    n_entries = fgetc(f)<<8 | fgetc(f);
		    if (n_entries < 0)
				;
		    if (n_entries <= CA_SIZE) {
			fread(ci->cmd_access, sizeof(short), n_entries, f);
		    } else {
			fread(ci->cmd_access, sizeof(short), CA_SIZE, f);
			fseek(f, sizeof(short) * (n_entries - CA_SIZE),
								SEEK_CUR);
		    }
		}

	    } /* while (fgetc(f) == 1) */

	} /* for (i) */

	break; /* case 3, etc. */

#endif
      case 2:
      case 1:

	for (i = 33; i < 256; ++i) {

	    while (fgetc(f) == 1) {

		ci = smalloc(sizeof(ChannelInfo));
		if (1 != fread(ci, sizeof(ChannelInfo), 1, f))
			;
		/* Can't guarantee the file is in a particular order...
		 * (Well, we can, but we don't have to depend on it.) */
		alpha_insert_chan(ci);
		ci->desc = read_string(f, CHANSERV_DB);
#if FILE_VERSION > 2
		ci->url = sstrdup("");
#endif
		if (ci->mlock_key)
		    ci->mlock_key = read_string(f, CHANSERV_DB);
		if (ci->last_topic)
		    ci->last_topic = read_string(f, CHANSERV_DB);

		if (ci->accesscount) {
		    ChanAccess *access;
		    access = smalloc(sizeof(ChanAccess) * ci->accesscount);
		    ci->access = access;
		    if (ci->accesscount != fread(access, sizeof(ChanAccess),
							ci->accesscount, f))
				;
		    for (j = 0; j < ci->accesscount; ++j, ++access)
			access->name = read_string(f, CHANSERV_DB);
		    j = 0; access = ci->access;
		    /* Clear out unused entries */
		    while (j < ci->accesscount) {
			if (access->is_nick < 0) {
			    --ci->accesscount;
			    free(access->name);
			    if (j < ci->accesscount)
				bcopy(access+1, access,	sizeof(*access) *
							(ci->accesscount - j));
			} else {
			    ++j; ++access;
			}
		    }
		    if (ci->accesscount)
			ci->access = srealloc(ci->access,
					sizeof(ChanAccess) * ci->accesscount);
		    else {
			free(ci->access);
			ci->access = NULL;
		    }
		} /* if (ci->accesscount) */

		if (ci->akickcount) {
		    AutoKick *akick;
		    akick = smalloc(sizeof(AutoKick) * ci->akickcount);
		    ci->akick = akick;
		    if (ci->akickcount !=
			    fread(akick, sizeof(AutoKick), ci->akickcount, f))
				;
		    for (j = 0; j < ci->akickcount; ++j, ++akick) {
			akick->name = read_string(f, CHANSERV_DB);
			if (akick->reason)
			    akick->reason = read_string(f, CHANSERV_DB);
		    }
		    j = 0; akick = ci->akick;
		    while (j < ci->akickcount) {
			if (akick->is_nick < 0) {
			    --ci->akickcount;
			    free(akick->name);
			    if (akick->reason)
				free(akick->reason);
			    if (j < ci->akickcount)
				bcopy(akick+1, akick, sizeof(*akick) *
							(ci->akickcount - j));
			} else {
			    ++j; ++akick;
			}
		    }
		    if (ci->akickcount) {
			ci->akick = srealloc(ci->akick,
					sizeof(AutoKick) * ci->akickcount);
		    } else {
			free(ci->akick);
			ci->akick = NULL;
		    }
		} /* if (ci->akickcount) */

		if (ci->cmd_access) {
		    int n_entries;
		    ci->cmd_access = smalloc(sizeof(short) * CA_SIZE);
		    n_entries = fgetc(f)<<8 | fgetc(f);
		    if (n_entries < 0)
				;
		    if (n_entries <= CA_SIZE) {
			fread(ci->cmd_access, sizeof(short), n_entries, f);
		    } else {
			fread(ci->cmd_access, sizeof(short), CA_SIZE, f);
			fseek(f, sizeof(short) * (n_entries - CA_SIZE),
								SEEK_CUR);
		    }
		}

	    } /* while (fgetc(f) == 1) */

	} /* for (i) */

	break; /* case 1, etc. */

      default:
		;

    } /* switch (version) */

    fclose(f);
}

/*************************************************************************/

void save_cs_dbase(void)
{
    FILE *f;
    int i, j, len;
    ChannelInfo *ci;

    remove(CHANSERV_DB ".save");
    if (rename(CHANSERV_DB, CHANSERV_DB ".save") < 0)
		;
    f = fopen(CHANSERV_DB, "w");
    if (!f) {
		;
	if (rename(CHANSERV_DB ".save", CHANSERV_DB) < 0)
		;
	return;
    }
    write_file_version(f, CHANSERV_DB);

    for (i = 33; i < 256; ++i) {
	for (ci = chanlists[i]; ci; ci = ci->next) {
	    fputc(1, f);
	    if (1 != fwrite(ci, sizeof(ChannelInfo), 1, f))
			;
	    write_string(ci->desc ? ci->desc : "", f, CHANSERV_DB);
#if FILE_VERSION > 2
		write_string(ci->desc ? ci->url : "", f, CHANSERV_DB);
#endif
	    if (ci->mlock_key)
		write_string(ci->mlock_key, f, CHANSERV_DB);
	    if (ci->last_topic)
		write_string(ci->last_topic, f, CHANSERV_DB);

	    if (ci->accesscount) {
		ChanAccess *access = ci->access;
		if (ci->accesscount !=
			fwrite(access, sizeof(ChanAccess), ci->accesscount, f))
				;
		for (j = 0; j < ci->accesscount; ++j, ++access)
		    write_string(access->name, f, CHANSERV_DB);
	    }

	    if (ci->akickcount) {
		AutoKick *akick = ci->akick;
		if (ci->akickcount !=
			fwrite(akick, sizeof(AutoKick), ci->akickcount, f))
				;
		for (j = 0; j < ci->akickcount; ++j, ++akick) {
		    write_string(akick->name, f, CHANSERV_DB);
		    if (akick->reason)
			write_string(akick->reason, f, CHANSERV_DB);
		}
	    }

	    if (ci->cmd_access) {
		fputc(CA_SIZE >> 8, f);
		fputc(CA_SIZE & 0xFF, f);
		fwrite(ci->cmd_access, sizeof(short), CA_SIZE, f);
	    }

	} /* for (chanlists[i]) */

	fputc(0, f);

    } /* for (i) */

    fclose(f);
    remove(CHANSERV_DB ".save");
}

static void alpha_insert_chan(ChannelInfo *ci)
{
    ChannelInfo *ci2, *ci3;
    char *chan = ci->name;

    for (ci3 = NULL, ci2 = chanlists[tolower(chan[1])];
			ci2 && stricmp(ci2->name, chan) < 0;
			ci3 = ci2, ci2 = ci2->next)
	;
    ci->prev = ci3;
    ci->next = ci2;
    if (!ci3)
	chanlists[tolower(chan[1])] = ci;
    else
	ci3->next = ci;
    if (ci2)
	ci2->prev = ci;
}


void main() {

	ChannelInfo *ci;
	int i, j;
	ChanAccess *access;

	load_cs_dbase();

	for (i = 33; i < 256; ++i) {
		for (ci = chanlists[i]; ci; ci = ci->next) {
			for (access = ci->access, j = 0; j < ci->accesscount; ++access, ++j) {
				if (access->level >= 5)
					access->level += 5;
			}
		}
	}

	save_cs_dbase();
}
#endif /* CHANSERV */
