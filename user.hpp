#ifndef USER_HPP
#define USER_HPP

#include "book.hpp"

#include <string>

class User {
public:
    User(const std::string& name, const std::string& email, const std::string& password);

    bool issueBookById(treenode* root, int book_id);
    bool returnBookById(treenode* root, int book_id);

private:
    std::string name_;
    std::string email_;
    std::string password_;
};

#endif
