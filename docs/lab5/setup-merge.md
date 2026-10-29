# Lab5 实验准备合并记录

本记录描述代码准备工作，不代表 Lab5 功能完成。

## 合并原则及结果

- 已同步教师接口、类型、构建、SBI、PLIC、启动汇编及新增 TODO 骨架。
- start 保留 w_tp/main，并补充 boot_hartid 和 sfence_vma。
- main 采用教师首次/次级入口与初始化顺序，保留由启动核进入首进程。
- uart 保留原换行和退格编辑，合入平台初始化及单独开放 RX。
- kvm 保留页表和内核栈/trampoline 映射，仅在叶项加入 PTE_MACHINE。
- proc 保留已有实现，适配平台生成头文件名称；mmap 初始化仅标 TODO。
- trap_kernel 保留原分派、claim/complete、定时重装；ticks 归属改为 boot_hartid；提示数组保持外部可见。
- trap_user 保留现场处理及用户态返回。旧 helloworld 分派以 #if 0 保留记录；当前 ecall 暂返回 -1，新 syscall 与缺页处理待实现。
- initcode 的旧 helloworld 测试保留为注释，后续由学生选择 Lab5 测试。
- design 采用 Lab5 目标规格并以注释保留旧设计；usertrap 保留已有契约，说明新功能尚未完成。
- print.c、spinlock.c、pmem.c、timer.c 未被教师空实现覆盖。

## 本机兼容性

Ubuntu 的 xxd 2021 不支持教师 Makefile 的 -n 选项，改为 xxd -i 后通过 sed 统一生成变量名称，仍按平台隔离产物。

## 已实际验证

- WSL 中执行 make -j2：已编译启动、SBI、UART、锁、内核页表等，停在教师 mmap.c 的空非 void 函数（-Werror=return-type）。没有通过整树构建。
- 单独强制构建 proc.o、trap_kernel.o、trap_user.o 成功；用户 ELF/bin/initcode.h 生成成功。
- 未运行 QEMU 或开发板；VF2 参数和 Lab5 核心 TODO 未填写。
- git diff --check 报告教师说明及教师规格原有尾随空白；未为格式清理改写这些教师文件。
## 准备状态复查补充

- 修正教师 Makefile 引用 CPUNUM 却未定义的问题：默认两核，并拒绝非 2 的设置。
- 恢复 process.yaml 中被教师空模板替代的 proc_pgtbl_init/proc_make_first 契约，并将首进程归属调整为 boot_hartid；C 实现始终保留。
- 当前属于可开始实现的骨架状态，不属于完整构建/运行通过状态。
- 教师材料存在以下原生矛盾，准备合并不擅自改变其接口或代做作业：
  - syscall.yaml 包含 helloworld/printf 编号 0/7，但教师头文件和分派表只提供 1..6。
  - uvm.yaml/patches 的 copy、mmap、munmap 等返回类型，与教师 method.h/源码签名不一致。
  - uvm.yaml 要求 trapframe 另行释放，而教师 uvm_destroy_pgtbl 已释放 trapframe，存在重复释放风险，任务5实现前须统一所有权。
  - spec 的失败返回/回滚要求比 README 和模板注释严格；旧 pmem_alloc 的耗尽 panic 行为也不满足该目标。这是待明确/实现的功能要求，不能视为已通过。
- mmap_init 调用应位于首进程创建之前；main 当前仅标 TODO，没有代填。

最终复查：make -n run 正确生成 -smp 2；非两核设置按预期拒绝。make -k -j2 继续检查后，编译错误集中在教师 mmap.c/uvm.c 的空返回函数及未使用辅助函数；没有通过链接或运行测试。受保护的 print/spinlock/pmem/timer/trampoline/swtch 与 lab4 比较完全一致。此次合并源码和调整文档的 diff --check 通过。
