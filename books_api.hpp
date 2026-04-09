#ifndef BOOKS_API_HPP
#define BOOKS_API_HPP

#include "cpp_api_shared.hpp"

#include <string>

namespace api_handlers {

cpp_api::HttpResponse handle_books_get(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_book_create(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_book_update(const std::string& public_id,
                                         const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_book_delete(const std::string& public_id);

}  // namespace api_handlers

#endif
