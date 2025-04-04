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

// srcからnバイト分をdstにコピー
void *memcpy(void *dst, const void *src, size_t n) {
  // 1byteずつ処理するためにuint8_tにキャスト
  uint8_t *d = (uint8_t *) dst;
  const uint8_t *s = (const uint8_t *) src;
  while (n--)
    *d++ = *s++; // sの値をdにコピーしてそれぞれ次のアドレスへ進める
  return dst;
}

// bufの先頭からnバイト分をcで埋める
void *memset(void *buf, char c, size_t n) {
  // 汎用ポインタを符号なし整数型ポインタにキャスト(1byteずつ処理するため)
  uint8_t *p = (uint8_t *)buf;
  while(n--)
  // ポインタ変数のインクリメント→アドレスを1byte進める
    *p++ = c;
  return buf;
}

// dst < src の場合、dstが確保していた領域をはみ出して書き込もうとするため、バッファオーバーフローが起こる
char *strcopy(char *dst, const char *src) {
  // コピー先変数の先頭アドレスを保持するために、走査用にポインタ変数を用意する
  char *d = dst;
  // \0が来るまで
  while (*src)
    *d++ = *src++;
  // 終端文字を忘れずにつける
  *d = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2) {
    if (*s1 != *s2) {
      break;
    }
    s1++;
    s2++;
  }
   return *(unsigned char *)s1 - *(unsigned char *)s2;
}
