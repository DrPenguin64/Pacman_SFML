

#include <iostream>
#include <SFML/Graphics.hpp>
#include <array>
#include <fstream> 
#include <vector>
#include <string>
#include <chrono>
#include <sstream>


// Tile related
const int TILE_SIZE = 32;
const int TILETYPE_startIndex = 0;
const int TILETYPE_endIndex = 6;
enum class TILETYPE { BLANK = 0, WALL = 1, PLAYERSPAWN = 2, RED = 3, BLU = 4, ORANGE = 5, PINK = 6 };
std::array<std::string, 7> tileTypeString = { "Empty", "Wall", "PlayerSpawn", "RedSpawn", "BlueSpawn", "OrangeSpawn", "PinkSpawn" };
const int TILETYPE_LEN = 6;

// Debug/play toggle
enum class MODE {DEBUG, PLAY};
MODE game_MODE = MODE::DEBUG;

struct InputHandling {
public:
    // Input events/ input related
    // True if mouse left click event this frame
    bool leftClickJustPressed = false;
    // If the left click is pressesd/held
    bool leftClickPressed = false;
    bool rightClickPressed = false;
    // True if mouse right click event this frame
    bool rightClickJustPressed = false;
    // The tile id that was input via keyboard by typing (0, 1, 2, etc)
    bool controlIsHeld = false;
    TILETYPE tileType;
};

InputHandling GLOBAL_input;


bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.is_open();
}

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
        TILETYPE tile_ID;
        int row, col;
        MapSuper* map;

        Tile(MapSuper* _map, int _r, int _c, TILETYPE _t = TILETYPE::BLANK)
        {
            this->map = _map;
            this->tile_ID = _t;
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
        
        static sf::Color tileType2DebugColor(TILETYPE type)
        {
            //std::cout << "tiletype is " << (int)type << "\n";
            if (type == TILETYPE::WALL) return sf::Color::White;
            else if (type == TILETYPE::PLAYERSPAWN) return sf::Color::Green;
            else if (type == TILETYPE::RED) return sf::Color::Red;
            else if (type == TILETYPE::PINK) return sf::Color::Magenta;
            else if (type == TILETYPE::ORANGE) return sf::Color::Yellow;
            else if (type == TILETYPE::BLU) return sf::Color::Blue;
            else return sf::Color::Black;
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

    // Top left corner on screen to start drawing from
    sf::Vector2f screenPos; 

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

    // Gets number of cols
    int getWidth() { return mapWidth; }
    // Gets number of rows
    int getHeight() { return mapHeight; }

    // Creates grid full of NULL ptrs.
    void CreateEmpty(int _rows, int _cols)
    {
        mapHeight = _rows;
        mapWidth = _cols;
        grid.assign(mapHeight * mapWidth, nullptr);
    }

    // Creates grid full of objects (not null)
    void CreateBlank(int _rows, int _cols)
    {
        mapHeight = _rows;
        mapWidth = _cols;
        grid.assign(mapHeight * mapWidth, nullptr);
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* tile = new Tile(this, i, j, TILETYPE::BLANK);
                this->set(i, j, tile);
                //grid[(i * mapWidth) + j] = tile;
            }
        }
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

                    if (asInt < 0 || asInt > TILETYPE_LEN) throw std::runtime_error("File error: csv field is not valid tileID");
                    // otherwise ok

                    // Create tile
                    Tile* newTile = new Tile(this, _row, _col, static_cast<TILETYPE>(asInt));
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


    // Gets {row, col} that is being moused over. returns {-1,-1} if not moused over anything
    std::array<int, 2> getTileMousedOver(sf::RenderWindow &window)
    {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);        // window coordinates
        sf::Vector2f mouseCoords = window.mapPixelToCoords(pixelPos) - screenPos;
        //sf::Vector2f mouseCoords;

        // Get nearest
        int colClosest = mouseCoords.x / TILE_SIZE;
        int rowClosest = mouseCoords.y / TILE_SIZE;
        if (colClosest >= getWidth()) colClosest = -1;
        if (rowClosest >= getHeight()) rowClosest = -1;

        return std::array<int, 2>{rowClosest, colClosest};
    }

    // Returns true if mouse is moused over row, col
    bool isMouseOver(int row, int col, sf::RenderWindow& window)
    {
        std::array<int, 2> mousedOver = getTileMousedOver(window);
        return mousedOver[0] == row && mousedOver[1] == col;
    }

    void Update(float dt, sf::RenderWindow& window)
    {
        if (game_MODE == MODE::DEBUG)
        {
            // Row, col of mouse over
            std::array<int, 2> mouseOver = getTileMousedOver(window);
            if (mouseOver[0] == -1 || mouseOver[1] == -1) return;

            if (GLOBAL_input.leftClickPressed) {
                // Left control + click to erase
                if (GLOBAL_input.controlIsHeld) this->get(mouseOver[0], mouseOver[1])->tile_ID = TILETYPE::BLANK;
                // Set to the new tile type
                else this->get(mouseOver[0], mouseOver[1])->tile_ID = GLOBAL_input.tileType;
            }
            else if (GLOBAL_input.rightClickJustPressed)
            {
                //this->get(mouseOver[0], mouseOver[1])->tile_ID = TILETYPE::BLANK;
            }
        }
    }

    void RenderDebug(sf::RenderWindow& window)
    {
        bool drawFront = true;
        sf::RectangleShape specialRender;
        std::array<int, 2> mouseOver = getTileMousedOver(window);
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* _tile = this->get(i, j);
                sf::RectangleShape r = sf::RectangleShape(sf::Vector2f{ (float)TILE_SIZE, (float)TILE_SIZE });
                if (_tile->tile_ID == TILETYPE::WALL) r.setFillColor(sf::Color::White);
                else if (_tile->tile_ID == TILETYPE::PLAYERSPAWN) r.setFillColor(sf::Color::Green);
                else if (_tile->tile_ID == TILETYPE::RED) r.setFillColor(sf::Color::Red);
                else if (_tile->tile_ID == TILETYPE::PINK) r.setFillColor(sf::Color::Magenta);
                else if (_tile->tile_ID == TILETYPE::ORANGE) r.setFillColor(sf::Color::Yellow);
                else if (_tile->tile_ID == TILETYPE::BLU) r.setFillColor(sf::Color::Blue);
                else r.setFillColor(sf::Color::Black);
                r.setPosition(screenPos + sf::Vector2f(j * TILE_SIZE, i * TILE_SIZE));

                // If mouse over, change outline color
                /*
                std::array<int, 2> mouseOver = getTileMousedOver(window);
                if (mouseOver[0] == i && mouseOver[1] == j) {
;                   r.setOutlineColor(sf::Color::Red);
                    r.setOutlineThickness(3);
                    drawFront = true;
                    specialRender = sf::RectangleShape(r);
                }
                else
                {
                    //r.setOutlineColor(r.getFillColor());
                    //r.setOutlineThickness(3);
                }
                */
                window.draw(r);
            } // end col loop
        } // end row loop
        
        // Draw the selected one  
        if (mouseOver[0] >= 0 && mouseOver[0] <= this->mapHeight - 1 && mouseOver[1] >= 0 && mouseOver[1] <= this->mapWidth - 1)
        {
            specialRender = sf::RectangleShape(sf::Vector2f{ (float)TILE_SIZE, (float)TILE_SIZE });
            specialRender.setPosition(screenPos + sf::Vector2f(mouseOver[1] * TILE_SIZE, mouseOver[0] * TILE_SIZE));
            sf::Color fill_c = Tile::tileType2DebugColor(GLOBAL_input.tileType);
            //std::cout << "fill color is " << fill_c.r << "," << fill_c.g  << "," << fill_c.b << "\n";
            specialRender.setFillColor(fill_c);
            specialRender.setOutlineColor(sf::Color::Red);
            specialRender.setOutlineThickness(3);

            window.draw(specialRender);
        }

        
    }



};

