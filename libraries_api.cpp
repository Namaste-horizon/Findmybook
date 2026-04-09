#include "libraries_api.hpp"

#include "data.hpp"
#include "library_module.hpp"

namespace api_handlers {

cpp_api::HttpResponse handle_libraries_get() {
    std::vector<std::string> items;
    for (const cpp_api::LibraryRecord& library : cpp_api::load_libraries()) {
        items.push_back(cpp_api::library_json(library));
    }
    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_libraries_overview() {
    std::vector<std::string> items;

    for (const cpp_api::LibraryOverview& overview : cpp_api::load_library_overviews()) {
        items.push_back(cpp_api::library_overview_json(overview));
    }

    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_libraries_nearby(const cpp_api::HttpRequest& request) {
    if (!request.query.count("lat") || !request.query.count("lng")) {
        return cpp_api::json_error(400, "lat and lng are required");
    }

    double lat = cpp_api::parse_double_or(request.query.at("lat"), 0.0);
    double lng = cpp_api::parse_double_or(request.query.at("lng"), 0.0);
    std::vector<std::pair<cpp_api::LibraryRecord, double>> ranked =
        cpp_api::rank_libraries_by_distance(lat, lng, cpp_api::load_libraries());

    std::vector<std::string> items;
    for (const auto& entry : ranked) {
        std::string payload = cpp_api::library_json(entry.first);
        if (!payload.empty() && payload.back() == '}') {
            payload.pop_back();
        }
        payload += ",\"distance_km\":" + cpp_api::format_double(entry.second) + "}";
        items.push_back(payload);
    }

    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_library_books(const std::string& public_id) {
    int library_id = cpp_api::parse_public_id(public_id, "lib");
    if (library_id <= 0) {
        return cpp_api::json_error(404, "library not found");
    }

    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
    const cpp_api::LibraryRecord* library = cpp_api::find_library_by_id(libraries, public_id);
    if (library == nullptr) {
        return cpp_api::json_error(404, "library not found");
    }

    std::vector<std::string> items;
    for (const cpp_api::BookRecord& book : cpp_api::load_books_for_library(public_id)) {
        items.push_back(cpp_api::book_json(book));
    }

    return {200, "application/json", cpp_api::list_wrapper_json("items", items)};
}

cpp_api::HttpResponse handle_library_summary(const std::string& public_id) {
    cpp_api::LibrarySummary summary;
    if (!cpp_api::load_library_summary(public_id, summary)) {
        return cpp_api::json_error(404, "library not found");
    }

    return {200, "application/json", cpp_api::library_summary_json(summary)};
}

cpp_api::HttpResponse handle_library_create(const cpp_api::HttpRequest& request) {
    auto body = cpp_api::parse_json_object(request.body);
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();

    std::string name = cpp_api::trim(body["name"]);
    if (name.empty()) {
        return cpp_api::json_error(400, "name, lat, and lng are required");
    }

    cpp_api::LibraryRecord library{
        cpp_api::next_library_id(libraries),
        name,
        cpp_api::trim(body["city"]),
        cpp_api::parse_double_or(body["lat"], 0.0),
        cpp_api::parse_double_or(body["lng"], 0.0),
        std::max(cpp_api::parse_int_or(body["total_seats"], 20), 1),
    };

    libraries.push_back(library);
    if (!cpp_api::save_libraries(libraries)) {
        return cpp_api::json_error(500, "failed to save libraries");
    }

    return cpp_api::json_message(201, "library created", "item", cpp_api::library_json(library));
}

cpp_api::HttpResponse handle_library_update(const std::string& public_id,
                                            const cpp_api::HttpRequest& request) {
    int library_id = cpp_api::parse_public_id(public_id, "lib");
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
    auto body = cpp_api::parse_json_object(request.body);

    for (cpp_api::LibraryRecord& library : libraries) {
        if (library.id != library_id) {
            continue;
        }

        if (!cpp_api::trim(body["name"]).empty()) {
            library.name = cpp_api::trim(body["name"]);
        }
        if (body.count("city")) {
            library.city = cpp_api::trim(body["city"]);
        }
        if (body.count("lat")) {
            library.lat = cpp_api::parse_double_or(body["lat"], library.lat);
        }
        if (body.count("lng")) {
            library.lng = cpp_api::parse_double_or(body["lng"], library.lng);
        }
        if (body.count("total_seats")) {
            library.total_seats = std::max(cpp_api::parse_int_or(body["total_seats"], library.total_seats), 1);
        }

        std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
        std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();
        cpp_api::sync_library_references(library, books, bookings);

        if (!cpp_api::save_books(books)) {
            return cpp_api::json_error(500, "failed to sync linked books");
        }
        if (!cpp_api::save_bookings(bookings)) {
            return cpp_api::json_error(500, "failed to sync linked bookings");
        }
        if (!cpp_api::save_libraries(libraries)) {
            return cpp_api::json_error(500, "failed to save libraries");
        }

        return cpp_api::json_message(200, "library updated", "item", cpp_api::library_json(library));
    }

    return cpp_api::json_error(404, "library not found");
}

cpp_api::HttpResponse handle_library_delete(const std::string& public_id) {
    int library_id = cpp_api::parse_public_id(public_id, "lib");
    std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
    std::vector<cpp_api::BookRecord> books = cpp_api::load_books();
    std::vector<cpp_api::BookingRecord> bookings = cpp_api::load_bookings();

    for (size_t index = 0; index < libraries.size(); ++index) {
        if (libraries[index].id != library_id) {
            continue;
        }

        if (cpp_api::library_has_linked_records(public_id, books, bookings)) {
            return cpp_api::json_error(409, "remove linked books and bookings before deleting this library");
        }

        cpp_api::LibraryRecord removed = libraries[index];
        libraries.erase(libraries.begin() + static_cast<long>(index));
        if (!cpp_api::save_libraries(libraries)) {
            return cpp_api::json_error(500, "failed to save libraries");
        }

        return cpp_api::json_message(200, "library deleted", "item", cpp_api::library_json(removed));
    }

    return cpp_api::json_error(404, "library not found");
}

}  // namespace api_handlers
