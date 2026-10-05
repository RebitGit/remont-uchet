#pragma once
#include <string>
#include "core/Types.h"

namespace remont {

class Logger {
public:
    static Logger& instance();

    void log(int userId, const std::string& action,
             const std::string& entityType = "",
             int entityId = 0);

private:
    Logger() = default;
};

}