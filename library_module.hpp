#ifndef LIBRARY_MODULE_HPP
#define LIBRARY_MODULE_HPP

#include "cpp_api_shared.hpp"

#include <string>
#include <utility>
#include <vector>

namespace cpp_api {

struct LibraryOverview {
    LibraryRecord library;
    int book_titles = 0;
    int total_copies = 0;
    int available_copies = 0;
    int seat_bookings = 0;
};

struct LibrarySummary {
    LibraryRecord library;
    std::vector<BookRecord> books;
    std::vector<BookingRecord> bookings;
    int total_copies = 0;
    int available_copies = 0;
    int seat_bookings = 0;
    int rental_bookings = 0;
};

std::vector<BookRecord> load_books_for_library(const std::string& library_id);
std::vector<BookingRecord> load_bookings_for_library(const std::string& library_id);
std::vector<LibraryOverview> load_library_overviews();
bool load_library_summary(const std::string& public_id, LibrarySummary& summary);

std::string library_overview_json(const LibraryOverview& overview);
std::string library_summary_json(const LibrarySummary& summary);

void sync_library_references(const LibraryRecord& library,
                             std::vector<BookRecord>& books,
                             std::vector<BookingRecord>& bookings);
bool library_has_linked_records(const std::string& library_id,
                                const std::vector<BookRecord>& books,
                                const std::vector<BookingRecord>& bookings);
std::vector<std::pair<LibraryRecord, double>> rank_libraries_by_distance(
    double origin_lat,
    double origin_lng,
    const std::vector<LibraryRecord>& libraries);
int greedy_allocate_seat(const std::vector<BookingRecord>& bookings,
                         const LibraryRecord& library,
                         const std::string& library_id,
                         const std::string& date,
                         const std::string& time,
                         int ignore_booking_id = -1);

}  // namespace cpp_api

#endif
