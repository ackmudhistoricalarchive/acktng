#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define LINE_MAX_LEN 4096

typedef struct vnum_node VNUM_NODE;

struct vnum_node
{
    int vnum;
    VNUM_NODE *next;
};


static int read_line(FILE *fp, char *line, int *line_number);
static const char *skip_space(const char *s);
static int starts_with(const char *line, const char *prefix);
static int is_int_token(const char *s, char **end);

static void fail_area_test(const char *area_path, int line_number, const char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "area format test failure in %s", area_path != NULL ? area_path : "<unknown area>");
    if (line_number > 0)
        fprintf(stderr, " at line %d", line_number);
    fprintf(stderr, ": ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
    exit(1);
}

static int vnum_set_contains(const VNUM_NODE *set, int vnum)
{
    while (set != NULL)
    {
        if (set->vnum == vnum)
            return 1;
        set = set->next;
    }
    return 0;
}

static void vnum_set_add(VNUM_NODE **set, int vnum)
{
    VNUM_NODE *node;

    if (vnum_set_contains(*set, vnum))
        return;

    node = malloc(sizeof(*node));
    if (node == NULL)
    {
        fprintf(stderr, "out of memory while building vnum set\n");
        exit(1);
    }

    node->vnum = vnum;
    node->next = *set;
    *set = node;
}

static void vnum_set_free(VNUM_NODE *set)
{
    while (set != NULL)
    {
        VNUM_NODE *next = set->next;
        free(set);
        set = next;
    }
}

static int parse_int_tokens(const char *line, int *out_values, int max_values)
{
    const char *cursor = skip_space(line);
    char *end;
    int count = 0;

    while (count < max_values && is_int_token(cursor, &end))
    {
        out_values[count++] = (int)strtol(cursor, &end, 10);
        cursor = skip_space(end);
    }

    return count;
}

static int is_blank_or_comment(const char *line)
{
    const char *trimmed = skip_space(line);

    return trimmed[0] == '\0' || trimmed[0] == '\r' || trimmed[0] == '\n' || trimmed[0] == '*';
}


static void add_unique_vnum(VNUM_NODE **set, int vnum, const char *area_path, int line_number, const char *kind)
{
    if (vnum_set_contains(*set, vnum))
        fail_area_test(area_path, line_number, "duplicate %s vnum %d across loaded area files", kind, vnum);

    vnum_set_add(set, vnum);
}

static void scan_area_indexes(const char *area_path, VNUM_NODE **global_room_vnums, VNUM_NODE **global_mobile_vnums, VNUM_NODE **global_object_vnums)
{
    FILE *fp = fopen(area_path, "r");
    char line[LINE_MAX_LEN];
    int line_number = 0;
    enum
    {
        SCAN_NONE,
        SCAN_ROOMS,
        SCAN_MOBILES,
        SCAN_OBJECTS
    } section = SCAN_NONE;

    if (fp == NULL)
        fail_area_test(area_path, 0, "unable to open area file while scanning index vnums");

    while (read_line(fp, line, &line_number))
    {
        const char *trimmed = skip_space(line);

        if (starts_with(trimmed, "#MOBILES"))
        {
            section = SCAN_MOBILES;
            continue;
        }

        if (starts_with(trimmed, "#OBJECTS"))
        {
            section = SCAN_OBJECTS;
            continue;
        }

        if (starts_with(trimmed, "#ROOMS"))
        {
            section = SCAN_ROOMS;
            continue;
        }

        if (section == SCAN_NONE)
            continue;

        if (starts_with(trimmed, "#0"))
        {
            section = SCAN_NONE;
            continue;
        }

        if (trimmed[0] == '#' && isdigit((unsigned char)trimmed[1]))
        {
            int vnum = atoi(trimmed + 1);
            if (section == SCAN_ROOMS)
                add_unique_vnum(global_room_vnums, vnum, area_path, line_number, "room");
            else if (section == SCAN_MOBILES)
                add_unique_vnum(global_mobile_vnums, vnum, area_path, line_number, "mobile");
            else if (section == SCAN_OBJECTS)
                add_unique_vnum(global_object_vnums, vnum, area_path, line_number, "object");
        }
    }

    fclose(fp);
}

