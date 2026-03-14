// tinyset_plus_v1.3.hpp
#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <any>
#include <variant>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <chrono>
#include <random>
#include <cmath>
#include <queue>
#include <stack>
#include <optional>
#include <typeindex>
#include <concepts>
#include <span>
#include <ranges>
#include <bit>
#include <cstring>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <filesystem>
#include <fstream>
#include <source_location>
#include <stop_token>
#include <syncstream>

namespace tinyset {

// ==================== VERSION ====================
inline constexpr const char* VERSION = "1.3.0";
inline constexpr int VERSION_MAJOR = 1;
inline constexpr int VERSION_MINOR = 3;
inline constexpr int VERSION_PATCH = 0;

// ==================== FORWARD DECLARATIONS ====================
class Store;
template<typename T> class TypedRef;

// ==================== EXCEPTIONS ====================
class TinysetException : public std::runtime_error {
public:
    explicit TinysetException(const std::string& msg, 
                              std::source_location loc = std::source_location::current())
        : std::runtime_error(format(msg, loc)) {}
    
private:
    static std::string format(const std::string& msg, std::source_location loc) {
        return std::filesystem::path(loc.file_name()).filename().string() + 
               ":" + std::to_string(loc.line()) + " - " + msg;
    }
};

class TypeMismatchException : public TinysetException {
    using TinysetException::TinysetException;
};

class NotFoundException : public TinysetException {
    using TinysetException::TinysetException;
};

class ConstraintViolationException : public TinysetException {
    using TinysetException::TinysetException;
};

// ==================== TYPE SYSTEM ====================

using Timestamp = uint64_t;
using ProcessID = std::string;
using SequenceNumber = uint64_t;

// Field types for schema
enum class FieldType : uint8_t {
    Null,
    Bool,
    Int8, Int16, Int32, Int64,
    Uint8, Uint16, Uint32, Uint64,
    Float, Double,
    String,
    Binary,
    Array,
    Object,
    Any,        // Dynamic typing
    Timestamp,
    Date,
    Geographic
};

// Type traits for C++ type -> FieldType mapping
template<typename T> struct FieldTypeMap {};
template<> struct FieldTypeMap<bool> { static constexpr FieldType value = FieldType::Bool; };
template<> struct FieldTypeMap<int8_t> { static constexpr FieldType value = FieldType::Int8; };
template<> struct FieldTypeMap<int16_t> { static constexpr FieldType value = FieldType::Int16; };
template<> struct FieldTypeMap<int32_t> { static constexpr FieldType value = FieldType::Int32; };
template<> struct FieldTypeMap<int64_t> { static constexpr FieldType value = FieldType::Int64; };
template<> struct FieldTypeMap<uint8_t> { static constexpr FieldType value = FieldType::Uint8; };
template<> struct FieldTypeMap<uint16_t> { static constexpr FieldType value = FieldType::Uint16; };
template<> struct FieldTypeMap<uint32_t> { static constexpr FieldType value = FieldType::Uint32; };
template<> struct FieldTypeMap<uint64_t> { static constexpr FieldType value = FieldType::Uint64; };
template<> struct FieldTypeMap<float> { static constexpr FieldType value = FieldType::Float; };
template<> struct FieldTypeMap<double> { static constexpr FieldType value = FieldType::Double; };
template<> struct FieldTypeMap<std::string> { static constexpr FieldType value = FieldType::String; };
template<> struct FieldTypeMap<Timestamp> { static constexpr FieldType value = FieldType::Timestamp; };

// ==================== VALUE TYPE ====================

class Value {
public:
    using Null = std::monostate;
    using Bool = bool;
    using Int8 = int8_t;
    using Int16 = int16_t;
    using Int32 = int32_t;
    using Int64 = int64_t;
    using Uint8 = uint8_t;
    using Uint16 = uint16_t;
    using Uint32 = uint32_t;
    using Uint64 = uint64_t;
    using Float = float;
    using Double = double;
    using String = std::string;
    using Binary = std::vector<uint8_t>;
    using Array = std::vector<Value>;
    using Object = std::unordered_map<std::string, Value>;
    
private:
    using VariantType = std::variant<
        Null, Bool, Int8, Int16, Int32, Int64,
        Uint8, Uint16, Uint32, Uint64,
        Float, Double, String, Binary, Array, Object
    >;
    
    VariantType data_;
    FieldType type_;

    template<typename T>
    static constexpr FieldType deduce_type() {
        if constexpr (std::is_same_v<T, Null>) return FieldType::Null;
        else if constexpr (std::is_same_v<T, Bool>) return FieldType::Bool;
        else if constexpr (std::is_same_v<T, Int8>) return FieldType::Int8;
        else if constexpr (std::is_same_v<T, Int16>) return FieldType::Int16;
        else if constexpr (std::is_same_v<T, Int32>) return FieldType::Int32;
        else if constexpr (std::is_same_v<T, Int64>) return FieldType::Int64;
        else if constexpr (std::is_same_v<T, Uint8>) return FieldType::Uint8;
        else if constexpr (std::is_same_v<T, Uint16>) return FieldType::Uint16;
        else if constexpr (std::is_same_v<T, Uint32>) return FieldType::Uint32;
        else if constexpr (std::is_same_v<T, Uint64>) return FieldType::Uint64;
        else if constexpr (std::is_same_v<T, Float>) return FieldType::Float;
        else if constexpr (std::is_same_v<T, Double>) return FieldType::Double;
        else if constexpr (std::is_same_v<T, String>) return FieldType::String;
        else if constexpr (std::is_same_v<T, Binary>) return FieldType::Binary;
        else if constexpr (std::is_same_v<T, Array>) return FieldType::Array;
        else if constexpr (std::is_same_v<T, Object>) return FieldType::Object;
        else return FieldType::Any;
    }

public:
    // Constructors
    Value() : data_(Null{}), type_(FieldType::Null) {}
    Value(Null) : data_(Null{}), type_(FieldType::Null) {}
    Value(bool b) : data_(b), type_(FieldType::Bool) {}
    Value(int8_t i) : data_(i), type_(FieldType::Int8) {}
    Value(int16_t i) : data_(i), type_(FieldType::Int16) {}
    Value(int32_t i) : data_(i), type_(FieldType::Int32) {}
    Value(int64_t i) : data_(i), type_(FieldType::Int64) {}
    Value(uint8_t i) : data_(i), type_(FieldType::Uint8) {}
    Value(uint16_t i) : data_(i), type_(FieldType::Uint16) {}
    Value(uint32_t i) : data_(i), type_(FieldType::Uint32) {}
    Value(uint64_t i) : data_(i), type_(FieldType::Uint64) {}
    Value(float f) : data_(f), type_(FieldType::Float) {}
    Value(double d) : data_(d), type_(FieldType::Double) {}
    Value(const char* s) : data_(String(s)), type_(FieldType::String) {}
    Value(const std::string& s) : data_(s), type_(FieldType::String) {}
    Value(std::string&& s) : data_(std::move(s)), type_(FieldType::String) {}
    Value(const Binary& b) : data_(b), type_(FieldType::Binary) {}
    Value(Binary&& b) : data_(std::move(b)), type_(FieldType::Binary) {}
    Value(const Array& a) : data_(a), type_(FieldType::Array) {}
    Value(Array&& a) : data_(std::move(a)), type_(FieldType::Array) {}
    Value(const Object& o) : data_(o), type_(FieldType::Object) {}
    Value(Object&& o) : data_(std::move(o)), type_(FieldType::Object) {}
    
    // Type information
    FieldType type() const { return type_; }
    std::string type_name() const;
    
    // Type checking
    bool is_null() const { return type_ == FieldType::Null; }
    bool is_bool() const { return type_ == FieldType::Bool; }
    bool is_int() const { 
        return type_ >= FieldType::Int8 && type_ <= FieldType::Uint64; 
    }
    bool is_float() const { return type_ == FieldType::Float || type_ == FieldType::Double; }
    bool is_string() const { return type_ == FieldType::String; }
    bool is_binary() const { return type_ == FieldType::Binary; }
    bool is_array() const { return type_ == FieldType::Array; }
    bool is_object() const { return type_ == FieldType::Object; }
    bool is_number() const { return is_int() || is_float(); }
    
