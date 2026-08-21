# travelers-board-split Bootloader 決定記録

> **同期ルール:** この文書と `BOOTLOADER.md` は常に同一内容に保ちます。
> `BOOTLOADER.md` は ZMK ファームウェア作成リポジトリへ渡す英語版です。

## 対象と決定事項

これは `travelers-board-split` の bootloader 用バリアントです。既存の
ZMK 直接起動用定義は、比較用ビルドおよび初期のフォールバック経路として
変更しません。

| 項目 | 決定 |
| --- | --- |
| MCU | nRF52840 QIAA / NINA-B302-00B |
| 通常更新 | USB UF2 mass storage |
| 復旧と初回書込み | OpenOCD または同等のプローブによる SWD |
| SoftDevice | S140 v7.3.0 |
| OTA/BLE DFU | 採用しない。`DUALBANK_FW` は無効 |
| 署名付き更新 | 採用しない。`SIGNED_FW` は無効 |
| DFU ボタンと LED GPIO | 未定義。物理接続が未確認 |
| UF2 起動 | 物理 RESET を 500 ms 以内に 2 回押す |
| 暫定 USB identity | `0x1209:0x000A`。私的テスト専用であり、配布前に置換する |

`DUALBANK_FW` は OTA 専用ではありません。旧式 Nordic DFU transport 用に
第 2 application bank を確保する機能ですが、UF2 には不要で、利用可能な
application 容量を半減させます。

## 採用 Flash map

すべての境界は 4 KiB の Flash page に揃えています。対応する ZMK の
bootloader 用バリアントを正とし、完全に同じ境界を使用してください。

| 領域 | 範囲 | サイズ | 用途 |
| --- | ---: | ---: | --- |
| MBR | `0x00000000-0x00000FFF` | 4 KiB | Nordic MBR |
| S140 v7.3.0 | `0x00001000-0x00026FFF` | 152 KiB | Nordic SoftDevice |
| code_partition | `0x00027000-0x000D3FFF` | 692 KiB | ZMK application |
| storage_partition | `0x000D4000-0x000F3FFF` | 128 KiB | ZMK NVS/settings と bonds |
| bootloader executable | `0x000F4000-0x000FD7FF` | 38 KiB | この bootloader |
| bootloader config | `0x000FD800-0x000FDFFF` | 2 KiB | CF2 configuration |
| MBR parameters | `0x000FE000-0x000FEFFF` | 4 KiB | Nordic MBR parameter page |
| bootloader settings | `0x000FF000-0x000FFFFF` | 4 KiB | Nordic bootloader settings |

secondary application partition はありません。bootloader 用 ZMK image は
`0x000AD000` bytes（692 KiB）以内でなければなりません。この Flash map を
片方のリポジトリだけで変更しないでください。

## 起動シーケンスと通常更新

```text
RESET -> MBR -> S140 v7.3.0 -> bootloader at 0xF4000
      -> valid ZMK image at 0x27000 -> ZMK
      -> 500 ms 以内の 2 回目の reset -> USB UF2 + CDC bootloader
```

有効な application がない場合、bootloader は USB DFU のまま待機します。
double-reset は確認済みの物理 RESET/NRST 経路を必要とし、matrix GPIO を
DFU trigger には使用しません。

通常更新では、RESET を 500 ms 以内に 2 回押し、`TRAVELERS` volume が
現れたら bootloader 用 Flash map でビルドした ZMK UF2 をコピーします。
更新が完了すると volume は切断されます。UF2 は署名なしの raw-flash
更新です。`0x27000` に link され、`code_partition` 内だけを書き換える
image だけを使用してください。

## ビルド環境

以下の Windows/MSYS2 UCRT64 手順は、提供された setup note を基にしたもの
です。CMake は 3.17 以上が必要で、古いバージョンはこのリポジトリで拒否
されます。

```bash
pacman -Syu
pacman -S \
  git \
  python \
  python-pip \
  mingw-w64-ucrt-x86_64-arm-none-eabi-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-openocd

python -m pip install --user intelhex
git submodule update --init --recursive
```

ビルド前に必要な tool を確認します。

