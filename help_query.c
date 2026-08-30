#include "help_query.h"

const char *HelpQuery_Text(void)
{
    return
        "MONOPOLY command help\n"
        "All commands are case-insensitive. The board has 70 blocks (0-69).\n"
        "\n"
        "Movement\n"
        "  Enter / roll       Roll a die and move 1-6 blocks.\n"
        "  step <steps>       Move by a positive test value.\n"
        "\n"
        "Assets and items\n"
        "  query [id]         Show assets for the current or selected player.\n"
        "  sell <position>    Sell one of your properties.\n"
        "  block <offset>     Place a barrier within -10..10 blocks.\n"
        "  bomb <offset>      Place a bomb within -10..10 blocks.\n"
        "  robot              Clear items in the next 10 blocks.\n"
        "\n"
        "System\n"
        "  reset              Clear the tutorial play record.\n"
        "  help               Show this help.\n"
        "  quit / q           End the game without saving.\n"
        "\n"
        "Rules implemented by the current game\n"
        "  Unowned property is purchased automatically when affordable.\n"
        "  Owned property upgrades automatically to level 3 when affordable.\n"
        "  Rent is base price multiplied by property level, divided by 10.\n"
        "  A barrier stops a moving player and is then removed.\n"
        "  A bomb sends a player to hospital for three turns, which are skipped.\n"
        "  A gift house grants 2000 money, 200 points, or five rent-free turns.\n"
        "  Rent is waived while God of Wealth is active or the owner is unavailable.\n"
        "  A player with negative money is bankrupt; the last active player wins.\n";
}
