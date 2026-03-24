#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*
 * Copies of JSON helpers from socket.c — must be kept in sync.
 */
static void json_append(char *buf, int *pos, int buf_size, const char *s)
{
   while (*s && *pos < buf_size - 1)
      buf[(*pos)++] = *s++;
   buf[*pos] = '\0';
}

static void json_str_escape_raw(char *buf, int *pos, int buf_size, const char *s, int len)
{
   int i;
   json_append(buf, pos, buf_size, "\"");
   for (i = 0; i < len && *pos < buf_size - 8; i++)
   {
      unsigned char c = (unsigned char)s[i];
      if (c == '"')
         json_append(buf, pos, buf_size, "\\\"");
      else if (c == '\\')
         json_append(buf, pos, buf_size, "\\\\");
      else if (c == '\n')
         json_append(buf, pos, buf_size, "\\n");
      else if (c == '\t')
         json_append(buf, pos, buf_size, "\\t");
      else if (c == '\r')
         ; /* strip carriage return */
      else if (c < 0x20 || c == 0x7f)
      {
         char esc[8];
         snprintf(esc, sizeof(esc), "\\u%04x", (unsigned int)c);
         json_append(buf, pos, buf_size, esc);
      }
      else
      {
         buf[(*pos)++] = (char)c;
         buf[*pos] = '\0';
      }
   }
   json_append(buf, pos, buf_size, "\"");
}

/* Copy of json_str_escape() from socket.c — must be kept in sync. */
static void json_str_escape(char *buf, int *pos, int buf_size, const char *s)
{
   json_append(buf, pos, buf_size, "\"");
   while (*s && *pos < buf_size - 3)
   {
      char c = *s++;
      if (c == '"' || c == '\\')
      {
         buf[(*pos)++] = '\\';
      }
      buf[(*pos)++] = c;
   }
   buf[*pos] = '\0';
   json_append(buf, pos, buf_size, "\"");
}

/* ---- helpers ---- */

static void escape(const char *input, char *out, size_t out_cap)
{
   int pos = 0;
   json_str_escape_raw(out, &pos, (int)out_cap, input, (int)strlen(input));
   out[pos] = '\0';
}

static void escape_simple(const char *input, char *out, size_t out_cap)
{
   int pos = 0;
   json_str_escape(out, &pos, (int)out_cap, input);
   out[pos] = '\0';
}

/* ---- tests ---- */

static void test_plain_ascii(void)
{
   char out[64];
   escape("hello world", out, sizeof(out));
   assert(strcmp(out, "\"hello world\"") == 0);
}

static void test_quote_escaped(void)
{
   char out[64];
   escape("say \"hi\"", out, sizeof(out));
   assert(strcmp(out, "\"say \\\"hi\\\"\"") == 0);
}

static void test_backslash_escaped(void)
{
   char out[64];
   escape("a\\b", out, sizeof(out));
   assert(strcmp(out, "\"a\\\\b\"") == 0);
}

static void test_newline_escaped(void)
{
   char out[64];
   escape("line1\nline2", out, sizeof(out));
   assert(strcmp(out, "\"line1\\nline2\"") == 0);
}

static void test_tab_escaped(void)
{
   char out[64];
   escape("col1\tcol2", out, sizeof(out));
   assert(strcmp(out, "\"col1\\tcol2\"") == 0);
}

static void test_cr_stripped(void)
{
   char out[64];
   escape("a\rb", out, sizeof(out));
   assert(strcmp(out, "\"ab\"") == 0);
}

static void test_ansi_esc_escaped(void)
{
   /* ESC (\x1b) should become \u001b */
   char out[64];
   escape("\x1b[32mgreen\x1b[0m", out, sizeof(out));
   assert(strcmp(out, "\"\\u001b[32mgreen\\u001b[0m\"") == 0);
}

static void test_other_control_char(void)
{
   /* BEL (\x07) should become \u0007 */
   char out[32];
   escape("\x07", out, sizeof(out));
   assert(strcmp(out, "\"\\u0007\"") == 0);
}

static void test_del_char(void)
{
   /* DEL (\x7f) should become \u007f */
   char out[32];
   escape("\x7f", out, sizeof(out));
   assert(strcmp(out, "\"\\u007f\"") == 0);
}

static void test_empty_string(void)
{
   char out[16];
   escape("", out, sizeof(out));
   assert(strcmp(out, "\"\"") == 0);
}

static void test_buffer_does_not_overflow(void)
{
   /* Small output buffer: result must still be valid (no overrun, NUL-terminated). */
   char out[12];
   int pos = 0;
   /* "hello world" is 11 chars; with quotes needs 13 — won't fit, but must not crash. */
   json_str_escape_raw(out, &pos, (int)sizeof(out), "hello world", 11);
   assert(pos < (int)sizeof(out));
   assert(out[pos] == '\0');
   /* Must start with opening quote */
   assert(out[0] == '"');
}

/* ---- json_str_escape tests ---- */

static void test_simple_escape_plain(void)
{
   char out[32];
   escape_simple("hello", out, sizeof(out));
   assert(strcmp(out, "\"hello\"") == 0);
}

static void test_simple_escape_quote(void)
{
   char out[32];
   escape_simple("say \"hi\"", out, sizeof(out));
   assert(strcmp(out, "\"say \\\"hi\\\"\"") == 0);
}

static void test_simple_escape_backslash(void)
{
   char out[32];
   escape_simple("a\\b", out, sizeof(out));
   assert(strcmp(out, "\"a\\\\b\"") == 0);
}

static void test_simple_escape_closing_quote_fits(void)
{
   /* Buffer exactly tight: opening " + content + closing " must all fit.
    * "ab" → \"ab\" needs 6 bytes; use buf of 8 — closing quote must appear. */
   char out[8];
   int pos = 0;
   json_str_escape(out, &pos, (int)sizeof(out), "ab");
   assert(pos < (int)sizeof(out));
   assert(out[pos] == '\0');
   assert(out[0] == '"');
   /* Last non-NUL byte must be the closing quote. */
   assert(pos >= 2);
   assert(out[pos - 1] == '"');
}

static void test_simple_escape_closing_quote_fits_at_escape_boundary(void)
{
   /* Regression: with the old -2 bound an escaped char at the boundary could
    * leave the closing quote unwritten.  Verify the closing quote is always
    * present even when an escape pair lands right at the buffer edge.
    *
    * Input: 3 chars "a\"b", needs \"a\\\"b\" = 8 bytes + NUL.
    * Use buf of 9 so the escape pair just barely fits — closing quote must appear. */
   char out[9];
   int pos = 0;
   json_str_escape(out, &pos, (int)sizeof(out), "a\"b");
   assert(pos < (int)sizeof(out));
   assert(out[pos] == '\0');
   assert(out[0] == '"');
   assert(out[pos - 1] == '"');
}

int main(void)
{
   test_plain_ascii();
   test_quote_escaped();
   test_backslash_escaped();
   test_newline_escaped();
   test_tab_escaped();
   test_cr_stripped();
   test_ansi_esc_escaped();
   test_other_control_char();
   test_del_char();
   test_empty_string();
   test_buffer_does_not_overflow();

   test_simple_escape_plain();
   test_simple_escape_quote();
   test_simple_escape_backslash();
   test_simple_escape_closing_quote_fits();
   test_simple_escape_closing_quote_fits_at_escape_boundary();

   puts("test_websocket_json_escape: all tests passed");
   return 0;
}
