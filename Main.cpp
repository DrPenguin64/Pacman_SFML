

#include <iostream>
#include <SFML/Graphics.hpp>
#include <array>
#include <fstream> 
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <windows.h>
#include <commdlg.h>
// File dialogs (windows only)
#include "resource.h"
#include "Win32FileDialogs.h"
#include "ResizeDialog.h"

// Tile related
#include "TileTypeDefinitions.h"

#include "Camera.h"

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
    bool middleButtonHeld = false;
    // True if mouse right click event this frame
    bool rightClickJustPressed = false;
    // The tile id that was input via keyboard by typing (0, 1, 2, etc)
    bool controlIsHeld = false;
    bool shiftIsHeld = false;

    float mouseScoll = 0;
    sf::Vector2f cameraMovAxis;

    TILETYPE tileType;

    void stopAll()
    {
        cameraMovAxis = sf::Vector2f(0, 0);
        mouseScoll = 0;
        middleButtonHeld = false;
        leftClickPressed = false;
        rightClickPressed = false;
    }

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
        TILETYPE tileType;
        int row, col;
        MapSuper* map;

        Tile(MapSuper* _map, int _r, int _c, TILETYPE _t = TILETYPE::BLANK)
        {
            this->map = _map;
            this->tileType = _t;
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
        
        /*
        static sf::Color tileType2DebugColor(TILETYPE type)
        {
            //std::cout << "tiletype is " << (int)type << "\n";
            if (type == TILETYPE::WALL) return sf::Color::White;
            else if (type == TILETYPE::PLAYERSPAWN) return sf::Color::Green;
            else return sf::Color::Black;
        }
        */

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
        this->isInitialized = true;
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

    // Frees all ptrs
    void Clear()
    {
        for (Tile* ptr : grid)
        {
            delete ptr;
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
                //std::cout << asInt << "\t";
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
                // Ensure no memory leaks
                Clear();
                CreateEmpty(mapHeight, mapWidth);
                std::cout << "\nsize: (" << mapHeight << "x" << mapWidth << ")\n";
            }
            //std::cout << "\n";
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
                file << (int)get(i, j)->tileType;
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
        float scale = CAMERA_ZOOM;
        sf::Vector2f cameraPos(CAMERA_X, CAMERA_Y);

        // Mouse position in window space
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f screen(static_cast<float>(pixelPos.x),
            static_cast<float>(pixelPos.y));

        // INVERSE of your render transform
        sf::Vector2f world =
            (screen - screenPos + cameraPos) / scale;

        int col = static_cast<int>(world.x / TILE_SIZE);
        int row = static_cast<int>(world.y / TILE_SIZE);

        if (col < 0 || col >= getWidth())  col = -1;
        if (row < 0 || row >= getHeight()) row = -1;

        return { row, col };
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
                if (GLOBAL_input.controlIsHeld) this->get(mouseOver[0], mouseOver[1])->tileType = TILETYPE::BLANK;
                // Click->shifthold->click
                else if (GLOBAL_input.shiftIsHeld)
                {
                    // Draw line
                    //x,y
                    std::vector<std::array<int, 2>> pixels = getLineFrom(lastPlaced->col, lastPlaced->row, mouseOver[1], mouseOver[0]);
                    for (std::array<int, 2> pt : pixels)
                    {
                        this->get(pt[1], pt[0])->tileType = GLOBAL_input.tileType;
                    }
                }
                // Set to the new tile type
                else this->get(mouseOver[0], mouseOver[1])->tileType = GLOBAL_input.tileType;
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
        sf::Vector2f _scale = sf::Vector2f(CAMERA_ZOOM, CAMERA_ZOOM);
        bool drawFront = true;
        sf::Sprite specialRender = sf::Sprite(this->tiletype_Textures[(int)GLOBAL_input.tileType]);
        std::array<int, 2> mouseOver = getTileMousedOver(window);
        sf::Vector2f cameraPos = sf::Vector2f(CAMERA_X, CAMERA_Y);

        
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* _tile = this->get(i, j);
                sf::Sprite _sprite(this->tiletype_Textures[(int)_tile->tileType]);
                _sprite.setScale(_scale);
                _sprite.setPosition(screenPos - cameraPos + sf::Vector2f(j * TILE_SIZE* _scale.x, i * TILE_SIZE* _scale.y));
                // if is selected
                window.draw(_sprite);
            } // end col loop
        } // end row loop

        // Render preview
        if (lastPlaced != nullptr && GLOBAL_input.shiftIsHeld)
        {
            std::vector<std::array<int, 2>> pixels = getLineFrom(lastPlaced->col, lastPlaced->row, mouseOver[1], mouseOver[0]);
            for (std::array<int, 2> pt : pixels)
            {
                sf::Sprite _sprite(this->tiletype_Textures[(int)GLOBAL_input.tileType]);
                _sprite.setScale(_scale);
                _sprite.setPosition(screenPos - cameraPos + sf::Vector2f(pt[0] * TILE_SIZE * _scale.x, pt[1] * TILE_SIZE * _scale.y));
                _sprite.setColor(sf::Color(255, 255, 255, 127));
                // if is selected
                window.draw(_sprite);
            }
        }


        
        // Draw the selected one  
        if (mouseOver[0] >= 0 && mouseOver[0] <= this->mapHeight - 1 && mouseOver[1] >= 0 && mouseOver[1] <= this->mapWidth - 1)
        {
            //specialRender = sf::Sprite(this->tiletype_Textures[(int)GLOBAL_input.tileType]);
            //_spspecialRenderrite.setPosition(screenPos + sf::Vector2f(j * TILE_SIZE, i * TILE_SIZE));
            specialRender.setPosition(screenPos -cameraPos + sf::Vector2f(mouseOver[1] * TILE_SIZE* _scale.x, mouseOver[0] * TILE_SIZE* _scale.y));
            specialRender.setScale(_scale);
            
            window.draw(specialRender);

            sf::RectangleShape specialRect = sf::RectangleShape(sf::Vector2f{ (float)TILE_SIZE*_scale.x, (float)TILE_SIZE*_scale.y});
            specialRect.setPosition(screenPos - cameraPos + sf::Vector2f(mouseOver[1] * TILE_SIZE*_scale.x, mouseOver[0] * TILE_SIZE*_scale.x));
            sf::Color fill_c = sf::Color::Transparent;
            specialRect.setFillColor(fill_c);
            specialRect.setOutlineColor(sf::Color::Red);
            specialRect.setOutlineThickness(3);
            window.draw(specialRect);
        }

        
    }

    void RenderScaledAt(sf::RenderWindow& window, sf::Vector2f scale)
    {
        sf::Vector2f scaledViewSize = sf::Vector2f((float)this->getWidth() * TILE_SIZE*scale.x, (float)this->getHeight() * TILE_SIZE*scale.y);
        sf::Vector2f topLeftViewLoc = sf::Vector2f(window.getSize().x, window.getSize().y) - scaledViewSize;

        sf::Vector2f scaledCameraPos = topLeftViewLoc + sf::Vector2f(CAMERA_X * scale.x, CAMERA_Y * scale.y);
        sf::Vector2f scaledCameraSize = sf::Vector2f((float)(window.getSize().x) * scale.x* 1/CAMERA_ZOOM, (float)(window.getSize().y) * scale.y*1/CAMERA_ZOOM);

        //sf::Vector2f cameraPos = sf::Vector2f(CAMERA_X, CAMERA_Y);
        for (int i = 0; i < mapHeight; i++)
        {
            for (int j = 0; j < mapWidth; j++)
            {
                Tile* _tile = this->get(i, j);
                sf::Sprite _sprite(this->tiletype_Textures[(int)_tile->tileType]);
                _sprite.setScale(scale);
                _sprite.setPosition(topLeftViewLoc + sf::Vector2f(j * TILE_SIZE * scale.x, i * TILE_SIZE * scale.y));
                window.draw(_sprite);
            } // end col loop
        } // end row loop
        sf::RectangleShape r = sf::RectangleShape(scaledViewSize);
        r.setFillColor(sf::Color::Transparent);
        r.setOutlineColor(sf::Color::Red);
        r.setOutlineThickness(3.0);
        r.setPosition(topLeftViewLoc);
        window.draw(r);

        sf::RectangleShape r2 = sf::RectangleShape(scaledCameraSize);
        r2.setFillColor(sf::Color::Transparent);
        r2.setOutlineColor(sf::Color::Green);
        r2.setOutlineThickness(3.0);
        r2.setPosition(scaledCameraPos);
        window.draw(r2);
    }


};

