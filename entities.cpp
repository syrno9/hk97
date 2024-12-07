#include "entities.hpp"
#include <iostream>

using namespace std;
using namespace sf;

const int Player::MAX_POWER = 10; 
const float Player::SIDE_VIEW_WIDTH_SCALE = 1.5f;  
Texture Bullet::bulletTexture;
Texture EnemyBullet::bulletTexture;
bool EnemyBullet::textureLoaded = false;
Texture Car::carTexture;
bool Car::textureLoaded = false;
vector<Texture> PowerUp::powerUpTextures;
bool PowerUp::texturesLoaded = false;
vector<Texture> Entity::deathTextures;
bool Entity::deathTexturesLoaded = false;
Entity::Entity(const Vector2f& pos, const Vector2f& vel) 
    : position(pos), velocity(vel) {}

void Entity::update(float deltaTime) {
    position += velocity * deltaTime;
    if (shape) shape->setPosition(position);
}

void Entity::draw(RenderWindow& window) {
    if (shape) window.draw(*shape);
}

bool Entity::isActive() const { return active; }
void Entity::setActive(bool state) { active = state; }
Vector2f Entity::getPosition() const { return position; }

void Entity::loadDeathTextures() {
    if (!deathTexturesLoaded) {
        deathTextures.resize(7);
        for (int i = 0; i < 7; i++) {
            if (!deathTextures[i].loadFromFile("assets/die/ex" + to_string(i + 1) + ".png")) {
                cerr << "Failed to load ex" << (i + 1) << ".png" << endl;
            }
        }
        deathTexturesLoaded = true;
    }
}

void Entity::startDeathAnimation() {
    isDying = true;
    deathAnimTimer = 0.0f;
    currentDeathFrame = 0;
    deathSprite.setTexture(deathTextures[0]);
    deathSprite.setPosition(position);
    FloatRect bounds = deathSprite.getLocalBounds();
    deathSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
}

void Entity::updateDeathAnimation(float deltaTime) {
    if (!isDying) return;

    deathAnimTimer += deltaTime;
    const float flashingDuration = 1.0f; 
    const float frameDuration = 0.1f;    

    if (currentDeathFrame < 7) {
        if (deathAnimTimer >= DEATH_FRAME_TIME) {
            deathAnimTimer = 0;
            currentDeathFrame++;
            if (currentDeathFrame < 7) {
                deathSprite.setTexture(deathTextures[currentDeathFrame]);
            }
        }
    }
}