static int read_line(FILE *fp, char *line, int *line_number)
{
    if (fgets(line, LINE_MAX_LEN, fp) == NULL)
        return 0;
    (*line_number)++;
    return 1;
}

static const char *skip_space(const char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
        s++;
    return s;
}

static int starts_with(const char *line, const char *prefix)
{
    line = skip_space(line);
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

static int is_int_token(const char *s, char **end)
{
    long value;

    s = skip_space(s);
    if (*s == '\0' || *s == '\r' || *s == '\n')
        return 0;

    value = strtol(s, end, 10);
    (void)value;
    return *end != s;
}

static int parse_exact_n_ints(const char *line, int count)
{
    const char *cursor = skip_space(line);
    char *end = NULL;
    int parsed = 0;

    while (parsed < count)
    {
        if (!is_int_token(cursor, &end))
            return 0;
        parsed++;
        cursor = skip_space(end);
    }

    cursor = skip_space(cursor);
    return *cursor == '\0' || *cursor == '\r' || *cursor == '\n';
}

static int parse_at_least_n_ints(const char *line, int count)
{
    const char *cursor = skip_space(line);
    char *end = NULL;
    int parsed = 0;

    while (parsed < count)
    {
        if (!is_int_token(cursor, &end))
            return 0;
        parsed++;
        cursor = skip_space(end);
    }

    return 1;
}

static void parse_mobile_loot_extension(const char *line, int line_number, const char *area_path)
{
    const char *trimmed = skip_space(line);

    if (trimmed[0] == 'l')
    {
        if (!parse_exact_n_ints(trimmed + 1, MAX_LOOT + 1))
            fail_area_test(area_path, line_number, "mobile 'l' line must contain loot_amount plus %d loot vnums", MAX_LOOT);
        return;
    }

    if (trimmed[0] == 'L')
    {
        if (!parse_exact_n_ints(trimmed + 1, MAX_LOOT))
            fail_area_test(area_path, line_number, "mobile 'L' line must contain exactly %d loot chance values", MAX_LOOT);
        return;
    }
}

static void require_inline_tilde_terminated_string(FILE *fp, char *line, int *line_number, const char *area_path,
                                                 const char *field_name)
{
    if (!read_line(fp, line, line_number))
        fail_area_test(area_path, *line_number, "unexpected EOF while reading mobile %s", field_name);

    if (strchr(line, '~') == NULL)
        fail_area_test(area_path, *line_number, "mobile %s must end with '~' on the same line", field_name);
}

static void consume_tilde_string(FILE *fp, char *line, int *line_number, const char *area_path)
{
    do
    {
        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF while reading ~-terminated string");
    } while (strchr(line, '~') == NULL);
}

static void parse_area_section(FILE *fp, char *line, int *line_number, const char *area_path)
{
    const char *trimmed;

    consume_tilde_string(fp, line, line_number, area_path);

    while (read_line(fp, line, line_number))
    {
        trimmed = skip_space(line);
        if (trimmed[0] == '#')
        {
            if (fseek(fp, -(long)strlen(line), SEEK_CUR) != 0)
                fail_area_test(area_path, *line_number, "unable to rewind while parsing #AREA");
            (*line_number)--;
            return;
        }

        if (trimmed[0] == '\0' || trimmed[0] == '\r' || trimmed[0] == '\n')
            continue;

        if (trimmed[0] == '*')
            continue;


        if (strchr("QKNILVXFUORWPTBSM", trimmed[0]) == NULL)
            fail_area_test(area_path, *line_number, "invalid #AREA directive '%c'", trimmed[0]);
    }
}

static void parse_mobiles_section(FILE *fp, char *line, int *line_number, const char *area_path)
{
    for (;;)
    {
        const char *trimmed;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside #MOBILES");

        trimmed = skip_space(line);
        if (is_blank_or_comment(trimmed))
            continue;
        if (starts_with(trimmed, "#0"))
            return;

        if (trimmed[0] != '#' || !isdigit((unsigned char)trimmed[1]))
            fail_area_test(area_path, *line_number, "expected mobile vnum '#<n>' or '#0'");

        require_inline_tilde_terminated_string(fp, line, line_number, area_path, "name");
        require_inline_tilde_terminated_string(fp, line, line_number, area_path, "short description");
        consume_tilde_string(fp, line, line_number, area_path);
        consume_tilde_string(fp, line, line_number, area_path);

        if (!read_line(fp, line, line_number) || strchr(skip_space(line), 'S') == NULL)
            fail_area_test(area_path, *line_number, "mobile stats header missing trailing 'S'");
        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 2))
            fail_area_test(area_path, *line_number, "mobile level/sex line must contain 2 integers");
        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 4))
            fail_area_test(area_path, *line_number, "mobile modifier line must contain 4 integers");

        for (;;)
        {
            char marker;
            if (!read_line(fp, line, line_number))
                fail_area_test(area_path, *line_number, "unexpected EOF inside mobile body");
            trimmed = skip_space(line);
            if (is_blank_or_comment(trimmed))
                continue;
            marker = trimmed[0];

            if (marker == '#')
            {
                if (fseek(fp, -(long)strlen(line), SEEK_CUR) != 0)
                    fail_area_test(area_path, *line_number, "unable to rewind while parsing mobiles");
                (*line_number)--;
                break;
            }

            if (marker == '!' || marker == '|' || marker == '+')
                continue;

            if (marker == 'l' || marker == 'L')
            {
                parse_mobile_loot_extension(trimmed, *line_number, area_path);
                continue;
            }

            if (marker == '>')
            {
                if (strchr(trimmed, '~') == NULL)
                    fail_area_test(area_path, *line_number, "inline mobprog header must end with '~'");
                consume_tilde_string(fp, line, line_number, area_path);
                if (!read_line(fp, line, line_number) || skip_space(line)[0] != '|')
                    fail_area_test(area_path, *line_number, "inline mobprog must terminate with '|'");
                continue;
            }

            fail_area_test(area_path, *line_number, "unexpected token inside mobile body: %.20s", trimmed);
        }
    }
}

