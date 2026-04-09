#include "library_module.hpp"

#include "data.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace cpp_api {
namespace {

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

LibraryOverview build_library_overview(const LibraryRecord& library,
                                       const std::vector<BookRecord>& books,
                                       const std::vector<BookingRecord>& bookings) {
    LibraryOverview overview;
    overview.library = library;

    std::string public_library_id = library_public_id(library.id);
    for (const BookRecord& book : books) {
        if (book.library_id != public_library_id) {
            continue;
        }
        ++overview.book_titles;
        overview.total_copies += book.total_copies;
        overview.available_copies += book.available_copies;
    }

    for (const BookingRecord& booking : bookings) {
        if (booking.library_id == public_library_id &&
            booking.category == "seat" &&
            booking.status != "cancelled") {
            ++overview.seat_bookings;
        }
    }

    return overview;
}

}  // namespace

std::vector<BookRecord> load_books_for_library(const std::string& library_id) {
    std::vector<BookRecord> items;
    for (const BookRecord& book : load_books()) {
        if (book.library_id == library_id) {
            items.push_back(book);
        }
    }
    return items;
}

std::vector<BookingRecord> load_bookings_for_library(const std::string& library_id) {
    std::vector<BookingRecord> items;
    for (const BookingRecord& booking : load_bookings()) {
        if (booking.library_id == library_id) {
            items.push_back(booking);
        }
    }
    return items;
}

std::vector<LibraryOverview> load_library_overviews() {
    std::vector<LibraryRecord> libraries = load_libraries();
    std::vector<BookRecord> books = load_books();
    std::vector<BookingRecord> bookings = load_bookings();
    std::vector<LibraryOverview> items;

    for (const LibraryRecord& library : libraries) {
        items.push_back(build_library_overview(library, books, bookings));
    }

    return items;
}

bool load_library_summary(const std::string& public_id, LibrarySummary& summary) {
    std::vector<LibraryRecord> libraries = load_libraries();
    const LibraryRecord* library = find_library_by_id(libraries, public_id);
    if (library == nullptr) {
        return false;
    }

    summary.library = *library;
    summary.books = load_books_for_library(public_id);
    summary.bookings = load_bookings_for_library(public_id);

    for (const BookRecord& book : summary.books) {
        summary.total_copies += book.total_copies;
        summary.available_copies += book.available_copies;
    }
    for (const BookingRecord& booking : summary.bookings) {
        if (booking.category == "seat" && booking.status != "cancelled") {
            ++summary.seat_bookings;
        } else if (booking.category == "rental") {
            ++summary.rental_bookings;
        }
    }

    return true;
}

std::string library_overview_json(const LibraryOverview& overview) {
    std::string payload = library_json(overview.library);
    if (!payload.empty() && payload.back() == '}') {
        payload.pop_back();
    }

    payload +=
        ",\"inventory\":{"
        "\"book_titles\":" + std::to_string(overview.book_titles) + ","
        "\"total_copies\":" + std::to_string(overview.total_copies) + ","
        "\"available_copies\":" + std::to_string(overview.available_copies) + ","
        "\"issued_copies\":" + std::to_string(std::max(overview.total_copies - overview.available_copies, 0)) +
        "},"
        "\"slots\":{"
        "\"total\":" + std::to_string(overview.library.total_seats) + ","
        "\"booked\":" + std::to_string(overview.seat_bookings) + ","
        "\"available\":" + std::to_string(std::max(overview.library.total_seats - overview.seat_bookings, 0)) +
        "}"
        "}";

    return payload;
}

std::string library_summary_json(const LibrarySummary& summary) {
    std::vector<std::string> book_items;
    for (const BookRecord& book : summary.books) {
        book_items.push_back(book_json(book));
    }

    std::vector<std::string> booking_items;
    for (const BookingRecord& booking : summary.bookings) {
        booking_items.push_back(booking_json(booking));
    }

    return
        "{"
        "\"library\":" + library_json(summary.library) + ","
        "\"books\":" + json_array(book_items) + ","
        "\"bookings\":" + json_array(booking_items) + ","
        "\"stats\":{"
        "\"book_titles\":" + std::to_string(static_cast<int>(summary.books.size())) + ","
        "\"total_copies\":" + std::to_string(summary.total_copies) + ","
        "\"available_copies\":" + std::to_string(summary.available_copies) + ","
        "\"issued_copies\":" + std::to_string(std::max(summary.total_copies - summary.available_copies, 0)) + ","
        "\"total_slots\":" + std::to_string(summary.library.total_seats) + ","
        "\"booked_slots\":" + std::to_string(summary.seat_bookings) + ","
        "\"available_slots\":" + std::to_string(std::max(summary.library.total_seats - summary.seat_bookings, 0)) + ","
        "\"rental_bookings\":" + std::to_string(summary.rental_bookings) +
        "}"
        "}";
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

}  // namespace cpp_api