```bash
python -c "import intelhex; print('intelhex OK')"
arm-none-eabi-gcc --version
cmake --version
ninja --version
openocd --version
```

必要に応じて、`/ucrt64/bin` が `PATH` に入っている MSYS2 UCRT64 shell から
実行してください。この手順では `-G Ninja` で Ninja generator を選ぶため、
configure 後は `make` ではなく `ninja build` を使用します。

```bash
cmake -S . -B build/travelers-board-split -G Ninja \
  -DBOARD=travelers-board-split \
  -DPython_EXECUTABLE=/ucrt64/bin/python.exe
cd build/travelers-board-split
ninja build
```

CMake build は `build/travelers-board-split/` に次の artifact を生成します。

| File | 用途 |
| --- | --- |
| `bootloader.hex` | MBR/S140 を含まない bootloader 本体 |
| `bootloader_mbr.hex` | MBR を含む bootloader。S140 は別途書き込む |
| `bootloader_mbr.uf2` | bootloader 自己更新用 UF2。ZMK application UF2 ではない |

S140 v7.3.0 は
`lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_softdevice.hex` から読み込みます。

## 初回 SWD 書込みと全消去後の復旧

SWDIO、SWCLK、GND、VDD へのアクセスは確認済みです。以下は ST-LINK の例です。
別の debug probe を使う場合は interface configuration を置き換えてください。

```bash
openocd -f interface/stlink.cfg -f target/nrf52.cfg
```

別 terminal から OpenOCD telnet server に接続します。

```bash
telnet localhost 4444
```

初回書込みまたは全消去後の復旧では、OpenOCD terminal で次を実行します。
順序が重要です。最初に S140、次に MBR と bootloader image を書き込みます。

```tcl
reset halt
program /absolute/path/to/lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_softdevice.hex verify
program /absolute/path/to/build/travelers-board-split/bootloader_mbr.hex verify reset
shutdown
```

erase-all を行うと bootloader、SoftDevice、ZMK application、NVS settings、BLE
bonds が削除されます。上記の 2 コマンドを再実行して復旧し、続けて
bootloader 用 Flash map でビルドした ZMK image を書き込みます。bootloader を
導入した後に、`0x00000000` の直接起動用 ZMK image を書き込んではいけません。

## ZMK リポジトリへの引渡し要件

直接起動用 board を変更せず、別の bootloader 用 variant を作成してください。
`CONFIG_BOOTLOADER_MCUBOOT=n` を維持します。この bootloader は MCUboot では
ありません。別 variant では UF2 output を有効にします。bootloader に ZMK USB
は不要なため、ZMK USB は無効のままでも構いません。

DTS では `code_partition` を選択し、以下の全領域を予約します。

```dts
/ {
    chosen {
        zephyr,code-partition = &code_partition;
    };
};

&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;

        mbr_partition: partition@0 {
            label = "mbr";
            reg = <0x00000000 0x00001000>;
            read-only;
        };
        s140_partition: partition@1000 {
            label = "s140";
            reg = <0x00001000 0x00026000>;
            read-only;
        };
        code_partition: partition@27000 {
            label = "code";
            reg = <0x00027000 0x000AD000>;
        };
        storage_partition: partition@d4000 {
            label = "storage";
            reg = <0x000D4000 0x00020000>;
        };
        bootloader_partition: partition@f4000 {
            label = "bootloader";
            reg = <0x000F4000 0x0000C000>;
            read-only;
        };
    };
};
```

したがって ZMK application reset vector は `0x00027000` に link されます。
final image size が `0x000AD000` bytes 以下であることを確認してください。
SoftDevice と ZMK/BLE の共存は hardware validation 項目です。利用開始前に、
通常起動、BLE、settings persistence、UF2 update、double reset、全 SWD 復旧を
実機で検証してください。

## 未解決項目

- 再配布前に test-only USB VID/PID を置き換える。
- USB enumeration と double-reset の実機動作を最終確認する。
- S140 v7.3.0 および移動後の vector table で ZMK が動作することを確認する。
- 物理 GPIO mapping の確認後にだけ、DFU button または LED 定義を追加する。
