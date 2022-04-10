#include <arpa/inet.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_signal.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/sys/dc_socket.h>
#include <errno.h>
#include <getopt.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

// local imports
#include "common.h"
#include "default_config.h"
#include "ncurses_client.h"
#include "network_util.h"
#include "types.h"
#include "clock_thread_ipc.h"
static volatile sig_atomic_t           exit_flag;

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->server_ip               = dc_setting_string_create(env, err);
    settings->server_hostname         = dc_setting_string_create(env, err);
    settings->server_udp_port         = dc_setting_uint16_create(env, err);
    settings->server_tcp_port         = dc_setting_uint16_create(env, err);
    struct options opts[]             = {{(struct dc_setting *)settings->opts.parent.config_path,
                              dc_options_set_path,
                              "config",
                              required_argument,
                              'c',
                              "CONFIG",
                              dc_string_from_string,
                              NULL,
                              dc_string_from_config,
                              NULL},
                             {(struct dc_setting *)settings->server_ip,
                              dc_options_set_string,
                              "server_ip",
                              required_argument,
                              'i',
                              "SERVER_IP",
                              dc_string_from_string,
                              "server_ip",
                              dc_string_from_config,
                              DEFAULT_IP_VERSION},
                             {(struct dc_setting *)settings->server_hostname,
                                 dc_options_set_string,
                                 "server_hostname",
                                 required_argument,
                                 'h',
                                 "SERVER_HOSTNAME",
                                 dc_string_from_string,
                                 "server_hostname",
                                 dc_string_from_config,
                                 DEFAULT_HOSTNAME},
                             {(struct dc_setting *)settings->server_udp_port,
                              dc_options_set_uint16,
                              "server_udp_port",
                              required_argument,
                              'u',
                              "SERVER_UDP_PORT",
                              dc_uint16_from_string,
                              "server_udp_port",
                              dc_uint16_from_config,
                              dc_uint16_from_string(env, err, DEFAULT_UDP_PORT)},
                             {(struct dc_setting *)settings->server_tcp_port,
                              dc_options_set_uint16,
                              "server_tcp_port",
                              required_argument,
                              't',
                              "SERVER_TCP_PORT",
                              dc_uint16_from_string,
                              "server_tcp_port",
                              dc_uint16_from_config,
                              dc_uint16_from_string(env, err, DEFAULT_TCP_PORT)}};

    // note the trick here - we use calloc and add 1 to ensure the last line is
    // all 0/NULL
    settings->opts.opts_count         = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size          = sizeof(struct options);
    settings->opts.opts               = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags      = "m:";
    settings->opts.env_prefix = "NCURSES_CLIENT_";

    return (struct dc_application_settings *)settings;
}

int create_udp_socket(const struct dc_posix_env *env, struct dc_error *err, const char *hostname, uint16_t server_udp_port,
                  const char *ip_version);

