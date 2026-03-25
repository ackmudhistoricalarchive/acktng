/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  Ack 2.2 improvements copyright (C) 1994 by Stephen Dooley              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *       _/          _/_/_/     _/    _/     _/    ACK! MUD is modified    *
 *      _/_/        _/          _/  _/       _/    Merc2.0/2.1/2.2 code    *
 *     _/  _/      _/           _/_/         _/    (c)Stephen Zepp 1998    *
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3          *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/*
 * save.h — internal header for the save module.
 * Included only by files within the save/ module; not for external use.
 */

#ifndef SAVE_SAVE_H
#define SAVE_SAVE_H

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include "globals.h"
#include "hash.h"

#if !defined(macintosh)
extern int _filbuf args((FILE *));
#endif

/* Current save-file revision written by fwrite_char. */
#define SAVE_REVISION 16

/* Vnum used as a placeholder when the real object vnum is missing. */
#define TEMP_VNUM 1006

/* Maximum nesting depth for object containers in save files. */
#define MAX_NEST 100

/*
 * KEY / SKEY macros used by the fread_* parsers.
 */
#define KEY(literal, field, value)                                                                 \
   if (!str_cmp(word, literal))                                                                    \
   {                                                                                               \
      field = value;                                                                               \
      fMatch = TRUE;                                                                               \
      break;                                                                                       \
   }

#define SKEY(literal, field, value)                                                                \
   if (!str_cmp(word, literal))                                                                    \
   {                                                                                               \
      if (field != NULL)                                                                           \
         free_string(field);                                                                       \
      field = value;                                                                               \
      fMatch = TRUE;                                                                               \
      break;                                                                                       \
   }

/*
 * Shared global state — defined in save.c, referenced by other save files.
 */
extern int cur_revision;
extern int loop_counter;
extern OBJ_DATA *rgObjNest[];
extern hash_table *hash_changed_vnums;

/*
 * Internal function declarations shared across save module files.
 */

/* save.c */
void abort_wrapper(void);
void init_changed_vnum_hash(void);
void *hash_ref_from_vnum(int vnum);
int vnum_from_hash_ref(void *ref);

/* save_objects.c */
void fwrite_obj args((CHAR_DATA * ch, OBJ_DATA *obj, FILE *fp, int iNest));
void fread_obj args((CHAR_DATA * ch, FILE *fp));

/* save_players.c */
void fwrite_char args((CHAR_DATA * ch, FILE *fp));
void fread_char args((CHAR_DATA * ch, FILE *fp));
const char *skill_name_legacy(const char *old_name);
void alloc_char_for_login(DESCRIPTOR_DATA *d, const char *name);
bool load_char_from_raw(CHAR_DATA *ch, const char *raw);

#endif /* SAVE_SAVE_H */
