#include <iostream>
#include <print>
#include <memory>
#include <optional>
#include <variant>

#include "pluspy/pluspy.h"

using namespace std::literals;

struct Address : pluspy::make_dict<Address> {
    std::string city;
    int zipcode;

    Address(std::string city = "", int zipcode = 0) : city(city), zipcode(zipcode) {
        PLUSPY_DICT_MEMBER(Address, city);
        PLUSPY_DICT_MEMBER(Address, zipcode);
    }

    std::string to_string() const {
        return std::format("{} {}", zipcode, city);
    }
};

struct A {
    int value_a;
};

struct B {
    std::string value_b;
};

struct Person : pluspy::make_dict<Person> {
    std::string name;
    int age;
    Address address;
    std::variant<A, B> favorite;

    Person() {
        PLUSPY_DICT_MEMBER(Person, name);
        PLUSPY_DICT_MEMBER(Person, age);
        PLUSPY_DICT_MEMBER(Person, address);
        PLUSPY_DICT_MEMBER(Person, favorite);
    }
};

void print(const pluspy::dict& object) {
    auto name = object["name"].as<std::string>();
    auto age = object["age"].as<int>();
    std::println("{} is {} years old", name, age);

    if (object.is<Person>()) {
        auto address = object["address"].as<Address>().to_string();
        std::println("they live in {}", address);
    }
}

int main() {
    Person a, b;

    a["address"]["city"] = "Alice"s;
    b["address"]["city"] = "Bob"s;

    std::println("{}", a["address"]["city"].as<std::string>());
    std::println("{}", b["address"]["city"].as<std::string>());

    a["favorite"] = std::variant<A, B>(A{42});
}
