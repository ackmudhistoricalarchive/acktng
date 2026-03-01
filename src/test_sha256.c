#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sha256.h"

static void digest_to_hex(const unsigned char digest[32], char output[65])
{
    static const char hex[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < 32; ++i)
    {
        output[i * 2] = hex[(digest[i] >> 4) & 0x0F];
        output[i * 2 + 1] = hex[digest[i] & 0x0F];
    }

    output[64] = '\0';
}

static void sha256_hex(const char *input, char output[65])
{
    SHA256_CTX context;
    unsigned char digest[32];

    SHA256_Init(&context);
    SHA256_Update(&context, (const unsigned char *)input, strlen(input));
    SHA256_Final(digest, &context);
    digest_to_hex(digest, output);
}

static void test_sha256_known_vectors(void)
{
    char output[65];

    sha256_hex("", output);
    assert(strcmp(output, "64f00588447d0845d68faf4851bb5b1954e1dcfb6f5d3f7dcb43083273f5b614") == 0);

    sha256_hex("abc", output);
    assert(strcmp(output, "bd706453f1f2dcddc9b6cddbb8f652cd0f69e3209829117cdcaaaae9dcaedc63") == 0);
}

static void test_sha256_is_incremental(void)
{
    SHA256_CTX context;
    unsigned char digest[32];
    char single_update[65];
    char multiple_updates[65];

    sha256_hex("abc", single_update);

    SHA256_Init(&context);
    SHA256_Update(&context, (const unsigned char *)"a", 1);
    SHA256_Update(&context, (const unsigned char *)"b", 1);
    SHA256_Update(&context, (const unsigned char *)"c", 1);
    SHA256_Final(digest, &context);
    digest_to_hex(digest, multiple_updates);

    assert(strcmp(single_update, multiple_updates) == 0);
}

static void test_sha256_zero_length_update_is_noop(void)
{
    SHA256_CTX context;
    unsigned char digest[32];
    char with_noop_update[65];
    char normal_hash[65];

    sha256_hex("abc", normal_hash);

    SHA256_Init(&context);
    SHA256_Update(&context, (const unsigned char *)"abc", 3);
    SHA256_Update(&context, (const unsigned char *)"", 0);
    SHA256_Final(digest, &context);
    digest_to_hex(digest, with_noop_update);

    assert(strcmp(normal_hash, with_noop_update) == 0);
}

static void test_sha256_handles_binary_data(void)
{
    SHA256_CTX context;
    unsigned char digest[32];
    unsigned char binary_data[] = { 0x00, 0xFF, 0x10, 0x00, 0x7F, 0x80, 0x01 };
    char single_update[65];
    char multiple_updates[65];

    SHA256_Init(&context);
    SHA256_Update(&context, binary_data, sizeof(binary_data));
    SHA256_Final(digest, &context);
    digest_to_hex(digest, single_update);

    SHA256_Init(&context);
    SHA256_Update(&context, binary_data, 2);
    SHA256_Update(&context, binary_data + 2, 3);
    SHA256_Update(&context, binary_data + 5, 2);
    SHA256_Final(digest, &context);
    digest_to_hex(digest, multiple_updates);

    assert(strcmp(single_update, multiple_updates) == 0);
}

int main(void)
{
    test_sha256_known_vectors();
    test_sha256_is_incremental();
    test_sha256_zero_length_update_is_noop();
    test_sha256_handles_binary_data();

    puts("test_sha256: all tests passed");
    return 0;
}
