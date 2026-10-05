#pragma once
#include <string>

namespace remont {

struct Client {
    int id = 0;
    std::string fullName;
    std::string phone;
    std::string email;
    std::string createdAt;

    std::string toString() const;
};

}