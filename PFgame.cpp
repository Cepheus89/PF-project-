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
enum class GameState {
    Menu,
    StartMenu, // Sub-menu after pressing START GAME
    Tutorial,  // Tutorial screen
    Playing,
    Settings,
    Paused, // (Haras)
    Death,
    Shop
};

enum heartType{
    red ,
    golden,
    blue ,
    green,
    pink ,
    purple

};

struct Star {
    Vector2f position;
    float speed;
    Color color;
};



// Trail node struct for the dangerous path
struct TrailNode {
    Vector2f position;
    float lifeTimer = 1.0f;
};


// Lightweight struct for projectiles
struct Projectile {
    Vector2f position;
    Vector2f velocity;
    bool isLaunched = true;
    float launchTimer = 0.0f;
    vector<TrailNode> trail;
    float trailTimer = 0.0f;
    bool leavesTrail = false;//basically whether the projectiles leave the dangerous trail or not (Bazil)
    bool tracksPlayer = true; //many of the projectiles I use are homing, ill set this to true if I want homing projectiles and False if I don't(Bazil)
    float spawnDelay = 0.0f;
    float hitBox = 10.f ;
    float rotation = 0.0f ; //This is required for the falling crystals ;
    bool parried = false ; //projectiles can be parried
    float steeringtime = 0.0f; //this is for homing projectiles (Haras)
};


// Helper function to create centered text cleanly
//this creates centered text so we do not have to qrite it again and again
Text createCenteredText(const Font& font, const string& str, unsigned int size, Vector2f pos, Color color) {
    Text text(font, str, size);
    text.setFillColor(color);
    FloatRect bounds = text.getLocalBounds();
    text.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
    text.setPosition(pos);
    return text;
}

// Reusable function to setup any list of menu items centered on screen (800px width center = 400.f)

//this handles the spacing

