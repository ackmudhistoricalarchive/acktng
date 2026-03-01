#include <assert.h>
#include <stdio.h>
#include <string.h>

#define UNIT_TEST_SAVE
#include "save.c"

static void test_cap_nocol_capitalizes_and_lowercases(void)
{
    assert(strcmp(cap_nocol("aLReAdY"), "Already") == 0);
    assert(strcmp(cap_nocol("tEST name"), "Test name") == 0);
}

static void test_cap_nocol_handles_empty_string(void)
{
    assert(strcmp(cap_nocol(""), "") == 0);
}

static void test_cap_nocol_reuses_static_buffer(void)
{
    char *first;
    char *second;

    first = cap_nocol("fIRST");
    second = cap_nocol("sECOND");

    assert(first == second);
    assert(strcmp(second, "Second") == 0);
}

int main(void)
{
    test_cap_nocol_capitalizes_and_lowercases();
    test_cap_nocol_handles_empty_string();
    test_cap_nocol_reuses_static_buffer();

    puts("test_save: all tests passed");
    return 0;
}
