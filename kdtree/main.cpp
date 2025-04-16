#include <iostream>
#include <random>
#include "KDTree.h"

#define D 2

int main() {

    int n = 100000;
    int max = 100000;

    KDTree<int, D> tree;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, max - 1);

    for (int i = 0; i < n; ++i) {
        tree.insert({dis(gen)});
    }

    Point<int, D> p1{1111, 3234};
    auto closest = tree.searchClosestNeighbor(p1);
    std::cout << "Plus proche voisin de " << p1 << " : " << closest << "\n";

    return 0;
}