vector<Text> createMenuItems(const Font& font, const vector<string>& options, float startY = 280.f, float spacing = 65.f, unsigned int fontSize = 30, float startX = 400.f) {
    vector<Text> items;
    //Takes a list of items and places them vertically so they look aligned
    int rowIndex = 0 ;

    for (const auto& textString : options) {
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


//reduces health bar each time when hit
void healthBarUpdate(RectangleShape& healthBar , float damage )
{
    //also now we pass damage instead of fixed value ;
    Vector2f currentPosition = healthBar.getSize() ;
    float hitpoints = currentPosition.x;
    hitpoints -= damage ;
    healthBar.setSize({hitpoints, 20.0f});

}




// Event polling and hover interaction logic
//Chnage state = 0 is menu change state 1 is pause change state 2 is playing
void menu_Interaction(RenderWindow& window, View& view, GameState& currentState,
                      int& hoveredIndex, vector<Text>& mainMenuItems,vector<Text>&deathMenuOptions,
                      vector<Text>& startMenuItems, vector<Text>& pauseMenuItems, vector<string>& startOptions,
                      int& difficultyIndex, const vector<string>& difficulties, const Font& font, int& changeState, GameState& previousState,
                      float& speed,float& projectileSpeed,float& waveInterval,float& launchInterval,
                      float& soundVolume, float& musicVolume, float& shakeIntensity,int&currentHeart , Texture& hearts, const vector<string>& heartFiles ,
                      float scaling[] , Sprite& playerSprite, Sprite& playerIcon  ) { // (Haras)
    //Making menu functional for soundvolume and musicvolume and shakeintensity changes (Haras)
    //gets new mouse pixel also works on all types / sizes of the screen

    Vector2i mousePixel = Mouse::getPosition(window);
    Vector2f mousePos = window.mapPixelToCoords(mousePixel);

    // Get active menu items list depending on current state
    vector<Text>* activeMenuItems = nullptr;

    //load the required menu
    if (currentState == GameState::Menu)
        activeMenuItems = &mainMenuItems;   // when in main menu load up the main menu items ;
    else if (currentState == GameState::StartMenu)  //when in sub menu load up the start menu items
        activeMenuItems = &startMenuItems;

    else if (currentState == GameState::Paused) {
        activeMenuItems = &pauseMenuItems;
    }
    else if (currentState== GameState::Death)
        activeMenuItems = &deathMenuOptions ;


    // checking all events
    while (const optional event = window.pollEvent()) {
        if (event->is<Event::Closed>()) {
            window.close();
        }
        //keeping screen proportional / scaled ;

        if (event->getIf<Event::Resized>()) {
            window.setView(view);
        }

        if (const auto* mouseClick = event->getIf<Event::MouseButtonPressed>()) {
            if (mouseClick->button == Mouse::Button::Left) {
                // Main menu work
                //Changing state when required ;
                if (currentState == GameState::Menu) {
                    if (hoveredIndex == 0)
                        currentState = GameState::StartMenu; // Goes to Start Sub-Menu
                    else if (hoveredIndex == 1) {
                        previousState = GameState::Menu;
                        currentState = GameState::Settings;
                    }
                    else if (hoveredIndex == 2)
                        window.close();

                    //These changes brought too the main menu
                    else if (hoveredIndex==3){
                        currentState = GameState::Tutorial ;
                    }
                    else if (hoveredIndex==4)
                        currentState = GameState::Shop;

                }

                // For the sub menu
                else if (currentState == GameState::StartMenu) {
                    if (hoveredIndex == 0) {
                        //change difficulty index 0 = easy, 1 = normal, 2 = hard
                        difficultyIndex = (difficultyIndex + 1) % difficulties.size();
                        startOptions[0] = "DIFFICULTY: " + difficulties[difficultyIndex];
                        startMenuItems = createMenuItems(font, startOptions, 240.f, 60.f, 26);
                        //changing stats based on difficulty ;
                        projectileSpeed = 20.0f + 5.0f * (difficultyIndex-1) ;
                        launchInterval = 0.3 -0.1 *(difficultyIndex -1) ;
                        waveInterval = 0.8f - 0.2f * (difficultyIndex-1) ;
                    }
                    //work in prpgress
                    else if (hoveredIndex == 1)
                        //Change this to endless please
                        currentState = GameState::Tutorial; //Tutorial page add textures
                    else if (hoveredIndex == 2) {
                        //
                        previousState = GameState::Playing;
                        currentState = GameState::Playing;  //to main battleField , change state to 2 so that we switch from menu to action
                        changeState = 2 ;
                    }
                    else if (hoveredIndex == 3)
                        currentState = GameState::Menu;     // take back to main menu
                }
                else if (currentState == GameState::Paused) {
                    if (hoveredIndex == 0) {

                        currentState = GameState::Playing;
                        changeState = 2; //Set to 2 which is the Playing state
                    }
                    else if (hoveredIndex == 1) {

                        previousState = GameState::Paused;
                        currentState = GameState::Settings;
                    }
                    else if (hoveredIndex == 2) {
                        previousState = GameState::Menu ; // THis was added because previous state remained pause if we
                        currentState = GameState::Menu;
                        changeState = 0; // (Haras) for game end screen overhaul
                    }
                    else if (hoveredIndex == 3) {
                        window.close();
                    }

                }
                else if (currentState == GameState::Death) {
                        if (hoveredIndex==0){
                            currentState = GameState::Playing ;
                            changeState = 2 ;
                        }
                        else if (hoveredIndex == 1) {
                            previousState = GameState::Menu ;
                            currentState = GameState::Menu;
                            changeState = 0; // (Haras) for game end screen overhaul

                        }


                }
            }
        }

        //ESC key press , return to previous stage
        if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) {
            if (keyPressed->code == Keyboard::Key::Escape) {
                if (currentState == GameState::StartMenu) {
                    currentState = GameState::Menu;
                }
                else if (currentState == GameState::Settings) {
                    currentState = previousState;
                }
                else if (currentState==GameState::Tutorial|| currentState== GameState::Shop){
                    currentState=GameState::Menu ;
                }

                else if (currentState == GameState::Paused) {
                    currentState = GameState::Playing;
                    changeState = 2;
                }
                //close game if escape pressed in the other screen
                else if (currentState == GameState::Menu){
                    window.close() ;
                }
                else if(currentState == GameState::Death){
                    previousState = GameState::Menu ;
                    currentState = GameState::Menu ;
                    changeState = 0; // (Haras) for game end screen overhaul
                }

            }

            //Making menu functional for soundvolume and musicvolume changes (Haras)
            if (currentState == GameState::Settings) {
                // For SF (Haras)
                if (keyPressed->code == Keyboard::Key::Left) {
                    soundVolume = max(0.0f, soundVolume - 10.0f); //Decrease SF by 10. Cannot go below zero
                }
                else if (keyPressed->code == Keyboard::Key::Right) {
                    soundVolume = min(100.0f, soundVolume + 10.0f); //Increase SF by 10. Cannot go above 100
                }
                // For BGM (Haras)
                else if (keyPressed->code == Keyboard::Key::Up) {
                    musicVolume = min(100.0f, musicVolume + 10.0f); //Increase BGM by 10. Cannot go above 100
                }
                else if (keyPressed->code == Keyboard::Key::Down) {
                    musicVolume = max(0.0f, musicVolume - 10.0f); //Decrease BGM by 10. Cannot go below zero
                }
                // For screen shake upon collision intensity (Haras)
                else if (keyPressed->code == Keyboard::Key::W) {
                    shakeIntensity = min(20.0f, shakeIntensity + 1.0f); //Increase SI
                }
                else if (keyPressed->code == Keyboard::Key::S) {
                    shakeIntensity = max(0.0f, shakeIntensity - 1.0f); //Decrease SI, if equals zero then feature is off
                }

            }
            if (currentState == GameState::Shop){
                bool changed =false ;
                if (keyPressed->code == Keyboard::Key::Right) {
                    currentHeart=( currentHeart +1)%6 ; // move to the right skin
                    changed = true ;
                }
                else if (keyPressed->code== Keyboard::Key::Left){
                    currentHeart = (currentHeart+5 )%6 ;
                    changed = true ;
                }


                //if a new skin selected we change the skin display and all sprites
                if (changed){
                    // get the new texture and skin
                    hearts.loadFromFile(heartFiles[currentHeart]);
                    hearts.setSmooth(false) ;

                    //change sprite texture ;
                    playerSprite.setTexture(hearts,true) ;
                    FloatRect bounds= playerSprite.getLocalBounds();
                    playerSprite.setOrigin({bounds.size.x/2.f, bounds.size.y/2.f}) ;
                    playerSprite.setScale({scaling[currentHeart],scaling[currentHeart]}) ;

                    playerIcon.setTexture( hearts,true) ;
                    playerIcon.setOrigin({bounds.size.x/2.f, bounds.size.y/2.f}) ;

                    playerIcon.setScale({scaling[currentHeart], scaling[currentHeart] }) ;

                }
            }

        }
    }

    // When mouse is above text highlight and rescale :
    if (activeMenuItems != nullptr) {
        hoveredIndex = -1;
        int indexCounter = 0 ;
        for (auto& item : *activeMenuItems) {
            if (item.getGlobalBounds().contains(mousePos)) {
                hoveredIndex = indexCounter;
                item.setFillColor(Color::Yellow);
                //resize
                item.setScale({1.15f, 1.15f});
            }
            else {
                // (Haras) for game end screen overhaul
                if (currentState == GameState::Death) {
                    item.setFillColor(indexCounter == 0 ? Color(255, 40, 40) : Color::White);
                }

                else {
                    item.setFillColor(Color(180, 180, 180));
                }

                item.setScale({1.0f, 1.0f});
            }
            indexCounter++ ;
        }
    }
}



// Starfield initialization across 800x600 bounds
void starfieldCreation(vector<Star>& myStars) {
    myStars.clear();
    //creates random star locations within the s[eed gives them a random velocity
    //stores the required stuff in vector array

    for (int i = 0; i < 300; i++) {
        float x = static_cast<float>(rand() % 800);
        float y = static_cast<float>(rand() % 600);
        float speed = (1.0f + static_cast<float>(rand() % 30) / 10.0f);
        uint8_t brightness = static_cast<uint8_t>(100 + rand() % 155);
        myStars.push_back({{x, y}, speed, Color(brightness, brightness, brightness)});
    }
}

// Background star movement reset bound at 600px height
// implemented delta-time for movement so it is independent of fps (Haras)
void starfieldBackground(vector<Star>& myStar, VertexArray& starfield, float dt) {
    starfield.clear();
    //clear the previious vertex array so a new can be loaded
    //change the position with updating new height and looping star back to front when it reaches end of the screen
    //checks vector array for the position and also updates it their
    for (auto& s : myStar) {
        // implemented delta-time for movement so it is independent of fps (Haras)
        s.position.y += s.speed * dt * 30.0f;
        if (s.position.y > 600.f) {
            s.position.y = 0.f;
            s.position.x = static_cast<float>(rand() % 800);
        }
        starfield.append(Vertex{s.position, s.color});
    }
}



void keys(RenderWindow& window,const Font& font,const string& newKey,Vector2f pos,Vector2f keySize, bool isPressed)
{


    float offSet = isPressed ? 3.f:0.f ; //Using this instead of messsy if

    //actual key being created
    //box surrounding it
    RectangleShape keyCreated ({keySize}) ;
    keyCreated.setOrigin({keySize.x/2.f,keySize.y/2.f});

    keyCreated.setPosition({pos.x,pos.y+offSet}) ;
    keyCreated.setFillColor(isPressed? Color(60,60,20): Color(20,20,25)) ;
    keyCreated.setOutlineThickness({3.f}) ;

    keyCreated.setOutlineColor(isPressed?Color::Yellow: Color::White) ; // this allowes to change color when key press of the border

    Text key =createCenteredText(font, newKey, 20, {pos.x,pos.y+offSet-2.f},isPressed?Color::Yellow : Color::White) ;

    window.draw (keyCreated ) ;
    window.draw (key);
    //draw in right sequence ;

}



// Rendering all the stuff for menu
//remove the stuff from here like the death rectangle bring it in main so it can be used in other functions
void rendering(RenderWindow& window, const VertexArray& starfield, GameState currentState,
               const Text& titleText, const vector<Text>& mainMenuItems, vector<Text>&deathMenuItems ,
               const vector<Text>& startMenuItems, const vector<Text>& pauseMenuItems, const Font& font, const string& currentDifficulty, float soundVolume, float musicVolume, float shakeIntensity,
                RectangleShape& dimBackground, const Sprite& playerSprite , int currentHeart) {
    //rendering will now render live percentage of SF, BGM, SI by updating static 100% string to dynamically display variable (Haras),
    // it has been edited for game end screen overhaul (Haras)

    if (currentState == GameState::Paused){
        dimBackground.setFillColor(Color(0,0,0,150)) ;
        window.draw(dimBackground) ;
    }
    else if (currentState == GameState::Death){
        window.clear(Color::Black); // game end screen overhaul
    }

    else{
        window.clear(Color(10, 10, 15));
        window.draw(starfield);

    }

    if (currentState == GameState::Menu){
        window.draw(titleText);
        //print all items in start menu
        for (const auto& item : mainMenuItems)
        {
            window.draw(item);
        }
    }
    else if (currentState == GameState::StartMenu) {
        Text subTitle = createCenteredText(font,"PRE-GAME SETUP", 40,{400.f, 130.f}, Color::Yellow);
        window.draw(subTitle);
        //Prints all items in start menu
        for (const auto& item : startMenuItems)
            {
                window.draw(item);
            }
    }
    //Prints the tutorial examples , (use better graphics by images/ texture)
    else if (currentState == GameState::Tutorial) {
        Text title = createCenteredText(font, "HOW TO PLAY", 40, {400.f, 120.f}, Color::Yellow);

        //checking for each case individually spacoing of 55 in between
        keys(window, font , "W" , {225.0f,200.0f} , { 45.f,45.f }, Keyboard::isKeyPressed(Keyboard::Key::W) );
        keys(window, font , "S" , {225.0f,255.0f} , { 45.f,45.f }, Keyboard::isKeyPressed(Keyboard::Key::S) );
        keys(window, font , "D" , {280.0f,255.0f} , { 45.f,45.f }, Keyboard::isKeyPressed(Keyboard::Key::D) );
        keys(window, font , "A" , {170.f,255.0f} , { 45.f,45.f }, Keyboard::isKeyPressed(Keyboard::Key::A) );

        keys(window, font , "Space" , {550.f,255.f} , { 160.f,45.f }, Keyboard::isKeyPressed(Keyboard::Key::Space) );
        Text movementControls = createCenteredText(font , "MOVEMENT" , 25 , {225.f,350.f},Color::White);

        Text W = createCenteredText(font , "UP" , 15 , {225.f, 160.f},Color(255,255,255,200));
        Text A = createCenteredText(font , "LEFT", 15 , {110.f,255.f},Color(255,255,255,200)) ;
        Text S = createCenteredText(font , "DOWN", 15 , {225.f,300.f},Color(255,255,255,200)) ;
        Text D = createCenteredText(font , "RIGHT", 15 , {340.f,255.f},Color(255,255,255,200)) ;
        Text parry = createCenteredText(font, "PARRY", 25 , {550.f,350.f} , Color::White) ;

        Text escapeText = createCenteredText(font , "Press ESC To Return", 25.f ,{400.f, 500.f},Color::Cyan) ;
        window.draw(W) ;
        window.draw(A) ;
        window.draw(S) ;
        window.draw(D) ;
        window.draw(parry) ;
        window.draw(escapeText) ;
        window.draw(movementControls);

        window.draw(title);

    }
    else if (currentState== GameState::Shop){
        //shop Title
        Text ShopTitle = createCenteredText(font,"HEART SKINS ", 30.f , {400.f,150.f} , Color::Yellow) ;
        window.draw(ShopTitle) ;
        Sprite previewHeart = playerSprite ;
        previewHeart.setScale({playerSprite.getScale().x*2.f, playerSprite.getScale().y*2.f}) ;
        previewHeart.setPosition({400.f,270.f}) ;
        window.draw(previewHeart) ;
        string skinName ;

        if (currentHeart == 0) {
            skinName = "Red" ;
        }
        else if(currentHeart==1)
            skinName= "Golden" ;
        else if ( currentHeart == 2)
            skinName = "Blue" ;
        else if(currentHeart == 3)
            skinName = "Green" ;
        else if (currentHeart == 4)
            skinName ="Pink" ;
        else
            skinName = "Purple" ;

        Text skinTitle = createCenteredText(font ,"<    " + skinName + "    >" , 30.f,{400.f , 300.f}, Color::White );

        Text controls = createCenteredText(font , "LEFT / RIGHT KEY TO SWITCH BETWEEN KEYS", 30.f , {400.f, 400.f}, Color::Yellow) ;
        window.draw(skinTitle) ;
        window.draw(controls) ;



    }

    else if (currentState == GameState::Paused) {
        Text pausetitle = createCenteredText(font, "PAUSED", 40, { 400.f, 130.f }, Color::Yellow);
        window.draw(pausetitle);
        for (const auto& item : pauseMenuItems) {
            window.draw(item);
        }
    }
    else if (currentState == GameState::Settings) {
        //update static 100 % string to dynamically display variable deals with both SF, BGM, SI (Haras)
        string settingsStr = " SETTINGS MENU \n\n[ Sound: " + to_string(static_cast<int>(soundVolume)) + "% ] <- Left/Right \n[ BGM: " + to_string(static_cast<int>(musicVolume)) + "% ] <- Up/Down\n[ Shake: " + to_string(static_cast<int>(shakeIntensity)) + " ] <- W/S\n\n[Controls:WASD + MOUSE]\n\n(Press ESC for Menu)";
        Text settingsText = createCenteredText(font, settingsStr, 22, {400.f, 300.f}, Color::White);
        window.draw(settingsText);
    }
    else if (currentState == GameState::Death) { // game end screen overhaul (Haras)
        // central spotlight implemented here (Haras)
        float beamtop = 140.f, beamheight = 305.f;
        RectangleShape beamouter({ 190.f, beamheight });
        beamouter.setOrigin({ 95.f, 0.f });
        beamouter.setPosition({ 400.f, beamtop });
        beamouter.setFillColor(Color(30, 8, 14));
        window.draw(beamouter);

        RectangleShape beammid({ 136.f, beamheight });
        beammid.setOrigin({ 68.f, 0.f });
        beammid.setPosition({ 400.f, beamtop });
        beammid.setFillColor(Color(44, 12, 20));
        window.draw(beammid);

        RectangleShape beaminner({ 82.f, beamheight });
        beaminner.setOrigin({ 41.f, 0.f });
        beaminner.setPosition({ 400.f, beamtop });
        beaminner.setFillColor(Color(58, 16, 26));
        window.draw(beaminner);

        // floor of spotlight and drop shadow (Haras)
        CircleShape padouter(95.f, 60);
        padouter.setOrigin({ 95.f, 95.f });
        padouter.setPosition({ 400.f, 445.f });
        padouter.setScale({ 1.0f, 0.38f });
        padouter.setFillColor(Color(90, 30, 44));
        window.draw(padouter);

        CircleShape padinner(70.f, 60);
        padinner.setOrigin({ 70.f, 70.f });
        padinner.setPosition({ 400.f, 445.f });
        padinner.setScale({ 1.0f, 0.38f });
        padinner.setFillColor(Color(110, 38, 54));
        window.draw(padinner);

        CircleShape heartShadow(20.f, 30);
        heartShadow.setOrigin({ 20.f, 20.f });
        heartShadow.setPosition({ 400.f, 453.f });
        heartShadow.setScale({ 1.0f, 0.35f });
        heartShadow.setFillColor(Color(40, 10, 18));
        window.draw(heartShadow);

        // player sprite on the spotlight stage (Haras)
        Sprite centerheart = playerSprite;
        centerheart.setScale({ 1.f, 1.f });
        centerheart.setPosition({ 400.f, 435.f });
        window.draw(centerheart);

        // game over and try again display (Haras)
        Text gameword = createCenteredText(font, "GAME", 78, { 400.f,105.f }, Color::White);
        gameword.setStyle(Text::Bold);
        gameword.setScale({ 1.2f, 1.0f });
        window.draw(gameword);
        Text overword = createCenteredText(font, "OVER", 78, { 400.f,195.f }, Color(255,40,40));
        overword.setStyle(Text::Bold);
        overword.setScale({ 1.2f, 1.0f });
        window.draw(overword);
        // encourage trying again (Haras)
        Text tryagain = createCenteredText(font, "Try Again?...", 22, { 400.f,305.f }, Color::White);
        window.draw(tryagain);
        //making the buttons aesthetically pretty (Haras)
        float buttoncentersX[2] = { 185.f, 615.f };
        float lineyoffsets[2] = { 440.f, 490.f };
        for (float cx : buttoncentersX) {


            for (float ly : lineyoffsets) {
                RectangleShape dotL({ 3.f, 2.f }); dotL.setPosition({ cx - 88.f, ly });

                RectangleShape dashL({ 10.f, 2.f }); dashL.setPosition({ cx - 78.f, ly });
                RectangleShape midBar({ 116.f, 2.f }); midBar.setPosition({ cx - 58.f, ly });
                RectangleShape dashR({ 10.f, 2.f }); dashR.setPosition({ cx + 68.f, ly });
                RectangleShape dotR({ 3.f, 2.f }); dotR.setPosition({ cx + 85.f, ly });

                window.draw(dotL); window.draw(dashL); window.draw(midBar); window.draw(dashR); window.draw(dotR);

            }
        }





        for (const auto& item : deathMenuItems) {
            window.draw(item);
        }

    }

}


// Player movement function with box containment (Bazil comments in this function)
void handlePlayerMovement(Sprite& playerSprite, float speed, const RectangleShape& battleBox, float dt, bool useBox) {
    //we need the sprite, the speed and the rectangle box to trap the player to prevent excess movement
    Vector2f pos = playerSprite.getPosition(); //apparently the SFML library already has x and y stored so it doesn't need you to declare them. Cool ig
    //the movement stuff is self explanatory except one thing which I wrote down
    if (Keyboard::isKeyPressed(Keyboard::Scan::S)) {
        pos.y += speed * dt * 30.0f; //so over here, the tutorial told me that y actually increases as we go down. Be careful of this omg I hate SFML
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::W)) {
        pos.y -= speed * dt * 30.0f;
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::D)) {
        pos.x += speed * dt * 30.0f;
    }
    if (Keyboard::isKeyPressed(Keyboard::Scan::A)) {
        pos.x -= speed * dt * 30.0f;
    }

    // exact explanation at the bottom
    Vector2f boxPosition = battleBox.getPosition(); //obtaining the exact coordinates of the box centre
    Vector2f boxSize = battleBox.getSize(); //getting the width and height of the box because I tried to define the height and width
                                                //directly into the code itself and I realized that I might neeed to change the box dimensions so I used getSize instead. Just to give myself some leeway
    float left = boxPosition.x - (boxSize.x / 2.0f) + 8.0f;
    float right = boxPosition.x + (boxSize.x / 2.0f) - 25.0f;
    float top = boxPosition.y - (boxSize.y / 2.0f) + 8.0f;
    float bottom = boxPosition.y + (boxSize.y / 2.0f) - 8.0f;

    if (!useBox) {
        // for phase 2 cuz we gotta allow player to move whereever fr
        left = 8.0f;
        right = 800.0f - 8.0f;
        top = 8.0f;
        bottom = 600.0f - 8.0f;
    }

    if (pos.x < left) {
        pos.x = left;
    }
    if (pos.x > right) {
        pos.x = right;
    }
    if (pos.y < top) {
        pos.y = top;
    }
    if (pos.y > bottom) {
        pos.y = bottom;
    }
    playerSprite.setPosition(pos);
    //ok so basically I wanted to do the Undertale box thing where the player is trapped inside a box. Initially my logic included that if my sprites position and the box line position was the same
    //(next line)then the player position would be set to the box position. But I noticed that the half the heart was actually clipping through, so I added a 12 pt barrier which was too large and then
    //(next line) so I experimented a bit and found that 8 was a good value
}



