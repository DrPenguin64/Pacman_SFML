#pragma once
#include <SFML/Graphics.hpp>
#include <fstream>

std::string OpenFileDialog(const char* filter);
std::string SaveFileDialog(const char* filter);
bool fileExists(const std::string& filename);