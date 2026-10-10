#include "utils/TableExport.h"

#include <hpdf.h>
#include <wx/string.h>
#include <wx/file.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <vector>

namespace remont {

namespace {

std::string escapeCsv(const std::string& s) {
    bool needs = false;
    for (char c : s) {
        if (c == ';' || c == '"' || c == '\n' || c == '\r') {
            needs = true;
            break;
        }
    }
    if (!needs) return s;

    std::string out;
    out.reserve(s.size() + 4);
    out += '"';
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += '"';
    return out;
}

}

std::string TableExport::timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm* tmv = std::localtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tmv);
    return buf;
}

bool TableExport::exportCsv(const TableData& data, const std::string& path) {
    std::ostringstream oss;
    oss << "\xEF\xBB\xBF";

    for (size_t i = 0; i < data.headers.size(); ++i) {
        if (i) oss << ";";
        oss << escapeCsv(data.headers[i]);
    }
    oss << "\r\n";

    for (auto& row : data.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i) oss << ";";
            oss << escapeCsv(row[i]);
        }
        oss << "\r\n";
    }

    std::string content = oss.str();

    wxFile file(wxString::FromUTF8(path), wxFile::write);
    if (!file.IsOpened()) return false;

    ssize_t written = file.Write(content.data(), content.size());
    file.Close();
    return written == static_cast<ssize_t>(content.size());
}

