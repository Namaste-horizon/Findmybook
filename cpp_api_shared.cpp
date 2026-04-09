#include "cpp_api_shared.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

namespace cpp_api {
namespace {

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

std::string json_string(const std::string& value) {
    return "\"" + json_escape(value) + "\"";
}

bool parse_json_string_token(const std::string& input, size_t& index, std::string& value) {
    if (index >= input.size() || input[index] != '"') {
        return false;
    }

    ++index;
    std::string result;
    while (index < input.size()) {
        char ch = input[index++];
        if (ch == '\\' && index < input.size()) {
            char escaped = input[index++];
            switch (escaped) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += escaped;
                    break;
            }
            continue;
        }

        if (ch == '"') {
            value = result;
            return true;
        }

        result += ch;
    }

    return false;
}

int role_code_index(const std::string& code, char prefix) {
    if (code.size() < 2 || code[0] != prefix) {
        return -1;
    }
    return parse_int_or(code.substr(1), -1);
}

}  // namespace

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string to_lower_copy(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::unordered_map<std::string, std::string> parse_json_object(const std::string& body) {
    std::unordered_map<std::string, std::string> values;
    size_t index = 0;

    while (index < body.size() && body[index] != '{') {
        ++index;
    }
    if (index < body.size() && body[index] == '{') {
        ++index;
    }

    while (index < body.size()) {
        while (index < body.size() &&
               (std::isspace(static_cast<unsigned char>(body[index])) || body[index] == ',')) {
            ++index;
        }

        if (index >= body.size() || body[index] == '}') {
            break;
        }

        std::string key;
        if (!parse_json_string_token(body, index, key)) {
            break;
        }

        while (index < body.size() &&
               (std::isspace(static_cast<unsigned char>(body[index])) || body[index] == ':')) {
            ++index;
        }

        if (index >= body.size()) {
            break;
        }

        std::string value;
        if (body[index] == '"') {
            if (!parse_json_string_token(body, index, value)) {
                break;
            }
        } else {
            size_t start = index;
            while (index < body.size() && body[index] != ',' && body[index] != '}') {
                ++index;
            }
            value = trim(body.substr(start, index - start));
        }

        values[key] = value;
    }

    return values;
}

int parse_int_or(const std::string& value, int fallback) {
    if (value.empty()) {
        return fallback;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

double parse_double_or(const std::string& value, double fallback) {
    if (value.empty()) {
        return fallback;
    }

    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

std::string format_double(double value) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(6) << value;
    std::string text = output.str();
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text.empty() ? "0" : text;
}

std::string person_json(const person& current_person) {
    return "{"
        "\"fname\":" + json_string(current_person.getfname()) + ","
        "\"lname\":" + json_string(current_person.getlname()) + ","
        "\"email\":" + json_string(current_person.getemail()) + ","
        "\"code\":" + json_string(current_person.getcode()) + ","
        "\"library_id\":" + json_string(library_id_from_role_code(current_person.getcode())) +
        "}";
}

std::string people_array_json(const std::vector<person>& people) {
    std::vector<std::string> items;
    for (const person& current_person : people) {
        items.push_back(person_json(current_person));
    }
    return json_array(items);
}

std::string book_json(const BookRecord& book) {
    bool available = book.available_copies > 0;
    return "{"
        "\"id\":" + json_string(book_public_id(book.id)) + ","
        "\"title\":" + json_string(book.title) + ","
        "\"author\":" + json_string(book.author) + ","
        "\"genre\":" + json_string(book.genre) + ","
        "\"library_id\":" + json_string(book.library_id) + ","
        "\"library\":" + json_string(book.library) + ","
        "\"total_copies\":" + std::to_string(book.total_copies) + ","
        "\"available_copies\":" + std::to_string(book.available_copies) + ","
        "\"available\":" + std::string(available ? "true" : "false") +
        "}";
}

std::string library_json(const LibraryRecord& library) {
    return "{"
        "\"id\":" + json_string(library_public_id(library.id)) + ","
        "\"name\":" + json_string(library.name) + ","
        "\"city\":" + json_string(library.city) + ","
        "\"lat\":" + format_double(library.lat) + ","
        "\"lng\":" + format_double(library.lng) + ","
        "\"total_seats\":" + std::to_string(library.total_seats) +
        "}";
}

std::string booking_json(const BookingRecord& booking) {
    return "{"
        "\"id\":" + json_string(booking_public_id(booking.id)) + ","
        "\"user_email\":" + json_string(booking.user_email) + ","
        "\"user_name\":" + json_string(booking.user_name) + ","
        "\"user_role\":" + json_string(booking.user_role) + ","
        "\"category\":" + json_string(booking.category) + ","
        "\"title\":" + json_string(booking.title) + ","
        "\"book_id\":" + json_string(booking.book_id) + ","
        "\"library_id\":" + json_string(booking.library_id) + ","
        "\"library\":" + json_string(booking.library) + ","
        "\"date\":" + json_string(booking.date) + ","
        "\"time\":" + json_string(booking.time) + ","
        "\"status\":" + json_string(booking.status) + ","
        "\"seat_number\":" + std::to_string(booking.seat_number) +
        "}";
}

std::string json_array(const std::vector<std::string>& items) {
    std::ostringstream output;
    output << "[";
    for (size_t index = 0; index < items.size(); ++index) {
        if (index > 0) {
            output << ",";
        }
        output << items[index];
    }
    output << "]";
    return output.str();
}

std::string book_public_id(int id) {
    return "book-" + std::to_string(id);
}

std::string library_public_id(int id) {
    return "lib-" + std::to_string(id);
}

std::string booking_public_id(int id) {
    return "booking-" + std::to_string(id);
}

int parse_public_id(const std::string& value, const std::string& prefix) {
    if (value.rfind(prefix + "-", 0) == 0) {
        return parse_int_or(value.substr(prefix.size() + 1), -1);
    }
    return parse_int_or(value, -1);
}

std::string library_id_from_role_code(const std::string& code) {
    int index = role_code_index(code, 'l');
    if (index <= 0) {
        return "";
    }
    return library_public_id(index);
}

namespace {

void collect_books_from_tree(treenode* root, std::vector<BookRecord>& books) {
    if (root == nullptr) {
        return;
    }

    collect_books_from_tree(root->left, books);
    books.push_back({
        root->book_id,
        root->title,
        root->author,
        "General",
        "",
        root->lib,
        root->total_copies,
        root->available_copies,
    });
    collect_books_from_tree(root->right, books);
}

}  // namespace

treenode* build_book_tree(const std::vector<BookRecord>& books) {
    treenode* root = nullptr;
    for (const BookRecord& book : books) {
        treenode* newnode = createnode(book.id,
                                       book.library.c_str(),
                                       book.title.c_str(),
                                       book.author.c_str(),
                                       book.total_copies);
        if (newnode == nullptr) {
            continue;
        }
        newnode->available_copies = book.available_copies;
        root = insert(root, newnode);
    }
    return root;
}

std::vector<BookRecord> books_from_tree(treenode* root) {
    std::vector<BookRecord> books;
    collect_books_from_tree(root, books);
    return books;
}

void merge_book_metadata(std::vector<BookRecord>& target_books,
                         const std::vector<BookRecord>& source_books) {
    std::unordered_map<int, BookRecord> source_by_id;
    for (const BookRecord& book : source_books) {
        source_by_id[book.id] = book;
    }

    for (BookRecord& book : target_books) {
        auto source = source_by_id.find(book.id);
        if (source == source_by_id.end()) {
            continue;
        }

        if (book.genre.empty() || book.genre == "General") {
            book.genre = source->second.genre;
        }
        if (book.library_id.empty()) {
            book.library_id = source->second.library_id;
        }
        if (book.library.empty() || book.library == "Central Library") {
            book.library = source->second.library;
        }
    }
}

void sync_book_references(const BookRecord& book, std::vector<BookingRecord>& bookings) {
    std::string public_book_id = book_public_id(book.id);
    for (BookingRecord& booking : bookings) {
        if (booking.book_id == public_book_id) {
            booking.title = book.title;
            booking.library_id = book.library_id;
            booking.library = book.library;
        }
    }
}

bool book_has_active_rentals(int book_id, const std::vector<BookingRecord>& bookings) {
    std::string public_book_id = book_public_id(book_id);
    for (const BookingRecord& booking : bookings) {
        if (booking.category == "rental" &&
            booking.book_id == public_book_id &&
            booking.status != "returned" &&
            booking.status != "cancelled") {
            return true;
        }
    }

    return false;
}

bool is_valid_email(const std::string& email) {
    size_t at_position = email.find('@');
    size_t dot_position = std::string::npos;
    if (at_position != std::string::npos) {
        dot_position = email.find('.', at_position);
    }

    return at_position != std::string::npos &&
           dot_position != std::string::npos &&
           at_position < dot_position;
}

std::string list_wrapper_json(const std::string& key,
                              const std::vector<std::string>& items) {
    return "{\"" + key + "\":" + json_array(items) + "}";
}

HttpResponse json_error(int status, const std::string& message) {
    HttpResponse response;
    response.status = status;
    response.body = "{\"error\":" + json_string(message) + "}";
    return response;
}

HttpResponse json_message(int status,
                          const std::string& message,
                          const std::string& key,
                          const std::string& payload) {
    HttpResponse response;
    response.status = status;
    std::ostringstream output;
    output << "{\"message\":" << json_string(message);
    if (!key.empty() && !payload.empty()) {
        output << ",\"" << key << "\":" << payload;
    }
    output << "}";
    response.body = output.str();
    return response;
}

HttpResponse options_response() {
    HttpResponse response;
    response.status = 204;
    response.body.clear();
    return response;
}

}  // namespace cpp_api
