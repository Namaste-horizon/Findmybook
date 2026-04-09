#ifndef LIBRARIES_API_HPP
#define LIBRARIES_API_HPP

#include "cpp_api_shared.hpp"

#include <string>

namespace api_handlers {

cpp_api::HttpResponse handle_libraries_get();
cpp_api::HttpResponse handle_libraries_overview();
cpp_api::HttpResponse handle_libraries_nearby(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_library_books(const std::string& public_id);
cpp_api::HttpResponse handle_library_summary(const std::string& public_id);
cpp_api::HttpResponse handle_library_create(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_library_update(const std::string& public_id,
                                            const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_library_delete(const std::string& public_id);

}  // namespace api_handlers

#endif
