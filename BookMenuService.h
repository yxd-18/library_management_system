#ifndef BOOKMENUSERVICE_H
#define BOOKMENUSERVICE_H

#include <optional>
#include <string>
#include <vector>
#include "ServiceResult.h"
#include "Book.h"
#include "BookCategory.h"
#include "BookDao.h"
#include "CategoryDao.h"

ActionResult createBook(BookDao& bookDao, const Book& book, long long operatorId);
DataResult<std::vector<Book>> listBooks(BookDao& bookDao);
DataResult<std::optional<Book>> findBookById(BookDao& bookDao, long long bookId);
DataResult<std::vector<Book>> searchBooksByKeyword(BookDao& bookDao, const std::string& keyword);
ActionResult updateExistingBook(BookDao& bookDao, const Book& book, long long operatorId);
ActionResult removeBook(BookDao& bookDao, long long bookId, long long operatorId);

ActionResult createCategory(CategoryDao& categoryDao, const BookCategory& category, long long operatorId);
DataResult<std::vector<BookCategory>> listCategories(CategoryDao& categoryDao);
DataResult<std::optional<BookCategory>> findCategoryById(CategoryDao& categoryDao, int categoryId);
ActionResult updateExistingCategory(CategoryDao& categoryDao, const BookCategory& category, long long operatorId);
ActionResult removeCategory(CategoryDao& categoryDao, int categoryId, long long operatorId);

#endif
