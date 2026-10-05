#include "models/PartUsage.h"
#include <sstream>

namespace remont {

std::string PartUsage::toString() const {
    std::ostringstream os;
    os << "PartUsage{id=" << id
       << ", order=" << orderId
       << ", part=" << partId
       << ", qty=" << quantity
       << ", at=" << usedAt
       << "}";
    return os.str();
}

}