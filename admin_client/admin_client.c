#include "admin_client.h"
#include "ncurses_client.h"

int main(int argc, char *argv[])
{
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = trace_reporter;
    tracer = NULL;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "Game Client");

    struct sigaction sa;
    sa.sa_handler = &signal_handler;
    sa.sa_flags = 0;
    dc_sigaction(&env, &err, SIGINT, &sa, NULL);
    dc_sigaction(&env, &err, SIGTERM, &sa, NULL);


    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err)
{
    struct admin_application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct admin_application_settings));

    if(settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->verbose       = dc_setting_bool_create(env, err);
    settings->hostname      = dc_setting_string_create(env, err);
    settings->port          = dc_setting_uint16_create(env, err);

    struct options opts[] = {
            {(struct dc_setting *)settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->verbose,
                    dc_options_set_bool,
                    "start_time",
                    required_argument,
                    's',
                    "START_TIME",
                    dc_string_from_string,
                    "start_time",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->hostname,
                    dc_options_set_string,
                    "host",
                    required_argument,
                    'h',
                    "HOST",
                    dc_string_from_string,
                    "host",
                    dc_string_from_config,
                    DEFAULT_HOSTNAME},
            {(struct dc_setting *)settings->port,
                    dc_options_set_uint16,
                    "port",
                    required_argument,
                    'p',
                    "PORT",
                    dc_uint16_from_string,
                    "port",
                    dc_uint16_from_config,
                    dc_uint16_from_string(env, err, DEFAULT_TCP_PORT_ADMIN_SERVER)},
                };

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "m:";
    settings->opts.env_prefix = "DC_EXAMPLE_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings)
{
    struct admin_application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct admin_application_settings *)*psettings;
    dc_setting_bool_destroy(env, &app_settings->verbose);
    dc_setting_string_destroy(env, &app_settings->hostname);
    dc_setting_uint16_destroy(env, &app_settings->port);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct admin_application_settings));

    if(env->null_free)
    {
        *psettings = NULL;
    }

    return 0;
}

uint8_t parseAdminCommand(const struct dc_posix_env *env, struct dc_error *err, char buffer[MAX_BUFFER_SIZE]) {
    uint8_t command;

    if (dc_strcmp(env, buffer, "/stop\n") == 0) {
        command = STOP;
    } else if (dc_strcmp(env, buffer, "/users") == 0) {
        command = USERS;
    } else if (dc_strcmp(env, buffer, "/kick") == 0) {
        command = KICK;
    } else if (dc_strcmp(env, buffer, "/warn") == 0) {
        command = WARN;
    } else if (dc_strcmp(env, buffer, "/notice") == 0) {
        command = NOTICE;
    } else {
        command = NOT_RECOGNIZED;
    }

    return command;
}

admin_client_packet * create_client_packet(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message) {
    admin_client_packet * clientPacket = NULL;
    clientPacket = dc_calloc(env, err, 1, sizeof(admin_client_packet));

    if (dc_error_has_error(err)) {
        fprintf(stderr, "Error: \"%s\" - %s : %s : %d @ %zu\n", err->message, err->file_name, err->function_name, err->errno_code, err->line_number);
        dc_exit(env, 1);
    }

    clientPacket->version = ADMIN_PROTOCOL_VERSION;
    clientPacket->command = (uint8_t) command;
    clientPacket->target_client_id = 0;
    if (message) {
        clientPacket->message_length = (uint16_t) dc_strlen(env, message);
        clientPacket->message = dc_strdup(env, err, message);
    } else {
        clientPacket->message_length = 0;
        clientPacket->message = NULL;
    }

    return clientPacket;
}