// Updates event flags in Global_input
void HandleInput(std::optional<sf::Event>& event)
{
    GLOBAL_input.leftClickJustPressed = false;
    GLOBAL_input.rightClickJustPressed = false;
    // Handle key press
    if (event->is < sf::Event::KeyPressed>())
    {
        auto keyEvent = event->getIf<sf::Event::KeyPressed>();
        //if (keyEvent->code == sf::Keyboard::Key::Num0) { GLOBAL_input.tileType = TILETYPE::BLANK; std::cout << "Selected: " << tileTypeString[0] << "\n"; }
        if (keyEvent->code == sf::Keyboard::Key::Num1) { GLOBAL_input.tileType = TILETYPE::WALL;}
        if (keyEvent->code == sf::Keyboard::Key::P) { GLOBAL_input.tileType = TILETYPE::PLAYERSPAWN; std::cout << "Selected: " << tileTypeString[(int)TILETYPE::PLAYERSPAWN] << "\n"; }
        
        if (keyEvent->code == sf::Keyboard::Key::Up) {
            int target = ((int)GLOBAL_input.tileType) + 1;
            if (target >= tileTypeString.size()) target = 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }
        else if (keyEvent->code == sf::Keyboard::Key::Down) {
            int target = ((int)GLOBAL_input.tileType) - 1;
            if (target < 0 ) target = TILETYPE_LEN - 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }

        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = true;
        else std::cout << "Selected: " << tileTypeString[(int)GLOBAL_input.tileType] << "\n";
        //if (keyEvent->code == sf::Keyboard::Key::)
        //std::cout << "Key code: " << keyEvent->code << "\n";
    }
    else if (event->is < sf::Event::KeyReleased>())
    {
        auto keyEvent = event->getIf<sf::Event::KeyReleased>();
        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = false;
    }
    else if (event->is<sf::Event::MouseButtonPressed>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseButtonPressed>();
        if (mouseEvent->button == sf::Mouse::Button::Left) {
            GLOBAL_input.leftClickJustPressed = true;
            GLOBAL_input.leftClickPressed = true;
        }
        if (mouseEvent->button == sf::Mouse::Button::Right) GLOBAL_input.rightClickJustPressed = true;
    }
    else if (event->is<sf::Event::MouseButtonReleased>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseButtonReleased>();
        if (mouseEvent->button == sf::Mouse::Button::Left) GLOBAL_input.leftClickPressed = false;
        if (mouseEvent->button == sf::Mouse::Button::Right) GLOBAL_input.rightClickPressed = false;
    }
    
    //asfdsf
}

sf::Clock game_clock;
int main()
{
    Map m;
    m.screenPos = sf::Vector2f(64, 64);
    m.CreateBlank(20, 20);
    //m.LoadFromFile("example.csv");
    sf::RenderWindow window(sf::VideoMode({ (unsigned)(m.getWidth()+4) * TILE_SIZE, (unsigned)(m.getHeight()+4) * TILE_SIZE }), "SFML Pacman");
    while (window.isOpen())
    {
        while (std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())window.close();
            HandleInput(event);
        }
        float dt = game_clock.restart().asSeconds();

        m.Update(dt, window);
        window.clear(sf::Color::Cyan);
        //m.Render(window);
        m.RenderDebug(window);
        window.display();
    }


    return 0;
}
