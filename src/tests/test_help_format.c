#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

static int has_invalid_filename_chars(const char *name)
{
   const unsigned char *p = (const unsigned char *)name;
   for (; *p; p++)
   {
      if (!(isalnum(*p) || *p == '.' || *p == '_' || *p == '-'))
         return 1;
   }
   return 0;
}

static void validate_keywords(const char *keywords)
{
   size_t keyword_len = strlen(keywords);
   assert(*keywords != '\0' && *keywords != '\n' && *keywords != '\r');
   while (keyword_len > 0 &&
          (keywords[keyword_len - 1] == '\n' || keywords[keyword_len - 1] == '\r'))
   {
      keyword_len--;
   }
   assert(keyword_len > 0);
   assert(!isspace((unsigned char)keywords[0]));
   assert(!isspace((unsigned char)keywords[keyword_len - 1]));
}

static void validate_help_entry_header(FILE *fp, char *line, size_t line_size)
{
   long level;
   char extra;

   assert(sscanf(line, "level %ld %c", &level, &extra) == 1);
   (void)level;

   assert(fgets(line, line_size, fp) != NULL);
   assert(strncmp(line, "keywords ", 9) == 0);
   validate_keywords(line + 9);

   assert(fgets(line, line_size, fp) != NULL);
   if (strncmp(line, "flags ", 6) == 0)
   {
      assert(strlen(line) > 6);
      assert(fgets(line, line_size, fp) != NULL);
   }
   assert(strcmp(line, "---\n") == 0 || strcmp(line, "---\r\n") == 0 || strcmp(line, "---") == 0);
}

static void validate_help_file(const char *path)
{
   FILE *fp = fopen(path, "r");
   char line[8192];
   size_t i;

   assert(fp != NULL);

   /* Read entry header (level, keywords, optional flags, separator) */
   assert(fgets(line, sizeof(line), fp) != NULL);
   validate_help_entry_header(fp, line, sizeof(line));

   /* Read text lines (help/shelp are single-entry) */
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      i = strlen(line);
      assert(i < sizeof(line) - 1 || line[i - 1] == '\n');
   }

   fclose(fp);
}

static void validate_lore_entry_header(FILE *fp, char *line, size_t line_size)
{
   /* line already contains "keywords ..." */
   assert(strncmp(line, "keywords ", 9) == 0);
   validate_keywords(line + 9);

   assert(fgets(line, line_size, fp) != NULL);
   if (strncmp(line, "flags ", 6) == 0)
   {
      assert(strlen(line) > 6);
      assert(fgets(line, line_size, fp) != NULL);
   }
   assert(strcmp(line, "---\n") == 0 || strcmp(line, "---\r\n") == 0 || strcmp(line, "---") == 0);
}

static void validate_lore_file(const char *path)
{
   FILE *fp = fopen(path, "r");
   char line[8192];
   size_t i;

   assert(fp != NULL);

   /* First line must be keywords (appears only once, at top of file) */
   assert(fgets(line, sizeof(line), fp) != NULL);
   assert(strncmp(line, "keywords ", 9) == 0);
   validate_keywords(line + 9);

   /* Optional flags line, then required separator */
   assert(fgets(line, sizeof(line), fp) != NULL);
   if (strncmp(line, "flags ", 6) == 0)
   {
      assert(strlen(line) > 6);
      assert(fgets(line, sizeof(line), fp) != NULL);
   }
   assert(strcmp(line, "---\n") == 0 || strcmp(line, "---\r\n") == 0 || strcmp(line, "---") == 0);

   /* Read text lines, watching for new entries starting with "flags " */
   while (fgets(line, sizeof(line), fp) != NULL)
   {
      if (strncmp(line, "flags ", 6) == 0)
      {
         assert(strlen(line) > 6);
         assert(fgets(line, sizeof(line), fp) != NULL);
         assert(strcmp(line, "---\n") == 0 || strcmp(line, "---\r\n") == 0 ||
                strcmp(line, "---") == 0);
         continue;
      }
      /* keywords must not repeat after the first line */
      assert(strncmp(line, "keywords ", 9) != 0);
      /* No color codes allowed in lore files */
      assert(strstr(line, "@@") == NULL);
      i = strlen(line);
      assert(i < sizeof(line) - 1 || line[i - 1] == '\n');
   }

   fclose(fp);
}

static int validate_directory(const char *directory, void (*validator)(const char *))
{
   DIR *dir = opendir(directory);
   struct dirent *entry;
   int files_seen = 0;

   assert(dir != NULL);

   while ((entry = readdir(dir)) != NULL)
   {
      char path[4096];
      const char *name = entry->d_name;
      size_t len;

      if (name[0] == '.')
         continue;

      len = strlen(name);
      assert(len > 0);
      if (name[len - 1] == '~')
         continue;

      snprintf(path, sizeof(path), "%s%s", directory, name);

      {
         struct stat st;
         if (stat(path, &st) == 0 && !S_ISREG(st.st_mode))
            continue;
      }

      assert(has_invalid_filename_chars(name) == 0);

      validator(path);
      files_seen++;
   }

   closedir(dir);
   return files_seen;
}

int main(void)
{
   int help_files = validate_directory(HELP_DIR, validate_help_file);
   int shelp_files = validate_directory(SHELP_DIR, validate_help_file);
   int lore_files = validate_directory(LORE_DIR, validate_lore_file);

   assert(help_files > 0);
   assert(shelp_files > 0);
   assert(lore_files > 0);

   return 0;
}
