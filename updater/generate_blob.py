#!/usr/bin/env python3
from pathlib import Path
import math
import sys

SECTOR_SIZE = 16 * 1024

def format_c_array(data: bytes, bytes_per_line: int = 16) -> str:
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk))
    return ",\n".join(lines)

def main() -> int:
    if len(sys.argv) < 2:
        print("usage: generate_blob.py firmware.bin [output_prefix]")
        return 1

    input_path = Path(sys.argv[1])
    prefix = sys.argv[2] if len(sys.argv) >= 3 else "firmware_blob"

    if not input_path.exists():
        print(f"error: file not found: {input_path}")
        return 1

    data = input_path.read_bytes()
    size = len(data)
    sector_count = math.ceil(size / SECTOR_SIZE)

    h_path = Path(f"{prefix}.h")
    c_path = Path(f"{prefix}.c")

    h_text = f"""#ifndef {prefix.upper()}_H
#define {prefix.upper()}_H

#include <stdint.h>

#define FW_BLOB_SIZE {size}UL
#define FW_BLOB_SECTOR_COUNT {sector_count}u

extern const uint8_t firmware_blob[];

#endif
"""

    c_array = format_c_array(data if data else b"\x00")

    c_text = f"""#include "{prefix}.h"

const uint8_t firmware_blob[] = {{
{c_array}
}};
"""

    h_path.write_text(h_text, encoding="ascii")
    c_path.write_text(c_text, encoding="ascii")

    print(f"generated {h_path}")
    print(f"generated {c_path}")
    print(f"blob size: {size} bytes")
    print(f"sector count: {sector_count}")

    if sector_count > 4:
        print("warning: target updater may only support sectors 0..3 unless FW_UPDATE_SECTOR_COUNT is increased")

    return 0

if __name__ == "__main__":
    raise SystemExit(main())