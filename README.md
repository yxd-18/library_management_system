# 图书管理系统

基于 `C++`、`PostgreSQL` 和 `Qt Widgets` 实现的图书管理系统课程项目。项目包含控制台版核心业务程序、数据库脚本、Qt 桌面前端以及测试与文档辅助脚本，可用于演示图书管理、用户管理、借阅管理、系统维护和基础测试流程。

## 项目组成

- 控制台核心程序：位于仓库根目录，负责用户管理、图书管理、借阅业务、系统维护等主要功能。
- 数据库脚本与配置样例：负责 PostgreSQL 建库、初始化和测试数据导入。
- Qt 桌面前端：位于 `qt_widgets/`，提供登录界面、管理员工作台、读者工作台和查询交互界面。
- 测试与文档辅助脚本：位于 `scripts/`、`doc_sections/`、`generated_test_figures/`、`report_images/` 等目录。

## 技术栈

- 语言：`C++`
- 数据库：`PostgreSQL`
- 前端：`Qt Widgets`
- 构建环境：`Visual Studio`、`CMake`
- 文档与测试辅助：`PowerShell`、`Python`

## 团队成员与分工

以下分工与实验报告保持一致：

| 学号 | 姓名 | 工作量占比 | 主要职责 |
| --- | --- | --- | --- |
| 20236125 | 袁小迪 | 约 32.5% | 组长；负责项目分工、进度推进、讨论沟通、系统集成；完成大部分管理系统核心代码与联调修复。 |
| 20235948 | 叶瑞卿 | 约 15% | 负责 PostgreSQL 数据库搭建、数据库环境配置、初始化脚本执行、测试数据准备与数据库问题排查。 |
| 20235897 | 宁嘉博 | 约 15% | 负责需求分析与系统设计文档主体撰写、数据库设计说明、用例与流程分析整理，并协助测试用例整理。 |
| 20235921 | 林晓彤 | 约 12.5% | 负责最终研发文档统稿、章节整合、格式规范化、截图与图表整理，并协助测试结果汇总。 |
| 20232365 | 张英鹏 | 约 25.5% | 负责 Qt 前端界面编写与交互实现，完成登录界面、管理员工作台、读者工作台及部分查询展示功能。 |

## 仓库目录说明

- `qt_widgets/`：Qt 桌面端源码与构建文件。
- `scripts/`：数据库检查、测试数据导入、文档更新等辅助脚本。
- `doc_sections/`：研发文档分章节文本素材。
- `generated_test_figures/`：系统测试章节中使用的数据库查询图与集成测试图片。
- `report_images/`：研发文档与测试相关图片素材。
- 根目录 `*.cpp`、`*.h`：控制台版图书管理系统主体源码。
- `library_management_system_postgresql.sql`：数据库建库和初始化脚本。

## GitHub 协作建议

如果要在 GitHub 上体现“团队协作完成”，建议按下面方式保留痕迹：

1. 每个成员使用自己的 GitHub 账号接受仓库协作邀请。
2. 每个成员在自己分支上提交自己负责的文件，不要都直接推到 `main`。
3. 每个成员至少发起一次 `Pull Request`，由组长统一审核合并。
4. 在 `Issues` 或 `Projects` 中保留简单的任务分配记录。
5. 在仓库首页保留本 `README.md` 和 `团队协作与提交分工.md`，使老师能直接看到分工与代码对应关系。

建议分支命名：

- `feature/backend-yuanxiaodi`
- `feature/database-yeruiqing`
- `docs/design-ningjiabo`
- `docs/report-linxiaotong`
- `feature/frontend-zhangyingpeng`

## 建议上传到 GitHub 的内容

- 控制台系统源码和工程文件
- `qt_widgets/` 前端源码
- `library_management_system_postgresql.sql`
- `db_config.example.txt`
- `scripts/` 中与数据库、测试、文档处理有关的脚本
- 研发文档相关的 Markdown、图片说明和测试图片素材

## 不建议上传到 GitHub 的内容

以下内容更适合保留在本地或通过压缩包提交，不建议作为源码仓库内容长期保留：

- `db_config.txt`
  - 原因：包含真实数据库账号和密码。
- `.vs/`、`.vscode/`、`build/`、`x64/`、`library_.daf2ced4/`
  - 原因：属于本地构建缓存或 IDE 临时文件。
- `tmp_docx_extract/`、`tmp_frontend_import/`、`tmp_softeng_images/`
  - 原因：属于中间过程目录，不是最终源码。
- 各类 `*.zip`
  - 原因：压缩包适合用于交付，不适合放在源码仓库里反复更新。
- `debug.log`
  - 原因：运行日志不属于最终项目源码。

## 运行说明

- 数据库初始化：执行 `library_management_system_postgresql.sql`，再根据本地环境填写 `db_config.txt`。
- 控制台版：使用 `library_management_system.sln` 在 Visual Studio 中打开并编译运行。
- Qt 桌面版：进入 `qt_widgets/` 后使用 CMake + Qt 环境构建，或参考仓库中的运行说明文件。

## 协作分工文件

具体到“每位成员应该提交哪些文件”，请直接查看：

- `团队协作与提交分工.md`