static int destroy_settings(const struct dc_posix_env               *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings         **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_string_destroy(env, &app_settings->server_ip);
    dc_setting_string_destroy(env, &app_settings->server_hostname);
    dc_setting_uint16_destroy(env, &app_settings->server_tcp_port);
    dc_setting_uint16_destroy(env, &app_settings->server_udp_port);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    dc_posix_tracer             tracer;
    dc_error_reporter           reporter;
    struct dc_posix_env         env;
    struct dc_error             err;
    struct dc_application_info *info;
    int                         ret_val;

    reporter = error_reporter;
    tracer   = NULL;
    //    tracer = trace_reporter;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "NCurses Client");

    struct sigaction sa;
    sa.sa_handler = &signal_handler;
    sa.sa_flags   = 0;
    dc_sigaction(&env, &err, SIGINT, &sa, NULL);
    dc_sigaction(&env, &err, SIGTERM, &sa, NULL);

    ret_val = dc_application_run(&env,
                                 &err,
                                 info,
                                 create_settings,
                                 destroy_settings,
                                 run,
                                 dc_default_create_lifecycle,
                                 dc_default_destroy_lifecycle,
                                 NULL,
                                 argc,
                                 argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);
    endwin();

    return ret_val;
}

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);

    app_settings          = (struct application_settings *)settings;

    // TCP
    int tcp_server_socket = connect_to_tcp_server(env,
                                                  err,
                                                  dc_setting_string_get(env, app_settings->server_hostname),
                                                  dc_setting_uint16_get(env, app_settings->server_tcp_port),
                                                  dc_setting_string_get(env, app_settings->server_ip));
    if(dc_error_has_error(err) || tcp_server_socket <= 0)
    {
        printf("could not connect to TCP socket\n");
        exit(1);
    }

    // UDP:
    int udp_server_sd = create_udp_socket(env,
                                          err,
                                          dc_setting_string_get(env, app_settings->server_hostname),
                                          dc_setting_uint16_get(env, app_settings->server_udp_port),
                                          dc_setting_string_get(env, app_settings->server_ip));

    // Clean buffers:
    uint8_t id_packet[2];
    size_t id_packet_len = 2;
    memset(id_packet, '\0', id_packet_len);

     // wait for server to give you an ID
    ssize_t bytes_recv = dc_read(env, err, tcp_server_socket, id_packet, id_packet_len);
    if(bytes_recv <= 0)
    {
        printf("could not get ID\n");
        exit(1);
    }

    // get id from server
    // char * tempID = strdup("001");
    uint16_t client_id = (uint16_t)(id_packet[0] | (uint16_t)id_packet[1] << 8);
    printf("client id %hu\n", client_id);

    // 100 ticks/s
//    const struct timeval tick_rate = {0, 10000};
//    struct timeval       timeout;
//    timeout.tv_sec                = tick_rate.tv_sec;
//    timeout.tv_usec               = tick_rate.tv_usec;

    ipc_path_pair ipcPathPair = {
            LISTEN_PATH_CLIENT,
            CLOCK_PATH_CLIENT
    };

    struct timespec tick_rate = {
            0,
            10000000
    };

    struct clock_thread_args clockThreadArgs = {
            &ipcPathPair,
            &tick_rate
    };
    //clock socket
    int clock_main_socket = create_unix_stream_socket(ipcPathPair.listen_path);

    pthread_t clock_thread;
    printf("Starting clock thread\n");
    pthread_create(&clock_thread, NULL, (void *(*)(void *)) clock_thread_socket, &clockThreadArgs);
    int clock_listen_socket = accept_ipc_connection(clock_main_socket);


    client_info *clientInfo       = calloc(1, sizeof(client_info));

    clientInfo->client_id         = client_id;
    clientInfo->udp_socket        = udp_server_sd;
    clientInfo->tcp_socket        = tcp_server_socket;
    clientInfo->has_client_entity = true;
    clientInfo->client_entity     = calloc(1, sizeof(client));

    pthread_t thread_id;
    printf("Starting display thread\n");
    pthread_create(&thread_id, NULL, (void *(*)(void *))ncurses_thread, clientInfo);
    // pthread_join(thread_id, NULL);

    fd_set readfds;
    while(!exit_flag)
    {
//        if(timeout.tv_usec == 0)
//        {
//            timeout.tv_usec = tick_rate.tv_usec;
//            //            printf("putting out packet %lu\n", clientInfo->last_packet_sent + 1);
//            send_game_state(clientInfo, udp_server_sd);
//            sent++;
//        }
        FD_ZERO(&readfds);
        FD_SET(udp_server_sd, &readfds);
        FD_SET(tcp_server_socket, &readfds);

        FD_SET(clock_listen_socket, &readfds);


        // FD_SET(STDIN_FILENO, &readfds);

        int maxfd = udp_server_sd;
        if(tcp_server_socket > udp_server_sd)
        {
            maxfd = udp_server_sd;
        }
        if (clock_listen_socket > maxfd) {
            maxfd = clock_listen_socket;
        }
        // printf("select\n");
        //    the big select statement
        if(select(maxfd + 1, &readfds, NULL, NULL, NULL) > 0)
        {
            //                        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            //                                exitFlag = true;
            //                        }

            if(FD_ISSET(clock_listen_socket, &readfds)) {
                send_game_state(clientInfo, udp_server_sd);
                //TODO: modify this to read as many times as necessary to clear the buffer
                read_packet_from_unix_socket(clock_listen_socket);

            }

            // check for udp messages
            if(FD_ISSET(udp_server_sd, &readfds))
            {
                receive_udp_packet(env, err, clientInfo);
            }

            // check for new client connections
            if(FD_ISSET(tcp_server_socket, &readfds))
            {
                exit_flag = true;
                // receive_tcp_packet(env, err, clientInfo, tcp_server_socket);
            }
        }
        else
        {
            // printf("select timed out\n");
        }
    }

    close(udp_server_sd);
    close(tcp_server_socket);

    return EXIT_SUCCESS;
}