static void parse_objects_section(FILE *fp, char *line, int *line_number, const char *area_path)
{
    for (;;)
    {
        const char *trimmed;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside #OBJECTS");

        trimmed = skip_space(line);
        if (is_blank_or_comment(trimmed))
            continue;
        if (starts_with(trimmed, "#0"))
            return;

        if (trimmed[0] != '#' || !isdigit((unsigned char)trimmed[1]))
            fail_area_test(area_path, *line_number, "expected object vnum '#<n>' or '#0'");

        consume_tilde_string(fp, line, line_number, area_path);
        consume_tilde_string(fp, line, line_number, area_path);
        consume_tilde_string(fp, line, line_number, area_path);

        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 4))
            fail_area_test(area_path, *line_number, "object type/flags line must contain 4 integers");
        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 10))
            fail_area_test(area_path, *line_number, "object values line must contain 10 integers");
        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 1))
            fail_area_test(area_path, *line_number, "object weight line must contain 1 integer");

        for (;;)
        {
            if (!read_line(fp, line, line_number))
                fail_area_test(area_path, *line_number, "unexpected EOF inside object body");
            trimmed = skip_space(line);
            if (is_blank_or_comment(trimmed))
                continue;

            if (trimmed[0] != 'A' && trimmed[0] != 'E' && trimmed[0] != 'L')
            {
                if (fseek(fp, -(long)strlen(line), SEEK_CUR) != 0)
                    fail_area_test(area_path, *line_number, "unable to rewind while parsing objects");
                (*line_number)--;
                break;
            }

            if (trimmed[0] == 'A')
            {
                if (parse_exact_n_ints(trimmed + 1, 2))
                    continue;

                if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 2))
                    fail_area_test(area_path, *line_number, "object affect must contain location and modifier");
            }
            else if (trimmed[0] == 'E')
            {
                consume_tilde_string(fp, line, line_number, area_path);
                consume_tilde_string(fp, line, line_number, area_path);
            }
            else
            {
                if (parse_exact_n_ints(trimmed + 1, 1))
                    continue;

                if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 1))
                    fail_area_test(area_path, *line_number, "object level marker must be followed by integer level");
            }
        }
    }
}

