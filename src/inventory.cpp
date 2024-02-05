#include "inventory.h"

void Inventory::fakeLoadInventory() {
    nodes[0] = ItemNode{
        52, 4
    };
    nodes[1] = ItemNode{
        255, 4
    };
    nodes[2] = ItemNode{
        5, 4
    };
    nodes[3] = ItemNode{
        55, 4
    };
    nodes[4] = ItemNode{
        50, 4
    };
    nodes[5] = ItemNode{
        51, 4
    };
    nodes[6] = ItemNode{
        100, 4
    };
    nodes[7] = ItemNode{
        150, 4
    };
    nodes[8] = ItemNode{
        53, 4
    };
    nodes[9] = ItemNode{
        54, 4
    };
}

bool Inventory::addItem(ItemNode item) {
    auto itemIt = std::find_if(nodes, nodes + numberOfSlots, [item](ItemNode& node){
        if(node.id == item.id && node.flags == item.flags && node.canStack) {
            return true;
        }
        return false;
    });
    if(itemIt == nodes + numberOfSlots) {
        auto emptyIt = std::find_if(nodes, nodes + numberOfSlots, [item](ItemNode& node){
            return (node.id == 0);
        });
        if(emptyIt == nodes + numberOfSlots) {
            return false;
        } else {
            *emptyIt = item;
            (*emptyIt).count = 1;
            return true;
        }
    } else {
        (*itemIt).count += 1;
        return true;
    }
}

void Inventory::saveToFile() {
    std::ofstream file("assets/inv", std::ios::trunc);
    for(ItemNode& node : nodes) {
        file << std::to_string(node.id) << "\n";
        file << std::to_string(node.count) << "\n";
        file << std::to_string(node.canStack) << "\n";
        file << std::to_string(node.canPlace) << "\n";
        file << std::to_string(node.flags) << "\n";
    }
        
}

void Inventory::loadFromFile() {
    if(std::filesystem::exists("assets/inv")) {
        std::ifstream file("assets/inv");
        std::string line;
        int index = 0;
        int lineIndex = 0;
        while(std::getline(file, line)) {
            std::istringstream linestream(line);
            std::string word;
            int itemIndex = (index/5);
            while(linestream >> word) {
                if(lineIndex == 0) {
                    nodes[itemIndex].id = std::stoi(word);
                }
                if(lineIndex == 1) {
                    nodes[itemIndex].count = std::stoi(word);
                }
                if(lineIndex == 2) {
                    nodes[itemIndex].canStack = (word == "true");
                }
                if(lineIndex == 3) {
                    nodes[itemIndex].canPlace = (word == "true");
                }
                if(lineIndex == 4) {
                    nodes[itemIndex].flags = std::stoi(word);
                }
            } 
            lineIndex = (lineIndex + 1) % 5;
            index++;
        }
        file.close();
    }
}