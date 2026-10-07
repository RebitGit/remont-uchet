#include "pdf/PDFGenerator.h"
#include "core/Types.h"
#include "resources/utf8.h"

#include <hpdf.h>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace remont {

namespace {

const char* toUtf8(const std::string& s) {
    return s.c_str();
}

std::string currentDate() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << tm->tm_mday << "."
       << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1) << "."
       << (tm->tm_year + 1900);
    return os.str();
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

    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", nullptr);

    HPDF_UseUTFEncodings(pdf);

    const char* fontName = HPDF_LoadTTFontFromFile(pdf, fontPath.c_str(), HPDF_TRUE);
    if (fontName) {
        font = HPDF_GetFont(pdf, fontName, "UTF-8");
    }

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    float width = HPDF_Page_GetWidth(page);
    float height = HPDF_Page_GetHeight(page);

    float left = 50;
    float y = height - 60;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 18);
    HPDF_Page_TextOut(page, left, y, utf8::U("Акт приёма-передачи").utf8_str());
    HPDF_Page_EndText(page);
    y -= 20;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 11);
    std::string sub = "Сервисный центр «Ремонт-Учёт»";
    HPDF_Page_TextOut(page, left, y, sub.c_str());
    HPDF_Page_EndText(page);
    y -= 30;

    auto printRow = [&](const std::string& label, const std::string& value) {
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, 11);
        HPDF_Page_TextOut(page, left, y, label.c_str());
        HPDF_Page_EndText(page);

        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, 12);
        HPDF_Page_TextOut(page, left + 200, y, value.c_str());
        HPDF_Page_EndText(page);
        y -= 22;
    };

    printRow("Номер заказа:", order.orderNumber);
    printRow("Дата приёма:", order.receivedAt);
    printRow("Дата печати:", currentDate());
    y -= 10;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 13);
    HPDF_Page_TextOut(page, left, y, "Клиент");
    HPDF_Page_EndText(page);
    y -= 20;

    printRow("ФИО:", client.fullName);
    printRow("Телефон:", client.phone);
    if (!client.email.empty()) printRow("Email:", client.email);
    y -= 10;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 13);
    HPDF_Page_TextOut(page, left, y, "Устройство");
    HPDF_Page_EndText(page);
    y -= 20;

    printRow("Тип:", device.deviceType);
    printRow("Модель:", device.model);
    if (!device.serialNumber.empty()) printRow("Серийный номер:", device.serialNumber);
    y -= 10;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 13);
    HPDF_Page_TextOut(page, left, y, "Описание неисправности");
    HPDF_Page_EndText(page);
    y -= 20;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 11);
    HPDF_Page_TextOut(page, left, y, order.description.c_str());
    HPDF_Page_EndText(page);
    y -= 40;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 11);
    std::ostringstream cost;
    cost << "Предварительная стоимость: " << std::fixed << std::setprecision(0)
         << order.totalCost << " руб.";
    HPDF_Page_TextOut(page, left, y, cost.str().c_str());
    HPDF_Page_EndText(page);
    y -= 60;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 11);
    HPDF_Page_TextOut(page, left, y, "Подпись клиента: ____________________");
    HPDF_Page_EndText(page);
    y -= 30;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 11);
    HPDF_Page_TextOut(page, left, y, "Подпись приёмщика: ____________________");
    HPDF_Page_EndText(page);

    HPDF_SaveToFile(pdf, outputPath.c_str());
    HPDF_Free(pdf);

    return true;
}

}