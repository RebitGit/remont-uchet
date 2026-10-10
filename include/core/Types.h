#pragma once
#include <string>

namespace remont {

enum class Role {
    Operator,
    Master,
    Warehouse,
    Admin
};

enum class OrderStatus {
    Accepted,
    Diagnostics,
    InRepair,
    WaitingPart,
    Ready,
    Issued
};

inline std::string roleToString(Role r) {
    switch (r) {
        case Role::Operator:  return "operator";
        case Role::Master:    return "master";
        case Role::Warehouse: return "warehouse";
        case Role::Admin:     return "admin";
    }
    return "unknown";
}

inline std::string statusToString(OrderStatus s) {
    switch (s) {
        case OrderStatus::Accepted:    return "Принят";
        case OrderStatus::Diagnostics: return "Диагностика";
        case OrderStatus::InRepair:    return "В ремонте";
        case OrderStatus::WaitingPart: return "Ожидает запчасть";
        case OrderStatus::Ready:       return "Готов";
        case OrderStatus::Issued:      return "Выдан";
    }
    return "Неизвестно";
}

inline bool canAccessOrders(Role)         { return true; }
inline bool canWriteOrders(Role r)        { return r != Role::Warehouse; }

inline bool canAccessWarehouse(Role r)    { return r != Role::Operator; }
inline bool canWriteWarehouse(Role r)     { return r == Role::Admin || r == Role::Warehouse; }

inline bool canAccessReports(Role r)      { return r == Role::Admin || r == Role::Master; }

inline bool canAccessAdmin(Role r)        { return r == Role::Admin; }

}