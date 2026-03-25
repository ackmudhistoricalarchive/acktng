#include <assert.h>
#include <ctype.h>
#include <string.h>

#define DEC_GLOBALS_H 1
#include "globals.h"

/* ------------------------------------------------------------------ */
/* Captured output tracking                                             */
/* ------------------------------------------------------------------ */

static char last_send_buf[4096];
static int send_count;
static int room_changed;
static ROOM_INDEX_DATA *last_dest_room;

void send_to_char(const char *txt, CHAR_DATA *ch)
{
   if (txt != NULL)
      strncpy(last_send_buf, txt, sizeof(last_send_buf) - 1);
   send_count++;
}

void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type)
{
}

void char_from_room(CHAR_DATA *ch)
{
   ch->in_room = NULL;
}

void char_to_room(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
   ch->in_room = room;
   last_dest_room = room;
   room_changed++;
}

void do_look(CHAR_DATA *ch, char *arg)
{
}

/* Minimal str_prefix: returns FALSE (match) when astr is a prefix of bstr */
bool str_prefix(const char *astr, const char *bstr)
{
   if (astr == NULL || bstr == NULL)
      return TRUE;
   while (*astr != '\0')
   {
      if (*bstr == '\0')
         return TRUE;
      if (tolower((unsigned char)*astr) != tolower((unsigned char)*bstr))
         return TRUE;
      astr++;
      bstr++;
   }
   return FALSE;
}

/* Stub get_room_index: returns a room only for the four destination vnums */
#define KIESS_VNUM 3661
#define KOWLOON_VNUM 3833
#define MAFDET_VNUM 3878
#define RAKUEN_VNUM 4556
#define MIDGAARD_CARAVAN_VNUM 1151

static ROOM_INDEX_DATA kiess_room;
static ROOM_INDEX_DATA kowloon_room;
static ROOM_INDEX_DATA mafdet_room;
static ROOM_INDEX_DATA rakuen_room;
static ROOM_INDEX_DATA midgaard_room;

ROOM_INDEX_DATA *get_room_index(int vnum)
{
   switch (vnum)
   {
   case KIESS_VNUM:
      return &kiess_room;
   case KOWLOON_VNUM:
      return &kowloon_room;
   case MAFDET_VNUM:
      return &mafdet_room;
   case RAKUEN_VNUM:
      return &rakuen_room;
   default:
      return NULL;
   }
}

/* ------------------------------------------------------------------ */
/* do_caravan forward declaration                                       */
/* ------------------------------------------------------------------ */
void do_caravan(CHAR_DATA *ch, char *argument);

/* ------------------------------------------------------------------ */
/* Helper: build a minimal PC in the Midgaard caravan square           */
/* ------------------------------------------------------------------ */
static void clear_character(CHAR_DATA *ch, PC_DATA *pcdata)
{
   memset(ch, 0, sizeof(*ch));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
   memset(pcdata, 0, sizeof(*pcdata));
   ch->pcdata = pcdata;
   ch->in_room = &midgaard_room;
   last_send_buf[0] = '\0';
   send_count = 0;
   room_changed = 0;
   last_dest_room = NULL;
}

/* ------------------------------------------------------------------ */
/* Tests                                                                */
/* ------------------------------------------------------------------ */

static void test_npc_is_ignored(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   ch.pcdata = NULL;    /* NPC has no pcdata */
   ch.act = ACT_IS_NPC; /* mark as NPC */
   do_caravan(&ch, "kiess");
   assert(send_count == 0); /* NPCs get no output and are silently rejected */
   assert(room_changed == 0);
}

static void test_fighting_is_blocked(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   CHAR_DATA enemy;
   clear_character(&ch, &pcdata);
   memset(&enemy, 0, sizeof(enemy));
   ch.fighting = &enemy;
   do_caravan(&ch, "kiess");
   assert(send_count > 0);
   assert(room_changed == 0);
}

