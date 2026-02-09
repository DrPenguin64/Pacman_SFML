

#include <iostream>
#include <SFML/Graphics.hpp>
#include <array>
#include <fstream> 
#include <vector>
#include <string>
#include <chrono>
#include <sstream>

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.is_open();
}

const int TILE_SIZE = 64;


bool parseNumber(std::string s, int& mod)
{
    try {
        int value = std::stoi(s);  // convert string to int
        // valid
        mod = value;
        return true;
    }
    catch (const std::invalid_argument&) {
        return false;
    }
}
enum class TILEID { BLANK = 0, WALL = 1, PLAYERSPAWN = 2, RED = 3, BLU = 4, ORANGE = 5, PINK = 6 };
const int TILEID_MAX = 6;

//Forward declaration
class Tile;
// Just exposes needed for tile
class MapSuper
{
    public:
        virtual void setSpecialLocVars(int index, sf::Vector2f value) = 0;
        virtual Tile* get(int r, int c) = 0;
        virtual int getWidth() = 0;
        virtual int getHeight() = 0;
}; 
class Tile
{
    public:
        TILEID tileId;
        int row, col;
        MapSuper* map;

        Tile(MapSuper* _map, int _r, int _c, TILEID _t = TILEID::BLANK)
        {
            this->map = _map;
            this->tileId = _t;
            this->row = _r;
            this->col = _c;
            // Handle special tile id (update the pointers of the grid)
            // (also raise error if already exists)
            if ((int)_t > 1)
            {
                // Set the loc var
                map->setSpecialLocVars((int)_t, sf::Vector2f(_c, _r));
            }
        }

        // Returns all the adjacent tiles (up,down,left right) to this tile
        Tile* above() { 
            if (row == 0) return nullptr;
            return map->get(row-1, col); }
        Tile* below()
        {
            if (row == map->getHeight() - 1) return nullptr;
            return map->get(row + 1, col);
        }
        Tile* left()
        {
            if (col == 0) return nullptr;
            return map->get(row, col-1);
        }
        Tile* right()
        {
            if (col == map->getWidth() -1) return nullptr;
            return map->get(row, col +1);
        }

        std::vector<Tile*> getAdjacentTiles()
        {
            std::vector<Tile*> _result;
            if (above() != nullptr) _result.push_back(above());
            if (below() != nullptr) _result.push_back(below());
            if (left() != nullptr) _result.push_back(left());
            if (right() != nullptr) _result.push_back(right());
            return _result;
        }


};
class Map : public MapSuper
{
public:
    int mapWidth;
    int mapHeight;
    bool isInitialized = false;
    std::vector<Tile*> grid;

    // Spawn positions
    sf::Vector2f playerSpawnPos;
    sf::Vector2f pinkSpawnPos;
    sf::Vector2f redSpawnPos;
    sf::Vector2f orangeSpawnPos;
    sf::Vector2f blueSpawnPos;

    // need to initilaize 
    std::array<sf::Vector2f*, 7> specialVars = { nullptr, nullptr, &playerSpawnPos, &redSpawnPos, &blueSpawnPos, &orangeSpawnPos, &pinkSpawnPos};


    // Gets tile ref at row,col
    Tile* get(int r, int c)
    {
        assert(r >= 0 && r < mapHeight && c >= 0 && c < mapWidth);
        return grid[r * mapWidth + c];
    }
    void set(int r, int c, Tile* what)
    {
        grid[(r * mapWidth) + c] = what;
    }

    //enum class TILEID { BLANK = 0, WALL = 1, PLAYERSPAWN = 2, RED = 3, BLU = 4, ORANGE = 5, PINK = 6 };
    void setSpecialLocVars(int index, sf::Vector2f value) override
    {
        // todo error checking if it already was set
        *specialVars[index] = value;
    }

    int getWidth() { return mapWidth; }
    int getHeight() { return mapHeight; }

