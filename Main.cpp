

#include <iostream>
#include <SFML/Graphics.hpp>
#include <array>
#include <fstream> 
#include <vector>
#include <string>
#include <chrono>
#include <sstream>


int CAMERA_X;
int CAMERA_Y;

// Tile related
const int TILE_SIZE = 32;
const int TILETYPE_startIndex = 0;
const int TILETYPE_endIndex = 3;
enum class TILETYPE { BLANK = 0, WALL = 1, PLAYERSPAWN = 2, COIN=3 };
std::array<std::string, 4> tileTypeString = { "empty", "wall", "player_spawn", "coin" };
const int TILETYPE_LEN = 4;

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
    bool shiftIsHeld = false;
    TILETYPE tileType;
};

InputHandling GLOBAL_input;

struct TextDraw
{
public:
    sf::Font font;
    sf::RenderWindow* window;
    //std::vector<sf::Text*> textList; // All texts to draw

    void Init(sf::RenderWindow& _w)
    {
        if (!font.openFromFile("fonts/arial.ttf"))
        {
            throw std::runtime_error("Error: font not found");
            // handle error
        }
        window = &_w;

    }

    void DrawText(std::string s, int x, int y, int fontSize = 24, sf::Color _color=sf::Color::White)
    {
        sf::Text text = sf::Text(font, s);
        //text.setFont(font);
        text.setCharacterSize(fontSize);     // in pixels
        text.setFillColor(_color);
        text.setPosition(sf::Vector2f{ (float)x, (float)y });

        window->draw(text);
    }
};


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

    
    // Tilesheet sprites
    std::array<sf::Texture, tileTypeString.size()> tiletype_Textures;

    // Top left corner on screen to start drawing from
    sf::Vector2f screenPos; 
    Tile* lastPlaced;

    // Spawn positions
    sf::Vector2f playerSpawnPos;
    sf::Vector2f pinkSpawnPos;
    sf::Vector2f redSpawnPos;
    sf::Vector2f orangeSpawnPos;
    sf::Vector2f blueSpawnPos;

    // need to initilaize 

    std::array<sf::Vector2f*, 7> specialVars = { nullptr, nullptr, &playerSpawnPos, &redSpawnPos, &blueSpawnPos, &orangeSpawnPos, &pinkSpawnPos};

    void LoadTileTextures()
    {
        std::cout << "Loading tile textures...\n";
        for (int i = 0; i < tileTypeString.size(); i++)
        {
            // Lookfor: tiletypeString[i].png
            if (!tiletype_Textures[i].loadFromFile("tiles/" + tileTypeString[i] + ".png"))
                throw std::runtime_error("Tile sprite" + tileTypeString[i] + ".png" + " not found");
        }
    }

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
        // Special case: allow map with header only
        if (maxRowReached == -1)
        {
            std::cout << "Header only map detected, filling with EMPTY\n";
            CreateBlank(mapHeight, mapWidth);
        }
        else
        {
            if (maxRowReached < mapHeight - 1) throw std::runtime_error("Error: actual rows < #rows declared in .csv file");
            if (maxColReached < mapWidth - 1) throw std::runtime_error("Error: actual cols < #cols declared in .csv file");
        }


        std::cout << "Map loaded.\n";
    }
    
    // Output to csv
    void SaveToFile(std::string path)
    {
        std::cout << "Saving to file '" << path << "'...\n";

        std::ofstream file(path); // creates file if it doesn't exist
        if (!file) {
            std::cerr << "Failed to open file for writing.\n";
            return;
        }

        
        // Write header
        file << mapHeight << "," << mapWidth << "\n";
        // Write contents
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                // Write tile type data
                file << (int)get(i, j)->tile_ID;
                // Add comma seperator (unless last in row)
                if (j != mapWidth - 1) file << ",";
            }
            file << "\n";
        }
        // done

        file.close();
        std::cout << "Done.\n";
    }


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

    // Returns a list of the coordinates connecting (x1,y1) to (x2, y2)
    std::vector<std::array<int, 2>> getLineFrom(int x1, int y1, int x2, int y2)
    {
        // Ensure y2/x2 is the bigger one
        if (y2 < y1)
        {
            int tmp = y2;
            y2 = y1;
            y1 = tmp;
        }
        if (x2 < x1)
        {
            int tmp = x2;
            x2 = x1;
            x1 = tmp;
        }

        std::vector<std::array<int, 2>> _result;
        // Horizontal
        if (x1 == x2)
        {
            for (int i = y1; i <= y2; i++)
            {
                _result.push_back(std::array<int, 2>{x1, i});
            }
        }
        // Vertical
        else if (y1 == y2)
        {
            for (int i = x1; i <= x2; i++)
            {
                _result.push_back(std::array<int, 2>{i, y1});
            }
        }
        return _result;
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
                // Click->shifthold->click
                else if (GLOBAL_input.shiftIsHeld)
                {
                    // Draw line
                    //x,y
                    std::vector<std::array<int, 2>> pixels = getLineFrom(lastPlaced->col, lastPlaced->row, mouseOver[1], mouseOver[0]);
                    for (std::array<int, 2> pt : pixels)
                    {
                        this->get(pt[1], pt[0])->tile_ID = GLOBAL_input.tileType;
                    }
                }
                // Set to the new tile type
                else this->get(mouseOver[0], mouseOver[1])->tile_ID = GLOBAL_input.tileType;
                lastPlaced = this->get(mouseOver[0], mouseOver[1]);
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
        sf::Sprite specialRender = sf::Sprite(this->tiletype_Textures[(int)GLOBAL_input.tileType]);
        std::array<int, 2> mouseOver = getTileMousedOver(window);
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* _tile = this->get(i, j);
                sf::Sprite _sprite(this->tiletype_Textures[(int)_tile->tile_ID]);
                _sprite.setPosition(screenPos + sf::Vector2f(j * TILE_SIZE, i * TILE_SIZE));
                window.draw(_sprite);
            } // end col loop
        } // end row loop
        
        // Draw the selected one  
        if (mouseOver[0] >= 0 && mouseOver[0] <= this->mapHeight - 1 && mouseOver[1] >= 0 && mouseOver[1] <= this->mapWidth - 1)
        {
            //specialRender = sf::Sprite(this->tiletype_Textures[(int)GLOBAL_input.tileType]);
            specialRender.setPosition(screenPos + sf::Vector2f(mouseOver[1] * TILE_SIZE, mouseOver[0] * TILE_SIZE));
            
            window.draw(specialRender);

            sf::RectangleShape specialRect = sf::RectangleShape(sf::Vector2f{ (float)TILE_SIZE, (float)TILE_SIZE });
            specialRect.setPosition(screenPos + sf::Vector2f(mouseOver[1] * TILE_SIZE, mouseOver[0] * TILE_SIZE));
            sf::Color fill_c = sf::Color::Transparent;
            specialRect.setFillColor(fill_c);
            specialRect.setOutlineColor(sf::Color::Red);
            specialRect.setOutlineThickness(3);
            window.draw(specialRect);
        }

        
    }



};

