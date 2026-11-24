#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace std;
using namespace sf;

// Expanded GameState enum
enum class GameState
{
    Menu,
    StartMenu, // Sub-menu after pressing START GAME
    Tutorial,  // Tutorial screen
    Playing,
    Settings,
    Paused, // (Haras)
    Death,
    Shop,
    Debug // (Haras)
};

// A DEBUG STATE YAYYYYYYYYYYYY (Haras)
struct DebugState
{
    string input = "";
    bool unlocked = false;
    bool wrongPassword = false;
    int jumpPhase = 0; // 0 means none selected and 1 or 2 or 3 = target phase
};

enum heartType
{
    red,
    golden,
    blue,
    green,
    pink,
    purple
};

struct Star
{
    Vector2f position;
    float speed;
    Color color;
};

// Trail node struct for the dangerous path
struct TrailNode
{
    Vector2f position;
    float lifeTimer = 1.0f;
};

// Lightweight struct for projectiles
struct Projectile
{
    Vector2f position;
    Vector2f velocity;
    bool isLaunched = true;
    float launchTimer = 0.0f;
    vector<TrailNode> trail;
    float trailTimer = 0.0f;
    bool leavesTrail = false; // basically whether the projectiles leave the dangerous trail or not (Bazil)
    bool tracksPlayer = true; // many of the projectiles I use are homing, ill set this to true if I want homing projectiles and False if I don't(Bazil)
    float spawnDelay = 0.0f;
    float hitBox = 10.f;
    float rotation = 0.0f;     // This is required for the falling crystals ;
    bool parried = false;      // projectiles can be parried
    float steeringtime = 0.0f; // this is for homing projectiles (Haras)
};

struct DialogueLine // hehheahehhaehaeh
{
    string text;  // well like, duh
    Color color;  // also duh
    string sound; // you'll see :>
};

// for all the dialogues
void drawDialogue(RenderWindow &window, const Font &font, const string &str, Color color, Vector2f boxPos) // function for the dialogues
{
    RectangleShape box({300.f, 100.f}); // rectangle text box size cuz yeah
    box.setPosition(boxPos);            // I mean, yeah
    box.setFillColor(Color::White);     // the background colour
    box.setOutlineColor(Color::Black);  // the outline
    box.setOutlineThickness(4.0f);
    Text text(font, str, 15); // change the 14 to change text size
    text.setFillColor(color);
    text.setPosition(boxPos + Vector2f(15.f, 12.f));

    window.draw(box);
    window.draw(text);
}

// Helper function to create centered text cleanly
// this creates centered text so we do not have to qrite it again and again
Text createCenteredText(const Font &font, const string &str, unsigned int size, Vector2f pos, Color color)
{
    Text text(font, str, size);
    text.setFillColor(color);
    FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
    text.setPosition(pos);
    return text;
}

// Reusable function to setup any list of menu items centered on screen (800px width center = 400.f)
// this handles the spacing
vector<Text> createMenuItems(const Font &font, const vector<string> &options, float startY = 280.f, float spacing = 65.f, unsigned int fontSize = 30, float startX = 400.f)
{
    vector<Text> items;
    // Takes a list of items and places them vertically so they look aligned
    int rowIndex = 0;

    for (const auto &textString : options)
    {
        Text menuItem(font, textString, fontSize);
        FloatRect bounds = menuItem.getLocalBounds();
        menuItem.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
        menuItem.setPosition({startX, startY + rowIndex * spacing});
        menuItem.setFillColor(Color(180, 180, 180));
        items.push_back(menuItem);

        rowIndex++;
    }
    return items;
}

// reduces health bar each time when hit
void healthBarUpdate(RectangleShape &healthBar, float damage)
{
    // also now we pass damage instead of fixed value ;
    Vector2f currentPosition = healthBar.getSize();
    float hitpoints = currentPosition.x;
    hitpoints -= damage;
    healthBar.setSize({hitpoints, 20.0f});
}

// Event polling and hover interaction logic
// Chnage state = 0 is menu change state 1 is pause change state 2 is playing
void menu_Interaction(RenderWindow &window, View &view, GameState &currentState,
                      int &hoveredIndex, vector<Text> &mainMenuItems, vector<Text> &deathMenuOptions,
                      vector<Text> &startMenuItems, vector<Text> &pauseMenuItems, vector<string> &startOptions,
                      int &difficultyIndex, const vector<string> &difficulties, const Font &font, int &changeState, GameState &previousState,
                      float &speed, float &projectileSpeed, float &waveInterval, float &launchInterval,
                      float &soundVolume, float &musicVolume, float &shakeIntensity, int &currentHeart, Texture &hearts, const vector<string> &heartFiles,
                      float scaling[], Sprite &playerSprite, Sprite &playerIcon, DebugState &debug)
{ // (Haras)
    // Making menu functional for soundvolume and musicvolume and shakeintensity changes, also added DEBUG state (Haras)
    // gets new mouse pixel also works on all types / sizes of the screen

    Vector2i mousePixel = Mouse::getPosition(window);
    Vector2f mousePos = window.mapPixelToCoords(mousePixel);

    // Get active menu items list depending on current state
    vector<Text> *activeMenuItems = nullptr;

    // load the required menu
    if (currentState == GameState::Menu)
        activeMenuItems = &mainMenuItems;          // when in main menu load up the main menu items ;
    else if (currentState == GameState::StartMenu) // when in sub menu load up the start menu items
        activeMenuItems = &startMenuItems;
    else if (currentState == GameState::Paused)
        activeMenuItems = &pauseMenuItems;
    else if (currentState == GameState::Death)
        activeMenuItems = &deathMenuOptions;

    // checking all events
    while (const optional event = window.pollEvent())
    {
        if (event->is<Event::Closed>())
        {
            window.close();
        }
        // keeping screen proportional / scaled ;
        if (event->getIf<Event::Resized>())
        {
            window.setView(view);
        }

        if (const auto *mouseClick = event->getIf<Event::MouseButtonPressed>())
        {
            if (mouseClick->button == Mouse::Button::Left)
            {
                // Main menu work
                // Changing state when required ;
                if (currentState == GameState::Menu)
                {
                    if (hoveredIndex == 0)
                        currentState = GameState::StartMenu; // Goes to Start Sub-Menu
                    else if (hoveredIndex == 1)
                    {
                        previousState = GameState::Menu;
                        currentState = GameState::Settings;
                    }
                    else if (hoveredIndex == 2)
                        window.close();
                    // These changes brought too the main menu
                    else if (hoveredIndex == 3)
                        currentState = GameState::Tutorial;
                    else if (hoveredIndex == 4)
                        currentState = GameState::Shop;
                }
                // For the sub menu
                else if (currentState == GameState::StartMenu)
                {
                    if (hoveredIndex == 0)
                    {
                        // change difficulty index 0 = easy, 1 = normal, 2 = hard
                        difficultyIndex = (difficultyIndex + 1) % difficulties.size();
                        startOptions[0] = "DIFFICULTY: " + difficulties[difficultyIndex];
                        startMenuItems = createMenuItems(font, startOptions, 240.f, 60.f, 26);
                        // changing stats based on difficulty ;
                        projectileSpeed = 20.0f + 5.0f * (difficultyIndex - 1);
                        launchInterval = 0.3 - 0.1 * (difficultyIndex - 1);
                        waveInterval = 0.8f - 0.2f * (difficultyIndex - 1);
                    }
                    // work in prpgress
                    else if (hoveredIndex == 1)
                        // Change this to endless please
                        currentState = GameState::Tutorial; // Tutorial page add textures
                    else if (hoveredIndex == 2)
                    {
                        previousState = GameState::Playing;
                        currentState = GameState::Playing; // to main battleField , change state to 2 so that we switch from menu to action
                        changeState = 2;
                    }
                    else if (hoveredIndex == 3)
                        currentState = GameState::Menu; // take back to main menu
                }
                else if (currentState == GameState::Paused)
                {
                    if (hoveredIndex == 0)
                    {
                        currentState = GameState::Playing;
                        changeState = 2; // Set to 2 which is the Playing state
                    }
                    else if (hoveredIndex == 1)
                    {
                        previousState = GameState::Paused;
                        currentState = GameState::Settings;
                    }
                    else if (hoveredIndex == 2)
                    {
                        previousState = GameState::Menu; // THis was added because previous state remained pause if we
                        currentState = GameState::Menu;
                        changeState = 0; // (Haras) for game end screen overhaul
                    }
                    else if (hoveredIndex == 3)
                    {
                        window.close();
                    }
                }
                else if (currentState == GameState::Death)
                {
                    if (hoveredIndex == 0)
                    {
                        currentState = GameState::Playing;
                        changeState = 2;
                    }
                    else if (hoveredIndex == 1)
                    {
                        previousState = GameState::Menu;
                        currentState = GameState::Menu;
                        changeState = 0; // (Haras) for game end screen overhaul
                    }
                }
            }
        }

        // Typing password in Debug menu (Haras)
        if (const auto *textEntered = event->getIf<Event::TextEntered>())
        {
            if (currentState == GameState::Debug && !debug.unlocked)
            {
                if (textEntered->unicode == 8) // Backspace
                {
                    if (!debug.input.empty())
                        debug.input.pop_back();
                    debug.wrongPassword = false;
                }
                else if (textEntered->unicode >= 32 && textEntered->unicode < 127 && debug.input.size() < 16)
                {
                    debug.input += static_cast<char>(textEntered->unicode);
                    debug.wrongPassword = false;
                }
            }
        }

        // ESC key press , return to previous stage
        if (const auto *keyPressed = event->getIf<Event::KeyPressed>())
        {
            if (keyPressed->code == Keyboard::Key::Escape)
            {
                if (currentState == GameState::StartMenu)
                {
                    currentState = GameState::Menu;
                }
                else if (currentState == GameState::Settings)
                {
                    currentState = previousState;
                }
                else if (currentState == GameState::Debug)
                {
                    debug.input.clear();
                    debug.unlocked = false;
                    debug.wrongPassword = false;
                    currentState = GameState::Settings;
                }
                else if (currentState == GameState::Tutorial || currentState == GameState::Shop)
                {
                    currentState = GameState::Menu;
                }
                else if (currentState == GameState::Paused)
                {
                    currentState = GameState::Playing;
                    changeState = 2;
                }
                // close game if escape pressed in the other screen
                else if (currentState == GameState::Menu)
                {
                    window.close();
                }
                else if (currentState == GameState::Death)
                {
                    previousState = GameState::Menu;
                    currentState = GameState::Menu;
                    changeState = 0; // (Haras) for game end screen overhaul
                }
            }

            // Making menu functional for soundvolume and musicvolume changes (Haras)
            if (currentState == GameState::Settings)
            {
                // For SF (Haras)
                if (keyPressed->code == Keyboard::Key::Left)
                {
                    soundVolume = max(0.0f, soundVolume - 10.0f); // Decrease SF by 10. Cannot go below zero
                }
                else if (keyPressed->code == Keyboard::Key::Right)
                {
                    soundVolume = min(100.0f, soundVolume + 10.0f); // Increase SF by 10. Cannot go above 100
                }
                // For BGM (Haras)
                else if (keyPressed->code == Keyboard::Key::Up)
                {
                    musicVolume = min(100.0f, musicVolume + 10.0f); // Increase BGM by 10. Cannot go above 100
                }
                else if (keyPressed->code == Keyboard::Key::Down)
                {
                    musicVolume = max(0.0f, musicVolume - 10.0f); // Decrease BGM by 10. Cannot go below zero
                }
                // For screen shake upon collision intensity (Haras)
                else if (keyPressed->code == Keyboard::Key::W)
                {
                    shakeIntensity = min(20.0f, shakeIntensity + 1.0f); // Increase SI
                }
                else if (keyPressed->code == Keyboard::Key::S)
                {
                    shakeIntensity = max(0.0f, shakeIntensity - 1.0f); // Decrease SI, if equals zero then feature is off
                }
                // new debug menu YEAHHHH (Haras)
                else if (keyPressed->code == Keyboard::Key::Tab)
                {
                    debug.input.clear();
                    debug.unlocked = false;
                    debug.wrongPassword = false;
                    currentState = GameState::Debug;
                }
            }
            else if (currentState == GameState::Debug)
            {
                if (!debug.unlocked)
                {
                    if (keyPressed->code == Keyboard::Key::Enter)
                    {
                        if (debug.input == "1234") // this is the debug password SHHHHHH (Haras)
                        {
                            debug.unlocked = true;
                            debug.wrongPassword = false;
                            debug.input.clear();
                        }
                        else
                        {
                            debug.wrongPassword = true;
                            debug.input.clear();
                        }
                    }
                }
                else
                {
                    int chosen = 0;
                    if (keyPressed->code == Keyboard::Key::Num1 || keyPressed->code == Keyboard::Key::Numpad1)
                        chosen = 1;
                    else if (keyPressed->code == Keyboard::Key::Num2 || keyPressed->code == Keyboard::Key::Numpad2)
                        chosen = 2;
                    else if (keyPressed->code == Keyboard::Key::Num3 || keyPressed->code == Keyboard::Key::Numpad3)
                        chosen = 3;

                    if (chosen != 0)
                    {
                        debug.jumpPhase = chosen;
                        debug.unlocked = false;
                        previousState = GameState::Playing;
                        currentState = GameState::Playing;
                        changeState = 2;
                    }
                }
            }
            if (currentState == GameState::Shop)
            {
                bool changed = false;
                if (keyPressed->code == Keyboard::Key::Right)
                {
                    currentHeart = (currentHeart + 1) % 6; // move to the right skin
                    changed = true;
                }
                else if (keyPressed->code == Keyboard::Key::Left)
                {
                    currentHeart = (currentHeart + 5) % 6;
                    changed = true;
                }

                // if a new skin selected we change the skin display and all sprites
                if (changed)
                {
                    // get the new texture and skin
                    if (!hearts.loadFromFile(heartFiles[currentHeart]))
                    {
                        return;
                    }
                    hearts.setSmooth(false);

                    // change sprite texture ;
                    playerSprite.setTexture(hearts, true);
                    FloatRect bounds = playerSprite.getLocalBounds();
                    playerSprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
                    playerSprite.setScale({scaling[currentHeart], scaling[currentHeart]});

                    playerIcon.setTexture(hearts, true);
                    playerIcon.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});

                    playerIcon.setScale({scaling[currentHeart], scaling[currentHeart]});
                }
            }
        }
    }

    // When mouse is above text highlight and rescale :
    if (activeMenuItems != nullptr)
    {
        hoveredIndex = -1;
        int indexCounter = 0;
        for (auto &item : *activeMenuItems)
        {
            if (item.getGlobalBounds().contains(mousePos))
            {
                hoveredIndex = indexCounter;
                item.setFillColor(Color::Yellow);
                // resize
                item.setScale({1.15f, 1.15f});
            }
            else
            {
                // (Haras) for game end screen overhaul
                if (currentState == GameState::Death)
                {
                    item.setFillColor(indexCounter == 0 ? Color(255, 40, 40) : Color::White);
                }
                else
                {
                    item.setFillColor(Color(180, 180, 180));
                }

                item.setScale({1.0f, 1.0f});
            }
            indexCounter++;
        }
    }
}

