#ifndef USERS_API_HPP
#define USERS_API_HPP

#include "cpp_api_shared.hpp"

namespace api_handlers {

cpp_api::HttpResponse handle_user_activity(const cpp_api::HttpRequest& request);

}  // namespace api_handlers

#endif
