#include "help_query.h"

const char *HelpQuery_Text(void)
{
    return
        "Commands: (case-insensitive)\n"
        "  Enter / roll       Roll a die and move 1-6 blocks.\n"
        "  step <steps>       Move the current player by a positive test value.\n"
        "  query [id]         Show assets for the current or selected player.\n"
        "  sell <position>    Sell one of your properties.\n"
        "  block <offset>     Place a barrier within -10..10 blocks.\n"
        "  bomb <offset>      Place a bomb within -10..10 blocks.\n"
        "  robot              Clear items in the next 10 blocks.\n"
        "  reset              Clear the tutorial play record.\n"
        "  help               Show this help.\n"
        "  quit / q           End the game without saving.\n"
        "\n"
        "Landing on a gift house grants money, points, or five rent-free turns.\n"
        "Barriers stop movement. Bombs send players to hospital for three turns.\n";
}