static void consume_exit_keyword_string(FILE *fp, char *line, int *line_number, const char *area_path)
{
    for (;;)
    {
        const char *trimmed;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF while reading room exit keyword");

        trimmed = skip_space(line);

        if ((trimmed[0] == 'D' && trimmed[1] >= '0' && trimmed[1] <= '5') || trimmed[0] == 'E' || trimmed[0] == 'S' || trimmed[0] == '#')
            fail_area_test(area_path, *line_number, "room exit keyword missing '~' terminator before room token");

        if (parse_exact_n_ints(trimmed, 2) || parse_exact_n_ints(trimmed, 3))
            fail_area_test(area_path, *line_number, "room exit keyword missing '~' terminator before numeric exit data");

        if (strchr(line, '~') != NULL)
            return;
    }
}

static void parse_rooms_section(FILE *fp, char *line, int *line_number, const char *area_path)
{
    int saw_room = 0;

    for (;;)
    {
        const char *trimmed;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside #ROOMS");

        trimmed = skip_space(line);
        if (is_blank_or_comment(trimmed))
            continue;
        if (starts_with(trimmed, "#0"))
            break;

        if (trimmed[0] != '#' || !isdigit((unsigned char)trimmed[1]))
            fail_area_test(area_path, *line_number, "expected room vnum '#<n>' or '#0'");

        saw_room = 1;

        consume_tilde_string(fp, line, line_number, area_path);
        consume_tilde_string(fp, line, line_number, area_path);

        if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 2))
            fail_area_test(area_path, *line_number, "room flags line must contain 2 integers");

        for (;;)
        {
            if (!read_line(fp, line, line_number))
                fail_area_test(area_path, *line_number, "unexpected EOF while reading room entries");
            trimmed = skip_space(line);
            if (is_blank_or_comment(trimmed))
                continue;

            if (trimmed[0] == 'S')
                break;

            if (trimmed[0] == 'D')
            {
                int door = trimmed[1] - '0';
                if (trimmed[1] < '0' || trimmed[1] > '5')
                    fail_area_test(area_path, *line_number, "room exit door must be D0..D5");
                (void)door;
                consume_tilde_string(fp, line, line_number, area_path);
                consume_exit_keyword_string(fp, line, line_number, area_path);
                if (!read_line(fp, line, line_number) || !parse_exact_n_ints(line, 3))
                    fail_area_test(area_path, *line_number, "exit destination line must contain exactly 3 integers");

                continue;
            }

            if (trimmed[0] == 'E')
            {
                consume_tilde_string(fp, line, line_number, area_path);
                consume_tilde_string(fp, line, line_number, area_path);
                continue;
            }

            fail_area_test(area_path, *line_number, "room entry token must be D/E/S");
        }
    }

    if (!saw_room)
        fail_area_test(area_path, *line_number, "#ROOMS section must contain at least one room");
}

static void parse_shops_section(FILE *fp, char *line, int *line_number, const char *area_path)
{
    for (;;)
    {
        const char *trimmed;
        char *end;
        int count = 0;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside #SHOPS");

        trimmed = skip_space(line);
        if (is_blank_or_comment(trimmed))
            continue;
        if (parse_exact_n_ints(trimmed, 1) && atoi(trimmed) == 0)
            return;

        while (is_int_token(trimmed, &end))
        {
            count++;
            trimmed = skip_space(end);
        }

        if (count < 8)
            fail_area_test(area_path, *line_number, "shop line must contain at least 8 integers");
    }
}

static void parse_specials_section(FILE *fp, char *line, int *line_number, const char *area_path, const char *allowed_leads)
{
    for (;;)
    {
        const char *trimmed;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside specials/object-functions section");

        trimmed = skip_space(line);
        if (trimmed[0] == 'S')
            return;
        if (is_blank_or_comment(trimmed))
            continue;
        if (strchr(allowed_leads, trimmed[0]) == NULL)
            fail_area_test(area_path, *line_number, "invalid section line: %.20s", trimmed);

    }
}

