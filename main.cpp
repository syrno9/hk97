#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <memory>
#include <SFML/Audio.hpp>
#include <iostream>
#include "entities.hpp"
#include <fstream>
#include <algorithm>
#include <random>

using namespace std;
using namespace sf;

enum class GameState {
    Menu,
    Playing,
    GameOver
};

int loadHighScore() {
    ifstream file("highscore.txt");
    int highScore = 0;
    if (file.is_open()) {
        file >> highScore;
    }
    return highScore;
}

void saveHighScore(int score) {
    ofstream file("highscore.txt");
    if (file.is_open()) {
        file << score;
    }
}

Texture loadRandomBackground() {
    Texture texture;
    int randomIndex = rand() % 6 + 1;
    string filename = "assets/bg/bg" + to_string(randomIndex) + ".png"; 
    if (!texture.loadFromFile(filename)) {
        cerr << "Error loading background image: " << filename << endl;
    }
    return texture;
}

Color getGradientColor(float ratio, const Color& startColor, const Color& endColor) {
    return Color(
        static_cast<Uint8>(startColor.r + ratio * (endColor.r - startColor.r)),
        static_cast<Uint8>(startColor.g + ratio * (endColor.g - startColor.g)),
        static_cast<Uint8>(startColor.b + ratio * (endColor.b - startColor.b)),
        255 
    );
}

Text createGradientText(const string& textString, const Font& font, int characterSize, const Color& startColor, const Color& endColor) {
    Text text(textString, font, characterSize);
    text.setOutlineColor(Color::Black); 
    text.setOutlineThickness(2); 

    for (size_t i = 0; i < textString.size(); ++i) {
        Text gradientText(textString[i], font, characterSize);
        float ratio = static_cast<float>(i) / (textString.size() - 1);
        gradientText.setFillColor(getGradientColor(ratio, startColor, endColor));
        gradientText.setPosition(text.getPosition().x + i * text.getGlobalBounds().width / textString.size(), text.getPosition().y);
    }
    return text; 
}

void showSlideshow(RenderWindow& window, Font& font) {
    const int NUM_SLIDES = 6;
    Texture slideTextures[NUM_SLIDES];
    Sprite slideSprites[NUM_SLIDES];
    
    // Load slideshow images
    for (int i = 0; i < NUM_SLIDES; ++i) {
        if (!slideTextures[i].loadFromFile("assets/title/story" + to_string(i + 1) + ".png")) {
            cerr << "Error loading slide image: assets/title/story" << (i + 1) << ".png" << endl;
            return;
        }
        slideSprites[i].setTexture(slideTextures[i]);
        slideSprites[i].setPosition(0, 0);
        
        // Scale to fill the window while maintaining aspect ratio
        float scaleX = static_cast<float>(window.getSize().x) / slideTextures[i].getSize().x;
        float scaleY = static_cast<float>(window.getSize().y) / slideTextures[i].getSize().y;
        float scale = max(scaleX, scaleY); // Use the larger scale to fill the screen
        slideSprites[i].setScale(scale, scale);
    }

    float fadeAlpha = 0.0f;
    const float FADE_SPEED = 255.0f / 2.0f; // Adjust fade speed
    int currentSlide = 0;
    bool isFadingOut = false;

    Clock clock; // Add a clock to track time
    float slideDuration = 10.0f; // Duration for each slide
    float elapsedTime = 0.0f; // Time tracker for slides

    while (window.isOpen() && currentSlide < NUM_SLIDES) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                window.close();
            }
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Enter) {
                    isFadingOut = true; // Start fading out on Enter key press
                }
            }
        }

        // Update fade effect and slide duration
        elapsedTime += clock.restart().asSeconds(); // Get the elapsed time
        if (elapsedTime >= slideDuration) {
            isFadingOut = true; // Automatically start fading out after the duration
        }

        if (isFadingOut) {
            fadeAlpha += FADE_SPEED * (elapsedTime / slideDuration); // Use elapsed time for fade effect
            if (fadeAlpha >= 255.0f) {
                fadeAlpha = 255.0f;
                currentSlide++;
                isFadingOut = false; // Reset fading
                fadeAlpha = 0.0f; // Reset fade for next slide
                elapsedTime = 0.0f; // Reset elapsed time for the next slide
            }
        }

        // Draw current slide
        window.clear(Color::Black);
        if (currentSlide < NUM_SLIDES) {
            window.draw(slideSprites[currentSlide]);
        }

        // Draw fade overlay
        RectangleShape fadeOverlay(Vector2f(window.getSize().x, window.getSize().y));
        fadeOverlay.setFillColor(Color(0, 0, 0, fadeAlpha));
        window.draw(fadeOverlay);

        window.display();
    }
}

