#include "cpp_api_server.hpp"

#include "auth_api.hpp"
#include "bookings_api.hpp"
#include "books_api.hpp"
#include "cpp_api_shared.hpp"
#include "libraries_api.hpp"
#include "users_api.hpp"

#include <crow.h>
#include <crow/middlewares/cors.h>

#include <iostream>
#include <mutex>
#include <string>

namespace {

using App = crow::App<crow::CORSHandler>;
std::mutex api_mutex;

cpp_api::HttpRequest make_request(const crow::request& req) {
    cpp_api::HttpRequest request;
    request.method = crow::method_name(req.method);
    request.target = req.raw_url;
    request.path = req.url;
    request.body = req.body;

    for (const std::string& key : req.url_params.keys()) {
        const char* value = req.url_params.get(key);
        request.query[key] = value == nullptr ? "" : value;
    }

    return request;
}

crow::response make_response(const cpp_api::HttpResponse& response) {
    crow::response result(response.status);
    result.set_header("Content-Type", response.content_type);
    result.write(response.body);
    return result;
}

template <typename Handler>
crow::response guard_api(Handler&& handler) {
    std::lock_guard<std::mutex> lock(api_mutex);
    return make_response(handler());
}

void configure_cors(App& app) {
    app.get_middleware<crow::CORSHandler>()
        .global()
        .origin("*")
        .headers("Content-Type")
        .methods(crow::HTTPMethod::Get,
                 crow::HTTPMethod::Post,
                 crow::HTTPMethod::Put,
                 crow::HTTPMethod::Delete,
                 crow::HTTPMethod::Options);
}

void register_routes(App& app) {
    CROW_ROUTE(app, "/api/health")
    ([] {
        return guard_api([] {
            return cpp_api::HttpResponse{200, "application/json", "{\"status\":\"ok\",\"backend\":\"cpp\",\"framework\":\"crow\"}"};
        });
    });

    CROW_ROUTE(app, "/api/admins")
    ([] {
        return guard_api([] {
            return api_handlers::handle_people_list("admin");
        });
    });

    CROW_ROUTE(app, "/api/users")
    ([] {
        return guard_api([] {
            return api_handlers::handle_people_list("user");
        });
    });

    CROW_ROUTE(app, "/api/library-accounts")
    ([] {
        return guard_api([] {
            return api_handlers::handle_people_list("lib");
        });
    });

    CROW_ROUTE(app, "/api/signup")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_signup(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/login")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_login(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_books_get(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_book_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_book_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/books/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_book_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/libraries")
        .methods(crow::HTTPMethod::Get)
    ([] {
        return guard_api([] {
            return api_handlers::handle_libraries_get();
        });
    });

    CROW_ROUTE(app, "/api/libraries/overview")
        .methods(crow::HTTPMethod::Get)
    ([] {
        return guard_api([] {
            return api_handlers::handle_libraries_overview();
        });
    });

    CROW_ROUTE(app, "/api/libraries/nearby")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_libraries_nearby(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_library_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>/books")
        .methods(crow::HTTPMethod::Get)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_library_books(id);
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>/summary")
        .methods(crow::HTTPMethod::Get)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_library_summary(id);
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_library_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/libraries/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_library_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/bookings")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_bookings_get(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/users/activity")
        .methods(crow::HTTPMethod::Get)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_user_activity(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings")
        .methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        return guard_api([&] {
            return api_handlers::handle_booking_create(make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Get)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_booking_get(id);
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Put)
    ([](const crow::request& req, const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_booking_update(id, make_request(req));
        });
    });

    CROW_ROUTE(app, "/api/bookings/<string>")
        .methods(crow::HTTPMethod::Delete)
    ([](const std::string& id) {
        return guard_api([&] {
            return api_handlers::handle_booking_delete(id);
        });
    });

    CROW_ROUTE(app, "/api/<path>")
        .methods(crow::HTTPMethod::Options)
    ([](const std::string&) {
        return guard_api([] {
            return cpp_api::options_response();
        });
    });
}

}  // namespace

int run_cpp_api_server(int port) {
    App app;
    app.loglevel(crow::LogLevel::Warning);
    configure_cors(app);
    register_routes(app);

    std::cout << "C++ API server running with Crow on http://127.0.0.1:" << port << "\n";
    app.port(static_cast<uint16_t>(port)).run();
    return 0;
}
