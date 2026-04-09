#include "auth_api.hpp"

#include "data.hpp"

namespace api_handlers {

cpp_api::HttpResponse handle_people_list(const std::string& role) {
    authsystem auth;
    return {200, "application/json", "{\"items\":" + cpp_api::people_array_json(auth.loadrolepeople(role)) + "}"};
}

cpp_api::HttpResponse handle_signup(const cpp_api::HttpRequest& request) {
    auto body = cpp_api::parse_json_object(request.body);
    authsystem auth;
    person created_person;
    std::string error;
    std::string requested_code;

    if (body["role"] == "lib") {
        std::vector<cpp_api::LibraryRecord> libraries = cpp_api::load_libraries();
        const cpp_api::LibraryRecord* managed_library = cpp_api::find_library_by_id(libraries, body["library_id"]);
        if (managed_library == nullptr) {
            return cpp_api::json_error(400, "library_id is required for library accounts");
        }
        requested_code = "l" + std::to_string(managed_library->id);
    }

    if (!auth.createaccountrecord(body["role"],
                                  body["fname"],
                                  body["lname"],
                                  body["email"],
                                  body["password"],
                                  body["admin_key"],
                                  created_person,
                                  error,
                                  requested_code)) {
        int status = error == "email already registered" ? 409 :
                     error == "code already assigned" ? 409 :
                     error == "invalid admin key" ? 403 :
                     400;
        return cpp_api::json_error(status, error);
    }

    return cpp_api::json_message(201,
                                 body["role"] + " created successfully",
                                 "person",
                                 cpp_api::person_json(created_person));
}

cpp_api::HttpResponse handle_login(const cpp_api::HttpRequest& request) {
    auto body = cpp_api::parse_json_object(request.body);
    authsystem auth;
    person matched_person;
    std::string error;

    if (!auth.loginrecord(body["role"],
                          body["email"],
                          body["password"],
                          body["admin_key"],
                          matched_person,
                          error)) {
        int status = 400;
        if (error == "invalid admin key") {
            status = 403;
        } else if (error == "invalid credentials") {
            status = 401;
        } else if (error.find("account not found") != std::string::npos) {
            status = 404;
        }
        return cpp_api::json_error(status, error);
    }

    return cpp_api::json_message(200, "login success", "person", cpp_api::person_json(matched_person));
}

}  // namespace api_handlers
