# Problem 071 - 集群调度

**ACMOJ Problem ID**: 2298

## Table of Contents

- [Problem 071 - 集群调度](#problem-071-集群调度)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Background](#background)
  - [Assignment Description](#assignment-description)
    - [Grade Composition](#grade-composition)
  - [Assignment Requirements](#assignment-requirements)
    - [Input Format](#input-format)
    - [Output Format](#output-format)
    - [Samples](#samples)
    - [Data Constraints](#data-constraints)
  - [Per-Testcase Resource Limits](#per-testcase-resource-limits)
  - [Test Data](#test-data)
  - [Submission Requirements](#submission-requirements)
    - [Submission Format](#submission-format)
    - [Submission Guidelines](#submission-guidelines)
    - [Evaluation Notes](#evaluation-notes)
    - [Academic Integrity](#academic-integrity)

## Introduction

### Background

Implement cluster scheduling system: design client requests and server scheduling for cloud computing. Interactive optimization.

## Assignment Description

### Grade Composition

| Grading Component | Percentage |
| :--: | :--: |
| Pass **2298. 集群调度** | 80% |
| Code Review | 20% |

Here are several points that need clarification:

- In the Code Review, we will **strictly examine your code style and repository organization structure, etc.**

- This assignment provides some sample data for testing, stored in the `/workspace/data/071/data_test/` directory. Note that these are not the test cases on the Online Judge. Passing all local test cases does not guarantee that you will pass the OJ tests.

- Besides the provided sample data, we also encourage you to design your own test data based on your program logic to assist debugging.

## Assignment Requirements

### Problem Description

update (13:10): $t$ 个时间片可以完成的任务量为 **$t\times k^c$**

tips (14:44): 大家提交之前可以先在本地跑一下，至少保证自己的 server 不要在自己的 client 上 SLO 满足率为 0。

notice (14:59): 取消一个 task 后可以认为该任务占据的服务器资源被立即释放。

notice (14:59): `schedule_tasks` 的入参中 `list` 代表当前时刻新到达的任务，若你想要知道此前任务的信息，可以自己通过 static 或全局变量进行存储。

notice (15:47): 提交的时候请不要向 stdout 进行任何输出，否则评测时可能出现问题。

**fatal fix** (16:03): 修复了 `runtime.h` 的已知问题、并加入了 log 功能，请在 [attachment]runtime.h[/attachment] 中获取最新版。

- 请注意，如果你在之前的提交中出现了 DDL 后尝试 save 的行为，你可以重新提交。

---

集群调度（cluster scheduling）一直是云计算与分布式系统领域的热门话题，在本次机考中，你将扮演客户与云服务厂商，设计刁钻的请求与高效的调度方案。

问题可以被简化为以下形式：集群中有 $a$ 台同构的服务器，在 $T$ 个时间片里会依次有 $m$ 个客户请求在线地到来，第 $i$ 个请求包含以下信息：
- 该请求的任务量 $w_i$
- 该请求的服务级别目标（Service Level Objective，SLO），即要求该请求在第 $ddl_i$ 个时间片之前完成
- 该任务的优先级 $p_i$

对于每一个客户请求，云服务厂商可以在任意时刻将其部署在占有 $k$ 台服务器的容器中，其中 $k\ge 1$，且同一时间片所有容器占据的服务器总数不超过 $a$。以一个模型训练任务为例，一个容器的执行通常会分为三个阶段：
1. 冷启动阶段（加载模型到指定设备并启动运行环境），需要 $\textit{kStartUp}$ 个时间片。
2. 执行阶段（进行模型训练），执行时间由云服务厂商决定，$t$ 个时间片可以完成的任务量为 $t\times k^c$，其中 $c$ 是给定的常数。
3. 保存阶段（将模型权重保存到硬盘，从而返回给用户或作为 checkpoint 用于下一次继续执行），需要 $\textit{kSaving}$ 个时间片。在保存后，若任务已经完成，则可以直接返回给客户端结束该请求；若任务未完成，随后可以在本次的基础上**继续执行**，但注意**需要再次进行容器的冷启动**。

在上述三个阶段，容器将完全占据 $k$ 台服务器，若你在扮演云服务厂商时错误地使得某一时刻所有容器需要的服务器总和超过 $a$，你的调度将被视为**完全失败**。当然，若你在某一时刻发现前面作出了错误的调度，且你需要立即更改（比如，有一个优先级高且时间需求短的任务到来），你可以取消正在运行的任务，但你将丢失**本次执行**的所有进度。

在本次机考中我们不考虑容器的负载动态调整，即一个任务的一个容器执行过程中不能调整其所占据的服务器个数，但在当前任务下一次执行时可以为新的容器指定不同的 $k$。

总的来讲，你需要完成的任务由以下两部分组成：

#### 客户端

你需要实现一个生成器，以生成刁钻的请求刁难其他同学的服务端。具体函数签名参考 `interface.h`。

其中，我们要求你返回一个任务列表，且满足传入的 Description。 如果生成的列表不满足 Description，则认为生成失败。Description 包括:
- 服务器数量，即上述 $a$
- 任务数量，即上述 $m$
- 生成的任务的 deadline 时间区间
- 单个任务任务量的区间
- 所有任务总任务量的区间
- 单个任务优先级的区间
- 所有任务优先级总和的区间

#### 服务端

你需要实现一个调度器，以满足其他同学的要求。具体函数签名参考 `interface.h`。

其中，我们要求你在每一个时间片，根据此前任务的情况与函数传入的当前时间片新的客户端请求，作出当前时间片的决策。其中可能包含对于此前正在执行的任务的操作（撤销或保存）或启动新的任务，并指定分配的服务器数目。你的函数将会被调用 $\max ddl$ 次。

假设你满足了编号为 $y_1,\cdots,y_n$ 的请求，那么你的整体 SLO 满足率为 $ \textit{SLO}=\frac{\sum_{i=1}^{n}{p_{y_i}}}{\sum_{i=1}^{m}{p_i}} $。

#### 评分方式

本次机考不使用主 OJ 进行评分，请移步 http://10.80.75.141/OnlineJudge 。

我们将会把所有人生成的 scheduling plan 和 data 放在一起测试，对于每一组调度方和数据生成方（以下简记为防守方、进攻方）：记防守方在此轮数据中的 SLO 满足率为 $x$，进攻方对自己生成的数据的满足率为 $y$，那么防守方在此轮中的得分为 $\min\\{\frac{x}{y}-1,1\\}$，进攻方的得分为其相反数。

如果进攻方生成的数据不满足要求，或者进攻方对于自己生成的数据生成的调度非法，或者 $y = 0$，那么防守方直接获得 $1$ 分。

在进攻方生成的数据合法， $y \ne 0$ 的基础上，如果防守方生成的调度不满足要求，那么防守方得 $-1$ 分。

#### 下发代码

请查看附件 [attachment]oj.zip[/attachment]

### 其他提示

#### OJ

##### 下发文件

我们下发与 OJ 几乎一致的相关文件。

`interface.h` 规定了你要实现的函数。

`definition.h` 中，有所有类型的定义，以及我们 oj 上的测试点相关的 Description 信息 (testcase_array)。同时，我们也把部分常数信息放在了 `PublicInformation` 类里面，你可以在你的代码中使用这些信息。

`runtime.h` 是我们 OJ 运行时用到的模拟器相关代码。你可以观察 `runtime.h` 中的 `RuntimeManager` 来了解我们 OJ 具体测评相关的细节。

`src.hpp` 是你需要实现对应接口的头文件。

`client.cpp`, `server.cpp`, `client_spj.cpp`, `server_spj.cpp` 是我们 OJ 测评用到的文件，你不应该修改这些文件。

我们的测评流程是:

从 stdin 读入 `0 ~ 3` 中某个数字 -> `client.cpp` -> `client_spj.cpp` -> `server.cpp` -> `server_spj.cpp`

在本地测评的时候，只需要使用 `run.sh` 即可。

##### 关于 gcc 版本

我们要求 gcc 的版本至少是 11， 编译参数请参考 `run.sh`。 如果你要安装 gcc 11 的话，请参考：

```shell
lsb_release -a # 查看你的 ubuntu 版本
# for ubuntu 20.04
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11

# for ubuntu 22.04
gcc --version # 如果这里已经显示是 11.*，就好了
sudo apt update
sudo apt install build-essential
gcc --version

# 运行程序
cd oj
sh run.sh
# 输入 0 ~ 3 的数字即可运行对应的测试点
```

##### 关于本地评测和 OJ 提交

你可以在命令行输入 `sh run.sh` 来测试你的 `generator` 和 `scheduler` 的正确性。

在上传到 OJ 的时候，你的 `src.hpp` 中不需要且不应该 `#include "interface.h"` 和 `#include "definition.h"`。

##### 关于某些 C++ 问题

首先，下发的所有 C++ 代码需要 `-std=c++20`。

在 Policy 类中，我们用到了 `std::variant`. 简单来说，对于一个 `std::variant<A,B>`，其要么存一个 `A` 类要么存一个 `B` 类，且支持从由 `A` 类构造或者由 `B` 类构造。你可以参考 [std::variant - cppreference.com](https://en.cppreference.com/w/cpp/utility/variant) 来了解更多。

##### 关于 hack 的部分

我们 **禁止** 在 `generator` 和 `scheduler` 之间直接进行通信。我们 **禁止** 利用 `oj` 测评端漏洞获取分数，以及 (在 scheduler 侧) 通过非法手段提前获取在 **当前时刻之后到来的数据**。如果恶意进攻，本次机考计为 $0$ 分。

当然，如果你在 `scheduler` 里面通过特判来识别是否是自己 `generator` 生成的数据，我们也没办法阻止。

### Input Format

See the problem description above.

### Output Format

See the problem description above.

### Samples

No sample data provided for this problem.

### Data Constraints

See the problem description for constraints.

## Per-Testcase Resource Limits

- **Time Limit (per test case)**: 1000 ms
- **Memory Limit (per test case)**: 512 MiB
- **Disk Usage**: No disk usage is permitted.

## Test Data

The test data for this problem is located at `/workspace/data/071/data_test/`.

Attachments and starter files for this problem are located at `/workspace/data/071/`.

## Submission Requirements

### Submission Format

Submit a single C++ source file to the Online Judge. The OJ provides its own `main.cpp` which `#include "src.hpp"`. Your submission will be written to `src.hpp` and compiled together with the OJ's driver code.

### Submission Guidelines

- The submitted code must be able to compile successfully through the above compilation process
- The program needs to be able to read data from standard input and write results to standard output
- Please ensure the code runs correctly within the given time and space limits
- **You must use C++ or C language** to implement this assignment

### Evaluation Notes

- The evaluation system will test your program using the provided test data
- The program output must exactly match the expected output (including format)
- Exceeding time or memory limits will be judged as the corresponding error type
- Please pay attention to the overall time performance of your code and the time complexity of each part of your algorithm.

### Academic Integrity

If any violations are found during evaluation or code review (including but not limited to using unconventional methods to pass test cases), your final score may be significantly reduced or become **0 points**.
