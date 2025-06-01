#include <iostream>
#include <vector>

#include "libs/reqs/reqs.h"

int main(int argv, char** argc) {
    WayHome home = WayHome(argc, argv);
    home.getWires(home.cfg.cityFrom, home.cfg.cityTo);
    home.print();

    return 0;
}