class Button
{
private:
    sf::RenderWindow* window;
    sf::Texture _texture;
    sf::Sprite* _sprite;
public:
    int x, y;
    int sizeX = 32;
    int sizeY = 32;
    Button(std::string texturePath, sf::RenderWindow* _win)
    {
        if (!_texture.loadFromFile(texturePath)) std::cout << "Error: could not load texture: " << texturePath << "\n";
        sizeX = _texture.getSize().x;
        sizeY = _texture.getSize().y;
        _sprite = new sf::Sprite(_texture);
        this->window = _win;
    }

    bool IsMouseOver()
    {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(*window);        // window coordinates
        sf::Vector2f mouseCoords = window->mapPixelToCoords(pixelPos);

        int minX = x;
        int maxX = x + sizeX;
        int minY = y;
        int maxY = y + sizeY;

        return mouseCoords.x >= minX && mouseCoords.x <= maxX && mouseCoords.y >= minY && mouseCoords.y <= maxY;
    }

    void setPosition(int _x, int _y)
    {
        this->x = _x;
        this->y = _y;
        _sprite->setPosition(sf::Vector2f{ (float)x, (float)y });
    }

    void Draw()
    {
        window->draw(*_sprite);
    }

    bool CheckIsJustClicked()
    { 
        return IsMouseOver() && GLOBAL_input.leftClickJustPressed;
    }

};

