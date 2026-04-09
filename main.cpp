#include "cpp_api_server.hpp"
#include "login.h"

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "server") {
        return run_cpp_api_server();
    }

    authsystem a;
    a.menu();
    return 0;
}
