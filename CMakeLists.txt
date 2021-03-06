# CMake entry point
cmake_minimum_required (VERSION 2.6)
project (lasercrabs)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/lvl)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/str)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/shader)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/air)

if (APPLE)
	set(ENV{PKG_CONFIG_PATH} ${CMAKE_SOURCE_DIR}/pkgconfig) # so pkg-config can find our custom gl.pc
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wno-deprecated-register -Wno-deprecated-declarations -Wno-invalid-offsetof")
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG=1 -g -O0")
	set(CMAKE_CXX_FLAGS_RELEASE "-DAK_OPTIMIZED=1 -g")
else() # MSVC
	set(CMAKE_C_FLAGS "/Zi /MP /F 8000000")
	set(CMAKE_C_FLAGS_DEBUG "-DDEBUG=1 /MTd /Od")
	set(CMAKE_C_FLAGS_RELEASE "/MT")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/MT")
	set(CMAKE_CXX_FLAGS "-wd4351 -wd4530 -wd4577 -DWIN64=1 /EHsc /Zi /MP /F 8000000")
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG=1 /MTd /Od")
	set(CMAKE_CXX_FLAGS_RELEASE "-DAK_OPTIMIZED=1 /MT")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DAK_OPTIMIZED=1 /MT")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if (PLAYSTATION)
	set(SDL_LIBS "")
elseif (CLIENT)
	find_package(OpenGL REQUIRED)

	## GLEW
	add_subdirectory(external/glew)
	add_definitions(-DGLEW_STATIC)

	## SDL
	if (WIN32)
		set(HAVE_XINPUT_H 1)
		link_directories($ENV{DXSDK_DIR}Lib\\x64)
	endif (WIN32)
	set(DIRECTX ON CACHE BOOL "" FORCE)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(SDL_AUDIO ON CACHE BOOL "" FORCE)
		set(ALSA ON CACHE BOOL "" FORCE)
		set(ALSA_SHARED OFF CACHE BOOL "" FORCE)
		set(PULSEAUDIO ON CACHE BOOL "" FORCE)
		set(PULSEAUDIO_SHARED OFF CACHE BOOL "" FORCE)
	else()
		set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
	endif()
	set(DISKAUDIO OFF CACHE BOOL "" FORCE)
	set(DUMMYAUDIO OFF CACHE BOOL "" FORCE)
	set(SDL_RENDER OFF CACHE BOOL "" FORCE)
	set(SDL_POWER ON CACHE BOOL "" FORCE)
	set(SDL_THREADS ON CACHE BOOL "" FORCE)
	set(SDL_FILE ON CACHE BOOL "" FORCE)
	set(SDL_SHARED OFF CACHE BOOL "" FORCE)
	set(SDL_STATIC ON CACHE BOOL "" FORCE)
	set(LIBC ON CACHE BOOL "" FORCE)
	add_subdirectory(external/sdl)

	set(SDL_LIBS
		SDL2-static
		SDL2main
		GLEW_static
	)
endif()
 
## Wwise
add_subdirectory(external/wwise)
cmake_policy(SET CMP0008 OLD)

## Bullet
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
set(USE_GLUT OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/bullet)
include(${CMAKE_CURRENT_BINARY_DIR}/external/bullet/BulletConfig.cmake)

## lodepng
if (CLIENT)
add_library(lodepng STATIC external/lodepng/lodepng.cpp)
endif()

## Recast
add_library(recast STATIC
	external/recast/Recast/Include/RecastAlloc.h
	external/recast/Recast/Include/Recast.h
	external/recast/Recast/Include/RecastAssert.h
	external/recast/Recast/Source/Recast.cpp
	external/recast/Recast/Source/RecastAlloc.cpp
	external/recast/Recast/Source/RecastArea.cpp
	external/recast/Recast/Source/RecastContour.cpp
	external/recast/Recast/Source/RecastFilter.cpp
	external/recast/Recast/Source/RecastLayers.cpp
	external/recast/Recast/Source/RecastMesh.cpp
	external/recast/Recast/Source/RecastMeshDetail.cpp
	external/recast/Recast/Source/RecastRasterization.cpp
	external/recast/Recast/Source/RecastRegion.cpp
	external/recast/Recast/Source/RecastAssert.cpp
)

target_include_directories(recast PRIVATE external/recast/Recast/Include)