// Starfield initialization across 800x600 bounds
void starfieldCreation(vector<Star> &myStars)
{
    myStars.clear();
    // creates random star locations within the s[eed gives them a random velocity
    // stores the required stuff in vector array

    for (int i = 0; i < 300; i++)
    {
        float x = static_cast<float>(rand() % 800);
        float y = static_cast<float>(rand() % 600);
        float speed = (1.0f + static_cast<float>(rand() % 30) / 10.0f);
        uint8_t brightness = static_cast<uint8_t>(100 + rand() % 155);
        myStars.push_back({{x, y}, speed, Color(brightness, brightness, brightness)});
    }
}

// Background star movement reset bound at 600px height
// implemented delta-time for movement so it is independent of fps (Haras)
void starfieldBackground(vector<Star> &myStar, VertexArray &starfield, float dt)
{
    starfield.clear();
    // clear the previious vertex array so a new can be loaded
    // change the position with updating new height and looping star back to front when it reaches end of the screen
    // checks vector array for the position and also updates it their
    for (auto &s : myStar)
    {
        // implemented delta-time for movement so it is independent of fps (Haras)
        s.position.y += s.speed * dt * 30.0f;
        if (s.position.y > 600.f)
        {
            s.position.y = 0.f;
            s.position.x = static_cast<float>(rand() % 800);
        }
        starfield.append(Vertex{s.position, s.color});
    }
}

void keys(RenderWindow &window, const Font &font, const string &newKey, Vector2f pos, Vector2f keySize, bool isPressed)
{
    float offSet = isPressed ? 3.f : 0.f; // Using this instead of messsy if

    // actual key being created
    // box surrounding it
    RectangleShape keyCreated({keySize});
    keyCreated.setOrigin({keySize.x / 2.f, keySize.y / 2.f});

    keyCreated.setPosition({pos.x, pos.y + offSet});
    keyCreated.setFillColor(isPressed ? Color(60, 60, 20) : Color(20, 20, 25));
    keyCreated.setOutlineThickness({3.f});

    keyCreated.setOutlineColor(isPressed ? Color::Yellow : Color::White); // this allowes to change color when key press of the border

    Text key = createCenteredText(font, newKey, 20, {pos.x, pos.y + offSet - 2.f}, isPressed ? Color::Yellow : Color::White);

    window.draw(keyCreated);
    window.draw(key);
    // draw in right sequence ;
}

// Rendering all the stuff for menu
// remove the stuff from here like the death rectangle bring it in main so it can be used in other functions
void rendering(RenderWindow &window, const VertexArray &starfield, GameState currentState,
               const Text &titleText, const vector<Text> &mainMenuItems, vector<Text> &deathMenuItems,
               const vector<Text> &startMenuItems, const vector<Text> &pauseMenuItems, const Font &font, const string &currentDifficulty, float soundVolume, float musicVolume, float shakeIntensity,
               RectangleShape &dimBackground, const Sprite &playerSprite, int currentHeart, const DebugState &debug)
{
    // rendering will now render live percentage of SF, BGM, SI by updating static 100% string to dynamically display variable (Haras),
    //  it has been edited for game end screen overhaul (Haras)
    // it has also been edited for DEBUG meny (Haras)

    if (currentState == GameState::Paused)
    {
        dimBackground.setFillColor(Color(0, 0, 0, 150));
        window.draw(dimBackground);
    }
    else if (currentState == GameState::Death)
    {
        window.clear(Color::Black); // game end screen overhaul
    }
    else
    {
        window.clear(Color(10, 10, 15));
        window.draw(starfield);
    }

    if (currentState == GameState::Menu)
    {
        window.draw(titleText);
        // print all items in start menu
        for (const auto &item : mainMenuItems)
        {
            window.draw(item);
        }
    }
    else if (currentState == GameState::StartMenu)
    {
        Text subTitle = createCenteredText(font, "PRE-GAME SETUP", 40, {400.f, 130.f}, Color::Yellow);
        window.draw(subTitle);
        // Prints all items in start menu
        for (const auto &item : startMenuItems)
        {
            window.draw(item);
        }
    }
    // Prints the tutorial examples , (use better graphics by images/ texture)
    else if (currentState == GameState::Tutorial)
    {
        Text title = createCenteredText(font, "HOW TO PLAY", 40, {400.f, 120.f}, Color::Yellow);

        // checking for each case individually spacoing of 55 in between
        keys(window, font, "W", {225.0f, 200.0f}, {45.f, 45.f}, Keyboard::isKeyPressed(Keyboard::Key::W));
        keys(window, font, "S", {225.0f, 255.0f}, {45.f, 45.f}, Keyboard::isKeyPressed(Keyboard::Key::S));
        keys(window, font, "D", {280.0f, 255.0f}, {45.f, 45.f}, Keyboard::isKeyPressed(Keyboard::Key::D));
        keys(window, font, "A", {170.f, 255.0f}, {45.f, 45.f}, Keyboard::isKeyPressed(Keyboard::Key::A));

        keys(window, font, "Space", {550.f, 255.f}, {160.f, 45.f}, Keyboard::isKeyPressed(Keyboard::Key::Space));
        Text movementControls = createCenteredText(font, "MOVEMENT", 25, {225.f, 350.f}, Color::White);

        Text W = createCenteredText(font, "UP", 15, {225.f, 160.f}, Color(255, 255, 255, 200));
        Text A = createCenteredText(font, "LEFT", 15, {110.f, 255.f}, Color(255, 255, 255, 200));
        Text S = createCenteredText(font, "DOWN", 15, {225.f, 300.f}, Color(255, 255, 255, 200));
        Text D = createCenteredText(font, "RIGHT", 15, {340.f, 255.f}, Color(255, 255, 255, 200));
        Text parry = createCenteredText(font, "PARRY/SHOOT", 25, {550.f, 350.f}, Color::White);

        Text escapeText = createCenteredText(font, "Press ESC To Return", 25.f, {400.f, 500.f}, Color::Cyan);
        window.draw(W);
        window.draw(A);
        window.draw(S);
        window.draw(D);
        window.draw(parry);
        window.draw(escapeText);
        window.draw(movementControls);

        window.draw(title);
    }
    else if (currentState == GameState::Shop)
    {
        // shop Title
        Text ShopTitle = createCenteredText(font, "HEART SKINS ", 30.f, {400.f, 150.f}, Color::Yellow);
        window.draw(ShopTitle);
        Sprite previewHeart = playerSprite;
        previewHeart.setScale({playerSprite.getScale().x * 2.f, playerSprite.getScale().y * 2.f});
        previewHeart.setPosition({400.f, 270.f});
        window.draw(previewHeart);
        string skinName;

        if (currentHeart == 0)
            skinName = "Red";
        else if (currentHeart == 1)
            skinName = "Golden";
        else if (currentHeart == 2)
            skinName = "Blue";
        else if (currentHeart == 3)
            skinName = "Green";
        else if (currentHeart == 4)
            skinName = "Pink";
        else
            skinName = "Purple";

        Text skinTitle = createCenteredText(font, "<    " + skinName + "    >", 30.f, {400.f, 300.f}, Color::White);

        Text controls = createCenteredText(font, "LEFT / RIGHT KEY TO SWITCH BETWEEN KEYS", 30.f, {400.f, 400.f}, Color::Yellow);
        window.draw(skinTitle);
        window.draw(controls);
    }
    else if (currentState == GameState::Paused)
    {
        Text pausetitle = createCenteredText(font, "PAUSED", 40, {400.f, 130.f}, Color::Yellow);
        window.draw(pausetitle);
        for (const auto &item : pauseMenuItems)
        {
            window.draw(item);
        }
    }
    else if (currentState == GameState::Settings)
    {
        // update static 100 % string to dynamically display variable deals with both SF, BGM, SI (Haras)
        string settingsStr = " SETTINGS MENU \n\n[ Sound: " + to_string(static_cast<int>(soundVolume)) + "% ] <- Left/Right \n[ BGM: " + to_string(static_cast<int>(musicVolume)) + "% ] <- Up/Down\n[ Shake: " + to_string(static_cast<int>(shakeIntensity)) + " ] <- W/S\n[ Debug Mode ] <- Press TAB\n\n[Controls:WASD + MOUSE]\n\n(Press ESC for Menu)";
        Text settingsText = createCenteredText(font, settingsStr, 22, {400.f, 300.f}, Color::White);
        window.draw(settingsText);
    }
    // This is all for debug menu implementation (Haras)
    else if (currentState == GameState::Debug)
    {
        if (!debug.unlocked)
        {
            Text title = createCenteredText(font, "DEBUG ACCESS", 38, {400.f, 160.f}, Color::Yellow);
            Text prompt = createCenteredText(font, "Enter Password (Press ENTER):", 22, {400.f, 240.f}, Color::White);

            string masked(debug.input.size(), '*');
            Text passText = createCenteredText(font, "[ " + masked + "_ ]", 28, {400.f, 305.f}, Color::Cyan);

            window.draw(title);
            window.draw(prompt);
            window.draw(passText);

            if (debug.wrongPassword)
            {
                Text err = createCenteredText(font, "WRONG PASSWORD", 20, {400.f, 365.f}, Color(255, 50, 50));
                window.draw(err);
            }

            Text back = createCenteredText(font, "(Press ESC to Return)", 20, {400.f, 450.f}, Color(180, 180, 180));
            window.draw(back);
        }
        else
        {
            Text title = createCenteredText(font, "DEBUG - SELECT PHASE", 34, {400.f, 150.f}, Color::Yellow);
            string phases = "[ 1 ] Phase 1 - Boss Waves\n\n[ 2 ] Phase 2 - Lava Track\n\n[ 3 ] Phase 3 - Parry Boss\n\n(Press ESC to Return)";
            Text phaseText = createCenteredText(font, phases, 24, {400.f, 330.f}, Color::White);
            window.draw(title);
            window.draw(phaseText);
        }
    }
    else if (currentState == GameState::Death)
    { // game end screen overhaul (Haras)
        // central spotlight implemented here (Haras)
        float beamtop = 140.f, beamheight = 305.f;
        RectangleShape beamouter({190.f, beamheight});
        beamouter.setOrigin({95.f, 0.f});
        beamouter.setPosition({400.f, beamtop});
        beamouter.setFillColor(Color(30, 8, 14));
        window.draw(beamouter);

        RectangleShape beammid({136.f, beamheight});
        beammid.setOrigin({68.f, 0.f});
        beammid.setPosition({400.f, beamtop});
        beammid.setFillColor(Color(44, 12, 20));
        window.draw(beammid);

        RectangleShape beaminner({82.f, beamheight});
        beaminner.setOrigin({41.f, 0.f});
        beaminner.setPosition({400.f, beamtop});
        beaminner.setFillColor(Color(58, 16, 26));
        window.draw(beaminner);

        // floor of spotlight and drop shadow (Haras)
        CircleShape padouter(95.f, 60);
        padouter.setOrigin({95.f, 95.f});
        padouter.setPosition({400.f, 445.f});
        padouter.setScale({1.0f, 0.38f});
        padouter.setFillColor(Color(90, 30, 44));
        window.draw(padouter);

        CircleShape padinner(70.f, 60);
        padinner.setOrigin({70.f, 70.f});
        padinner.setPosition({400.f, 445.f});
        padinner.setScale({1.0f, 0.38f});
        padinner.setFillColor(Color(110, 38, 54));
        window.draw(padinner);

        CircleShape heartShadow(20.f, 30);
        heartShadow.setOrigin({20.f, 20.f});
        heartShadow.setPosition({400.f, 453.f});
        heartShadow.setScale({1.0f, 0.35f});
        heartShadow.setFillColor(Color(40, 10, 18));
        window.draw(heartShadow);

        // player sprite on the spotlight stage (Haras)
        Sprite centerheart = playerSprite;
        centerheart.setScale({1.f, 1.f});
        centerheart.setPosition({400.f, 435.f});
        window.draw(centerheart);

        // game over and try again display (Haras)
        Text gameword = createCenteredText(font, "GAME", 78, {400.f, 105.f}, Color::White);
        gameword.setStyle(Text::Bold);
        gameword.setScale({1.2f, 1.0f});
        window.draw(gameword);
        Text overword = createCenteredText(font, "OVER", 78, {400.f, 195.f}, Color(255, 40, 40));
        overword.setStyle(Text::Bold);
        overword.setScale({1.2f, 1.0f});
        window.draw(overword);
        // encourage trying again (Haras)
        Text tryagain = createCenteredText(font, "Try Again?...", 22, {400.f, 305.f}, Color::White);
        window.draw(tryagain);
        // making the buttons aesthetically pretty (Haras)
        float buttoncentersX[2] = {185.f, 615.f};
        float lineyoffsets[2] = {440.f, 490.f};
        for (float cx : buttoncentersX)
        {
            for (float ly : lineyoffsets)
            {
                RectangleShape dotL({3.f, 2.f});
                dotL.setPosition({cx - 88.f, ly});

                RectangleShape dashL({10.f, 2.f});
                dashL.setPosition({cx - 78.f, ly});
                RectangleShape midBar({116.f, 2.f});
                midBar.setPosition({cx - 58.f, ly});
                RectangleShape dashR({10.f, 2.f});
                dashR.setPosition({cx + 68.f, ly});
                RectangleShape dotR({3.f, 2.f});
                dotR.setPosition({cx + 85.f, ly});

                window.draw(dotL);
                window.draw(dashL);
                window.draw(midBar);
                window.draw(dashR);
                window.draw(dotR);
            }
        }

        for (const auto &item : deathMenuItems)
        {
            window.draw(item);
        }
    }
}

