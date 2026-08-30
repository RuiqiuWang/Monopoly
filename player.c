#include "player.h"

const char *status_to_string(PlayerStatus status)
{
    switch (status) {
    case PLAYER_NORMAL: return "NORMAL";
    case PLAYER_HOSPITAL: return "HOSPITAL";
    case PLAYER_JAIL: return "JAIL";
    case PLAYER_GOD: return "GOD";
    default: return "UNKNOWN";
    }
}
