#include "pdf/PDFGenerator.h"
#include "core/Types.h"
#include "resources/utf8.h"

#include <hpdf.h>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>

namespace remont {

namespace {

std::string currentDate() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << tm->tm_mday << "."
       << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1) << "."
       << (tm->tm_year + 1900);
    return os.str();
}

std::vector<std::string> wrapText(HPDF_Page page, HPDF_Font font, float fontSize,
                                  const std::string& text, float maxWidth) {
    std::vector<std::string> result;

    std::istringstream iss(text);
    std::string paragraph;
    std::vector<std::string> paragraphs;

    while (std::getline(iss, paragraph)) {
        paragraphs.push_back(paragraph);
    }

    HPDF_Page_SetFontAndSize(page, font, fontSize);

    for (auto& par : paragraphs) {
        if (par.empty()) {
            result.push_back("");
            continue;
        }

        std::istringstream words(par);
        std::string word;
        std::string line;

        while (words >> word) {
            std::string candidate = line.empty() ? word : line + " " + word;
            float w = HPDF_Page_TextWidth(page, candidate.c_str());
            if (w > maxWidth && !line.empty()) {
                result.push_back(line);
                line = word;
            } else {
                line = candidate;
            }
        }
        if (!line.empty()) {
            result.push_back(line);
        }
    }

    return result;
}

void printLine(HPDF_Page page, HPDF_Font font, float fontSize,
               float x, float y, const std::string& text) {
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, fontSize);
    HPDF_Page_TextOut(page, x, y, text.c_str());
    HPDF_Page_EndText(page);
}

std::string buildSmallPrintText() {
    std::string t;
    t += "1. Срок диагностики — до 3 рабочих дней. Срок ремонта — до 30 рабочих дней. При ожидании запчасти срок ремонта может быть продлён.";
    t += "\n2. При отказе от ремонта оплачивается диагностика согласно прайс-листу сервисного центра.";
    t += "\n3. Гарантия на выполненные работы — 30 дней, на установленные запчасти — 14 дней с момента выдачи устройства.";
    t += "\n4. Гарантия не распространяется на: следы влаги, механические повреждения, вскрытие устройства третьими лицами, использование неоригинальных аксессуаров, а также на программное обеспечение и данные клиента.";
    t += "\n5. Готовое устройство хранится 60 календарных дней с момента уведомления клиента. По истечении срока невостребованное устройство может быть утилизировано.";
    t += "\n6. Сервис не несёт ответственности за сохранность данных на носителях информации. Резервное копирование клиент выполняет самостоятельно до передачи устройства в ремонт.";
    t += "\n7. Выдача устройства производится при предъявлении документа, удостоверяющего личность, и настоящего акта.";
    t += "\n";
    t += "\nС условиями приёма ознакомлен и согласен:  ____________________  /  ____________________";
    return t;
}

}

PDFGenerator& PDFGenerator::instance() {
    static PDFGenerator inst;
    return inst;
}

bool PDFGenerator::generateAcceptanceAct(const Order& order,
                                         const Client& client,
                                         const Device& device,
                                         const std::string& outputPath,
                                         const std::string& fontPath) {
    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) return false;

    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    HPDF_UseUTFEncodings(pdf);

    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", nullptr);

    const char* fontName = HPDF_LoadTTFontFromFile(pdf, fontPath.c_str(), HPDF_TRUE);
    if (fontName) {
        font = HPDF_GetFont(pdf, fontName, "UTF-8");
    }

    if (!font) {
        HPDF_Free(pdf);
        return false;
    }

    const float fontSize    = 11.0f;
    const float lineHeight  = 16.0f;
    const float smallFont   = 9.0f;
    const float smallHeight = 13.0f;

    HPDF_Page page = HPDF_AddPage(pdf);
    if (!page) {
        HPDF_Free(pdf);
        return false;
    }

    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    float width  = HPDF_Page_GetWidth(page);
    float height = HPDF_Page_GetHeight(page);
    float left   = 50;
    float right  = width - 50;
    float y      = height - 60;

    printLine(page, font, fontSize, left, y, "Акт приёма-передачи");
    y -= lineHeight * 1.5f;

    printLine(page, font, fontSize, left, y, "Сервисный центр «Ремонт-Учёт»");
    y -= lineHeight * 1.5f;

    auto printRow = [&](const std::string& label, const std::string& value) {
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, fontSize);
        HPDF_Page_TextOut(page, left, y, label.c_str());
        HPDF_Page_EndText(page);

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, fontSize);
        HPDF_Page_TextOut(page, left + 180, y, value.c_str());
        HPDF_Page_EndText(page);
        y -= lineHeight;
    };

    printRow("Номер заказа:", order.orderNumber);
    printRow("Дата приёма:", order.receivedAt);
    printRow("Дата печати:", currentDate());
    y -= lineHeight * 0.5f;

    printLine(page, font, fontSize, left, y, "Клиент");
    y -= lineHeight;

    printRow("ФИО:", client.fullName);
    printRow("Телефон:", client.phone);
    if (!client.email.empty()) printRow("Email:", client.email);
    y -= lineHeight * 0.5f;

    printLine(page, font, fontSize, left, y, "Устройство");
    y -= lineHeight;

    printRow("Тип:", device.deviceType);
    printRow("Модель:", device.model);
    if (!device.serialNumber.empty()) printRow("Серийный номер:", device.serialNumber);
    y -= lineHeight * 0.5f;

    printLine(page, font, fontSize, left, y, "Описание неисправности");
    y -= lineHeight;

    printLine(page, font, fontSize, left, y, order.description);
    y -= lineHeight * 2.0f;

    std::ostringstream cost;
    cost << "Предварительная стоимость: " << std::fixed << std::setprecision(0)
         << order.totalCost << " руб.";
    printLine(page, font, fontSize, left, y, cost.str());
    y -= lineHeight * 3.0f;

    printLine(page, font, fontSize, left, y, "Подпись клиента: ____________________");
    y -= lineHeight * 1.8f;

    printLine(page, font, fontSize, left, y, "Подпись приёмщика: ____________________");
    y -= lineHeight * 2.0f;

    HPDF_Page_SetLineWidth(page, 0.5f);
    HPDF_Page_SetRGBStroke(page, 0.55f, 0.55f, 0.55f);
    HPDF_Page_MoveTo(page, left, y);
    HPDF_Page_LineTo(page, right, y);
    HPDF_Page_Stroke(page);
    y -= lineHeight;

    printLine(page, font, fontSize, left, y,
              "УСЛОВИЯ ПРИЁМА, ГАРАНТИИ И ОТВЕТСТВЕННОСТЬ");
    y -= lineHeight;

    std::string smallText = buildSmallPrintText();
    std::vector<std::string> lines = wrapText(page, font, smallFont,
                                              smallText, right - left);

    for (auto& line : lines) {
        if (y < 60) {
            HPDF_Page nextPage = HPDF_AddPage(pdf);
            HPDF_Page_SetSize(nextPage, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
            page = nextPage;
            y = height - 60;
        }
        if (!line.empty()) {
            printLine(page, font, smallFont, left, y, line);
        }
        y -= smallHeight;
    }

    HPDF_SaveToFile(pdf, outputPath.c_str());

    HPDF_Free(pdf);

    return true;
}

}