bool TableExport::printPdf(const TableData& data,
                           const std::string& path,
                           const std::string& fontPath,
                           const std::string& headerLine1,
                           const std::string& headerLine2)
{
    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) return false;

    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    HPDF_UseUTFEncodings(pdf);

    const char* fontName = HPDF_LoadTTFontFromFile(pdf, fontPath.c_str(), HPDF_TRUE);
    if (!fontName) {
        HPDF_Free(pdf);
        return false;
    }
    HPDF_Font font = HPDF_GetFont(pdf, fontName, "UTF-8");

    // Ландшафт — больше места под колонки
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);

    float width  = HPDF_Page_GetWidth(page);
    float height = HPDF_Page_GetHeight(page);
    float margin = 40;

    auto newPage = [&]() -> HPDF_Page {
        HPDF_Page p = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(p, HPDF_PAGE_SIZE_A4, HPDF_PAGE_LANDSCAPE);
        return p;
    };

    float y = height - margin;

    // ---- Заголовок документа ----
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 16);
    HPDF_Page_SetRGBFill(page, 0, 0, 0);
    HPDF_Page_TextOut(page, margin, y, data.title.c_str());
    HPDF_Page_EndText(page);
    y -= 22;

    if (!headerLine1.empty()) {
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, 10);
        HPDF_Page_SetRGBFill(page, 0.35f, 0.35f, 0.35f);
        HPDF_Page_TextOut(page, margin, y, headerLine1.c_str());
        HPDF_Page_EndText(page);
        y -= 14;
    }
    if (!headerLine2.empty()) {
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, 10);
        HPDF_Page_SetRGBFill(page, 0.35f, 0.35f, 0.35f);
        HPDF_Page_TextOut(page, margin, y, headerLine2.c_str());
        HPDF_Page_EndText(page);
        y -= 14;
    }
    y -= 12;

    int n = static_cast<int>(data.headers.size());
    if (n <= 0) {
        HPDF_SaveToStream(pdf);
        HPDF_UINT32 streamSize = HPDF_GetStreamSize(pdf);
        std::vector<HPDF_BYTE> buffer(streamSize);
        HPDF_UINT32 bytesRead = streamSize;
        HPDF_ReadFromStream(pdf, buffer.data(), &bytesRead);
        HPDF_ResetStream(pdf);
        HPDF_Free(pdf);

        if (bytesRead != streamSize) return false;

        wxFile file(wxString::FromUTF8(path), wxFile::write);
        if (!file.IsOpened()) return false;
        ssize_t written = file.Write(buffer.data(), streamSize);
        file.Close();
        return written == static_cast<ssize_t>(streamSize);
    }

    // ---- Расчёт ширин колонок по содержимому ----
    HPDF_Page_SetFontAndSize(page, font, 9);

    float tableW = width - 2 * margin;
    float maxColW = tableW * 0.30f;
    float minColW = 40.0f;

    std::vector<float> colW(n, 0.0f);

    for (int i = 0; i < n; ++i) {
        float w = HPDF_Page_TextWidth(page, data.headers[i].c_str());
        for (auto& row : data.rows) {
            if (i < static_cast<int>(row.size())) {
                float wc = HPDF_Page_TextWidth(page, row[i].c_str());
                if (wc > w) w = wc;
            }
        }
        w += 14.0f;
        if (w > maxColW) w = maxColW;
        if (w < minColW) w = minColW;
        colW[i] = w;
    }

    float sumW = 0.0f;
    for (auto w : colW) sumW += w;

    float scale = tableW / sumW;
    for (auto& w : colW) w *= scale;

    // ---- Рисование строк ----
    const float fontSize = 9.0f;

    auto drawRow = [&](const std::vector<std::string>& cells,
                       float rowH, bool isHeader)
    {
        float x = margin;
        for (int i = 0; i < n; ++i) {
            std::string text = (i < static_cast<int>(cells.size()))
                               ? cells[i]
                               : "";

            HPDF_Page_GSave(page);
            HPDF_Page_Rectangle(page, x, y, colW[i], rowH);
            HPDF_Page_Clip(page);
            HPDF_Page_EndPath(page);

            HPDF_Page_BeginText(page);
            HPDF_Page_SetFontAndSize(page, font, fontSize);
            if (isHeader) {
                HPDF_Page_SetRGBFill(page, 0.25f, 0.25f, 0.25f);
            } else {
                HPDF_Page_SetRGBFill(page, 0, 0, 0);
            }
            float textY = y + (rowH - fontSize) / 2.0f + fontSize * 0.2f;
            HPDF_Page_TextOut(page, x + 5, textY, text.c_str());
            HPDF_Page_EndText(page);

            HPDF_Page_GRestore(page);

            x += colW[i];
        }

        // Линия под строкой
        HPDF_Page_SetLineWidth(page, 0.5);
        HPDF_Page_SetRGBStroke(page, 0.85f, 0.85f, 0.85f);
        HPDF_Page_MoveTo(page, margin, y);
        HPDF_Page_LineTo(page, width - margin, y);
        HPDF_Page_Stroke(page);

        y -= rowH;
    };

    drawRow(data.headers, 22, true);

    for (auto& row : data.rows) {
        if (y < margin + 40) {
            page = newPage();
            y = height - margin;
            drawRow(data.headers, 22, true);
        }
        drawRow(row, 18, false);
    }

    // ---- Дата внизу ----
    std::time_t t = std::time(nullptr);
    std::tm* tmv = std::localtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d.%m.%Y", tmv);

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 9);
    HPDF_Page_SetRGBFill(page, 0.4f, 0.4f, 0.4f);
    HPDF_Page_TextOut(page, margin, margin - 10, buf);
    HPDF_Page_EndText(page);

    // ---- Сохранение через поток (для UTF-8 путей) ----
    HPDF_SaveToStream(pdf);
    HPDF_UINT32 streamSize = HPDF_GetStreamSize(pdf);

    std::vector<HPDF_BYTE> buffer(streamSize);
    HPDF_UINT32 bytesRead = streamSize;
    HPDF_ReadFromStream(pdf, buffer.data(), &bytesRead);
    HPDF_ResetStream(pdf);
    HPDF_Free(pdf);

    if (bytesRead != streamSize) return false;

    wxFile file(wxString::FromUTF8(path), wxFile::write);
    if (!file.IsOpened()) return false;

    ssize_t written = file.Write(buffer.data(), streamSize);
    file.Close();

    return written == static_cast<ssize_t>(streamSize);
}

}