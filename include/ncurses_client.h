//
// Created by drep on 2022-04-01.
//

#ifndef STIRLING_GAME_DEMO_NCURSES_CLIENT_H
#define STIRLING_GAME_DEMO_NCURSES_CLIENT_H

typedef struct sockaddr_in sockaddr_in;

struct application_settings
{
    struct dc_opt_settings    opts;
    struct dc_setting_string *server_ip;
    struct dc_setting_string *server_hostname;
    struct dc_setting_uint16 *server_udp_port;
    struct dc_setting_uint16 *server_tcp_port;
};

void                                   signal_handler(__attribute__((unused)) int signnum);

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);

static int
destroy_settings(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings **psettings);

static int  run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);

static void error_reporter(const struct dc_error *err);

static void
trace_reporter(const struct dc_posix_env *env, const char *file_name, const char *function_name, size_t line_number);

size_t get_time_difference(int future_hour, int future_minute, int current_hour, int current_min, int current_seconds);

static void send_game_state(client_info *clientInfo, int udp_socket);

static void receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, client_info *clientInfo);

static void free_client_entities_list(client_node **head);

static void fill_entities_list(client_node **entities_list, const uint8_t *entity_buffer, uint16_t num_entities);

static void update_player_position(client_info *clientInfo);

void       *ncurses_thread(client_info *clientInfo);

void        move_player(client *client_entity, int direction_x, int direction_y);

void        draw_game(client_info *clientInfo);

static void free_bullet_list(bullet_node **head);

static void fill_bullet_list(bullet_node **bullet_list, const uint8_t *entity_buffer, uint16_t num_bullets);

#endif    // STIRLING_GAME_DEMO_NCURSES_CLIENT_H
