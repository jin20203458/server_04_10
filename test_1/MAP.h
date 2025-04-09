#pragma once
#include <vector>
#include <stdexcept>

class Map {
public:
    Map(int minX, int maxX, int minY, int maxY)
        : minX(minX), maxX(maxX), minY(minY), maxY(maxY)
    {
    }

    bool isValidPosition(int x, int y) const
    {
        return (x >= minX && x <= maxX && y >= minY && y <= maxY);
    }

private: // x : -36, 6
         // y :   0, 11
    int minX;
    int maxX;
    int minY;
    int maxY;
};
