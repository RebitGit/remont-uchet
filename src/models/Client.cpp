#include "models/Client.h"
#include <sstream>

namespace remont {

std::string Client::toString() const {
    std::ostringstream os;
    os << "Client{id=" << id
       << ", name=" << fullName
       << ", phone=" << phone
       << ", email=" << email
       << "}";
    return os.str();
}

}