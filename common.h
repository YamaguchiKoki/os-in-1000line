#pragma once

//可変超引数を扱うマクロ群。clangの組み込み関数を使用
#define va_list  __builtin_va_list
#define va_start __builtin_va_start
#define va_end   __builtin_va_end
#define va_arg   __builtin_va_arg

void printf(const char *fmt, ...);
