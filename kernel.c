#include "kernel.h"
#include "common.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

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

void kernel_main(void) {
    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    for (;;) {
        __asm__ __volatile__("wfi");
    }
}

// boot関数をtext.bootセクションに配置する
// リンカスクリプトで、text.bootセクションはROMの先頭に配置される
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
