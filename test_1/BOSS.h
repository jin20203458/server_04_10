#pragma once
#include <windows.h>
#include <cstdint>

// 보스의 상태 정의
enum class BossState : uint8_t 
{
    IDLE = 0x00,          // 대기
    LeftFistDown = 0x01,  // 왼손 내려치기
    RightFistDown = 0x02, // 오른손 내려치기
    AllFistDown = 0x03,   // 양손 내려치기
	Shout = 0x04,         // 외침 (추후 확장 가능)
	LeftWield = 0x05,     // 왼손 휘두르기
	DEAD = 0x06,         // 죽음  
};

class BOSS {
public:
    BOSS();
    ~BOSS();

    void update();                 // 상태 갱신 (3초마다 변경)
    void takeDamage(int amount);   // 피해 처리
    bool isDead() const;           // 죽었는지 확인
    void reset();                  // 초기화

    // 상태 정보 접근
    BossState getState() const;
    void setState(BossState newState);

	// 상태 변경 여부 확인
    bool hasStateChanged();
    bool hasHpChanged();

    // 위치와 생명력 정보
    float x, y;
    int hp;
    int maxHp;

    void lock() const;
    void unlock() const;
    bool tryLock();
private:
    void attack();        // 행동 실행 (추후 확장 가능)
    void movePattern();   // 이동 로직 (필요시)

    BossState currentState;

    DWORD lastUpdateTime; // 마지막 상태 갱신 시간
    
    int previousHp;
    BossState previousState;

    mutable CRITICAL_SECTION cs;
};
