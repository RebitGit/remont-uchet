#pragma once
#include <string>
#include "models/User.h"

namespace remont {

class AuthService {
public:
    static AuthService& instance();

    bool authenticate(const std::string& login,
                      const std::string& password,
                      User& outUser);

    static std::string hashPassword(const std::string& password);

private:
    AuthService() = default;
};

}