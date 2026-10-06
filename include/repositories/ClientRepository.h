#pragma once
#include <vector>
#include "models/Client.h"

namespace remont {

class ClientRepository {
public:
    static ClientRepository& instance();

    bool add(Client& client);
    bool update(const Client& client);
    bool remove(int id);

    Client findById(int id);
    Client findByPhone(const std::string& phone);
    std::vector<Client> getAll();

private:
    ClientRepository() = default;
};

}