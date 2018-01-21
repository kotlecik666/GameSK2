#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
#include <atomic>
#include "GameStructs.h"

using namespace std;

template<typename T>
sf::Vector2<T> normalize(const sf::Vector2<T> &source) {
    float length = sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0)
        return sf::Vector2<T>(source.x / length, source.y / length);
    else
        return source;
}

atomic<bool> gameStarted;
atomic<int> me;
atomic<int> playersNum;
atomic<int> bulletsNum;

AllState * state = nullptr;

vector<sf::Sprite> bulletSprites;
vector<sf::Sprite> timbSprites;
vector<sf::Sprite> sprites;

sf::UdpSocket socket;

sf::Clock lastUpdate;

string myID;

PlayerInput input;

sf::RenderWindow window;

int controls = 0;
int moveSpeed = 3;

void sendInput(sf::IpAddress recipient, unsigned short port) {
    if (controls == 0) {
        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    } else {
        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
        input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    }

    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f aimDirection =
            sf::Vector2f(mousePos) - sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2);

    //if (input.up || input.right || input.left || input.down || input.shoot) {
        if (input.shoot) {
            sf::Vector2f normalDir = normalize(aimDirection);
            input.xDir = normalDir.x;
            input.yDir = normalDir.y;
        }
        if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
            std::cout << "send error" << std::endl;

        }
    //}
}

void receiveData(PlayerState playersArr[], Bullet bulletsArr[]) {
    while(gameStarted && window.isOpen()) {
        char gameData[4096];
        memset(gameData, 0, sizeof(gameData));
        size_t received1;
        sf::IpAddress sender1;
        unsigned short remotePort1;
        if (socket.receive(gameData, 4096, received1, sender1, remotePort1) != sf::Socket::Done) {
            std::cout << "receive error" << std::endl;
        }

        lastUpdate.restart();

        //auto *state = (AllState *) gameData;
        state = (AllState *) gameData;

        //int me = 0;

        playersNum = state->numberOfPlayers;

        for (int i = 0; i < state->numberOfPlayers; i++) {
            playersArr[i] = state->players[i];
            //playersArr[i].position = {(float)state->players[i].x, (float)state->players[i].y};
            //playersArr[i].alive = state->players[i].alive;
            sprites[i].setPosition(state->players[i].x, state->players[i].y);
            timbSprites[i].setPosition(state->players[i].x, state->players[i].y);
            if (strcmp(myID.c_str(), state->players[i].name) == 0)
                me = i;
        }

        bulletsNum = state->numberOfBullets;

        for (int i = 0; i < state->numberOfBullets; i++) {
//            bulletsArr[i].xDir = state->bullets[i].xDir;
//            bulletsArr[i].yDir = state->bullets[i].yDir;
//            bulletsArr[i].xPos = state->bullets[i].xPos;
//            bulletsArr[i].yPos = state->bullets[i].yPos;
            bulletsArr[i] = state->bullets[i];
            bulletSprites[i].setPosition(state->bullets[i].xPos, state->bullets[i].yPos);
        }
    }
}

void loadingScreen(const sf::Sprite &bg, sf::Sprite circle) {
    sf::Clock clock;
    while (window.isOpen() && !gameStarted) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        circle.setRotation(91 * clock.getElapsedTime().asSeconds());
        window.clear();
        window.draw(bg);
        window.draw(circle);
        window.display();
    }
}