static void receive_udp_packet(const struct dc_posix_env *env, struct dc_error *err, client_info *clientInfo)
{
    uint8_t header[12]         = {0};
    ssize_t header_size        = 12;
    ssize_t entity_packet_size = 6;
    ssize_t bullet_packet_size = 4;
    //    printf("recv udp packet\n");
    ssize_t count;
    count = recvfrom(clientInfo->udp_socket, header, (size_t)header_size, MSG_PEEK | MSG_WAITALL, NULL, NULL);
    // printf("count %zd\n", count);
    if(count < header_size)
    {
        // printf("packet not fully received\n");
        return;
    }

    uint64_t packet_no    = (uint64_t)((uint64_t)header[7] | (uint64_t)header[6] << 8 | (uint64_t)header[5] << 16 |
                                    (uint64_t)header[4] << 24 | (uint64_t)header[3] << 32 | (uint64_t)header[2] << 40 |
                                    (uint64_t)header[1] << 48 | (uint64_t)header[0] << 56);
    uint16_t num_entities = (uint16_t)(header[8] | (uint16_t)header[9] << 8);
    uint16_t num_bullets  = (uint16_t)(header[10] | (uint16_t)header[11] << 8);
    ssize_t  buffer_size  = num_entities * entity_packet_size + num_bullets * bullet_packet_size + header_size;
    uint8_t *buffer       = calloc(1, (size_t)buffer_size);
    count                 = recvfrom(clientInfo->udp_socket, buffer, (size_t)buffer_size, MSG_WAITALL, NULL, NULL);
    if(count < buffer_size)
    {
        free(buffer);
        printf("count not full %zd < %zd", count, buffer_size);
        return;
    }

    // printf("received packet %lu from the server with entities %hu\n",
    // packet_no, num_entities);
    if(packet_no <= clientInfo->last_packet_received)
    {
        // printf("old packet, discard\n");
        free(buffer);
        return;
    }

    free_client_entities_list(&clientInfo->client_entities);
    fill_entities_list(&clientInfo->client_entities, buffer + header_size, num_entities);
    free_bullet_list(&clientInfo->bulletList);
    fill_bullet_list(&clientInfo->bulletList, buffer + header_size + entity_packet_size * num_entities, num_bullets);
    free(buffer);
    // update_player_position(clientInfo);
    draw_game(clientInfo);
}

static void update_player_position(client_info *clientInfo)
{
    client_node *node = clientInfo->client_entities;
    //        while (node) {
    //                if (clientInfo->client_id == node->client_entity->client_id) {
    //                        clientInfo->client_entity = node->client_entity;
    //                }
    //                node = node->next;
    //        }
}

static void fill_entities_list(client_node **entities_list, const uint8_t *entity_buffer, uint16_t num_entities)
{
    size_t shift = 0;
    for(uint16_t entity_no = 0; entity_no < num_entities; ++entity_no)
    {
        client *new_entity     = calloc(1, sizeof(client));
        new_entity->client_id  = (uint16_t)(entity_buffer[0 + shift] | (uint16_t)entity_buffer[1 + shift] << 8);
        new_entity->position_x = (uint16_t)(entity_buffer[2 + shift] | (uint16_t)entity_buffer[3 + shift] << 8);
        new_entity->position_y = (uint16_t)(entity_buffer[4 + shift] | (uint16_t)entity_buffer[5 + shift] << 8);
        shift += 6;
        client_node *clientNode   = calloc(1, sizeof(client_node));
        clientNode->client_entity = new_entity;
        clientNode->next          = *entities_list;
        *entities_list            = clientNode;
    }
}