void Entity::drawDeathAnimation(RenderWindow& window) {
    if (isDying) {
        window.draw(deathSprite); 
    }
}
Player::Player(const Vector2f& pos) 
    : Entity(pos, Vector2f(0, 0)), isAtFullPower(false) {
    if (!upTexture.loadFromFile("assets/chin/up.png") ||
        !downTexture.loadFromFile("assets/chin/down.png")) {
        throw runtime_error("Failed to load player textures");
    }

    for (int i = 1; i <= 4; i++) { 
        Texture texture;
        if (!texture.loadFromFile("assets/chin/right" + to_string(i) + ".png")) {
            throw runtime_error("Failed to load right animation frame " + to_string(i));
        }
        rightAnimFrames.push_back(texture);
        Texture flippedTexture = texture;
        leftAnimFrames.push_back(flippedTexture);
    }
    playerSprite.setTexture(upTexture);
    playerSprite.setOrigin(playerSprite.getGlobalBounds().width / 2,
                          playerSprite.getGlobalBounds().height / 2);
    playerSprite.setPosition(position);
}
vector<PowerUp*> powerUps;  
void Player::update(float deltaTime) {
    if (currentCooldown > 0) {
        currentCooldown -= deltaTime;
    }
    if (currentInvincibilityTime > 0) {
        currentInvincibilityTime -= deltaTime;
    }
    isFocused = Keyboard::isKeyPressed(Keyboard::LShift) || 
                Keyboard::isKeyPressed(Keyboard::RShift);
    speed = isFocused ? focusedSpeed : normalSpeed;

    velocity = Vector2f(0, 0);
    bool isMoving = false;
    animationTimer += deltaTime;
    if (animationTimer >= FRAME_TIME) {
        animationTimer = 0;
        currentFrame = (currentFrame + 1) % leftAnimFrames.size();
    }
    
    if (Keyboard::isKeyPressed(Keyboard::Left)) {
        velocity.x = -speed;
        playerSprite.setTexture(rightAnimFrames[currentFrame]);
        playerSprite.setScale(-SIDE_VIEW_WIDTH_SCALE, 1.0f);  
        isMoving = true;
    }
    if (Keyboard::isKeyPressed(Keyboard::Right)) {
        velocity.x = speed;
        playerSprite.setTexture(rightAnimFrames[currentFrame]);
        playerSprite.setScale(SIDE_VIEW_WIDTH_SCALE, 1.0f); 
        isMoving = true;
    }
    if (Keyboard::isKeyPressed(Keyboard::Up)) {
        velocity.y = -speed;
        playerSprite.setTexture(upTexture);
        playerSprite.setScale(SIDE_VIEW_WIDTH_SCALE, 1.0f);  
        wasMovingUp = true;
        isMoving = true;
    }
    if (Keyboard::isKeyPressed(Keyboard::Down)) {
        velocity.y = speed;
        playerSprite.setTexture(downTexture);
        playerSprite.setScale(SIDE_VIEW_WIDTH_SCALE, 1.0f); 
        wasMovingUp = false;
        isMoving = true;
    }
    if (!isMoving) {
        playerSprite.setTexture(wasMovingUp ? upTexture : downTexture);
    }
    position += velocity * deltaTime;
    position.x = max(0.0f, min(position.x, 600.0f));
    position.y = max(0.0f, min(position.y, 600.0f));
    playerSprite.setPosition(position);
    for (auto& powerUp : powerUps) {
        if (isAtFullPower) {
            powerUp->setGravitate(true);
            Vector2f direction = this->getPosition() - powerUp->getPosition();
            float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
            if (distance > 0) {
                direction /= distance;
                powerUp->setPosition(powerUp->getPosition() + direction * 200.0f * deltaTime);
            }
        } else {
            powerUp->setGravitate(false);
        }
        powerUp->update(deltaTime, this);
    }
    int oldPower = powerLevel;
    if (oldPower < Player::MAX_POWER && this->getPowerLevel() >= Player::MAX_POWER) { 
        isAtFullPower = true;
        isShowingFullPower = true;
        fullPowerTimer = 0.0f;
        fullPowerSound.play(); 
    }
}

void Player::draw(RenderWindow& window) {
    if (active) {  
        window.draw(playerSprite);
    } else if (isDying) {  
        drawDeathAnimation(window);
    }
}
bool Player::canShoot() {
    if (currentCooldown <= 0) {
        currentCooldown = shootCooldown;
        return true;
    }
    return false;
}

void Player::hit() {
    if (!isInvincible()) {
        lives--;
        currentInvincibilityTime = invincibilityTime;
        startDeathAnimation();
        position = Vector2f(300.0f, 600.0f);  
        playerSprite.setPosition(position);  
        powerLevel = 0;  
        isAtFullPower = false;
    }
}

bool Player::useBomb() {
    if (bombs > 0) {
        bombs--;
        return true;
    }
    return false;
}

void Player::reset(const Vector2f& position) {
    this->position = position;
    this->velocity = Vector2f(0.0f, 0.0f);
    this->lives = 3;
    this->score = 0;
    this->bombs = 3;
    this->currentCooldown = 0.0f;
    this->currentInvincibilityTime = 0.0f;
    this->active = true;
    this->powerLevel = 0;
    this->shootCooldown = getShootCooldown();

    if (!fullPowerSoundBuffer.loadFromFile("assets/sfx/powerup.wav")) { 
        throw runtime_error("Failed to load powerup.wav");
    }
    fullPowerSound.setBuffer(fullPowerSoundBuffer); 
}
void Player::increasePower(int amount) {
    powerLevel = min(MAX_POWER, powerLevel + amount);
    shootCooldown = getShootCooldown();
}
Bullet::Bullet(const Vector2f& pos, const Vector2f& vel)
    : Entity(pos, vel) {
    static bool textureLoaded = false;
    if (!textureLoaded) {
        if (!bulletTexture.loadFromFile("assets/bullet.png")) {
            throw runtime_error("Failed to load bullet.png");
        }
        textureLoaded = true;
    }
    
    bulletSprite.setTexture(bulletTexture);
    bulletSprite.setOrigin(bulletSprite.getGlobalBounds().width / 2,
                          bulletSprite.getGlobalBounds().height / 2);
    bulletSprite.setPosition(position);
}

