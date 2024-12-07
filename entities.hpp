#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <cmath>
#include <vector>

using namespace std;
using namespace sf;

class Player;

class Entity {
protected:
    Vector2f position;
    Vector2f velocity;
    unique_ptr<Shape> shape;
    bool active = true;
    bool isDying = false;
    float deathAnimTimer = 0.0f;
    static vector<Texture> deathTextures;
    Sprite deathSprite;
    static bool deathTexturesLoaded;
    int currentDeathFrame = 0;
    static constexpr float DEATH_FRAME_TIME = 0.05f; 
    static constexpr float FLASH_FRAME_TIME = 0.03f; 
    virtual void hit() = 0;

public:
    Entity(const Vector2f& pos, const Vector2f& vel);
    virtual ~Entity() = default;
    
    virtual void update(float deltaTime);
    virtual void draw(RenderWindow& window);

    bool isActive() const;
    void setActive(bool state);
    Vector2f getPosition() const;
    virtual FloatRect getBounds() const { return shape ? shape->getGlobalBounds() : FloatRect(); }
    virtual void startDeathAnimation();
    bool isInDeathAnimation() const { return isDying; }
    void updateDeathAnimation(float deltaTime);
    virtual void drawDeathAnimation(RenderWindow& window);
    static void loadDeathTextures();

    virtual void update(float deltaTime, Player* player) = 0;  
};

class Player : public Entity {
private:
    static const float SIDE_VIEW_WIDTH_SCALE;
    float shootCooldown = 0.1f;
    float currentCooldown = 0.0f;
    float speed = 400.0f;
    int lives = 3;
    int score = 0;
    float invincibilityTime = 2.0f; 
    float currentInvincibilityTime = 0.0f;
    int bombs = 3;
    int powerLevel = 0;
    bool isAtFullPower;
    
    bool isShowingFullPower = false;
    float fullPowerTimer = 0.0f;      

    vector<Texture> animFrames;
    float animationTimer = 0.0f;
    const float FRAME_TIME = 0.1f;
    size_t currentFrame = 0;

    Sprite playerSprite;
    Texture upTexture;
    Texture downTexture;
    vector<Texture> leftAnimFrames;
    vector<Texture> rightAnimFrames;
    bool wasMovingUp = true;  

    float normalSpeed = 400.0f;
    float focusedSpeed = 200.0f;
    bool isFocused = false;

    SoundBuffer fullPowerSoundBuffer; 
    Sound fullPowerSound;

public:
    static const int MAX_POWER;  

    explicit Player(const Vector2f& pos);
    void update(float deltaTime) override;
    void draw(RenderWindow& window) override; 
    bool canShoot();
    bool isInvincible() const { return currentInvincibilityTime > 0; }
    void hit() override;  
    int getLives() const { return lives; }
    int getScore() const { return score; }
    void addScore(int points) { score += points; }
    void reset(const Vector2f& position);
    int getBombs() const { return bombs; }
    bool useBomb();
    void increasePower(int amount);
    int getPowerLevel() const { return powerLevel; }
    int getSpreadCount() const { 
        if (isFocused) {
            return 1 + (powerLevel / 2); 
        }
        return 1 + (powerLevel / 2);     
    }
    float getShootCooldown() const { return max(0.05f, 0.1f - (powerLevel * 0.005f)); }  
    FloatRect getBounds() const override { return playerSprite.getGlobalBounds(); }
    void setPosition(const Vector2f& pos) {
        position = pos;
        playerSprite.setPosition(position);
    }

    vector<Vector2f> getFocusedBulletPositions() const {
        vector<Vector2f> positions;
        int rows = 1 + (powerLevel / 2); 
        float verticalSpacing = 10.0f; 
        
        for (int i = 0; i < rows; i++) {
            float xOffset = 0;
            float yOffset = (i - (rows-1)/2.0f) * verticalSpacing;
            positions.push_back(position + Vector2f(xOffset, yOffset));
        }
        return positions;
    }
    bool getIsFocused() const { return isFocused; }
    void addLife() {
        lives++;
    }
    void update(float deltaTime, Player* player) override {
        // Implement the update logic for Player
    }
};

class Bullet : public Entity {
private:
    Sprite bulletSprite;
    static Texture bulletTexture;  
public:
    Bullet(const Vector2f& pos, const Vector2f& vel);
    void draw(RenderWindow& window) override;
    void update(float deltaTime) override;
    FloatRect getBounds() const override { return bulletSprite.getGlobalBounds(); }
    void hit() override;
    void update(float deltaTime, Player* player) override {
        // Implement the update logic for Bullet
    }
};

class EnemyBullet : public Entity {
private:
    Sprite bulletSprite;
    static Texture bulletTexture;
    static bool textureLoaded;

public:
    EnemyBullet(const Vector2f& pos, const Vector2f& vel);
    void update(float deltaTime) override;
    void draw(RenderWindow& window) override;
    FloatRect getBounds() const override { return bulletSprite.getGlobalBounds(); }
    void hit() override;
    void update(float deltaTime, Player* player) override {
        // Implement the update logic for EnemyBullet
    }
};

class Enemy : public Entity {
public:
    enum class Pattern {
        Straight,
        Wave,
        Zigzag,
        Shooter    
    };

    explicit Enemy(const Vector2f& pos);
    void update(float deltaTime) override;
    void draw(RenderWindow& window) override;
    FloatRect getBounds() const override { return enemySprite.getGlobalBounds(); }
    bool canShoot() const { return movePattern == Pattern::Shooter && shootTimer <= 0; }
    Vector2f getPosition() const { return position; }
    bool hasShot = false;  
    static const int BURST_SIZE = 5; 
    void hit();  

    void updateDeathAnimation(float deltaTime); 
    void drawDeathAnimation(RenderWindow& window); 
    void update(float deltaTime, Player* player) override {
        // Implement the update logic for Enemy
    }
private:
    static vector<Texture> enemyTextures;
    static bool texturesLoaded;
    Sprite enemySprite;
    Pattern movePattern;
    float waveAmplitude = 100.0f;
    float waveFrequency = 2.0f;
    float initialX;
    float totalTime = 0.0f;
    float shootTimer = 0.0f;
    const float shootInterval = 5.5f;  
    const float shootChance = 0.3f;  
};

class PowerUp : public Entity {
public:
    enum class Type {
        Small,
        Large
    };

private:
    static vector<Texture> powerUpTextures;
    static bool texturesLoaded;
    Sprite powerSprite;
    Type type; 
    int currentFrame;
    float animationTimer;
    static constexpr float FRAME_TIME = 0.1f;
    bool shouldGravitate = false;

public:
    PowerUp(const Vector2f& pos, Type powerType);
    void update(float deltaTime, Player* player) override;
    void draw(RenderWindow& window) override;
    FloatRect getBounds() const override;
    Type getType() const { return type; }
    void hit() override;
    void setPosition(const Vector2f& newPosition);
    void setGravitate(bool gravitate) {
        shouldGravitate = gravitate;
    }

    void handleFullPower(Player* player) {
        if (player->getPowerLevel() >= Player::MAX_POWER) {
            type = Type::Large; 
        }
    }
};

class Car : public Entity {
private:
    static Texture carTexture;
    static bool textureLoaded;
    Sprite carSprite;

public:
    Car(float yPosition);
    void update(float deltaTime) override;
    void draw(RenderWindow& window) override;
    FloatRect getBounds() const override;
    void hit() override;
    void update(float deltaTime, Player* player) override {
        // Implement the update logic for Car
    }
}; 