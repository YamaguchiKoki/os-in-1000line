#pragma once

struct sbiret {
    long error;
    long value;
};
// 複数行マクロを書く時のイディオム（プリプロセッサによるマクロ展開の仕様上、呼び出し方によっては意図しないエラーになるため）
#define PANIC(fmt, ...) \
    do { \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        while (1) {} \
    } while (0)