    // Safe access with type checking
    template<typename T>
    const T* get_if() const {
        if constexpr (std::is_same_v<T, Null>) {
            return std::holds_alternative<Null>(data_) ? &std::get<Null>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Bool>) {
            return std::holds_alternative<Bool>(data_) ? &std::get<Bool>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Int8>) {
            return std::holds_alternative<Int8>(data_) ? &std::get<Int8>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Int16>) {
            return std::holds_alternative<Int16>(data_) ? &std::get<Int16>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Int32>) {
            return std::holds_alternative<Int32>(data_) ? &std::get<Int32>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Int64>) {
            return std::holds_alternative<Int64>(data_) ? &std::get<Int64>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Uint8>) {
            return std::holds_alternative<Uint8>(data_) ? &std::get<Uint8>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Uint16>) {
            return std::holds_alternative<Uint16>(data_) ? &std::get<Uint16>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Uint32>) {
            return std::holds_alternative<Uint32>(data_) ? &std::get<Uint32>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Uint64>) {
            return std::holds_alternative<Uint64>(data_) ? &std::get<Uint64>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Float>) {
            return std::holds_alternative<Float>(data_) ? &std::get<Float>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Double>) {
            return std::holds_alternative<Double>(data_) ? &std::get<Double>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, String>) {
            return std::holds_alternative<String>(data_) ? &std::get<String>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Binary>) {
            return std::holds_alternative<Binary>(data_) ? &std::get<Binary>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Array>) {
            return std::holds_alternative<Array>(data_) ? &std::get<Array>(data_) : nullptr;
        } else if constexpr (std::is_same_v<T, Object>) {
            return std::holds_alternative<Object>(data_) ? &std::get<Object>(data_) : nullptr;
        }
        return nullptr;
    }
    
    template<typename T>
    T as() const {
        if (auto ptr = get_if<T>()) return *ptr;
        throw TypeMismatchException("Type mismatch: expected " + std::string(typeid(T).name()));
    }
    
    // Conversion to double for numeric operations
    std::optional<double> to_double() const {
        if (auto p = get_if<Int8>()) return static_cast<double>(*p);
        if (auto p = get_if<Int16>()) return static_cast<double>(*p);
        if (auto p = get_if<Int32>()) return static_cast<double>(*p);
        if (auto p = get_if<Int64>()) return static_cast<double>(*p);
        if (auto p = get_if<Uint8>()) return static_cast<double>(*p);
        if (auto p = get_if<Uint16>()) return static_cast<double>(*p);
        if (auto p = get_if<Uint32>()) return static_cast<double>(*p);
        if (auto p = get_if<Uint64>()) return static_cast<double>(*p);
        if (auto p = get_if<Float>()) return static_cast<double>(*p);
        if (auto p = get_if<Double>()) return *p;
        return std::nullopt;
    }
    
    std::string to_string() const;
    std::vector<uint8_t> to_binary() const;
    
    // Comparison
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }
};

// ==================== RELATIVE OPERATION ====================

class RelOp {
public:
    enum class Op { Add, Subtract, Multiply, Divide, Mod, Append, Prepend };
    
private:
    Op op_;
    double amount_;
    std::string str_amount_;
    
public:
    explicit RelOp(Op op, double amount) : op_(op), amount_(amount) {}
    explicit RelOp(Op op, std::string amount) : op_(op), amount_(0), str_amount_(std::move(amount)) {}
    
    static RelOp add(double amount) { return RelOp(Op::Add, amount); }
    static RelOp sub(double amount) { return RelOp(Op::Subtract, amount); }
    static RelOp mul(double amount) { return RelOp(Op::Multiply, amount); }
    static RelOp div(double amount) { return RelOp(Op::Divide, amount); }
    static RelOp mod(double amount) { return RelOp(Op::Mod, amount); }
    static RelOp append(std::string s) { return RelOp(Op::Append, std::move(s)); }
    static RelOp prepend(std::string s) { return RelOp(Op::Prepend, std::move(s)); }
    
    Value apply(const Value& current) const;
};

// ==================== SCHEMA DEFINITION ====================

class Schema {
public:
    struct FieldDef {
        FieldType type;
        bool required{false};
        std::optional<Value> default_value;
        std::optional<std::string> validator;  // JavaScript-like validator expression
        std::optional<std::vector<std::string>> enum_values;
        std::optional<std::pair<double, double>> range;  // min/max for numbers
        std::optional<size_t> min_length;
        std::optional<size_t> max_length;
        std::optional<std::string> pattern;  // regex for strings
        bool indexed{false};
        bool unique{false};
        std::optional<std::string> ref;  // reference to another type
    };
    
    using Fields = std::unordered_map<std::string, FieldDef>;
    
private:
    Fields fields_;
    std::vector<std::string> primary_key_;  // which fields form primary key
    std::unordered_map<std::string, std::vector<std::string>> indexes_;
    
public:
    Schema& field(std::string name, FieldType type) {
        fields_[std::move(name)] = FieldDef{.type = type};
        return *this;
    }
    
    Schema& required(std::string name) {
        auto it = fields_.find(name);
        if (it != fields_.end()) it->second.required = true;
        return *this;
    }
    
    Schema& defaults(std::string name, Value value) {
        auto it = fields_.find(name);
        if (it != fields_.end()) it->second.default_value = std::move(value);
        return *this;
    }
    
    Schema& indexed(std::string name, bool unique = false) {
        auto it = fields_.find(name);
        if (it != fields_.end()) {
            it->second.indexed = true;
            it->second.unique = unique;
        }
        return *this;
    }
    
    Schema& range(std::string name, double min, double max) {
        auto it = fields_.find(name);
        if (it != fields_.end()) it->second.range = std::make_pair(min, max);
        return *this;
    }
    
    Schema& pattern(std::string name, std::string regex) {
        auto it = fields_.find(name);
        if (it != fields_.end()) it->second.pattern = std::move(regex);
        return *this;
    }
    
    const Fields& fields() const { return fields_; }
    
    FieldType get_type(const std::string& name) const {
        auto it = fields_.find(name);
        if (it == fields_.end()) return FieldType::Any;
        return it->second.type;
    }
    
    void validate(const std::string& field, const Value& value) const;
    void validate_all(const Value::Object& obj) const;
};

// ==================== ITEM ====================

class Item {
public:
    std::string id;
    std::string type;
    Timestamp created;
    Timestamp modified;
    SequenceNumber version{0};
    Value::Object data;
    std::optional<std::string> owner;
    std::vector<std::string> readers;
    std::vector<std::string> writers;
    
    Item() = default;
    Item(std::string type) : type(std::move(type)) {}
    
    Value get(const std::string& key) const {
        if (key == "id") return id;
        if (key == "type") return type;
        if (key == "created") return static_cast<int64_t>(created);
        if (key == "modified") return static_cast<int64_t>(modified);
        if (key == "version") return static_cast<int64_t>(version);
        
        auto it = data.find(key);
        if (it != data.end()) return it->second;
        return Value{};
    }
    
