#ifndef STIRLING_GAME_DEMO_ADMIN_CLIENT_H
#define STIRLING_GAME_DEMO_ADMIN_CLIENT_H

#include <dc_posix/dc_stdio.h>
#include "default_config.h"
#include "admin_integration.h"

#define MAX_BUFFER_SIZE 1024

struct admin_application_settings
{
    struct dc_opt_settings opts;
    struct dc_setting_bool *verbose;
    struct dc_setting_string *hostname;
    struct dc_setting_uint16 *port;
};

/**
 * Writes contents of admin_client_log.txt to console for statistical purposes.
 * @param env
 * @param err
 */
void write_log_to_console(const struct dc_posix_env *env, struct dc_error *err);

/**
 * Parses the keyboard input of an admin client to determine which command to use.
 * Current functional commands:
 * /stop
 * /users
 * /quit
 * /log
 * @param env
 * @param err
 * @param buffer --> keyboard input from STDIN
 * @param exit_flag --> exit flag for while loop
 * @return uint8_t --> enum of command to send
 */
uint8_t parseAdminCommand(const struct dc_posix_env *env, struct dc_error *err, char buffer[MAX_BUFFER_SIZE], volatile sig_atomic_t * exit_flag);

/**
 * Deserializes an admin packet and stores results to an admin_client_packet struct.
 * @param env
 * @param err
 * @param adminClientPacket --> admin_client_packet struct to store results
 * @param admin_socket --> socket to read packet from
 */
void admin_client_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet *adminClientPacket, int admin_socket);

/**
 * Writes contents of a USERS message to admin_client_log.txt
 * @param env
 * @param err
 * @param message
 */
void write_to_log(const struct dc_posix_env *env, struct dc_error *err, char *message);

/**
 * Receives an admin packet from
 * @param env
 * @param err
 * @param admin_socket
 */
void receiveAdminPacket(const struct dc_posix_env *env, struct dc_error *err, int admin_socket);

#endif //STIRLING_GAME_DEMO_ADMIN_CLIENT_H