// Player movement function with box containment (Bazil comments in this function)
void handlePlayerMovement(Sprite &playerSprite, float speed, const RectangleShape &battleBox, float dt, bool useBox)
{
    // we need the sprite, the speed and the rectangle box to trap the player to prevent excess movement
    Vector2f pos = playerSprite.getPosition(); // apparently the SFML library already has x and y stored so it doesn't need you to declare them. Cool ig

    // ok so phase 2 was being difficult and I wanted to be a bit kinder to the players so Im making the changes that the up and down speed is slower compared ot the left and right speed, no need to (not) thank me (Bazil)
    // but it got too slow (0.6f) so i upped it to 0.8
    float verticalSpeed = speed;
    if (!useBox) //
    {
        verticalSpeed = speed * 0.8f;
    }

    // the movement stuff is self explanatory except one thing which I wrote down
    if (Keyboard::isKeyPressed(Keyboard::Scan::S))
    {
        pos.y += verticalSpeed * dt * 30.0f; // so over here, the tutorial told me that y actually increases as we go down. Be careful of this omg I hate SFML
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::W))
    {
        pos.y -= verticalSpeed * dt * 30.0f;
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::D))
    {
        pos.x += speed * dt * 30.0f;
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::A))
    {
        pos.x -= speed * dt * 30.0f;
    }

    // exact explanation at the bottom
    Vector2f boxPosition = battleBox.getPosition(); // obtaining the exact coordinates of the box centre
    Vector2f boxSize = battleBox.getSize();         // getting the width and height of the box because I tried to define the height and width
    // directly into the code itself and I realized that I might neeed to change the box dimensions so I used getSize instead. Just to give myself some leeway
    float left = boxPosition.x - (boxSize.x / 2.0f) + 8.0f;
    float right = boxPosition.x + (boxSize.x / 2.0f) - 25.0f;
    float top = boxPosition.y - (boxSize.y / 2.0f) + 8.0f;
    float bottom = boxPosition.y + (boxSize.y / 2.0f) - 8.0f;

    if (!useBox)
    {
        // for phase 2 cuz we gotta allow player to move whereever fr
        left = 8.0f;
        right = 800.0f - 8.0f;
        top = 8.0f;
        bottom = 600.0f - 8.0f;
    }

    if (pos.x < left)
        pos.x = left;
    if (pos.x > right)
        pos.x = right;
    if (pos.y < top)
        pos.y = top;
    if (pos.y > bottom)
        pos.y = bottom;

    playerSprite.setPosition(pos);
    // ok so basically I wanted to do the Undertale box thing where the player is trapped inside a box. Initially my logic included that if my sprites position and the box line position was the same
    //(next line)then the player position would be set to the box position. But I noticed that the half the heart was actually clipping through, so I added a 12 pt barrier which was too large and then
    //(next line) so I experimented a bit and found that 8 was a good value
}

// New fuunction for attack
// attack spawns at te top and slowly comes down
void verticle(vector<Projectile> &pillars, float xposition, int difficultyIndex)
{
    // created a new varible of type projectile assign it its attrigbutes position updated in update projectile function
    Projectile magic;
    //-ev position so it seems they spawn
    magic.position = {xposition, -150.f};
    // difficulty changes speed
    magic.velocity = {0.0f, 12.f + 5 * (difficultyIndex - 1)};
    magic.isLaunched = true;
    magic.tracksPlayer = false;
    magic.leavesTrail = false;
    magic.hitBox = 50.f;
    pillars.push_back(magic);
}

void burst(vector<Projectile> &machineGun, int difficultyIndex, Vector2f bossPos, float launchInterval)
{
    // creating 20 randomly created crystals to remove predictability
    for (int i = 0; i < 13 + 5 * (difficultyIndex - 1); i++)
    {
        // new projectile to push to vector
        Projectile crystal;
        crystal.position = bossPos;
        crystal.isLaunched = true;
        crystal.leavesTrail = false;
        crystal.tracksPlayer = false;
        crystal.hitBox = 12.0;
        // so it doesnt shoot al at once
        crystal.spawnDelay = i * launchInterval / 4.f;
        // random speeds and angle

        float angle;
        // setting angle to cover the field setting rotation setting speed and calculating velocity ;
        angle = 28.f + rand() % 125;
        crystal.rotation = angle - 90;
        float speed;
        speed = 5.0f + rand() % 5 + 3.f * (difficultyIndex - 1);
        angle = angle * 3.14157f / 180.0;
        crystal.velocity = {speed * cos(angle), speed * sin(angle)};
        // new element
        machineGun.push_back(crystal);
    }
}

// The first wave stuff, crescent bullets (Bazil comments in this function)
// bullets need to be launched more frequently
void crescent(vector<Projectile> &activeProjectiles, float crescentcentre, float spawnposition, float spawnInterval, float launchInterval)
{
    int totalBullets = 7;
    float totalSpawnTime = (totalBullets - 1) * spawnInterval;
    for (int i = 0; i < totalBullets; i++)
    {
        float step = 1.0f / (totalBullets - 1);
        float horizontalshift = -0.5f + (i * step);                           // shift for each bullet
        float horizontalspawn = crescentcentre + (horizontalshift * 180.0f);  // basically the spawn position, change the 180 to a higher number to create a bigger spread and vice versa
        float arcDip = (0.25f - (horizontalshift * horizontalshift)) * 60.0f; // same logic as above one
        Vector2f spawnPos = {horizontalspawn, spawnposition - arcDip};        // basically the position it spawns at, I hope this doesn't need an explanation
        Projectile proj;
        proj.position = spawnPos;
        proj.isLaunched = false;
        proj.spawnDelay = i * spawnInterval;

        // Staggers each bullet sequentially using the local interval
        proj.launchTimer = (totalSpawnTime - proj.spawnDelay) + launchInterval + (i * 1.0f); // the 1.0f is CUZ OMG I HAD TO DIE AND DIE AGAIN JUST TO PUT THIS INTERVAL IN OMG I HATE SFML

        proj.tracksPlayer = true;
        proj.leavesTrail = false;

        activeProjectiles.push_back(proj);
    }
}

// The second wave stuff, horizontal projectiles (Bazil comments in this function)
void horizontal(vector<Projectile> &activeProjectiles, const RectangleShape &battleBox, float projectileSpeed, float projectileposition)
{
    Vector2f boxP = battleBox.getPosition(); // boxPosition too large a name and im lazeh :P
    Vector2f boxSize = battleBox.getSize();  // same thing

    float height = boxP.y + projectileposition; // basically the height at which the projectile spawns, we HATE this function
    float left = boxP.x - boxSize.x / 2.0f - 120.0f;
    float right = boxP.x + boxSize.x / 2.0f + 120.0f;
    // ok so I added the +90 and -90 because initially I dind't have them and the projectiles were spawning like RIGHT on the border and it was so weird ew. So I added the +90 and -90
    Projectile pLeft;
    pLeft.position = {left, height + 20.0f};
    pLeft.velocity = {projectileSpeed, 0.0f};
    pLeft.leavesTrail = true;
    pLeft.launchTimer = 0.8f;

    Projectile pRight;
    pRight.position = {right, height};
    pRight.velocity = {-projectileSpeed, 0.0f};
    pRight.leavesTrail = true;
    pRight.launchTimer = 0.8f;

    activeProjectiles.push_back(pLeft);
    activeProjectiles.push_back(pRight);
}

// This is the attack where the projectiles spawn in a circle and burst outwards (Bazil comments in this function)
void pinwheel(vector<Projectile> &activeProjectiles, Vector2f centerPos, float projectileSpeed)
{
    int numBullets = 8;
    for (int i = 0; i < numBullets; i++)
    {                                                 // this is basically the loop for each of the bullets(pretty self explanatory)
        float angle = (2.0f * 3.14 * i) / numBullets; // ok so over here, I used the cmath library for the circle angles because its the easiest way Lord praise "Basic Coding" yt channel and their showcase
        //(next line) of cmath functions
        // the logic for the angle is pretty simple, so basically what im doing is im using radians for the angle of each projectile seperately
        // 2𝝅 is the total angle and I multiply it by i to get the angle of each seperate projectile. So for the first projectile, i is 0 so angle comes out to be 0, 2nd projectile i is 1 so 2𝝅*1/8 = 𝝅/4 or 45 degrees. Rest is simple
        Vector2f dir = {cos(angle), sin(angle)}; // simple SOHCAHTOA stuff im not explaining this

        Projectile proj;
        proj.position = centerPos;
        proj.velocity = dir * (projectileSpeed * 0.75f);
        proj.isLaunched = false;
        proj.launchTimer = 1.0f;
        proj.tracksPlayer = false;
        proj.leavesTrail = false;
        activeProjectiles.push_back(proj);
    }
}

// for the 4th attack, the homing projectiles (Bazil comments in this function, unless specified otherwise)
void homingdarts(vector<Projectile> &activeProjectiles, const RectangleShape &battleBox, float centerX)
{
    Vector2f boxSize = battleBox.getSize(); // simple enough, obtaining size, copy pasted this mweheheh (FROM THE CODE ABOVE NOT AI OMG)

    for (int i = 0; i < 4; i++)
    {
        Projectile proj;
        float horizontallocation = -60.0f + i * 40.0f;                                                        // its basically where the projectile spawns in the horizontal position, -60 being the initial one for the first projectile
        proj.position = {centerX + horizontallocation, battleBox.getPosition().y - boxSize.y / 2.0f - 15.0f}; // initial projectile location
        proj.isLaunched = false;
        proj.launchTimer = 0.7f + (i * 0.2f); // launched at intervals with i
        proj.tracksPlayer = true;             // well like, duh
        proj.leavesTrail = false;             // no trail for this one
        proj.steeringtime = 1.2f;             // steer projectile for 1.2 sceonds and then it flies of screen, //this is for homing projectiles (Haras)
        activeProjectiles.push_back(proj);
    }
}

// this is to check if the projectiles of the wave have launched because without this, my first crescent had just started firing and the next one would start spawning like holy(Bazil)
bool allLaunched(const vector<Projectile> &activeProjectiles)
{
    for (const auto &proj : activeProjectiles)
    {
        if (proj.isLaunched == false || proj.spawnDelay > 0.0f)
        {
            return false;
        }
    }
    return true;
}