static void free_client_entities_list(client_node **head)
{
    client_node *node = *head;
    while(node)
    {
        free(node->client_entity);
        client_node *temp = node->next;
        free(node);
        node = temp;
    }
    *head = NULL;
}

static void fill_bullet_list(bullet_node **bullet_list, const uint8_t *entity_buffer, uint16_t num_bullets)
{
    size_t shift = 0;
    for(uint16_t entity_no = 0; entity_no < num_bullets; ++entity_no)
    {
        bullet *new_entity     = calloc(1, sizeof(bullet));
        new_entity->position_x = (uint16_t)(entity_buffer[0 + shift] | (uint16_t)entity_buffer[1 + shift] << 8);
        new_entity->position_y = (uint16_t)(entity_buffer[2 + shift] | (uint16_t)entity_buffer[3 + shift] << 8);
        shift += 4;
        bullet_node *bulletNode = calloc(1, sizeof(bullet_node));
        bulletNode->bullet      = new_entity;
        bulletNode->next        = *bullet_list;
        *bullet_list            = bulletNode;
    }
}

static void free_bullet_list(bullet_node **head)
{
    bullet_node *node = *head;
    while(node)
    {
        free(node->bullet);
        bullet_node *temp = node->next;
        free(node);
        node = temp;
    }
    *head = NULL;
}

static void send_game_state(client_info *clientInfo, int udp_socket)
{
    uint8_t packet[11];
    memset(packet, 0, 11);
    uint64_t packet_no = ++clientInfo->last_packet_sent;
    // FROM STACK OVERFLOW: https://stackoverflow.com/a/35153234
    // USER https://stackoverflow.com/users/3482801/straw1239
    for(int i = 0; i < 8; i++)
    {
        packet[i] = (uint8_t)((packet_no >> 8 * (7 - i)) & 0xFF);
    }
    packet[8] = clientInfo->client_id & 0xFF;
    packet[9] = clientInfo->client_id >> 8;

    if(clientInfo->client_entity)
    {
        clientInfo->clientInputState.move_down;
        uint8_t packed_byte = 0;
        packed_byte |= (clientInfo->clientInputState.move_up << 0);
        packed_byte |= (clientInfo->clientInputState.move_down << 1);
        packed_byte |= (clientInfo->clientInputState.move_left << 2);
        packed_byte |= (clientInfo->clientInputState.move_right << 3);
        packed_byte |= (clientInfo->clientInputState.shoot_up << 4);
        packed_byte |= (clientInfo->clientInputState.shoot_down << 5);
        packed_byte |= (clientInfo->clientInputState.shoot_left << 6);
        packed_byte |= (clientInfo->clientInputState.shoot_right << 7);
        packet[10] = packed_byte;
        // old for sending position from client
        //                packet[10] = clientInfo->client_entity->position_x & 0xFF;
        //                packet[11] = clientInfo->client_entity->position_x >> 8;
        //                packet[12] = clientInfo->client_entity->position_y & 0xFF;
        //                packet[13] = clientInfo->client_entity->position_y >> 8;
    }

    if(send(udp_socket, packet, 11, 0) < 0)
    {
        printf("Unable to send message\n");
        exit(1);
    }
    memset(&clientInfo->clientInputState, 0, sizeof(client_input_state));
}