class Menu
{
private:
    Button* config;
    Button* open;
    Button* save;
    Button* _new;
    Button* resize;


    std::vector<Button*> buttons;

    // Arrange from right
    int offset = 5;

    sf::RenderWindow* window;
    int screenWidth, screenHeight;
public:
    void Init(sf::RenderWindow* _window, int _screenWidth, int _screenHeight)
    {
        this->window = _window;
        this->screenWidth = _screenWidth;
        this->screenHeight = _screenHeight;
    }

    void LoadTextures()
    {
        std::cout << "Loading menu textures...\n";
        config = new Button("menu\\config.png", window);
        open = new Button("menu\\open.png", window);
        save = new Button("menu\\save.png", window);
        resize = new Button("menu\\resize.png", window);
        _new = new Button("menu\\new.png", window);

        buttons.push_back(_new);
        buttons.push_back(open);
        buttons.push_back(resize);
        buttons.push_back(save);

        std::cout << "Menu textures loaded.\n";
    }

    void Draw()
    {
        for (int i = 0; i < buttons.size(); i++)
        {
            buttons[i]->setPosition(screenWidth-(32*(buttons.size() - i)), 0);
            buttons[i]->Draw();
        }
    }

    void Update(float dt, Map& _map)
    {
        if (_new->CheckIsJustClicked())
        {
            ResizeDialog_InputData data;

            DialogBoxParam(
                GetModuleHandle(nullptr),
                MAKEINTRESOURCE(IDD_INPUT_DIALOG),
                nullptr,
                InputDlgProc,
                reinterpret_cast<LPARAM>(&data)
            );

            if (data.confirmed)
            {
                std::cout << "A: " << data.a << "\n";
                std::cout << "B: " << data.b << "\n";
            }
            _map.Clear();
            _map.CreateBlank(data.b, data.a);
            CAMERA_X = 0;
            CAMERA_Y = 0;
        }
        else if (resize->CheckIsJustClicked())
        {
            ResizeDialog_InputData data;

            DialogBoxParam(
                GetModuleHandle(nullptr),
                MAKEINTRESOURCE(IDD_INPUT_DIALOG),
                nullptr,
                InputDlgProc,
                reinterpret_cast<LPARAM>(&data)
            );

            if (data.confirmed)
            {
                std::cout << "A: " << data.a << "\n";
                std::cout << "B: " << data.b << "\n";
            }
        }
        else if (save->CheckIsJustClicked())
        {
            std::string path = SaveFileDialog(
                "CSV Files (*.csv)\0*.csv\0"
            );

            if (!path.empty())
            {
                std::cout << "Selected: " << path << "\n";
            }
            _map.SaveToFile(path);
        }
        else if (open->CheckIsJustClicked())
        {
            std::string path = OpenFileDialog(
                "CSV Files (*.csv)\0*.csv\0"
                "All Files (*.*)\0*.*\0"
            );

            if (!path.empty())
            {
                std::cout << "Selected: " << path << "\n";
            }
            GLOBAL_input.stopAll();
            _map.LoadFromFile(path);
            CAMERA_X = 0;
            CAMERA_Y = 0;
        }
    }

};

