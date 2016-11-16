#!/bin/sh
# ffmpeg build script for android
# Author:	Max.Chiu

# Config version
VERSION=2.8.7

# Configure enviroment
export ANDROID_NDK_ROOT="/Applications/Android/android-ndk"
export TOOLCHAIN=$(pwd)/toolchain
export SYSROOT=$TOOLCHAIN/sysroot
export ANDROID_API=android-21
export TOOLCHAIN_VERSION=4.9

function configure_toolchain {
	if [ -d $TOOLCHAIN ]; then
		rm -rf $TOOLCHAIN
	fi
	$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh \
	    --toolchain=$ANDROID_EABI \
	    --platform=$ANDROID_API --install-dir=$TOOLCHAIN 
}

function configure_prefix {
	export PREFIX=$(pwd)/out/$ARCH

	export PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
	export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig

	export EXTRA_CFLAGS="-I$PREFIX/include" 
	export EXTRA_LDFLAGS="-L$PREFIX/lib"
}

function configure_arm {
	export ARCH=arm
	export ANDROID_ARCH=arch-arm
	ANDROID_EABI="arm-linux-androideabi-$TOOLCHAIN_VERSION"
	CROSS_COMPILE="arm-linux-androideabi-"
	CROSS_COMPILE_PREFIX=$TOOLCHAIN/bin/$CROSS_COMPILE
	#export CC=$TOOLCHAIN/bin/${CROSS_COMPILE}gcc
	
	configure_prefix
}

function configure_arm64 {
	export ARCH=aarch64
	export ANDROID_ARCH=arch-arm64
	ANDROID_EABI="aarch64-linux-android-$TOOLCHAIN_VERSION"
	CROSS_COMPILE="aarch64-linux-android-"
	CROSS_COMPILE_PREFIX=$TOOLCHAIN/bin/$CROSS_COMPILE
	#export CC=$TOOLCHAIN/bin/${CROSS_COMPILE}gcc
	
	configure_prefix
}

function configure_x86 {
	export ARCH=x86
	export ANDROID_ARCH=arch-x86
	ANDROID_EABI="x86-$TOOLCHAIN_VERSION"
	CROSS_COMPILE="i686-linux-android-"
	CROSS_COMPILE_PREFIX=$TOOLCHAIN/bin/$CROSS_COMPILE
	#export CC=$TOOLCHAIN/bin/${CROSS_COMPILE}gcc
	
	configure_prefix
}

function show_enviroment {
	echo "####################### $ANDROID_ARCH ###############################"
	echo "ANDROID_NDK_ROOT : $ANDROID_NDK_ROOT"
	echo "TOOLCHAIN : $TOOLCHAIN"
	echo "TOOLCHAIN_VERSION : $TOOLCHAIN_VERSION"
	echo "ANDROID_API : $ANDROID_API"
	echo "ANDROID_ARCH : $ANDROID_ARCH"
	echo "ANDROID_EABI : $ANDROID_EABI"
	echo "SYSROOT : $SYSROOT"
	echo "CROSS_COMPILE : $CROSS_COMPILE"
	echo "CROSS_COMPILE_PREFIX : $CROSS_COMPILE_PREFIX"
	echo "PREFIX : $PREFIX"
	echo "PKG_CONFIG_LIBDIR : $PKG_CONFIG_LIBDIR"
	echo "PKG_CONFIG_PATH : $PKG_CONFIG_PATH"
	echo "####################### $ANDROID_ARCH enviroment ok ###############################"
}
  
function build_x264 {
	echo "# Start building x264 for $ARCH"
	
	cd x264
	./configure \
				--prefix=$PREFIX \
				--sysroot=$SYSROOT \
    		--cross-prefix=$CROSS_COMPILE_PREFIX \
				--host=$ARCH-linux \
				--enable-static \
				--enable-pic \
    		--disable-asm \
    		--disable-cli 
    		
	make clean
	make install
	
	cd ..
	echo "# Build x264 finish for $ARCH"
}

function build_ffmpeg {
	echo "# Start building ffmpeg for $ARCH"
	FFMPEG="ffmpeg-$VERSION"
	cd $FFMPEG

	# build
	./configure \
						--prefix=$PREFIX \
						--extra-cflags=$EXTRA_CFLAGS \
						--extra-ldflags=$EXTRA_LDFLAGS \
						--target-os=linux \
						--enable-cross-compile \
						--enable-runtime-cpudetect \
						--sysroot=$SYSROOT \
    				--cross-prefix=$CROSS_COMPILE_PREFIX \
    				--arch=$ARCH \
						--disable-shared \
						--enable-static \
						--enable-gpl \
						--enable-libx264 \
				    --disable-doc \
				    --enable-version3 \
    				--enable-small \
    				--disable-vda \
   					--disable-iconv \
    				--disable-asm \
				    --disable-encoders \
				    --enable-encoder=libx264 \
				    --disable-decoders \
				    --enable-decoder=h264 \
    				--disable-demuxers \
    				--enable-demuxer=h264 \
    				--disable-parsers \
    				--enable-parser=h264 \
    				--disable-outdevs \
    				--disable-ffprobe \
    				--disable-ffplay \
    				--disable-ffserver 
    				#--disable-ffmpeg \
    				#--disable-debug 
		
	make clean
	make install

	cd ..
	echo "# Build ffmpeg finish for $ARCH"
}

function build_ffmpeg_so {
	${CROSS_COMPILE_PREFIX}ld -rpath-link=$SYSROOT/usr/lib -L$SYSROOT/usr/lib -L$PREFIX/lib -soname libffmpeg.so -shared -nostdlib -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so \
    $PREFIX/lib/libx264.a \
    $PREFIX/lib/libavcodec.a \
    $PREFIX/lib/libavfilter.a \
    $PREFIX/lib/libswresample.a \
    $PREFIX/lib/libavformat.a \
    $PREFIX/lib/libavutil.a \
    $PREFIX/lib/libswscale.a \
    $PREFIX/lib/libpostproc.a \
    $PREFIX/lib/libavdevice.a \
    -lc -lm -lz -ldl -llog
}

# Start Build
BUILD_ARCH=(arm x86 arm64)

echo "# Starting building..."

for var in ${BUILD_ARCH[@]};do
	configure_$var
	show_enviroment
	configure_toolchain
	build_x264
	build_ffmpeg
done

echo "# Build finish" 