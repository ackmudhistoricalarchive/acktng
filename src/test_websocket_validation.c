#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#define DEC_GLOBALS_H 1
#include "ack.h"

bool validate_websocket_handshake_request(const char *request, char *key_out, size_t key_out_size, size_t *header_len_out);

int get_max_hp(CHAR_DATA *ch) { (void)ch; return 100; }
int get_max_mana(CHAR_DATA *ch) { (void)ch; return 100; }
int get_max_move(CHAR_DATA *ch) { (void)ch; return 100; }

static void test_accepts_valid_request(void)
{
    char key[64];
    size_t header_len = 0;
    const char *request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:4000\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n"
        "look\n";

    assert(validate_websocket_handshake_request(request, key, sizeof(key), &header_len));
    assert(strcmp(key, "dGhlIHNhbXBsZSBub25jZQ==") == 0);
    assert(header_len == (size_t)(strstr(request, "\r\n\r\n") - request + 4));
}

static void test_rejects_non_get_or_incomplete(void)
{
    char key[64];
    size_t header_len = 0;

    assert(!validate_websocket_handshake_request("POST / HTTP/1.1\r\n\r\n", key, sizeof(key), &header_len));
    assert(!validate_websocket_handshake_request("GET / HTTP/1.1\r\nHost: local\r\n", key, sizeof(key), &header_len));
}

static void test_rejects_missing_required_headers(void)
{
    char key[64];
    const char *missing_upgrade =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Sec-WebSocket-Key: abc123==\r\n"
        "\r\n";
    const char *missing_key =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Upgrade: websocket\r\n"
        "\r\n";

    assert(!validate_websocket_handshake_request(missing_upgrade, key, sizeof(key), NULL));
    assert(!validate_websocket_handshake_request(missing_key, key, sizeof(key), NULL));
}

static void test_rejects_oversized_key_buffer(void)
{
    char tiny_key[8];
    const char *request =
        "GET / HTTP/1.1\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Key: this-key-is-way-too-long\r\n"
        "\r\n";

    assert(!validate_websocket_handshake_request(request, tiny_key, sizeof(tiny_key), NULL));
}

int main(void)
{
    test_accepts_valid_request();
    test_rejects_non_get_or_incomplete();
    test_rejects_missing_required_headers();
    test_rejects_oversized_key_buffer();

    puts("test_websocket_validation: all tests passed");
    return 0;
}
