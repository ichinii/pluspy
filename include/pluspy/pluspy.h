#pragma once

#include <string>
#include <format>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <source_location>

namespace pluspy {
namespace {
std::string source_location_to_string(const std::source_location& loc) {
    return std::format("`{}` ({}:{}:{})", loc.function_name(), loc.file_name(), loc.line(), loc.column());
}

void throw_type_mismatch(const std::string& object_name, const std::type_index& found_type, const std::type_index& expected_type, const std::source_location& caller_loc, const std::source_location& callee_loc) {
    throw std::runtime_error(
        std::format("type mismatch while casting dict '{}'. found: '{}'. expected: '{}'. detected in {} required from {}",
            object_name,
            found_type.name(),
            expected_type.name(),
            source_location_to_string(callee_loc),
            source_location_to_string(caller_loc)
        ));
}

void throw_no_such_field(const std::string& field_name, const std::string& object_name, const std::source_location& caller_loc, const std::source_location& callee_loc) {
    throw std::out_of_range(
        std::format("no such field '{}' found in dict '{}'. detected in {} required from {}",
            field_name,
            object_name,
            source_location_to_string(callee_loc),
            source_location_to_string(caller_loc)
        ));

}

void throw_not_indexable(const std::string& object_name, const std::string& key, const std::source_location& caller_loc, const std::source_location& callee_loc) {
    throw std::runtime_error(
        std::format("leafe node '{}' cannot be indexed with key {}. detected in {} required from {}",
            object_name,
            key,
            source_location_to_string(callee_loc),
            source_location_to_string(caller_loc)
        ));
}
} // anonymous namespace

class dict;

// ============================
// Base make_dict_base type
// ============================
class make_dict_base {
public:
    virtual ~make_dict_base() = default;
    virtual bool has_key(const std::string& key) const = 0;
    virtual dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) = 0;
    virtual const dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) const = 0;
};

// ============================
// dict: Proxy class
// ============================
struct dict_vtable_base {
    virtual ~dict_vtable_base() = default;
    virtual void* get() = 0;
    virtual const void* get_const() const = 0;
    virtual void set(const void* ) = 0;
};

struct dict_vtable_null : public dict_vtable_base {
    void* get() { throw std::runtime_error("attempt to get value of null dict"); }
    const void* get_const() const { throw std::runtime_error("attempt to get value of null dict"); }
    void set(const void*) { throw std::runtime_error("attempt to set value of null dict"); }
};

template <typename T>
struct dict_vtable_impl : public dict_vtable_base {
    T& dict;

    dict_vtable_impl(T& dict) : dict(dict) {}

    void* get() { return &dict; }
    const void* get_const() const { return &dict; }
    void set(const void* value) { dict = *static_cast<const T*>(value); }
};

class dict {
    std::type_index m_type_id;
    std::string m_name;
    std::shared_ptr<dict_vtable_base> vtable;

public:
    dict()
        : m_type_id(typeid(void)),
          m_name("<null>"),
          vtable(std::make_unique<dict_vtable_null>()) {}

    template<typename T>
    dict(T& obj, std::string name = "<unknown>")
        : m_type_id(typeid(T)),
          m_name(std::move(name)),
          vtable(std::make_unique<dict_vtable_impl<T>>(obj)) {
    }

    // ===========================
    // Type checking and casting
    // ===========================

    template <typename T>
    bool is() const {
        return typeid(T) == m_type_id;
    }

    template<typename T>
    T& as(const std::source_location& caller_loc = std::source_location::current()) {
        if (!is<T>()) throw_type_mismatch(m_name, m_type_id, typeid(T), caller_loc, std::source_location::current());
        return *static_cast<T*>(vtable->get());
    }

    template<typename T>
    const T& as(const std::source_location& caller_loc = std::source_location::current()) const {
        if (!is<T>()) throw_type_mismatch(m_name, m_type_id, typeid(T), caller_loc, std::source_location::current());
        return *static_cast<const T*>(vtable->get_const());
    }

    template<typename T>
    explicit operator T&() { return as<T>(); }

    template<typename T>
    explicit operator const T&() const { return as<T>(); }

    // ===========================
    // Attribute access
    // ===========================

    bool has_key(const std::string& key) const {
        const make_dict_base* ref = dynamic_cast<const make_dict_base*>(static_cast<const make_dict_base*>(vtable->get()));
        return ref && ref->has_key(key);
    }

    dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) {
        make_dict_base* ref = dynamic_cast<make_dict_base*>(static_cast<make_dict_base*>(vtable->get()));
        if (!ref) throw_not_indexable(m_name, key, caller_loc, std::source_location::current());
        return (*ref)[key];
    }

    const dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) const {
        const make_dict_base* ref = dynamic_cast<const make_dict_base*>(static_cast<const make_dict_base*>(vtable->get_const()));
        if (!ref) throw_not_indexable(m_name, key, caller_loc, std::source_location::current());
        return (*ref)[key];
    }

    template<typename T>
    void set(const T& value, const std::source_location& caller_loc = std::source_location::current()) {
        if (!is<T>()) throw_type_mismatch(m_name, m_type_id, typeid(T), caller_loc, std::source_location::current());
        vtable->set(&value);
    }

    // NOTE: unfortunately operator= can not take additional arguments, even if they have default values
    // thus, we cannot receive a std::source_location from the caller side
    // NOTE: this function is very convenient but exposes risky characteristics,
    // like the missing ability to implicitly cast T to whatever type we want to write to
    template<typename T>
    dict operator=(const T& value) {
        set(value);
        return *this;
    }
};

// ============================
// Reflection helper template
// ============================
template<typename T>
class make_dict : public make_dict_base {
    using AccessMap = std::unordered_map<std::string, std::function<dict(T&)>>;
    static AccessMap& registry() {
        static AccessMap map;
        return map;
    }

protected:
    static void registerMember(const std::string& attributeName, std::function<dict(T&)> accessor) {
        registry()[attributeName] = std::move(accessor);
    }

public:
    bool has_key(const std::string& key) const {
        auto it = registry().find(key);
        return it != registry().end();
    }

    dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) override {
        auto it = registry().find(key);
        if (it == registry().end())
            throw_no_such_field(key, typeid(T).name(), caller_loc, std::source_location::current());
        return it->second(static_cast<T&>(*this));
    }

    const dict operator[](const std::string& key, const std::source_location& caller_loc = std::source_location::current()) const override {
        auto it = registry().find(key);
        if (it == registry().end())
            throw_no_such_field(key, typeid(T).name(), caller_loc, std::source_location::current());
        return it->second(const_cast<T&>(static_cast<const T&>(*this)));
    }

    operator dict() {
        return dict(
            *static_cast<T*>(this),
            std::string(typeid(T).name())
        );
    }
};

// ============================
// Member registration macro
// ============================
#define PLUSPY_DICT_MEMBER(Type, Member) \
    pluspy::make_dict<Type>::registerMember(#Member, [](Type& obj) -> pluspy::dict { \
        return pluspy::dict(obj.Member, #Type "::" #Member); \
    });
} // namespace pluspy
