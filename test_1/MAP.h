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

private: // x :-32, 2
         // y : -2, 10
    int minX;
    int maxX;
    int minY;
    int maxY;
};
