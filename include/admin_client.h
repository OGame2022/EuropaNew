#ifndef STIRLING_GAME_DEMO_ADMIN_CLIENT_H
#define STIRLING_GAME_DEMO_ADMIN_CLIENT_H

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

uint8_t parseAdminCommand(const struct dc_posix_env *env, struct dc_error *err, char buffer[MAX_BUFFER_SIZE], volatile sig_atomic_t * exit_flag);
void admin_client_readPacketFromSocket(const struct dc_posix_env *env, struct dc_error *err, admin_client_packet *adminClientPacket, int admin_socket);
admin_client * receiveAdminPacket(const struct dc_posix_env *env, struct dc_error *err, int admin_socket);

#endif //STIRLING_GAME_DEMO_ADMIN_CLIENT_H
