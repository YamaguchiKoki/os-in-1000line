ENTRY(boot)/* エントリポイントを決定　*/

SECTIONS {
    . = 0x80200000;/* 開始アドレスを決定　*/

    .text :{/* 実行コードが格納されるセクション　*/
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {/* イミュータブルなデータを格納するセクション　*/
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {/* 初期値を持つstaticな変数を格納するセクション　*/
        *(.data .data.*);
    }

    .bss : ALIGN(4) {/* 初期値を持たないstaticな変数を格納するセクション　*/
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4);/* アドレスを４バイト境界に整列(32bit RISC-なのでn)　*/
    . += 128 * 1024;/* 128KB */
    __stack_top = .;/* スタック領域の予約 関数呼び出し時のローカル変数、戻りアドレス、レジスタ退避領域などで使用　*/

     . = ALIGN(4096);
    __free_ram = .;/* フリーRAMの予約 ヒープ領域、バッファなどで使用 　*/
    . += 64 * 1024 * 1024;/* 64MB */
    __free_ram_end = .;
}
