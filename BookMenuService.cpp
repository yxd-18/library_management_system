#include "BookMenuService.h"

ActionResult createBook(BookDao& bookDao, const Book& book, long long operatorId) {
    ActionResult result;
    result.success = bookDao.addBook(book, operatorId);
    result.message = result.success ? "新增图书成功！" : "新增图书失败！";
    return result;
}

DataResult<std::vector<Book>> listBooks(BookDao& bookDao) {
    DataResult<std::vector<Book>> result;
    result.success = true;
    result.data = bookDao.getAllBooks();
    result.message = result.data.empty() ? "当前暂无图书数据。" : "查询图书列表成功。";
    return result;
}

DataResult<std::optional<Book>> findBookById(BookDao& bookDao, long long bookId) {
    DataResult<std::optional<Book>> result;
    result.data = bookDao.getBookById(bookId);
    result.success = result.data.has_value();
    result.message = result.success ? "查询图书成功。" : "未找到该图书！";
    return result;
}

DataResult<std::vector<Book>> searchBooksByKeyword(BookDao& bookDao, const std::string& keyword) {
    DataResult<std::vector<Book>> result;
    result.success = true;
    result.data = bookDao.searchBooks(keyword);
    result.message = result.data.empty() ? "未查询到匹配图书。" : "图书搜索完成。";
    return result;
}

ActionResult updateExistingBook(BookDao& bookDao, const Book& book, long long operatorId) {
    ActionResult result;
    result.success = bookDao.updateBook(book, operatorId);
    result.message = result.success ? "修改图书成功！" : "修改图书失败！";
    return result;
}

ActionResult removeBook(BookDao& bookDao, long long bookId, long long operatorId) {
    ActionResult result;
    result.success = bookDao.deleteBook(bookId, operatorId);
    result.message = result.success ? "删除图书成功！" : "删除图书失败！";
    return result;
}

ActionResult createCategory(CategoryDao& categoryDao, const BookCategory& category, long long operatorId) {
    ActionResult result;
    result.success = categoryDao.addCategory(category, operatorId);
    result.message = result.success ? "新增分类成功！" : "新增分类失败！";
    return result;
}

DataResult<std::vector<BookCategory>> listCategories(CategoryDao& categoryDao) {
    DataResult<std::vector<BookCategory>> result;
    result.success = true;
    result.data = categoryDao.getAllCategories();
    result.message = result.data.empty() ? "当前暂无分类数据。" : "查询分类列表成功。";
    return result;
}

DataResult<std::optional<BookCategory>> findCategoryById(CategoryDao& categoryDao, int categoryId) {
    DataResult<std::optional<BookCategory>> result;
    result.data = categoryDao.getCategoryById(categoryId);
    result.success = result.data.has_value();
    result.message = result.success ? "查询分类成功。" : "未找到该分类！";
    return result;
}

ActionResult updateExistingCategory(CategoryDao& categoryDao, const BookCategory& category, long long operatorId) {
    ActionResult result;
    result.success = categoryDao.updateCategory(category, operatorId);
    result.message = result.success ? "修改分类成功！" : "修改分类失败！";
    return result;
}

ActionResult removeCategory(CategoryDao& categoryDao, int categoryId, long long operatorId) {
    ActionResult result;
    result.success = categoryDao.deleteCategory(categoryId, operatorId);
    result.message = result.success ? "删除分类成功！" : "删除分类失败！";
    return result;
}
