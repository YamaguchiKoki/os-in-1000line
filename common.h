#pragma once

#define PAGE_SIZE 4096

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
// 物理アドレス
typedef uint32_t paddr_t;
// 仮想アドレス
typedef uint32_t vaddr_t;

#define true  1
#define false 0
#define NULL  ((void *) 0)
// valueをalignの倍数に切り上げる。alignは2のべき乗である必要がある。
#define align_up(value, align)   __builtin_align_up(value, align)
// valueがalignの倍数かどうかを判定する。alignは2のべき乗である必要がある。
#define is_aligned(value, align) __builtin_is_aligned(value, align)
// 構造体のメンバのオフセット (メンバが構造体の先頭から何バイト目にあるか) を返す。
#define offsetof(type, member)   __builtin_offsetof(type, member)

//可変長引数を扱うマクロ群。clangの組み込み関数を使用
#define va_list  __builtin_va_list
#define va_start __builtin_va_start
#define va_end   __builtin_va_end
#define va_arg   __builtin_va_arg

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);

void printf(const char *fmt, ...);
