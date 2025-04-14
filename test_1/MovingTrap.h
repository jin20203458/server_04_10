#pragma once
#include "Map.h"
#include <cmath>
#include <random>
#include <string>

constexpr float TRAP_SPEED = 0.2f; // 트랩 속도
class MovingTrap {
public:
    MovingTrap(float x, float y, const Map& map, const std::string& id)
        : x(x), y(y), speed(TRAP_SPEED), mapRef(map), id(id) 
    {
        setRandomDirection();
    }

    void update()
    {
        float nextX = x + dirX * speed;
        float nextY = y + dirY * speed;

        // 이동 후 맵 유효 범위 확인
        if (mapRef.isValidPosition(static_cast<int>(nextX), static_cast<int>(nextY)))
        {
            x = nextX;
            y = nextY;
        }
        else {
            setRandomDirection(); // 유효 범위를 벗어나면 방향 다시 설정
        }
    }

    float getX() const { return x; }
    float getY() const { return y; }
    const std::string& getId() const { return id; } // 식별자 반환

private:
    float x, y;
    float dirX, dirY; // 방향 단위 벡터
    float speed;
    const Map& mapRef;
    std::string id;   // 고유 식별자

    void setRandomDirection() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> offset(-3.0f, 3.0f); // 주변 3칸 범위

		// 랜덤한 위치 생성
        float tx = x + offset(gen);
        float ty = y + offset(gen);

		// 방향 단위 벡터 계산
        float vx = tx - x;
        float vy = ty - y;
        float mag = std::sqrt(vx * vx + vy * vy);

		// 방향 단위 벡터 정규화
        if (mag != 0.0f)
        {
            dirX = vx / mag;
            dirY = vy / mag;
        }
		else  // 0으로 나누는 경우 방지
        {
            dirX = 1.0f;
            dirY = 0.0f;
        }
    }
};