    void set(const std::string& key, Value value) {
        data[key] = std::move(value);
        modified = now();
        version++;
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    static Timestamp now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// ==================== OPERATION ====================

struct Operation {
    std::string id;
    ProcessID process_id;
    std::map<ProcessID, uint64_t> vector_clock;
    std::string type;  // "create", "update", "delete", "checkpoint"
    std::string item_id;
    std::string item_type;
    Value::Object changes;
    std::optional<Value> old_value;
    std::optional<Value> new_value;
    Timestamp timestamp;
    SequenceNumber sequence{0};
    
    // For causality tracking
    std::vector<std::string> depends_on;  // operation IDs this depends on
};

// ==================== QUERY ====================

class Query {
public:
    struct Condition {
        std::optional<double> gt;
        std::optional<double> lt;
        std::optional<double> gte;
        std::optional<double> lte;
        std::optional<std::string> eq;
        std::optional<std::string> ne;
        std::optional<std::string> contains;
        std::optional<std::string> starts_with;
        std::optional<std::string> ends_with;
        std::optional<std::vector<Value>> in;
        std::optional<std::vector<Value>> nin;
        std::optional<bool> exists;
        std::optional<std::string> regex;
    };
    
private:
    std::optional<std::string> type_filter_;
    std::unordered_map<std::string, std::variant<Value, Condition>> criteria_;
    std::optional<std::pair<double, double>> near_;
    std::optional<double> max_distance_;
    std::vector<std::string> sort_fields_;
    std::vector<bool> sort_descending_;
    size_t limit_{0};
    size_t offset_{0};
    bool count_{false};
    bool ids_only_{false};
    bool explain_{false};
    
public:
    Query& with_type(std::string type) {
        type_filter_ = std::move(type);
        return *this;
    }
    
    Query& where(std::string field, Value value) {
        criteria_[std::move(field)] = value;
        return *this;
    }
    
    Query& where(std::string field, Condition cond) {
        criteria_[std::move(field)] = std::move(cond);
        return *this;
    }
    
    Query& near(double x, double y, double max_distance) {
        near_ = std::make_pair(x, y);
        max_distance_ = max_distance;
        return *this;
    }
    
    Query& sort_by(std::string field, bool descending = false) {
        sort_fields_.push_back(std::move(field));
        sort_descending_.push_back(descending);
        return *this;
    }
    
    Query& limit(size_t n) { limit_ = n; return *this; }
    Query& offset(size_t n) { offset_ = n; return *this; }
    Query& count(bool b = true) { count_ = b; return *this; }
    Query& ids_only(bool b = true) { ids_only_ = b; return *this; }
    Query& explain(bool b = true) { explain_ = b; return *this; }
    
    const auto& type_filter() const { return type_filter_; }
    const auto& criteria() const { return criteria_; }
    const auto& near() const { return near_; }
    auto max_distance() const { return max_distance_; }
    const auto& sort_fields() const { return sort_fields_; }
    const auto& sort_descending() const { return sort_descending_; }
    auto limit() const { return limit_; }
    auto offset() const { return offset_; }
    auto count() const { return count_; }
    auto ids_only() const { return ids_only_; }
    auto explain() const { return explain_; }
};

// ==================== SPATIAL INDEX ====================

template<typename T>
class QuadTree {
    struct Node {
        double x, y, w, h;  // center x,y, width, height
        std::vector<std::pair<double, double>> points;  // (x,y) pairs
        std::vector<T> items;
        std::unique_ptr<Node> nw, ne, sw, se;
        size_t capacity{16};
        bool is_leaf{true};
        
        Node(double cx, double cy, double width, double height) 
            : x(cx), y(cy), w(width), h(height) {}
        
        void subdivide() {
            double hw = w / 2;
            double hh = h / 2;
            nw = std::make_unique<Node>(x - hw/2, y - hh/2, hw, hh);
            ne = std::make_unique<Node>(x + hw/2, y - hh/2, hw, hh);
            sw = std::make_unique<Node>(x - hw/2, y + hh/2, hw, hh);
            se = std::make_unique<Node>(x + hw/2, y + hh/2, hw, hh);
            is_leaf = false;
            
            // Redistribute points
            for (size_t i = 0; i < points.size(); i++) {
                insert_point(points[i].first, points[i].second, items[i]);
            }
            points.clear();
            items.clear();
        }
        
        void insert_point(double px, double py, const T& item) {
            if (is_leaf) {
                if (points.size() < capacity) {
                    points.emplace_back(px, py);
                    items.push_back(item);
                } else {
                    subdivide();
                    insert_point(px, py, item);
                }
            } else {
                if (px < x) {
                    if (py < y) nw->insert_point(px, py, item);
                    else sw->insert_point(px, py, item);
                } else {
                    if (py < y) ne->insert_point(px, py, item);
                    else se->insert_point(px, py, item);
                }
            }
        }
        
        void query_near(double qx, double qy, double r, std::vector<T>& results) const {
            if (!intersects_circle(qx, qy, r)) return;
            
            if (is_leaf) {
                for (size_t i = 0; i < points.size(); i++) {
                    double dx = points[i].first - qx;
                    double dy = points[i].second - qy;
                    if (dx*dx + dy*dy <= r*r) {
                        results.push_back(items[i]);
                    }
                }
            } else {
                nw->query_near(qx, qy, r, results);
                ne->query_near(qx, qy, r, results);
                sw->query_near(qx, qy, r, results);
                se->query_near(qx, qy, r, results);
            }
        }
        
        bool intersects_circle(double cx, double cy, double r) const {
            double dx = std::abs(cx - x);
            double dy = std::abs(cy - y);
            
            if (dx > w/2 + r) return false;
            if (dy > h/2 + r) return false;
            
            if (dx <= w/2 || dy <= h/2) return true;
            
            double corner_dx = dx - w/2;
            double corner_dy = dy - h/2;
            return (corner_dx*corner_dx + corner_dy*corner_dy <= r*r);
        }
    };
    
    std::unique_ptr<Node> root_;
    std::unordered_map<T, std::pair<double, double>> locations_;
    
public:
    QuadTree(double x, double y, double width, double height, size_t capacity = 16) {
        root_ = std::make_unique<Node>(x, y, width, height);
        root_->capacity = capacity;
    }
    
    void insert(const T& id, double x, double y) {
        root_->insert_point(x, y, id);
        locations_[id] = {x, y};
    }
    
    void update(const T& id, double new_x, double new_y) {
        auto it = locations_.find(id);
        if (it != locations_.end()) {
            remove(id, it->second.first, it->second.second);
            insert(id, new_x, new_y);
        }
    }
    
    void remove(const T& id, double x, double y) {
        // Simplified - in production you'd need to remove from tree
        locations_.erase(id);
    }
    
    std::vector<T> query_near(double x, double y, double radius) const {
        std::vector<T> results;
        if (root_) root_->query_near(x, y, radius, results);
        return results;
    }
};

// ==================== JOURNAL ====================

class Journal {
    std::vector<Operation> operations_;
    std::shared_mutex mutex_;
    size_t max_size_{1000000};  // 1M operations default
    size_t checkpoint_interval_{10000};
    
public:
    void append(Operation op) {
        std::unique_lock lock(mutex_);
        operations_.push_back(std::move(op));
        
        if (operations_.size() > max_size_) {
            prune();
        }
    }
    
    std::vector<Operation> query(Timestamp since = 0, 
                                 const std::optional<ProcessID>& process = std::nullopt) const {
        std::shared_lock lock(mutex_);
        std::vector<Operation> result;
        
        auto it = std::lower_bound(operations_.begin(), operations_.end(), since,
            [](const Operation& op, Timestamp ts) { return op.timestamp < ts; });
        
        for (; it != operations_.end(); ++it) {
            if (!process || it->process_id == *process) {
                result.push_back(*it);
            }
        }
        
        return result;
    }
    
    void checkpoint(Timestamp checkpoint_time) {
        std::unique_lock lock(mutex_);
        auto it = std::remove_if(operations_.begin(), operations_.end(),
            [checkpoint_time](const Operation& op) {
                return op.timestamp < checkpoint_time && op.type != "checkpoint";
            });
        operations_.erase(it, operations_.end());
    }
    
    void prune() {
        // Keep last N operations, plus all checkpoints
        size_t keep = max_size_ / 2;
        auto it = operations_.end() - keep;
        while (it != operations_.end() && it->type != "checkpoint") {
            ++it;
        }
        operations_.erase(operations_.begin(), it);
    }
    
    size_t size() const {
        std::shared_lock lock(mutex_);
        return operations_.size();
    }
    
    void clear() {
        std::unique_lock lock(mutex_);
        operations_.clear();
    }
};

// ==================== TRANSACTION ====================

class Transaction : public std::enable_shared_from_this<Transaction> {
public:
    using ID = uint64_t;
    using Callback = std::function<void()>;
    
    struct Record {
        enum Type { CREATE, UPDATE, DELETE } type;
        std::string id;
        std::shared_ptr<Item> old_item;
        std::shared_ptr<Item> new_item;
    };
    
private:
    ID id_;
    std::vector<Record> records_;
    Callback on_commit_;
    Callback on_rollback_;
    bool committed_{false};
    bool rolled_back_{false};
    std::shared_mutex mutex_;
    
public:
    Transaction(ID id, Callback commit_cb, Callback rollback_cb)
        : id_(id), on_commit_(std::move(commit_cb)), on_rollback_(std::move(rollback_cb)) {}
    
    ~Transaction() {
        if (!committed_ && !rolled_back_) {
            rollback();
        }
    }
    
    ID id() const { return id_; }
    
    void record(Record rec) {
        std::unique_lock lock(mutex_);
        records_.push_back(std::move(rec));
    }
    
    void commit() {
        std::unique_lock lock(mutex_);
        if (!committed_ && !rolled_back_) {
            committed_ = true;
            if (on_commit_) on_commit_();
        }
    }
    
    void rollback() {
        std::unique_lock lock(mutex_);
        if (!committed_ && !rolled_back_) {
            rolled_back_ = true;
            if (on_rollback_) on_rollback_();
        }
    }
    
    const auto& records() const { return records_; }
    bool is_active() const { return !committed_ && !rolled_back_; }
};

// ==================== CONFIG ====================

struct Config {
    std::function<std::string()> id_generator = []() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;
        
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        return std::to_string(ts) + "-" + std::to_string(dis(gen));
    };
    
    bool validate_types{true};
    bool strict_mode{false};  // Throw on validation errors vs warn
    ProcessID process_id{"process-1"};
    std::unordered_map<std::string, Schema> schemas;
    std::unordered_map<std::string, Value> defaults;
    
    // Performance tuning
    size_t journal_max_size{1000000};
    size_t checkpoint_interval{10000};
    bool enable_spatial_index{true};
    size_t spatial_index_capacity{16};
    
    // Persistence
    std::optional<std::filesystem::path> persistence_path;
    std::chrono::milliseconds persistence_interval{5000};
    
    // Security
    bool enable_auth{false};
    std::function<bool(const std::string&, const std::string&)> authorize;
};

// ==================== MAIN STORE ====================

class Store : public std::enable_shared_from_this<Store> {
public:
    using ReadLock = std::shared_lock<std::shared_mutex>;
    using WriteLock = std::unique_lock<std::shared_mutex>;
    
private:
    mutable std::shared_mutex mutex_;
    
