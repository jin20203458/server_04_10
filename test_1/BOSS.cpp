#include "BOSS.h"
#include <cstdlib>
#include <ctime>

BOSS::BOSS() : x(0), y(0), hp(2500), maxHp(3000), currentState(BossState::IDLE)
{
	previousHp = hp;
	previousState = currentState;

    lastUpdateTime = GetTickCount64();
    srand(static_cast<unsigned>(time(nullptr)));
    InitializeCriticalSection(&cs);
}

BOSS::~BOSS()
{
    DeleteCriticalSection(&cs);
}

void BOSS::update()
{
    lock();

    DWORD currentTime = GetTickCount64();
    if (currentTime - lastUpdateTime < 2000)
    {
        unlock();
        return;
    }

    lastUpdateTime = currentTime;

    if (isDead())
    {
        currentState = BossState::DEAD;
        unlock();
        return;
    }

    // 상태 변경 - 같은 상태 피하기
    BossState newState = BossState::IDLE; // 기본값
    do {
        int r = rand() % 6;
        switch (r)
        {
        case 0: newState = BossState::IDLE; break;
        case 1: newState = BossState::LeftFistDown; break;
        case 2: newState = BossState::RightFistDown; break;
        case 3: newState = BossState::AllFistDown; break;
        case 4: newState = BossState::Shout; break;
		case 5: newState = BossState::LeftWield; break;
        }
    } while (newState == currentState);

    currentState = newState;

    unlock();
}

void BOSS::takeDamage(int amount)
{
    lock();
    hp -= amount;
    if (hp <= 0)
    {
        hp = 0;
        currentState = BossState::IDLE;
    }
    unlock();
}

bool BOSS::isDead() const
{
    lock();  // const 함수이므로 lock() 호출을 위해 cast
    bool dead = (hp <= 0);
    unlock();
    return dead;
}

void BOSS::reset()
{
    lock();
    hp = maxHp;
    currentState = BossState::IDLE;
    x = y = 0;
    lastUpdateTime = GetTickCount64();
    unlock();
}

BossState BOSS::getState() const
{
    lock();
    BossState state = currentState;
    unlock();
    return state;
}

void BOSS::setState(BossState newState)
{
    lock();
    currentState = newState;
    unlock();
}

bool BOSS::hasStateChanged() {
    lock();
    bool changed = (previousState != currentState);
    previousState = currentState;
    unlock();
    return changed;
}

bool BOSS::hasHpChanged() {
    lock();
    bool changed = (previousHp != hp);
    previousHp = hp;
    unlock();
    return changed;
}

void BOSS::lock() const
{
    EnterCriticalSection(&cs);
}

void BOSS::unlock() const
{
    LeaveCriticalSection(&cs);
}

bool BOSS::tryLock()
{
    return TryEnterCriticalSection(&cs);
}
