"""Validate the private build input against this source snapshot. No network or writes."""
import pathlib, re, struct, sys

def validate(index, header):
    data = pathlib.Path(index).read_bytes()
    if len(data) < 24:
        raise ValueError("Truncated index")
    magic, version, count, record_size, identity = struct.unpack_from("<4IQ", data)
    if (magic, version, record_size) != (0x31435053, 1, 304) or not 0 < count <= 4096:
        raise ValueError("Unsupported index layout")
    if len(data) != 24 + count * record_size:
        raise ValueError("Invalid index length")
    digest = 14695981039346656037
    for byte in data[24:]:
        digest = ((digest ^ byte) * 1099511628211) & ((1 << 64) - 1)
    match = re.search(r"#define DIRECT_CACHE_ID UINT64_C\((\d+)\)", pathlib.Path(header).read_text())
    if not match or digest != identity or identity != int(match.group(1)):
        raise ValueError("Index identity mismatch; regenerate index and header together")
    return count

if __name__ == "__main__":
    try:
        if len(sys.argv) != 3:
            raise ValueError("Usage: check_build_index.py INDEX HEADER")
        print("Build index verified:", validate(*sys.argv[1:]), "records")
    except (ValueError, OSError) as error:
        sys.exit(str(error))
