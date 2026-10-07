#pragma once
#include "models/Order.h"
#include "models/Client.h"
#include "models/Device.h"
#include <string>

namespace remont {

class PDFGenerator {
public:
    static PDFGenerator& instance();

    bool generateAcceptanceAct(const Order& order,
                               const Client& client,
                               const Device& device,
                               const std::string& outputPath,
                               const std::string& fontPath);

private:
    PDFGenerator() = default;
};

}