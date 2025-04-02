typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

// シンボル リンカによって作成されたラベルをアドレスとして参照するための宣言
// リンカスクリプトで、__bss：Block Started by Symbol（初期値を持たない変数やstatic変数が格納される領域）の先頭アドレス, __bss_endは未初期化データ領域の末尾アドレス
// __stack_top：スタックの末尾アドレス
extern char __bss[], __bss_end[], __stack_top[];

// bufの先頭からnバイト分をcで埋める
void *memset(void *buf, char c, size_t n) {
  // 汎用ポインタを符号なし整数型ポインタにキャスト(1byteずつ処理するため)
  uint8_t *p = (uint8_t *)buf;
  while(n--)
    *p++ = c;
  return buf;
}

void kernel_main(void) {
  memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);
  for (;;);
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
