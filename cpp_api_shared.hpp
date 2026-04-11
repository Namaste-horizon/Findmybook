#ifndef CPP_API_SHARED_HPP
#define CPP_API_SHARED_HPP

#include "book.hpp"
#include "login.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpp_api {

struct BookRecord {
    int id;
    std::string title;
    std::string author;
    std::string genre;
    std::string library_id;
    std::string library;
    int total_copies;
    int available_copies;
};

struct LibraryRecord {
    int id;
    std::string name;
    std::string city;
    double lat;
    double lng;
    int total_seats;
};

struct BookingRecord {
    int id;
    std::string user_email;
    std::string user_name;
    std::string user_role;
    std::string category;
    std::string title;
    std::string book_id;
    std::string library_id;
    std::string library;
    std::string date;
    std::string time;
    std::string status;
    int seat_number;
};

struct HttpRequest {
    std::string method;
    std::string target;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> query;
};

struct HttpResponse {
    int status = 200;
    std::string content_type = "application/json";
    std::string body = "{}";
};

std::string trim(const std::string& value);
std::string to_lower_copy(std::string value);
std::unordered_map<std::string, std::string> parse_json_object(const std::string& body);
int parse_int_or(const std::string& value, int fallback);
double parse_double_or(const std::string& value, double fallback);
std::string format_double(double value);

std::string person_json(const person& current_person);
std::string people_array_json(const std::vector<person>& people);
std::string book_json(const BookRecord& book);
std::string library_json(const LibraryRecord& library);
std::string booking_json(const BookingRecord& booking);

std::string book_public_id(int id);
std::string library_public_id(int id);
std::string booking_public_id(int id);
int parse_public_id(const std::string& value, const std::string& prefix);
std::string library_id_from_role_code(const std::string& code);

std::vector<BookRecord> load_books();
std::vector<LibraryRecord> load_libraries();
std::vector<BookingRecord> load_bookings();
bool save_books(const std::vector<BookRecord>& books);
bool save_libraries(const std::vector<LibraryRecord>& libraries);
bool save_bookings(const std::vector<BookingRecord>& bookings);

int next_book_id(const std::vector<BookRecord>& books);
int next_library_id(const std::vector<LibraryRecord>& libraries);
int next_booking_id(const std::vector<BookingRecord>& bookings);

treenode* build_book_tree(const std::vector<BookRecord>& books);
std::vector<BookRecord> books_from_tree(treenode* root);
void merge_book_metadata(std::vector<BookRecord>& target_books,
                         const std::vector<BookRecord>& source_books);

const LibraryRecord* find_library_by_id(const std::vector<LibraryRecord>& libraries,
                                        const std::string& library_id);
const LibraryRecord* find_library_by_name(const std::vector<LibraryRecord>& libraries,
                                          const std::string& library_name);

void sync_library_references(const LibraryRecord& library,
                             std::vector<BookRecord>& books,
                             std::vector<BookingRecord>& bookings);
void sync_book_references(const BookRecord& book,
                          std::vector<BookingRecord>& bookings);
bool library_has_linked_records(const std::string& library_id,
                                const std::vector<BookRecord>& books,
                                const std::vector<BookingRecord>& bookings);
bool book_has_active_rentals(int book_id,
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
bool is_valid_email(const std::string& email);

std::string list_wrapper_json(const std::string& key,
                              const std::vector<std::string>& items);
HttpResponse json_error(int status, const std::string& message);
HttpResponse json_message(int status,
                          const std::string& message,
                          const std::string& key = "",
                          const std::string& payload = "");
HttpResponse options_response();

bool read_http_request(int client_socket, HttpRequest& request);
void send_http_response(int client_socket, const HttpResponse& response);

}  // namespace cpp_api

#endif
