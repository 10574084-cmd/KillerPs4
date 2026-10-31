#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
VALIDATOR="${GLSLANG_VALIDATOR:-glslangValidator}"

if ! command -v "$VALIDATOR" >/dev/null 2>&1; then
    echo "glslangValidator is required. Install glslang-tools." >&2
    exit 2
fi

count=0
while IFS= read -r file; do
    case "$file" in
        *.vert) stage=vert ;;
        *.frag) stage=frag ;;
        *) continue ;;
    esac

    echo "Checking $file"
    "$VALIDATOR" -S "$stage" "$file"
    count=$((count + 1))
done <<EOF
$(find "$ROOT/shaders" -type f \( -name '*.vert' -o -name '*.frag' \) | sort)
EOF

preset_count=0
while IFS= read -r preset; do
    echo "Checking $preset"
    python3 -m json.tool "$preset" >/dev/null
    preset_count=$((preset_count + 1))
done <<EOF
$(find "$ROOT/shaders/presets" -type f -name '*.json' | sort)
EOF

if [ "$count" -lt 1 ] || [ "$preset_count" -lt 1 ]; then
    echo "No GLSL shader files were found." >&2
    exit 1
fi

echo "Validated $count GLSL shader files and $preset_count preset manifests."
