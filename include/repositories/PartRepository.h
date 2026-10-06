#pragma once
#include <vector>
#include <sqlite3.h>
#include "models/Part.h"

namespace remont {

class PartRepository {
public:
    static PartRepository& instance();

    bool add(Part& part);
    bool update(const Part& part);
    bool remove(int id);
    bool writeOff(int partId, int quantity);

    Part findById(int id);
    Part findByArticle(const std::string& article);
    std::vector<Part> getAll();
    std::vector<Part> getLowStock();

private:
    PartRepository() = default;

    Part readRow(sqlite3_stmt* stmt);
};

}