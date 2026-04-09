#ifndef DATA_HPP
#define DATA_HPP

#include "cpp_api_shared.hpp"

#include <string>
#include <vector>

namespace cpp_api {

std::vector<BookRecord> load_books();
std::vector<LibraryRecord> load_libraries();
std::vector<BookingRecord> load_bookings();

bool save_books(const std::vector<BookRecord>& books);
bool save_libraries(const std::vector<LibraryRecord>& libraries);
bool save_bookings(const std::vector<BookingRecord>& bookings);

int next_book_id(const std::vector<BookRecord>& books);
int next_library_id(const std::vector<LibraryRecord>& libraries);
int next_booking_id(const std::vector<BookingRecord>& bookings);

const LibraryRecord* find_library_by_id(const std::vector<LibraryRecord>& libraries,
                                        const std::string& library_id);
const LibraryRecord* find_library_by_name(const std::vector<LibraryRecord>& libraries,
                                          const std::string& library_name);

}  // namespace cpp_api

#endif