    // Core storage
    std::unordered_map<std::string, std::shared_ptr<Item>> items_;
    
    // Indexes
    std::unordered_map<std::string, std::unordered_set<std::string>> type_index_;
    std::unordered_map<std::string, std::unordered_map<Value, std::unordered_set<std::string>>> value_index_;
    std::unique_ptr<QuadTree<std::string>> spatial_index_;
    
    // Schema
    std::unordered_map<std::string, Schema> schemas_;
    
    // Event system
    std::unordered_map<std::string, std::vector<std::function<void(const Value&)>>> listeners_;
    mutable std::shared_mutex listeners_mutex_;
    
    // Transactions
    std::vector<std::shared_ptr<Transaction>> transaction_stack_;
    
    // Journal
    Journal journal_;
    
    // Vector clock
    std::map<ProcessID, uint64_t> vector_clock_;
    std::shared_mutex clock_mutex_;
    
    // Config
    Config config_;
    
    // Persistence thread
    std::jthread persistence_thread_;
    std::condition_variable_any persistence_cv_;
    std::shared_mutex persistence_mutex_;
    bool persistence_dirty_{false};
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> reads{0};
        std::atomic<uint64_t> writes{0};
        std::atomic<uint64_t> queries{0};
        std::atomic<uint64_t> transactions{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> cache_misses{0};
    } stats_;
    
public:
    explicit Store(Config config = {}) : config_(std::move(config)) {
        vector_clock_[config_.process_id] = 0;
        
        if (config_.enable_spatial_index) {
            spatial_index_ = std::make_unique<QuadTree<std::string>>(
                0, 0, 1000000, 1000000, config_.spatial_index_capacity
            );
        }
        
        // Load from disk if configured
        if (config_.persistence_path) {
            load_from_disk();
            start_persistence_thread();
        }
    }
    
    ~Store() {
        if (persistence_thread_.joinable()) {
            persistence_thread_.request_stop();
            persistence_cv_.notify_all();
            persistence_thread_.join();
        }
        
        // Final save
        if (config_.persistence_path) {
            save_to_disk();
        }
    }
    
    // Disable copy
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    
    // ==================== SCHEMA API ====================
    
    void define_schema(std::string type, Schema schema) {
        WriteLock lock(mutex_);
        schemas_[std::move(type)] = std::move(schema);
    }
    
    // ==================== CREATE ====================
    
    std::string create(const std::string& type, Value::Object data = {}, 
                       std::optional<std::string> id = std::nullopt) {
        stats_.writes++;
        
        // Validate against schema
        validate_against_schema(type, data);
        
        // Generate ID if not provided
        std::string item_id = id.value_or(config_.id_generator());
        
        // Apply defaults
        apply_defaults(type, data);
        
        WriteLock lock(mutex_);
        
        auto item = std::make_shared<Item>(type);
        item->id = item_id;
        item->created = Item::now();
        item->modified = item->created;
        item->data = std::move(data);
        
        items_[item_id] = item;
        
        // Update indexes
        update_indexes("add", item);
        
        // Record operation
        record_operation("create", item_id, type, item->data);
        
        // Emit events (outside lock)
        lock.unlock();
        emit("create", item_id);
        emit("change", Value::Object{{"type", "create"}, {"id", item_id}});
        
        return item_id;
    }
    
    // ==================== TYPED CREATE ====================
    
    template<typename T>
    requires std::is_class_v<T>
    std::string create(const T& obj, std::optional<std::string> id = std::nullopt) {
        Value::Object data;
        
        // Convert struct to Value::Object using reflection (simplified)
        // In production, use a serialization library like Boost.PFR or custom macros
        visit_struct(obj, [&](const char* name, const auto& value) {
            data[name] = Value(value);
        });
        
        return create(T::type_name, std::move(data), id);
    }
    
    // ==================== GET ====================
    
    std::optional<Value> get(const std::string& id, 
                             std::optional<std::vector<std::string>> fields = std::nullopt) const {
        stats_.reads++;
        
        ReadLock lock(mutex_);
        
        auto it = items_.find(id);
        if (it == items_.end()) {
            stats_.cache_misses++;
            return std::nullopt;
        }
        
        stats_.cache_hits++;
        
        if (!fields) {
            // Return full item as Value
            Value::Object result;
            result["id"] = it->second->id;
            result["type"] = it->second->type;
            result["created"] = static_cast<int64_t>(it->second->created);
            result["modified"] = static_cast<int64_t>(it->second->modified);
            
            for (const auto& [k, v] : it->second->data) {
                result[k] = v;
            }
            
            return result;
        }
        
        // Return only requested fields
        Value::Object result;
        for (const auto& f : *fields) {
            if (f == "id") result["id"] = it->second->id;
            else if (f == "type") result["type"] = it->second->type;
            else if (f == "created") result["created"] = static_cast<int64_t>(it->second->created);
            else if (f == "modified") result["modified"] = static_cast<int64_t>(it->second->modified);
            else {
                auto dit = it->second->data.find(f);
                if (dit != it->second->data.end()) {
                    result[f] = dit->second;
                }
            }
        }
        
        return result;
    }
    
    template<typename T>
    std::optional<T> get_as(const std::string& id) {
        auto val = get(id);
        if (!val) return std::nullopt;
        
        T result;
        // Deserialize from Value to T (simplified)
        // In production, use a serialization library
        return result;
    }
    
    bool exists(const std::string& id) const {
        ReadLock lock(mutex_);
        return items_.contains(id);
    }
    
    // ==================== SET (TYPED) ====================
    
    void set_number(const std::string& id, const std::string& field, double value) {
        stats_.writes++;
        set_impl(id, field, Value(value), FieldType::Double);
    }
    
    void set_string(const std::string& id, const std::string& field, std::string_view value) {
        stats_.writes++;
        set_impl(id, field, Value(std::string(value)), FieldType::String);
    }
    
    void set_bool(const std::string& id, const std::string& field, bool value) {
        stats_.writes++;
        set_impl(id, field, Value(value), FieldType::Bool);
    }
    
