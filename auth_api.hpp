#ifndef AUTH_API_HPP
#define AUTH_API_HPP

#include "cpp_api_shared.hpp"

#include <string>

namespace api_handlers {

cpp_api::HttpResponse handle_people_list(const std::string& role);
cpp_api::HttpResponse handle_signup(const cpp_api::HttpRequest& request);
cpp_api::HttpResponse handle_login(const cpp_api::HttpRequest& request);

}  // namespace api_handlers

#endif
