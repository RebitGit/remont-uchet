#include "models/User.h"
#include <sstream>

namespace remont {

std::string User::toString() const {
    std::ostringstream os;
    os << "User{id=" << id
       << ", login=" << login
       << ", role=" << roleToString(role)
       << ", active=" << (isActive ? "yes" : "no")
       << "}";
    return os.str();
}

}