#ifndef BOOKINGS_API_HPP
#define BOOKINGS_API_HPP

#include "cpp_api_shared.hpp"

#include <string>

namespace api_handlers {

cpp_api::HttpResponse handle_bookings_get(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_booking_get(const std::string& public_id);
cpp_api::HttpResponse handle_booking_create(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_booking_update(const std::string& public_id,
                                            const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_booking_delete(const std::string& public_id);

}  // namespace api_handlers

#endif
