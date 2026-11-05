#!/usr/bin/env python3
"""Pack rendered PNG sizes into the standard macOS ICNS container."""

from pathlib import Path
from struct import pack
from sys import argv


def main() -> None:
    iconset, output = map(Path, argv[1:])
    sizes = (
        (b"icp4", "icon_16x16.png"),
        (b"icp5", "icon_32x32.png"),
        (b"icp6", "icon_32x32@2x.png"),
        (b"ic07", "icon_128x128.png"),
        (b"ic08", "icon_256x256.png"),
        (b"ic09", "icon_512x512.png"),
        (b"ic10", "icon_512x512@2x.png"),
    )
    sections = []
    for kind, name in sizes:
        image = (iconset / name).read_bytes()
        if not image.startswith(b"\x89PNG\r\n\x1a\n"):
            raise ValueError(f"Invalid PNG: {name}")
        sections.append(kind + pack(">I", len(image) + 8) + image)
    body = b"".join(sections)
    output.write_bytes(b"icns" + pack(">I", len(body) + 8) + body)


if __name__ == "__main__":
    main()
