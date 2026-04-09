#include "books_api.hpp"

#include "data.hpp"
#include "user.hpp"

#include <cstring>
#include <vector>

namespace api_handlers {

cpp_api::HttpResponse handle_books_get(const cpp_api::HttpRequest& request) {
    std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
    std::string query = cpp_api::to_lower_copy(cpp_api::trim(request.query.count("q") ? request.query.at("q") : ""));
    std::string library_id_filter = cpp_api::trim(request.query.count("library_id") ? request.query.at("library_id") : "");
    std::string library_filter = cpp_api::to_lower_copy(cpp_api::trim(request.query.count("library") ? request.query.at("library") : ""));
    std::vector<std::string> items;

    for (const cpp_api::BookRecord& book : books) {
        if (!library_id_filter.empty() && book.library_id != library_id_filter) {
            continue;
        }
        if (!library_filter.empty() && cpp_api::to_lower_copy(book.library) != library_filter) {
            continue;
        }
        if (!query.empty()) {
            std::string haystack = cpp_api::to_lower_copy(book.title + " " + book.author + " " + book.library + " " + book.genre);
            if (haystack.find(query) == std::string::npos) {
                continue;
            }
        }
        items.push_back(cpp_api::book_json(book));
    }

    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_book_create(const cpp_api::HttpRequest& request) {
    auto body = cpp_api::parse_json_object(request.body);
    std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();

    if (cpp_api::trim(body["title"]).empty() || cpp_api::trim(body["author"]).empty()) {
        return cpp_api::json_error(400, "title and author are required");
    }

    std::string library_id = cpp_api::trim(body["library_id"]);
    std::string library_name = cpp_api::trim(body["library"]);
    const cpp_api::LibraryRecord* selected_library = nullptr;
    if (!library_id.empty()) {
        selected_library = cpp_api::find_library_by_id(libraries, library_id);
    }
    if (selected_library == nullptr && !library_name.empty()) {
        selected_library = cpp_api::find_library_by_name(libraries, library_name);
    }
    if (selected_library == nullptr) {
        selected_library = cpp_api::find_library_by_id(libraries, "lib-1");
    }

    cpp_api::BookRecord created{
        cpp_api::next_book_id(books),
        cpp_api::trim(body["title"]),
        cpp_api::trim(body["author"]),
        cpp_api::trim(body["genre"]),
        selected_library != nullptr ? cpp_api::library_public_id(selected_library->id) : "lib-1",
        selected_library != nullptr ? selected_library->name : "Central Library",
        cpp_api::parse_int_or(body["total_copies"], 1),
        cpp_api::parse_int_or(body["available_copies"], cpp_api::parse_int_or(body["total_copies"], 1)),
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

    treenode* root = cpp_api::build_book_tree(books);
    treenode* node = createnode(created.id,
                                created.library.c_str(),
                                created.title.c_str(),
                                created.author.c_str(),
                                created.total_copies);
    if (node == nullptr) {
        free_bst(root);
        return cpp_api::json_error(500, "failed to create book");
    }

    node->available_copies = created.available_copies;
    root = insert(root, node);
    books = cpp_api::books_from_tree(root);
    cpp_api::merge_book_metadata(books, cpp_api::load_books());
    books.push_back(created);
    for (size_t index = 0; index + 1 < books.size(); ++index) {
        if (books[index].id == created.id) {
            books.erase(books.begin() + static_cast<long>(index));
            break;
        }
    }
    free_bst(root);

    if (!cpp_api::save_books(books)) {
        return cpp_api::json_error(500, "failed to save books");
    }

    return cpp_api::json_message(201, "book created", "item", cpp_api::book_json(created));
}

cpp_api::HttpResponse handle_book_update(const std::string& public_id,
                                         const cpp_api::HttpRequest& request) {
    int book_id = cpp_api::parse_public_id(public_id, "book");
    if (book_id <= 0) {
        return cpp_api::json_error(404, "book not found");
    }

    auto body = cpp_api::parse_json_object(request.body);
    std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
    treenode* root = cpp_api::build_book_tree(books);
    treenode* node = search_id(root, book_id);
    if (node == nullptr) {
        free_bst(root);
        return cpp_api::json_error(404, "book not found");
    }

    std::string title = cpp_api::trim(body["title"].empty() ? node->title : body["title"]);
    std::string author = cpp_api::trim(body["author"].empty() ? node->author : body["author"]);
    std::string genre = cpp_api::trim(body["genre"]);
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
    std::string library_id = cpp_api::trim(body["library_id"]);
    std::string library = cpp_api::trim(body["library"].empty() ? node->lib : body["library"]);
    int total_copies = cpp_api::parse_int_or(body["total_copies"], node->total_copies);
    int available_copies = cpp_api::parse_int_or(body["available_copies"], node->available_copies);

    if (title.empty() || author.empty()) {
        free_bst(root);
        return cpp_api::json_error(400, "title and author are required");
    }

    if (library_id.empty()) {
        for (const cpp_api::BookRecord& current_book : books) {
            if (current_book.id == book_id) {
                library_id = current_book.library_id;
                if (library.empty()) {
                    library = current_book.library;
                }
                break;
            }
        }
    }

    const cpp_api::LibraryRecord* selected_library = !library_id.empty()
        ? cpp_api::find_library_by_id(libraries, library_id)
        : (!library.empty() ? cpp_api::find_library_by_name(libraries, library) : nullptr);
    if (selected_library != nullptr) {
        library_id = cpp_api::library_public_id(selected_library->id);
        library = selected_library->name;
    }
    if (library.empty()) {
        library = "Central Library";
        library_id = "lib-1";
    }
    if (genre.empty()) {
        for (const cpp_api::BookRecord& current_book : books) {
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

    std::vector<cpp_api::BookRecord> original_books = books;
    books = cpp_api::books_from_tree(root);
    cpp_api::merge_book_metadata(books, original_books);
    free_bst(root);

    for (cpp_api::BookRecord& current_book : books) {
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

    cpp_api::BookRecord updated{book_id, title, author, genre, library_id, library, total_copies, available_copies};
    std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();
    cpp_api::sync_book_references(updated, bookings);

    if (!cpp_api::save_books(books)) {
        return cpp_api::json_error(500, "failed to save books");
    }
    if (!cpp_api::save_bookings(bookings)) {
        return cpp_api::json_error(500, "failed to sync linked bookings");
    }

    return cpp_api::json_message(200, "book updated", "item", cpp_api::book_json(updated));
}

cpp_api::HttpResponse handle_book_delete(const std::string& public_id) {
    int book_id = cpp_api::parse_public_id(public_id, "book");
    if (book_id <= 0) {
        return cpp_api::json_error(404, "book not found");
    }

    if (cpp_api::book_has_active_rentals(book_id, cpp_api::load_bookings())) {
        return cpp_api::json_error(409, "return the active rental before deleting this book");
    }

    std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
    treenode* root = cpp_api::build_book_tree(books);
    treenode* node = search_id(root, book_id);
    if (node == nullptr) {
        free_bst(root);
        return cpp_api::json_error(404, "book not found");
    }

    std::string removed_genre = "General";
    std::string removed_library_id = "lib-1";
    for (const cpp_api::BookRecord& current_book : books) {
        if (current_book.id == node->book_id) {
            removed_genre = current_book.genre;
            removed_library_id = current_book.library_id;
            break;
        }
    }

    cpp_api::BookRecord removed{
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
    books = cpp_api::books_from_tree(root);
    cpp_api::merge_book_metadata(books, cpp_api::load_books());
    free_bst(root);

    if (!cpp_api::save_books(books)) {
        return cpp_api::json_error(500, "failed to save books");
    }

    return cpp_api::json_message(200, "book deleted", "item", cpp_api::book_json(removed));
}

}  // namespace api_handlers