static void test_wrong_room_is_blocked(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   ROOM_INDEX_DATA other_room;
   clear_character(&ch, &pcdata);
   memset(&other_room, 0, sizeof(other_room));
   other_room.vnum = 9999;
   ch.in_room = &other_room;
   do_caravan(&ch, "kiess");
   assert(send_count > 0);
   assert(room_changed == 0);
}

static void test_empty_argument_shows_usage(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   do_caravan(&ch, "");
   assert(send_count > 0);
   assert(room_changed == 0);
}

static void test_invalid_city_shows_usage(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   do_caravan(&ch, "narnia");
   assert(send_count > 0);
   assert(room_changed == 0);
}

static void test_missing_cartography_blocks_travel(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   /* completed_quests is all FALSE (zeroed by memset) */
   do_caravan(&ch, "kiess");
   assert(send_count > 0);
   assert(room_changed == 0);
}

/* Template IDs for each city's cartography survey (file N.prop -> id N-1) */
#define KIESS_TEMPLATE_ID 144
#define KOWLOON_TEMPLATE_ID 145
#define MAFDET_TEMPLATE_ID 146
#define RAKUEN_TEMPLATE_ID 148

static void test_kiess_travel_with_cartography_done(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   kiess_room.vnum = KIESS_VNUM;
   pcdata.completed_quests[KIESS_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "kiess");
   assert(room_changed > 0);
   assert(last_dest_room == &kiess_room);
}

static void test_kowloon_travel_with_cartography_done(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   kowloon_room.vnum = KOWLOON_VNUM;
   pcdata.completed_quests[KOWLOON_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "kowloon");
   assert(room_changed > 0);
   assert(last_dest_room == &kowloon_room);
}

static void test_mafdet_travel_with_cartography_done(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   mafdet_room.vnum = MAFDET_VNUM;
   pcdata.completed_quests[MAFDET_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "mafdet");
   assert(room_changed > 0);
   assert(last_dest_room == &mafdet_room);
}

static void test_rakuen_travel_with_cartography_done(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   rakuen_room.vnum = RAKUEN_VNUM;
   pcdata.completed_quests[RAKUEN_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "rakuen");
   assert(room_changed > 0);
   assert(last_dest_room == &rakuen_room);
}

static void test_partial_name_prefix_works(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   kowloon_room.vnum = KOWLOON_VNUM;
   pcdata.completed_quests[KOWLOON_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "kow"); /* prefix of "kowloon" */
   assert(room_changed > 0);
   assert(last_dest_room == &kowloon_room);
}

static void test_other_city_cartography_does_not_unlock_kiess(void)
{
   CHAR_DATA ch;
   PC_DATA pcdata;
   clear_character(&ch, &pcdata);
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;
   /* Mark kowloon done but try kiess — should still be blocked */
   pcdata.completed_quests[KOWLOON_TEMPLATE_ID] = TRUE;
   do_caravan(&ch, "kiess");
   assert(room_changed == 0);
}

/* ------------------------------------------------------------------ */
int main(void)
{
   memset(&midgaard_room, 0, sizeof(midgaard_room));
   midgaard_room.vnum = MIDGAARD_CARAVAN_VNUM;

   memset(&kiess_room, 0, sizeof(kiess_room));
   memset(&kowloon_room, 0, sizeof(kowloon_room));
   memset(&mafdet_room, 0, sizeof(mafdet_room));
   memset(&rakuen_room, 0, sizeof(rakuen_room));

   test_npc_is_ignored();
   test_fighting_is_blocked();
   test_wrong_room_is_blocked();
   test_empty_argument_shows_usage();
   test_invalid_city_shows_usage();
   test_missing_cartography_blocks_travel();
   test_kiess_travel_with_cartography_done();
   test_kowloon_travel_with_cartography_done();
   test_mafdet_travel_with_cartography_done();
   test_rakuen_travel_with_cartography_done();
   test_partial_name_prefix_works();
   test_other_city_cartography_does_not_unlock_kiess();
   return 0;
}
