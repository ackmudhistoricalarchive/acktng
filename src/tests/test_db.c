#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#define DEC_GLOBALS_H 1
#include "ack.h"

void db_test_format_status(char *dest, size_t dest_size, const char *prefix, const char *file_name);
void db_test_shuffle_maze_room_exits(ROOM_INDEX_DATA *pRoomIndex);
long db_test_parse_lore_flags(const char *s);

void init_mm(void);

time_t current_time;
FILE *fpReserve = NULL;

void hang(const char *str)
{
   fprintf(stderr, "hang called in test_db: %s\n", str != NULL ? str : "(null)");
   abort();
}
void db_test_set_area_name(const char *file_name);
const char *db_test_get_area_name(void);
int object_spawn_level(int prototype_level, int requested_level);
int count_file_line(FILE *fp);

static int area_validation_failure_count = 0;
static jmp_buf area_validation_jmp;
static int area_validation_jmp_active = 0;

static void test_object_spawn_level_uses_requested_level_when_present(void)
{
   assert(object_spawn_level(45, 12) == 12);
   assert(object_spawn_level(45, 1) == 1);
}

static void test_object_spawn_level_falls_back_to_prototype_level(void)
{
   assert(object_spawn_level(45, 0) == 45);
   assert(object_spawn_level(45, -3) == 45);
}
static void test_format_status_builds_expected_message(void)
{
   char buf[64];

   db_test_format_status(buf, sizeof(buf), "Loading", "foo.dat");
   assert(strcmp(buf, "Loading foo.dat") == 0);
}

static void test_set_area_name_truncates_and_terminates(void)
{
   char long_name[MAX_STRING_LENGTH + 64];

   memset(long_name, 'a', sizeof(long_name) - 1);
   long_name[sizeof(long_name) - 1] = '\0';

   db_test_set_area_name(long_name);

   assert(db_test_get_area_name()[MAX_INPUT_LENGTH - 1] == '\0');
   assert(strlen(db_test_get_area_name()) == MAX_INPUT_LENGTH - 1);
}

static FILE *create_temp_file(const char *content, size_t len)
{
   FILE *fp = tmpfile();
   assert(fp != NULL);
   fwrite(content, 1, len, fp);
   return fp;
}

static void test_count_file_line_normal_file(void)
{
   /* 3 lines with trailing newline */
   const char *content = "line1\nline2\nline3\n";
   FILE *fp = create_temp_file(content, strlen(content));

   /* Seek to start of line 3 (offset 12) */
   fseek(fp, 12, SEEK_SET);
   assert(count_file_line(fp) == 2);

   /* Seek to start of line 2 (offset 6) */
   fseek(fp, 6, SEEK_SET);
   assert(count_file_line(fp) == 1);

   /* Seek to start of file */
   fseek(fp, 0, SEEK_SET);
   assert(count_file_line(fp) == 0);

   fclose(fp);
}

static void test_count_file_line_no_trailing_newline(void)
{
   /* File with no trailing newline — the bug that caused an infinite loop */
   const char *content = "line1\nline2\n#$";
   FILE *fp = create_temp_file(content, strlen(content));

   /* Seek to the '#' of '#$' (offset 12) */
   fseek(fp, 12, SEEK_SET);
   assert(count_file_line(fp) == 2);

   /* Seek to end of file */
   fseek(fp, (long)strlen(content), SEEK_SET);
   /* Must terminate, not hang */
   count_file_line(fp);

   fclose(fp);
}

static void test_fread_number_ull_reads_large_values(void)
{
   FILE *fp = tmpfile();
   assert(fp != NULL);

   fputs("4294967296 ", fp);
   rewind(fp);
   assert(fread_number_ull(fp) == 4294967296ULL);

   fclose(fp);
}

static void test_fread_number_ull_supports_or_syntax(void)
{
   FILE *fp = tmpfile();
   assert(fp != NULL);

   fputs("1|4294967296 ", fp);
   rewind(fp);
   assert(fread_number_ull(fp) == 4294967297ULL);

   fclose(fp);
}

static void test_count_file_line_single_line_no_newline(void)
{
   const char *content = "#$";
   FILE *fp = create_temp_file(content, strlen(content));

   fseek(fp, 1, SEEK_SET);
   assert(count_file_line(fp) == 0);

   fclose(fp);
}

static void test_maze_room_exits_are_valid_permutation_after_shuffle(void)
{
   ROOM_INDEX_DATA room;
   EXIT_DATA exits[6];
   EXIT_DATA *orig[6];
   int i, j;

   memset(&room, 0, sizeof(room));
   memset(exits, 0, sizeof(exits));
   room.room_flags = ROOM_MAZE;

   for (i = 0; i < 6; i++)
   {
      room.exit[i] = &exits[i];
      orig[i] = &exits[i];
   }

   current_time = 1;
   init_mm();
   db_test_shuffle_maze_room_exits(&room);

   /* Each original exit pointer must appear exactly once after shuffle */
   for (i = 0; i < 6; i++)
   {
      int count = 0;
      for (j = 0; j < 6; j++)
      {
         if (room.exit[j] == orig[i])
            count++;
      }
      assert(count == 1);
   }
}

