#include "BOSS.h"
#include <cstdlib>
#include <ctime>

BOSS::BOSS() : x(0), y(0), hp(100), maxHp(100), currentState(BossState::IDLE)
{
    lastUpdateTime = GetTickCount64();
    srand(static_cast<unsigned>(time(nullptr)));
}

void BOSS::update()
{
    DWORD currentTime = GetTickCount64();
    if (currentTime - lastUpdateTime < 3000)
    {
        return; // 아직 3초 안 지남
    }

    lastUpdateTime = currentTime;

    if (isDead())
    {
		currentState = BossState::DEAD;
        return;
    }

    // 상태 변경 - 같은 상태가 나오지 않게
    BossState newState;
    do {
        int r = rand() % 3;
        switch (r) {
        case 0: newState = BossState::IDLE; break;
        case 1: newState = BossState::LFD; break;
        case 2: newState = BossState::RFD; break;
        }
    } while (newState == currentState);  // 이전과 같은 상태면 다시 뽑기

    currentState = newState;
}


void BOSS::takeDamage(int amount) 
{
    hp -= amount;
    if (hp <= 0)
 {
        hp = 0;
        currentState = BossState::IDLE;
    }
}

bool BOSS::isDead() const 
{
    return hp <= 0;
}

void BOSS::reset() 
{
    hp = maxHp;
    currentState = BossState::IDLE;
    x = y = 0;
    lastUpdateTime = GetTickCount64();
}

BossState BOSS::getState() const
{
    return currentState;
}

void BOSS::setState(BossState newState) 
{
    currentState = newState;
}