    // Creates grid full of null ptrs.
    void CreateEmpty(int _rows, int _cols)
    {
        mapHeight = _rows;
        mapWidth = _cols;
        grid.assign(mapHeight * mapWidth, nullptr);
    }

    // Input will be a .csv. Each tile = tileid
    // First row should be [ROWS, COLS]
    void LoadFromFile(std::string path)
    {
        std::cout << "Loading map '" << path << "'\n";
        //std::cout << std::filesystem::absolute(path) << std::endl;
        std::ifstream file(path);


        if (!file.is_open()) {
            std::cerr << "Failed to open file.\n";
            return;
        }

        // Keep track of row, col
        int _row = -1; // first row does not count
        int _col = 0;

        int maxRowReached = -1;
        int maxColReached = -1;
        std::string line;
        while (std::getline(file, line)) {           // read line by line
            std::vector<std::string> fields; // what is in the file
            std::istringstream ss(line);
            std::string field;

            while (std::getline(ss, field, ',')) {   // split by comma
                fields.push_back(field);
            }

            if (_row > -1)
            {
                if (_row >= mapHeight) {
                    std::cout << "Error: rows > ROWS declared in file \n";
                    throw std::runtime_error("Error: rows > ROWS declared in file");
                }
            }

            // Example: print the fields
            _col = 0;
            for (const auto& f : fields) {
                if (_row != -1)
                {
                    if (_col >= mapWidth) {
                        std::cout << "Error: too many columns in row " << _row << "\n";
                        throw std::runtime_error("Error: too many columns in row");
                    }
                }
                // convert to int
                int asInt;
                if (!parseNumber(f, asInt)) throw std::runtime_error("File error: could not parse csv field to int");
                std::cout << asInt << "\t";
                if (_row != -1)
                {

                    if (asInt < 0 || asInt > TILEID_MAX) throw std::runtime_error("File error: csv field is not valid tileID");
                    // otherwise ok

                    // Create tile
                    Tile* newTile = new Tile(this, _row, _col, static_cast<TILEID>(asInt));
                    set(_row, _col, newTile);
                }
                else // For first row, cols are [mapHeight, mapWidth] /!IMPORTANT
                {
                    if (_col == 0) mapHeight = asInt;
                    else if (_col == 1) mapWidth = asInt;
                }
                if (_col > maxColReached) maxColReached = _col;
                _col++;
            }
            // Construct empty
            if (_row == -1)
            {
                CreateEmpty(mapHeight, mapWidth);
                std::cout << "\nsize: (" << mapHeight << "x" << mapWidth << ")\n";
            }
            std::cout << "\n";
            if (_row > maxRowReached) maxRowReached = _row;
            _row++; // row ctr
        }
        if (maxRowReached < mapHeight-1) throw std::runtime_error("Error: actual rows < #rows declared in .csv file");
        if (maxColReached < mapWidth-1) throw std::runtime_error("Error: actual cols < #cols declared in .csv file");
        std::cout << "Map loaded.";
    }
    
    // Output to csv
    void SaveToFile(std::string path);

    void Render(sf::RenderWindow& window)
    {
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* _tile = this->get(i, j);
                sf::RectangleShape r = sf::RectangleShape(sf::Vector2f{ (float) TILE_SIZE, (float)TILE_SIZE });
                if (_tile->tileId == TILEID::WALL) r.setFillColor(sf::Color::White);
                else r.setFillColor(sf::Color::Black);
                r.setPosition(sf::Vector2f(j* TILE_SIZE, i* TILE_SIZE));
                window.draw(r);
            }
        }
    }



};

int main()
{
    Map m;
    m.LoadFromFile("example.csv");
    sf::RenderWindow window(sf::VideoMode({ (unsigned)m.getWidth() * TILE_SIZE, (unsigned)m.getHeight() * TILE_SIZE }), "SFML Test");
    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

        }
        window.clear();
        m.Render(window);
        window.display();
    }


    return 0;
}