//New fuunction for attack
//attack spawns at te top and slowly comes down
void verticle(vector<Projectile>& pillars,float xposition, int difficultyIndex)
{
    //created a new varible of type projectile assign it its attrigbutes position updated in update projectile function
    Projectile magic ;
    //-ev position so it seems they spawn
    magic.position={xposition, -150.f} ;
    //difficulty changes speed
    magic.velocity={0.0f, 12.f+5*(difficultyIndex-1)} ;
    magic.isLaunched=true ;
    magic.tracksPlayer = false ;
    magic.leavesTrail = false ;
    magic.hitBox = 50.f ;
    pillars.push_back(magic) ;


}


void burst(vector<Projectile>& machineGun, int difficultyIndex, Vector2f bossPos, float launchInterval){
    //creating 20 randomly created crystals to remove predictability
    for (int i =0; i <13 +5*(difficultyIndex-1) ; i++){
        //new projectile to push to vector
        Projectile crystal ;
        crystal.position = bossPos ;
        crystal.isLaunched = true ;
        crystal.leavesTrail = false ;
        crystal.tracksPlayer = false ;
        crystal.hitBox = 12.0 ;
        //so it doesnt shoot al at once
        crystal.spawnDelay = i* launchInterval/4.f ;
        //random speeds and angle

        float angle ;
        //setting angle to cover the field setting rotation setting speed and calculating velocity ;
        angle = 28.f + rand()%125 ;
        crystal.rotation= angle- 90 ;
        float speed ;
        speed = 5.0f + rand()%5 +3.f*(difficultyIndex -1);
        angle = angle * 3.14157f/180.0 ;
        crystal.velocity ={speed *cos(angle), speed*sin(angle)};
        //new element
        machineGun.push_back(crystal);


    }
}