Map _map;

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
        if (keyEvent->code == sf::Keyboard::Key::Num1) { GLOBAL_input.tileType = TILETYPE::WALL; }
        if (keyEvent->code == sf::Keyboard::Key::P) { GLOBAL_input.tileType = TILETYPE::PLAYERSPAWN; std::cout << "Selected: " << tileTypeString[(int)TILETYPE::PLAYERSPAWN] << "\n"; }
        
        if (keyEvent->code == sf::Keyboard::Key::Up) {
            int target = ((int)GLOBAL_input.tileType) + 1;
            if (target >= tileTypeString.size()) target = 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }
        else if (keyEvent->code == sf::Keyboard::Key::Down) {
            int target = ((int)GLOBAL_input.tileType) - 1;
            if (target < 0) target = TILETYPE_LEN - 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }

        else if (keyEvent->code == sf::Keyboard::Key::S)
        {
            if (GLOBAL_input.controlIsHeld) _map.SaveToFile("outputtest.csv");
        }
        else if (keyEvent->code == sf::Keyboard::Key::LShift) GLOBAL_input.shiftIsHeld = true;
        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = true;
        
        //if (keyEvent->code == sf::Keyboard::Key::)
        //std::cout << "Key code: " << keyEvent->code << "\n";
    }
    else if (event->is < sf::Event::KeyReleased>())
    {
        auto keyEvent = event->getIf<sf::Event::KeyReleased>();
        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = false;
        else if (keyEvent->code == sf::Keyboard::Key::LShift) GLOBAL_input.shiftIsHeld = false;
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
TextDraw textDraw;
int main()
{

    _map.screenPos = sf::Vector2f(64, 64);
    _map.LoadTileTextures();
    //_map.CreateBlank(20, 20);
    _map.LoadFromFile("example.csv");
    sf::RenderWindow window(sf::VideoMode({ (unsigned)(_map.getWidth()+4) * TILE_SIZE, (unsigned)(_map.getHeight()+4) * TILE_SIZE }), "Pacman Maze Editor");
    textDraw.Init(window);
    while (window.isOpen())
    {
        while (std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())window.close();
            HandleInput(event);
        }
        float dt = game_clock.restart().asSeconds();

        _map.Update(dt, window);
        window.clear(sf::Color::Cyan);
        //RenderTopMenu();
        //m.Render(window);
        _map.RenderDebug(window);
        textDraw.DrawText("Selected: " + tileTypeString[(int)GLOBAL_input.tileType], 0, 0, 22, sf::Color::Black);
        window.display();
    }


    return 0;
}
