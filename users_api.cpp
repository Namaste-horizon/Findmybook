#include "users_api.hpp"

#include "data.hpp"

#include <vector>

namespace api_handlers {
namespace {

std::string booking_array_json(const std::vector<cpp_api::BookingRecord>& bookings) {
    std::string payload = "[";
    for (size_t index = 0; index < bookings.size(); ++index) {
        if (index > 0) {
            payload += ",";
        }
        payload += cpp_api::booking_json(bookings[index]);
    }
    payload += "]";
    return payload;
}

}  // namespace

cpp_api::HttpResponse handle_user_activity(const cpp_api::HttpRequest& request) {
    std::string email = cpp_api::to_lower_copy(cpp_api::trim(request.query.count("email") ? request.query.at("email") : ""));
    if (email.empty()) {
        return cpp_api::json_error(400, "email is required");
    }

    std::vector<cpp_api::BookingRecord> rentals;
    std::vector<cpp_api::BookingRecord> seats;
    for (const cpp_api::BookingRecord& booking : cpp_api::load_bookings()) {
        if (cpp_api::to_lower_copy(booking.user_email) != email) {
            continue;
        }

        if (booking.category == "rental") {
            rentals.push_back(booking);
        } else if (booking.category == "seat") {
            seats.push_back(booking);
        }
    }

    int active_rentals = 0;
    for (const cpp_api::BookingRecord& rental : rentals) {
        if (rental.status != "returned" && rental.status != "cancelled") {
            ++active_rentals;
        }
    }

    std::string payload =
        "{"
        "\"email\":\"" + email + "\","
        "\"issued_books\":" + booking_array_json(rentals) + ","
        "\"seat_bookings\":" + booking_array_json(seats) + ","
        "\"stats\":{"
        "\"total_issued_books\":" + std::to_string(static_cast<int>(rentals.size())) + ","
        "\"active_issued_books\":" + std::to_string(active_rentals) + ","
        "\"total_slot_bookings\":" + std::to_string(static_cast<int>(seats.size())) +
        "}"
        "}";

    return {200, "application/json", payload};
}

}  // namespace api_handlers