void Bullet::update(float deltaTime) {
    Entity::update(deltaTime);
    if (position.x < -10 || position.x > 810 || 
        position.y < -10 || position.y > 610) {
        active = false;
    }
}
void Bullet::draw(RenderWindow& window) {
    bulletSprite.setPosition(position);
    window.draw(bulletSprite);
}
void Bullet::hit() {
    active = false;
}
vector<Texture> Enemy::enemyTextures;
bool Enemy::texturesLoaded = false;

float animationTimer = 0.0f; 
int currentFrame = 0;
const float FRAME_TIME = 0.1f;

Enemy::Enemy(const Vector2f& pos)
    : Entity(pos, Vector2f(0, 300)), initialX(pos.x) {
    animationTimer = 0.0f; 
    currentFrame = 0;     
    if (!texturesLoaded) {
        enemyTextures.resize(3);
        for (int i = 1; i <= 3; i++) {
            if (!enemyTextures[i-1].loadFromFile("assets/enemies/e" + to_string(i) + ".png")) {
                throw runtime_error("Failed to load enemy texture " + to_string(i));
            }
        }
        texturesLoaded = true;
    }
    int spriteIndex = rand() % 3;
    enemySprite.setTexture(enemyTextures[spriteIndex]);
    enemySprite.setOrigin(enemySprite.getGlobalBounds().width / 2,
                         enemySprite.getGlobalBounds().height / 2);
    int patternIndex = rand() % 4; 
    movePattern = static_cast<Pattern>(patternIndex);
    switch (movePattern) {
        case Pattern::Straight:
            velocity = Vector2f(0, 300);
            break;
        case Pattern::Wave:
            velocity = Vector2f(0, 200);
            break;
        case Pattern::Zigzag:
            velocity = Vector2f(0, 250);
            break;
        case Pattern::Shooter:
            velocity = Vector2f(0, 50); 
            break;
    }

    isDying = false;
    deathAnimTimer = 0.0f;
    currentDeathFrame = 0;
    deathSprite.setTexture(deathTextures[0]);
    deathSprite.setPosition(position);
    FloatRect bounds = deathSprite.getLocalBounds();
    deathSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
}

void Enemy::update(float deltaTime) {
    totalTime += deltaTime;
    
    if (movePattern == Pattern::Shooter) {
        shootTimer -= deltaTime;
    }

    switch (movePattern) {
        case Pattern::Straight:
            position += velocity * deltaTime;
            break;
            
        case Pattern::Wave:
            position.y += velocity.y * deltaTime;
            position.x = initialX + sin(totalTime * waveFrequency) * waveAmplitude;
            break;
            
        case Pattern::Zigzag:
            position.y += velocity.y * deltaTime;
            position.x = initialX + sin(totalTime * 5.0f) * 50.0f;
            break;
    }

    animationTimer += deltaTime;  
    if (animationTimer >= FRAME_TIME) {  
        animationTimer = 0;  

        if (enemySprite.getScale().x > 0) {
            enemySprite.setScale(-1.0f, 1.0f);  
        } else {
            enemySprite.setScale(1.0f, 1.0f);   
        }
    }
    enemySprite.setPosition(position);

    if (position.y > 650) {
        active = false;
    }
}

void Enemy::draw(RenderWindow& window) {
    window.draw(enemySprite);
    if (isDying) {
        drawDeathAnimation(window);
    }
}
void Enemy::hit() {
    if (!isDying) {
        startDeathAnimation(); 
        active = false; 
    }
}

void Enemy::updateDeathAnimation(float deltaTime) {
    if (!isDying) return;

    deathAnimTimer += deltaTime;

    const float frameDuration = 0.1f;  
    if (deathAnimTimer >= frameDuration) {
        deathAnimTimer = 0; 
        currentDeathFrame++; 
        if (currentDeathFrame >= 5) { 
            currentDeathFrame = 4;
            isDying = false; 
            active = false;  
        } else {
            deathSprite.setTexture(deathTextures[currentDeathFrame]); 
        }
    }
}