static void parse_resets_section(FILE *fp, char *line, int *line_number, const char *area_path, const VNUM_NODE *global_room_vnums, int area_min_vnum, int area_max_vnum)
{
    for (;;)
    {
        const char *trimmed;
        char cmd;

        if (!read_line(fp, line, line_number))
            fail_area_test(area_path, *line_number, "unexpected EOF inside #RESETS");

        trimmed = skip_space(line);
        cmd = trimmed[0];
        if (cmd == '\0' || cmd == '\r' || cmd == '\n' || cmd == '*')
            continue;
        if (cmd == 'S')
            return;

        if (strchr("MOGEDRAP", cmd) == NULL)
            fail_area_test(area_path, *line_number, "unknown reset command '%c'", cmd);

        {
            int values[5];
            int value_count = parse_int_tokens(trimmed + 1, values, 5);

            if ((cmd == 'M' || cmd == 'O' || cmd == 'P') && value_count >= 4)
            {
                int room_vnum = values[3];
                if (room_vnum >= area_min_vnum && room_vnum <= area_max_vnum && !vnum_set_contains(global_room_vnums, room_vnum))
                    fail_area_test(area_path, *line_number, "reset '%c' references unknown room vnum %d", cmd, room_vnum);
            }
            else if ((cmd == 'D' || cmd == 'R') && value_count >= 2)
            {
                int room_vnum = values[1];
                if (room_vnum >= area_min_vnum && room_vnum <= area_max_vnum && !vnum_set_contains(global_room_vnums, room_vnum))
                    fail_area_test(area_path, *line_number, "reset '%c' references unknown room vnum %d", cmd, room_vnum);
            }
        }

        if (cmd == 'G' || cmd == 'R')
        {
            if (!parse_at_least_n_ints(trimmed + 1, 3))
                fail_area_test(area_path, *line_number, "G/R resets must have command+ifflag+arg1+arg2 (4 integers total)");
        }
        else
        {
            char *end;
            int n = 0;
            const char *cursor = trimmed + 1;
            while (is_int_token(cursor, &end) && n < 5)
            {
                n++;
                cursor = skip_space(end);
            }
            if (n < 4)
                fail_area_test(area_path, *line_number, "reset must provide ifflag arg1 arg2 arg3");
        }
    }
}


static void read_area_vnum_range(const char *area_path, int *min_vnum, int *max_vnum)
{
    FILE *fp = fopen(area_path, "r");
    char line[LINE_MAX_LEN];
    int line_number = 0;

    *min_vnum = -1;
    *max_vnum = -1;

    if (fp == NULL)
        fail_area_test(area_path, 0, "unable to open area file while reading area vnum range");

    while (read_line(fp, line, &line_number))
    {
        const char *trimmed = skip_space(line);
        if (trimmed[0] == 'V')
        {
            int values[2];
            if (parse_int_tokens(trimmed + 1, values, 2) == 2)
            {
                *min_vnum = values[0];
                *max_vnum = values[1];
                break;
            }
        }

        if (starts_with(trimmed, "#ROOMS"))
            break;
    }

    fclose(fp);
}

