#pragma once
#include <vector>
#include "models/Part.h"

namespace remont {

class WarehouseService {
public:
    static WarehouseService& instance();

    bool writeOffPart(int partId, int quantity, int orderId, int userId);

    std::vector<Part> getLowStockParts();

private:
    WarehouseService() = default;
};

}