void Enemy::drawDeathAnimation(RenderWindow& window) {
    if (isDying) { 
        window.draw(deathSprite);
    }
}
PowerUp::PowerUp(const Vector2f& pos, Type powerType)
    : Entity(pos, Vector2f(0, 100)), type(powerType), currentFrame(0), animationTimer(0) {
    if (!texturesLoaded) {
        powerUpTextures.resize(5); 
        for (int i = 1; i <= 5; i++) {
            if (!powerUpTextures[i-1].loadFromFile("assets/power/power" + to_string(i) + ".png")) {
                throw runtime_error("Failed to load power" + to_string(i) + ".png");
            }
        }
        texturesLoaded = true;
    }
    powerSprite.setTexture(powerUpTextures[0]);  
    powerSprite.setOrigin(powerSprite.getGlobalBounds().width / 2,
                         powerSprite.getGlobalBounds().height / 2);
    powerSprite.setPosition(pos);
    shouldGravitate = false;
}

void PowerUp::update(float deltaTime, Player* player) {
    if (shouldGravitate) {
        Vector2f playerPosition = player->getPosition();
        Vector2f direction = playerPosition - position;
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance > 0) {
            direction /= distance; 
            position += direction * 200.0f * deltaTime; 
        }
        return; 
    }
    position += velocity * deltaTime;
    animationTimer += deltaTime;
    if (animationTimer >= FRAME_TIME) {
        animationTimer = 0;
        currentFrame = (currentFrame + 1) % 5;  
        powerSprite.setTexture(powerUpTextures[currentFrame]);
    }
    powerSprite.setPosition(position);
    if (position.y > 600) {
        active = false;
    }
}
void PowerUp::draw(RenderWindow& window) {
    window.draw(powerSprite);
}
FloatRect PowerUp::getBounds() const {
    return powerSprite.getGlobalBounds();
}
void PowerUp::hit() {
    active = false;
}
void PowerUp::setPosition(const Vector2f& newPosition) {
    position = newPosition;  
}
EnemyBullet::EnemyBullet(const Vector2f& pos, const Vector2f& vel)
    : Entity(pos, vel) {
    
    if (!textureLoaded) {
        if (!bulletTexture.loadFromFile("assets/bullet.png")) {
            throw runtime_error("Failed to load bullet texture");
        }
        textureLoaded = true;
    }

    bulletSprite.setTexture(bulletTexture);
    
    Vector2u textureSize = bulletTexture.getSize();
    bulletSprite.setOrigin(textureSize.x / 2.0f, textureSize.y / 2.0f);
    bulletSprite.setScale(1.0f, -1.0f);
    bulletSprite.setPosition(position);
}

void EnemyBullet::update(float deltaTime) {
    position += velocity * deltaTime;
    bulletSprite.setPosition(position);
    
    if (position.y > 650 || position.y < -50 || 
        position.x < -50 || position.x > 650) {
        active = false;
    }
}
void EnemyBullet::draw(RenderWindow& window) {
    window.draw(bulletSprite);
}
void EnemyBullet::hit() {
    active = false;
}
Car::Car(float yPosition) 
    : Entity(Vector2f(800, yPosition), Vector2f(-400, 0)) {  
    if (!textureLoaded) {
        if (!carTexture.loadFromFile("assets/enemies/car.png")) {
            throw runtime_error("Failed to load car.png");
        }
        textureLoaded = true;
    }
    
    carSprite.setTexture(carTexture);
    carSprite.setOrigin(carSprite.getGlobalBounds().width / 2,
                        carSprite.getGlobalBounds().height / 2);
    carSprite.setPosition(position);
}

void Car::update(float deltaTime) {
    position += velocity * deltaTime;
    carSprite.setPosition(position);
    if (position.x < -100) { 
        active = false;
    }
}
void Car::draw(RenderWindow& window) {
    window.draw(carSprite);
}
FloatRect Car::getBounds() const {
    return carSprite.getGlobalBounds();
}
void Car::hit() {
    active = false;
}