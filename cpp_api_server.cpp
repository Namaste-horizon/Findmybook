#include "cpp_api_server.hpp"

#include "cpp_api_shared.hpp"
#include "user.hpp"

#if __has_include(<crow.h>) && __has_include(<crow/middlewares/cors.h>)
#define FINDMYBOOK_HAS_CROW 1
#include <crow.h>
#include <crow/middlewares/cors.h>
#else
#define FINDMYBOOK_HAS_CROW 0
#endif

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#if !FINDMYBOOK_HAS_CROW
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using namespace cpp_api;

namespace {
#if FINDMYBOOK_HAS_CROW
using App = crow::App<crow::CORSHandler>;
#endif
std::mutex api_mutex;

HttpResponse handle_people_list(const std::string& role) {
    authsystem auth;
    return {200, "application/json", "{\"items\":" + people_array_json(auth.loadrolepeople(role)) + "}"};
}

HttpResponse handle_signup(const HttpRequest& request) {
    auto body = parse_json_object(request.body);
    authsystem auth;
    person created_person;
    std::string error;
    std::string requested_code;

    if (body["role"] == "lib") {
        std::vector<LibraryRecord> libraries = load_libraries();
        const LibraryRecord* managed_library = find_library_by_id(libraries, body["library_id"]);
        if (managed_library == nullptr) {
            return json_error(400, "library_id is required for library accounts");
        }
        requested_code = "l" + std::to_string(managed_library->id);
    }

    if (!auth.createaccountrecord(body["role"],
                                  body["fname"],
                                  body["lname"],
                                  body["email"],
                                  body["password"],
                                  body["admin_key"],
                                  created_person,
                                  error,
                                  requested_code)) {
        int status = error == "email already registered" ? 409 :
                     error == "code already assigned" ? 409 :
                     error == "invalid admin key" ? 403 :
                     400;
        return json_error(status, error);
    }

    return json_message(201, body["role"] + " created successfully", "person", person_json(created_person));
}

HttpResponse handle_login(const HttpRequest& request) {
    auto body = parse_json_object(request.body);
    authsystem auth;
    person matched_person;
    std::string error;

    if (!auth.loginrecord(body["role"],
                          body["email"],
                          body["password"],
                          body["admin_key"],
                          matched_person,
                          error)) {
        int status = 400;
        if (error == "invalid admin key") {
            status = 403;
        } else if (error == "invalid credentials") {
            status = 401;
        } else if (error.find("account not found") != std::string::npos) {
            status = 404;
        }
        return json_error(status, error);
    }

    return json_message(200, "login success", "person", person_json(matched_person));
}

HttpResponse handle_books_get(const HttpRequest& request) {
    std::vector<BookRecord> books = load_books();
    std::string query = to_lower_copy(trim(request.query.count("q") ? request.query.at("q") : ""));
    std::string library_id_filter = trim(request.query.count("library_id") ? request.query.at("library_id") : "");
    std::string library_filter = to_lower_copy(trim(request.query.count("library") ? request.query.at("library") : ""));
    std::vector<std::string> items;

    for (const BookRecord& book : books) {
        if (!library_id_filter.empty() && book.library_id != library_id_filter) {
            continue;
        }
        if (!library_filter.empty() && to_lower_copy(book.library) != library_filter) {
            continue;
        }
        if (!query.empty()) {
            std::string haystack = to_lower_copy(book.title + " " + book.author + " " + book.library + " " + book.genre);
            if (haystack.find(query) == std::string::npos) {
                continue;
            }
        }
        items.push_back(book_json(book));
    }

    return {200, "application/json", list_wrapper_json("items", items)};
}

HttpResponse handle_book_create(const HttpRequest& request) {
    auto body = parse_json_object(request.body);
    std::vector<BookRecord> books = load_books();
    std::vector<LibraryRecord> libraries = load_libraries();

    if (trim(body["title"]).empty() || trim(body["author"]).empty()) {
        return json_error(400, "title and author are required");
    }

    std::string library_id = trim(body["library_id"]);
    std::string library_name = trim(body["library"]);
    const LibraryRecord* selected_library = nullptr;
    if (!library_id.empty()) {
        selected_library = find_library_by_id(libraries, library_id);
    }
    if (selected_library == nullptr && !library_name.empty()) {
        selected_library = find_library_by_name(libraries, library_name);
    }
    if (selected_library == nullptr) {
        selected_library = find_library_by_id(libraries, "lib-1");
    }

    BookRecord created{
        next_book_id(books),
        trim(body["title"]),
        trim(body["author"]),
        trim(body["genre"]),
        selected_library != nullptr ? library_public_id(selected_library->id) : "lib-1",
        selected_library != nullptr ? selected_library->name : "Central Library",
        parse_int_or(body["total_copies"], 1),
        parse_int_or(body["available_copies"], parse_int_or(body["total_copies"], 1)),
    };

    if (created.genre.empty()) {
        created.genre = "General";
    }
    if (created.total_copies < 1) {
        created.total_copies = 1;
    }
    if (created.available_copies < 0) {
        created.available_copies = 0;
    }
    if (created.available_copies > created.total_copies) {
        created.available_copies = created.total_copies;
    }

    treenode* root = build_book_tree(books);
    treenode* node = createnode(created.id,
                                created.library.c_str(),
                                created.title.c_str(),
                                created.author.c_str(),
                                created.total_copies);
    if (node == nullptr) {
        free_bst(root);
        return json_error(500, "failed to create book");
    }

    node->available_copies = created.available_copies;
    root = insert(root, node);
    books = books_from_tree(root);
    merge_book_metadata(books, load_books());
    books.push_back(created);
    for (size_t index = 0; index + 1 < books.size(); ++index) {
        if (books[index].id == created.id) {
            books.erase(books.begin() + static_cast<long>(index));
            break;
        }
    }
    free_bst(root);

    if (!save_books(books)) {
        return json_error(500, "failed to save books");
    }

    return json_message(201, "book created", "item", book_json(created));
}

HttpResponse handle_book_update(const std::string& public_id, const HttpRequest& request) {
    int book_id = parse_public_id(public_id, "book");
    if (book_id <= 0) {
        return json_error(404, "book not found");
    }

    auto body = parse_json_object(request.body);
    std::vector<BookRecord> books = load_books();
    treenode* root = build_book_tree(books);
    treenode* node = search_id(root, book_id);
    if (node == nullptr) {
        free_bst(root);
        return json_error(404, "book not found");
    }

    std::string title = trim(body["title"].empty() ? node->title : body["title"]);
    std::string author = trim(body["author"].empty() ? node->author : body["author"]);
    std::string genre = trim(body["genre"]);
    std::vector<LibraryRecord> libraries = load_libraries();
    std::string library_id = trim(body["library_id"]);
    std::string library = trim(body["library"].empty() ? node->lib : body["library"]);
    int total_copies = parse_int_or(body["total_copies"], node->total_copies);
    int available_copies = parse_int_or(body["available_copies"], node->available_copies);

    if (title.empty() || author.empty()) {
        free_bst(root);
        return json_error(400, "title and author are required");
    }

    if (library_id.empty()) {
        for (const BookRecord& current_book : books) {
            if (current_book.id == book_id) {
                library_id = current_book.library_id;
                if (library.empty()) {
                    library = current_book.library;
                }
                break;
            }
        }
    }

    const LibraryRecord* selected_library = !library_id.empty()
        ? find_library_by_id(libraries, library_id)
        : (!library.empty() ? find_library_by_name(libraries, library) : nullptr);
    if (selected_library != nullptr) {
        library_id = library_public_id(selected_library->id);
        library = selected_library->name;
    }
    if (library.empty()) {
        library = "Central Library";
        library_id = "lib-1";
    }
    if (genre.empty()) {
        for (const BookRecord& current_book : books) {
            if (current_book.id == book_id) {
                genre = current_book.genre;
                break;
            }
        }
    }
    if (genre.empty()) {
        genre = "General";
    }
    if (total_copies < 1) {
        total_copies = 1;
    }
    if (available_copies < 0) {
        available_copies = 0;
    }
    if (available_copies > total_copies) {
        available_copies = total_copies;
    }

    edit(node, author.c_str(), title.c_str(), available_copies, total_copies);
    std::strncpy(node->lib, library.c_str(), sizeof(node->lib) - 1);
    node->lib[sizeof(node->lib) - 1] = '\0';

    std::vector<BookRecord> original_books = books;
    books = books_from_tree(root);
    merge_book_metadata(books, original_books);
    free_bst(root);

    for (BookRecord& current_book : books) {
        if (current_book.id == book_id) {
            current_book.genre = genre;
            current_book.library_id = library_id;
            current_book.library = library;
            current_book.title = title;
            current_book.author = author;
            current_book.total_copies = total_copies;
            current_book.available_copies = available_copies;
            break;
        }
    }

    BookRecord updated{book_id, title, author, genre, library_id, library, total_copies, available_copies};
    std::vector<BookingRecord> bookings = load_bookings();
    sync_book_references(updated, bookings);

    if (!save_books(books)) {
        return json_error(500, "failed to save books");
    }
    if (!save_bookings(bookings)) {
        return json_error(500, "failed to sync linked bookings");
    }

    return json_message(200, "book updated", "item", book_json(updated));
}

HttpResponse handle_book_delete(const std::string& public_id) {
    int book_id = parse_public_id(public_id, "book");
    if (book_id <= 0) {
        return json_error(404, "book not found");
    }

    if (book_has_active_rentals(book_id, load_bookings())) {
        return json_error(409, "return the active rental before deleting this book");
    }

    std::vector<BookRecord> books = load_books();
    treenode* root = build_book_tree(books);
    treenode* node = search_id(root, book_id);
    if (node == nullptr) {
        free_bst(root);
        return json_error(404, "book not found");
    }

    std::string removed_genre = "General";
    std::string removed_library_id = "lib-1";
    for (const BookRecord& current_book : books) {
        if (current_book.id == node->book_id) {
            removed_genre = current_book.genre;
            removed_library_id = current_book.library_id;
            break;
        }
    }

    BookRecord removed{
        node->book_id,
        node->title,
        node->author,
        removed_genre,
        removed_library_id,
        node->lib,
        node->total_copies,
        node->available_copies,
    };

    root = deletenode(root, book_id);
    books = books_from_tree(root);
    merge_book_metadata(books, load_books());
    free_bst(root);

    if (!save_books(books)) {
        return json_error(500, "failed to save books");
    }

    return json_message(200, "book deleted", "item", book_json(removed));
}

HttpResponse handle_libraries_get() {
    std::vector<std::string> items;
    for (const LibraryRecord& library : load_libraries()) {
        items.push_back(library_json(library));
    }
    return {200, "application/json", list_wrapper_json("items", items)};
}

HttpResponse handle_libraries_nearby(const HttpRequest& request) {
    if (!request.query.count("lat") || !request.query.count("lng")) {
        return json_error(400, "lat and lng are required");
    }

    double lat = parse_double_or(request.query.at("lat"), 0.0);
    double lng = parse_double_or(request.query.at("lng"), 0.0);
    std::vector<std::pair<LibraryRecord, double>> ranked =
        rank_libraries_by_distance(lat, lng, load_libraries());

    std::vector<std::string> items;
    for (const auto& entry : ranked) {
        items.push_back(
            "{"
            "\"id\":\"" + library_public_id(entry.first.id) + "\","
            "\"name\":\"" + entry.first.name + "\","
            "\"city\":\"" + entry.first.city + "\","
            "\"lat\":" + format_double(entry.first.lat) + ","
            "\"lng\":" + format_double(entry.first.lng) + ","
            "\"total_seats\":" + std::to_string(entry.first.total_seats) + ","
            "\"distance_km\":" + format_double(entry.second) +
            "}");
    }

    return {200, "application/json", list_wrapper_json("items", items)};
}

HttpResponse handle_library_create(const HttpRequest& request) {
    auto body = parse_json_object(request.body);
    std::vector<LibraryRecord> libraries = load_libraries();

    std::string name = trim(body["name"]);
    if (name.empty()) {
        return json_error(400, "name, lat, and lng are required");
    }

    LibraryRecord library{
        next_library_id(libraries),
        name,
        trim(body["city"]),
        parse_double_or(body["lat"], 0.0),
        parse_double_or(body["lng"], 0.0),
        std::max(parse_int_or(body["total_seats"], 20), 1),
    };

    libraries.push_back(library);
    if (!save_libraries(libraries)) {
        return json_error(500, "failed to save libraries");
    }

    return json_message(201, "library created", "item", library_json(library));
}

HttpResponse handle_library_update(const std::string& public_id, const HttpRequest& request) {
    int library_id = parse_public_id(public_id, "lib");
    std::vector<LibraryRecord> libraries = load_libraries();
    auto body = parse_json_object(request.body);

    for (LibraryRecord& library : libraries) {
        if (library.id != library_id) {
            continue;
        }

        if (!trim(body["name"]).empty()) {
            library.name = trim(body["name"]);
        }
        if (body.count("city")) {
            library.city = trim(body["city"]);
        }
        if (body.count("lat")) {
            library.lat = parse_double_or(body["lat"], library.lat);
        }
        if (body.count("lng")) {
            library.lng = parse_double_or(body["lng"], library.lng);
        }
        if (body.count("total_seats")) {
            library.total_seats = std::max(parse_int_or(body["total_seats"], library.total_seats), 1);
        }

        std::vector<BookRecord> books = load_books();
        std::vector<BookingRecord> bookings = load_bookings();
        sync_library_references(library, books, bookings);

        if (!save_books(books)) {
            return json_error(500, "failed to sync linked books");
        }
        if (!save_bookings(bookings)) {
            return json_error(500, "failed to sync linked bookings");
        }
        if (!save_libraries(libraries)) {
            return json_error(500, "failed to save libraries");
        }

        return json_message(200, "library updated", "item", library_json(library));
    }

    return json_error(404, "library not found");
}

HttpResponse handle_library_delete(const std::string& public_id) {
    int library_id = parse_public_id(public_id, "lib");
    std::vector<LibraryRecord> libraries = load_libraries();
    std::vector<BookRecord> books = load_books();
    std::vector<BookingRecord> bookings = load_bookings();

    for (size_t index = 0; index < libraries.size(); ++index) {
        if (libraries[index].id != library_id) {
            continue;
        }

        if (library_has_linked_records(public_id, books, bookings)) {
            return json_error(409, "remove linked books and bookings before deleting this library");
        }

        LibraryRecord removed = libraries[index];
        libraries.erase(libraries.begin() + static_cast<long>(index));
        if (!save_libraries(libraries)) {
            return json_error(500, "failed to save libraries");
        }

        return json_message(200, "library deleted", "item", library_json(removed));
    }

    return json_error(404, "library not found");
}

HttpResponse handle_bookings_get(const HttpRequest& request) {
    std::string email = to_lower_copy(trim(request.query.count("email") ? request.query.at("email") : ""));
    std::string library_id = trim(request.query.count("library_id") ? request.query.at("library_id") : "");
    std::string library_name = to_lower_copy(trim(request.query.count("library") ? request.query.at("library") : ""));
    std::string user_role = trim(request.query.count("user_role") ? request.query.at("user_role") : "");
    std::vector<std::string> items;

    for (const BookingRecord& booking : load_bookings()) {
        if (!email.empty() && to_lower_copy(booking.user_email) != email) {
            continue;
        }
        if (!library_id.empty() && booking.library_id != library_id) {
            continue;
        }
        if (!library_name.empty() && to_lower_copy(booking.library) != library_name) {
            continue;
        }
        if (!user_role.empty() && booking.user_role != user_role) {
            continue;
        }
        items.push_back(booking_json(booking));
    }

    return {200, "application/json", list_wrapper_json("items", items)};
}

HttpResponse handle_booking_get(const std::string& public_id) {
    int booking_id = parse_public_id(public_id, "booking");
    for (const BookingRecord& booking : load_bookings()) {
        if (booking.id == booking_id) {
            return {200, "application/json", "{\"item\":" + booking_json(booking) + "}"};
        }
    }
    return json_error(404, "booking not found");
}

HttpResponse handle_booking_create(const HttpRequest& request) {
    auto body = parse_json_object(request.body);
    std::string user_email = trim(body["user_email"]);
    std::string category = trim(body["category"]);
    std::vector<LibraryRecord> libraries = load_libraries();

    if (!is_valid_email(user_email)) {
        return json_error(400, "valid user_email is required");
    }
    if (category != "seat" && category != "rental") {
        return json_error(400, "category must be seat or rental");
    }

    std::vector<BookingRecord> bookings = load_bookings();
    BookingRecord created{};
    created.id = next_booking_id(bookings);
    created.user_email = user_email;
    created.user_name = trim(body["user_name"]);
    created.user_role = trim(body["user_role"].empty() ? "user" : body["user_role"]);
    created.category = category;
    created.title = trim(body["title"]);
    created.book_id = trim(body["book_id"]);
    created.library_id = trim(body["library_id"]);
    created.library = trim(body["library"]);
    created.date = trim(body["date"]);
    created.time = trim(body["time"]);
    created.status = trim(body["status"].empty() ? "active" : body["status"]);
    created.seat_number = 0;

    if (category == "seat") {
        const LibraryRecord* seat_library = !created.library_id.empty()
            ? find_library_by_id(libraries, created.library_id)
            : (!created.library.empty() ? find_library_by_name(libraries, created.library) : nullptr);

        if (seat_library != nullptr) {
            created.library_id = library_public_id(seat_library->id);
            created.library = seat_library->name;
        }

        if (created.library.empty() || created.date.empty() || created.time.empty() || created.library_id.empty()) {
            return json_error(400, "library, date, and time are required for seat bookings");
        }
        if (created.title.empty()) {
            created.title = "Study Seat";
        }

        const LibraryRecord* allocated_library = find_library_by_id(libraries, created.library_id);
        if (allocated_library == nullptr) {
            return json_error(400, "selected library was not found");
        }

        created.seat_number = greedy_allocate_seat(bookings,
                                                   *allocated_library,
                                                   created.library_id,
                                                   created.date,
                                                   created.time);
        if (created.seat_number == 0) {
            return json_error(409, "no study seats are available for the selected slot");
        }
    } else {
        if (created.title.empty()) {
            return json_error(400, "title is required for rental bookings");
        }

        int book_id = parse_public_id(created.book_id, "book");
        if (book_id <= 0) {
            return json_error(400, "book_id is required for rental bookings");
        }

        std::vector<BookRecord> books = load_books();
        std::vector<BookRecord> original_books = books;
        treenode* root = build_book_tree(books);
        User renter("Web User", user_email, "");
        if (!renter.issueBookById(root, book_id)) {
            free_bst(root);
            return json_error(400, "book is not available");
        }

        books = books_from_tree(root);
        merge_book_metadata(books, original_books);
        free_bst(root);
        if (!save_books(books)) {
            return json_error(500, "failed to save books");
        }

        for (const BookRecord& book : books) {
            if (book.id == book_id) {
                created.library_id = book.library_id;
                created.library = book.library;
                if (created.title.empty()) {
                    created.title = book.title;
                }
                break;
            }
        }
    }

    bookings.push_back(created);
    if (!save_bookings(bookings)) {
        return json_error(500, "failed to save bookings");
    }

    return json_message(201, "booking created", "item", booking_json(created));
}

HttpResponse handle_booking_update(const std::string& public_id, const HttpRequest& request) {
    int booking_id = parse_public_id(public_id, "booking");
    auto body = parse_json_object(request.body);
    std::vector<BookingRecord> bookings = load_bookings();

    for (BookingRecord& booking : bookings) {
        if (booking.id != booking_id) {
            continue;
        }

        std::string previous_status = booking.status;
        if (body.count("title")) {
            booking.title = trim(body["title"]);
        }
        if (body.count("library")) {
            booking.library = trim(body["library"]);
        }
        if (body.count("library_id")) {
            booking.library_id = trim(body["library_id"]);
        }
        if (body.count("date")) {
            booking.date = trim(body["date"]);
        }
        if (body.count("time")) {
            booking.time = trim(body["time"]);
        }
        if (body.count("status")) {
            booking.status = trim(body["status"]);
        }

        if (booking.status.empty()) {
            booking.status = "active";
        }
        if (booking.category == "seat" &&
            (booking.library.empty() || booking.date.empty() || booking.time.empty())) {
            return json_error(400, "library, date, and time are required for seat bookings");
        }
        if (booking.category == "rental" && booking.title.empty()) {
            return json_error(400, "title is required for rental bookings");
        }

        if (booking.category == "seat") {
            std::vector<LibraryRecord> libraries = load_libraries();
            const LibraryRecord* seat_library = !booking.library_id.empty()
                ? find_library_by_id(libraries, booking.library_id)
                : find_library_by_name(libraries, booking.library);
            if (seat_library == nullptr) {
                return json_error(400, "selected library was not found");
            }

            booking.library_id = library_public_id(seat_library->id);
            booking.library = seat_library->name;
            int new_seat = greedy_allocate_seat(bookings,
                                                *seat_library,
                                                booking.library_id,
                                                booking.date,
                                                booking.time,
                                                booking.id);
            if (new_seat == 0) {
                return json_error(409, "no study seats are available for the selected slot");
            }
            booking.seat_number = new_seat;
        }

        if (booking.category == "rental" && !booking.book_id.empty()) {
            bool was_active = previous_status != "returned" && previous_status != "cancelled";
            bool is_active = booking.status != "returned" && booking.status != "cancelled";
            int book_id = parse_public_id(booking.book_id, "book");

            if (book_id > 0 && was_active != is_active) {
                std::vector<BookRecord> books = load_books();
                std::vector<BookRecord> original_books = books;
                treenode* root = build_book_tree(books);
                User renter("Web User", booking.user_email, "");

                if (was_active && !is_active) {
                    renter.returnBookById(root, book_id);
                } else if (!was_active && is_active) {
                    if (!renter.issueBookById(root, book_id)) {
                        free_bst(root);
                        return json_error(409, "book is not available");
                    }
                }

                books = books_from_tree(root);
                merge_book_metadata(books, original_books);
                free_bst(root);
                if (!save_books(books)) {
                    return json_error(500, "failed to save books");
                }

                for (const BookRecord& book : books) {
                    if (book.id == book_id) {
                        if (booking.title.empty()) {
                            booking.title = book.title;
                        }
                        booking.library_id = book.library_id;
                        booking.library = book.library;
                        break;
                    }
                }
            }
        }

        if (!save_bookings(bookings)) {
            return json_error(500, "failed to save bookings");
        }

        return json_message(200, "booking updated", "item", booking_json(booking));
    }

    return json_error(404, "booking not found");
}

HttpResponse handle_booking_delete(const std::string& public_id) {
    int booking_id = parse_public_id(public_id, "booking");
    std::vector<BookingRecord> bookings = load_bookings();

    for (size_t index = 0; index < bookings.size(); ++index) {
        if (bookings[index].id != booking_id) {
            continue;
        }

        BookingRecord removed = bookings[index];
        if (removed.category == "rental" && !removed.book_id.empty()) {
            int book_id = parse_public_id(removed.book_id, "book");
            if (book_id > 0) {
                std::vector<BookRecord> books = load_books();
                std::vector<BookRecord> original_books = books;
                treenode* root = build_book_tree(books);
                User renter("Web User", removed.user_email, "");
                renter.returnBookById(root, book_id);
                books = books_from_tree(root);
                merge_book_metadata(books, original_books);
                free_bst(root);
                save_books(books);
            }
        }

        bookings.erase(bookings.begin() + static_cast<long>(index));
        if (!save_bookings(bookings)) {
            return json_error(500, "failed to save bookings");
        }

        return json_message(200, "booking deleted", "item", booking_json(removed));
    }

    return json_error(404, "booking not found");
}

[[maybe_unused]] HttpResponse route_request(const HttpRequest& request) {
    if (request.method == "OPTIONS") {
        return options_response();
    }

    if (request.method == "GET" && request.path == "/api/health") {
        return {200, "application/json", "{\"status\":\"ok\",\"backend\":\"cpp\"}"};
    }
    if (request.method == "GET" && request.path == "/api/admins") {
        return handle_people_list("admin");
    }
    if (request.method == "GET" && request.path == "/api/users") {
        return handle_people_list("user");
    }
    if (request.method == "GET" && request.path == "/api/library-accounts") {
        return handle_people_list("lib");
    }
    if (request.method == "POST" && request.path == "/api/signup") {
        return handle_signup(request);
    }
    if (request.method == "POST" && request.path == "/api/login") {
        return handle_login(request);
    }
    if (request.method == "GET" && request.path == "/api/books") {
        return handle_books_get(request);
    }
    if (request.method == "POST" && request.path == "/api/books") {
        return handle_book_create(request);
    }
    if (request.path.rfind("/api/books/", 0) == 0) {
        std::string id = request.path.substr(std::string("/api/books/").size());
        if (request.method == "PUT") {
            return handle_book_update(id, request);
        }
        if (request.method == "DELETE") {
            return handle_book_delete(id);
        }
    }
    if (request.method == "GET" && request.path == "/api/libraries") {
        return handle_libraries_get();
    }
    if (request.method == "GET" && request.path == "/api/libraries/nearby") {
        return handle_libraries_nearby(request);
    }
    if (request.method == "POST" && request.path == "/api/libraries") {
        return handle_library_create(request);
    }
    if (request.path.rfind("/api/libraries/", 0) == 0) {
        std::string id = request.path.substr(std::string("/api/libraries/").size());
        if (request.method == "PUT") {
            return handle_library_update(id, request);
        }
        if (request.method == "DELETE") {
            return handle_library_delete(id);
        }
    }
    if (request.method == "GET" && request.path == "/api/bookings") {
        return handle_bookings_get(request);
    }
    if (request.method == "POST" && request.path == "/api/bookings") {
        return handle_booking_create(request);
    }
    if (request.path.rfind("/api/bookings/", 0) == 0) {
        std::string id = request.path.substr(std::string("/api/bookings/").size());
        if (request.method == "GET") {
            return handle_booking_get(id);
        }
        if (request.method == "PUT") {
            return handle_booking_update(id, request);
        }
        if (request.method == "DELETE") {
            return handle_booking_delete(id);
        }
    }

    return json_error(404, "route not found");
}

#if FINDMYBOOK_HAS_CROW
HttpRequest make_request(const crow::request& req) {
    HttpRequest request;
    request.method = crow::method_name(req.method);
    request.target = req.raw_url;
    request.path = req.url;
    request.body = req.body;

    for (const std::string& key : req.url_params.keys()) {
        const char* value = req.url_params.get(key);
        request.query[key] = value == nullptr ? "" : value;
    }

    return request;
}

crow::response make_response(const HttpResponse& response) {
    crow::response result(response.status);
    result.set_header("Content-Type", response.content_type);
    result.write(response.body);
    return result;
}

template <typename Handler>
crow::response guard_api(Handler&& handler) {
    std::lock_guard<std::mutex> lock(api_mutex);
    return make_response(handler());
}

void configure_cors(App& app) {
    app.get_middleware<crow::CORSHandler>()
        .global()
        .origin("*")
        .headers("Content-Type")
        .methods(crow::HTTPMethod::Get,
                 crow::HTTPMethod::Post,
                 crow::HTTPMethod::Put,
                 crow::HTTPMethod::Delete,
                 crow::HTTPMethod::Options);
}

void register_routes(App& app) {
    CROW_ROUTE(app, "/api/health")
    ([] {
        return guard_api([] {
            return HttpResponse{200, "application/json", "{\"status\":\"ok\",\"backend\":\"cpp\",\"framework\":\"crow\"}"};
        });
    });

    CROW_ROUTE(app, "/api/admins")
    ([] {
        return guard_api([] {
            return handle_people_list("admin");
        });
    });

    CROW_ROUTE(app, "/api/users")
    ([] {
        return guard_api([] {
            return handle_people_list("user");
        });
    });

    CROW_ROUTE(app, "/api/library-accounts")
    ([] {
        return guard_api([] {
            return handle_people_list("lib");
        });
    });

    CROW_ROUTE(app, "/api/signup")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_signup(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/login")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_login(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_books_get(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_book_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return handle_book_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return handle_book_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/libraries")
        .methods(crow::HTTPMethod::Get)
    ([] {
        return guard_api([] {
            return handle_libraries_get();
        });
    });

    CROW_ROUTE(app, "/api/libraries/nearby")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_libraries_nearby(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_library_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return handle_library_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return handle_library_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/bookings")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_bookings_get(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return handle_booking_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Get)
    ([](const std::string& id) {
        return guard_api([&] {
            return handle_booking_get(id);
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return handle_booking_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return handle_booking_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/<path>")
        .methods(crow::HTTPMethod::Options)
    ([](const std::string&) {
        return guard_api([] {
            return options_response();
        });
    });
}
#else
int run_socket_server(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create server socket: " << std::strerror(errno) << "\n";
        return 1;
    }

    int enabled = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(server_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind server socket: " << std::strerror(errno) << "\n";
        close(server_socket);
        return 1;
    }

    if (::listen(server_socket, 16) < 0) {
        std::cerr << "Failed to listen on server socket: " << std::strerror(errno) << "\n";
        close(server_socket);
        return 1;
    }

    std::cout << "C++ API server running on http://127.0.0.1:" << port
              << " (Crow headers not found, using built-in socket server)\n";

    while (true) {
        int client_socket = ::accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            continue;
        }

        HttpRequest request;
        HttpResponse response;
        {
            std::lock_guard<std::mutex> lock(api_mutex);
            if (read_http_request(client_socket, request)) {
                response = route_request(request);
            } else {
                response = json_error(400, "bad request");
            }
        }

        send_http_response(client_socket, response);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}
#endif

}  // namespace

int run_cpp_api_server(int port) {
#if FINDMYBOOK_HAS_CROW
    App app;
    app.loglevel(crow::LogLevel::Warning);
    configure_cors(app);
    register_routes(app);

    std::cout << "C++ API server running with Crow on http://127.0.0.1:" << port << "\n";
    app.port(static_cast<uint16_t>(port)).run();
    return 0;
#else
    return run_socket_server(port);
#endif
}
