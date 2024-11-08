#ifndef INVENTORY_H
#define INVENTORY_H

#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>

struct ItemNode {
    GLubyte id;
    int count;
    bool canStack = true;
    bool canPlace = true;
    GLubyte flags = 0b00000000;
    bool operator==(const ItemNode& other) const;
};

class Inventory {
public:
    inline static const int numberOfSlots = 24;
    ItemNode nodes[numberOfSlots];
    void fakeLoadInventory(); 
    bool addItem(ItemNode item);

    void saveToFile();
    void loadFromFile();
};

#endif 