    void set_int(const std::string& id, const std::string& field, int64_t value) {
        stats_.writes++;
        set_impl(id, field, Value(value), FieldType::Int64);
    }
    
    void set(const std::string& id, const std::string& field, Value value) {
        stats_.writes++;
        
        // Get type from schema if available
        auto item = get_item(id);
        if (!item) throw NotFoundException("Item not found: " + id);
        
        FieldType expected = FieldType::Any;
        auto sit = schemas_.find(item->type);
        if (sit != schemas_.end()) {
            expected = sit->second.get_type(field);
        }
        
        set_impl(id, field, std::move(value), expected);
    }
    
    // ==================== RELATIVE UPDATES ====================
    
    void increment(const std::string& id, const std::string& field, double by = 1.0) {
        apply_relative(id, field, RelOp::add(by));
    }
    
    void decrement(const std::string& id, const std::string& field, double by = 1.0) {
        apply_relative(id, field, RelOp::sub(by));
    }
    
    void multiply(const std::string& id, const std::string& field, double by) {
        apply_relative(id, field, RelOp::mul(by));
    }
    
    void divide(const std::string& id, const std::string& field, double by) {
        if (by == 0) throw ConstraintViolationException("Division by zero");
        apply_relative(id, field, RelOp::div(by));
    }
    
    void append(const std::string& id, const std::string& field, std::string_view value) {
        apply_relative(id, field, RelOp::append(std::string(value)));
    }
    
    void prepend(const std::string& id, const std::string& field, std::string_view value) {
        apply_relative(id, field, RelOp::prepend(std::string(value)));
    }
    
    void apply_relative(const std::string& id, const std::string& field, const RelOp& op) {
        stats_.writes++;
        
        auto item = get_item(id);
        if (!item) throw NotFoundException("Item not found: " + id);
        
        // Get current value
        Value current = item->get(field);
        
        // Apply operation
        Value new_value = op.apply(current);
        
        // Set with appropriate type
        set(id, field, std::move(new_value));
    }
    
    // ==================== REMOVE ====================
    
    bool remove(const std::string& id) {
        stats_.writes++;
        
        WriteLock lock(mutex_);
        
        auto it = items_.find(id);
        if (it == items_.end()) return false;
        
        auto item = it->second;
        
        // Remove from indexes
        update_indexes("remove", item);
        
        // Record operation
        record_operation("delete", id, item->type, {});
        
        // Erase from storage
        items_.erase(it);
        
        lock.unlock();
        
        emit("delete", id);
        emit("change", Value::Object{{"type", "delete"}, {"id", id}});
        
        return true;
    }
    
    size_t remove_many(const std::vector<std::string>& ids) {
        size_t count = 0;
        for (const auto& id : ids) {
            if (remove(id)) count++;
        }
        return count;
    }
    
    size_t remove_by_query(const Query& query) {
        auto ids = find_ids(query);
        return remove_many(ids);
    }
    
    // ==================== FIND ====================
    
    std::vector<std::string> find_ids(const Query& query) const {
        stats_.queries++;
        
        ReadLock lock(mutex_);
        
        std::unordered_set<std::string> results;
        
        // Start with type filter if present
        if (query.type_filter()) {
            auto it = type_index_.find(*query.type_filter());
            if (it != type_index_.end()) {
                results = it->second;
            } else {
                return {};
            }
        } else {
            // All items
            for (const auto& [id, _] : items_) {
                results.insert(id);
            }
        }
        
        // Apply criteria
        for (const auto& [field, cond] : query.criteria()) {
            std::unordered_set<std::string> filtered;
            
            if (std::holds_alternative<Value>(cond)) {
                // Direct equality - use index if available
                const auto& value = std::get<Value>(cond);
                auto idx_it = value_index_.find(field);
                if (idx_it != value_index_.end()) {
                    auto val_it = idx_it->second.find(value);
                    if (val_it != idx_it->second.end()) {
                        for (const auto& id : val_it->second) {
                            if (results.contains(id)) filtered.insert(id);
                        }
                    }
                } else {
                    // Fallback to scan
                    for (const auto& id : results) {
                        auto it = items_.find(id);
                        if (it != items_.end()) {
                            auto val = it->second->get(field);
                            if (val == value) filtered.insert(id);
                        }
                    }
                }
            } else {
                // Complex condition - must scan
                const auto& condition = std::get<Query::Condition>(cond);
                
                for (const auto& id : results) {
                    auto it = items_.find(id);
                    if (it != items_.end()) {
                        if (matches_condition(it->second, field, condition)) {
                            filtered.insert(id);
                        }
                    }
                }
            }
            
            results = std::move(filtered);
            if (results.empty()) break;
        }
        
        // Spatial filter
        if (query.near()) {
            auto [x, y] = *query.near();
            double max_dist = query.max_distance().value_or(100.0);
            
            if (spatial_index_) {
                auto spatial_results = spatial_index_->query_near(x, y, max_dist);
                std::unordered_set<std::string> spatial_set(
                    spatial_results.begin(), spatial_results.end());
                
                std::erase_if(results, [&](const auto& id) {
                    return !spatial_set.contains(id);
                });
            }
        }
        
        // Convert to vector and sort
        std::vector<std::string> result_vec(results.begin(), results.end());
        
        // Apply sorting
        if (!query.sort_fields().empty()) {
            sort_results(result_vec, query);
        }
        
        // Apply pagination
        if (query.offset() > 0 && query.offset() < result_vec.size()) {
            result_vec.erase(result_vec.begin(), 
                           result_vec.begin() + query.offset());
        }
        
        if (query.limit() > 0 && query.limit() < result_vec.size()) {
            result_vec.resize(query.limit());
        }
        
        return result_vec;
    }
    
    std::vector<Value> find(const Query& query) const {
        auto ids = find_ids(query);
        
        if (query.ids_only()) {
            std::vector<Value> result;
            for (const auto& id : ids) {
                result.push_back(id);
            }
            return result;
        }
        
        if (query.count()) {
            return {static_cast<int64_t>(ids.size())};
        }
        
        std::vector<Value> results;
        for (const auto& id : ids) {
            auto val = get(id);
            if (val) results.push_back(*val);
        }
        
        return results;
    }
    
    // ==================== TRANSACTIONS ====================
    
    std::shared_ptr<Transaction> begin_transaction() {
        stats_.transactions++;
        
        WriteLock lock(mutex_);
        
        static std::atomic<uint64_t> next_id{1};
        auto id = next_id++;
        
        auto tx = std::make_shared<Transaction>(
            id,
            [self = shared_from_this(), id]() { self->commit_transaction(id); },
            [self = shared_from_this(), id]() { self->rollback_transaction(id); }
        );
        
        transaction_stack_.push_back(tx);
        return tx;
    }
    
    template<typename F>
    auto with_transaction(F&& f) -> decltype(f(std::declval<std::shared_ptr<Transaction>>())) {
        auto tx = begin_transaction();
        try {
            auto result = f(tx);
            tx->commit();
            return result;
        } catch (...) {
            tx->rollback();
            throw;
        }
    }
    
    // ==================== EVENTS ====================
    
    size_t on(const std::string& event, std::function<void(const Value&)> callback) {
        WriteLock lock(listeners_mutex_);
        listeners_[event].push_back(std::move(callback));
        return listeners_[event].size() - 1;
    }
    
    void off(const std::string& event, size_t index) {
        WriteLock lock(listeners_mutex_);
        auto it = listeners_.find(event);
        if (it != listeners_.end() && index < it->second.size()) {
            it->second.erase(it->second.begin() + index);
        }
    }
    
    template<typename... Args>
    void emit(const std::string& event, Args&&... args) {
        Value data;
        if constexpr (sizeof...(Args) == 1) {
            // Single argument
            data = Value(std::forward<Args>(args)...);
        } else {
            // Multiple arguments as array
            Value::Array arr;
            (arr.push_back(Value(std::forward<Args>(args))), ...);
            data = std::move(arr);
        }
        
        std::vector<std::function<void(const Value&)>> callbacks;
        {
            ReadLock lock(listeners_mutex_);
            auto it = listeners_.find(event);
            if (it != listeners_.end()) {
                callbacks = it->second;
            }
        }
        
        for (const auto& cb : callbacks) {
            try {
                cb(data);
            } catch (const std::exception& e) {
                std::cerr << "Event handler error: " << e.what() << std::endl;
            }
        }
    }
    
