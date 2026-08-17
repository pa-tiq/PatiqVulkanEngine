#pragma once

namespace pve {

enum class GameState {
    MENU,
    PLAYING,
    PAUSED
};

class GameStateManager {
public:
    static GameStateManager& getInstance() {
        static GameStateManager instance;
        return instance;
    }

    void setState(GameState newState) {
        currentState = newState;
    }

    GameState getState() const {
        return currentState;
    }

    bool isPaused() const {
        return currentState == GameState::PAUSED;
    }

    bool isInMenu() const {
        return currentState == GameState::MENU;
    }

    bool isPlaying() const {
        return currentState == GameState::PLAYING;
    }

private:
    GameStateManager() : currentState(GameState::MENU) {}
    GameState currentState;
};

} // namespace pve