static void test_maze_room_exits_change_across_resets(void)
{
   ROOM_INDEX_DATA room;
   EXIT_DATA exits[6];
   EXIT_DATA *orig[6];
   int i, iter, changed;

   memset(&room, 0, sizeof(room));
   memset(exits, 0, sizeof(exits));
   room.room_flags = ROOM_MAZE;

   for (i = 0; i < 6; i++)
   {
      room.exit[i] = &exits[i];
      orig[i] = &exits[i];
   }

   current_time = 42;
   init_mm();

   changed = 0;
   for (iter = 0; iter < 100 && !changed; iter++)
   {
      /* Reset to original order before each trial */
      for (i = 0; i < 6; i++)
         room.exit[i] = orig[i];

      db_test_shuffle_maze_room_exits(&room);

      for (i = 0; i < 6; i++)
      {
         if (room.exit[i] != orig[i])
         {
            changed = 1;
            break;
         }
      }
   }
   assert(changed);
}

static void test_parse_lore_flags_single(void)
{
   assert(db_test_parse_lore_flags("MIDGAARD") == LORE_FLAG_MIDGAARD);
   assert(db_test_parse_lore_flags("KIESS") == LORE_FLAG_KIESS);
   assert(db_test_parse_lore_flags("KOWLOON") == LORE_FLAG_KOWLOON);
   assert(db_test_parse_lore_flags("RAKUEN") == LORE_FLAG_RAKUEN);
   assert(db_test_parse_lore_flags("MAFDET") == LORE_FLAG_MAFDET);
   assert(db_test_parse_lore_flags("HUMAN") == LORE_FLAG_HUMAN);
   assert(db_test_parse_lore_flags("KHENARI") == LORE_FLAG_KHENARI);
   assert(db_test_parse_lore_flags("KHEPHARI") == LORE_FLAG_KHEPHARI);
   assert(db_test_parse_lore_flags("ASHBORN") == LORE_FLAG_ASHBORN);
   assert(db_test_parse_lore_flags("UMBRAL") == LORE_FLAG_UMBRAL);
   assert(db_test_parse_lore_flags("RIVENNID") == LORE_FLAG_RIVENNID);
   assert(db_test_parse_lore_flags("DELTARI") == LORE_FLAG_DELTARI);
   assert(db_test_parse_lore_flags("USHABTI") == LORE_FLAG_USHABTI);
   assert(db_test_parse_lore_flags("SERATHI") == LORE_FLAG_SERATHI);
   assert(db_test_parse_lore_flags("KETHARI") == LORE_FLAG_KETHARI);
}

static void test_parse_lore_flags_multiple(void)
{
   long flags = db_test_parse_lore_flags("MIDGAARD KIESS");
   assert(flags == (LORE_FLAG_MIDGAARD | LORE_FLAG_KIESS));

   flags = db_test_parse_lore_flags("KOWLOON RAKUEN MAFDET");
   assert(flags == (LORE_FLAG_KOWLOON | LORE_FLAG_RAKUEN | LORE_FLAG_MAFDET));

   flags = db_test_parse_lore_flags("MIDGAARD HUMAN");
   assert(flags == (LORE_FLAG_MIDGAARD | LORE_FLAG_HUMAN));

   flags = db_test_parse_lore_flags("KIESS SERATHI KETHARI");
   assert(flags == (LORE_FLAG_KIESS | LORE_FLAG_SERATHI | LORE_FLAG_KETHARI));
}

static void test_parse_lore_flags_case_insensitive(void)
{
   assert(db_test_parse_lore_flags("midgaard") == LORE_FLAG_MIDGAARD);
   assert(db_test_parse_lore_flags("Kiess") == LORE_FLAG_KIESS);
   assert(db_test_parse_lore_flags("human") == LORE_FLAG_HUMAN);
   assert(db_test_parse_lore_flags("Serathi") == LORE_FLAG_SERATHI);
}

static void test_parse_lore_flags_empty(void)
{
   assert(db_test_parse_lore_flags("") == 0);
   assert(db_test_parse_lore_flags("\n") == 0);
}

int main(void)
{
   test_object_spawn_level_uses_requested_level_when_present();
   test_object_spawn_level_falls_back_to_prototype_level();
   test_format_status_builds_expected_message();
   test_set_area_name_truncates_and_terminates();
   test_count_file_line_normal_file();
   test_fread_number_ull_reads_large_values();
   test_fread_number_ull_supports_or_syntax();
   test_count_file_line_no_trailing_newline();
   test_count_file_line_single_line_no_newline();
   test_maze_room_exits_are_valid_permutation_after_shuffle();
   test_maze_room_exits_change_across_resets();
   test_parse_lore_flags_single();
   test_parse_lore_flags_multiple();
   test_parse_lore_flags_case_insensitive();
   test_parse_lore_flags_empty();

   puts("test_db: all tests passed");
   return 0;
}
