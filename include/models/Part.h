#pragma once
#include <string>

namespace remont {

struct Part {
    int id = 0;
    std::string name;
    std::string article;
    int quantity = 0;
    double price = 0.0;
    int minQuantity = 0;

    bool isLowStock() const { return quantity <= minQuantity; }
    std::string toString() const;
};

}