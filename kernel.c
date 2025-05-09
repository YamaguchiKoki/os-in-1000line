#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

struct process procs[PROCS_MAX];

// Sv32では32ビットの仮想アドレスをVPN[1](10bit), VPN[0](10bit), offset(12bit)で扱う
// 仮想アドレス→物理アドレスへのマッピング処理の流れ
// 1.satpレジスタが保持している際上位のページテーブルアドレス（物理）にアクセス
// 2. 第一階層のページテーブルに対して、VPN[1]をインデックスとしてアクセスし、第二階層のページテーブルのアドレスを得る
// 3. 第二階層のページテーブルに対して、VPN[0]をインデックスとしてアクセスし、offsetとたしあわせて物理アドレスを得る
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
  if (!is_aligned(vaddr, PAGE_SIZE))
    PANIC("unaligned vaddr %x", vaddr);

  if (!is_aligned(paddr, PAGE_SIZE))
    PANIC("unaligned paddr %x", paddr);

  // 22bit右にシフトして、１０ビットのビットでマスクしてVPN[1]を得る
  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
  // *(table1 + vpn1)と同じ意味
  if ((table1[vpn1] & PAGE_V) == 0) {
    // 2段目のページテーブルが存在しないので作成する
    uint32_t pt_paddr = alloc_pages(1);
    table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
  }

  // 2段目のページテーブルにエントリを追加する
  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
  uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
  table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;


}

__attribute__((naked)) void switch_context(
  uint32_t *prev_sp,
  uint32_t *next_sp
) {
  __asm__ __volatile__(
        // 実行中プロセスのスタックへレジスタを保存
        // spは高位のアドレスから末尾に向かって伸びるため、一時保存のために53バイト確保している
        "addi sp, sp, -13 * 4\n"
        // レジスタの値をスタックポインタに書き込んでいる(4バイトずつずらしながら)
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // a0: 第一引数 現在のスタックポインタが指す値をprev_spに保存
        "sw sp, (a0)\n"
        // next_spを現在のスタックポインタに格納
        "lw sp, (a1)\n"

        // 次のプロセスのスタックからレジスタを復元
        "lw ra,  0  * 4(sp)\n"
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        // 確保領域を元に戻す
        "addi sp, sp, 13 * 4\n"
        "ret\n" // raから実行
    );
}

struct process *create_process(uint32_t pc) {
    // 空いているプロセス管理構造体を探す
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // switch_context() で復帰できるように、スタックに呼び出し先保存レジスタを積む
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) pc;          // ra

    // 各フィールドを初期化
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    return proc;
}

extern char __free_ram[], __free_ram_end[];

//フリーRAM領域からページ単位で動的にメモリを割り当てる簡易的なアルゴリズム(Bumpアロケータ)
paddr_t alloc_pages(uint32_t n) {
  // staticとして宣言すると、関数が終了してもメモリから消えない上(localとの違い)、この関数からしかアクセスできない(globalとの違い)。
  static paddr_t next_paddr = (paddr_t) __free_ram;
  paddr_t paddr = next_paddr;
  next_paddr += n * PAGE_SIZE;

  if (next_paddr > (paddr_t) __free_ram_end)
    PANIC("out of memory");

  memset((void *) paddr, 0, n * PAGE_SIZE);
  return paddr;
}

// シンボル リンカによって作成されたラベルをアドレスとして参照するための宣言
// リンカスクリプトで、__bss：Block Started by Symbol（初期値を持たない変数やstatic変数が格納される領域）の先頭アドレス, __bss_endは未初期化データ領域の末尾アドレス
// __stack_top：スタックの末尾アドレス
extern char __bss[], __bss_end[], __stack_top[];

// S-Mode(カーネル)からM-Mode(OpenSBI)を呼び出す
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                long arg5, long fid, long eid) {
    // レジスタに値を設定
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;
    // ecall : 上位のモードに制御を移す。例外を発生させて、常駐OpenSBIはそれを捕捉し、OSーハード間の差異を吸収する
    __asm__ __volatile__("ecall"
                        : "=r"(a0), "=r"(a1)
                        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                          "r"(a6), "r"(a7)
                        : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar（SBI） */);
}

void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

/*
各レジスタの値をスタックに保存しておき、トラップハンドラを呼び出す→ハンドリング完了後、レジスタから値をよみだしsret命令で前の状態に戻る
*/
__attribute__((naked))
__attribute__((aligned(4))) // 関数のアラインメント（配置）を4バイト境界に揃えることで、RISC-Vの命令フェッチやデータアクセスの効率を向上させる
void kernel_entry(void) {
    __asm__ __volatile__(
        // 実行中プロセスのカーネルスタックをsscratchから取り出す
        // tmp = sp; sp = sscratch; sscratch = tmp;
        "csrw sscratch, sp\n"
        "addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        // 例外発生時のspを取り出して保存
        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        // カーネルスタックを設定し直す
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        "mv a0, sp\n"
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

struct process *current_proc; // 実行中プロセス
struct process *idle_proc; // アイドルプロセス

void yield(void) {
  struct process *next = idle_proc;
  for (int i = 0; i < PROCS_MAX; i++) {
    struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
    if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
      next = proc;
      break;
    }
  }

   // 現在実行中のプロセス以外に、実行可能なプロセスがない。戻って処理を続行する
  if (next == current_proc)
    return;

  __asm__ __volatile__(
      "csrw sscratch, %[sscratch]\n"
      :
      : [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])
  );

  struct process *prev = current_proc;
  current_proc = next;
  switch_context(&prev->sp, &next->sp);
}

void delay(void) {
  for (int i = 0; i < 30000000; i++) {
    __asm__ __volatile__("nop"); // 何もしない
  }
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
  printf("starting process A \n");
  while(1) {
    putchar('A');
    yield();
    delay();
  }
}

void proc_b_entry(void) {
  printf("starting process B \n");
  while(1) {
    putchar('B');
    yield();
    delay();
  }
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    idle_proc = create_process((uint32_t) NULL);
    idle_proc->pid = 0;
    current_proc = idle_proc;

    proc_a = create_process((uint32_t) proc_a_entry); // 関数が配置されているアドレスとして渡す
    proc_b = create_process((uint32_t) proc_b_entry);

    yield();

    PANIC("switched to idle process");
}

// boot関数をtext.bootセクションに配置する
// リンカスクリプトで、text.bootセクションはROMの先頭に配置される
// ブートコードがROMで実行された後、RAMに配置されたkernel_mainへ制御が移る
__attribute__((section(".text.boot")))
// コンパイラによる関数エピローグ/プロローグの生成を抑制する
__attribute__((naked))
void boot(void) {
  __asm__ __volatile__(
    // mv: あるレジスタの値を別のレジスタにコピーする, sp: スタックポインタレジスタ
    // ->スタックポインタレジスタに__stack_topを設定(スタックは０に向かって伸びるため末尾アドレス)
    "mv sp, %[stack_top]\n"
    "j kernel_main\n"
    :
    : [stack_top] "r"(__stack_top)
  );
}