// update projectiles but it makes sense to me brrrrr(Bazil comments in this function)
void updateProjectiles(vector<Projectile> &activeProjectiles, float t, unsigned int width, unsigned int height, Vector2f playerPos, float projectileSpeed)
{
    for (int i = 0; i < (int)activeProjectiles.size();)
    {                                         // omg I tried doing just activeProjectiles.size() initially and it gave me an error, apparently the proper format is encasing the int in brackets and then it worked. The more I know ig
        Projectile &p = activeProjectiles[i]; // p is the current projectile in the list, we go through them with i

        // projectile hasn't appeared yet, just count down its spawn delay and skip everything else for the next frame
        if (p.spawnDelay > 0.0f)
        {
            p.spawnDelay -= t;
            if (p.spawnDelay > 0.0f)
            {
                i++;
                continue;
            }
        }
        if (!p.isLaunched)
        {                       // if its NOT launched
            p.launchTimer -= t; // subtraction from the launch timer, its also why we subtracted from the spawnDelay above
            if (p.launchTimer <= 0.0f)
            { // has the timer hit 0? let the projectile out the prison :(
                if (p.tracksPlayer)
                { // is it gonna home in?
                    Vector2f direction = {playerPos.x - p.position.x, playerPos.y - p.position.y};
                    float length = sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length != 0.0f)
                    {
                        direction /= length; // to give a unit vector so it doesn't fly at mach speed (unless I change this and decide to troll the players ig)
                    }
                    float launchspeed = (p.steeringtime > 0.0f) ? projectileSpeed * 0.5f : projectileSpeed;
                    p.velocity = direction * launchspeed;
                }
                p.isLaunched = true; // its out of the prison yay
            }
        }
        else
        { // projectile is launched
            // implemented delta-time, also implemented homing missles (Haras)
            if (p.steeringtime > 0.0f)
            {
                p.steeringtime -= t;
                Vector2f desired = {playerPos.x - p.position.x, playerPos.y - p.position.y};
                float desiredlength = sqrt(desired.x * desired.x + desired.y * desired.y);
                if (desiredlength > 0.0f)
                {
                    desired /= desiredlength;
                    float dartspeed = projectileSpeed * 0.5f;
                    float turnrate = 1.5f; // if you lower this value then subtler curve, higher then more accurate homing
                    p.velocity += desired * (dartspeed * turnrate * t);
                    float curvespeed = sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
                    if (curvespeed > 0.0f)
                    {
                        p.velocity = (p.velocity / curvespeed) * dartspeed;
                    }
                }
            }
            p.position += p.velocity * t * 30.0f; // takes your point and adds the velocity onto it every frame which moves it
            // if this projectile leaves a trail, insert the trail every frame
            if (p.leavesTrail)
            {
                p.trailTimer += t;
                if (p.trailTimer >= 0.03f)
                {
                    p.trailTimer = 0.0f;
                    p.trail.push_back({p.position, 1.0f});
                }
            }
        }

        // age the trail points
        for (int j = 0; j < (int)p.trail.size();)
        {
            p.trail[j].lifeTimer -= t;
            if (p.trail[j].lifeTimer <= 0.0f)
            {
                p.trail.erase(p.trail.begin() + j); // remove the trail after its lifetime is done
            }
            else
            {
                j++;
            }
        }
        bool isOffScreen = (p.position.y > height + 40 || p.position.y < -200 || p.position.x < -100 || p.position.x > width + 100);
        if (isOffScreen && p.isLaunched)
        {
            activeProjectiles.erase(activeProjectiles.begin() + i); // remove the projectile once it goes offscreen, rip my laptop holy lag
        }
        else
        {
            i++;
        }
    }
}

// Map collisions go brrrrrrrrrrr heahehaheahehaheha
bool mapcollission(const sf::Image &mapimage, const sf::Sprite &mapsprite, Vector2f playerpos)
{
    Vector2f mapposition = mapsprite.getPosition();                      // map centre
    sf::Vector2u imagesize = mapimage.getSize();                         // map size
    int pixelx = (playerpos.x - mapposition.x) / mapsprite.getScale().x; // player positnon with respect to x cuz yeah
    int pixely = (playerpos.y - mapposition.y) / mapsprite.getScale().y; // player positnon with respect to y cuz yeah
    if (pixelx < 0 || pixelx >= (int)imagesize.x || pixely < 0 || pixely >= (int)imagesize.y)
    {
        return true;
    }
    Color pixelcolour = mapimage.getPixel({static_cast<unsigned int>(pixelx), static_cast<unsigned int>(pixely)});
    bool stonePath = (pixelcolour.r < 200 && pixelcolour.b > 50);
    return !stonePath;
}

// For the collisions with projectiles logic here we go brrrrrrrrrrrr (Bazil comments in this function)
bool collision(const vector<Projectile> &activeProjectiles, Vector2f playerPos)
{
    for (const auto &proj : activeProjectiles)
    {
        if (proj.isLaunched && !proj.parried)
        {                                                // well like, yeah. It should only run if its launched , also shouldnot work if parried
            float distX = playerPos.x - proj.position.x; // basically the horizontal distance between the projectile and the player
            float distY = playerPos.y - proj.position.y; // same thing but vertical
            if (sqrt(distX * distX + distY * distY) < proj.hitBox)
            { // pythagoras theorem shEEEESh
                return true;
            }
        }

        for (const auto &hazard : proj.trail)
        { // same thing over here but for the trail
            float distX = playerPos.x - hazard.position.x;
            float distY = playerPos.y - hazard.position.y;
            if (sqrt(distX * distX + distY * distY) < proj.hitBox)
            {
                return true;
            }
        }
    }
    return false;
}

// New line of code for boss proejctiles only runs if the object has been parried
bool bossCollision(const vector<Projectile> &activeProjectiles, Vector2f bossPos)
{
    for (const auto &proj : activeProjectiles)
    {
        if (proj.parried == true)
        {
            float dx = bossPos.x - proj.position.x;
            float dy = bossPos.y - proj.position.y;
            if (sqrt(dx * dx + dy * dy) < proj.hitBox)
            {
                return true;
            }
        }
    }
    return false;
}

// phase 3 boss throw attack (Haras)
void bossThrow(vector<Projectile> &activeProjectiles, Vector2f bossPos)
{
    Projectile proj;
    proj.position = bossPos;
    proj.isLaunched = false;
    proj.launchTimer = 0.4f; // short charge telegraph before throwing
    proj.tracksPlayer = true;
    proj.leavesTrail = false;
    proj.hitBox = 12.f;
    activeProjectiles.push_back(proj);
}

void parryProj(vector<Projectile> &Projectile, Vector2f playerPos, Vector2f bossPos)
{
    for (auto &proj : Projectile)
    {
        if (proj.isLaunched && !proj.parried)
        {
            float dx = playerPos.x - proj.position.x;
            float dy = playerPos.y - proj.position.y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist <= 45.f && dy >= -15.f)
            {
                proj.tracksPlayer = false;
                proj.steeringtime = 0.0f;

                Vector2f bossDisplacement = {bossPos.x - proj.position.x, bossPos.y - proj.position.y};
                float length = sqrt(bossDisplacement.x * bossDisplacement.x + bossDisplacement.y * bossDisplacement.y);
                if (length != 0.0f)
                {
                    bossDisplacement /= length;
                }
                proj.velocity = bossDisplacement * 25.0f;
                proj.parried = true;
            }
        }
    }
}

// new function to check for parrying
void updateParrying(Sprite &blazeRod, const Sprite &playerSprite, Vector2f bossPos, const Sprite &icon1,
                    CircleShape &cooldownRing, RectangleShape &cover,
                    vector<Projectile> &activeProjectiles, vector<Projectile> &crystalBurst,
                    bool &isParrying, float &parryTimer, float &cooldown, float dt)
{
    blazeRod.setPosition({playerSprite.getPosition().x + 5.f, playerSprite.getPosition().y + 6.0f});

    if (isParrying)
    {
        parryTimer += dt;

        float progress = parryTimer / 0.15f;
        parryProj(activeProjectiles, playerSprite.getPosition(), bossPos);
        if (progress >= 1.0f)
        {
            isParrying = false;
            blazeRod.setRotation(degrees(0));
            cooldown = 0.9f;
        }
        else
        {
            float newAngle = -140.f * progress;
            blazeRod.setRotation(degrees(newAngle));
        }
    }
    if (cooldown > 0.0f)
    {
        cooldown -= dt;
        blazeRod.setColor(Color(120, 120, 120, 100));
        float ratio = max(0.0f, cooldown / 0.9f);
        cover.setPosition({icon1.getPosition().x, icon1.getPosition().y + (40.f - ratio * 40.f)});
        cover.setFillColor(Color::Black);
        cooldownRing.setOutlineColor(Color(255, static_cast<uint8_t>(255.f * (1.0f - ratio)), 0));
    }
    else
    {
        blazeRod.setColor(Color::White);
        cover.setFillColor(Color::Transparent);
        cover.setPosition(icon1.getPosition());
        cooldownRing.setOutlineColor(Color::Cyan);
    }
}

