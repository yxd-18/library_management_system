# Qt Widgets 桌面版界面

这个目录已经整合为一套可继续开发的 Qt Widgets 前端，直接复用项目根目录下已有的 `UserDao`、`BookDao`、`BorrowDao` 和数据库连接逻辑。

## 当前包含

- `main.cpp`
  Qt 程序入口
- `MainWindow.*`
  登录页、管理员工作台、读者工作台，以及搜索、分页、详情和借阅操作入口
- `LibraryRepository.*`
  把现有 DAO 封成 Qt 界面可直接调用的仓储层
- `QtDbSupport.*`
  启动时的数据库配置读取和 schema 升级
- `CMakeLists.txt`
  Qt 工程构建脚本

## 你后面安装 Qt 后怎么编译

1. 安装 Qt 6 或 Qt 5 的 Widgets 组件
2. 打开终端进入 `qt_widgets`
3. 运行：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

## 当前已覆盖的界面能力

- 登录与角色分流
- 管理员视图中的用户、图书、借阅、预约数据展示
- 读者视图中的目录、借阅、预约数据展示
- 基础搜索、分页、详情查看
- 借书、还书、预约、取消预约、续借、挂失、图书维护等仓储层调用入口

## 说明

- 如果需要回看并入前的旧版界面文件，备份位于 `qt_widgets/backup_before_frontend_merge`
- 构建该前端需要本机已安装 Qt Widgets 和 CMake