int main() {
    cout << "Podaj nick" << endl;
    getline(cin, myID);
    cout << "Wybierz sterowanie: 0 - strzalki, else - wsad" << endl;
    cin >> controls;

    gameStarted = false;

    sf::IpAddress recipient = "127.0.0.1";
    unsigned short port = 50000;

    unsigned short startport = 54000;

    while (socket.bind(startport) != sf::Socket::Done) {
        std::cout << "cant bind to " << startport << std::endl;
        ++startport;
    }

    // graphics setup
    window.create(sf::VideoMode(800, 600), "piu piu piu");
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    sf::View view(sf::Vector2f(128, 128), sf::Vector2f(400, 300));

    sf::Texture loadingbg;
    if (!loadingbg.loadFromFile("resources/loadingbg.png")) {
        return 0;
    }
    sf::Texture circle;
    if (!circle.loadFromFile("resources/loadingcircle.png")) {
        return 0;
    }
    sf::Texture texture;
    if (!texture.loadFromFile("resources/Pacman 3.png")) {
        return 0;
    }
    sf::Texture map;
    if (!map.loadFromFile("resources/map.png")) {
        return 0;
    }
    sf::Texture bullet;
    if (!bullet.loadFromFile("resources/bullet.png")) {
        return 0;
    }
    sf::Texture timb;
    if (!timb.loadFromFile("resources/timbbb.png")) {
        return 0;
    }

    texture.setSmooth(true);
    circle.setSmooth(true);
    map.setSmooth(true);

    sf::Sprite mapSprite(map);

    sf::Sprite loadingSprite(loadingbg);
    sf::Sprite circleSprite(circle);
    circleSprite.setOrigin(50, 50);
    circleSprite.setPosition(400, 400);

    bulletSprites = vector<sf::Sprite>(MAX_BULLETS, sf::Sprite(bullet));
    timbSprites = vector<sf::Sprite>(MAX_PLAYERS, sf::Sprite(timb));
    sprites = vector<sf::Sprite>(MAX_PLAYERS, sf::Sprite(texture));

    for (auto &tim : timbSprites) {
        tim.setOrigin(sf::Vector2f(86, 86));
        tim.setScale(0.6f, 0.6f);
    }

    for (auto &sprite : sprites) {
        sprite.setPosition(1.0, 2.0);
        sprite.setOrigin(sf::Vector2f(128, 128));
        sprite.setScale(0.3f, 0.3f);
    }

    PlayerState playerArr[MAX_PLAYERS];

    Bullet bulletArr[MAX_BULLETS];

    sf::Clock bulletTimer;

    // wejscie do gry
    if (socket.send(myID.c_str(), myID.size(), recipient, port) != sf::Socket::Done) {
        std::cout << "send error" << std::endl;
    }

    char data[100];
    memset(data, 0, sizeof(data));
    size_t received;
    sf::IpAddress sender;
    unsigned short remotePort;
    if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done) {
        std::cout << "receive error" << std::endl;
    }

    cout << "Message: " << data << endl;
    if (strcmp(data, "nameinuse") == 0) {
        cout << "This name is already in use. Bye bye." << endl;
        return -1;
    }

    cout << "Waiting for the game to start" << endl;

    thread loadingThread(loadingScreen, loadingSprite, circleSprite);

    if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done) {
        std::cout << "receive error" << std::endl;
    }

    gameStarted = true;
    loadingThread.join();

    strcpy(input.name, myID.c_str());

    thread r(receiveData, playerArr, bulletArr);

    // petla gry
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        sendInput(recipient, port);

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f aimDirection =
                sf::Vector2f(mousePos) - sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2);

        double radianRotation = atan2(aimDirection.x, -aimDirection.y) / M_PI * 180.0;

        //interpolacja
        if (state != nullptr) {
            auto elapsed = lastUpdate.getElapsedTime().asSeconds() * 60.0; // 60 Hz server freq
            for (int i = 0; i < bulletsNum; i++) {
                bulletSprites[i].setPosition((float)(bulletArr[i].xPos + bulletArr[i].xDir * elapsed),
                                             (float)(bulletArr[i].yPos + bulletArr[i].yDir * elapsed));
            }
            for (int i = 0; i < playersNum; i++) {
                sprites[i].setPosition((float)(playerArr[i].x + playerArr[i].xDir * elapsed * moveSpeed),
                                       (float)(playerArr[i].y + playerArr[i].yDir * elapsed * moveSpeed));
            }
        }

        sprites[me].setRotation((float) radianRotation);
        view.setCenter(sprites[me].getPosition());
        window.setView(view);
        window.clear();
        window.draw(mapSprite);
        for (int i = 0; i < playersNum; i++)
            window.draw((playerArr[i].alive ? sprites : timbSprites)[i]);
        for (int i = 0; i < bulletsNum; i++)
            window.draw(bulletSprites[i]);
        window.display();
    }
    r.join();
    return 0;
}