static void error_reporter(const struct dc_error *err)
{
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter(__attribute__((unused)) const struct dc_posix_env *env,
                           const char                                        *file_name,
                           const char                                        *function_name,
                           size_t                                             line_number)
{
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}


void *ncurses_thread(client_info *clientInfo)
{
    printf("thread started\n");
    int key = 0;

    // THIS CODE IS MODIFIED FROM LIAM'S DEMO
    initscr();            /* initialize ncurses screen */
    noecho();             /* disable rendering text on input */
    keypad(stdscr, TRUE); /* allow input from special keys */
    curs_set(0);          /* make cursor invisible */
    cbreak();             /* enables intant input */
    //draw_game(clientInfo);

    // printw("\n");
    // mvaddch(1, 1, '0');
    // timeout(50);
    while(!exit_flag && (key = getch()) != EXIT_KEY)
    {
        // clear();
        // mvprintw(0, 20, "key: %c x: %d y: %d", key,
        // clientInfo->client_entity->position_x,
        // clientInfo->client_entity->position_y);

        switch(key)
        {
            case MOVE_UP:
                printw("up:");
                clientInfo->clientInputState.move_up = true;
                break;
            case MOVE_LEFT:
                clientInfo->clientInputState.move_left = true;
                break;
            case MOVE_DOWN:
                clientInfo->clientInputState.move_down = true;
                break;
            case MOVE_RIGHT:
                clientInfo->clientInputState.move_right = true;
                break;
            case KEY_UP:
                clientInfo->clientInputState.shoot_up = true;
                break;
            case KEY_LEFT:
                clientInfo->clientInputState.shoot_left = true;
                break;
            case KEY_DOWN:
                clientInfo->clientInputState.shoot_down = true;
                break;
            case KEY_RIGHT:
                clientInfo->clientInputState.shoot_right = true;
                break;
            default:
                break;
        }
        // draw_game(clientInfo);

        // mvaddch(clientInfo->client_entity->position_x,
        // clientInfo->client_entity->position_y, '0'); printw("\n");
    }

    endwin();

    //        const struct timeval tick_rate = {0, 10000};
    //        struct timeval t1, t2;
    //        long elapsed_us = 0;
    //        struct timespec sleepTime;
    //        while(true) {
    //                // start timer
    //                gettimeofday(&t1, NULL);
    //
    //
    //
    //
    //
    //                // stop timer
    //                gettimeofday(&t2, NULL);
    //                elapsed_us = (t2.tv_usec - t1.tv_usec);     // us to ms
    //                if (elapsed_us > tick_rate.tv_usec) {
    //                        // were late
    //                } else {
    //                        sleepTime.tv_nsec = (tick_rate.tv_usec - elapsed_us) * 1000;
    //                        nanosleep(&sleepTime, NULL);
    //                }
    //        }
    return 0;
}

void draw_game(client_info *clientInfo)
{
    clear();
    // draw some sort of map

    // draw the bullets
    bullet_node *bulletNode;
    bulletNode = clientInfo->bulletList;
    while(bulletNode)
    {
        mvaddch(bulletNode->bullet->position_x, bulletNode->bullet->position_y, '*');
        bulletNode = bulletNode->next;
    }

    // draw the client entities if there are any
    if(clientInfo->client_entities)
    {
        client_node *clientNode = clientInfo->client_entities;

        while(clientNode)
        {
            uint16_t row = clientNode->client_entity->position_x;
            uint16_t col = clientNode->client_entity->position_y;
            // printf("entity @ %hu %hu", row, col);
            // move(clientNode->client_entity->position_y,
            // clientNode->client_entity->position_x);
            if(clientNode->client_entity->client_id == clientInfo->client_id)
            {
                attron(COLOR_PAIR(COLOR_GREEN));
                mvaddch(row, col, '0');
                attroff(COLOR_PAIR(COLOR_GREEN));
            }
            else
            {
                attron(COLOR_PAIR(COLOR_RED));
                mvaddch(row, col, '0');
                attroff(COLOR_PAIR(COLOR_RED));
            }

            clientNode = clientNode->next;
        }
    }
    refresh();
}

void move_player(client *client_entity, int direction_x, int direction_y)
{
    if((client_entity->position_x == 0 && direction_x < 0) || (client_entity->position_x == 50 && direction_x > 0) ||
       (client_entity->position_y == 0 && direction_y < 0) || (client_entity->position_y == 50 && direction_y > 0))
    {
        return;
    }
    client_entity->position_x += direction_x;
    client_entity->position_y += direction_y;
}

void signal_handler(__attribute__((unused)) int signnum)
{
    printf("\nexit flag set\n");
    exit_flag = 1;
}
