#pragma once
#include <string>
#include <vector>

namespace remont {

struct TableData {
    std::string title;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
};

class TableExport {
public:
    static bool exportCsv(const TableData& data, const std::string& path);
    static bool printPdf(const TableData& data,
                         const std::string& path,
                         const std::string& fontPath,
                         const std::string& headerLine1 = "",
                         const std::string& headerLine2 = "");
    static std::string timestamp();
};

}