    // ==================== SYNC API ====================
    
    Value export_log(Timestamp since = 0, 
                    const std::optional<ProcessID>& process = std::nullopt) const {
        auto ops = journal_.query(since, process);
        
        Value::Array op_array;
        for (const auto& op : ops) {
            Value::Object op_obj;
            op_obj["id"] = op.id;
            op_obj["processId"] = op.process_id;
            op_obj["type"] = op.type;
            op_obj["itemId"] = op.item_id;
            op_obj["timestamp"] = static_cast<int64_t>(op.timestamp);
            op_array.push_back(std::move(op_obj));
        }
        
        Value::Object result;
        result["operations"] = std::move(op_array);
        result["processId"] = config_.process_id;
        result["timestamp"] = static_cast<int64_t>(Item::now());
        
        // Add vector clock
        Value::Object clock;
        ReadLock lock(clock_mutex_);
        for (const auto& [pid, time] : vector_clock_) {
            clock[pid] = static_cast<int64_t>(time);
        }
        result["vectorClock"] = std::move(clock);
        
        return result;
    }
    
    size_t import_log(const Value& log, bool force = false) {
        if (!log.is_object()) return 0;
        
        const auto& obj = *log.get<Value::Object>();
        
        // Merge vector clock
        auto clock_it = obj.find("vectorClock");
        if (clock_it != obj.end() && clock_it->second.is_object()) {
            merge_clock(*clock_it->second.get<Value::Object>());
        }
        
        // Import operations
        auto ops_it = obj.find("operations");
        if (ops_it == obj.end() || !ops_it->second.is_array()) return 0;
        
        const auto& ops = *ops_it->second.get<Value::Array>();
        size_t imported = 0;
        
        for (const auto& op_val : ops) {
            if (!op_val.is_object()) continue;
            
            const auto& op_obj = *op_val.get<Value::Object>();
            
            auto type_it = op_obj.find("type");
            if (type_it == op_obj.end() || !type_it->second.is_string()) continue;
            
            std::string type = *type_it->second.get<std::string>();
            
            if (type == "create") {
                imported += import_create(op_obj, force);
            } else if (type == "update") {
                imported += import_update(op_obj, force);
            } else if (type == "delete") {
                imported += import_delete(op_obj, force);
            }
        }
        
        return imported;
    }
    
    // ==================== PERSISTENCE ====================
    
    void save_to_disk() const {
        if (!config_.persistence_path) return;
        
        std::ofstream file(*config_.persistence_path, std::ios::binary);
        if (!file) return;
        
        // Write header
        uint32_t magic = 0x54494E59;  // "TINY"
        uint32_t version = VERSION_MAJOR;
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        
        // Write item count
        uint64_t count = items_.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        // Write items (simplified - would need proper serialization)
        ReadLock lock(mutex_);
        for (const auto& [id, item] : items_) {
            // Write ID length + ID
            uint32_t id_len = id.size();
            file.write(reinterpret_cast<const char*>(&id_len), sizeof(id_len));
            file.write(id.data(), id_len);
            
            // Write type length + type
            uint32_t type_len = item->type.size();
            file.write(reinterpret_cast<const char*>(&type_len), sizeof(type_len));
            file.write(item->type.data(), type_len);
            
            // Write timestamps
            file.write(reinterpret_cast<const char*>(&item->created), sizeof(item->created));
            file.write(reinterpret_cast<const char*>(&item->modified), sizeof(item->modified));
            
            // Write data (simplified - would need proper serialization)
            // For production, use a library like msgpack or protobuf
        }
    }
    
    void load_from_disk() {
        if (!config_.persistence_path || 
            !std::filesystem::exists(*config_.persistence_path)) return;
        
        std::ifstream file(*config_.persistence_path, std::ios::binary);
        if (!file) return;
        
        // Read and validate header
        uint32_t magic, version;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        
        if (magic != 0x54494E59 || version > VERSION_MAJOR) return;
        
        // Read item count
        uint64_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        WriteLock lock(mutex_);
        
        for (uint64_t i = 0; i < count; i++) {
            // Read ID
            uint32_t id_len;
            file.read(reinterpret_cast<char*>(&id_len), sizeof(id_len));
            std::string id(id_len, '\0');
            file.read(id.data(), id_len);
            
            // Read type
            uint32_t type_len;
            file.read(reinterpret_cast<char*>(&type_len), sizeof(type_len));
            std::string type(type_len, '\0');
            file.read(type.data(), type_len);
            
            // Read timestamps
            Timestamp created, modified;
            file.read(reinterpret_cast<char*>(&created), sizeof(created));
            file.read(reinterpret_cast<char*>(&modified), sizeof(modified));
            
            auto item = std::make_shared<Item>(type);
            item->id = id;
            item->created = created;
            item->modified = modified;
            
            // Read data (simplified)
            // Would need to read serialized data
            
            items_[id] = item;
        }
    }
    
    void mark_dirty() {
        std::unique_lock lock(persistence_mutex_);
        persistence_dirty_ = true;
        persistence_cv_.notify_one();
    }
    
    // ==================== STATISTICS ====================
    
    Stats get_stats() const { return stats_; }
    
    size_t size() const {
        ReadLock lock(mutex_);
        return items_.size();
    }
    
    size_t journal_size() const {
        return journal_.size();
    }
    
    // ==================== DEBUG ====================
    
    void dump(std::ostream& os = std::cout) const {
        ReadLock lock(mutex_);
        
        os << "Tinyset+ v" << VERSION << " Store\n";
        os << "Items: " << items_.size() << "\n";
        os << "Journal: " << journal_.size() << " ops\n";
        os << "Types:\n";
        
        for (const auto& [type, ids] : type_index_) {
            os << "  " << type << ": " << ids.size() << "\n";
        }
        
        os << "Stats:\n";
        os << "  Reads: " << stats_.reads << "\n";
        os << "  Writes: " << stats_.writes << "\n";
        os << "  Queries: " << stats_.queries << "\n";
        os << "  Transactions: " << stats_.transactions << "\n";
        os << "  Cache hits: " << stats_.cache_hits << "\n";
        os << "  Cache misses: " << stats_.cache_misses << "\n";
    }
    
private:
    // ==================== IMPLEMENTATION ====================
    
    std::shared_ptr<Item> get_item(const std::string& id) const {
        ReadLock lock(mutex_);
        auto it = items_.find(id);
        return it != items_.end() ? it->second : nullptr;
    }
    
    void set_impl(const std::string& id, const std::string& field, 
                  Value value, FieldType expected) {
        
        // Type validation
        if (expected != FieldType::Any && value.type() != expected) {
            if (config_.strict_mode) {
                throw TypeMismatchException("Field " + field + " expects " + 
                                           std::to_string(static_cast<int>(expected)));
            } else {
                std::cerr << "Warning: Type mismatch for field " << field << std::endl;
            }
        }
        
        auto item = get_item(id);
        if (!item) throw NotFoundException("Item not found: " + id);
        
        auto old_item = std::make_shared<Item>(*item);
        Value old_value = item->get(field);
        
        WriteLock lock(mutex_);
        
        // Update value
        item->set(field, std::move(value));
        
        // Update indexes
        update_indexes_for_field(item, field, old_value, item->get(field));
        
        // Record operation
        Value::Object changes;
        changes[field] = item->get(field);
        record_operation("update", id, item->type, std::move(changes));
        
        lock.unlock();
        
        emit("update", id, field);
        emit("change", Value::Object{{"type", "update"}, {"id", id}});
        
        mark_dirty();
    }
    
