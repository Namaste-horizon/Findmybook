#ifndef ADMIN_HPP
#define ADMIN_HPP

#include <string>

class Admin {
public:
    static int next_id;

    int id;
    std::string lib;
    std::string name;
    std::string email;
    std::string password;
    Admin* left;
    Admin* right;

    Admin(const std::string& name,
          const std::string& email,
          const std::string& password,
          const std::string& lib);
};

Admin* create_admin(const std::string& name,
                    const std::string& email,
                    const std::string& password,
                    const std::string& lib);

Admin* insert_admin(Admin* root, Admin* newnode);

Admin* search_admin_by_email(Admin* root, const std::string& email);

bool authenticate_admin(Admin* root,
                        const std::string& email,
                        const std::string& password);

void free_admin(Admin* root);

#endif