int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    unsigned int width = 800;
    unsigned int height = 600;

    RenderWindow window(VideoMode({width, height}), "Bullet Requiem");
    window.setFramerateLimit(60); // increased fps from 30 to 60 for smoother gameplay (Haras)

    View view(FloatRect({0.0f, 0.0f}, {static_cast<float>(width), static_cast<float>(height)}));
    window.setView(view);

    // Creating vector arrays to store info about the stars falling in back ground
    // Then call void function to initilaize the vector
    vector<Star> myStars;
    starfieldCreation(myStars);
    // vertex array used in background since it is much more efficient
    VertexArray starBackground(PrimitiveType::Points);

    vector<string> heartFiles = {
        "Assets/RedHeart.png",
        "Assets/GoldenHeart.png",
        "Assets/BlueHeart.png",
        "Assets/GreenHeart.png",
        "Assets/PinkHeart.png",
        "Assets/PurpleHeart.png"};

    float scaling[6] = {0.6, 0.6, 0.42, 0.6, 0.6, 0.50};

    // Load Textures & Fonts
    Texture hearts;
    if (!hearts.loadFromFile(heartFiles[0]))
    {
        return -1;
    }
    hearts.setSmooth(false);
    int currentHeart = 0;

    Font font;
    if (!font.openFromFile("Assets/font.ttf"))
    {
        return -1;
    }
    Texture texture2;
    // resize this pls
    if (!texture2.loadFromFile("Assets/frostattack.png"))
    {
        return -1;
    }
    Texture texture3;
    if (!texture3.loadFromFile("Assets/boss_sprite_phase1.png"))
    {
        return -1;
    }
    // for phase3 (Haras)
    Texture bossphase3tex;
    if (!bossphase3tex.loadFromFile("Assets/boss_sprite_phase3.jpg"))
    {
        return -1;
    }
    Texture bossStage2;
    if (!bossStage2.loadFromFile("Assets/bossStage2.png"))
    {
        return -1;
    }

    Texture bossFinalStage;
    if (!bossFinalStage.loadFromFile("Assets/bossFinalStage.png"))
    {
        return -1;
    }

    // cool phase 1 background (Haras)
    Texture bgPhase1Tex;
    if (!bgPhase1Tex.loadFromFile("Assets/bg_phase1.jpeg"))
    {
        return -1;
    }
    // scaling the background (Haras)
    Sprite bgPhase1Sprite(bgPhase1Tex);
    Vector2u bgPhase1Size = bgPhase1Tex.getSize();
    bgPhase1Sprite.setScale({static_cast<float>(width) / bgPhase1Size.x, static_cast<float>(height) / bgPhase1Size.y});

    Text speech(font);
    speech.setCharacterSize(12);
    speech.setFillColor(Color::Black);

    // Title text set up centered at x=400, y=120;
    Text titletext(font, "BULLET REQUIEM", 55);
    FloatRect titleBounds = titletext.getLocalBounds();
    titletext.setFillColor(Color(255, 40, 40));
    titletext.setStyle(Text::Bold);
    titletext.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y / 2.f});
    titletext.setPosition({400.0f, 120.0f});

    // Further work on Pause screen
    RectangleShape DimScreen(Vector2f(window.getSize()));
    DimScreen.setFillColor(Color(0, 0, 0, 150));

    // Set the current state to menu so that when we enter game we at the menu
    GameState currentState = GameState::Menu;
    int hoveredIndex = -1;

    // Main Menu Setup
    vector<string> mainOptions = {"START GAME", "SETTINGS", "EXIT"};
    vector<Text> mainMenuItems = createMenuItems(font, mainOptions, 280.f, 65.f, 30);
    // Did this to move tutorial on main page
    mainOptions.push_back("TUTORIAL");
    // using [0] because of create menu item returns vector text
    mainMenuItems.push_back(createMenuItems(font, {mainOptions[3]}, 550.f, 65.f, 30.f, 700.f)[0]);

    mainOptions.push_back("SKINS");
    mainMenuItems.push_back(createMenuItems(font, {mainOptions[4]}, 550.f, 65.f, 30.f, 70.f)[0]);

    // Start Sub-Menu Setup (Difficulty & Tutorial)
    vector<string> difficulties = {"EASY", "NORMAL", "BULLET HELL"};
    int difficultyIndex = 1; // Default to NORMAL

    vector<string> startOptions = {
        "DIFFICULTY: " + difficulties[difficultyIndex],
        "ENDLESS",
        "NEW GAME",
        "BACK"};

    vector<Text> startMenuItems = createMenuItems(font, startOptions, 240.f, 60.f, 26);

    vector<string> pauseOptions = {"RESUME", "SETTINGS", "MAIN MENU", "EXIT GAME"};
    vector<Text> pauseMenuItems = createMenuItems(font, pauseOptions, 240.f, 60.f, 26);

    // for game end menu overhaul (Haras)
    vector<Text> deathMenuItems = {
        createCenteredText(font, "PERSEVERE", 22, {185.f, 465.f}, Color(255, 40, 40)),
        createCenteredText(font, "GIVE UP", 22, {615.f, 465.f}, Color::White)};
    GameState previousState = GameState::Menu;

    // Battle Box Setup
    RectangleShape battleBox({380.0f, 120.0f});
    battleBox.setFillColor(Color::Black);
    battleBox.setOutlineColor(Color::White);
    battleBox.setOutlineThickness(3.0f);
    battleBox.setOrigin({380.0f / 2.0f, 120.0f / 2.0f});
    battleBox.setPosition({width / 2.0f, height / 2.0f + 100.0f});

    // health bar (location changed, centering done (Haras))
    RectangleShape healthBar({300.f, 20.f});
    healthBar.setOrigin({150.f, 10.f});
    healthBar.setPosition({width / 2.0f, 510.0f});
    healthBar.setFillColor(Color(57, 255, 20));

    // health bar boundary and underneath (location changed, centering done (Haras))
    RectangleShape boundaryRect({300.0f, 20.0f});
    boundaryRect.setOrigin({150.f, 10.f});
    boundaryRect.setPosition({width / 2.0f, 510.0f});
    boundaryRect.setFillColor(sf::Color(190, 44, 35));
    boundaryRect.setOutlineThickness(4.f);
    boundaryRect.setOutlineColor(sf::Color(140, 90, 43));

    // bossHealth
    RectangleShape bossHealth({500.0f, 20.0f});
    bossHealth.setOrigin({250.0f, 10.0f});
    bossHealth.setPosition({400.0f, 100.0f});
    bossHealth.setFillColor(Color(155, 40, 230));

    // boss health bar underneath and border :
    RectangleShape healthBoundary({500.f, 20.f});
    healthBoundary.setOrigin({250.f, 10.f});
    healthBoundary.setPosition({400.f, 100.f});
    healthBoundary.setOutlineThickness(4.f);
    healthBoundary.setOutlineColor(Color(220, 25, 40));
    healthBoundary.setFillColor(Color(50, 180, 255));

    float cooldown = 0.0f; // cooldwon for next swing
    bool isParrying = false;
    float parryTimer = 0.0;      // how many seconds gone
    float parryDuration = 0.15f; // total swing duration

    Texture rod;
    if (!rod.loadFromFile("Assets/NewBlazeRod.png"))
    {
        return -1;
    }

    Sprite blazeRod(rod);
    blazeRod.setScale({0.37f, 0.37f});

    Vector2u rodSize = rod.getSize();
    // set to left corner
    blazeRod.setOrigin({0, float(rodSize.y)});
    blazeRod.setPosition({406.f, 306.f});

    // Boss Sprite Setup
    Sprite Bosssprite(texture3);
    FloatRect bossbounds = Bosssprite.getLocalBounds();
    Bosssprite.setScale({0.1f, 0.1f});
    Bosssprite.setOrigin({bossbounds.size.x / 2.f, bossbounds.size.y / 2.f});
    Bosssprite.setPosition({width / 2.0f, 240.0f});
    Text bossName = createCenteredText(font, "MEHMOOD SAHAB", 15, {400.f, 75.f}, Color(255, 255, 255, 200));

    // Texture magic/ new projectile;
    Texture magic;
    if (!magic.loadFromFile("Assets/Magic atack.png"))
    {
        return -1;
    }

    // the player's own shot texture/sprite (merged from the 1st code)
    Texture playerProj;
    if (!playerProj.loadFromFile("Assets/flame attack.png"))
    {
        return -1;
    }
    Sprite playerShot(playerProj);
    Vector2u shootSize = playerProj.getSize();
    playerShot.setScale({0.5f, 0.5f});
    playerShot.setOrigin({shootSize.x / 2.f, shootSize.y / 2.f});

    // magic attack sprite setup
    Sprite MagicAttack(magic);
    MagicAttack.setTexture(magic);
    MagicAttack.setScale({5.f, 5.f});
    Vector2u proj2size = magic.getSize();
    MagicAttack.setOrigin({proj2size.x / 2.0f, proj2size.y / 2.0f});
    MagicAttack.setPosition({200.f, 300.f});

    // Crystal Projectile that will be fired like a machine gun randomly
    Texture crystal;
    if (!crystal.loadFromFile("Assets/fallingCrystal.png"))
        return -1;

    Sprite fallingCrystal(crystal);
    fallingCrystal.setTexture(crystal);
    fallingCrystal.setScale({1.f, 1.f});
    Vector2u proj3size = crystal.getSize();
    fallingCrystal.setOrigin({proj3size.x / 2.0f, proj3size.y / 2.0f});

    // Player Sprite Setup
    Sprite Playersprite(hearts);
    FloatRect playerbounds = Playersprite.getLocalBounds();
    Playersprite.setScale({scaling[currentHeart], scaling[currentHeart]});
    Playersprite.setOrigin({playerbounds.size.x / 2.0f, playerbounds.size.y / 2.0f});
    Playersprite.setPosition(battleBox.getPosition());

    // playerIcon next to updated health bar location (Haras)
    Sprite playerIcon(hearts);
    playerIcon.setScale({scaling[currentHeart], scaling[currentHeart]});
    playerIcon.setOrigin({playerbounds.size.x / 2.f, playerbounds.size.y / 2.0f});
    playerIcon.setPosition({225.f, 510.f});

    // Indicator of parry with icon
    CircleShape cooldownRing({15.f});
    cooldownRing.setOrigin({15.f, 15.f});
    cooldownRing.setPosition(playerIcon.getPosition()); // setting origin as same so it covers uniformally
    cooldownRing.setFillColor(Color::Transparent);
    cooldownRing.setOutlineThickness({4.f});
    cooldownRing.setOutlineColor(Color::Cyan);

    // rectangle shape for covering the ring to show cooldown ;
    RectangleShape cover({40.f, 44.f});
    cover.setOrigin({20.f, 22.f});
    cover.setPosition(playerIcon.getPosition());
    cover.setFillColor(Color(120, 120, 30, 230));

    // standard speed
    float speed = 7.0f;

    // Projectile & Wave Setup
    vector<Projectile> activeProjectiles;
    vector<Projectile> magicPillar;
    vector<Projectile> crystalBurst;
    vector<Projectile> playershooting; // for the players projectiles

    // Change this
    float projectileSpeed = 20.0f;
    float launchInterval = 0.3f;

    // system clocks
    Clock waveTriggerClock;
    Clock frameClock;
    Clock clock;
    Clock magicTimer;
    Clock shootyclock; // for the projectiles (Bazil)
    Clock burstTimer;
    Clock phase3Clock; // phase three throw timer (Haras)

    float burstDelay = 10.0f;
    float waveInterval = 0.8f;
    int wavecount = 0;

    int currentphase = 1;              // the phase we're at, duh
    float invulnerabilitytimer = 0.0f; // invincibility frames cuz im generous :)

    bool passiveHeal = false; // boss passively heals once it drops low in phase 1 (reset on every restart)

    // Projectile sprite template
    Sprite projSprite(texture2);
    FloatRect projBounds = projSprite.getLocalBounds();
    projSprite.setScale({0.8f, 0.8f});
    projSprite.setOrigin({projBounds.size.x / 2.f, projBounds.size.y / 2.f});

    // The Map texture yayyy 2nd phase woohoo
    vector<string> mapFiles = {
        "Assets/map_track.png",
        "Assets/map_track2.png",
        "Assets/map_track3.png"};
    vector<Texture> mapTextures(mapFiles.size()); // store the textures in a vector (THIS IS SO COOL)
    vector<sf::Image> mapImages(mapFiles.size()); // need the images too for the pixels
    for (size_t i = 0; i < mapFiles.size(); i++)
    {
        if (!mapTextures[i].loadFromFile(mapFiles[i]) || !mapImages[i].loadFromFile(mapFiles[i]))
        { // loading each texture, outputting an error if it doesn't happen
            return -1;
        }
    }

    int mapindex = 0;                           // to cycle between the maps
    float mapbiggie = 1.4f;                     // map was too small when I initially used it soo yayy
    Sprite mapsprite(mapTextures[0]);           //
    mapsprite.setScale({mapbiggie, mapbiggie}); // heh, just scaling

    float halfwidth = mapTextures[2].getSize().x * 0.5f; // for map3 cuz it wasn't detecting the endpoint at the end of map 3

    vector<float> starter = {
        390.f,                             // for the first map
        mapTextures[1].getSize().x * 0.5f, // 2nd map
        halfwidth                          // 3rd map
    };

    vector<Vector2f> finishline = {
        {45.f, 115.f},                                                              // first map
        {mapTextures[1].getSize().x * 0.305f, mapTextures[1].getSize().y * 0.055f}, // 2nd map
        {halfwidth, mapTextures[2].getSize().y * 0.065f}                            // 3rd map
    };

    auto placemapstart = [&]() { // this is a function notation for when you wanna use functions in main. the & allows it to use all variables declared above it as referecne. omg tis so cool :3
        mapsprite.setTexture(mapTextures[mapindex], true);
        float texWidth = static_cast<float>(mapTextures[mapindex].getSize().x); // finds the pixel width of the map
        mapbiggie = static_cast<float>(width) / texWidth;                       // divides the screen width by pixel width
        mapsprite.setScale({mapbiggie, mapbiggie});                             // makes sure the zooming in doesn't cut parts of the map out dyring scaling
        float mapheight = mapsprite.getGlobalBounds().size.y;
        mapsprite.setPosition({0.0f, height - mapheight}); // put the rest of the map above the screen
    };

    auto playerspawnmap = [&]()
    {
        placemapstart();
        Playersprite.setPosition({mapsprite.getPosition().x + starter[mapindex] * mapbiggie, height - 50.0f});
    };
    float invulnerabilitytimerMagicAttack = 0.0f;
    // created a diff for this 1 second immunity too much for this

    float soundVolume = 50.0f; // SF variable declaration, I lowered the default value (Haras)
    float musicVolume = 20.0f; // BGM variable dec (Haras)

    Music bgMusic;
    string currentMusicTrack = "";

    auto playMusicTrack = [&](const string &trackPath)
    {
        if (trackPath == currentMusicTrack)
        {
            if (bgMusic.getStatus() != SoundSource::Status::Playing && !trackPath.empty())
            {
                bgMusic.setVolume(musicVolume);
                bgMusic.play();
            }
            return;
        }

        currentMusicTrack = trackPath;
        bgMusic.stop();

        if (!trackPath.empty())
        {
            if (bgMusic.openFromFile(trackPath))
            {
                bgMusic.setVolume(musicVolume);
                bgMusic.setLooping(true);
                bgMusic.play();
            }
            else
            {
                cout << "FAILED TO LOAD MUSIC: " << trackPath << endl;
            }
        }
    };

    playMusicTrack("Assets/menu_bgm.wav");

    auto resetToPhase1 = [&]()
    {
        Playersprite.setPosition(battleBox.getPosition());
        activeProjectiles.clear();
        magicPillar.clear();
        crystalBurst.clear();
        wavecount = 0;
        currentphase = 1;
        mapindex = 0;
        invulnerabilitytimer = 0.0f;
        placemapstart();
        waveTriggerClock.restart();
        playershooting.clear();
        shootyclock.restart();
        magicTimer.restart();
        burstTimer.restart();
        phase3Clock.restart(); // phase 3 (Haras)
        bossHealth.setSize({500.0f, 20.0f});
        invulnerabilitytimerMagicAttack = 0.0f;
        isParrying = false; // phase 3 (Haras)
        cooldown = 0.0f;    // phase 3 (Haras)
        parryTimer = 0.0f;  // phase 3 (Haras)
        passiveHeal = false; // boss heal flag must not carry over into a new run

        // restore phase one boss sprite (Haras)
        Bosssprite.setTexture(texture3, true); // resets bounding box back to phase 1 size (Haras)
        Bosssprite.setScale({0.1f, 0.1f});     // boss scaling (Haras)
        FloatRect p1Bounds = Bosssprite.getLocalBounds();
        Bosssprite.setOrigin({p1Bounds.size.x / 2.f, p1Bounds.size.y / 2.f});
        Bosssprite.setPosition({width / 2.0f, 240.0f});

        playMusicTrack("Assets/phase1_bgm.wav");
    };

    // phase 3 (Haras)
    auto enterPhase3 = [&]()
    {
        currentphase = 3;
        Playersprite.setPosition(battleBox.getPosition());
        activeProjectiles.clear();
        magicPillar.clear();
        crystalBurst.clear();
        playershooting.clear();
        bossHealth.setSize({500.0f, 20.0f});
        invulnerabilitytimer = 1.0f;
        isParrying = false;
        cooldown = 0.0f;
        parryTimer = 0.0f;
        blazeRod.setColor(Color::White);
        blazeRod.setRotation(degrees(0.0f));
        phase3Clock.restart();
        speech.setString("SPACE to deflect!");

        Bosssprite.setTexture(bossphase3tex, true); // reset bounding box for the new boss sprite (Haras)
        FloatRect p3Bounds = Bosssprite.getLocalBounds();
        Bosssprite.setOrigin({p3Bounds.size.x / 2.f, p3Bounds.size.y / 2.f});
        float p3Scale = 155.f / max(p3Bounds.size.x, p3Bounds.size.y); // fixed clipping (Haras)
        Bosssprite.setScale({p3Scale, p3Scale});                       // fixed clipping (Haras)
        Bosssprite.setPosition({width / 2.0f, 240.0f});
    };

    // Hazard trail dot shape
    // Use the created flame sprite / texture pack here
    CircleShape hazardDot(3.0f);
    hazardDot.setFillColor(Color(255, 69, 0, 200));
    hazardDot.setOrigin({3.0f, 3.0f});

    // used to change current state of game from menu too playing ;
    // change this and add an endless mode also make it go back to menu if escape pressed during the playing stage
    int changeState = 0;
    DebugState debug; // for debug menu (Haras)

    // load SoundBuffer before while window.isOpen loop, so it only loads once (Haras)
    SoundBuffer shootBuffer;
    if (!shootBuffer.loadFromFile("Assets/snd_damage.mp3"))
    {
        return -1; // if audio file doesn't exist it will exit (Haras)
    }
    // load Sound which actually plays/makes sound (Haras)
    Sound shootSound(shootBuffer);

    float shakeIntesity = 2.0f; // Default pixel offset for SI, lowered it for smoother experience (Haras)
    float shakeTimer = 0.0f;    // Will keep track of duration of active shake (Haras)

    // Boss dialogue setup yay tis SO long omg holy help me im dead
    vector<DialogueLine> dialogueLines;
    size_t dialogueIndex = 0;
    int dialogueContext = 0; // 1 = intro, 2 = after phase 1, 3 = after maps, 4 = after the fake death
    bool introDone = false;

    SoundBuffer dialogueBuffer;
    Sound dialogueSound(dialogueBuffer);

    // Stuff for te fake death and for the "killing" projectile
    Projectile cutsceneProj;
    bool cutsceneProjActive = false;
    int popupHovered = -1;

    RectangleShape popupBox({460.f, 180.f});
    popupBox.setOrigin({230.f, 90.f});
    popupBox.setPosition({width / 2.f, height / 2.f});
    popupBox.setFillColor(Color(15, 15, 22, 245));
    popupBox.setOutlineColor(Color::Red);
    popupBox.setOutlineThickness(3.f);

    Text popupPrompt = createCenteredText(font, "Do you..... wish to end it here?", 20, {width / 2.f, height / 2.f - 35.f}, Color::White);
    Text popupYes = createCenteredText(font, "YES", 24, {width / 2.f - 85.f, height / 2.f + 35.f}, Color(180, 180, 180));
    Text popupNo = createCenteredText(font, "NO", 24, {width / 2.f + 85.f, height / 2.f + 35.f}, Color(180, 180, 180));

    auto showLine = [&]()
    {
        dialogueSound.stop();
        if (!dialogueLines[dialogueIndex].sound.empty())
        {
            if (dialogueBuffer.loadFromFile(dialogueLines[dialogueIndex].sound))
            {
                dialogueSound.setBuffer(dialogueBuffer);
                dialogueSound.setVolume(soundVolume);
                dialogueSound.play();
            }
            else
            {
                cout << "FAILED TO LOAD: " << dialogueLines[dialogueIndex].sound << endl;
            }
        }
    };

    auto startDialogue = [&](int context)
    {
        dialogueContext = context;
        dialogueIndex = 0;
        if (context == 1)
        {
            dialogueLines = {
                {"Oh....", Color::Black, "Assets/voice1.wav"},
                {"So.....", Color::Black, "Assets/voice1.wav"},
                {"You decided\nto come challenge me?", Color::Black, "Assets/voice1.wav"},
                {"Oh well", Color::Black, "Assets/voice1.wav"},
                {"You weren't\nthe first", Color::Black, "Assets/voice1.wav"},
                {"And you certainly\nwon't be the last", Color::Black, "Assets/voice1.wav"},
                {"Well then.....", Color::Black, "Assets/voice1.wav"},
                {"Come at me.", Color::Blue, "Assets/voice1.wav"},
            };
        }
        else if (context == 2)
        {
            dialogueLines = {
                {"AGHHh", Color::Black, "Assets/voice1.wav"},
                {"....", Color::Black, "Assets/voice1.wav"},
                {"How...??", Color::Black, "Assets/voice1.wav"},
                {"Just how???", Color::Black, "Assets/voice1.wav"},
                {"How did you get\nthis power?", Color::Black, "Assets/voice1.wav"},
                {"I....", Color::Black, "Assets/voice1.wav"},
                {"I can't die here.", Color::Black, "Assets/voice1.wav"},
                {"No I can't", Color::Black, "Assets/voice1.wav"},
                {"Ive never been\ndefeated before", Color::Black, "Assets/voice1.wav"},
                {"....", Color::Black, "Assets/voice1.wav"},
                {"And I never will", Color::Red, "Assets/voice1.wav"},
                {"AHahhahHhahHAHha", Color::Red, "Assets/laugh.wav"},
                {"Just what did\nyou think?", Color::Red, "Assets/voice1.wav"},
                {"That id go\ndown this easy?", Color::Black, "Assets/voice1.wav"},
                {"That im just some\nfodder for you to defeat?", Color::Black, "Assets/voice1.wav"},
                {"Foolish", Color::Black, "Assets/voice1.wav"},
                {"Pathetic", Color::Black, "Assets/voice1.wav"},
                {"My powers.....", Color::Black, "Assets/voice1.wav"},
                {"They're beyond your\nmeagre comprehension", Color::Black, "Assets/voice1.wav"},
                {"Curious?", Color::Black, "Assets/voice1.wav"},
                {"Take a look.", Color::Red, ""},
            };
        }
        else if (context == 3)
        {
            // after the maps finish(Boss might be salty tbh)
            dialogueLines = {
                {"Heheahehahehae", Color::Black, "Assets/laugh.wav"},
                {"It's so funny to be honest", Color::Black, "Assets/voice1.wav"},
                {"Seeing you miserably struggle for survival", Color::Red, "Assets/voice1.wav"},
                {"Now now", Color::Black, "Assets/voice1.wav"},
                {"I think", Color::Black, "Assets/voice1.wav"},
                {"It's time", Color::Black, "Assets/voice1.wav"},
                {"You stopped bothering me", Color::Black, ""},
                {"To hell with you\nannoying prick", Color::Black, ""},
            };
        }
        else if (context == 4)
        {
            // OH THE BOSS IS SALTY, OH THEY'RE SO SALTY
            dialogueLines = {
                {"What.", Color::Black, "Assets/voice1.wav"},
                {"WHAT?", Color::Red, "Assets/voice1.wav"},
                {"HOW ARE YOU\nSTILL ALIVE?", Color::Red, "Assets/voice1.wav"},
                {"JUST DIE ALREADY", Color::Red, "Assets/voice1.wav"},

            };
        }

        playMusicTrack(""); // silence during dialogue
        showLine();
        changeState = 4;
    };

    auto startPhase2 = [&]()
    {
        currentphase = 2;
        mapindex = 0;
        activeProjectiles.clear();
        magicPillar.clear();
        crystalBurst.clear();
        playershooting.clear();
        playerspawnmap();
    };

    auto endDialogue = [&]()
    {
        if (dialogueContext == 1)
        {
            playMusicTrack("Assets/phase1_bgm.wav");
        }
        else if (dialogueContext == 2)
        {
            startPhase2();
            playMusicTrack("Assets/phase2_bgm.wav");
        }
        else if (dialogueContext == 3)
        {
            cutsceneProj.position = Bosssprite.getPosition();
            cutsceneProj.isLaunched = true;
            cutsceneProj.tracksPlayer = false;
            cutsceneProj.leavesTrail = false;
            cutsceneProj.hitBox = 14.f;

            Vector2f dir = Playersprite.getPosition() - cutsceneProj.position;
            float len = sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len != 0.f)
                dir /= len;
            cutsceneProj.velocity = dir * 18.f;

            cutsceneProjActive = true;
            dialogueContext = 0;
            changeState = 5; // Go to scripted projectile launch
            return;          // Exit early so changeState doesn't get set to 2 below
        }
        else if (dialogueContext == 4)
        {
            playMusicTrack("Assets/phase3_bgm.wav");
        }

        dialogueContext = 0;
        waveTriggerClock.restart();
        magicTimer.restart();
        burstTimer.restart();
        shootyclock.restart();
        phase3Clock.restart();
        changeState = 2;
    };

    while (window.isOpen())
    {
        float dt = frameClock.restart().asSeconds();

        // gets recent mouse position
        // this statemet check that if there new games is pressed we change menu too the recent menu and end the previous menu
        if (changeState == 0 || changeState == 3)
        { // for game end menu overhall (Haras)

            // required functions for menu
            // implemented delta-time (Haras)
            starfieldBackground(myStars, starBackground, dt);
            // integrated soundvolume and musicvolume variable
            menu_Interaction(window, view, currentState, hoveredIndex,
                             mainMenuItems, deathMenuItems, startMenuItems, pauseMenuItems, startOptions,
                             difficultyIndex, difficulties, font, changeState, previousState, speed, projectileSpeed, waveInterval, launchInterval,
                             soundVolume, musicVolume, shakeIntesity, currentHeart, hearts, heartFiles, scaling, Playersprite, playerIcon, debug);

            if (currentState != GameState::Death)
                introDone = false; // intro replays on new game so it doesn't replay on a retry cuz holy yap
            if (currentState == GameState::Death)
                playMusicTrack("");
            else
                playMusicTrack("Assets/menu_bgm.wav");

            bgMusic.setVolume(musicVolume);

            // Making new game option actually start new game, not start from where left off (Haras)
            if (changeState == 0 || changeState == 3)
            { // Also added for death menu so shake not apply to it (Haras)
                if (previousState != GameState::Paused)
                {
                    Playersprite.setPosition(battleBox.getPosition()); // This will put player position back on center
                    activeProjectiles.clear();                         // Clears all previous projectiles
                    wavecount = 0;                                     // restart waves

                    currentphase = 1; // back to phase 1 cuz yes
                    mapindex = 0;
                    invulnerabilitytimer = 0.0f; // no leftover invunerability frames cuz I can't have the players getting too comfy
                    waveTriggerClock.restart();  // resets attack timings
                    shakeTimer = 0.0f;           // resets shakeTimer
                    healthBar.setSize({300.f, 20.f});
                    healthBar.setFillColor(Color(57, 255, 20));
                    bossHealth.setSize({500.f, 20.f});
                    passiveHeal = false; // boss starts fresh (not healing / not in final form) on a new run
                    magicTimer.restart();
                    burstTimer.restart();
                    playershooting.clear();
                    shootyclock.restart();
                    magicPillar.clear();
                    crystalBurst.clear();
                    isParrying = false;
                    cooldown = 0.f;
                    placemapstart(); // map back to start (bottom of the map on the bottom of the screen)

                    Bosssprite.setTexture(texture3, true);
                    FloatRect bounds = Bosssprite.getLocalBounds();
                    Bosssprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
                    Bosssprite.setScale({0.1f, 0.1f});
                    Bosssprite.setPosition({width / 2.0f, 240.0f});

                    burstDelay = 10.f;

                    parryTimer = 0.0f;
                    blazeRod.setColor(Color::White);
                    blazeRod.setRotation(degrees(0.0f));
                }
                frameClock.restart();                          // prevent dt jump on first frame
                view.setCenter({width / 2.0f, height / 2.0f}); // snaps back to center
                window.setView(view);                          // implements the view
            }
            // integrated soundvolume and musicVolume variable
            rendering(window, starBackground, currentState, titletext, mainMenuItems, deathMenuItems,
                      startMenuItems, pauseMenuItems, font, difficulties[difficultyIndex], soundVolume,
                      musicVolume, shakeIntesity, DimScreen, Playersprite, currentHeart, debug);
            window.display();
        }
        else
        {
            if (changeState == 2 && debug.jumpPhase != 0)
            { // for debug menu (Haras)
                healthBar.setSize({300.f, 20.f});
                healthBar.setFillColor(Color(57, 255, 20));
                shakeTimer = 0.0f;
                introDone = true;

                if (debug.jumpPhase == 1)
                {
                    resetToPhase1();
                }
                else if (debug.jumpPhase == 2)
                {
                    resetToPhase1();
                    startPhase2();
                    playMusicTrack("Assets/phase2_bgm.wav");
                }
                else if (debug.jumpPhase == 3)
                {
                    enterPhase3();
                    playMusicTrack("Assets/phase3_bgm.wav");
                }
                debug.jumpPhase = 0;
            }

            if (changeState == 2 && !introDone && currentphase == 1)
            {
                introDone = true;
                startDialogue(1);
            }
            if (changeState == 2)
            {
                while (const optional<Event> event = window.pollEvent())
                {
                    if (event->is<Event::Closed>())
                    {
                        window.close();
                    }
                    else if (const auto *keyPressed = event->getIf<Event::KeyPressed>())
                    {
                        if (keyPressed->scancode == Keyboard::Scancode::Escape)
                        {
                            currentState = GameState::Paused;
                            previousState = GameState::Paused;
                            hoveredIndex = -1;
                            changeState = 1;
                        }
                        else if (keyPressed->scancode == Keyboard::Scancode::Space && !isParrying && cooldown <= 0.0f && currentphase == 3)
                        {
                            isParrying = true;
                            parryTimer = 0.0f;
                        }
                    }
                }

                // Call player movement with box to prevent excess movement hahahahah suffer players(Bazil)
                // implemented delta-time (Haras)

                // to ensure you cant go outside map bounds (Haras)
                Vector2f oldplayerpos = Playersprite.getPosition();
                bool hitwallphase2 = false;

                handlePlayerMovement(Playersprite, currentphase == 2 ? speed * 0.8f : speed, battleBox, dt, currentphase == 1 || currentphase == 3); // ensured you CANT go out of battle box bounds in phase 3 (Haras)

                if (currentphase == 2)
                { // to ensure you cant go outside map bounds (Haras)
                    Vector2f newpos = Playersprite.getPosition();
                    if (mapcollission(mapImages[mapindex], mapsprite, {newpos.x, oldplayerpos.y}))
                    {
                        newpos.x = oldplayerpos.x;
                        hitwallphase2 = true;
                    }
                    if (mapcollission(mapImages[mapindex], mapsprite, {newpos.x, newpos.y}))
                    {
                        newpos.y = oldplayerpos.y;
                        hitwallphase2 = true;
                    }
                    Playersprite.setPosition(newpos);
                }

                // the speed of the map scroll cuz, yeah
                float mapscrollspeed = 30.0f - 5 * (difficultyIndex - 1);
                if (currentphase == 2) // must ONLY happen in the 2nd wave
                {
                    // keeps player on path if the scrolling top wall pushes into them, also fixed a bug where if map push you out of path, it was not allowing you to move, keeping you stuck (Haras)
                    float oldMapY = mapsprite.getPosition().y;
                    // the following code is cuz omg I didn't include this and the map scrolled off the screen AND IT CONSIDERED IT OUT OF BOUNDS AND I WAS DYING AAAA
                    mapsprite.move({0.0f, mapscrollspeed * dt});
                    if (mapsprite.getPosition().y > 0.0f)
                    {
                        mapsprite.setPosition({mapsprite.getPosition().x, 0.0f});
                    }
                    // keeps player on path if the scrolling wall pushes into them (Haras)
                    float scrollDelta = mapsprite.getPosition().y - oldMapY;
                    if (mapcollission(mapImages[mapindex], mapsprite, Playersprite.getPosition()))
                    {
                        hitwallphase2 = true;
                        Vector2f p = Playersprite.getPosition();
                        bool recovered = false;
                        // tries riding down with the map scroll (Haras)
                        if (scrollDelta > 0.0f && p.y + scrollDelta <= height - 8.0f &&
                            !mapcollission(mapImages[mapindex], mapsprite, {p.x, p.y + scrollDelta}))
                        {
                            p.y += scrollDelta;
                            Playersprite.setPosition(p);
                            recovered = true;
                        }

                        // nudge player slightly to nearby stone if squeeze by a curved angled edge (Haras)
                        if (!recovered)
                        {
                            for (float r = 2.0f; r <= 12.0f && !recovered; r += 2.0f)
                            {
                                Vector2f offsets[4] = {{0.f, -r}, {0.f, r}, {-r, 0.f}, {r, 0.f}};
                                for (auto off : offsets)
                                {
                                    Vector2f testPos = p + off;
                                    if (testPos.x >= 8.f && testPos.x <= width - 8.f && testPos.y >= 8.f && testPos.y <= height - 8.f && !mapcollission(mapImages[mapindex], mapsprite, testPos))
                                    {
                                        Playersprite.setPosition(testPos);
                                        recovered = true;
                                        break;
                                    }
                                }
                            }
                        }
                        // crushed off the bottom edge scenario: reset current map so player canot get trapped
                        if (!recovered)
                        {
                            playerspawnmap();
                        }
                    }

                    if (mapsprite.getPosition().y >= 0.0f)
                    {
                        // this is for us reaching the endpoint cuz I mean we gotta be able to move on to the next one I suppose
                        Vector2f endpoint = mapsprite.getPosition() + (finishline[mapindex] * mapbiggie);
                        Vector2f diff = Playersprite.getPosition() - endpoint;
                        if (sqrt(diff.x * diff.x + diff.y * diff.y) < 55.0f)
                        {
                            mapindex++;
                            if (mapindex < (int)mapTextures.size())
                            {
                                invulnerabilitytimer = 1.0f;
                                playerspawnmap();
                            }
                            else
                            {

                                activeProjectiles.clear();
                                magicPillar.clear();
                                crystalBurst.clear();
                                playershooting.clear();
                                Playersprite.setPosition(battleBox.getPosition());
                                Bosssprite.setPosition({width / 2.0f, 240.0f});
                                startDialogue(3);
                            }
                        }
                    }
                }

                // first phase stuff ugh(Bazil comments here)
                if (currentphase == 1)
                {
                    bool nextwave = false;
                    if (wavecount <= 3)
                    { // this is only for the first crescent waves
                        nextwave = allLaunched(activeProjectiles);
                    }
                    else
                    {
                        nextwave = activeProjectiles.empty();
                    }
                    if (waveTriggerClock.getElapsedTime().asSeconds() >= waveInterval && nextwave)
                    {                                 // this is to rapid fire waves cuz I mean, im not merciless
                        waveTriggerClock.restart();   // set it to 0 for the next projectile
                        float spawnposition = 220.0f; //
                        float leftX = width * 0.25f;
                        float middleX = width * 0.5f;
                        float rightX = width * 0.75f;
                        if (wavecount == 0)
                        {
                            crescent(activeProjectiles, middleX, spawnposition, 0.15f, launchInterval); // Crescent Middle
                        }
                        else if (wavecount == 1)
                        {
                            crescent(activeProjectiles, rightX, spawnposition, 0.15f, launchInterval); // Crescent Right
                        }
                        else if (wavecount == 2)
                        {
                            crescent(activeProjectiles, leftX, spawnposition, 0.15f, launchInterval); // Crescent Left
                        }
                        else if (wavecount == 3)
                        {
                            crescent(activeProjectiles, leftX, spawnposition, 0.15f, launchInterval); // Triple Crescent
                            crescent(activeProjectiles, middleX, spawnposition, 0.15f, launchInterval);
                            crescent(activeProjectiles, rightX, spawnposition, 0.15f, launchInterval);
                        }
                        else if (wavecount == 4)
                        {
                            horizontal(activeProjectiles, battleBox, projectileSpeed - 5, -45.0f);
                            horizontal(activeProjectiles, battleBox, projectileSpeed - 5, 0.0f);
                        }
                        else if (wavecount == 5)
                        {
                            horizontal(activeProjectiles, battleBox, projectileSpeed - 5, 0.0f);
                            horizontal(activeProjectiles, battleBox, projectileSpeed - 5, 30.0f);
                        }
                        else if (wavecount == 6)
                        {
                            pinwheel(activeProjectiles, {middleX, battleBox.getPosition().y + 50}, projectileSpeed);
                            pinwheel(activeProjectiles, {rightX - 50.f, battleBox.getPosition().y - 50}, projectileSpeed);
                            pinwheel(activeProjectiles, {leftX + 50.f, battleBox.getPosition().y - 50}, projectileSpeed);
                        }
                        else if (wavecount == 7)
                        {
                            pinwheel(activeProjectiles, {rightX, battleBox.getPosition().y}, projectileSpeed);
                        }
                        else if (wavecount == 8)
                        {
                            pinwheel(activeProjectiles, {leftX, battleBox.getPosition().y}, projectileSpeed);
                        }
                        else if (wavecount == 9)
                        {
                            homingdarts(activeProjectiles, battleBox, middleX);
                        }
                        else if (wavecount == 10)
                        {
                            homingdarts(activeProjectiles, battleBox, rightX);
                        }
                        else if (wavecount == 11)
                        {
                            homingdarts(activeProjectiles, battleBox, leftX);
                        }

                        wavecount = (wavecount + 1) % 12;
                    }

                    if (magicTimer.getElapsedTime().asSeconds() >= 3.0f - (difficultyIndex))
                    {
                        // Spawn on random sides of the map :
                        float randomX = static_cast<float>(rand() % 400 + 220);
                        verticle(magicPillar, randomX, difficultyIndex);
                        magicTimer.restart();
                    }
                    if (burstTimer.getElapsedTime().asSeconds() >= burstDelay)
                    {
                        // burst loading
                        burst(crystalBurst, difficultyIndex, Bosssprite.getPosition(), launchInterval);
                        // creates random BUrst interval from 1 - 9 seconds
                        burstDelay = 1.f + rand() % 900 / 100.f;

                        burstTimer.restart();
                    }

                    // Change Phase 1 boss sprite based on remaining HP (max bar width = 500.f)
                    float hp = bossHealth.getSize().x;

                    // once the boss drops to the final third it enters its final form and keeps it (passiveHeal),
                    // fixed: removed the stray "&& hp" from the old condition
                    if (hp <= 150.0f || passiveHeal)
                    {
                        Bosssprite.setTexture(bossFinalStage, true);
                        FloatRect b = Bosssprite.getLocalBounds();
                        Bosssprite.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
                        Bosssprite.setScale({0.14f, 0.14f}); // Adjust scale for bossFinalStage.png
                        Bosssprite.setPosition({width / 2.0f, 230.0f});
                        passiveHeal = true;

                        // passive heal: slowly regenerates up to 400, never while the boss is at/below 0 so it can't undo a killing blow
                        if (hp > 0.0f && hp <= 400.f)
                            bossHealth.setSize({min(400.f, hp + 5.f * dt), 20.f});
                    }
                    else if (hp <= 300.0f) // below 60-30 stage 2 ;
                    {
                        Bosssprite.setTexture(bossStage2, true);
                        FloatRect b = Bosssprite.getLocalBounds();
                        Bosssprite.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
                        Bosssprite.setScale({0.12f, 0.12f}); // Adjust scale for bossStage2.png
                        Bosssprite.setPosition({width / 2.0f, 235.0f});
                    }
                    else // default when hp above 60%
                    {
                        Bosssprite.setTexture(texture3, true);
                        FloatRect b = Bosssprite.getLocalBounds();
                        Bosssprite.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
                        Bosssprite.setScale({0.1f, 0.1f});
                        Bosssprite.setPosition({width / 2.0f, 240.0f});
                    }
                }

                if (currentphase == 1 && Keyboard::isKeyPressed(Keyboard::Key::Space) && shootyclock.getElapsedTime().asSeconds() >= 0.25f) // shooting them projectiles rahhh. The 0.25 is to put the delay between each projectile so we don't become a minigun (teammates would never approve of the minigun tis so sad :[ )
                {
                    shootyclock.restart();
                    Projectile s;
                    s.position = Playersprite.getPosition();
                    s.velocity = {0.f, -25.f};
                    s.isLaunched = true;
                    s.tracksPlayer = false;
                    playershooting.push_back(s);
                }

                // boss throws projectiles at player to deflect (Haras)
                if (currentphase == 3)
                {
                    float throwInterval = 1.8f - 0.25f * difficultyIndex; // 1.8 seconds for easy, 1.5 seconds Normal, 1.3 seconds for Bullet Hell
                    if (phase3Clock.getElapsedTime().asSeconds() >= throwInterval)
                    {
                        bossThrow(activeProjectiles, Bosssprite.getPosition());
                        phase3Clock.restart();
                    }
                }

                // update the projectiles
                updateProjectiles(crystalBurst, dt, width, height, Playersprite.getPosition(), 12.f - 5 * (difficultyIndex - 1));
                updateProjectiles(magicPillar, dt, width, height, Playersprite.getPosition(), 12.f - 5 * (difficultyIndex - 1));
                updateProjectiles(activeProjectiles, dt, width, height, Playersprite.getPosition(), currentphase == 3 ? projectileSpeed * 0.55f : projectileSpeed);
                updateProjectiles(playershooting, dt, width, height, Playersprite.getPosition(), 0.f); // for the players own projectiles cuz I made the game easier praise me

                // run parrying after projectiles move so swings deflect before any collision triggers (Haras)
                if (currentphase == 3)
                {
                    updateParrying(blazeRod, Playersprite, Bosssprite.getPosition(), playerIcon,
                                   cooldownRing, cover, activeProjectiles, crystalBurst,
                                   isParrying, parryTimer, cooldown, dt);
                }

                // counts down the i-frames
                if (invulnerabilitytimer > 0.0f || invulnerabilitytimerMagicAttack > 0.0f)
                {
                    invulnerabilitytimer -= dt;
                    invulnerabilitytimerMagicAttack -= dt;
                }

                // Check deflected boss hits and direct player hits (Haras)
                // (merged from the 1st code: missed deflects no longer hurt the player)
                if (currentphase == 3)
                {
                    for (int i = 0; i < (int)activeProjectiles.size();)
                    {
                        Projectile &p = activeProjectiles[i];
                        if (p.parried && Bosssprite.getGlobalBounds().contains(p.position))
                        {
                            healthBarUpdate(bossHealth, 15.f); // deflect damage (merged from the 1st code)
                            shootSound.setVolume(soundVolume);
                            shootSound.play();
                            activeProjectiles.erase(activeProjectiles.begin() + i);
                        }
                        else if (!p.parried && p.isLaunched)
                        {
                            float dx = Playersprite.getPosition().x - p.position.x;
                            float dy = Playersprite.getPosition().y - p.position.y;
                            bool hitPlayer = sqrt(dx * dx + dy * dy) < p.hitBox;

                            if (hitPlayer)
                            {
                                shootSound.setVolume(soundVolume);
                                shootSound.play();
                                healthBarUpdate(healthBar, 15.f);
                                shakeTimer = 0.3f;
                                activeProjectiles.erase(activeProjectiles.begin() + i);
                            }
                            else
                            {
                                i++;
                            }
                        }
                        else
                        {
                            i++;
                        }
                    }

                    if (bossHealth.getSize().x <= 0)
                    {
                        resetToPhase1(); // loops back after beating Phase 3 ( will replace with a VICTORY screen later)
                    }
                }

                // Separate projectile hits from Phase 2 map collisions
                bool projHit = (currentphase != 3 && collision(activeProjectiles, Playersprite.getPosition())) ||
                               collision(crystalBurst, Playersprite.getPosition());

                bool mapHit = (currentphase == 2) &&
                              (hitwallphase2 || mapcollission(mapImages[mapindex], mapsprite, Playersprite.getPosition()));

                if ((projHit || mapHit) && invulnerabilitytimer <= 0.0f)
                {
                    float damage = 0.0f;

                    if (projHit)
                    {
                        // Keep full projectile damage unchanged
                        damage = 15.f - 3.f * (difficultyIndex - 1);
                    }
                    else if (mapHit)
                    {
                        // Lower damage only for Phase 2 map collisions
                        damage = 6.f;
                    }

                    if (currentphase == 1)
                        invulnerabilitytimer = 0.5f - 0.3f * (difficultyIndex - 1); // frames where you can't get hit again (scales with difficulty, merged from the 1st code)
                    if (currentphase == 2)
                        invulnerabilitytimer = 2.0f; // for the 2nd phase

                    shootSound.setVolume(soundVolume); // Sets volume of current selected menu volume (Haras)
                    shootSound.play();                 // Makes sound effect play when player gets hit (Haras)
                    healthBarUpdate(healthBar, damage);
                    shakeTimer = 0.3f; // This will actually engage the SI for 0.3 seconds (Haras)
                }

                projHit = collision(magicPillar, Playersprite.getPosition());

                if (projHit && invulnerabilitytimerMagicAttack <= 0.0f)
                {
                    if (currentphase == 1)
                        invulnerabilitytimerMagicAttack = 1.f;
                    shootSound.setVolume(soundVolume); // Sets volume of current selected menu volume (Haras)
                    shootSound.play();                 // Makes sound effect play when player gets hit (Haras)
                    healthBarUpdate(healthBar, 30 - 5 * (difficultyIndex - 1));
                    shakeTimer = 0.3f; // This will actually engage the SI for 0.3 seconds (Haras)
                }

                if (healthBar.getSize().x <= 0)
                {
                    changeState = 3;
                    currentState = GameState::Death;
                }
                // for the projectiles
                if (currentphase == 1)
                {
                    for (int i = 0; i < (int)playershooting.size();)
                    {
                        if (Bosssprite.getGlobalBounds().contains(playershooting[i].position)) // if the projectile bonks the boss
                        {
                            healthBarUpdate(bossHealth, 5.f);  // hahehaheha DAMAGE YAAAS (5 per shot, merged from the 1st code)
                            shootSound.setVolume(soundVolume); // play the oof
                            shootSound.play();
                            playershooting.erase(playershooting.begin() + i);
                        }
                        else
                            i++; // ooh so cool thing, when a projectile hits and gets erased, the next projectile in the list slides to the position of the successful projectile, so we only need to iterate in case it doesn't hit
                    }
                }

                if (bossHealth.getSize().x <= 0 && currentphase == 1)
                {
                    activeProjectiles.clear();
                    magicPillar.clear();
                    crystalBurst.clear();
                    playershooting.clear();
                    startDialogue(2); // phase 2 starts when the dialogue ends
                }

                // SI Code is below this (Haras)
                if (shakeTimer > 0.0f)
                {
                    shakeTimer -= dt;
                    // Below random number generation between positive, negative shakeIntensity (Haras)
                    float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * shakeIntesity;
                    float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * shakeIntesity;
                    view.setCenter({(width / 2.0f) + offsetX, (height / 2.0f) + offsetY});
                }
                else
                {
                    // Camera goes back to center when no shake (Haras)
                    view.setCenter({width / 2.0f, height / 2.0f});
                }
            }
            if (changeState == 4)
            {
                while (const optional<Event> event = window.pollEvent())
                {
                    if (event->is<Event::Closed>())
                    {
                        window.close();
                    }
                    else if (const auto *key = event->getIf<Event::KeyPressed>())
                    {
                        if (key->code == Keyboard::Key::Enter)
                        {
                            dialogueIndex++;
                            if (dialogueIndex >= dialogueLines.size())
                            {
                                endDialogue();
                                break;
                            }
                            showLine();
                        }
                    }
                }
            }
            if (changeState == 5) // Boss throws the killing projectile thingy
            {
                while (const optional<Event> event = window.pollEvent())
                {
                    if (event->is<Event::Closed>())
                        window.close();
                }

                if (cutsceneProjActive)
                {
                    cutsceneProj.position += cutsceneProj.velocity * dt * 30.0f; // GO PROJECTILE GOOO
                    Vector2f diff = Playersprite.getPosition() - cutsceneProj.position;
                    if (sqrt(diff.x * diff.x + diff.y * diff.y) <= cutsceneProj.hitBox)
                    {
                        cutsceneProjActive = false;
                        shootSound.setVolume(soundVolume);
                        shootSound.play();
                        healthBar.setSize({0.f, 20.f});
                        shakeTimer = 0.4f;
                        changeState = 6; // cue the trick
                    }
                }
            }

            if (changeState == 6) // the fake death popup
            {
                Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                popupHovered = -1;

                if (popupYes.getGlobalBounds().contains(mousePos))
                {
                    popupHovered = 0;
                    popupYes.setFillColor(Color::Yellow);
                    popupYes.setScale({1.15f, 1.15f});
                }
                else
                {
                    popupYes.setFillColor(Color(180, 180, 180));
                    popupYes.setScale({1.0f, 1.0f});
                }

                if (popupNo.getGlobalBounds().contains(mousePos))
                {
                    popupHovered = 1;
                    popupNo.setFillColor(Color::Yellow);
                    popupNo.setScale({1.15f, 1.15f});
                }
                else
                {
                    popupNo.setFillColor(Color(180, 180, 180));
                    popupNo.setScale({1.0f, 1.0f});
                }

                while (const optional<Event> event = window.pollEvent())
                {
                    if (event->is<Event::Closed>())
                        window.close();

                    if (const auto *mouseClick = event->getIf<Event::MouseButtonPressed>())
                    {
                        if (mouseClick->button == Mouse::Button::Left)
                        {
                            if (popupHovered == 0) // awww you gave up? so sad
                            {
                                previousState = GameState::Menu;
                                currentState = GameState::Menu;
                                changeState = 0;
                            }
                            else if (popupHovered == 1) // so, thee decided to persevere. Exciting......
                            {
                                healthBar.setSize({300.f, 20.f});
                                healthBar.setFillColor(Color(57, 255, 20));
                                enterPhase3();
                                startDialogue(4); // cue the yapfest (endDialogue() will start BGM & set changeState = 2)
                            }
                        }
                    }
                }
            }
            window.setView(view); // This line actually applies teh offset to before drawing happens below (Haras)
            // Draw static UI and Box first cuz

            window.clear();

            bool isPostMapCutscene = (changeState == 4 && dialogueContext == 3) || (changeState == 5) || (changeState == 6);

            if (!isPostMapCutscene)
            {
                if (currentphase == 1 || currentphase == 3)
                {
                    window.draw(bgPhase1Sprite);
                }
                if (currentphase == 2)
                {
                    window.draw(mapsprite);
                }

                window.draw(boundaryRect);
                window.draw(healthBar);
            }

            if (currentphase == 1 || currentphase == 3 || isPostMapCutscene)
            {
                if (!isPostMapCutscene)
                {
                    window.draw(battleBox);
                }
                window.draw(Bosssprite);
                if (!isPostMapCutscene)
                {
                    window.draw(healthBoundary);
                    window.draw(bossHealth);
                    window.draw(bossName);
                }
            }

            if (currentphase == 3)
            {
                window.draw(cooldownRing);
                window.draw(cover);
            }

            if (!isPostMapCutscene)
            {
                window.draw(playerIcon);
            }

            // draw the projectiles with the hazard trail
            for (const auto &pillar : magicPillar)
            {
                MagicAttack.setPosition(pillar.position);
                window.draw(MagicAttack);
            }

            for (const auto &newCrystal : crystalBurst)
            {
                if (newCrystal.spawnDelay <= 0.0f)
                {
                    fallingCrystal.setPosition(newCrystal.position);
                    fallingCrystal.setRotation(degrees(newCrystal.rotation));
                    window.draw(fallingCrystal);
                }
            }

            for (const auto &proj : activeProjectiles)
            {
                for (const auto &hazard : proj.trail)
                {
                    hazardDot.setPosition(hazard.position);
                    window.draw(hazardDot);
                }
                if (proj.parried)
                {
                    projSprite.setColor(Color(255, 255, 0, 200));
                }
                else
                    projSprite.setColor(Color::White);

                if (proj.spawnDelay <= 0.0f && (proj.isLaunched || proj.launchTimer > 0.0f))
                {
                    projSprite.setPosition(proj.position);
                    window.draw(projSprite);
                }
            }
            // draw each of the player's shots (now uses the flame attack sprite, merged from the 1st code)
            for (const auto &shot : playershooting)
            {
                Sprite s = playerShot;
                s.setPosition(shot.position);
                window.draw(s);
            }

            // Draw Player on top
            window.draw(Playersprite);
            // phase 3 draw blaze rod (phase 3)
            if (currentphase == 3)
            {
                window.draw(blazeRod);
            }

            if (healthBar.getSize().x <= 135 && healthBar.getSize().x > 60)
                healthBar.setFillColor(sf::Color(243, 156, 18));

            if (healthBar.getSize().x <= 60 && healthBar.getSize().x > 0)
            {
                float time = clock.getElapsedTime().asSeconds();
                float pulse = (sin(time * 4.f) + 1.2f) / 2.f;
                healthBar.setFillColor(Color(240, static_cast<int>(180 * pulse), 20));
            }

            if (changeState == 1)
            {
                menu_Interaction(window, view, currentState, hoveredIndex,
                                 mainMenuItems, deathMenuItems, startMenuItems, pauseMenuItems, startOptions,
                                 difficultyIndex, difficulties, font, changeState, previousState, speed, projectileSpeed, waveInterval,
                                 launchInterval, soundVolume, musicVolume, shakeIntesity, currentHeart, hearts, heartFiles, scaling, Playersprite, playerIcon, debug);

                bgMusic.setVolume(musicVolume); // this allows live update from within settings menu

                rendering(window, starBackground, currentState, titletext,
                          mainMenuItems, deathMenuItems, startMenuItems, pauseMenuItems, font, difficulties[difficultyIndex], soundVolume, musicVolume, shakeIntesity,
                          DimScreen, Playersprite, currentHeart, debug);
            }
            if (changeState == 3)
            {
                Playersprite.setPosition(battleBox.getPosition()); // This will put player position back on center
                activeProjectiles.clear();                         // Clears all previous projectiles
                wavecount = 0;                                     // restart waves
                currentphase = 1;                                  // back to phase 1
                mapindex = 0;
                invulnerabilitytimer = 0.0f; // no leftover i-frames
                placemapstart();             // map back to start (bottom of the map on the bottom of the screen)

                waveTriggerClock.restart(); // resets attack timings
                shakeTimer = 0.0f;          // resets shakeTimer
                healthBar.setSize({300.f, 20.f});
                healthBar.setFillColor(Color(57, 255, 20));
                bossHealth.setSize({500.f, 20.f});
                passiveHeal = false; // boss must not start the retry in its healing final form
                magicTimer.restart();
                burstTimer.restart();
                playershooting.clear(); // REMOOOF THE SHOOTS MWAEHAEHAHEHAHEAHEH
                shootyclock.restart();
                magicPillar.clear();
                crystalBurst.clear();
                isParrying = false;
                cooldown = 0.f;

                burstDelay = 10.f;

                parryTimer = 0.0f;
                blazeRod.setColor(Color::White);
                blazeRod.setRotation(degrees(0.0f));

                bgMusic.setVolume(musicVolume); // this allows live update from within settings menu

                // reseting sprites scale etc
                Bosssprite.setTexture(texture3, true);
                FloatRect bounds = Bosssprite.getLocalBounds();
                Bosssprite.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
                Bosssprite.setScale({0.1f, 0.1f});
                Bosssprite.setPosition({width / 2.0f, 240.0f});
            }
            if (changeState == 4)
            {
                const DialogueLine &line = dialogueLines[dialogueIndex];
                drawDialogue(window, font, line.text, line.color, Bosssprite.getPosition() + Vector2f(Bosssprite.getGlobalBounds().size.x / 2.0f + 12.f, -45.f));
            }

            if (changeState == 5 && cutsceneProjActive)
            {
                projSprite.setColor(Color::Red);
                projSprite.setPosition(cutsceneProj.position);
                window.draw(projSprite);
                projSprite.setColor(Color::White);
            }

            if (changeState == 6)
            {
                DimScreen.setFillColor(Color(0, 0, 0, 180));
                window.draw(DimScreen);
                window.draw(popupBox);
                window.draw(popupPrompt);
                window.draw(popupYes);
                window.draw(popupNo);
            }

            window.display();
        }
    }
    return 0;
}