Map _map;

// Updates event flags in Global_input
void HandleInput(std::optional<sf::Event>& event, float dt)
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

        if (keyEvent->code == sf::Keyboard::Key::Q) {
            int target = ((int)GLOBAL_input.tileType) + 1;
            if (target >= tileTypeString.size()) target = 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }
        else if (keyEvent->code == sf::Keyboard::Key::E) {
            int target = ((int)GLOBAL_input.tileType) - 1;
            if (target < 0) target = TILETYPE_LEN - 1; // loop around
            GLOBAL_input.tileType = static_cast<TILETYPE>(target);
        }

        // WASD
        if (keyEvent->code == sf::Keyboard::Key::W)
        {
            GLOBAL_input.cameraMovAxis.y = -1;
        }
        else if (keyEvent->code == sf::Keyboard::Key::S)
        {
            GLOBAL_input.cameraMovAxis.y = 1;
        }
        else if (keyEvent->code == sf::Keyboard::Key::A)
        {
            GLOBAL_input.cameraMovAxis.x = -1;
        }
        else if (keyEvent->code == sf::Keyboard::Key::D)
        {
            GLOBAL_input.cameraMovAxis.x = 1;
        }
        else if (keyEvent->code == sf::Keyboard::Key::LShift) GLOBAL_input.shiftIsHeld = true;
        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = true;

    }
    else if (event->is < sf::Event::KeyReleased>())
    {
        auto keyEvent = event->getIf<sf::Event::KeyReleased>();
        if (keyEvent->code == sf::Keyboard::Key::LControl) GLOBAL_input.controlIsHeld = false;
        else if (keyEvent->code == sf::Keyboard::Key::LShift) GLOBAL_input.shiftIsHeld = false;
        // WASD
        else if (keyEvent->code == sf::Keyboard::Key::W)
        {
            if (GLOBAL_input.cameraMovAxis.y == -1) GLOBAL_input.cameraMovAxis.y = 0;
        }
        else if (keyEvent->code == sf::Keyboard::Key::S)
        {
            if (GLOBAL_input.cameraMovAxis.y == 1) GLOBAL_input.cameraMovAxis.y = 0;
        }
        else if (keyEvent->code == sf::Keyboard::Key::A)
        {
            if (GLOBAL_input.cameraMovAxis.x == -1) GLOBAL_input.cameraMovAxis.x = 0;
        }
        else if (keyEvent->code == sf::Keyboard::Key::D)
        {
            if (GLOBAL_input.cameraMovAxis.x == 1) GLOBAL_input.cameraMovAxis.x = 0;
        }
    }
    else if (event->is<sf::Event::MouseButtonPressed>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseButtonPressed>();
        if (mouseEvent->button == sf::Mouse::Button::Left) {
            GLOBAL_input.leftClickJustPressed = true;
            GLOBAL_input.leftClickPressed = true;
        }
        else if (mouseEvent->button == sf::Mouse::Button::Right) GLOBAL_input.rightClickJustPressed = true;
        else if (mouseEvent->button == sf::Mouse::Button::Middle) GLOBAL_input.middleButtonHeld = true;
    }
    else if (event->is<sf::Event::MouseButtonReleased>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseButtonReleased>();
        if (mouseEvent->button == sf::Mouse::Button::Left) GLOBAL_input.leftClickPressed = false;
        if (mouseEvent->button == sf::Mouse::Button::Right) GLOBAL_input.rightClickPressed = false;
        if (mouseEvent->button == sf::Mouse::Button::Middle) GLOBAL_input.middleButtonHeld = false;
    }
    else if (event->is<sf::Event::MouseWheelScrolled>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseWheelScrolled>();
        GLOBAL_input.mouseScoll = mouseEvent->delta;
        //std::cout << "mouseScoll:  " << mouseEvent->delta;
    }
    else if (event->is<sf::Event::MouseMoved>())
    {
        auto mouseEvent = event->getIf<sf::Event::MouseMoved>();
    
    }
    
    //asfdsf
}