// The first wave stuff, crescent bullets (Bazil comments in this function)
//bullets need to be launched more frequently
void crescent(vector<Projectile>& activeProjectiles, float crescentcentre, float spawnposition, float spawnInterval, float launchInterval) {
    int totalBullets = 7  ;
    float totalSpawnTime = (totalBullets - 1) * spawnInterval;
    for (int i = 0; i < totalBullets; i++) {
        float step = 1.0f / (totalBullets - 1);
        float horizontalshift = -0.5f+(i*step); //shift for each bullet
        float horizontalspawn = crescentcentre+(horizontalshift*180.0f); //basically the spawn position, change the 180 to a higher number to create a bigger spread and vice versa
        float arcDip = (0.25f-(horizontalshift*horizontalshift))*60.0f; //same logic as above one
        Vector2f spawnPos= {horizontalspawn,spawnposition-arcDip}; //basically the position it spawns at, I hope this doesn't need an explanation
        Projectile proj;
        proj.position = spawnPos;
        proj.isLaunched = false;
        proj.spawnDelay = i * spawnInterval;

        // Staggers each bullet sequentially using the local interval
        proj.launchTimer = (totalSpawnTime - proj.spawnDelay) + launchInterval + (i * 1.0f); //the 1.0f is CUZ OMG I HAD TO DIE AND DIE AGAIN JUST TO PUT THIS INTERVAL IN OMG I HATE SFML

        proj.tracksPlayer = true;
        proj.leavesTrail = false;

        activeProjectiles.push_back(proj);
    }
}

// The second wave stuff, horizontal projectiles (Bazil comments in this function)
void horizontal(vector<Projectile>& activeProjectiles, const RectangleShape& battleBox, float projectileSpeed, float projectileposition) {
    Vector2f boxP = battleBox.getPosition(); //boxPosition too large a name and im lazeh :P
    Vector2f boxSize = battleBox.getSize(); //same thing

    float height = boxP.y + projectileposition; //basically the height at which the projectile spawns, we HATE this function
    float left = boxP.x - boxSize.x / 2.0f-120.0f;
    float right = boxP.x + boxSize.x / 2.0f+120.0f;
    //ok so I added the +90 and -90 because initially I dind't have them and the projectiles were spawning like RIGHT on the border and it was so weird ew. So I added the +90 and -90
    Projectile pLeft;
    pLeft.position = {left, height+20.0f};
    pLeft.velocity = {projectileSpeed, 0.0f};
    pLeft.leavesTrail = true;
    pLeft.launchTimer = 0.8f  ;

    Projectile pRight;
    pRight.position = {right, height};
    pRight.velocity = {-projectileSpeed, 0.0f};
    pRight.leavesTrail = true;
    pRight.launchTimer = 0.8f ;


    activeProjectiles.push_back(pLeft);
    activeProjectiles.push_back(pRight);
}


