#pragma once

#include <string>
#include <iostream>

class FavoritePayment {
public:
    std::string name;      
    std::string toCard;    
    std::string note;      

    friend std::ostream &operator<<(std::ostream &os, const FavoritePayment &f) {
        os << f.name << "," << f.toCard << "," << f.note;
        return os;
    }

    friend std::istream &operator>>(std::istream &is, FavoritePayment &f) {
        std::getline(is, f.name, ',');
        std::getline(is, f.toCard, ',');
        std::getline(is, f.note);
        return is;
    }
};


