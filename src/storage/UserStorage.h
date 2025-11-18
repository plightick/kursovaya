#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <optional>
#include <algorithm>
#include "../models/User.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"

namespace storage {

static inline std::filesystem::path usersRoot() {
    return std::filesystem::path("data/users");
}

template <typename TContainer, typename TItemWriter>
void writeVector(std::ostream &os, const TContainer &container, TItemWriter writer) {
    os << container.size() << "\n";
    for (const auto &item : container) {
        writer(os, item);
        os << "\n";
    }
}

class UserStorage {
public:
    static void ensureDataDirs() {
        std::filesystem::create_directories(usersRoot());
    }

    static void saveUser(const RegularUser &user) {
        ensureDataDirs();
        auto path = usersRoot() / (user.usernameValue + ".txt");
        std::ofstream ofs(path);
        if (!ofs) throw BankingError("Cannot write user file: " + path.string());
        ofs << user;
    }

    static RegularUser loadUser(const std::string &username) {
        auto path = usersRoot() / (username + ".txt");
        std::ifstream ifs(path);
        if (!ifs) throw NotFoundError("User not found: " + username);
        RegularUser u;
        ifs >> u;
        return u;
    }

    static bool exists(const std::string &username) {
        auto path = usersRoot() / (username + ".txt");
        return std::filesystem::exists(path);
    }

    static std::vector<std::string> listUsernames() {
        ensureDataDirs();
        std::vector<std::string> names;
        for (auto &entry : std::filesystem::directory_iterator(usersRoot())) {
            if (!entry.is_regular_file()) continue;
            auto name = entry.path().stem().string();
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    static std::vector<RegularUser> loadAll() {
        std::vector<RegularUser> out;
        for (const auto &name : listUsernames()) {
            try {
                out.push_back(loadUser(name));
            } catch (...) {
            }
        }
        return out;
    }
};

}


