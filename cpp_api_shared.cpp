#include "cpp_api_shared.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <netinet/in.h>
#include <queue>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpp_api {
namespace {

const char* BOOKS_FILE = "books.json";
const char* LIBRARIES_FILE = "libraries.json";
const char* BOOKINGS_FILE = "bookings.json";

std::string read_file(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return "";
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool write_file(const std::string& path, const std::string& content) {
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << content;
    return true;
}

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

std::string url_decode(const std::string& input) {
    std::string output;
    output.reserve(input.size());

    for (size_t index = 0; index < input.size(); ++index) {
        if (input[index] == '%' && index + 2 < input.size()) {
            std::string hex = input.substr(index + 1, 2);
            char decoded = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            output += decoded;
            index += 2;
        } else if (input[index] == '+') {
            output += ' ';
        } else {
            output += input[index];
        }
    }

    return output;
}

std::unordered_map<std::string, std::string> parse_query_string(const std::string& query_string) {
    std::unordered_map<std::string, std::string> values;
    std::stringstream stream(query_string);
    std::string part;

    while (std::getline(stream, part, '&')) {
        if (part.empty()) {
            continue;
        }

        size_t equals = part.find('=');
        std::string key = equals == std::string::npos ? part : part.substr(0, equals);
        std::string value = equals == std::string::npos ? "" : part.substr(equals + 1);
        values[url_decode(key)] = url_decode(value);
    }

    return values;
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

std::vector<std::string> extract_json_objects(const std::string& content) {
    std::vector<std::string> objects;
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    size_t object_start = std::string::npos;

    for (size_t index = 0; index < content.size(); ++index) {
        char ch = content[index];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (ch == '\\') {
            escaped = true;
            continue;
        }

        if (ch == '"') {
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            continue;
        }

        if (ch == '{') {
            if (depth == 0) {
                object_start = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && object_start != std::string::npos) {
                objects.push_back(content.substr(object_start, index - object_start + 1));
                object_start = std::string::npos;
            }
        }
    }

    return objects;
}

std::vector<BookRecord> default_books() {
    return {
        {1, "Data Structures", "Mark Allen", "Computer Science", "lib-1", "Central Library", 5, 5},
        {2, "Algorithms", "Thomas H. Cormen", "Computer Science", "lib-1", "Central Library", 4, 4},
        {3, "Artificial Intelligence", "Stuart Russell", "AI", "lib-2", "City Library", 3, 3},
    };
}

std::vector<LibraryRecord> default_libraries() {
    return {
        {1, "Central Library", "Dehradun", 30.3165, 78.0322, 60},
        {2, "City Library", "Dehradun", 30.31, 78.02, 40},
    };
}

std::string serialize_books(const std::vector<BookRecord>& books) {
    std::ostringstream output;
    output << "[\n";
    for (size_t index = 0; index < books.size(); ++index) {
        output << "  " << book_json(books[index]);
        if (index + 1 < books.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]\n";
    return output.str();
}

std::string serialize_libraries(const std::vector<LibraryRecord>& libraries) {
    std::ostringstream output;
    output << "[\n";
    for (size_t index = 0; index < libraries.size(); ++index) {
        output << "  " << library_json(libraries[index]);
        if (index + 1 < libraries.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]\n";
    return output.str();
}

std::string serialize_bookings(const std::vector<BookingRecord>& bookings) {
    std::ostringstream output;
    output << "[\n";
    for (size_t index = 0; index < bookings.size(); ++index) {
        output << "  " << booking_json(bookings[index]);
        if (index + 1 < bookings.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]\n";
    return output.str();
}

int role_code_index(const std::string& code, char prefix) {
    if (code.size() < 2 || code[0] != prefix) {
        return -1;
    }
    return parse_int_or(code.substr(1), -1);
}

double degrees_to_radians(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

double haversine_km(double first_lat,
                    double first_lng,
                    double second_lat,
                    double second_lng) {
    const double earth_radius_km = 6371.0;
    double lat_delta = degrees_to_radians(second_lat - first_lat);
    double lng_delta = degrees_to_radians(second_lng - first_lng);
    double start_lat = degrees_to_radians(first_lat);
    double end_lat = degrees_to_radians(second_lat);

    double a = std::sin(lat_delta / 2.0) * std::sin(lat_delta / 2.0) +
               std::cos(start_lat) * std::cos(end_lat) *
               std::sin(lng_delta / 2.0) * std::sin(lng_delta / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return earth_radius_km * c;
}

const char* reason_phrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 409: return "Conflict";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

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
    std::ostringstream output;
    output << "[";
    for (size_t index = 0; index < people.size(); ++index) {
        if (index > 0) {
            output << ",";
        }
        output << person_json(people[index]);
    }
    output << "]";
    return output.str();
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

std::vector<BookRecord> load_books() {
    std::string content = read_file(BOOKS_FILE);
    if (content.empty()) {
        std::vector<BookRecord> books = default_books();
        write_file(BOOKS_FILE, serialize_books(books));
        return books;
    }

    std::vector<LibraryRecord> libraries = load_libraries();
    std::vector<BookRecord> books;
    for (const std::string& object_text : extract_json_objects(content)) {
        auto values = parse_json_object(object_text);
        BookRecord book{};
        book.id = parse_public_id(values["id"], "book");
        if (book.id <= 0) {
            continue;
        }
        book.title = values["title"];
        book.author = values["author"];
        book.genre = values["genre"].empty() ? "General" : values["genre"];
        book.library_id = values["library_id"];
        book.library = values["library"];
        if (book.library.empty() && !book.library_id.empty()) {
            const LibraryRecord* library = find_library_by_id(libraries, book.library_id);
            if (library != nullptr) {
                book.library = library->name;
            }
        }
        if (book.library_id.empty() && !book.library.empty()) {
            const LibraryRecord* library = find_library_by_name(libraries, book.library);
            if (library != nullptr) {
                book.library_id = library_public_id(library->id);
            }
        }
        if (book.library_id.empty()) {
            book.library_id = "lib-1";
        }
        if (book.library.empty()) {
            const LibraryRecord* library = find_library_by_id(libraries, book.library_id);
            book.library = library != nullptr ? library->name : "Central Library";
        }
        int fallback_total = 1;
        int fallback_available = values.count("available") && to_lower_copy(values["available"]) == "false" ? 0 : 1;
        book.total_copies = parse_int_or(values["total_copies"], fallback_total);
        book.available_copies = parse_int_or(values["available_copies"], fallback_available);
        books.push_back(book);
    }

    if (books.empty()) {
        books = default_books();
        write_file(BOOKS_FILE, serialize_books(books));
    }

    return books;
}

std::vector<LibraryRecord> load_libraries() {
    std::string content = read_file(LIBRARIES_FILE);
    if (content.empty()) {
        std::vector<LibraryRecord> libraries = default_libraries();
        write_file(LIBRARIES_FILE, serialize_libraries(libraries));
        return libraries;
    }

    std::vector<LibraryRecord> libraries;
    for (const std::string& object_text : extract_json_objects(content)) {
        auto values = parse_json_object(object_text);
        LibraryRecord library{};
        library.id = parse_public_id(values["id"], "lib");
        if (library.id <= 0) {
            continue;
        }
        library.name = values["name"];
        library.city = values["city"];
        library.lat = parse_double_or(values["lat"], 0.0);
        library.lng = parse_double_or(values["lng"], 0.0);
        library.total_seats = parse_int_or(values["total_seats"], 20);
        libraries.push_back(library);
    }

    if (libraries.empty()) {
        libraries = default_libraries();
        write_file(LIBRARIES_FILE, serialize_libraries(libraries));
    }

    return libraries;
}

std::vector<BookingRecord> load_bookings() {
    std::string content = read_file(BOOKINGS_FILE);
    if (content.empty()) {
        write_file(BOOKINGS_FILE, "[]\n");
        return {};
    }

    std::vector<BookingRecord> bookings;
    for (const std::string& object_text : extract_json_objects(content)) {
        auto values = parse_json_object(object_text);
        BookingRecord booking{};
        booking.id = parse_public_id(values["id"], "booking");
        if (booking.id <= 0) {
            continue;
        }
        booking.user_email = values["user_email"];
        booking.user_name = values["user_name"];
        booking.user_role = values["user_role"].empty() ? "user" : values["user_role"];
        booking.category = values["category"];
        booking.title = values["title"];
        booking.book_id = values["book_id"];
        booking.library_id = values["library_id"];
        booking.library = values["library"];
        booking.date = values["date"];
        booking.time = values["time"];
        booking.status = values["status"];
        booking.seat_number = parse_int_or(values["seat_number"], 0);
        bookings.push_back(booking);
    }

    return bookings;
}

bool save_books(const std::vector<BookRecord>& books) {
    return write_file(BOOKS_FILE, serialize_books(books));
}

bool save_libraries(const std::vector<LibraryRecord>& libraries) {
    return write_file(LIBRARIES_FILE, serialize_libraries(libraries));
}

bool save_bookings(const std::vector<BookingRecord>& bookings) {
    return write_file(BOOKINGS_FILE, serialize_bookings(bookings));
}

int next_book_id(const std::vector<BookRecord>& books) {
    int highest = 0;
    for (const BookRecord& book : books) {
        if (book.id > highest) {
            highest = book.id;
        }
    }
    return highest + 1;
}

int next_library_id(const std::vector<LibraryRecord>& libraries) {
    int highest = 0;
    for (const LibraryRecord& library : libraries) {
        if (library.id > highest) {
            highest = library.id;
        }
    }
    return highest + 1;
}

int next_booking_id(const std::vector<BookingRecord>& bookings) {
    int highest = 0;
    for (const BookingRecord& booking : bookings) {
        if (booking.id > highest) {
            highest = booking.id;
        }
    }
    return highest + 1;
}

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

const LibraryRecord* find_library_by_id(const std::vector<LibraryRecord>& libraries,
                                        const std::string& library_id) {
    int parsed_id = parse_public_id(library_id, "lib");
    if (parsed_id <= 0) {
        return nullptr;
    }

    for (const LibraryRecord& library : libraries) {
        if (library.id == parsed_id) {
            return &library;
        }
    }

    return nullptr;
}

const LibraryRecord* find_library_by_name(const std::vector<LibraryRecord>& libraries,
                                          const std::string& library_name) {
    std::string lowered_target = to_lower_copy(trim(library_name));
    for (const LibraryRecord& library : libraries) {
        if (to_lower_copy(library.name) == lowered_target) {
            return &library;
        }
    }
    return nullptr;
}

void sync_library_references(const LibraryRecord& library,
                             std::vector<BookRecord>& books,
                             std::vector<BookingRecord>& bookings) {
    std::string public_library_id = library_public_id(library.id);
    for (BookRecord& book : books) {
        if (book.library_id == public_library_id) {
            book.library = library.name;
        }
    }

    for (BookingRecord& booking : bookings) {
        if (booking.library_id == public_library_id) {
            booking.library = library.name;
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

bool library_has_linked_records(const std::string& library_id,
                                const std::vector<BookRecord>& books,
                                const std::vector<BookingRecord>& bookings) {
    for (const BookRecord& book : books) {
        if (book.library_id == library_id) {
            return true;
        }
    }

    for (const BookingRecord& booking : bookings) {
        if (booking.library_id == library_id) {
            return true;
        }
    }

    return false;
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

std::vector<std::pair<LibraryRecord, double>> rank_libraries_by_distance(
    double origin_lat,
    double origin_lng,
    const std::vector<LibraryRecord>& libraries) {
    if (libraries.empty()) {
        return {};
    }

    std::vector<std::vector<std::pair<int, double>>> graph(libraries.size() + 1);
    const int source_index = static_cast<int>(libraries.size());

    for (size_t index = 0; index < libraries.size(); ++index) {
        double direct_distance = haversine_km(origin_lat, origin_lng, libraries[index].lat, libraries[index].lng);
        graph[source_index].push_back({static_cast<int>(index), direct_distance});

        for (size_t neighbor = index + 1; neighbor < libraries.size(); ++neighbor) {
            double weight = haversine_km(libraries[index].lat,
                                         libraries[index].lng,
                                         libraries[neighbor].lat,
                                         libraries[neighbor].lng);
            graph[index].push_back({static_cast<int>(neighbor), weight});
            graph[neighbor].push_back({static_cast<int>(index), weight});
        }
    }

    std::vector<double> distances(graph.size(), std::numeric_limits<double>::infinity());
    using QueueEntry = std::pair<double, int>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;

    distances[source_index] = 0.0;
    queue.push({0.0, source_index});

    while (!queue.empty()) {
        QueueEntry current = queue.top();
        queue.pop();

        if (current.first > distances[current.second]) {
            continue;
        }

        for (const auto& edge : graph[current.second]) {
            double candidate = current.first + edge.second;
            if (candidate < distances[edge.first]) {
                distances[edge.first] = candidate;
                queue.push({candidate, edge.first});
            }
        }
    }

    std::vector<std::pair<LibraryRecord, double>> ranked;
    for (size_t index = 0; index < libraries.size(); ++index) {
        ranked.push_back({libraries[index], distances[index]});
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const auto& first, const auto& second) {
                  return first.second < second.second;
              });
    return ranked;
}

int greedy_allocate_seat(const std::vector<BookingRecord>& bookings,
                         const LibraryRecord& library,
                         const std::string& library_id,
                         const std::string& date,
                         const std::string& time,
                         int ignore_booking_id) {
    std::vector<bool> occupied(static_cast<size_t>(std::max(library.total_seats, 0)) + 1, false);

    for (const BookingRecord& booking : bookings) {
        if (booking.id == ignore_booking_id ||
            booking.category != "seat" ||
            booking.library_id != library_id ||
            booking.date != date ||
            booking.time != time ||
            booking.status == "cancelled") {
            continue;
        }

        if (booking.seat_number > 0 && booking.seat_number < static_cast<int>(occupied.size())) {
            occupied[static_cast<size_t>(booking.seat_number)] = true;
        }
    }

    for (int seat_number = 1; seat_number <= library.total_seats; ++seat_number) {
        if (!occupied[static_cast<size_t>(seat_number)]) {
            return seat_number;
        }
    }

    return 0;
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
    std::ostringstream output;
    output << "{\"" << key << "\":[";
    for (size_t index = 0; index < items.size(); ++index) {
        if (index > 0) {
            output << ",";
        }
        output << items[index];
    }
    output << "]}";
    return output.str();
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

bool read_http_request(int client_socket, HttpRequest& request) {
    std::string raw;
    char buffer[4096];
    size_t header_end = std::string::npos;

    while ((header_end = raw.find("\r\n\r\n")) == std::string::npos) {
        ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            return false;
        }
        raw.append(buffer, static_cast<size_t>(received));
    }

    std::string header_part = raw.substr(0, header_end);
    std::istringstream header_stream(header_part);
    std::string request_line;
    if (!std::getline(header_stream, request_line)) {
        return false;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    std::string version;
    request_line_stream >> request.method >> request.target >> version;
    if (request.method.empty() || request.target.empty()) {
        return false;
    }

    std::string header_line;
    size_t content_length = 0;
    while (std::getline(header_stream, header_line)) {
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }

        size_t colon = header_line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = to_lower_copy(trim(header_line.substr(0, colon)));
        std::string value = trim(header_line.substr(colon + 1));
        request.headers[key] = value;
        if (key == "content-length") {
            content_length = static_cast<size_t>(parse_int_or(value, 0));
        }
    }

    size_t body_start = header_end + 4;
    while (raw.size() < body_start + content_length) {
        ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            return false;
        }
        raw.append(buffer, static_cast<size_t>(received));
    }
    request.body = raw.substr(body_start, content_length);

    size_t query_start = request.target.find('?');
    request.path = query_start == std::string::npos ? request.target : request.target.substr(0, query_start);
    if (query_start != std::string::npos) {
        request.query = parse_query_string(request.target.substr(query_start + 1));
    }

    return true;
}

void send_http_response(int client_socket, const HttpResponse& response) {
    std::ostringstream output;
    output << "HTTP/1.1 " << response.status << ' ' << reason_phrase(response.status) << "\r\n";
    output << "Content-Type: " << response.content_type << "\r\n";
    output << "Access-Control-Allow-Origin: *\r\n";
    output << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    output << "Access-Control-Allow-Headers: Content-Type\r\n";
    output << "Connection: close\r\n";
    output << "Content-Length: " << response.body.size() << "\r\n\r\n";
    output << response.body;

    const std::string payload = output.str();
    send(client_socket, payload.c_str(), payload.size(), 0);
}

}  // namespace cpp_api
