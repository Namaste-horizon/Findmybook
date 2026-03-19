#include "admin.hpp"

int Admin::next_id = 1;

Admin::Admin(const std::string& name,
             const std::string& email,
             const std::string& password,
             const std::string& lib)
    : id(next_id++),
      lib(lib),
      name(name),
      email(email),
      password(password),
      left(nullptr),
      right(nullptr) {}

Admin* create_admin(const std::string& name,
                    const std::string& email,
                    const std::string& password,
                    const std::string& lib) {
    return new Admin(name, email, password, lib);
}

Admin* insert_admin(Admin* root, Admin* newnode) {
    if (root == nullptr) {
        return newnode;
    }

    int cmp = newnode->email.compare(root->email);
    if (cmp < 0) {
        root->left = insert_admin(root->left, newnode);
    } else if (cmp > 0) {
        root->right = insert_admin(root->right, newnode);
    } else {
        delete newnode;
    }
    return root;
}

Admin* search_admin_by_email(Admin* root, const std::string& email) {
    if (root == nullptr) {
        return nullptr;
    }

    int cmp = email.compare(root->email);
    if (cmp == 0) {
        return root;
    }
    if (cmp < 0) {
        return search_admin_by_email(root->left, email);
    }
    return search_admin_by_email(root->right, email);
}

bool authenticate_admin(Admin* root,
                        const std::string& email,
                        const std::string& password) {
    Admin* temp = search_admin_by_email(root, email);
    if (temp == nullptr) {
        return false;
    }
    return temp->password == password;
}

void free_admin(Admin* root) {
    if (root == nullptr) {
        return;
    }

    free_admin(root->left);
    free_admin(root->right);
    delete root;
}