## Detour
add_library(detour STATIC
	external/recast/Detour/Include/DetourAlloc.h
	external/recast/Detour/Include/DetourAssert.h
	external/recast/Detour/Include/DetourCommon.h
	external/recast/Detour/Include/DetourMath.h
	external/recast/Detour/Include/DetourNavMesh.h
	external/recast/Detour/Include/DetourNavMeshBuilder.h
	external/recast/Detour/Include/DetourNavMeshQuery.h
	external/recast/Detour/Include/DetourNode.h
	external/recast/Detour/Include/DetourStatus.h
	external/recast/Detour/Source/DetourAlloc.cpp
	external/recast/Detour/Source/DetourCommon.cpp
	external/recast/Detour/Source/DetourNavMesh.cpp
	external/recast/Detour/Source/DetourNavMeshBuilder.cpp
	external/recast/Detour/Source/DetourNavMeshQuery.cpp
	external/recast/Detour/Source/DetourNode.cpp
	external/recast/Detour/Source/DetourAssert.cpp
	external/recast/DetourTileCache/Include/DetourTileCache.h
	external/recast/DetourTileCache/Include/DetourTileCacheBuilder.h
	external/recast/DetourTileCache/Source/DetourTileCache.cpp
	external/recast/DetourTileCache/Source/DetourTileCacheBuilder.cpp
)

target_include_directories(detour PUBLIC
	external/recast/Detour/Include
	external/recast/DetourTileCache/Include
)

## FastLZ
add_library(fastlz STATIC
	external/fastlz/fastlz.c
	external/fastlz/fastlz.h
)

## Mersenne Twister
add_library(mersenne STATIC
	external/mersenne/mersenne-twister.cpp
	external/mersenne/mersenne-twister.h
)

## sha1
add_library(sha1 STATIC
	external/sha1/sha1.cpp
	external/sha1/sha1.h
)

## mongoose
add_library(mongoose STATIC
	external/mongoose/mongoose.c
	external/mongoose/mongoose.h
)
target_compile_definitions(mongoose PUBLIC -DMG_ENABLE_HTTP_STREAMING_MULTIPART=1 -DMG_ENABLE_IPV6=1)
if (WIN32)
elseif(PLAYSTATION)
else() # linux or mac
	target_compile_definitions(mongoose PUBLIC -DDISABLE_MD5=1)
endif()

## cJSON
add_library(cJSON STATIC
	external/cjson/cJSON.h
	external/cjson/cJSON.c
	external/cjson/cJSON_Utils.c
	external/cjson/cJSON_Utils.h
)

## cURL
if(APPLE)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif(APPLE)
if (WIN32)
	set(CURL_STATIC_CRT ON CACHE BOOL "" FORCE)
	set(CMAKE_USE_WINSSL ON CACHE BOOL "" FORCE)
	set(OPENSSL_MSVC_STATIC_RT ON CACHE BOOL "" FORCE)
