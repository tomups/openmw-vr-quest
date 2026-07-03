#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ./include/version.sh

HOST_TAG="linux-x86_64"

if [[ ! -d toolchain ]]; then
	# Unzip ndk.zip
	mkdir -p toolchain

	echo "(Extracting, this will take a while...)"
	unzip -q downloads/$NDK_FILE -d toolchain/
	mv toolchain/android-ndk-* toolchain/ndk/
fi

pushd toolchain

# Absolute paths into the unified (r23+) LLVM toolchain that ships inside the NDK.
TOOLCHAIN_ROOT="$(pwd)"
UNIFIED_PREBUILT="$TOOLCHAIN_ROOT/ndk/toolchains/llvm/prebuilt/$HOST_TAG"
UNIFIED_BIN="$UNIFIED_PREBUILT/bin"

# The clang driver derives its target triple from the wrapper name. For 64-bit
# arm/x86 the clang wrapper triple matches NDK_TRIPLET, but 32-bit arm is special
# (armv7a-linux-androideabi). binutils still use the NDK_TRIPLET prefix everywhere.
case $ARCH in
	arm) CLANG_TRIPLET="armv7a-linux-androideabi" ;;
	*)   CLANG_TRIPLET="$NDK_TRIPLET" ;;
esac

# NDK r22 shipped triple-prefixed binutils (aarch64-linux-android-ar, ...) in the
# unified bin; r23+ removed them in favour of the llvm-* tools. Some build systems
# (e.g. OpenMW's bundled ICU) still invoke <triple>-ar directly out of the unified
# bin, so recreate those symlinks. Idempotent, runs on every invocation.
for tool in ar ranlib strip nm objcopy objdump readelf addr2line; do
	ln -sf "llvm-$tool" "$UNIFIED_BIN/$NDK_TRIPLET-$tool"
done
ln -sf "ld.lld" "$UNIFIED_BIN/$NDK_TRIPLET-ld"

if [[ ! -d $ARCH ]]; then
	echo "==> Synthesising standalone toolchain for architecture $ARCH"
	echo "    (make_standalone_toolchain.py was removed in NDK r23; recreating its"
	echo "     layout from the unified toolchain so the rest of the build is unchanged)"

	mkdir -p "$ARCH/bin"

	# clang/clang++ wrappers: exec the unified per-API driver so the correct
	# --target and sysroot are selected regardless of how we're invoked.
	make_clang_wrapper() {
		local out="$ARCH/bin/$1"
		local real="$UNIFIED_BIN/$2"
		if [[ $CCACHE = "true" ]]; then
			printf '#!/bin/bash\nexec ccache "%s" "$@"\n' "$real" > "$out"
		else
			printf '#!/bin/bash\nexec "%s" "$@"\n' "$real" > "$out"
		fi
		chmod +x "$out"
	}
	make_clang_wrapper "$NDK_TRIPLET-clang"   "${CLANG_TRIPLET}${ANDROID_API}-clang"
	make_clang_wrapper "$NDK_TRIPLET-clang++" "${CLANG_TRIPLET}${ANDROID_API}-clang++"

	# gcc/g++ are clang under another name (some configure scripts hard-code them).
	ln -sf "$NDK_TRIPLET-clang"   "$ARCH/bin/$NDK_TRIPLET-gcc"
	ln -sf "$NDK_TRIPLET-clang++" "$ARCH/bin/$NDK_TRIPLET-g++"

	# binutils are now the triple-agnostic llvm-* tools.
	ln -sf "$UNIFIED_BIN/llvm-ar"      "$ARCH/bin/$NDK_TRIPLET-ar"
	ln -sf "$UNIFIED_BIN/llvm-ranlib"  "$ARCH/bin/$NDK_TRIPLET-ranlib"
	ln -sf "$UNIFIED_BIN/llvm-strip"   "$ARCH/bin/$NDK_TRIPLET-strip"
	ln -sf "$UNIFIED_BIN/llvm-nm"      "$ARCH/bin/$NDK_TRIPLET-nm"
	ln -sf "$UNIFIED_BIN/llvm-objcopy" "$ARCH/bin/$NDK_TRIPLET-objcopy"
	ln -sf "$UNIFIED_BIN/llvm-objdump" "$ARCH/bin/$NDK_TRIPLET-objdump"
	ln -sf "$UNIFIED_BIN/ld.lld"       "$ARCH/bin/$NDK_TRIPLET-ld"

	# sysroot (headers + libs, incl. libc++_shared.so) is shared in the unified NDK.
	ln -sf "$UNIFIED_PREBUILT/sysroot" "$ARCH/sysroot"

	# Compiler-rt (asan) lives under lib/clang in newer NDKs; expose it as lib64
	# so build.sh's toolchain/$ARCH/lib64/clang/*/lib/linux path keeps working.
	if [[ -d "$UNIFIED_PREBUILT/lib64" ]]; then
		ln -sf "$UNIFIED_PREBUILT/lib64" "$ARCH/lib64"
	else
		ln -sf "$UNIFIED_PREBUILT/lib" "$ARCH/lib64"
	fi

	# build.sh adds toolchain/$ARCH/$NDK_TRIPLET/bin to PATH for gdb-add-index.
	mkdir -p "$ARCH/$NDK_TRIPLET"
	ln -sf "$UNIFIED_BIN" "$ARCH/$NDK_TRIPLET/bin"

	# gas-preprocessor for ffmpeg
	cp ../patches/gas-preprocessor.pl $ARCH/bin/
fi

popd
