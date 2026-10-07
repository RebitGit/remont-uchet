#include "services/OrderService.h"
#include "repositories/ClientRepository.h"
#include "repositories/DeviceRepository.h"
#include "repositories/OrderRepository.h"
#include "pdf/PDFGenerator.h"
#include "core/ConfigManager.h"
#include "core/Logger.h"

namespace remont {

OrderService& OrderService::instance() {
    static OrderService inst;
    return inst;
}

bool OrderService::createOrder(const std::string& clientFullName,
                               const std::string& clientPhone,
                               const std::string& clientEmail,
                               const std::string& deviceType,
                               const std::string& deviceModel,
                               const std::string& deviceSerial,
                               const std::string& description,
                               int userId,
                               Order& outOrder) {
    if (clientFullName.empty() || clientPhone.empty()) return false;
    if (deviceModel.empty()) return false;

    Client client = ClientRepository::instance().findByPhone(clientPhone);
    if (client.id == 0) {
        client.fullName = clientFullName;
        client.phone = clientPhone;
        client.email = clientEmail;
        if (!ClientRepository::instance().add(client)) return false;
    }

    Device device;
    device.model = deviceModel;
    device.serialNumber = deviceSerial;
    device.deviceType = deviceType;
    if (!DeviceRepository::instance().add(device)) return false;

    Order order;
    order.clientId = client.id;
    order.deviceId = device.id;
    order.userId = userId;
    order.status = OrderStatus::Accepted;
    order.description = description;
    order.totalCost = 0.0;

    if (!OrderRepository::instance().add(order)) return false;

    Logger::instance().log(userId, "Создание заказа", "order", order.id);

    outOrder = order;
    return true;
}

bool OrderService::changeStatus(int orderId, OrderStatus newStatus) {
    bool ok = OrderRepository::instance().updateStatus(orderId, newStatus);
    if (ok) {
        Logger::instance().log(0, "Смена статуса: " + statusToString(newStatus),
                               "order", orderId);
    }
    return ok;
}

bool OrderService::printAcceptanceAct(int orderId, const std::string& outputPath) {
    Order order = OrderRepository::instance().findById(orderId);
    if (order.id == 0) return false;

    Client client = ClientRepository::instance().findById(order.clientId);
    Device device = DeviceRepository::instance().findById(order.deviceId);

    bool ok = PDFGenerator::instance().generateAcceptanceAct(
        order, client, device, outputPath,
        ConfigManager::instance().fontPath());

    if (ok) {
        Logger::instance().log(0, "Печать акта для заказа " + order.orderNumber,
                               "order", orderId);
    }
    return ok;
}

}