static void assert_area_matches_spec(const char *area_path, const VNUM_NODE *global_room_vnums, int area_min_vnum, int area_max_vnum)
{
    FILE *fp = fopen(area_path, "r");
    char line[LINE_MAX_LEN];
    int line_number = 0;
    int saw_rooms = 0;
    int saw_area = 0;
    int saw_eof = 0;

    if (fp == NULL)
        fail_area_test(area_path, 0, "unable to open area file");

    while (read_line(fp, line, &line_number))
    {
        const char *trimmed = skip_space(line);

        if (trimmed[0] != '#')
            continue;

        if (starts_with(trimmed, "#$"))
        {
            saw_eof = 1;
            break;
        }

        if (starts_with(trimmed, "#AREA"))
        {
            saw_area = 1;
            parse_area_section(fp, line, &line_number, area_path);
            continue;
        }
        if (starts_with(trimmed, "#MOBILES"))
        {
            parse_mobiles_section(fp, line, &line_number, area_path);
            continue;
        }
        if (starts_with(trimmed, "#MOBPROGS"))
        {
            parse_specials_section(fp, line, &line_number, area_path, "M");
            continue;
        }
        if (starts_with(trimmed, "#OBJECTS"))
        {
            parse_objects_section(fp, line, &line_number, area_path);
            continue;
        }
        if (starts_with(trimmed, "#ROOMS"))
        {
            saw_rooms = 1;
            parse_rooms_section(fp, line, &line_number, area_path);
            continue;
        }
        if (starts_with(trimmed, "#SHOPS"))
        {
            parse_shops_section(fp, line, &line_number, area_path);
            continue;
        }
        if (starts_with(trimmed, "#SPECIALS"))
        {
            parse_specials_section(fp, line, &line_number, area_path, "MO");
            continue;
        }
        if (starts_with(trimmed, "#OBJFUNS"))
        {
            parse_specials_section(fp, line, &line_number, area_path, "O");
            continue;
        }
        if (starts_with(trimmed, "#RESETS"))
        {
            parse_resets_section(fp, line, &line_number, area_path, global_room_vnums, area_min_vnum, area_max_vnum);
            continue;
        }

        fail_area_test(area_path, line_number, "unrecognized section header: %.40s", trimmed);
    }

    fclose(fp);

    if (!saw_area)
        fail_area_test(area_path, 0, "missing required #AREA section");
    if (!saw_rooms)
        fail_area_test(area_path, 0, "missing required #ROOMS section");
    if (!saw_eof)
        fail_area_test(area_path, line_number, "missing required #$ end-of-file marker");
}


int main(void)
{
    FILE *list_fp = fopen("../area/area.lst", "r");
    char area_name[1024];
    VNUM_NODE *global_room_vnums = NULL;
    VNUM_NODE *global_mobile_vnums = NULL;
    VNUM_NODE *global_object_vnums = NULL;

    if (list_fp == NULL)
        list_fp = fopen("area/area.lst", "r");

    if (list_fp == NULL)
        fail_area_test("area.lst", 0, "unable to open area list");

    while (fgets(area_name, sizeof(area_name), list_fp) != NULL)
    {
        char *nl = strpbrk(area_name, "\r\n");
        char area_path[2048];

        if (nl != NULL)
            *nl = '\0';

        if (area_name[0] == '\0')
            continue;
        if (strcmp(area_name, "$") == 0)
            break;

        FILE *area_fp;

        snprintf(area_path, sizeof(area_path), "../area/%s", area_name);
        area_fp = fopen(area_path, "r");
        if (area_fp == NULL)
        {
            snprintf(area_path, sizeof(area_path), "area/%s", area_name);
            area_fp = fopen(area_path, "r");
        }
        if (area_fp == NULL)
            fail_area_test(area_name, 0, "listed area file could not be opened");
        fclose(area_fp);

        scan_area_indexes(area_path, &global_room_vnums, &global_mobile_vnums, &global_object_vnums);
    }

    rewind(list_fp);

    while (fgets(area_name, sizeof(area_name), list_fp) != NULL)
    {
        char *nl = strpbrk(area_name, "\r\n");
        char area_path[2048];

        if (nl != NULL)
            *nl = '\0';

        if (area_name[0] == '\0')
            continue;
        if (strcmp(area_name, "$") == 0)
            break;

        int area_min_vnum;
        int area_max_vnum;
        FILE *area_fp;

        snprintf(area_path, sizeof(area_path), "../area/%s", area_name);
        area_fp = fopen(area_path, "r");
        if (area_fp == NULL)
        {
            snprintf(area_path, sizeof(area_path), "area/%s", area_name);
            area_fp = fopen(area_path, "r");
        }
        if (area_fp == NULL)
            fail_area_test(area_name, 0, "listed area file could not be opened");
        fclose(area_fp);

        read_area_vnum_range(area_path, &area_min_vnum, &area_max_vnum);
        assert_area_matches_spec(area_path, global_room_vnums, area_min_vnum, area_max_vnum);
    }

    fclose(list_fp);
    vnum_set_free(global_room_vnums);
    vnum_set_free(global_mobile_vnums);
    vnum_set_free(global_object_vnums);
    puts("test_area_format: all tests passed");
    return 0;
}