endif (WIN32)
set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
if(WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
else(WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif(WIN32)
if(APPLE)
find_package(OpenSSL REQUIRED)
endif(APPLE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(CURL_STATICLIB ON CACHE BOOL "" FORCE)
set(HTTP_ONLY OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_FTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_DICT ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_FILE ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_TFTP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_SMTP OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_GOPHER ON CACHE BOOL "" FORCE)
set(ENABLE_MANUAL OFF CACHE BOOL "" FORCE)
set(CMAKE_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(external/curl)
set_target_properties(html pdf opts-html opts-pdf PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
add_definitions(-DCURL_STATICLIB)

## next

add_library(sodium STATIC
	external/sodium/src/libsodium/crypto_aead/chacha20poly1305/sodium/aead_chacha20poly1305.c
	external/sodium/src/libsodium/crypto_aead/xchacha20poly1305/sodium/aead_xchacha20poly1305.c
	external/sodium/src/libsodium/crypto_auth/crypto_auth.c
	external/sodium/src/libsodium/crypto_auth/hmacsha256/auth_hmacsha256.c
	external/sodium/src/libsodium/crypto_auth/hmacsha512/auth_hmacsha512.c
	external/sodium/src/libsodium/crypto_auth/hmacsha512256/auth_hmacsha512256.c
	external/sodium/src/libsodium/crypto_box/crypto_box.c
	external/sodium/src/libsodium/crypto_box/crypto_box_easy.c
	external/sodium/src/libsodium/crypto_box/crypto_box_seal.c
	external/sodium/src/libsodium/crypto_box/curve25519xsalsa20poly1305/box_curve25519xsalsa20poly1305.c
	external/sodium/src/libsodium/crypto_core/ed25519/ref10/ed25519_ref10.c
	external/sodium/src/libsodium/crypto_core/hchacha20/core_hchacha20.c
	external/sodium/src/libsodium/crypto_core/hsalsa20/ref2/core_hsalsa20_ref2.c
	external/sodium/src/libsodium/crypto_core/hsalsa20/core_hsalsa20.c
	external/sodium/src/libsodium/crypto_core/salsa/ref/core_salsa_ref.c
	external/sodium/src/libsodium/crypto_generichash/crypto_generichash.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/generichash_blake2.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ref.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-sse2.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-sse41.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-load-avx2.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/generichash_blake2b.c
	external/sodium/src/libsodium/crypto_hash/crypto_hash.c
	external/sodium/src/libsodium/crypto_hash/sha256/hash_sha256.c
	external/sodium/src/libsodium/crypto_hash/sha256/cp/hash_sha256_cp.c
	external/sodium/src/libsodium/crypto_hash/sha512/hash_sha512.c
	external/sodium/src/libsodium/crypto_hash/sha512/cp/hash_sha512_cp.c
	external/sodium/src/libsodium/crypto_kdf/blake2b/kdf_blake2b.c
	external/sodium/src/libsodium/crypto_kdf/crypto_kdf.c
	external/sodium/src/libsodium/crypto_kx/crypto_kx.c
	external/sodium/src/libsodium/crypto_onetimeauth/crypto_onetimeauth.c
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/onetimeauth_poly1305.c
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/onetimeauth_poly1305.h
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna.h
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna32.h
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna64.h
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/donna/poly1305_donna.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-core.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-core.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-encoding.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-encoding.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-ref.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/blake2b-long.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/blake2b-long.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/blamka-round-ref.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/pwhash_argon2i.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/pwhash_argon2id.c
	external/sodium/src/libsodium/crypto_pwhash/crypto_pwhash.c
	external/sodium/src/libsodium/crypto_scalarmult/crypto_scalarmult.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/ref10/x25519_ref10.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/scalarmult_curve25519.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/scalarmult_curve25519.h
	external/sodium/src/libsodium/crypto_secretbox/crypto_secretbox.c
	external/sodium/src/libsodium/crypto_secretbox/crypto_secretbox_easy.c
	external/sodium/src/libsodium/crypto_secretbox/xsalsa20poly1305/secretbox_xsalsa20poly1305.c
	external/sodium/src/libsodium/crypto_secretstream/xchacha20poly1305/secretstream_xchacha20poly1305.c
	external/sodium/src/libsodium/crypto_shorthash/crypto_shorthash.c
	external/sodium/src/libsodium/crypto_shorthash/siphash24/shorthash_siphash24.c
	external/sodium/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphash24_ref.c
	external/sodium/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphash_ref.h
	external/sodium/src/libsodium/crypto_sign/crypto_sign.c
	external/sodium/src/libsodium/crypto_sign/ed25519/sign_ed25519.c
	external/sodium/src/libsodium/crypto_sign/ed25519/ref10/keypair.c
	external/sodium/src/libsodium/crypto_sign/ed25519/ref10/open.c
	external/sodium/src/libsodium/crypto_sign/ed25519/ref10/sign.c
	external/sodium/src/libsodium/crypto_sign/ed25519/ref10/sign_ed25519_ref10.h
	external/sodium/src/libsodium/crypto_stream/chacha20/stream_chacha20.c
	external/sodium/src/libsodium/crypto_stream/chacha20/stream_chacha20.h
	external/sodium/src/libsodium/crypto_stream/chacha20/ref/chacha20_ref.h
	external/sodium/src/libsodium/crypto_stream/chacha20/ref/chacha20_ref.c
	external/sodium/src/libsodium/crypto_stream/crypto_stream.c
	external/sodium/src/libsodium/crypto_stream/salsa20/stream_salsa20.c
	external/sodium/src/libsodium/crypto_stream/salsa20/stream_salsa20.h
	external/sodium/src/libsodium/crypto_stream/salsa20/ref/salsa20_ref.c
	external/sodium/src/libsodium/crypto_stream/salsa20/ref/salsa20_ref.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.c
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.c
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.c
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-sse2.h
	external/sodium/src/libsodium/crypto_stream/xsalsa20/stream_xsalsa20.c
	external/sodium/src/libsodium/crypto_verify/sodium/verify.c
	external/sodium/src/libsodium/include/sodium/private/common.h
	external/sodium/src/libsodium/include/sodium/private/ed25519_ref10.h
	external/sodium/src/libsodium/include/sodium/private/implementations.h
	external/sodium/src/libsodium/include/sodium/private/mutex.h
	external/sodium/src/libsodium/include/sodium/private/sse2_64_32.h
	external/sodium/src/libsodium/randombytes/randombytes.c
	external/sodium/src/libsodium/randombytes/sysrandom/randombytes_sysrandom.c
	external/sodium/src/libsodium/sodium/codecs.c
	external/sodium/src/libsodium/sodium/core.c
	external/sodium/src/libsodium/sodium/runtime.c
	external/sodium/src/libsodium/sodium/utils.c
	external/sodium/src/libsodium/sodium/version.c
	external/sodium/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/base.h
	external/sodium/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/base2.h
	external/sodium/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/constants.h
	external/sodium/src/libsodium/crypto_core/ed25519/ref10/fe_25_5/fe.h
	external/sodium/src/libsodium/include/sodium/private/ed25519_ref10_fe_25_5.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6-asm.S
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.c
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6/salsa20_xmm6.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/consts.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_mul.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_nsquare.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_pack.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_base.S
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/consts_namespace.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/curve25519_sandy2x.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_invert.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe51_namespace.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/fe_frombytes_sandy2x.c
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_base.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_base_namespace.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/ladder_namespace.h
	external/sodium/src/libsodium/crypto_scalarmult/curve25519/sandy2x/sandy2x.S
	external/sodium/src/libsodium/crypto_box/curve25519xchacha20poly1305/box_curve25519xchacha20poly1305.c
	external/sodium/src/libsodium/crypto_box/curve25519xchacha20poly1305/box_seal_curve25519xchacha20poly1305.c
	external/sodium/src/libsodium/crypto_core/ed25519/core_ed25519.c
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/crypto_scrypt-common.c
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/crypto_scrypt.h
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/scrypt_platform.c
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pbkdf2-sha256.c
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pbkdf2-sha256.h
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/pwhash_scryptsalsa208sha256.c
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/nosse/pwhash_scryptsalsa208sha256_nosse.c
	external/sodium/src/libsodium/crypto_scalarmult/ed25519/ref10/scalarmult_ed25519_ref10.c
	external/sodium/src/libsodium/crypto_secretbox/xchacha20poly1305/secretbox_xchacha20poly1305.c
	external/sodium/src/libsodium/crypto_shorthash/siphash24/shorthash_siphashx24.c
	external/sodium/src/libsodium/crypto_shorthash/siphash24/ref/shorthash_siphashx24_ref.c
	external/sodium/src/libsodium/crypto_sign/ed25519/ref10/obsolete.c
	external/sodium/src/libsodium/crypto_stream/salsa2012/ref/stream_salsa2012_ref.c
	external/sodium/src/libsodium/crypto_stream/salsa2012/stream_salsa2012.c
	external/sodium/src/libsodium/crypto_stream/salsa208/ref/stream_salsa208_ref.c
	external/sodium/src/libsodium/crypto_stream/salsa208/stream_salsa208.c
	external/sodium/src/libsodium/crypto_stream/xchacha20/stream_xchacha20.c
	external/sodium/src/libsodium/randombytes/salsa20/randombytes_salsa20_random.c
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.c
	external/sodium/src/libsodium/crypto_onetimeauth/poly1305/sse2/poly1305_sse2.h
	external/sodium/src/libsodium/crypto_pwhash/scryptsalsa208sha256/sse/pwhash_scryptsalsa208sha256_sse.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-ssse3.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-ssse3.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/blamka-round-ssse3.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.c
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-ssse3.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/u0.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/u1.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/u4.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-sse41.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-sse41.h
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-avx2.c
	external/sodium/src/libsodium/crypto_generichash/blake2b/ref/blake2b-compress-avx2.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx2.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/blamka-round-avx2.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.c
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/chacha20_dolbeau-avx2.h
	external/sodium/src/libsodium/crypto_stream/chacha20/dolbeau/u8.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.c
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/salsa20_xmm6int-avx2.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/u0.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/u1.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/u4.h
	external/sodium/src/libsodium/crypto_stream/salsa20/xmm6int/u8.h
	external/sodium/src/libsodium/crypto_pwhash/argon2/argon2-fill-block-avx512f.c
	external/sodium/src/libsodium/crypto_pwhash/argon2/blamka-round-avx512f.h
)
target_include_directories(sodium PRIVATE
	external/sodium/src/libsodium/include/sodium/
)
target_compile_definitions(sodium PUBLIC -DSODIUM_STATIC -DSODIUM_EXPORT=)
if (WIN32)
	target_compile_definitions(sodium PRIVATE -Dinline=__inline -DNATIVE_LITTLE_ENDIAN -D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

if (WIN32)
	set(NEXT_PLATFORM_FILES
		external/next/next_windows.h
		external/next/next_windows.cpp
	)
else()
	set(NEXT_PLATFORM_FILES
		external/next/next_unix.h
		external/next/next_unix.cpp
	)
endif()

add_library(next STATIC
	${NEXT_PLATFORM_FILES}
	external/next/next.h
	external/next/next.cpp
	external/next/next_internal.h
	external/next/next_internal.cpp
	external/next/next_curl.h
	external/next/next_curl.cpp 
)
target_compile_definitions(next PRIVATE -DNEXT_EXPORT)
target_include_directories(next PRIVATE
	external/curl/include
	external
	external/sodium/src/libsodium/include/
)
target_link_libraries(next libcurl sodium)
if(WIN32)
else()
	target_link_libraries(next "-lpthread")
endif()

## includes

set(ZLIB_INCLUDE_DIR
	${CMAKE_CURRENT_SOURCE_DIR}/external/assimp/contrib/zlib
	${CMAKE_CURRENT_BINARY_DIR}/external/assimp/contrib/zlib
)

set(IMPORT_INCLUDES
	external
	${SDL2_BINARY_DIR}/include
	external/bullet/src
	src
	${CMAKE_CURRENT_BINARY_DIR}/external/assimp/include
	external/assimp/include
)

if (WIN32)
	set(IMPORT_INCLUDES ${IMPORT_INCLUDES} external/dirent)
endif(WIN32)

set(ALL_INCLUDES
	external
	src
	${ZLIB_INCLUDE_DIR}
)

if (WIN32)
	set(ALL_INCLUDES ${ALL_INCLUDES} external/dirent)
endif(WIN32)

set(SERVER_CLIENT_INCLUDES
	${ALL_INCLUDES}
	external/bullet/src
)

string(REPLACE ";" "," ALL_INCLUDES_COMMA "${ALL_INCLUDES}")
configure_file(.includes.in .includes)
configure_file(.exrc.in ${CMAKE_CURRENT_SOURCE_DIR}/.exrc)

## source

set(SRC 
	CMakeLists.txt
	src/data/array.h
	src/data/pin_array.h
	src/data/import_common.h
	src/data/import_common.cpp
	src/data/entity.h
	src/data/entity.cpp
	src/data/components.h
	src/data/components.cpp
	src/data/animator.h
	src/data/animator.cpp
	src/data/ragdoll.h
	src/data/ragdoll.cpp
	src/data/priority_queue.h
	src/data/json.h
	src/data/json.cpp
	src/data/unicode.h
	src/data/unicode.cpp
	src/render/render.h
	src/render/render.cpp
	src/render/views.h
	src/render/views.cpp
	src/render/particles.h
	src/render/particles.cpp
	src/render/skinned_model.h
	src/render/skinned_model.cpp
	src/render/glvm.h
	src/render/ui.h
	src/render/ui.cpp
	src/asset/lookup.h
	src/asset/animation.h
	src/asset/armature.h
	src/asset/font.h
	src/asset/mesh.h
	src/asset/shader.h
	src/asset/level.h
	src/asset/texture.h
	src/asset/values.cpp
	src/asset/soundbank.h
	src/asset/Wwise_IDs.h
	src/asset/string.h
	src/platform/util.h
	src/platform/sock.h
	src/platform/sock.cpp
	src/game/master.h
	src/game/master.cpp
	src/sync.h
	src/types.h
	src/vi_assert.h
	src/noise.h
	src/noise.cpp
	src/lmath.h
	src/lmath.cpp
	src/input.h
	src/input.cpp
	src/localization.h
	src/localization.cpp
	src/common.h
	src/common.cpp
	src/console.h
	src/console.cpp
	src/load.h
	src/load.cpp
	src/settings.h
	src/net.h
	src/net.cpp
	src/net_serialize.h
	src/net_serialize.cpp
	src/physics.h
	src/physics.cpp
	src/ai.h
	src/ai.cpp
	src/ai_worker.cpp
	src/loop.h
	src/ease.h
	src/game/component_ids.cpp
	src/game/entities.h
	src/game/entities.cpp
	src/game/minion.h
	src/game/minion.cpp
	src/game/player.h
	src/game/player.cpp
	src/game/team.h
	src/game/team.cpp
	src/game/overworld.h
	src/game/overworld.cpp
	src/game/ai_player.h
	src/game/ai_player.cpp
	src/game/drone.h
	src/game/drone.cpp
	src/game/walker.h
	src/game/walker.cpp
	src/game/constants.h
	src/game/game.h
	src/game/game.cpp
	src/game/audio.h
	src/game/audio.cpp
	src/game/menu.h
	src/game/menu.cpp
	src/game/scripts.h
	src/game/scripts.cpp
	src/game/parkour.h
	src/game/parkour.cpp
	external/sdl_controllers/gamecontrollerdb.txt
	assets/shader/particle_standard.glsl
	assets/shader/particle_alpha.glsl
	assets/shader/particle_limited_size.glsl
	assets/shader/particle_eased.glsl
	assets/shader/particle_textured.glsl
	assets/shader/particle_spark.glsl
	assets/shader/particle_rain.glsl
	assets/shader/armature.glsl
	assets/shader/composite.glsl
	assets/shader/flat.glsl
	assets/shader/nav_dots.glsl
	assets/shader/flat_texture.glsl
	assets/shader/flat_texture_offset.glsl
	assets/shader/fresnel.glsl
	assets/shader/water.glsl
	assets/shader/point_light.glsl
	assets/shader/global_light.glsl
	assets/shader/spot_light.glsl
	assets/shader/standard.glsl
	assets/shader/standard_flat.glsl
	assets/shader/standard_instanced.glsl
	assets/shader/flat_instanced.glsl
	assets/shader/culled.glsl
	assets/shader/ui.glsl
	assets/shader/ui_texture.glsl
	assets/shader/debug_depth.glsl
	assets/shader/ssao.glsl
	assets/shader/ssao_blur.glsl
	assets/shader/ssao_downsample.glsl
	assets/shader/downsample.glsl
	assets/shader/blit.glsl
	assets/shader/stencil_back_faces.glsl
	assets/shader/scan_lines.glsl
	assets/shader/bloom_downsample.glsl
	assets/shader/blur.glsl
	assets/shader/skybox.glsl
	assets/shader/underwater.glsl
	assets/shader/sky_decal.glsl
	assets/shader/clouds.glsl
	assets/script/blend_to_fbx.py
	assets/script/blend_to_lvl.py
	assets/script/ttf_to_fbx.py
	assets/str/en.json
)

# stolen from https://cheind.wordpress.com/2009/12/10/reducing-compilation-time-unity-builds/
function(enable_unity_build UB_SUFFIX SOURCE_VARIABLE_NAME)
	set(files ${${SOURCE_VARIABLE_NAME}})
	# generate a unique filename for the unity build translation unit
	set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/ub_${UB_SUFFIX}.cpp)
	# exclude all translation units from compilation
	set_source_files_properties(${files} PROPERTIES HEADER_FILE_ONLY true)
	# open the ub file
	FILE(WRITE ${unit_build_file} "// Unity Build generated by CMake\n")
	# add include statement for each translation unit
	foreach(source_file ${files} )
	  FILE( APPEND ${unit_build_file} "#include <${CMAKE_CURRENT_SOURCE_DIR}/${source_file}>\n")
	endforeach(source_file)
	# complement list of translation units with the name of ub
	set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${unit_build_file} PARENT_SCOPE)  
endfunction(enable_unity_build)

if (SERVER)

	## sqlite
	add_library(sqlite STATIC
		external/sqlite/sqlite3.h
		external/sqlite/sqlite3.c
	)

	## server
	set(SRC_SERVER
		${SRC}
		src/platform/server.cpp
	)
	#enable_unity_build(lasercrabsrv SRC_SERVER)
	add_executable(lasercrabsrv ${SRC_SERVER})

	## master
	set(SRC_MASTER
		src/data/array.h
		src/data/pin_array.h
		src/data/priority_queue.h
		src/data/json.h
		src/data/json.cpp
		src/data/unicode.h
		src/data/unicode.cpp
		src/net_serialize.h
		src/net_serialize.cpp
		src/vi_assert.h
		src/types.h
		src/platform/sock.h
		src/platform/sock.cpp
		src/platform/util.h
		src/game/master.h
		src/game/master.cpp
		src/game/constants.h
		src/asset/level.h
		src/platform/master_server.cpp
		src/http.h
		src/http.cpp
	)
	#enable_unity_build(lasercrabmaster SRC_MASTER)
	add_executable(lasercrabmaster
		${SRC_MASTER}
	)

	target_include_directories(lasercrabsrv PRIVATE
		${SERVER_CLIENT_INCLUDES}
		external/wwise
	)

	target_include_directories(lasercrabmaster PRIVATE
		${ALL_INCLUDES}
		external/curl/include
		${CMAKE_CURRENT_BINARY_DIR}/external/curl/include/curl
		external/sodium/src/libsodium/include/
	)

	target_link_libraries(lasercrabsrv
		BulletDynamics
		BulletCollision
		BulletSoftBody
		LinearMath
		recast
		detour
		fastlz
		cJSON
		mersenne
		zlibstatic
		next
	)

	if (APPLE)
		target_link_libraries(lasercrabmaster ${OPENSSL_LIBRARIES})
	elseif (WIN32)
	else()
		target_link_libraries(lasercrabsrv "-lpthread")
		target_link_libraries(lasercrabmaster "-lpthread")
	endif()

	target_compile_definitions(lasercrabsrv PRIVATE -DSERVER=1)
	target_compile_definitions(lasercrabmaster PRIVATE -DMASTER_SERVER=1)

	target_link_libraries(lasercrabmaster
		zlibstatic
		cJSON
		sqlite
		libcurl
		sodium
		mersenne
		mongoose
		sha1
	)

endif()

if (CLIENT)
	## client
	if (PLAYSTATION)
		set(SRC_PLAYSTATION
			${SRC}
			src/platform/ps/glvm.cpp
			src/platform/ps/main.cpp
		)
		#enable_unity_build(lasercrabs SRC_PLAYSTATION)
		add_executable(lasercrabs ${SRC_PLAYSTATION})
	else()
		set(SRC_CLIENT
			${SRC}
			src/platform/glvm.cpp
			src/platform/sdl.cpp
		)
		#enable_unity_build(lasercrabs SRC_CLIENT)
		add_executable(lasercrabs ${SRC_CLIENT})

		## discord
		set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(ENABLE_IO_THREAD OFF CACHE BOOL "" FORCE)
		set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)
		add_subdirectory(external/discord)

		target_include_directories(lasercrabs PRIVATE
			${SDL2_BINARY_DIR}/include
			external/curl/include
			${CMAKE_CURRENT_BINARY_DIR}/external/curl/include/curl
		)

		target_link_libraries(lasercrabs discord-rpc)
	endif()

	add_custom_command(
		TARGET lasercrabs POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/external/sdl_controllers/gamecontrollerdb.txt ${CMAKE_CURRENT_BINARY_DIR}/gamecontrollerdb.txt
	)

	target_include_directories(lasercrabs PRIVATE ${SERVER_CLIENT_INCLUDES})

	if(WIN32)
		set_property(TARGET lasercrabs APPEND PROPERTY LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_property(TARGET lasercrabs APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_CONSOLE>)
		set_property(TARGET lasercrabs APPEND PROPERTY LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
		set_property(TARGET lasercrabs APPEND PROPERTY LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
		set_property(TARGET lasercrabs APPEND PROPERTY LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
	endif(WIN32)

	## libraries

	target_link_libraries(lasercrabs
		${OPENGL_LIBRARIES}
		${SDL_LIBS}
		BulletDynamics
		BulletCollision
		BulletSoftBody
		LinearMath
		recast
		detour
		fastlz
		cJSON
		mersenne
		lodepng
		wwise_integration
		zlibstatic
		libcurl
		mongoose
		next
	)
	
	## Steam
	if (WIN32)
		target_link_libraries(lasercrabs ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/win64/steam_api64.lib)
		add_custom_command(
			TARGET lasercrabs POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/win64/steam_api64.dll ${CMAKE_CURRENT_BINARY_DIR}/steam_api64.dll
		)
	elseif (APPLE)
		target_link_libraries(lasercrabs ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/osx32/libsteam_api.dylib)
		add_custom_command(
			TARGET lasercrabs POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/osx32/libsteam_api.dylib ${CMAKE_CURRENT_BINARY_DIR}/libsteam_api.dylib
		)
	elseif (PLAYSTATION)
	else() # Linux
		target_link_libraries(lasercrabs ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/linux64/libsteam_api.so)
		add_custom_command(
			TARGET lasercrabs POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/external/steam/linux64/libsteam_api.so ${CMAKE_CURRENT_BINARY_DIR}/libsteam_api.so
		)
	endif()

	if (APPLE)
		target_link_libraries(lasercrabs "-framework AudioUnit -framework AudioToolbox" "-framework CoreAudio" ${OPENSSL_LIBRARIES})
	elseif (WIN32)
	else()
		target_link_libraries(lasercrabs "-lpthread")
	endif()
endif()

if (NOT PLAYSTATION)
	## AssImp

	set(ASSIMP_LIB_INSTALL_DIR "lib")
	set(LIBASSIMP_COMPONENT libassimp)
	set(BUILD_STATIC_LIB ON)
	set(ASSIMP_BUILD_3DS_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_3D_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_AC_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ASE_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ASSBIN_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ASSXML_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_B3D_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_BLEND_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_BVH_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_COB_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_COLLADA_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_CSM_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_DXF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_HMP_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_IFC_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_IRR_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_LWO_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_LWS_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MD2_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MD3_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MD5_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MDC_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MDL_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MS3D_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_NDO_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_NFF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OBJ_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OFF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OGRE_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OPENGEX_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_PLY_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_Q3BSP_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_Q3D_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_RAW_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_SMD_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_STL_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_TERRAGEN_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_XGL_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_X_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_AMF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_IRRMESH_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_SIB_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_X3D_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_GLTF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_3MF_IMPORTER OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_MMD_IMPORTER OFF CACHE BOOL "" FORCE)

	# search for external dependencies, and build them from source if not found
	# search for zlib
	include(CheckIncludeFile)
	include(CheckTypeSize)
	include(CheckFunctionExists)
	set(AMD64 OFF)
	# compile from sources
	add_subdirectory(external/assimp/contrib/zlib)
	set(ZLIB_FOUND 1)
	set(ZLIB_LIBRARIES zlibstatic)
	set_target_properties(zlibstatic PROPERTIES LINKER_LANGUAGE CXX)

	set(GIT_COMMIT_HASH 0)
	configure_file(
		external/assimp/revision.h.in
		${CMAKE_SOURCE_DIR}/external/assimp/code/revision.h
	)
	configure_file(
		external/assimp/include/assimp/config.h.in
		${CMAKE_CURRENT_BINARY_DIR}/external/assimp/include/assimp/config.h
	)
	add_subdirectory(external/assimp/code)

	# compile without boost
	target_compile_definitions(assimp PRIVATE
		-DASSIMP_BUILD_BOOST_WORKAROUND
		-DASSIMP_BUILD_NO_EXPORT
		-DASSIMP_BUILD_NO_3DS_IMPORTER
		-DASSIMP_BUILD_NO_AC_IMPORTER
		-DASSIMP_BUILD_NO_ASE_IMPORTER
		-DASSIMP_BUILD_NO_B3D_IMPORTER
		-DASSIMP_BUILD_NO_BLEND_IMPORTER
		-DASSIMP_BUILD_NO_BVH_IMPORTER
		-DASSIMP_BUILD_NO_COB_IMPORTER
		-DASSIMP_BUILD_NO_COLLADA_IMPORTER
		-DASSIMP_BUILD_NO_CSM_IMPORTER
		-DASSIMP_BUILD_NO_DXF_IMPORTER
		-DASSIMP_BUILD_NO_OBJ_IMPORTER
		-DASSIMP_BUILD_NO_STL_IMPORTER
		-DASSIMP_BUILD_NO_PLY_IMPORTER
		-DASSIMP_BUILD_NO_HMP_IMPORTER
		-DASSIMP_BUILD_NO_IFC_IMPORTER
		-DASSIMP_BUILD_NO_X_IMPORTER
		-DASSIMP_BUILD_NO_MDL_IMPORTER
		-DASSIMP_BUILD_NO_MD3_IMPORTER
		-DASSIMP_BUILD_NO_MD2_IMPORTER
		-DASSIMP_BUILD_NO_SMD_IMPORTER
		-DASSIMP_BUILD_NO_MDC_IMPORTER
		-DASSIMP_BUILD_NO_LWO_IMPORTER
		-DASSIMP_BUILD_NO_LWS_IMPORTER
		-DASSIMP_BUILD_NO_NFF_IMPORTER
		-DASSIMP_BUILD_NO_RAW_IMPORTER
		-DASSIMP_BUILD_NO_OFF_IMPORTER
		-DASSIMP_BUILD_NO_IRRMESH_IMPORTER
		-DASSIMP_BUILD_NO_IRR_IMPORTER
		-DASSIMP_BUILD_NO_Q3D_IMPORTER
		-DASSIMP_BUILD_NO_TERRAGEN_IMPORTER
		-DASSIMP_BUILD_NO_3D_IMPORTER
		-DASSIMP_BUILD_NO_OGRE_IMPORTER
		-DASSIMP_BUILD_NO_MS3D_IMPORTER
		-DASSIMP_BUILD_NO_Q3BSP_IMPORTER
		-DASSIMP_BUILD_NO_NDO_IMPORTER
		-DASSIMP_BUILD_NO_XGL_IMPORTER
		-DASSIMP_BUILD_NO_C4D_IMPORTER
		-DASSIMP_BUILD_NO_AMF_IMPORTER
		-DASSIMP_BUILD_NO_SIB_IMPORTER
		-DASSIMP_BUILD_NO_X3D_IMPORTER
		-DASSIMP_BUILD_NO_GLTF_IMPORTER
		-DASSIMP_BUILD_NO_3MF_IMPORTER
		-DASSIMP_BUILD_NO_MMD_IMPORTER
	)

	target_include_directories(assimp PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/external/assimp/include
		external/assimp/include
		external/assimp/code/BoostWorkaround
		${ZLIB_INCLUDE_DIR}
	)

	set(SRC_IMPORT
		src/game/constants.h
		src/data/import_common.h
		src/data/import_common.cpp
		src/render/glvm.h
		src/data/array.h
		src/types.h
		src/lmath.h
		src/lmath.cpp
		src/import.cpp
		src/data/json.h
		src/data/json.cpp
		src/data/unicode.h
		src/data/unicode.cpp
		src/platform/glvm.cpp
	)
	#enable_unity_build(import SRC_IMPORT)
	add_executable(import ${SRC_IMPORT})

	target_include_directories(import PRIVATE ${IMPORT_INCLUDES})

	target_link_libraries(import
		${OPENGL_LIBRARIES}
		${SDL_LIBS}
		assimp
		recast
		detour
		fastlz
		cJSON
		mersenne
	)

	add_custom_target(
		assets ALL
		COMMAND $<TARGET_FILE:import>
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
	add_dependencies(assets import)
	if (CLIENT)
		add_dependencies(lasercrabs assets)
	endif()
	if (SERVER)
		add_dependencies(lasercrabsrv assets)
		add_dependencies(lasercrabmaster assets)
	endif()
endif()
