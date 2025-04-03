#include "common.h"

void putchar(char ch);

void printf(const char *fmt, ...) {
  va_list vargs;
  va_start(vargs, fmt);

  while(*fmt) {
    if(*fmt == '%') {
      fmt++;
      switch(*fmt) {
        case '\0':
          putchar('%');
          goto end;
        case '%':
          putchar('%');
          break;
        // 文字列を表示
        case 's':
          const char *s = va_arg(vargs, const char *);
          // 終端文字が来るまで回る
          while(*s) {
            putchar(*s);
            s++;
          }
          break;
        // 10進数整数を表示
        case 'd':
          int value = va_arg(vargs, int);
          // 符号なしbitで扱う
          // 桁溢れが起きた時の動作を定義するため（intならオーバフローになるがunsignedだと自動的にmodが取られるらしい）
          unsigned magnitude = value;
          if (value < 0) {
            // 一桁ずつ出力するため各桁は非負整数で扱う
            putchar('-');
            magnitude = -magnitude;
          }

          unsigned divisor = 1;
          while (magnitude / divisor > 9) {
            divisor *= 10;
          }

          while (divisor > 0) {
            putchar('0' + magnitude / divisor);
            magnitude %= divisor;
            divisor /= 10;
          }
          break;
        // 16進数整数を表示
        case 'x': {
            unsigned value = va_arg(vargs, unsigned);
            for (int i = 7; i >= 0; i--) {
                unsigned nibble = (value >> (i * 4)) & 0xf;
                putchar("0123456789abcdef"[nibble]);
            }
            break;
        }
      }
    } else {
      putchar(*fmt);
    }
    fmt++;
  }
end:
  va_end(vargs);
}
