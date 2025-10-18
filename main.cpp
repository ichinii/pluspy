#include <iostream>
#include <print>
#include <memory>
#include <optional>
#include <variant>

using namespace std::literals;

#include "pyindex.h"

struct Address : pyindex::Reflector<Address> {
    std::string city;
    int zipcode;

    Address(std::string city = "", int zipcode = 0) : city(city), zipcode(zipcode) {
        PYINDEX_REGISTER(Address, city);
        PYINDEX_REGISTER(Address, zipcode);
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

struct Person : pyindex::Reflector<Person> {
    std::string name;
    int age;
    Address address;
    std::variant<A, B> favorite;

    Person() {
        PYINDEX_REGISTER(Person, name);
        PYINDEX_REGISTER(Person, age);
        PYINDEX_REGISTER(Person, address);
        PYINDEX_REGISTER(Person, favorite);
    }
};

void print(const pyindex::PyObject& object) {
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
