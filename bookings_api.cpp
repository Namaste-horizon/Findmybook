#include "bookings_api.hpp"

#include "data.hpp"
#include "library_module.hpp"
#include "user.hpp"

#include <vector>

namespace api_handlers {

cpp_api::HttpResponse handle_bookings_get(const cpp_api::HttpRequest& request) {
    std::string email = cpp_api::to_lower_copy(cpp_api::trim(request.query.count("email") ? request.query.at("email") : ""));
    std::string library_id = cpp_api::trim(request.query.count("library_id") ? request.query.at("library_id") : "");
    std::string library_name = cpp_api::to_lower_copy(cpp_api::trim(request.query.count("library") ? request.query.at("library") : ""));
    std::string user_role = cpp_api::trim(request.query.count("user_role") ? request.query.at("user_role") : "");
    std::vector<std::string> items;

    for (const cpp_api::BookingRecord& booking : cpp_api::load_bookings()) {
        if (!email.empty() && cpp_api::to_lower_copy(booking.user_email) != email) {
            continue;
        }
        if (!library_id.empty() && booking.library_id != library_id) {
            continue;
        }
        if (!library_name.empty() && cpp_api::to_lower_copy(booking.library) != library_name) {
            continue;
        }
        if (!user_role.empty() && booking.user_role != user_role) {
            continue;
        }
        items.push_back(cpp_api::booking_json(booking));
    }

    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_booking_get(const std::string& public_id) {
    int booking_id = cpp_api::parse_public_id(public_id, "booking");
    for (const cpp_api::BookingRecord& booking : cpp_api::load_bookings()) {
        if (booking.id == booking_id) {
            return {200, "application/json", "{\"item\":" + cpp_api::booking_json(booking) + "}"};
        }
    }
    return cpp_api::json_error(404, "booking not found");
}

cpp_api::HttpResponse handle_booking_create(const cpp_api::HttpRequest& request) {
    auto body = cpp_api::parse_json_object(request.body);
    std::string user_email = cpp_api::trim(body["user_email"]);
    std::string category = cpp_api::trim(body["category"]);
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();

    if (!cpp_api::is_valid_email(user_email)) {
        return cpp_api::json_error(400, "valid user_email is required");
    }
    if (category != "seat" && category != "rental") {
        return cpp_api::json_error(400, "category must be seat or rental");
    }

    std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();
    cpp_api::BookingRecord created{};
    created.id = cpp_api::next_booking_id(bookings);
    created.user_email = user_email;
    created.user_name = cpp_api::trim(body["user_name"]);
    created.user_role = cpp_api::trim(body["user_role"].empty() ? "user" : body["user_role"]);
    created.category = category;
    created.title = cpp_api::trim(body["title"]);
    created.book_id = cpp_api::trim(body["book_id"]);
    created.library_id = cpp_api::trim(body["library_id"]);
    created.library = cpp_api::trim(body["library"]);
    created.date = cpp_api::trim(body["date"]);
    created.time = cpp_api::trim(body["time"]);
    created.status = cpp_api::trim(body["status"].empty() ? "active" : body["status"]);
    created.seat_number = 0;

    if (category == "seat") {
        const cpp_api::LibraryRecord* seat_library = !created.library_id.empty()
            ? cpp_api::find_library_by_id(libraries, created.library_id)
            : (!created.library.empty() ? cpp_api::find_library_by_name(libraries, created.library) : nullptr);

        if (seat_library != nullptr) {
            created.library_id = cpp_api::library_public_id(seat_library->id);
            created.library = seat_library->name;
        }

        if (created.library.empty() || created.date.empty() || created.time.empty() || created.library_id.empty()) {
            return cpp_api::json_error(400, "library, date, and time are required for seat bookings");
        }
        if (created.title.empty()) {
            created.title = "Study Seat";
        }

        const cpp_api::LibraryRecord* allocated_library = cpp_api::find_library_by_id(libraries, created.library_id);
        if (allocated_library == nullptr) {
            return cpp_api::json_error(400, "selected library was not found");
        }

        created.seat_number = cpp_api::greedy_allocate_seat(bookings,
                                                            *allocated_library,
                                                            created.library_id,
                                                            created.date,
                                                            created.time);
        if (created.seat_number == 0) {
            return cpp_api::json_error(409, "no study seats are available for the selected slot");
        }
    } else {
        if (created.title.empty()) {
            return cpp_api::json_error(400, "title is required for rental bookings");
        }

        int book_id = cpp_api::parse_public_id(created.book_id, "book");
        if (book_id <= 0) {
            return cpp_api::json_error(400, "book_id is required for rental bookings");
        }

        std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
        std::vector<cpp_api::BookRecord> original_books = books;
        treenode* root = cpp_api::build_book_tree(books);
        User renter("Web User", user_email, "");
        if (!renter.issueBookById(root, book_id)) {
            free_bst(root);
            return cpp_api::json_error(400, "book is not available");
        }

        books = cpp_api::books_from_tree(root);
        cpp_api::merge_book_metadata(books, original_books);
        free_bst(root);
        if (!cpp_api::save_books(books)) {
            return cpp_api::json_error(500, "failed to save books");
        }

        for (const cpp_api::BookRecord& book : books) {
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
    if (!cpp_api::save_bookings(bookings)) {
        return cpp_api::json_error(500, "failed to save bookings");
    }

    return cpp_api::json_message(201, "booking created", "item", cpp_api::booking_json(created));
}

cpp_api::HttpResponse handle_booking_update(const std::string& public_id,
                                            const cpp_api::HttpRequest& request) {
    int booking_id = cpp_api::parse_public_id(public_id, "booking");
    auto body = cpp_api::parse_json_object(request.body);
    std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();

    for (cpp_api::BookingRecord& booking : bookings) {
        if (booking.id != booking_id) {
            continue;
        }

        std::string previous_status = booking.status;
        if (body.count("title")) {
            booking.title = cpp_api::trim(body["title"]);
        }
        if (body.count("library")) {
            booking.library = cpp_api::trim(body["library"]);
        }
        if (body.count("library_id")) {
            booking.library_id = cpp_api::trim(body["library_id"]);
        }
        if (body.count("date")) {
            booking.date = cpp_api::trim(body["date"]);
        }
        if (body.count("time")) {
            booking.time = cpp_api::trim(body["time"]);
        }
        if (body.count("status")) {
            booking.status = cpp_api::trim(body["status"]);
        }

        if (booking.status.empty()) {
            booking.status = "active";
        }
        if (booking.category == "seat" &&
            (booking.library.empty() || booking.date.empty() || booking.time.empty())) {
            return cpp_api::json_error(400, "library, date, and time are required for seat bookings");
        }
        if (booking.category == "rental" && booking.title.empty()) {
            return cpp_api::json_error(400, "title is required for rental bookings");
        }

        if (booking.category == "seat") {
            std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
            const cpp_api::LibraryRecord* seat_library = !booking.library_id.empty()
                ? cpp_api::find_library_by_id(libraries, booking.library_id)
                : cpp_api::find_library_by_name(libraries, booking.library);
            if (seat_library == nullptr) {
                return cpp_api::json_error(400, "selected library was not found");
            }

            booking.library_id = cpp_api::library_public_id(seat_library->id);
            booking.library = seat_library->name;
            int new_seat = cpp_api::greedy_allocate_seat(bookings,
                                                         *seat_library,
                                                         booking.library_id,
                                                         booking.date,
                                                         booking.time,
                                                         booking.id);
            if (new_seat == 0) {
                return cpp_api::json_error(409, "no study seats are available for the selected slot");
            }
            booking.seat_number = new_seat;
        }

        if (booking.category == "rental" && !booking.book_id.empty()) {
            bool was_active = previous_status != "returned" && previous_status != "cancelled";
            bool is_active = booking.status != "returned" && booking.status != "cancelled";
            int book_id = cpp_api::parse_public_id(booking.book_id, "book");

            if (book_id > 0 && was_active != is_active) {
                std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
                std::vector<cpp_api::BookRecord> original_books = books;
                treenode* root = cpp_api::build_book_tree(books);
                User renter("Web User", booking.user_email, "");

                if (was_active && !is_active) {
                    renter.returnBookById(root, book_id);
                } else if (!was_active && is_active) {
                    if (!renter.issueBookById(root, book_id)) {
                        free_bst(root);
                        return cpp_api::json_error(409, "book is not available");
                    }
                }

                books = cpp_api::books_from_tree(root);
                cpp_api::merge_book_metadata(books, original_books);
                free_bst(root);
                if (!cpp_api::save_books(books)) {
                    return cpp_api::json_error(500, "failed to save books");
                }

                for (const cpp_api::BookRecord& book : books) {
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

        if (!cpp_api::save_bookings(bookings)) {
            return cpp_api::json_error(500, "failed to save bookings");
        }

        return cpp_api::json_message(200, "booking updated", "item", cpp_api::booking_json(booking));
    }

    return cpp_api::json_error(404, "booking not found");
}

cpp_api::HttpResponse handle_booking_delete(const std::string& public_id) {
    int booking_id = cpp_api::parse_public_id(public_id, "booking");
    std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();

    for (size_t index = 0; index < bookings.size(); ++index) {
        if (bookings[index].id != booking_id) {
            continue;
        }

        cpp_api::BookingRecord removed = bookings[index];
        if (removed.category == "rental" && !removed.book_id.empty()) {
            int book_id = cpp_api::parse_public_id(removed.book_id, "book");
            if (book_id > 0) {
                std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
                std::vector<cpp_api::BookRecord> original_books = books;
                treenode* root = cpp_api::build_book_tree(books);
                User renter("Web User", removed.user_email, "");
                renter.returnBookById(root, book_id);
                books = cpp_api::books_from_tree(root);
                cpp_api::merge_book_metadata(books, original_books);
                free_bst(root);
                cpp_api::save_books(books);
            }
        }

        bookings.erase(bookings.begin() + static_cast<long>(index));
        if (!cpp_api::save_bookings(bookings)) {
            return cpp_api::json_error(500, "failed to save bookings");
        }

        return cpp_api::json_message(200, "booking deleted", "item", cpp_api::booking_json(removed));
    }

    return cpp_api::json_error(404, "booking not found");
}

}  // namespace api_handlers
