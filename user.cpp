#include "user.hpp"

User::User(const std::string& name, const std::string& email, const std::string& password)
    : name_(name), email_(email), password_(password) {}

bool User::issueBookById(treenode* root, int book_id) {
    treenode* node = search_id(root, book_id);
    if (node == nullptr || node->available_copies <= 0) {
        return false;
    }

    --node->available_copies;
    return true;
}

bool User::returnBookById(treenode* root, int book_id) {
    treenode* node = search_id(root, book_id);
    if (node == nullptr) {
        return false;
    }

    if (node->available_copies < node->total_copies) {
        ++node->available_copies;
    }
    return true;
}
