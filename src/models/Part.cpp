#include "models/Part.h"
#include <sstream>

namespace remont {

std::string Part::toString() const {
    std::ostringstream os;
    os << "Part{id=" << id
       << ", name=" << name
       << ", article=" << article
       << ", qty=" << quantity
       << ", price=" << price
       << ", min=" << minQuantity
       << "}";
    return os.str();
}

}