#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(istream &input);

Node LoadArray(istream &input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']') {
        throw ParsingError("Failed to read Array from stream"s);
    }
    return Node(move(result));
}

using Number = std::variant<int, double>;

Node LoadNumber(std::istream &input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream &input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(s);
}

Node LoadDict(istream &input) {
    Dict result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }
        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    if (c != '}') {
        throw ParsingError("Failed to read Dict from stream"s);
    }
    return Node(move(result));
}

Node LoadNullOrBool(istream &input) {
    // char tmp_c;
    std::string tmp_str, clean_str;
    for (char c; input >> c;) {
        if (c == ']' || c == '}' || c == ',') {
            input.putback(c);
            break;
        }
        switch (c) {
        case '\\':
            break;
        case '\n':
            break;
        case '\t':
            break;
        case '\r':
            break;
        case '\"':
            break;
        case ' ':
            break;
        default:
            clean_str += c;
        }
    }
    if (clean_str == "true"s) {
        return Node(true);
    } else if (clean_str == "false"s) {
        return Node(false);
    } else if (clean_str == "null"s) {
        return Node();
    } else {
        throw ParsingError("Null Or Bool value is not valid");
    }
    return Node();
}

Node LoadNode(istream &input) {
    char c;
    input >> c;
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (isdigit(c) || c == '-' || c == '+' || c == 'E' ||
               c == 'e') {
        input.putback(c);
        return LoadNumber(input);
    } else if (c == 't' || c == 'f' || c == 'n') {
        input.putback(c);
        return LoadNullOrBool(input);
    } else {
        throw ParsingError("Not valid pattern");
    }
}

} // namespace
Node::Node() {}

Node::Node(std::nullptr_t) : value_(nullptr) {}

Node::Node(int value)
    : value_(value) {
}

Node::Node(bool value)
    : value_(value) {
}

Node::Node(double value)
    : value_(value) {
}

Node::Node(std::string value)
    : value_(move(value)) {
}

Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

int Node::AsInt() const {
    if (!(IsInt())) {
        throw std::logic_error(""s);
    }
    return std::get<int>(value_);
}

bool Node::AsBool() const {
    if (!(IsBool())) {
        throw std::logic_error(""s);
    }
    return std::get<bool>(value_);
}

double Node::AsDouble() const {
    if (IsInt()) {
        return static_cast<double>(std::get<int>(value_));
    } else if (!(IsDouble())) {
        throw std::logic_error(""s);
    }
    return std::get<double>(value_);
}

const string &Node::AsString() const {
    if (!(IsString())) {
        throw std::logic_error(""s);
    }
    return std::get<std::string>(value_);
}

const Array &Node::AsArray() const {
    if (!(IsArray())) {
        throw std::logic_error(""s);
    }
    return std::get<json::Array>(value_);
}

const Dict &Node::AsDict() const {
    if (!(IsMap())) {
        throw std::logic_error(""s);
    }
    return std::get<json::Dict>(value_);
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
    return std::holds_alternative<int>(value_) || std::holds_alternative<double>(value_);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(value_);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(value_);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(value_);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(value_);
}

bool Node::IsArray() const {
    return std::holds_alternative<json::Array>(value_);
}

bool Node::IsMap() const {
    return std::holds_alternative<json::Dict>(value_);
}

Document::Document(Node root)
    : root_(move(root)) {
}

Node::Value &Node::GetValue() {
    return value_;
}

const Node::Value &Node::GetValue() const {
    return value_;
}

const Node &Document::GetRoot() const {
    return root_;
}

Document Load(istream &input) {
    return Document{LoadNode(input)};
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
void PrintContext::PrintIndent() const {
    for (int i = 0; i < indent; ++i) {
        out.put(' ');
    }
}

PrintContext PrintContext::Indented() const {
    return {out, indent_step, indent_step + indent};
}

void Print(const Document &doc, std::ostream &output) {
    (void)&doc;
    (void)&output;
    PrintContext ctx(output, 4, 0);
    PrintNode(doc.GetRoot(), ctx);
}

void PrintValue(const std::string &str, const PrintContext &ctx) {
    ctx.out << "\"";
    for (const auto &c : str) {
        if (c == '\\') {
            ctx.out << "\\\\"s;
        } else if (c == '\"') {
            ctx.out << "\\\""s;
        } else if (c == '\r') {
            ctx.out << "\\r"s;
        } else if (c == '\n') {
            ctx.out << "\\n"s;
        } else if (c == '\t') {
            ctx.out << "\\t"s;
        } else {
            ctx.out << c;
        }
    }
    ctx.out << "\"";
}

void PrintValue(const std::nullptr_t&, const PrintContext &ctx) {
    ctx.out << "null"s;
}

void PrintValue(const Array& array, const PrintContext &ctx) {
    auto ctx_a = ctx.Indented();
    bool is_first = true;
    ctx.out << "["s << '\n';
    for (const auto &item : array) {
        if (!is_first) {
            ctx.out << "," << '\n';
        }
        ctx_a.PrintIndent();
        PrintNode(item, ctx_a);
        is_first = false;
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << "]"s;
}

void PrintValue(const Dict& dict, const PrintContext &ctx) {
    auto ctx_a = ctx.Indented();
    bool is_first = true;
    std::string tmp_str;
    ctx.out << "{"s;
    for (const auto &item : dict) {
        if (!is_first) {
            ctx.out << ","s;
        }
        ctx.out << '\n';
        ctx_a.PrintIndent();
        ctx.out << "\""s;
        ctx.out << item.first << "\": "s;
        PrintNode(item.second, ctx_a);
        is_first = false;
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << "}"s;
}

void PrintValue(const bool& b, const PrintContext &ctx) {
    ctx.out << std::boolalpha << b;
}

void PrintNode(const Node &node, const PrintContext &ctx) {
    std::visit(
        [&ctx](const auto &value) { PrintValue(value, ctx); },
        node.GetValue());
}

bool operator==(const json::Node &node1, const json::Node &node2) {
    return node1.GetValue() == node2.GetValue();
}

bool operator!=(const json::Node &node1, const json::Node &node2) {
    return node1.GetValue() != node2.GetValue();
}

bool operator==(const json::Document &doc1, const json::Document &doc2) {
    return doc1.GetRoot() == doc2.GetRoot();
}

bool operator!=(const json::Document &doc1, const json::Document &doc2) {
    return doc1.GetRoot() != doc2.GetRoot();
}

} // namespace json