int serialize_client_packet(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet * clientPacket, uint8_t **output_buffer, size_t *size)
{
    size_t header_size = 4;
    uint8_t client_header[header_size];
//    uint8_t client_header[8];

    client_header[0] = clientPacket->version;
    client_header[1] = clientPacket->command;
    client_header[2] = clientPacket->target_client_id & 0xFF;
    client_header[3] = clientPacket->target_client_id >> 8;
//    client_header[4] = clientPacket->message_length & 0xFF;
//    client_header[5] = clientPacket->message_length >> 8;
//    client_header[6] = clientPacket->message & 0xFF;
//    client_header[7] = clientPacket->message >> 8;

    *output_buffer = dc_calloc(env, err, (sizeof(client_header)) + clientPacket->message_length, sizeof(uint8_t));
    if (dc_error_has_error(err)) {
        fprintf(stderr, "Error: \"%s\" - %s : %s : %d @ %zu\n", err->message, err->file_name, err->function_name, err->errno_code, err->line_number);
        dc_exit(env, 1);
    }
    dc_memcpy(env, *output_buffer, client_header, sizeof(client_header));
    dc_memcpy(env, *output_buffer + sizeof(client_header), clientPacket->message, clientPacket->message_length);

    *size = (size_t) (sizeof(client_header) + clientPacket->message_length);

    return 0;
}



void send_admin_client_message(const struct dc_posix_env *env, struct dc_error *err, enum ADMIN_COMMANDS command, char *message, int tcp_server_socket) {
    admin_client_packet *clientPacket;
    uint8_t *output_buffer = NULL;
    size_t packetSize = 0;

    clientPacket = create_client_packet(env, err, command, message);

    serialize_client_packet(env, err, clientPacket, &output_buffer, &packetSize);

    dc_write(env, err, tcp_server_socket, output_buffer, packetSize);
    dc_free(env, output_buffer, packetSize);

        if (clientPacket->message) {
        dc_free(env, clientPacket->message, dc_strlen(env, clientPacket->message) + 1);
    }
    dc_free(env, clientPacket, sizeof(admin_client_packet));


}

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    struct admin_application_settings *app_settings;
    const char *hostname;
    uint16_t port;
    char buffer[MAX_BUFFER_SIZE];
    uint8_t command;

    DC_TRACE(env);

    app_settings = (struct admin_application_settings *)settings;
    hostname = dc_setting_string_get(env, app_settings->hostname);
    port = dc_setting_uint16_get(env, app_settings->port);

    //TCP
    int tcp_server_socket = connect_to_tcp_server(env, err, hostname, port, DEFAULT_IP_VERSION);
    if (dc_error_has_error(err) || tcp_server_socket <= 0) {
        printf("could not connect to TCP socket\n");
        dc_exit(env, 1);
    }

    // Clean buffers:
    char server_message[2000];
    char client_message[2000];
    dc_memset(env, server_message, '\0', sizeof(server_message));
    dc_memset(env, client_message, '\0', sizeof(client_message));
    bool exitFlag = false;

    fd_set readfds;
    while (!exit_flag) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(tcp_server_socket, &readfds);

        int maxfd = tcp_server_socket;

        if(select(maxfd + 1, &readfds, NULL, NULL, NULL) > 0){
            if(FD_ISSET(STDIN_FILENO, &readfds)) {

                // To use messages, we will need to use strtok or similar to parse the buffer for additional input.
                dc_read(env, err, STDIN_FILENO, buffer, sizeof(buffer));
                command = parseAdminCommand(env, err, buffer);
                if (command == NOT_RECOGNIZED) {
                    printf("Command not recognized\n");
                } else {
                    send_admin_client_message(env, err, command, NULL, tcp_server_socket);
                }
            }

            // check for new client connections
            if (FD_ISSET(tcp_server_socket, &readfds)) {

            }

        }
    }
    dc_close(env, err, tcp_server_socket);

    return EXIT_SUCCESS;
}



static void error_reporter(const struct dc_error *err)
{
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
    fprintf(stderr, "ERROR: %s\n", err->message);
}

static void trace_reporter(__attribute__((unused)) const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number)
{
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}



void signal_handler(__attribute__ ((unused)) int signnum) {
    printf("\nexit flag set\n");
    exit_flag = 1;
    //exit(0);
}










