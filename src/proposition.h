/***************************************************************************
 * proposition.h
 *
 * Defines for the proposition quest system.
 * Proposition state is stored in PROPOSITION_DATA entries that live in
 * PC_DATA (ack.h). Each PC can store up to PROP_MAX_PROPOSITIONS entries.
 *
 ***************************************************************************/

#ifndef PROPOSITION_H
#define PROPOSITION_H

/* ------------------------------------------------------------------
 * Quest type constants (stored in pcdata->prop_type)
 * ------------------------------------------------------------------ */
#define PROP_TYPE_NONE          0   /* No active proposition             */
#define PROP_TYPE_KILL_VARIETY  1   /* Kill 3-5 different mob types      */
#define PROP_TYPE_COLLECT_ITEMS 2   /* Collect 2-4 items of right level  */
#define PROP_TYPE_KILL_COUNT    3   /* Kill 5-15 of one mob type         */
#define PROP_TYPE_CARTOGRAPHY   4   /* Explore every room in one area    */

/* Cartography tracks rooms by vnum bitset (0..65535). */
#define PROP_CART_ROOM_MAX_VNUM      65535
#define PROP_CART_ROOM_BITSET_BYTES  ((PROP_CART_ROOM_MAX_VNUM + 8) / 8)

/* Maximum number of distinct targets (largest type uses 5 slots) */
#define PROP_MAX_TARGETS        5

/* Maximum simultaneous propositions on a player */
#define PROP_MAX_PROPOSITIONS   3
#define PROP_MAX_STATIC_PROPOSITIONS 48

/* ------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------ */
void do_proposition          args((CHAR_DATA *ch, char *argument));
void proposition_request     args((CHAR_DATA *ch, CHAR_DATA *postman));
void proposition_status      args((CHAR_DATA *ch));
void proposition_complete    args((CHAR_DATA *ch, CHAR_DATA *postman));
void proposition_kill_notify args((CHAR_DATA *ch, CHAR_DATA *victim));
void proposition_obj_notify  args((CHAR_DATA *ch, OBJ_DATA  *obj));
void proposition_room_notify args((CHAR_DATA *ch, ROOM_INDEX_DATA *room));
void proposition_cancel      args((CHAR_DATA *ch, int slot));
void clear_proposition       args((CHAR_DATA *ch));
void proposition_load_static_templates args((void));

#endif /* PROPOSITION_H */
