#include "admin_client.h"
#include "ncurses_client.h"
#include "network_util.h"

static volatile sig_atomic_t           exit_flag;

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

void write_log_to_console(const struct dc_posix_env *env, struct dc_error *err) {
    int adminLogFD;
    FILE *adminLogFileDescriptor;
    char *logStorage = NULL;
    size_t lineSize = 0;

    adminLogFD = dc_open(env, err, "../../adminLogs/admin_client_log.txt", O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    adminLogFileDescriptor = dc_fdopen(env, err, adminLogFD, "r");

    dc_write(env, err, STDOUT_FILENO, "Displaying User List Log\n", dc_strlen(env, "Displaying User List Log\n"));

    dc_write(env, err, STDOUT_FILENO, "----------------------------------\n", dc_strlen(env, "----------------------------------\n"));

    while(dc_getline(env, err, &logStorage, &lineSize, adminLogFileDescriptor) > 0) {
        dc_write(env, err, STDOUT_FILENO, logStorage, dc_strlen(env, logStorage));
        dc_memset(env, logStorage, 0, sizeof(logStorage));
    }
    dc_free(env, logStorage, lineSize);

    dc_write(env, err, STDOUT_FILENO, "----------------------------------\n", dc_strlen(env, "----------------------------------\n"));
}

uint8_t parseAdminCommand(const struct dc_posix_env *env, struct dc_error *err, char buffer[MAX_BUFFER_SIZE], volatile sig_atomic_t * exitFlag) {
    uint8_t enumCommand;
    char *charCommand;
    char *commandString;
    char *endPointer;

    commandString = dc_strdup(env, err, buffer);

    dc_strtok_r(env, commandString, "\n", &endPointer);
    charCommand = dc_strdup(env, err, dc_strtok_r(env, commandString, " ", &endPointer));

    if (dc_strcmp(env, charCommand, "/stop") == 0) {
        enumCommand = STOP;
        *exitFlag = true;
    } else if (dc_strcmp(env, charCommand, "/users") == 0) {
        enumCommand = USERS;
    } else if (dc_strcmp(env, charCommand, "/kick") == 0) {
        enumCommand = KICK;
    } else if (dc_strcmp(env, charCommand, "/warn") == 0) {
        enumCommand = WARN;
    } else if (dc_strcmp(env, charCommand, "/notice") == 0) {
        enumCommand = NOTICE;
    } else if (dc_strcmp(env, charCommand, "/quit") == 0) {
        *exitFlag = true;
    } else if (dc_strcmp(env, charCommand, "/log") == 0) {
        write_log_to_console(env, err);
    }  else {
        enumCommand = NOT_RECOGNIZED;
    }
    return enumCommand;
}

void admin_client_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet *adminClientPacket, int admin_socket) {
    uint8_t header[ADMIN_HEADER_SIZE] = {0};
    ssize_t count = 0;
    ssize_t total_received = 0;
    size_t remaining_bytes = ADMIN_HEADER_SIZE;

    bool readComplete = false;
    while(!readComplete && (count = dc_read(env, err, admin_socket, (header + total_received), remaining_bytes)) > 0 && dc_error_has_no_error(err))
    {
        remaining_bytes -= (size_t) count;
        total_received += count;
        if (remaining_bytes == 0) {
            readComplete = true;
        }
    }
    if (count <= 0) {
        readComplete = true;
    }
    // parse header
    (*adminClientPacket).version = header[0];
    (*adminClientPacket).command = header[1];
    (*adminClientPacket).target_client_id =  (uint16_t) (header[2] | (uint16_t) header[3] << 8);
    (*adminClientPacket).message_length =  (uint16_t) (header[4] | (uint16_t) header[5] << 8);

    if ((*adminClientPacket).message_length > 0) {
        // read remaining message
        (*adminClientPacket).message = dc_calloc(env, err, (*adminClientPacket).message_length, sizeof(uint8_t));
        count = 0;
        total_received = 0;
        remaining_bytes = (*adminClientPacket).message_length;
        readComplete = false;
        while(!readComplete && (count = dc_read(env, err, admin_socket, ((*adminClientPacket).message + total_received), remaining_bytes)) > 0 && dc_error_has_no_error(err))
        {
            remaining_bytes -= (size_t) count;
            total_received += count;
            if (remaining_bytes == 0) {
                readComplete = true;
            }
        }
        if (count <= 0) {
            readComplete = true;
        }
    }
}

void write_to_log(const struct dc_posix_env *env, struct dc_error *err, char *message) {
    char *messageCopy;
    char *endPointer;
    int logFD;
    char *buffer;
    char timeBuffer[MAX_BUFFER_SIZE];
    time_t rawtime;
    struct tm * timeinfo;
    size_t wordCounter = 0;

    logFD = dc_open(env, err, "../../adminLogs/admin_client_log.txt", O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);;

    time(&rawtime);
    timeinfo = localtime (&rawtime);
    sprintf(timeBuffer, "Log Entry Date: %s", asctime (timeinfo));
    dc_write(env, err, logFD,  timeBuffer, dc_strlen(env, timeBuffer));

    dc_write(env, err, logFD,  "Client List <ID> <IP>:\n", dc_strlen(env, "Client List <ID> <IP>:\n"));

    messageCopy = dc_strdup(env, err, message);

    buffer = dc_strtok_r(env, messageCopy, " ", &endPointer);
    while (buffer) {
        if (wordCounter > 0) {
            buffer = dc_strtok_r(env, endPointer, " ", &endPointer);
            wordCounter++;
        }
        if (buffer) {
            dc_write(env, err, logFD,  buffer, dc_strlen(env, buffer));
            dc_write(env, err, logFD,  " ", 1);
            buffer = dc_strtok_r(env, endPointer, " ", &endPointer);
            wordCounter++;
            dc_write(env, err, logFD,  buffer, dc_strlen(env, buffer));
            dc_write(env, err, logFD,  " ", 1);
            dc_write(env, err, logFD,  "\n", 1);
        }
    }
    dc_close(env, err, logFD);
}

void receiveAdminPacket(const struct dc_posix_env *env, struct dc_error *err, int admin_socket) {
    admin_client_packet adminClientPacket = {0};

    admin_client_readPacketFromSocket(env, err, &adminClientPacket, admin_socket);

    if (adminClientPacket.command == USERS) {
        if (adminClientPacket.message) {
            write_to_log(env, err, adminClientPacket.message);
            printf("Client List written to admin_client_log.txt\n");
        } else {
            printf("No Game Clients Connected.\n");
        }
    }
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
                dc_read(env, err, STDIN_FILENO, buffer, sizeof(buffer));
                command = parseAdminCommand(env, err, buffer, &exit_flag);
                if (command == NOT_RECOGNIZED) {
                    printf("Command not recognized\n");
                } else {
                    send_admin_client_message(env, err, command, NULL, tcp_server_socket);
                }
            }

            // check for new client connections
            if (FD_ISSET(tcp_server_socket, &readfds)) {
                receiveAdminPacket(env, err, tcp_server_socket);
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










