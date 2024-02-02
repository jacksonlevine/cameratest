#include "inventory.h"

void Inventory::fakeLoadInventory() {
    nodes[0] = ItemNode{
        52, 4
    };
    nodes[1] = ItemNode{
        255, 4
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