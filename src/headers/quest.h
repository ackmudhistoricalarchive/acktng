/***************************************************************************
 * quest.h
 *
 * Defines for the quest system.
 * Quest state is stored in QUEST_DATA entries that live in
 * PC_DATA (ack.h). Each PC can store up to QUEST_MAX_QUESTS entries.
 *
 ***************************************************************************/

#ifndef QUEST_H
#define QUEST_H

/* ------------------------------------------------------------------
 * Quest type constants (stored in pcdata->quests[x].quest_type)
 * ------------------------------------------------------------------ */
#define QUEST_TYPE_NONE 0          /* No active quest             */
#define QUEST_TYPE_KILL_VARIETY 1  /* Kill several different mob types */
#define QUEST_TYPE_COLLECT_ITEMS 2 /* Collect several target items */
#define QUEST_TYPE_KILL_COUNT 3    /* Kill N of one mob type */
#define QUEST_TYPE_CARTOGRAPHY 4   /* Explore every room in one area */

/* Maximum number of distinct targets (largest type uses 5 slots) */
#define QUEST_MAX_TARGETS 5

/* Maximum simultaneous quests on a player */
#define QUEST_MAX_QUESTS 5
#define QUEST_MAX_TEMPLATES 200
#define QUEST_CARTOGRAPHY_MAX_ROOMS 2048
#define QUEST_CARTOGRAPHY_BYTES (QUEST_CARTOGRAPHY_MAX_ROOMS / 8)

void do_quest args((CHAR_DATA * ch, char *argument));
void quest_request args((CHAR_DATA * ch, CHAR_DATA *postman));
void quest_status args((CHAR_DATA * ch));
void quest_complete args((CHAR_DATA * ch, CHAR_DATA *postman));
void quest_kill_notify args((CHAR_DATA * ch, CHAR_DATA *victim));
void quest_obj_notify args((CHAR_DATA * ch, OBJ_DATA *obj));
void quest_room_notify args((CHAR_DATA * ch, ROOM_INDEX_DATA *room));
void quest_cancel args((CHAR_DATA * ch, int slot));
void clear_quest args((CHAR_DATA * ch));
void quest_accept args((CHAR_DATA * ch, int list_number));

#endif /* QUEST_H */