int main() {
    const float WINDOW_WIDTH = 800.0f;
    const float WINDOW_HEIGHT = 600.0f;
    const float GAME_SIZE = 600.0f;
    const float HUD_WIDTH = 200.0f;
    const float GAME_X = 0;  
    const float GAME_Y = 0;

    RenderWindow window(VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Hong Kong 97");
    window.setFramerateLimit(60);

    FloatRect gameArea(GAME_X, GAME_Y, GAME_SIZE, GAME_SIZE);
    
    Music music;
    if (!music.openFromFile("assets/song.mp3")) {
        cerr << "Error loading music file! Make sure 'song.mp3' exists in the current directory." << endl;
    } else {
        music.setLoop(true);
        music.setVolume(100.0f); 
        music.play();
        cout << "Music started playing..." << endl;
    }

    SoundBuffer shootSoundBuffer;
    if (!shootSoundBuffer.loadFromFile("assets/sfx/plst00.wav")) {
        cerr << "Error loading shoot sound file!" << endl;
    }
    Sound shootSound;
    shootSound.setBuffer(shootSoundBuffer);

    SoundBuffer deathSoundBuffer;
    if (!deathSoundBuffer.loadFromFile("assets/sfx/pldead00.wav")) {
        cerr << "Error loading death sound file!" << endl;
    }
    Sound deathSound;
    deathSound.setBuffer(deathSoundBuffer);

    SoundBuffer powerUpSoundBuffer;
    if (!powerUpSoundBuffer.loadFromFile("assets/sfx/item00.wav")) {
        cerr << "Error loading power-up sound file!" << endl;
    }
    Sound powerUpSound;
    powerUpSound.setBuffer(powerUpSoundBuffer);

    SoundBuffer enemyDeathSoundBuffer;
    if (!enemyDeathSoundBuffer.loadFromFile("assets/sfx/enep00.wav")) {
        cerr << "Error loading enemy death sound file!" << endl;
    }
    Sound enemyDeathSound;
    enemyDeathSound.setBuffer(enemyDeathSoundBuffer);

    SoundBuffer enemyShootSoundBuffer;
    if (!enemyShootSoundBuffer.loadFromFile("assets/sfx/tan02.wav")) {
        cerr << "Error loading enemy shoot sound file!" << endl;
    }
    Sound enemyShootSound;
    enemyShootSound.setBuffer(enemyShootSoundBuffer);

    SoundBuffer fullPowerSoundBuffer;
    if (!fullPowerSoundBuffer.loadFromFile("assets/sfx/powerup.wav")) {
        cerr << "Error loading full power sound file!" << endl;
    }
    Sound fullPowerSound;
    fullPowerSound.setBuffer(fullPowerSoundBuffer);

    SoundBuffer extendSoundBuffer;
    if (!extendSoundBuffer.loadFromFile("assets/sfx/extend.wav")) {
        cerr << "Error loading extend sound file!" << endl;
    }
    Sound extendSound;
    extendSound.setBuffer(extendSoundBuffer);

    Player player(Vector2f(GAME_X + GAME_SIZE/2, 550));

    vector<unique_ptr<Bullet>> bullets;
    vector<unique_ptr<Enemy>> enemies;
    vector<unique_ptr<PowerUp>> powerUps;
    vector<unique_ptr<EnemyBullet>> enemyBullets;
    vector<unique_ptr<Car>> cars;
    
    Clock clock;
    float enemySpawnTimer = 0;
    float enemySpawnInterval = 0.5f;
    float carSpawnTimer = 0;
    float carSpawnInterval = 2.0f;  

    Font font;
    if (!font.loadFromFile("assets/DFPPOPCorn-W12.ttf")) { 
        cerr << "Error loading font!" << endl;
        return -1;
    }

    Text scoreText = createGradientText("Score: 0", font, 20, Color::Red, Color::Yellow);
    Text livesText = createGradientText("Lives: 3", font, 20, Color::Green, Color::Blue);
    Text powerText = createGradientText("Power: 1", font, 20, Color::White, Color::Black);
    Text highScoreText = createGradientText("HS: 0", font, 20, Color::White, Color::Blue);

    GameState gameState = GameState::Menu;
    int highScore = loadHighScore();

    Text titleText;
    titleText.setFont(font);
    titleText.setString("Hong Kong 97");
    titleText.setCharacterSize(50);
    titleText.setPosition(400 - titleText.getGlobalBounds().width/2, 200);
    titleText.setFillColor(Color::White);

    Text menuText;
    menuText.setFont(font);
    menuText.setString("Press ENTER to start");
    menuText.setCharacterSize(30);
    menuText.setPosition(400 - menuText.getGlobalBounds().width/2, 300);
    menuText.setFillColor(Color::White);

    Texture hudTexture;
    if (!hudTexture.loadFromFile("assets/bg/ba.gif")) {
        cerr << "Error loading HUD background texture!" << endl;
        return -1;
    }
    hudTexture.setRepeated(true); 
    RectangleShape hudBackground;
    hudBackground.setSize(Vector2f(HUD_WIDTH, WINDOW_HEIGHT));
    hudBackground.setPosition(GAME_SIZE, 0); 
    hudBackground.setTexture(&hudTexture); 
    hudBackground.setTextureRect(IntRect(0, 0, HUD_WIDTH, WINDOW_HEIGHT)); 

    const Color hudTextColor(180, 180, 180);  
    highScoreText.setFillColor(hudTextColor);
    scoreText.setFillColor(hudTextColor);
    livesText.setFillColor(hudTextColor);
    powerText.setFillColor(hudTextColor);

    Text bombText;
    bombText.setFont(font);
    bombText.setString("Bombs: 3"); 
    bombText.setCharacterSize(20);
    bombText.setFillColor(hudTextColor); 
    bombText.setOutlineColor(Color::Black); 
    bombText.setOutlineThickness(2); 

    const float HUD_X = GAME_SIZE + 20;  
    const float TEXT_PADDING = 40;

    highScoreText.setPosition(HUD_X, 10);
    scoreText.setPosition(HUD_X, 10 + TEXT_PADDING);
    livesText.setPosition(HUD_X, 10 + TEXT_PADDING * 3);
    powerText.setPosition(HUD_X, 10 + TEXT_PADDING * 4);
    bombText.setPosition(HUD_X, 10 + TEXT_PADDING * 5); 

    Text fullPowerText;
    fullPowerText.setFont(font);
    fullPowerText.setString("FULL POWER!");
    fullPowerText.setCharacterSize(60);
    fullPowerText.setFillColor(Color::Yellow);
    fullPowerText.setPosition(GAME_SIZE, GAME_SIZE / 2);  
    bool isShowingFullPower = false;
    float fullPowerTimer = 0.0f;
    const float FULL_POWER_DURATION = 1.0f;  

    Entity::loadDeathTextures();

    Texture backgroundTexture = loadRandomBackground();
    Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setScale(
        (WINDOW_WIDTH / backgroundTexture.getSize().x) * 0.75f, 
        WINDOW_HEIGHT / backgroundTexture.getSize().y
    );
    backgroundSprite.setPosition(0, 0); 

    Text lifeUpText;
    lifeUpText.setFont(font);
    lifeUpText.setString("Life Up!");
    lifeUpText.setCharacterSize(40);
    lifeUpText.setFillColor(Color::Green);
    lifeUpText.setPosition(WINDOW_WIDTH, GAME_SIZE / 2); 
    bool isShowingLifeUp = false;
    float lifeUpTimer = 0.0f;
    const float LIFE_UP_DURATION = 1.0f; 

    Texture gameOverTexture;
    Sprite gameOverSprite;
    bool isGameOver = false;
    float fadeAlpha = 0.0f; 
    const float FADE_SPEED = 255.0f / 2.0f; 
    if (!gameOverTexture.loadFromFile("assets/gameover.png")) {
        cerr << "Error loading game over texture!" << endl;
        return -1;
    }
    gameOverSprite.setTexture(gameOverTexture);
    gameOverSprite.setPosition(0, 0); 

    Texture logoTexture;
    if (!logoTexture.loadFromFile("assets/logo.png")) {
        cerr << "Error loading logo texture!" << endl;
        return -1; 
    }
    Sprite logoSprite(logoTexture);
    logoSprite.setPosition(HUD_X, 500);
    logoSprite.setScale(0.5f, 0.5f);

    Image icon;
    if (!icon.loadFromFile("icon.png")) { 
        cerr << "Error loading icon image!" << endl;
        return -1; 
    }
    
    window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    // title screen
    Texture titleTexture;
    if (!titleTexture.loadFromFile("assets/title/title.png")) {
        cerr << "Error loading title image!" << endl;
        return -1; 
    }
    Sprite titleSprite(titleTexture); 
    titleSprite.setScale(
        WINDOW_WIDTH / titleTexture.getSize().x,
        WINDOW_HEIGHT / titleTexture.getSize().y
    );
    titleSprite.setPosition(0, 0);

    // put borders in game
    Texture borderTexture;
    if (!borderTexture.loadFromFile("assets/bg/ba.gif")) {
        cerr << "Error loading border texture!" << endl;
        return -1; 
    }
    RectangleShape topBorder(Vector2f(GAME_SIZE, 20)); //top
    topBorder.setPosition(GAME_X, GAME_Y); 

    RectangleShape bottomBorder(Vector2f(GAME_SIZE, 20)); // bottom
    bottomBorder.setTexture(&borderTexture);
    bottomBorder.setPosition(GAME_X, GAME_Y + GAME_SIZE - 20);

    RectangleShape leftBorder(Vector2f(20, GAME_SIZE)); // left
    leftBorder.setTexture(&borderTexture);
    leftBorder.setPosition(GAME_X, GAME_Y); 

    // Show the slideshow before the title screen
    showSlideshow(window, font);

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        enemySpawnTimer += deltaTime;
        carSpawnTimer += deltaTime;
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            
            if (event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Enter && gameState == GameState::Menu) {
                    gameState = GameState::Playing;
                    player.reset(Vector2f(GAME_X + GAME_SIZE/2, 550));
                    bullets.clear();
                    enemies.clear();
                    enemySpawnTimer = 0;
                }
                if (event.key.code == Keyboard::X && gameState == GameState::Playing) {
                    if (player.useBomb()) {
                        enemies.clear();
                    }
                }
            }
        }

        if (gameState == GameState::Menu) {
            window.clear(Color::Black);
            window.draw(titleSprite); 
            window.draw(menuText); 
            window.display();
            continue;
        }

        if (enemySpawnTimer >= enemySpawnInterval) {
            enemySpawnTimer = 0;
            float randomX = rand() % 700 + 50;
            enemies.push_back(make_unique<Enemy>(Vector2f(randomX, -50)));
        }

        if (carSpawnTimer >= carSpawnInterval) {
            carSpawnTimer = 0;
            float randomY = rand() % 500 + 50;  
            cars.push_back(make_unique<Car>(randomY));
        }

        if (Keyboard::isKeyPressed(Keyboard::Z) && player.canShoot()) {
            shootSound.play();
            
            if (player.getIsFocused()) {
                auto bulletPositions = player.getFocusedBulletPositions();
                for (const auto& pos : bulletPositions) {
                    bullets.push_back(make_unique<Bullet>(
                        pos,
                        Vector2f(0, -800)  
                    ));
                }
            } else {
                int spreadCount = player.getSpreadCount();
                float spreadAngle = 15.0f;
                
                for (int i = 0; i < spreadCount; i++) {
                    float angle = -spreadAngle * (spreadCount - 1) / 2.0f + spreadAngle * i;
                    float radians = angle * 3.14159f / 180.0f;
                    
                    Vector2f bulletVelocity(
                        -sin(radians) * 800,
                        -cos(radians) * 800
                    );
                    
                    bullets.push_back(make_unique<Bullet>(
                        player.getPosition(),
                        bulletVelocity
                    ));
                }
            }
        }

        player.update(deltaTime);

        for (auto& bullet : bullets) {
            bullet->update(deltaTime);
        }

        for (auto& enemy : enemies) {
            enemy->update(deltaTime);
        }

        for (auto& powerUp : powerUps) {
            if (powerUp->isActive()) {
                powerUp->update(deltaTime, &player); 
            }
        }

        for (auto& car : cars) {
            car->update(deltaTime);
        }

        bullets.erase(
            remove_if(bullets.begin(), bullets.end(),
                [](const auto& bullet) { return !bullet->isActive(); }),
            bullets.end()
        );

        enemies.erase(
            remove_if(enemies.begin(), enemies.end(),
                [](const auto& enemy) { return !enemy->isActive(); }),
            enemies.end()
        );

        for (auto& enemy : enemies) {
            if (!player.isInvincible() && player.getBounds().intersects(enemy->getBounds())) {
                deathSound.play();
                player.hit();
                if (player.getLives() <= 0) {
                    if (!isGameOver) {
                        isGameOver = true; 
                    }
                }
            }
            
            for (auto& bullet : bullets) {
                if (bullet->isActive() && enemy->isActive() && 
                    bullet->getBounds().intersects(enemy->getBounds())) {
                    enemyDeathSound.play();
                    bullet->setActive(false);
                    enemy->hit();  
                    player.addScore(100);
                    
                    Vector2f spawnPos = enemy->getPosition();
                    
                    PowerUp::Type type = (rand() % 100 < 80) ? 
                        PowerUp::Type::Small : PowerUp::Type::Large;
                    powerUps.push_back(make_unique<PowerUp>(spawnPos, type));
                }
            }
        }

        for (auto& powerUp : powerUps) {
            if (powerUp->isActive() && player.getBounds().intersects(powerUp->getBounds())) {
                powerUpSound.play();
                powerUp->setActive(false);
                int oldPower = player.getPowerLevel();
                player.increasePower(powerUp->getType() == PowerUp::Type::Small ? 1 : 3);
                
                powerUp->handleFullPower(&player); 

                player.addScore(50); 

                if (player.getScore() % 1500 == 0) {
                    player.addLife(); 
                    extendSound.play(); 
                    isShowingLifeUp = true; 
                    lifeUpTimer = 0.0f; 
                }

                if (oldPower < Player::MAX_POWER && player.getPowerLevel() >= Player::MAX_POWER) {
                    isShowingFullPower = true;
                    fullPowerTimer = 0.0f;
                    fullPowerSound.play();  
                }
            }
        }

        powerUps.erase(
            remove_if(powerUps.begin(), powerUps.end(),
                [](const auto& powerUp) { return !powerUp->isActive(); }),
            powerUps.end()
        );

        for (auto& enemy : enemies) {
            if (enemy->canShoot() && !enemy->hasShot) {
                enemyShootSound.play();
                enemy->hasShot = true;  
                
                for (int i = 0; i < Enemy::BURST_SIZE; i++) {

                    float angle = -30.0f + (60.0f * i / (Enemy::BURST_SIZE - 1));
                    float radians = angle * 3.14159f / 180.0f;
                    
                    Vector2f direction(
                        sin(radians),  
                        cos(radians) 
                    );
                    
                    enemyBullets.push_back(make_unique<EnemyBullet>(
                        enemy->getPosition(),
                        direction * 150.0f  
                    ));
                }
            }
        }

        for (auto& bullet : enemyBullets) {
            bullet->update(deltaTime);
        }

        enemyBullets.erase(
            remove_if(enemyBullets.begin(), enemyBullets.end(),
                [](const auto& bullet) { return !bullet->isActive(); }),
            enemyBullets.end()
        );

        for (const auto& bullet : enemyBullets) {
            if (!player.isInvincible() && player.getBounds().intersects(bullet->getBounds())) {
                deathSound.play();
                player.hit();
                bullet->setActive(false);
                player.setPosition(Vector2f(GAME_X + GAME_SIZE/2, 550)); 
                if (player.getLives() <= 0) {
                    if (!isGameOver) {
                        isGameOver = true;
                        fadeAlpha = 0.0f;
                    }
                }
            }
        }

        for (const auto& car : cars) {
            if (!player.isInvincible() && player.getBounds().intersects(car->getBounds())) {
                deathSound.play();
                player.hit();
                player.setPosition(Vector2f(GAME_X + GAME_SIZE/2, 550)); 
                if (player.getLives() <= 0) {
                    if (!isGameOver) {
                        isGameOver = true; 
                        fadeAlpha = 0.0f; 
                    }
                }
            }
        }

        cars.erase(
            remove_if(cars.begin(), cars.end(),
                [](const auto& car) { return !car->isActive(); }),
            cars.end()
        );

        scoreText.setString("Score: " + to_string(player.getScore()));
        livesText.setString("Chin: " + to_string(player.getLives()));
        powerText.setString(player.getPowerLevel() >= Player::MAX_POWER ? 
            "FULL POWER" : 
            "Power: " + to_string(player.getPowerLevel()));
        bombText.setString("Bombs: " + to_string(player.getBombs())); 

        window.clear(Color::Black);

        window.draw(topBorder);
        window.draw(bottomBorder);
        window.draw(leftBorder);

        window.draw(backgroundSprite);

        View gameView(FloatRect(0, 0, GAME_SIZE, GAME_SIZE));
        gameView.setViewport(FloatRect(
            0,
            0,
            0.75f,
            1.0f
        ));
        window.setView(gameView);
        
        player.draw(window);
        if (player.isInDeathAnimation()) {
            player.updateDeathAnimation(deltaTime);
            player.drawDeathAnimation(window);
        }
        for (auto& bullet : bullets) {
            bullet->draw(window);
        }
        for (auto& enemy : enemies) {
            if (enemy->isActive() || enemy->isInDeathAnimation()) {
                enemy->draw(window);
                if (enemy->isInDeathAnimation()) {
                    enemy->updateDeathAnimation(deltaTime);
                    enemy->drawDeathAnimation(window);
                }
            }
        }
        for (auto& powerUp : powerUps) {
            powerUp->draw(window);
        }
        for (auto& bullet : enemyBullets) {
            bullet->draw(window);
        }
        for (auto& car : cars) {
            car->draw(window);
        }

        window.setView(window.getDefaultView());
        
        hudBackground.setSize(Vector2f(WINDOW_WIDTH * 0.25f, WINDOW_HEIGHT));
        hudBackground.setPosition(WINDOW_WIDTH * 0.75f, 0);
        window.draw(hudBackground);

        const float HUD_X = WINDOW_WIDTH * 0.75f + 20; 
        highScoreText.setPosition(HUD_X, 10);
        scoreText.setPosition(HUD_X, 50);
        livesText.setPosition(HUD_X, 90);
        powerText.setPosition(HUD_X, 130);
        bombText.setPosition(HUD_X, 170);

        if (gameState != GameState::Menu) {
            window.draw(highScoreText);
            window.draw(scoreText);
            window.draw(livesText);
            window.draw(powerText);
            window.draw(bombText); 
            window.draw(logoSprite); 
        }

        if (isShowingFullPower) {
            fullPowerTimer += deltaTime;
            
            float xPos = GAME_SIZE - (fullPowerTimer / FULL_POWER_DURATION) * (GAME_SIZE * 2);
            fullPowerText.setPosition(xPos, GAME_SIZE / 2);
            
            if (fullPowerTimer >= FULL_POWER_DURATION) {
                isShowingFullPower = false;
            }
        }

        if (isShowingLifeUp) {
            lifeUpTimer += deltaTime;
            
            float xPos = WINDOW_WIDTH - (lifeUpTimer / LIFE_UP_DURATION) * (WINDOW_WIDTH + lifeUpText.getGlobalBounds().width);
            lifeUpText.setPosition(xPos, GAME_SIZE / 2);
            
            if (lifeUpTimer >= LIFE_UP_DURATION) {
                isShowingLifeUp = false;
            }
        }

        if (isShowingFullPower) {
            View currentView = window.getView();
            window.setView(gameView);
            window.draw(fullPowerText);
            window.setView(currentView);
        }

        if (isShowingLifeUp) {
            window.draw(lifeUpText);
        }

        if (isGameOver) {
            fadeAlpha += FADE_SPEED * deltaTime; 
            if (fadeAlpha >= 255.0f) {
                fadeAlpha = 255.0f;
                gameState = GameState::Menu;
                player.reset(Vector2f(GAME_X + GAME_SIZE / 2, 550)); 
                bullets.clear();
                enemies.clear();
                powerUps.clear();
                cars.clear();
                enemyBullets.clear();
                highScoreText.setString("HS: " + to_string(highScore));
                isGameOver = false; 
                
                backgroundTexture = loadRandomBackground(); 
                backgroundSprite.setTexture(backgroundTexture); 
            }
        }
        if (isGameOver) {
            RectangleShape fadeOverlay(Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            fadeOverlay.setFillColor(Color(0, 0, 0, fadeAlpha)); 
            window.draw(fadeOverlay); 

            gameOverSprite.setScale(
                WINDOW_WIDTH / gameOverTexture.getSize().x,
                WINDOW_HEIGHT / gameOverTexture.getSize().y
            );
            window.draw(gameOverSprite); 
        }
        window.display();
    }
    return 0;
}