// This is the attack where the projectiles spawn in a circle and burst outwards (Bazil comments in this function)
void pinwheel(vector<Projectile>& activeProjectiles, Vector2f centerPos, float projectileSpeed) {
    int numBullets = 8;
    for (int i = 0; i < numBullets; i++) { //this is basically the loop for each of the bullets(pretty self explanatory)
        float angle = (2.0f * 3.14 * i) / numBullets; // ok so over here, I used the cmath library for the circle angles because its the easiest way Lord praise "Basic Coding" yt channel and their showcase
        //(next line) of cmath functions
        //the logic for the angle is pretty simple, so basically what im doing is im using radians for the angle of each projectile seperately
        // 2𝝅 is the total angle and I multiply it by i to get the angle of each seperate projectile. So for the first projectile, i is 0 so angle comes out to be 0, 2nd projectile i is 1 so 2𝝅*1/8 = 𝝅/4 or 45 degrees. Rest is simple
        Vector2f dir = {cos(angle), sin(angle)}; //simple SOHCAHTOA stuff im not explaining this

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

void homingdarts(vector<Projectile>& activeProjectiles, const RectangleShape& battleBox, float centerX) {
    Vector2f boxSize = battleBox.getSize();//simple enough, obtaining size, copy pasted this mweheheh (FROM THE CODE ABOVE NOT AI OMG)

    for (int i = 0; i < 4; i++) {
        Projectile proj;
        float horizontallocation = -60.0f + i * 40.0f; // its basically where the projectile spawns in the horizontal position, -60 being the initial one for the first projectile
        proj.position = {centerX + horizontallocation, battleBox.getPosition().y - boxSize.y / 2.0f - 15.0f}; //initial projectile location
        proj.isLaunched = false;
        proj.launchTimer = 0.7f + (i * 0.2f); //launched at intervals with i
        proj.tracksPlayer = true;      //well like, duh
        proj.leavesTrail = false; //no trail for this one
        proj.steeringtime = 1.2f; // steer projectile for 1.2 sceonds and then it flies of screen, //this is for homing projectiles (Haras)
        activeProjectiles.push_back(proj);
    }
}

//this is to check if the projectiles of the wave have launched because without this, my first crescent had just started firing and the next one would start spawning like holy(Bazil)
bool allLaunched(const vector<Projectile>& activeProjectiles) {
    for (const auto& proj : activeProjectiles) {
        if (proj.isLaunched==false || proj.spawnDelay > 0.0f) {
            return false;
        }
    }
    return true;
}

// update projectiles but it makes sense to me brrrrr(Bazil comments in this function)
void updateProjectiles(vector<Projectile>& activeProjectiles, float t, unsigned int width, unsigned int height, Vector2f playerPos, float projectileSpeed) {
    for (int i = 0; i < (int)activeProjectiles.size(); ) { //omg I tried doing just activeProjectiles.size() initially and it gave me an error, apparently the proper format is encasing the int in brackets and then it worked. The more I know ig
        Projectile& p = activeProjectiles[i];   //p is the current projectile in the list, we go through them with i

        // projectile hasn't appeared yet, just count down its spawn delay and skip everything else for the next frame
        if (p.spawnDelay > 0.0f) {
            p.spawnDelay -= t;
            if (p.spawnDelay > 0.0f) {
                i++;
                continue;
            }
        }
        if (!p.isLaunched) { // if its NOT launched
            p.launchTimer -= t; //subtraction from the launch timer, its also why we subtracted from the spawnDelay above
            if (p.launchTimer <= 0.0f) { //has the timer hit 0? let the projectile out the prison :(
                if (p.tracksPlayer) { //is it gonna home in?
                    Vector2f direction = {playerPos.x - p.position.x, playerPos.y - p.position.y};
                    float length = sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length != 0.0f) {
                        direction /= length; //to give a unit vector so it doesn't fly at mach speed (unless I change this and decide to troll the players ig)
                    }
                    float launchspeed = (p.steeringtime > 0.0f) ? projectileSpeed * 0.5f : projectileSpeed;
                    p.velocity = direction * launchspeed;
                }
                p.isLaunched = true; //its out of the prison yay
            }
        }
        else { //projectile is launched
            //implemented delta-time, also implemented homing missles (Haras)
            if (p.steeringtime > 0.0f) {
                p.steeringtime -= t;
                Vector2f desired = { playerPos.x - p.position.x, playerPos.y - p.position.y };
                float desiredlength = sqrt(desired.x * desired.x + desired.y * desired.y);
                if (desiredlength > 0.0f) {
                    desired /= desiredlength;
                    float dartspeed = projectileSpeed * 0.5f;
                    float turnrate = 1.5f; // if you lower this value then subtler curve, higher then more accurate homing
                    p.velocity += desired * (dartspeed * turnrate * t);
                    float curvespeed = sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
                    if (curvespeed > 0.0f) {
                        p.velocity = (p.velocity / curvespeed) * dartspeed;
                    }
                }
            }
            p.position += p.velocity * t * 30.0f; //takes your point and adds the velocity onto it every frame which moves it
            // if this projectile leaves a trail, insert the trail every frame
            if (p.leavesTrail) {
                p.trailTimer += t;
                if (p.trailTimer >= 0.03f) {
                    p.trailTimer = 0.0f;
                    p.trail.push_back({p.position, 1.0f});
                }
            }
        }



        // age the trail points
        for (int j = 0; j < (int)p.trail.size(); ) {
            p.trail[j].lifeTimer -= t;
            if (p.trail[j].lifeTimer <= 0.0f) {
                p.trail.erase(p.trail.begin() + j);   //remove the trail after its lifetime is done
            } else {
                j++;
            }
        }
        bool isOffScreen = (p.position.y > height + 40 || p.position.y < -200 || p.position.x < -100 || p.position.x > width + 100);
        if (isOffScreen && p.isLaunched) {
            activeProjectiles.erase(activeProjectiles.begin() + i);   //remove the projectile once it goes offscreen, rip my laptop holy lag
        } else {
            i++;
        }
    }
}


// Map collisions go brrrrrrrrrrr heahehaheahehaheha
bool mapcollission(const sf::Image &mapimage, const sf::Sprite &mapsprite, Vector2f playerpos) {
    Vector2f mapposition = mapsprite.getPosition();                      // map centre
    sf::Vector2u imagesize = mapimage.getSize();                         // map size
    int pixelx = (playerpos.x - mapposition.x) / mapsprite.getScale().x; // player positnon with respect to x cuz yeah
    int pixely = (playerpos.y - mapposition.y) / mapsprite.getScale().y; // player positnon with respect to y cuz yeah
    if (pixelx < 0 || pixelx >= (int)imagesize.x || pixely < 0 || pixely >= (int)imagesize.y) {
        return true;
    }
    Color pixelcolour = mapimage.getPixel({static_cast<unsigned int>(pixelx), static_cast<unsigned int>(pixely)});
    bool stonePath = (pixelcolour.r < 200 && pixelcolour.b > 50);
    return !stonePath;
}



// For the collisions with projectiles logic here we go brrrrrrrrrrrr (Bazil comments in this function)
bool collision(const vector<Projectile>& activeProjectiles, Vector2f playerPos ) {
    for (const auto& proj : activeProjectiles) {
        if (proj.isLaunched&&!proj.parried) { //well like, yeah. It should only run if its launched , also shouldnot work if parried
            float distX = playerPos.x - proj.position.x; //basically the horizontal distance between the projectile and the player
            float distY = playerPos.y - proj.position.y; //same thing but vertical
            if (sqrt(distX * distX + distY * distY) < proj.hitBox) { //pythagoras theorem shEEEESh
                return true;
            }
        }

        for (const auto& hazard : proj.trail) { //same thing over here but for the trail

            float distX = playerPos.x - hazard.position.x;
            float distY = playerPos.y - hazard.position.y;
            if (sqrt(distX * distX + distY * distY) < proj.hitBox) {
                return true;
            }
        }
    }
    return false;
}


// New line of code for boss proejctiles only runs if the object has been parried
bool bossCollision(const vector<Projectile>& activeProjectiles, Vector2f bossPos) {
    for (const auto& proj: activeProjectiles) {
        if(proj.parried == true){
            float dx = bossPos.x - proj.position.x ;
            float dy  = bossPos.y -proj.position.y ;
            if (sqrt(dx*dx+dy*dy)<proj.hitBox){
                return true;
            }

        }
    }
    return false ;
}

void parryProj(vector<Projectile>& Projectile , Vector2f playerPos,Vector2f bossPos){

    //For all projectiles
    for (auto& proj :Projectile){
        if(proj.isLaunched  ){
            //calculating right distance for parry

            float dx = playerPos.x - proj.position.x ;
            float dy =playerPos.y -proj.position.y;
            if (sqrt(dx*dx+dy*dy) <=28.f&& sqrt(dx*dx+dy*dy)>=12.f &&dy>=0) {
                proj.tracksPlayer = false ;
                //go to boss back

                Vector2f bossDisplacement= {bossPos.x-proj.position.x , bossPos.y-proj.position.y} ;
                //current projectile differce from boss
                float length = sqrt(bossDisplacement.x*bossDisplacement.x+bossDisplacement.y*bossDisplacement.y) ;
                if (length!= 0){
                    bossDisplacement /=length ; //this is for unit vector so we can give new speed
                }
                proj.velocity = bossDisplacement * 20.0f ;

                proj.parried = true ;

            }
        }
    }

}


//new function to check for parrying
void updateParrying(Sprite& blazeRod,const Sprite& playerSprite ,Vector2f bossPos ,  const Sprite& icon1,
                 CircleShape& cooldownRing, RectangleShape& cover ,
                 vector<Projectile>& activeProjectiles,vector<Projectile>& crystalBurst,
                 bool& isParrying, float& parryTimer , float& cooldown, float dt){
    // keeps balze rod by the side of player
    blazeRod.setPosition({playerSprite.getPosition().x+5.f, playerSprite.getPosition().y+6.0f}) ;


    if (isParrying) {
        //finds time passed since button press
        parryTimer+=dt;

        float progress = parryTimer/0.15 ;//ratio of full swing
        parryProj(activeProjectiles,playerSprite.getPosition(),bossPos) ; //calls the function so the projectile is redirected if hit exact
        //if swing complete reset
        if(progress>=1.0f){
            isParrying  = false ;
            blazeRod.setRotation(degrees(0)) ;
            cooldown = 2.15f ; // if you parry can not parry for next 2 seconds//THis also includes
            //the Motion of the swing of parry
        }
        else
            {
            //else set new rotation
            float newAngle = -140.f*progress;
            blazeRod.setRotation(degrees(newAngle)) ;
            }
        }
        if (cooldown>0.0f) {
            cooldown -= dt ;
            //dim it
            blazeRod.setColor(Color(120,120,120,100));
            //New position of cover
            cover.setPosition({icon1.getPosition().x,icon1.getPosition().y+(40-cooldown*20.f)});
            cover.setFillColor(Color::Black);
            cooldownRing.setOutlineColor(Color(255,255-255*cooldown/2.15,0));

        }
        //
        else{
            blazeRod.setColor(Color::White) ;
            cover.setFillColor(Color::Transparent);
            cover.setPosition(icon1.getPosition());
            cooldownRing.setOutlineColor(Color::Cyan) ;
        }
}


int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    unsigned int width = 800;
    unsigned int height = 600;

    RenderWindow window(VideoMode({ width, height }), "Bullet Requiem");
    window.setFramerateLimit(60); // increased fps from 30 to 60 for smoother gameplay (Haras)

    View view(FloatRect({0.0f, 0.0f}, {static_cast<float>(width), static_cast<float>(height)}));
    window.setView(view);


    //Creating vector arrays to store info about the stars falling in back ground
    //Then call void function to initilaize the vector

    vector<Star> myStars;
    starfieldCreation(myStars);
    //vertex array used in background since it is much more efficient

    VertexArray starBackground(PrimitiveType::Points);


    vector<string> heartFiles={
        "Assets/RedHeart.png",
        "Assets/GoldenHeart.png",
        "Assets/BlueHeart.png",
        "Assets/GreenHeart.png",
        "Assets/PinkHeart.png",
        "Assets/PurpleHeart.png"
    };

    float scaling[6] = {0.6, 0.6 , 0.42 , 0.6 , 0.6 , 0.50 } ;

    // Load Textures & Fonts
    Texture hearts;
    if (!hearts.loadFromFile(heartFiles[0])) {
        return -1;

    }
    hearts.setSmooth(false) ;
    int currentHeart= 0 ;


    Font font;
    if (!font.openFromFile("Assets/font.ttf")) {
        return -1;
    }
    Texture texture2;
    //resize this pls
    if (!texture2.loadFromFile("Assets/frostattack.png")) {
        return -1;
    }
    Texture texture3;
    if (!texture3.loadFromFile("Assets/Bosssprite.png")) {
        return -1;
    }
    // Speech bubble setup
    RectangleShape speechBubble({180.0f, 65.0f});
    speechBubble.setFillColor(Color::White);
    speechBubble.setOutlineColor(Color::Black);
    speechBubble.setOutlineThickness(4.0f);

    Text speech(font);
    speech.setString("Testing 12-step sequence");
    speech.setCharacterSize(12);
    speech.setFillColor(Color::Black);


    //further Work on background


    // Title text set up centered at x=400, y=120;
    Text titletext(font, "BULLET REQUIEM", 55);
    FloatRect titleBounds = titletext.getLocalBounds();
    titletext.setFillColor(Color(255, 40, 40));
    titletext.setStyle(Text::Bold);
    titletext.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y / 2.f});
    titletext.setPosition({400.0f, 120.0f});

    //Further work on Pause screen
    RectangleShape DimScreen (Vector2f(window.getSize())) ;
    DimScreen.setFillColor(Color(0,0,0,150));

    //Set the current state to menu so that when we enter game we at the menu
    GameState currentState = GameState::Menu;
    int hoveredIndex = -1;

    // Main Menu Setup

    vector<string> mainOptions = {"START GAME", "SETTINGS", "EXIT"};
    vector<Text> mainMenuItems = createMenuItems(font, mainOptions, 280.f, 65.f, 30);
    //Did this to move tutorial on main page
    mainOptions.push_back("TUTORIAL");
    //using [0] because of create menu item returns vector text
    mainMenuItems.push_back(createMenuItems(font, {mainOptions[3]},550.f,65.f,30.f,700.f)[0]);

    mainOptions.push_back("SKINS") ;

    mainMenuItems.push_back(createMenuItems(font,{mainOptions[4]},550.f, 65.f,30.f,70.f)[0]) ;

    // Start Sub-Menu Setup (Difficulty & Tutorial)
    //Add more difficulties
    vector<string> difficulties = {"EASY", "NORMAL", "BULLET HELL"};
    int difficultyIndex = 1; // Default to NORMAL

    //Removed tutorial to make the screen cleaner
    vector<string> startOptions = {
        "DIFFICULTY: " + difficulties[difficultyIndex],
        "ENDLESS",
        "NEW GAME",
        "BACK"
    };

    vector<Text> startMenuItems = createMenuItems(font, startOptions, 240.f, 60.f, 26);

    vector<string> pauseOptions = { "RESUME", "SETTINGS", "MAIN MENU", "EXIT GAME" };
    vector<Text> pauseMenuItems = createMenuItems(font, pauseOptions, 240.f, 60.f, 26);

    // for game end menu overhaul (Haras)
    vector<Text> deathMenuItems = {


        createCenteredText(font, "PERSEVERE", 22,{ 185.f, 465.f }, Color(255,40,40)),
        createCenteredText(font, "GIVE UP", 22,{ 615.f, 465.f }, Color::White)
    };
    GameState previousState = GameState::Menu;


    // Battle Box Setup
    RectangleShape battleBox({380.0f, 120.0f});
    battleBox.setFillColor(Color::Black);
    battleBox.setOutlineColor(Color::White);
    battleBox.setOutlineThickness(3.0f);
    battleBox.setOrigin({380.0f / 2.0f, 120.0f / 2.0f});
    battleBox.setPosition({width / 2.0f, height / 2.0f + 100.0f});

    //health bar
    RectangleShape healthBar({300.f , 20.f}) ;
    healthBar.setOrigin({150.f, 10.f}) ;
    healthBar.setPosition({235.0f , 30.0f}) ;
    healthBar.setFillColor(Color(57, 255, 20));

    //health bar boundary and underneath
    RectangleShape boundaryRect({300.0f, 20.0f});
    boundaryRect.setOrigin({150.f,10.0f}) ;
    boundaryRect.setPosition({235.0f, 30.0f}) ;
    boundaryRect.setFillColor(sf::Color(190,44 , 35)) ;
    boundaryRect.setOutlineThickness(4.f);
    boundaryRect.setOutlineColor(sf::Color(140,90,43));
    //bossHealth
    RectangleShape bossHealth({500.0f,20.0f}) ;
    bossHealth.setOrigin({250.0f,10.0f}) ;
    bossHealth.setPosition({400.0f,100.0f}) ;
    bossHealth.setFillColor(Color(155, 40, 230)) ;

    //boss health bar underneath and border :
    RectangleShape healthBoundary({500.f,20.f}) ;
    healthBoundary.setOrigin({250.f,10.f}) ;
    healthBoundary.setPosition({400.f,100.f}) ;
    healthBoundary.setOutlineThickness(4.f) ;
    healthBoundary.setOutlineColor(Color(220,25,40)) ;
    healthBoundary.setFillColor(Color(50,180,255)) ;


    float cooldown = 0.0f ; //cooldwon for next swing
    bool isParrying = false ;
    float parryTimer = 0.0 ; // how many seconds gone
    float parryDuration = 0.15f ; // total swing duration

    Texture rod ;
    rod.loadFromFile("Assets/NewBlazeRod.png") ;

    Sprite blazeRod(rod) ;
    blazeRod.setScale({0.37f,0.37f}) ;

    Vector2u rodSize = rod.getSize() ;
    //set to left corner
    blazeRod.setOrigin({0,float(rodSize.y)}) ;
    blazeRod.setPosition({406.f,306.f}) ;

    // Boss Sprite Setup
    Sprite Bosssprite(texture3);
    FloatRect bossbounds = Bosssprite.getLocalBounds();
    Bosssprite.setScale({0.1f, 0.1f});
    Bosssprite.setOrigin({bossbounds.size.x / 2.f, bossbounds.size.y / 2.f});
    Bosssprite.setPosition({width / 2.0f, 240.0f });
    Text bossName = createCenteredText(font, "MEHMOOD SAHAB",15 , {400.f,75.f}, Color(255,255,255,200)) ;

    //Texture magic/ new projectile;
    Texture magic ;
    if (!magic.loadFromFile("Assets/Magic atack.png")) {
        return -1;
    }
    Sprite MagicAttack(magic);

    MagicAttack.setTexture(magic);
    MagicAttack.setScale({5.f,5.f});
    Vector2u proj2size = magic.getSize() ;
    MagicAttack.setOrigin({proj2size.x/2.0f,proj2size.y/2.0f}) ;
    MagicAttack.setPosition({200.f,300.f});

    //Crystal Projectile that will be fired like a machine gun randomly
    Texture crystal ;

    if(!crystal.loadFromFile("Assets/fallingCrystal.png"))
        return -1 ;

    Sprite fallingCrystal(crystal) ;
    fallingCrystal.setTexture(crystal);
    fallingCrystal.setScale({1.f,1.f});
    Vector2u proj3size = crystal.getSize() ;
    fallingCrystal.setOrigin({proj3size.x/2.0f,proj3size.y/2.0f}) ;


    // Player Sprite Setup
    Sprite Playersprite(hearts);
    FloatRect playerbounds = Playersprite.getLocalBounds();
    Playersprite.setScale({scaling[currentHeart], scaling[currentHeart]});
    Playersprite.setOrigin({playerbounds.size.x / 2.0f, playerbounds.size.y / 2.0f});
    Playersprite.setPosition(battleBox.getPosition());

    //playerIcon beside the screen
    Sprite playerIcon (hearts) ;
    playerIcon.setScale({scaling[currentHeart], scaling[currentHeart] }) ;

    playerIcon.setOrigin({playerbounds.size.x/2.f,playerbounds.size.y/2.0f}) ;
    playerIcon.setPosition({25.f,30.f}) ;


    //Indicator of parry with icon
    CircleShape cooldownRing ({15.f}) ;
    cooldownRing.setOrigin({15.f,15.f}) ;
    cooldownRing.setPosition(playerIcon.getPosition()) ; // setting origin as same so it covers uniformally
    cooldownRing.setFillColor(Color::Transparent) ;
    cooldownRing.setOutlineThickness({4.f}) ;
    cooldownRing.setOutlineColor(Color::Cyan) ;

    //rectangle shape for covering the ring to show cooldown ;
    RectangleShape cover ({40.f,44.f}) ;
    cover.setOrigin({20.f,22.f}) ;
    cover.setPosition(playerIcon.getPosition()) ;
    cover.setFillColor(Color(120,120,30,230)) ;


    //standard speed

    float speed = 7.0f ;

    // Projectile & Wave Setup

    vector<Projectile> activeProjectiles;
    //standard attacks on normal difficulty

    //New projectile class created with different stats
    vector<Projectile> magicPillar ;

    vector <Projectile> crystalBurst ;

    //Change this
    float projectileSpeed = 20.0f ;
    float launchInterval = 0.3f  ;


    //system clocks
    Clock waveTriggerClock;

    Clock frameClock;
    Clock clock ;

    Clock magicTimer ;
    //for falling crystal
    Clock burstTimer;

    float burstDelay = 10.0f ;

    float waveInterval = 0.8f ;
    int wavecount = 0;

    int currentphase = 1;     // the phase we're at, duh
    float invulnerabilitytimer = 0.0f; // invincibility frames cuz im generous :)

    // Projectile sprite template
    Sprite projSprite(texture2);
    FloatRect projBounds = projSprite.getLocalBounds();
    projSprite.setScale({0.8f, 0.8f});
    projSprite.setOrigin({projBounds.size.x / 2.f, projBounds.size.y / 2.f});

     // The Map texture yayyy 2nd phase woohoo
    vector<string> mapFiles = {
        "Assets/map_track.png",
        "Assets/map_track2.png",
        "Assets/map_track3.png"
    };
    vector<Texture> mapTextures(mapFiles.size()); //store the textures in a vector (THIS IS SO COOL)
    vector<sf::Image> mapImages(mapFiles.size()); //need the images too for the pixels
    for (size_t i = 0; i < mapFiles.size(); i++) {
        if (!mapTextures[i].loadFromFile(mapFiles[i]) || !mapImages[i].loadFromFile(mapFiles[i])) { //loading each texture, outputting an error if it doesn't happen
            return -1;
        }
    }

    int mapindex = 0; //to cycle between the maps
    float mapbiggie = 1.4f; //map was too small when I initially used it soo yayy
    Sprite mapsprite(mapTextures[0]); //
    mapsprite.setScale({mapbiggie, mapbiggie}); //heh, just scaling

    float halfwidth = mapTextures[2].getSize().x * 0.5f; //for map3 cuz it wasn't detecting the endpoint at the end of map 3

    vector<float> starter = {
        390.f, //for the first map
        mapTextures[1].getSize().x * 0.5f, //2nd map
        halfwidth //3rd map
    };

    vector<Vector2f> finishline = {
        {45.f, 115.f}, //first map
        {mapTextures[1].getSize().x * 0.305f, mapTextures[1].getSize().y * 0.055f},// 2nd map
        {halfwidth, mapTextures[2].getSize().y * 0.065f} //3rd map
    };

    auto placemapstart = [&]() { //this is a function notation for when you wanna use functions in main. the & allows it to use all variables declared above it as referecne. omg tis so cool :3
        mapsprite.setTexture(mapTextures[mapindex], true);
        float texWidth = static_cast<float>(mapTextures[mapindex].getSize().x); //finds the pixel width of the map
        mapbiggie = static_cast<float>(width) / texWidth; //divides the screen width by pixel width
        mapsprite.setScale({mapbiggie, mapbiggie}); //makes sure the zooming in doesn't cut parts of the map out dyring scaling
        float mapheight = mapsprite.getGlobalBounds().size.y;
        mapsprite.setPosition({0.0f, height - mapheight}); //put the rest of the map above the screen
    };

    auto playerspawnmap = [&]() {
        placemapstart();
        Playersprite.setPosition({mapsprite.getPosition().x + starter[mapindex] * mapbiggie, height - 50.0f});
    };
    float invulnerabilitytimerMagicAttack = 0.0f ;
    //created a diff for this 1 second immunity too much for this


    auto resetToPhase1 = [&]() {
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
        magicTimer.restart();
        burstTimer.restart();
        bossHealth.setSize({500.0f,20.0f}) ;
        invulnerabilitytimerMagicAttack = 0.0f ;
    };


    // Hazard trail dot shape
    //Use the created flame sprite / texture pack here
    CircleShape hazardDot(3.0f);
    hazardDot.setFillColor(Color(255, 69, 0, 200));
    hazardDot.setOrigin({3.0f, 3.0f});

    //used to change current state of game from menu too playing ;
    //change this and add an endless mode also make it go back to menu if escape pressed during the playing stage
    int changeState = 0 ;

    //load SoundBuffer before while window.isOpen loop, so it only loads once (Haras)
    SoundBuffer shootBuffer;
    if (!shootBuffer.loadFromFile("Assets/snd_damage.mp3")) {
        return -1; // if audio file doesn't exist it will exit (Haras)
    }
    //load Sound which actually plays/makes sound (Haras)
    Sound shootSound(shootBuffer);

    float soundVolume = 50.0f; // SF variable declaration, I lowered the default value (Haras)
    float musicVolume = 20.0f; // BGM variable dec (Haras)

    float shakeIntesity = 2.0f; // Default pixel offset for SI, lowered it for smoother experience (Haras)
    float shakeTimer = 0.0f; // Will keep track of duration of active shake (Haras)

    Music bgMusic; // (Haras)
    if (!bgMusic.openFromFile("Assets/bgm.ogg")) {
        return -1;
    }
    bgMusic.setVolume(musicVolume);
    bgMusic.setLooping(true);
    bgMusic.play();


    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();

        //gets recent mouse position
        //this statemet check that if there new games is pressed we change menu too the recent menu and end the previous menu
        if (changeState==0 || changeState==3){ // for game end menu overhall (Haras)

            //required functions for menu
            //implemented delta-time (Haras)
            starfieldBackground(myStars, starBackground, dt);
            // integrated soundvolume and musicvolume variable
            menu_Interaction(window, view, currentState, hoveredIndex,
                 mainMenuItems,deathMenuItems , startMenuItems, pauseMenuItems, startOptions,
                 difficultyIndex, difficulties, font, changeState, previousState, speed, projectileSpeed, waveInterval, launchInterval,
                 soundVolume, musicVolume, shakeIntesity, currentHeart, hearts, heartFiles, scaling, Playersprite, playerIcon);

            bgMusic.setVolume(musicVolume); //this allows live update from within settings menu

            //Making new game option actually start new game, not start from where left off (Haras)
            if (changeState==0 || changeState==3) { // Also added for death menu so shake not apply to it (Haras)
                if (previousState != GameState::Paused) {
                    Playersprite.setPosition(battleBox.getPosition()); //This will put player position back on center
                    activeProjectiles.clear(); //Clears all previous projectiles
                    wavecount = 0; //restart waves

                    currentphase = 1; // back to phase 1 cuz yes
                    mapindex = 0;
                    invulnerabilitytimer = 0.0f; // no leftover invunerability frames cuz I can't have the players getting too comfy
                    waveTriggerClock.restart(); //resets attack timings
                    shakeTimer = 0.0f; // resets shakeTimer
                    healthBar.setSize({300.f,20.f})  ;
                    healthBar.setFillColor(Color(57,255,20));
                    bossHealth.setSize({500.f,20.f}) ;
                    magicTimer.restart() ;
                    burstTimer.restart() ;
                    magicPillar.clear() ;
                    crystalBurst.clear();
                    isParrying = false ;
                    cooldown = 0.f ;
                    placemapstart(); // map back to start (bottom of the map on the bottom of the screen)

                    burstDelay = 10.f ;

                    parryTimer = 0.0f ;
                    blazeRod.setColor(Color::White);
                    blazeRod.setRotation(degrees(0.0f)) ;
                }
                frameClock.restart(); //prevent dt jump on first frame
                view.setCenter({ width / 2.0f , height / 2.0f }); // snaps back to center
                window.setView(view); // implements the view

            }
            // integrated soundvolume and musicVolume variable
            rendering(window, starBackground, currentState, titletext,mainMenuItems,deathMenuItems,
                       startMenuItems, pauseMenuItems, font, difficulties[difficultyIndex], soundVolume,
                       musicVolume, shakeIntesity, DimScreen, Playersprite , currentHeart);
            window.display() ;

        }
        else {
            if(changeState ==2){
                while (const optional<Event> event = window.pollEvent()) {
                    if (event->is<Event::Closed>()) {
                        window.close();
                    }
                    else if (const auto* keyPressed = event->getIf<Event::KeyPressed>()) {


                            if(keyPressed->scancode == Keyboard::Scancode::Escape) {
                                currentState = GameState::Paused; //
                                previousState = GameState::Paused;
                                hoveredIndex = -1;
                                changeState = 1 ;
                            }
                            else if(keyPressed->scancode == Keyboard::Scancode::Space&&!isParrying&& cooldown<=0.0f&& currentphase==1) {
                                isParrying =true ;
                                parryTimer = 0.0f ;
                            }


                    }
                }


                // Call player movement with box to prevent excess movement hahahahah suffer players(Bazil)
                //implemented delta-time (Haras)

                handlePlayerMovement(Playersprite, currentphase == 2 ? speed * 0.9f : speed, battleBox, dt, currentphase == 1);


                updateParrying(blazeRod, Playersprite, Bosssprite.getPosition(), playerIcon,
                        cooldownRing, cover, activeProjectiles, crystalBurst,
                        isParrying, parryTimer, cooldown, dt) ;

                // the speed of the map scroll cuz, yeah
                float mapscrollspeed = 40.0f;
                if (currentphase == 2) //must ONLY happen in the 2nd wave
                {
                    //the following code is cuz omg I didn't include this and the map scrolled off the screen AND IT CONSIDERED IT OUT OF BOUNDS AND I WAS DYING AAAA
                    mapsprite.move({0.0f, mapscrollspeed * dt});
                    if (mapsprite.getPosition().y > 0.0f)
                    {
                        mapsprite.setPosition({mapsprite.getPosition().x, 0.0f});
                    }

                    if (mapsprite.getPosition().y >= 0.0f)
                    {
                        //this is for us reaching the endpoint cuz I mean we gotta be able to move on to the next one I suppose
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
                                resetToPhase1();
                            }
                        }
                    }
                }

                // first phase stuff ugh(Bazil comments here)
                if (currentphase == 1) {
                    bool nextwave = false;
                    if (wavecount <= 3) { //this is only for the first crescent waves
                        nextwave = allLaunched(activeProjectiles);
                    }
                    else {
                        nextwave = activeProjectiles.empty();
                    }
                    if (waveTriggerClock.getElapsedTime().asSeconds() >= waveInterval && nextwave) { //this is to rapid fire waves cuz I mean, im not merciless
                        waveTriggerClock.restart(); //set it to 0 for the next projectile
                        float spawnposition = 220.0f; //
                        float leftX = width * 0.25f;
                        float middleX = width * 0.5f;
                        float rightX = width * 0.75f;
                        if (wavecount == 0) {
                            crescent(activeProjectiles, middleX, spawnposition, 0.15f, launchInterval); // Crescent Middle
                        }
                        else if (wavecount == 1) {
                            crescent(activeProjectiles, rightX, spawnposition, 0.15f, launchInterval);  // Crescent Right
                        }
                        else if (wavecount == 2) {
                            crescent(activeProjectiles, leftX, spawnposition, 0.15f, launchInterval);   // Crescent Left
                        }
                        else if (wavecount == 3) {
                            crescent(activeProjectiles, leftX, spawnposition, 0.15f, launchInterval);   // Triple Crescent
                            crescent(activeProjectiles, middleX, spawnposition, 0.15f, launchInterval);
                            crescent(activeProjectiles, rightX, spawnposition, 0.15f, launchInterval);
                        }
                        else if (wavecount == 4) {

                            horizontal(activeProjectiles, battleBox, projectileSpeed-5, -45.0f);
                            horizontal(activeProjectiles, battleBox, projectileSpeed-5, 0.0f);
                        }
                        else if (wavecount == 5) {
                            horizontal(activeProjectiles, battleBox, projectileSpeed-5, 0.0f);
                            horizontal(activeProjectiles, battleBox, projectileSpeed-5, 30.0f);
                        }
                        else if (wavecount == 6) {
                            pinwheel(activeProjectiles, {middleX, battleBox.getPosition().y+50}, projectileSpeed);
                            pinwheel(activeProjectiles, {rightX - 50.f, battleBox.getPosition().y-50}, projectileSpeed);
                            pinwheel(activeProjectiles, {leftX + 50.f, battleBox.getPosition().y-50}, projectileSpeed);
                        }
                        else if (wavecount == 7) {
                            pinwheel(activeProjectiles, {rightX, battleBox.getPosition().y}, projectileSpeed);
                        }
                        else if (wavecount == 8) {
                            pinwheel(activeProjectiles, {leftX, battleBox.getPosition().y}, projectileSpeed);
                        }
                        else if (wavecount == 9) {
                            homingdarts(activeProjectiles, battleBox, middleX);
                        }
                        else if (wavecount == 10) {
                            homingdarts(activeProjectiles, battleBox, rightX);
                        }
                        else if (wavecount == 11) {
                            homingdarts(activeProjectiles, battleBox, leftX);
                        }

                        wavecount = (wavecount+1)%12;

                    }


                    if (magicTimer.getElapsedTime().asSeconds()>=3.0f-(difficultyIndex))
                    {
                        //Spawn on random sides of the map :
                        float randomX= static_cast<float>(rand()%400+220);
                        verticle(magicPillar,randomX,difficultyIndex) ;
                        magicTimer.restart();
                    }
                    if (burstTimer.getElapsedTime().asSeconds()>=burstDelay){
                        //burst loading
                        burst(crystalBurst,difficultyIndex,Bosssprite.getPosition(),launchInterval);
                        //creates random BUrst interval from 1 - 9 seconds
                        burstDelay = 1.f + rand()%900/100.f ;

                        burstTimer.restart();
                    }

                }

                //update the projectiles
                updateProjectiles(crystalBurst, dt, width , height , Playersprite.getPosition(),12.f- 5*(difficultyIndex-1)) ;
                updateProjectiles(magicPillar, dt, width , height , Playersprite.getPosition(), 12.f-5*(difficultyIndex-1));
                updateProjectiles(activeProjectiles, dt, width, height, Playersprite.getPosition(), projectileSpeed);

                // counts down the i-frames
                if (invulnerabilitytimer > 0.0f||invulnerabilitytimerMagicAttack>0.0f)
                {
                    invulnerabilitytimer -= dt;
                    invulnerabilitytimerMagicAttack -= dt ;
                }

                // check the collissins of they happen brrrr (Bazil)
                // only ONE collision check per frame, map collision only counts in phase 2
                bool hit = collision(activeProjectiles, Playersprite.getPosition()) ||
                           collision(crystalBurst, Playersprite.getPosition());
                if (currentphase == 2)
                {
                    hit = hit || mapcollission(mapImages[mapindex], mapsprite, Playersprite.getPosition());
                }
                //should this be removed ???
                if (hit && invulnerabilitytimer <= 0.0f) {
                    if (currentphase ==1) invulnerabilitytimer = 0.1f; // frames where you can't get hit again
                    if (currentphase ==2) invulnerabilitytimer= 2.0f; //for the 2nd phase
                    shootSound.setVolume(soundVolume); //Sets volume of current selected menu volume (Haras)
                    shootSound.play(); // Makes sound effect play when player gets hit (Haras)
                    healthBarUpdate(healthBar, 15 - 3* (difficultyIndex-1) );
                    shakeTimer = 0.3f; // This will actually engage the SI for 0.3 seconds (Haras)


                }
                hit = collision(magicPillar, Playersprite.getPosition()) ;

                if ( hit && invulnerabilitytimerMagicAttack <=0.0f) {
                    if (currentphase==1)
                        invulnerabilitytimerMagicAttack = 1.f ;
                    shootSound.setVolume(soundVolume); //Sets volume of current selected menu volume (Haras)
                    shootSound.play(); // Makes sound effect play when player gets hit (Haras)
                    healthBarUpdate(healthBar , 30 - 5*(difficultyIndex-1));
                    shakeTimer = 0.3f; // This will actually engage the SI for 0.3 seconds (Haras)


                }
                if (healthBar.getSize().x <= 0){
                    changeState = 3 ;
                    currentState = GameState::Death;
                }

                if (bossCollision(activeProjectiles, Bosssprite.getPosition())&&currentphase==1){
                    shootSound.setVolume(soundVolume);
                    shootSound.play();
                    shakeTimer = 0.3f;
                    healthBarUpdate(bossHealth, 15 - 5* difficultyIndex) ;

                }
                if (bossHealth.getSize().x<=0&&currentphase==1) // integrated the boss defeating with new stage
                {
                    currentphase++;
                    mapindex = 0;
                    activeProjectiles.clear();
                    magicPillar.clear();
                    crystalBurst.clear();
                    // the map is placed so that its BOTTOM edge sits on the bottom of the screen, because the track starts down there
                    playerspawnmap(); // the wide start of the path is in the bottom centre of the map
                }

                // Dialogue box positioning
                Vector2f bossPos = Bosssprite.getPosition();
                Vector2f bubbleOffset(65.0f, -35.0f);

                speechBubble.setPosition(bossPos + bubbleOffset);
                speech.setPosition(speechBubble.getPosition() + Vector2f(10.0f, 8.0f));

                //SI Code is below this (Haras)
                if (shakeTimer > 0.0f) {
                    shakeTimer -= dt;
                    // Below random number generation between positive, negative shakeIntensity (Haras)
                    float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * shakeIntesity;
                    float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 2.0f * shakeIntesity;
                    view.setCenter({ (width / 2.0f) + offsetX, (height / 2.0f) + offsetY });
                }
                else {
                    // Camera goes back to center when no shake (Haras)
                    view.setCenter({ width / 2.0f, height / 2.0f });
                }
            }
            window.setView(view); // This line actually applies teh offset to before drawing happens below (Haras)
            // Draw static UI and Box first cuz

            window.clear();
            //Work required
            if (currentphase == 2)
            {
                window.draw(mapsprite); // map only shows up in phase 2, it did not make for cool background in phase 1 tbh
            }

            window.draw(boundaryRect) ;
            window.draw(healthBar) ;
            if (currentphase==1){
                window.draw(battleBox);
                window.draw(Bosssprite);
                window.draw(speechBubble);
                window.draw(speech);
                window.draw(blazeRod);
                window.draw(cooldownRing);
                window.draw(cover) ;
                window.draw(healthBoundary) ;
                window.draw(bossHealth);
                window.draw(bossName) ;
            }
            window.draw(playerIcon) ;

            // draw the projectiles with the hazard trail
            for(const auto& pillar: magicPillar){
                MagicAttack.setPosition(pillar.position);
                window.draw(MagicAttack);
            }

            for (const auto& newCrystal :crystalBurst)
            {
                if(newCrystal.spawnDelay <= 0.0f){
                    fallingCrystal.setPosition(newCrystal.position);
                    fallingCrystal.setRotation(degrees(newCrystal.rotation)) ;
                    window.draw(fallingCrystal);
                }
            }

            for (const auto& proj : activeProjectiles) {
                for (const auto& hazard : proj.trail) {
                    hazardDot.setPosition(hazard.position);
                    window.draw(hazardDot);
                }
                if (proj.parried)
                {
                    projSprite.setColor(Color(255,255,0,200));
                }
                else
                    projSprite.setColor(Color::White) ;

                if (proj.spawnDelay <= 0.0f && (proj.isLaunched || proj.launchTimer > 0.0f)) {
                    projSprite.setPosition(proj.position);
                    window.draw(projSprite);
                }
            }

            // Draw Player on top
            window.draw(Playersprite);

            if (healthBar.getSize().x <= 135&& healthBar.getSize().x >60 )
            healthBar.setFillColor(sf::Color(243 , 156 , 18));

            if (healthBar.getSize().x<=60 && healthBar.getSize().x>0){
                float time = clock.getElapsedTime().asSeconds();
                float pulse = (sin(time*4.f)+1.2f)/2.f ;
                healthBar.setFillColor(Color(240,static_cast<int>(180*pulse), 20));
            }

            if(changeState==1){
                menu_Interaction(window, view, currentState, hoveredIndex,
                    mainMenuItems,deathMenuItems, startMenuItems, pauseMenuItems, startOptions,
                    difficultyIndex, difficulties, font, changeState, previousState, speed, projectileSpeed, waveInterval,
                    launchInterval, soundVolume, musicVolume, shakeIntesity, currentHeart , hearts , heartFiles, scaling , Playersprite , playerIcon);

                bgMusic.setVolume(musicVolume); //this allows live update from within settings menu

                rendering(window, starBackground, currentState, titletext,
                    mainMenuItems,deathMenuItems, startMenuItems, pauseMenuItems, font, difficulties[difficultyIndex], soundVolume, musicVolume, shakeIntesity,
                    DimScreen, Playersprite , currentHeart);


            }
            if (changeState == 3){
                Playersprite.setPosition(battleBox.getPosition()); //This will put player position back on center
                activeProjectiles.clear(); //Clears all previous projectiles
                wavecount = 0; //restart waves
                currentphase = 1; // back to phase 1
                mapindex = 0;
                invulnerabilitytimer = 0.0f; // no leftover i-frames
                placemapstart(); // map back to start (bottom of the map on the bottom of the screen)

                waveTriggerClock.restart(); //resets attack timings
                shakeTimer = 0.0f; // resets shakeTimer
                healthBar.setSize({300.f,20.f})  ;
                healthBar.setFillColor(Color(57,255,20));
                bossHealth.setSize({500.f,20.f}) ;
                magicTimer.restart() ;
                burstTimer.restart() ;
                magicPillar.clear() ;
                crystalBurst.clear();
                isParrying = false ;
                cooldown = 0.f ;

                burstDelay = 10.f ;

                parryTimer = 0.0f ;
                blazeRod.setColor(Color::White);
                blazeRod.setRotation(degrees(0.0f)) ;

                bgMusic.setVolume(musicVolume); //this allows live update from within settings menu

            }

            window.display();
            }
    }
    return 0;
}
