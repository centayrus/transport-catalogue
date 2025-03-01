#pragma once

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using namespace std::literals;

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


class Node : private std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
public:
    using variant::variant;
	using Value = variant;
    
    Node(Value value) : variant(std::move(value)) {}

    Node();
    Node(std::nullptr_t);
    Node(int value);
    Node(bool is_yn);
    Node(double value);
    Node(std::string value);
    Node(Array array);
    Node(Dict map);

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; // Возвращает значение типа double, если внутри хранится double либо int.
                             // В последнем случае возвращается приведённое в double значение.
    const std::string &AsString() const;
    const Array &AsArray() const;
    const Dict &AsDict() const;

    bool IsNull() const;
    bool IsInt() const;
    bool IsBool() const;
    bool IsDouble() const;     // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const; // Возвращает true, если в Node хранится double.
    bool IsString() const;
    bool IsArray() const;
    bool IsMap() const;

    Value &GetValue();
    const Value &GetValue() const;    

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);

    const Node &GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream &input);

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
 struct PrintContext {
    PrintContext(std::ostream& output, int step, int ind) : out(output), indent_step(step), indent(ind) {}
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const;

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const;
}; 

void Print(const Document &doc, std::ostream &out);

void PrintNode(const Node &node, const PrintContext& ctx);

inline void PrintValue(const std::string &str, const PrintContext& ctx);

// Шаблон, подходящий для вывода double и int
// Поскольку функция шаблонная, она остается в h-файле
template <typename Value>
void PrintValue(const Value &value, const PrintContext& ctx) {
    ctx.out << value;
}

// Если оставлять определение функций в заголовочном файле
// нужно объявить их как inline
void PrintValue(const std::nullptr_t&, const PrintContext& ctx);

void PrintValue(const Array& array, const PrintContext& ctx);

void PrintValue(const Dict& dict, const PrintContext& ctx);

void PrintValue(const bool& b, const PrintContext& ctx);

void PrintNode(const Node &node, std::ostream &out);

bool operator==(const json::Node &node1, const json::Node &node2);

bool operator!=(const json::Node &node1, const json::Node &node2);

bool operator==(const json::Document &doc1, const json::Document &doc2);

bool operator!=(const json::Document &doc1, const json::Document &doc2);

} // namespace json