    void update_indexes(const std::string& action, const std::shared_ptr<Item>& item) {
        // Type index
        if (action == "add") {
            type_index_[item->type].insert(item->id);
        } else if (action == "remove") {
            type_index_[item->type].erase(item->id);
        }
        
        // Value indexes
        if (action == "add") {
            for (const auto& [field, value] : item->data) {
                value_index_[field][value].insert(item->id);
            }
        } else if (action == "remove") {
            for (const auto& [field, value] : item->data) {
                auto fit = value_index_.find(field);
                if (fit != value_index_.end()) {
                    fit->second[value].erase(item->id);
                }
            }
        }
        
        // Spatial index
        if (spatial_index_) {
            if (action == "add") {
                auto x = item->get("x");
                auto y = item->get("y");
                if (x.is_number() && y.is_number()) {
                    if (auto xd = x.to_double()) {
                        if (auto yd = y.to_double()) {
                            spatial_index_->insert(item->id, *xd, *yd);
                        }
                    }
                }
            } else if (action == "remove") {
                auto x = item->get("x");
                auto y = item->get("y");
                if (x.is_number() && y.is_number()) {
                    if (auto xd = x.to_double()) {
                        if (auto yd = y.to_double()) {
                            spatial_index_->remove(item->id, *xd, *yd);
                        }
                    }
                }
            }
        }
    }
    
    void update_indexes_for_field(const std::shared_ptr<Item>& item, 
                                  const std::string& field,
                                  const Value& old_value,
                                  const Value& new_value) {
        // Update value index
        if (!old_value.is_null()) {
            value_index_[field][old_value].erase(item->id);
        }
        if (!new_value.is_null()) {
            value_index_[field][new_value].insert(item->id);
        }
        
        // Update spatial index if x or y changed
        if (spatial_index_ && (field == "x" || field == "y")) {
            auto x = item->get("x");
            auto y = item->get("y");
            if (x.is_number() && y.is_number()) {
                if (auto xd = x.to_double()) {
                    if (auto yd = y.to_double()) {
                        spatial_index_->update(item->id, *xd, *yd);
                    }
                }
            }
        }
    }
    
    void record_operation(const std::string& type, const std::string& item_id,
                          const std::string& item_type, Value::Object changes) {
        increment_clock();
        
        Operation op;
        op.id = config_.id_generator();
        op.process_id = config_.process_id;
        op.vector_clock = snapshot_clock();
        op.type = type;
        op.item_id = item_id;
        op.item_type = item_type;
        op.changes = std::move(changes);
        op.timestamp = Item::now();
        
        journal_.append(std::move(op));
    }
    
    void increment_clock() {
        std::unique_lock lock(clock_mutex_);
        vector_clock_[config_.process_id]++;
    }
    
    std::map<ProcessID, uint64_t> snapshot_clock() const {
        std::shared_lock lock(clock_mutex_);
        return vector_clock_;
    }
    
    void merge_clock(const Value::Object& clock) {
        std::unique_lock lock(clock_mutex_);
        for (const auto& [pid, time_val] : clock) {
            if (time_val.is_int()) {
                uint64_t time = *time_val.get<int64_t>();
                vector_clock_[pid] = std::max(vector_clock_[pid], time);
            }
        }
    }
    
    void commit_transaction(uint64_t id) {
        WriteLock lock(mutex_);
        
        std::erase_if(transaction_stack_, [id](const auto& tx) {
            return tx->id() == id;
        });
        
        emit("transaction", "commit", id);
    }
    
    void rollback_transaction(uint64_t id) {
        WriteLock lock(mutex_);
        
        auto it = std::find_if(transaction_stack_.begin(), transaction_stack_.end(),
            [id](const auto& tx) { return tx->id() == id; });
        
        if (it != transaction_stack_.end()) {
            const auto& records = (*it)->records();
            
            // Apply rollback in reverse order
            for (auto rit = records.rbegin(); rit != records.rend(); ++rit) {
                switch (rit->type) {
                    case Transaction::Record::CREATE:
                        items_.erase(rit->id);
                        break;
                    case Transaction::Record::UPDATE:
                        if (rit->old_item) {
                            items_[rit->id] = rit->old_item;
                        }
                        break;
                    case Transaction::Record::DELETE:
                        if (rit->new_item) {
                            items_[rit->id] = rit->new_item;
                        }
                        break;
                }
            }
            
            transaction_stack_.erase(it);
        }
        
        emit("transaction", "rollback", id);
    }
    
    void validate_against_schema(const std::string& type, const Value::Object& data) const {
        auto sit = schemas_.find(type);
        if (sit == schemas_.end()) return;
        
        const auto& schema = sit->second;
        schema.validate_all(data);
    }
    
    void apply_defaults(const std::string& type, Value::Object& data) const {
        auto sit = schemas_.find(type);
        if (sit == schemas_.end()) return;
        
        for (const auto& [field, def] : sit->second.fields()) {
            if (!data.contains(field) && def.default_value) {
                data[field] = *def.default_value;
            }
        }
    }
    
    bool matches_condition(const std::shared_ptr<Item>& item,
                          const std::string& field,
                          const Query::Condition& cond) const {
        Value val = item->get(field);
        
        if (cond.gt) {
            auto num = val.to_double();
            if (!num || *num <= *cond.gt) return false;
        }
        if (cond.lt) {
            auto num = val.to_double();
            if (!num || *num >= *cond.lt) return false;
        }
        if (cond.gte) {
            auto num = val.to_double();
            if (!num || *num < *cond.gte) return false;
        }
        if (cond.lte) {
            auto num = val.to_double();
            if (!num || *num > *cond.lte) return false;
        }
        if (cond.eq) {
            if (val.to_string() != *cond.eq) return false;
        }
        if (cond.ne) {
            if (val.to_string() == *cond.ne) return false;
        }
        if (cond.contains) {
            if (val.to_string().find(*cond.contains) == std::string::npos) return false;
        }
        if (cond.starts_with) {
            auto s = val.to_string();
            if (s.rfind(*cond.starts_with, 0) != 0) return false;
        }
        if (cond.ends_with) {
            auto s = val.to_string();
            if (s.size() < cond.ends_with->size() ||
                s.substr(s.size() - cond.ends_with->size()) != *cond.ends_with) {
                return false;
            }
        }
        if (cond.in) {
            bool found = false;
            for (const auto& v : *cond.in) {
                if (val.to_string() == v.to_string()) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        if (cond.nin) {
            for (const auto& v : *cond.nin) {
                if (val.to_string() == v.to_string()) return false;
            }
        }
        if (cond.exists) {
            if (*cond.exists != item->has(field)) return false;
        }
        
        return true;
    }
    
    void sort_results(std::vector<std::string>& ids, const Query& query) const {
        const auto& fields = query.sort_fields();
        const auto& descending = query.sort_descending();
        
        std::sort(ids.begin(), ids.end(),
            [&](const std::string& a, const std::string& b) {
                auto it_a = items_.find(a);
                auto it_b = items_.find(b);
                if (it_a == items_.end() || it_b == items_.end()) return false;
                
                for (size_t i = 0; i < fields.size(); i++) {
                    Value va = it_a->second->get(fields[i]);
                    Value vb = it_b->second->get(fields[i]);
                    
                    int cmp = compare_values(va, vb);
                    if (cmp != 0) {
                        return descending.size() > i && descending[i] ? cmp > 0 : cmp < 0;
                    }
                }
                return false;
            });
    }
    
    int compare_values(const Value& a, const Value& b) const {
        // Try numeric comparison first
        auto an = a.to_double();
        auto bn = b.to_double();
        if (an && bn) {
            if (*an < *bn) return -1;
            if (*an > *bn) return 1;
            return 0;
        }
        
        // Fall back to string comparison
        auto as = a.to_string();
        auto bs = b.to_string();
        if (as < bs) return -1;
        if (as > bs) return 1;
        return 0;
    }
    
    size_t import_create(const Value::Object& op, bool force) {
        auto id_it = op.find("itemId");
        auto type_it = op.find("itemType");
        if (id_it == op.end() || type_it == op.end()) return 0;
        
        if (!id_it->second.is_string() || !type_it->second.is_string()) return 0;
        
        std::string id = *id_it->second.get<std::string>();
        std::string type = *type_it->second.get<std::string>();
        
        // Check if exists and vector clock
        if (!force && items_.contains(id)) {
            // Check causality - would need proper CRDT merge
            return 0;
        }
        
        create(type, {}, id);
        return 1;
    }
    
    size_t import_update(const Value::Object& op, bool force) {
        auto id_it = op.find("itemId");
        auto changes_it = op.find("changes");
        if (id_it == op.end() || changes_it == op.end()) return 0;
        
        if (!id_it->second.is_string() || !changes_it->second.is_object()) return 0;
        
        std::string id = *id_it->second.get<std::string>();
        
        if (!force && !items_.contains(id)) return 0;
        
        const auto& changes = *changes_it->second.get<Value::Object>();
        for (const auto& [field, value] : changes) {
            set(id, field, value);
        }
        
        return 1;
    }
    
    size_t import_delete(const Value::Object& op, bool force) {
        auto id_it = op.find("itemId");
        if (id_it == op.end() || !id_it->second.is_string()) return 0;
        
        std::string id = *id_it->second.get<std::string>();
        
        if (!force && !items_.contains(id)) return 0;
        
        return remove(id) ? 1 : 0;
    }
    
    void start_persistence_thread() {
        persistence_thread_ = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {
                {
                    std::unique_lock lock(persistence_mutex_);
                    persistence_cv_.wait_for(lock, config_.persistence_interval, 
                        [this] { return persistence_dirty_; });
                }
                
                if (st.stop_requested()) break;
                
                save_to_disk();
                
                {
                    std::unique_lock lock(persistence_mutex_);
                    persistence_dirty_ = false;
                }
            }
        });
    }
};

// ==================== TYPED REFERENCE ====================

template<typename T>
class TypedRef {
    std::shared_ptr<Store> store_;
    std::string id_;
    
public:
    TypedRef(std::shared_ptr<Store> store, std::string id)
        : store_(std::move(store)), id_(std::move(id)) {}
    
