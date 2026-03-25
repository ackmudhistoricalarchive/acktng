#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"

char *get_adept_name(CHAR_DATA *ch);
int nocol_strlen(const char *text);
char *center_text(char *text, int width);

/* Minimal gclass_table: entries CLASS_GMA through CLASS_GMA+MAX_CLASS-1 are ADEPT */
const struct class_type gclass_table[MAX_TOTAL_CLASS] = {
    [CLASS_GMA + 0] = {.tier = ADEPT}, [CLASS_GMA + 1] = {.tier = ADEPT},
    [CLASS_GMA + 2] = {.tier = ADEPT}, [CLASS_GMA + 3] = {.tier = ADEPT},
    [CLASS_GMA + 4] = {.tier = ADEPT}, [CLASS_GMA + 5] = {.tier = ADEPT},
};

static void clear_character(CHAR_DATA *ch)
{
   memset(ch, 0, sizeof(*ch));
   ch->mortal_class[0] = ch->mortal_class[1] = ch->mortal_class[2] = ch->mortal_class[3] = -1;
   ch->remort_class[0] = ch->remort_class[1] = -1;
   ch->adept_class = -1;
}

static void test_nocol_strlen_strips_color_codes(void)
{
   assert(nocol_strlen("Hello") == 5);
   assert(nocol_strlen("@@bHello@@N") == 5);
   assert(nocol_strlen("@@r Red @@N text @@g") == 11);
   assert(nocol_strlen("") == 0);
}

static void test_get_adept_name_fallback_centered_is_18_visible(void)
{
   CHAR_DATA ch;
   clear_character(&ch);
   /* No adept levels set: returns generic fallback; centered to 18 must be 18 visible */
   char *name = get_adept_name(&ch);
   int visible = nocol_strlen(center_text(name, 18));
   if (visible != 18)
      printf("FAIL fallback centered: '%s' visible=%d (expected 18)\n", name, visible);
   assert(visible == 18);
}

static void test_get_adept_name_realm_lord_centered_is_18_visible(void)
{
   CHAR_DATA ch;
   clear_character(&ch);
   /* Set a level beyond MAX_ADEPT to trigger "Realm Lord" return */
   ch.adept_class = CLASS_GMA;
   ch.adept_level = MAX_ADEPT + 1;
   char *name = get_adept_name(&ch);
   int visible = nocol_strlen(center_text(name, 18));
   if (visible != 18)
      printf("FAIL realm lord centered: '%s' visible=%d (expected 18)\n", name, visible);
   assert(visible == 18);
}

static void test_get_adept_name_all_titles_centered_are_18_visible(void)
{
   CHAR_DATA ch;
   for (int cls = 0; cls < MAX_CLASS; cls++)
   {
      for (int level = 1; level <= MAX_ADEPT; level++)
      {
         clear_character(&ch);
         ch.adept_class = CLASS_GMA + cls;
         ch.adept_level = level;
         char *name = get_adept_name(&ch);
         /* Raw title should be shorter than 18 so center_text can pad it */
         int raw_visible = nocol_strlen(name);
         if (raw_visible >= 18)
            printf("FAIL class=%d level=%d: raw title '%s' visible=%d (must be < 18 to center)\n",
                   cls, level, name, raw_visible);
         assert(raw_visible < 18);
         /* After centering it must be exactly 18 visible chars */
         int centered_visible = nocol_strlen(center_text(name, 18));
         if (centered_visible != 18)
            printf("FAIL class=%d level=%d: '%s' centered visible=%d (expected 18)\n", cls, level,
                   name, centered_visible);
         assert(centered_visible == 18);
      }
   }
}

int main(void)
{
   test_nocol_strlen_strips_color_codes();
   test_get_adept_name_fallback_centered_is_18_visible();
   test_get_adept_name_realm_lord_centered_is_18_visible();
   test_get_adept_name_all_titles_centered_are_18_visible();
   puts("test_strfuns: all tests passed");
   return 0;
}
