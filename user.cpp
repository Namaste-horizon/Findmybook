#include "user.hpp"

#include <iostream>

namespace {
void printBookDetails(treenode* node) {
    std::cout << "Book ID: " << node->book_id
              << " | Name: " << node->title
              << " | Author: " << node->author
              << " | Available: " << node->available_copies
              << " | Library: " << node->lib << '\n';
}

int searchByAuthorRecursive(treenode* root, const std::string& author) {
    if (root == nullptr) {
        return 0;
    }

    int found_count = 0;
    if (case_insensitive_cmp(root->author, author.c_str())) {
        printBookDetails(root);
        found_count = 1;
    }

    found_count += searchByAuthorRecursive(root->left, author);
    found_count += searchByAuthorRecursive(root->right, author);
    return found_count;
}
}

User::User(const std::string& name,
           const std::string& email,
           const std::string& password)
    : user_id(id++),
      user_name(name),
      user_email(email),
      user_password(password) {}

int User::getuser_id() const {
    return user_id;
}

std::string User::getuser_name() const {
    return user_name;
}

std::string User::getuser_email() const {
    return user_email;
}

std::string User::getuser_password() const {
    return user_password;
}

void User::setuser_name(const std::string& name) {
    user_name = name;
}

void User::setuser_email(const std::string& email) {
    user_email = email;
}

void User::setuser_password(const std::string& password) {
    user_password = password;
}

void User::searchBookByName(treenode* root, const std::string& query) const {
    std::cout << "\nSearch results for \"" << query << "\":\n";
    if (search_string(root, query.c_str()) == 0) {
        std::cout << "No matching books found.\n";
    }
}

void User::searchBookByAuthor(treenode* root, const std::string& author) const {
    std::cout << "\nSearch results for author \"" << author << "\":\n";
    if (searchByAuthorRecursive(root, author) == 0) {
        std::cout << "No matching books found.\n";
    }
}

bool User::issueBookById(treenode* root, int bookId) const {
    treenode* book = search_id(root, bookId);
    if (book == nullptr) {
        std::cout << "Book not found.\n";
        return false;
    }

    if (book->available_copies <= 0) {
        std::cout << "No copies available for issue.\n";
        return false;
    }

    book->available_copies--;
    std::cout << "Book issued successfully: " << book->title << '\n';
    return true;
}

bool User::returnBookById(treenode* root, int bookId) const {
    treenode* book = search_id(root, bookId);
    if (book == nullptr) {
        std::cout << "Book not found.\n";
        return false;
    }

    if (book->available_copies >= book->total_copies) {
        std::cout << "All copies are already in the library.\n";
        return false;
    }

    book->available_copies++;
    std::cout << "Book returned successfully: " << book->title << '\n';
    return true;
}

SuperUser::SuperUser(const std::string& name,
                     const std::string& email,
                     const std::string& password,
                     const std::string& lib)
    : User(name, email, password),
      lib_name(lib) {}

treenode* SuperUser::createBook(int bookId,
                                const std::string& bookName,
                                const std::string& authorName,
                                int totalCopies) const {
    return createnode(bookId,
                      lib_name.c_str(),
                      bookName.c_str(),
                      authorName.c_str(),
                      totalCopies);
}

treenode* SuperUser::addBookToCatalog(treenode* root,
                                      int bookId,
                                      const std::string& bookName,
                                      const std::string& authorName,
                                      int totalCopies) const {
    treenode* newnode = createBook(bookId, bookName, authorName, totalCopies);
    if (newnode == nullptr) {
        return root;
    }
    return insert(root, newnode);
}

std::string SuperUser::getlib_name() const {
    return lib_name;
}

void SuperUser::setlib_name(const std::string& lib) {
    lib_name = lib;
}
