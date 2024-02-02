#ifndef INVENTORY_H
#define INVENTORY_H

#include <GLFW/glfw3.h>

struct ItemNode {
    GLubyte id;
    int count;
    bool canStack = true;
    bool canPlace = true;
    GLubyte flags = 0b00000000;
};

class Inventory {
public:
    inline static const int numberOfSlots = 24;
    ItemNode nodes[numberOfSlots];
    void fakeLoadInventory();
};

#endif 