    std::optional<T> get() const {
        return store_->get_as<T>(id_);
    }
    
    template<typename F>
    void update(F&& f) {
        auto tx = store_->begin_transaction();
        auto val = get();
        if (val) {
            auto new_val = f(*val);
            // Would need to save back
        }
        tx->commit();
    }
    
    void remove() {
        store_->remove(id_);
    }
    
    const std::string& id() const { return id_; }
};

// ==================== FACTORY FUNCTION ====================

inline std::shared_ptr<Store> create_store(Config config = {}) {
    return std::make_shared<Store>(std::move(config));
}

// ==================== VALUE IMPLEMENTATIONS ====================

std::string Value::type_name() const {
    switch (type_) {
        case FieldType::Null: return "null";
        case FieldType::Bool: return "bool";
        case FieldType::Int8: return "int8";
        case FieldType::Int16: return "int16";
        case FieldType::Int32: return "int32";
        case FieldType::Int64: return "int64";
        case FieldType::Uint8: return "uint8";
        case FieldType::Uint16: return "uint16";
        case FieldType::Uint32: return "uint32";
        case FieldType::Uint64: return "uint64";
        case FieldType::Float: return "float";
        case FieldType::Double: return "double";
        case FieldType::String: return "string";
        case FieldType::Binary: return "binary";
        case FieldType::Array: return "array";
        case FieldType::Object: return "object";
        case FieldType::Any: return "any";
        case FieldType::Timestamp: return "timestamp";
        case FieldType::Date: return "date";
        case FieldType::Geographic: return "geo";
        default: return "unknown";
    }
}

std::string Value::to_string() const {
    struct Visitor {
        std::string operator()(Null) const { return "null"; }
        std::string operator()(Bool b) const { return b ? "true" : "false"; }
        std::string operator()(Int8 i) const { return std::to_string(i); }
        std::string operator()(Int16 i) const { return std::to_string(i); }
        std::string operator()(Int32 i) const { return std::to_string(i); }
        std::string operator()(Int64 i) const { return std::to_string(i); }
        std::string operator()(Uint8 i) const { return std::to_string(i); }
        std::string operator()(Uint16 i) const { return std::to_string(i); }
        std::string operator()(Uint32 i) const { return std::to_string(i); }
        std::string operator()(Uint64 i) const { return std::to_string(i); }
        std::string operator()(Float f) const { return std::to_string(f); }
        std::string operator()(Double d) const { return std::to_string(d); }
        std::string operator()(const String& s) const { return "\"" + s + "\""; }
        std::string operator()(const Binary& b) const { 
            return "<binary " + std::to_string(b.size()) + " bytes>"; 
        }
        std::string operator()(const Array& a) const {
            std::string r = "[";
            for (size_t i = 0; i < a.size(); i++) {
                if (i > 0) r += ",";
                r += a[i].to_string();
            }
            return r + "]";
        }
        std::string operator()(const Object& o) const {
            std::string r = "{";
            bool first = true;
            for (const auto& [k, v] : o) {
                if (!first) r += ",";
                r += "\"" + k + "\":" + v.to_string();
                first = false;
            }
            return r + "}";
        }
    };
    return std::visit(Visitor{}, data_);
}

std::vector<uint8_t> Value::to_binary() const {
    if (auto p = get_if<Binary>()) return *p;
    if (auto p = get_if<String>()) {
        return std::vector<uint8_t>(p->begin(), p->end());
    }
    return {};
}

bool Value::operator==(const Value& other) const {
    if (type_ != other.type_) {
        // Try numeric conversion for comparison
        auto n1 = to_double();
        auto n2 = other.to_double();
        if (n1 && n2) return *n1 == *n2;
        return false;
    }
    
    return data_ == other.data_;
}

// ==================== RELOP IMPLEMENTATIONS ====================

Value RelOp::apply(const Value& current) const {
    switch (op_) {
        case Op::Add: {
            if (auto num = current.to_double()) {
                return Value(*num + amount_);
            }
            break;
        }
        case Op::Subtract: {
            if (auto num = current.to_double()) {
                return Value(*num - amount_);
            }
            break;
        }
        case Op::Multiply: {
            if (auto num = current.to_double()) {
                return Value(*num * amount_);
            }
            break;
        }
        case Op::Divide: {
            if (auto num = current.to_double()) {
                if (amount_ != 0) {
                    return Value(*num / amount_);
                }
            }
            break;
        }
        case Op::Mod: {
            if (auto num = current.to_double()) {
                return Value(std::fmod(*num, amount_));
            }
            break;
        }
        case Op::Append: {
            std::string s = current.is_string() ? *current.get_if<String>() : "";
            s += str_amount_;
            return Value(std::move(s));
        }
        case Op::Prepend: {
            std::string s = str_amount_;
            if (current.is_string()) {
                s += *current.get_if<String>();
            }
            return Value(std::move(s));
        }
    }
    
    throw TypeMismatchException("Cannot apply operation to value");
}

// ==================== SCHEMA IMPLEMENTATIONS ====================

void Schema::validate(const std::string& field, const Value& value) const {
    auto it = fields_.find(field);
    if (it == fields_.end()) return;
    
    const auto& def = it->second;
    
    // Type check
    if (def.type != FieldType::Any && value.type() != def.type) {
        throw TypeMismatchException("Field " + field + " expects type " + 
                                   std::to_string(static_cast<int>(def.type)));
    }
    
    // Enum check
    if (def.enum_values) {
        bool found = false;
        for (const auto& ev : *def.enum_values) {
            if (value.to_string() == ev) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw ConstraintViolationException("Value not in enum for field " + field);
        }
    }
    
    // Range check for numbers
    if (def.range && value.is_number()) {
        auto num = value.to_double();
        if (num) {
            if (*num < def.range->first || *num > def.range->second) {
                throw ConstraintViolationException("Value out of range for field " + field);
            }
        }
    }
    
    // Length checks for strings
    if (value.is_string()) {
        auto s = *value.get_if<Value::String>();
        if (def.min_length && s.size() < *def.min_length) {
            throw ConstraintViolationException("String too short for field " + field);
        }
        if (def.max_length && s.size() > *def.max_length) {
            throw ConstraintViolationException("String too long for field " + field);
        }
        if (def.pattern) {
            // Would need regex library
        }
    }
}

void Schema::validate_all(const Value::Object& obj) const {
    for (const auto& [field, def] : fields_) {
        auto it = obj.find(field);
        if (it == obj.end()) {
            if (def.required && !def.default_value) {
                throw ConstraintViolationException("Required field missing: " + field);
            }
        } else {
            validate(field, it->second);
        }
    }
}

} // namespace tinyset
