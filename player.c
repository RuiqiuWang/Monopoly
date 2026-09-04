#include "player.h"

const char *status_to_string(PlayerStatus status)
{
    switch (status) {
    case PLAYER_NORMAL: return "NORMAL";
    default: return "UNKNOWN";
    }
}
