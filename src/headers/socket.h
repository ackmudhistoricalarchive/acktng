#ifndef DEC_SOCKET_H
#define DEC_SOCKET_H 1

/*
 * Public interface for socket.c — OS-dependent socket/network code.
 */

extern int global_port;
extern int global_ws_port;
extern int global_wss_port;
extern int global_tls_port;
extern int global_sniff_port;
extern int global_http_port;
extern const char echo_off_str[];
extern const char echo_on_str[];
extern const char go_ahead_str[];

/* Protocol telnet option numbers */
#define TELOPT_MSSP 70
#define TELOPT_MSDP 69
#define TELOPT_GMCP 201
#define TELOPT_MCCP2 86
#define TELOPT_MCCP3 87

/* MSSP byte markers */
#define MSSP_BYTE_VAR 1
#define MSSP_BYTE_VAL 2

/* MSDP byte markers */
#define MSDP_BYTE_VAR 1
#define MSDP_BYTE_VAL 2
#define MSDP_TABLE_OPEN 3
#define MSDP_TABLE_CLOSE 4
#define MSDP_ARRAY_OPEN 5
#define MSDP_ARRAY_CLOSE 6

/* MSDP variable subscription bitmask positions */
#define MSDP_BIT_CHARACTER_NAME (1U << 0)
#define MSDP_BIT_HEALTH (1U << 1)
#define MSDP_BIT_HEALTH_MAX (1U << 2)
#define MSDP_BIT_MANA (1U << 3)
#define MSDP_BIT_MANA_MAX (1U << 4)
#define MSDP_BIT_MOVEMENT (1U << 5)
#define MSDP_BIT_MOVEMENT_MAX (1U << 6)
#define MSDP_BIT_LEVEL (1U << 7)
#define MSDP_BIT_EXPERIENCE (1U << 8)
#define MSDP_BIT_EXPERIENCE_TNL (1U << 9)
#define MSDP_BIT_GOLD (1U << 10)
#define MSDP_BIT_ALIGNMENT (1U << 11)
#define MSDP_BIT_HITROLL (1U << 12)
#define MSDP_BIT_DAMROLL (1U << 13)
#define MSDP_BIT_AC (1U << 14)
#define MSDP_BIT_OPPONENT_NAME (1U << 16)
#define MSDP_BIT_OPPONENT_HEALTH (1U << 17)
#define MSDP_BIT_OPPONENT_HEALTH_MAX (1U << 18)
#define MSDP_BIT_OPPONENT_LEVEL (1U << 19)
#define MSDP_BIT_ROOM_NAME (1U << 20)
#define MSDP_BIT_ROOM_VNUM (1U << 21)
#define MSDP_BIT_ROOM_TERRAIN (1U << 22)
#define MSDP_BIT_ROOM_EXITS (1U << 23)
#define MSDP_BIT_ROOM_FLAGS (1U << 24)
#define MSDP_BIT_SERVER_ID (1U << 25)
#define MSDP_BIT_SERVER_TIME (1U << 26)
#define MSDP_BIT_WORLD_TIME (1U << 27)

/* GMCP package subscription bitmask */
#define GMCP_PKG_CHAR (1U << 0)
#define GMCP_PKG_ROOM (1U << 1)
#define GMCP_PKG_COMM (1U << 2)

void ws_send_room(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
void ws_send_map(DESCRIPTOR_DATA *d, CHAR_DATA *ch);
void ws_send_map_scan(DESCRIPTOR_DATA *d, CHAR_DATA *ch);

int init_socket(int port, unsigned long bind_addr);
#ifdef HAVE_OPENSSL
bool init_tls_context(const char *cert_file, const char *key_file);
#endif
void game_loop(int control, int control_ws, int control_tls, int control_sniff, int control_http,
               int control_wss);
void init_descriptor(DESCRIPTOR_DATA *dnew, int desc);
bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length);
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA *d);
void free_desc(DESCRIPTOR_DATA *d);
void queue_login_greeting(DESCRIPTOR_DATA *d);
void send_area_music(CHAR_DATA *ch);
void msdp_update(void);
void gmcp_update(void);
void gmcp_send_channel(DESCRIPTOR_DATA *d, const char *channel, const char *talker,
                       const char *text);
void init_mssp_counts(void);

#endif /* DEC_SOCKET_H */
