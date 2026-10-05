#pragma once
#include <string>
#include "core/Types.h"

namespace remont {

struct User {
    int id = 0;
    std::string login;
    std::string passwordHash;
    Role role = Role::Operator;
    bool isActive = true;

    std::string toString() const;
};

}