sf::Clock game_clock;
TextDraw textDraw;
Menu menu;
sf::RenderWindow* window;

void LoadTextures()
{
    menu.LoadTextures();
    _map.LoadTileTextures();
}

void UpdateCamera(float dt)
{
    CAMERA_X += GLOBAL_input.cameraMovAxis.x * cameraMoveSpd * dt;
    CAMERA_Y += GLOBAL_input.cameraMovAxis.y * cameraMoveSpd * dt;
    if (GLOBAL_input.controlIsHeld) {
        
        CAMERA_ZOOM += GLOBAL_input.mouseScoll * CAMERA_ZOOMSPD;
        GLOBAL_input.mouseScoll = 0;
        if (CAMERA_ZOOM < MIN_CAMERA_ZOOM) CAMERA_ZOOM = MIN_CAMERA_ZOOM;
        else if (CAMERA_ZOOM > MAX_CAMERA_ZOOM) CAMERA_ZOOM = MAX_CAMERA_ZOOM;
    }

}

void Update(float dt)
{
    UpdateCamera(dt);
    _map.Update(dt, *window);
    menu.Update(dt, _map);
    window->clear(sf::Color::Cyan);
}

void DrawMiniView(Map* map, sf::RenderWindow* window)
{
    _map.RenderScaledAt(*window, sf::Vector2f(1.000f/32, 1.000f/32));
}

void Draw()
{
    _map.RenderDebug(*window);
    textDraw.DrawText("Selected: " + tileTypeString[(int)GLOBAL_input.tileType], 0, 0, 22, sf::Color::Red);
    textDraw.DrawText(std::string("current map:") + std::to_string(_map.getWidth()) + "x" + std::to_string(_map.getHeight()), 0, 22, 22, sf::Color::Red);
    
    // Render mini view of map
    DrawMiniView(&_map, window);
    
    // Render ui
    menu.Draw();
}

int main()
{
    
    _map.screenPos = sf::Vector2f(0, 0);
    //_map.CreateBlank(20, 20);
    _map.LoadFromFile("example.csv");
    int screenWidth = 1024;
    int screenHeight = 720;
    window = new sf::RenderWindow(sf::VideoMode({ (unsigned)screenWidth, (unsigned)screenHeight }), "Pacman Maze Editor");
    menu.Init(window, screenWidth, screenHeight);
    LoadTextures();
    textDraw.Init(*window);
    while (window->isOpen())
    {
        float dt = game_clock.restart().asSeconds();
        //GLOBAL_input.mouseScoll = 0;
        while (std::optional event = window->pollEvent())
        {
            if (event->is<sf::Event::Closed>())window->close();
            HandleInput(event, dt);
        }
        Update(dt);
        
        // Draw
        Draw();

        window->display();
    }


    return 0;
}
