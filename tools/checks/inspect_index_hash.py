"""Read a trusted SDI index to identify its persisted HWID hash algorithm."""
import lzma
import struct
import sys
from pathlib import Path

def legacy(value):
    h = 5381
    for c in value:
        h = ((h * 33) ^ (c if c < 128 else c - 256)) & 0xffffffff
    return h

def aphash(value):
    h = 0xaaaaaaaa
    for i, c in enumerate(value):
        if i & 1:
            h ^= ~(((h << 11) & 0xffffffff) + (c ^ (h >> 5)))
        else:
            h ^= ((h << 7) & 0xffffffff) ^ (c * (h >> 3))
        h &= 0xffffffff
    return h

if __name__ == '__main__':
    raw = Path(sys.argv[1]).read_bytes()
    if raw[7] != 0:
        raise SystemExit('BCJ-filtered index: not handled by this diagnostic')
    data = lzma.decompress(raw[8:], format=lzma.FORMAT_ALONE)
    offset = 0
    blocks = []
    for _ in range(5):
        size, count = struct.unpack_from('<II', data, offset)
        offset += 8
        blocks.append(data[offset:offset+size])
        offset += size
    table_size, used, count = struct.unpack_from('<III', data, offset)
    table = data[offset+12:offset+12+used]
    print('Index version:', hex(struct.unpack_from('<I', raw, 3)[0]))
    print('Hash table:', table_size, used, count)
    # Keep raw details available for interpreting the pinned Hashitem layout.
    print('First HWID record:', struct.unpack_from('<III', blocks[3]))
    hwid_offset = struct.unpack_from('<III', blocks[3])[2]
    hwid = blocks[4][hwid_offset:].split(b'\0')[0]
    print('First HWID:', hwid.decode('ascii', 'replace'))
    print('Legacy hash:', hex(legacy(hwid)), 'APHash:', hex(aphash(hwid)))
    print('Legacy key present:', struct.pack('<I', legacy(hwid)) in table)
    print('APHash key present:', struct.pack('<I', aphash(hwid)) in table)
