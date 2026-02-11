#pragma once
#include <SFML/Graphics.hpp>

const int TILE_SIZE = 32;
const int TILETYPE_startIndex = 0;
const int TILETYPE_endIndex = 3;
enum class TILETYPE { BLANK = 0, WALL = 1, PLAYERSPAWN = 2, COIN = 3 };
std::array<std::string, 4> tileTypeString = { "empty", "wall", "player_spawn", "coin" };
const int TILETYPE_LEN = 4;

/*
void LoadAdditionalTileTypes()
{

}
*/