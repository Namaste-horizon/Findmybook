#include "data.hpp"

#include <fstream>
#include <sstream>

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
    std::vector<std::string> items;
    for (const BookRecord& book : books) {
        items.push_back(book_json(book));
    }
    return json_array(items) + "\n";
}

std::string serialize_libraries(const std::vector<LibraryRecord>& libraries) {
    std::vector<std::string> items;
    for (const LibraryRecord& library : libraries) {
        items.push_back(library_json(library));
    }
    return json_array(items) + "\n";
}

std::string serialize_bookings(const std::vector<BookingRecord>& bookings) {
    std::vector<std::string> items;
    for (const BookingRecord& booking : bookings) {
        items.push_back(booking_json(booking));
    }
    return json_array(items) + "\n";
}

}  // namespace

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
        int fallback_available =
            values.count("available") && to_lower_copy(values["available"]) == "false" ? 0 : 1;
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

}  // namespace cpp_api
