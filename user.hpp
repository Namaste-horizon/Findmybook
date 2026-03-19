// User supports search, issue, and return actions.
// SuperUser extends User with book creation and catalog management.
#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <vector>
#include "book.hpp"

class User {
private:
    inline static int id = 0;
    int user_id;
    std::string user_name;
    std::string user_email;
    std::string user_password;

public:
    User(const std::string& name,
         const std::string& email,
         const std::string& password);

    int getuser_id() const;
    std::string getuser_name() const;
    std::string getuser_email() const;
    std::string getuser_password() const;

    void setuser_name(const std::string& name);
    void setuser_email(const std::string& email);
    void setuser_password(const std::string& password);

    void searchBookByName(const std::vector<Book>& catalog,
                          const std::string& query) const;
    void searchBookByAuthor(const std::vector<Book>& catalog,
                            const std::string& author) const;
    bool issueBookById(std::vector<Book>& catalog, int bookId) const;
    bool returnBookById(std::vector<Book>& catalog, int bookId) const;
};

class SuperUser : public User {
private:
    std::string lib_name;

public:
    SuperUser(const std::string& name,
              const std::string& email,
              const std::string& password,
              const std::string& lib);

    Book createBook(int totalCopies,
                    const std::string& bookName,
                    const std::string& authorName) const;
    void addBookToCatalog(std::vector<Book>& catalog,
                          int totalCopies,
                          const std::string& bookName,
                          const std::string& authorName) const;

    std::string getlib_name() const;
    void setlib_name(const std::string& lib);
};

#endif
