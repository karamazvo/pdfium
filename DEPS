use_relative_paths = True

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'checkout_android',
  'checkout_skia',
]

vars = {
  # By default, we should check out everything needed to run on the main
  # pdfium waterfalls. This var can be also be set to 'small', in order to skip
  # things are not strictly needed to build pdfium for development purposes,
  # by adding the following line to the .gclient file inside a solutions entry:
  #      "custom_vars": { "checkout_configuration": "small" },
  # Similarly, this var can be set to 'minimal' to also skip the Skia and V8
  # checkouts for the smallest possible checkout, where some features will not
  # work.
  'checkout_configuration': 'default',

  # By default, don't check out android. Will be overridden by gclient
  # variables.
  # TODO(crbug.com/875037): Remove this once the bug in gclient is fixed.
  'checkout_android': False,

  # Pull in Android native toolchain dependencies, so we can build ARC++
  # support libraries.
  'checkout_android_native_support': 'checkout_android',

  'checkout_clang_coverage_tools': 'False',

  'checkout_clang_tidy': 'False',

  'checkout_clangd': 'False',

  'checkout_instrumented_libraries': 'checkout_linux and checkout_configuration != "small" and checkout_configuration != "minimal"',

  # Fetch the rust toolchain.
  #
  # Use a custom_vars section to enable it:
  # "custom_vars": {
  #   "checkout_rust": True,
  # }
  'checkout_rust': False,

  'checkout_skia': 'checkout_configuration != "minimal"',

  'checkout_testing_corpus': 'checkout_configuration != "small" and checkout_configuration != "minimal"',

  'checkout_v8': 'checkout_configuration != "minimal"',

  # condition to allowlist deps for non-git-source processing.
  'non_git_source': 'True',

  # Fetch configuration files required for the 'use_remoteexec' gn arg
  'download_remoteexec_cfg': False,
  # RBE instance to use for running remote builds
  'rbe_instance': Str('projects/rbe-chrome-untrusted/instances/default_instance'),
  # RBE project to download rewrapper config files for. Only needed if
  # different from the project used in 'rbe_instance'
  'rewrapper_cfg_project': Str(''),
  # reclient CIPD package
  'reclient_package': 'infra/rbe/client/',
  # reclient CIPD package version
  'reclient_version': 're_client_version:0.172.0.3cf60ba5-gomaip',

  'chromium_git': 'https://chromium.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling abseil
  # and whatever else without interference from each other.
  'abseil_revision': '72093794ac42be8105817ae0b0569fb411a6ca9b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_toolchain
  # and whatever else without interference from each other.
  'android_toolchain_version': 'Idl-vYnWGnM8K3XJhM3h6zjYVDXlnljVz3FE00V9IM8C',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling build
  # and whatever else without interference from each other.
  'build_revision': '4aba7b76ec765b0fe78cf6341887aaa80832f8c6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling buildtools
  # and whatever else without interference from each other.
  'buildtools_revision': '469b4146c5017c7a863dc795dd71d91b92326a57',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': '86d6f8ee6130eb3d6a43db7ac883c7d39df1c158',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang format
  # and whatever else without interference from each other.
  'clang_format_revision': '1549a8dba21b6c022c6f5ccee4edf18e5ceb2109',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang
  # and whatever else without interference from each other.
  'clang_revision': 'bf9a3411372f2d5eed8b3d27ee8bd8cf6c17135f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'code_coverage_revision': '5e7c277c0d8c82878208f9eded7b3133183ac482',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'cpu_features_revision': '936b9ab5515dead115606559502e3864958f7f6e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling depot_tools
  # and whatever else without interference from each other.
  'depot_tools_revision': '58625e82c685426d441be5b422c9ad88e4867d20',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fast_float
  # and whatever else without interference from each other.
  'fast_float_revision': '3e57d8dcfb0a04b5a8a26b486b54490a2e9b310f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling fp16
  # and whatever else without interference from each other.
  'fp16_revision': '581ac1c79dd9d9f6f4e8b2934e7a55c7becf0799',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling freetype
  # and whatever else without interference from each other.
  'freetype_revision': 'afc7000cacb8cc90ae61036858c5306defa1236a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling GN CIPD package version
  # and whatever else without interference from each other.
  'gn_version': 'git_revision:c97a86a72105f3328a540f5a5ab17d11989ab7dd',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling goldctl CIPD package version
  # and whatever else without interference from each other.
  'goldctl_version': 'git_revision:db814b551104336b5e0dc7053ae2e4f5aa4bed65',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling gtest
  # and whatever else without interference from each other.
  'gtest_revision': '7d76a231b0e29caf86e68d1df858308cd53b2a66',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling highway
  # and whatever else without interference from each other.
  'highway_revision': '00fe003dac355b979f36157f9407c7c46448958e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling icu
  # and whatever else without interference from each other.
  'icu_revision': 'bbccc2f6efc1b825de5f2c903c48be685cd0cf22',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling instrumented_lib
  # and whatever else without interference from each other.
  'instrumented_lib_revision': '3cc43119a29158bcde39d288a8def4b8ec49baf8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jinja2
  # and whatever else without interference from each other.
  'jinja2_revision': '5e1ee241ab04b38889f8d517f2da8b3df7cfbd9a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling jpeg_turbo
  # and whatever else without interference from each other.
  'jpeg_turbo_revision': '927aabfcd26897abb9776ecf2a6c38ea5bb52ab6',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++
  # and whatever else without interference from each other.
  # If you change this, also update the libc++ revision in
  # //buildtools/deps_revisions.gni.
  'libcxx_revision': '74dd760826063f15ce4d373b31bc34d1c65e41c2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++abi
  # and whatever else without interference from each other.
  'libcxxabi_revision': '83dfa1f5bfce32d5f75695542468e37ead8163b8',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libpng
  # and whatever else without interference from each other.
  'libpng_revision': 'cf7c36ed084cd051100c4a1597a901c08377b835',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libunwind
  # and whatever else without interference from each other.
  'libunwind_revision': 'd1e95b102f113ded38974cf06a65fe0457b6004b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision': '60b7db20ace1cb750f8e9e06e7e9c7c6585ace42',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling markupsafe
  # and whatever else without interference from each other.
  'markupsafe_revision': '9f8efc8637f847ab1ba984212598e6fb9cf1b3d4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling nasm_source
  # and whatever else without interference from each other.
  'nasm_source_revision': 'f477acb1049f5e043904b87b825c5915084a9a29',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling Ninja CIPD package version
  # and whatever else without interference from each other.
  'ninja_version': 'version:3@1.12.1.chromium.4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling partition_allocator
  # and whatever else without interference from each other.
  'partition_allocator_revision': '9cab8b0d103998c4552b3ac14ef8429b7fe652c4',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling pdfium_tests
  # and whatever else without interference from each other.
  'pdfium_tests_revision': '7e9430c92e07cf45bd151bd4158a7795d117ffeb',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling resultdb
  # and whatever else without interference from each other.
  'resultdb_version': 'git_revision:ebc74d10fa0d64057daa6f128e89f3672eeeec95',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling rust
  # and whatever else without interference from each other.
  'rust_revision': '08704ba66941c48f570b21152b9ec1745aa7ef52',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling skia
  # and whatever else without interference from each other.
  'skia_revision': '975788ea90f24519aa8740085378d1a8db05f5d2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling test_fonts
  # and whatever else without interference from each other.
  'test_fonts_revision': '7f51783942943e965cd56facf786544ccfc07713',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling testing_rust
  # and whatever else without interference from each other.
  'testing_rust_revision': '6712dc59f4a6c5626f391057cded3842700a17eb',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_memory
  # and whatever else without interference from each other.
  'tools_memory_revision': '8e9b58419b41fe0d9dfe2e870f3164f446a0d7ab',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_rust
  # and whatever else without interference from each other.
  'tools_rust_revision': '474e007a4ee99c5f4cff3e7d22511bbc2cb03517',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling v8
  # and whatever else without interference from each other.
  'v8_revision': '75be3dcb5330aa3ebcf79031e7123bc3f0f07179',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling zlib
  # and whatever else without interference from each other.
  'zlib_revision': 'b763971bcaa3e4c50a344035837a1dd60d7e6753',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, and the new host is not in Chromium's DEPS
# file's allowed_hosts list, contact Chrome infrastructure team.
allowed_hosts = [
  'chromium.googlesource.com',
  'pdfium.googlesource.com',
  'skia.googlesource.com',

   # TODO(337061377): Move into a separate allowed gcs bucket list.
  'chromium-browser-clang',
]

deps = {
  'base/allocator/partition_allocator':
    Var('chromium_git') +
        '/chromium/src/base/allocator/partition_allocator.git@' +
        Var('partition_allocator_revision'),

  'build':
    Var('chromium_git') + '/chromium/src/build.git@' + Var('build_revision'),

  'buildtools':
    Var('chromium_git') + '/chromium/src/buildtools.git@' +
        Var('buildtools_revision'),

  'buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux"',
  },

  'buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac"',
  },

  'buildtools/reclient': {
    'packages': [
      {
        'package': Var('reclient_package') + '${{platform}}',
        'version': Var('reclient_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "win"',
  },

  'testing/corpus': {
    'url': Var('pdfium_git') + '/pdfium_tests@' + Var('pdfium_tests_revision'),
    'condition': 'checkout_testing_corpus',
  },

  'testing/scripts/rust': {
    'url': Var('chromium_git') + '/chromium/src/testing/scripts/rust.git@' +
        Var('testing_rust_revision'),
    'condition': 'checkout_rust',
  },

  'third_party/abseil-cpp':
    Var('chromium_git') + '/chromium/src/third_party/abseil-cpp.git@' +
        Var('abseil_revision'),

  'third_party/android_toolchain/ndk': {
    'packages': [
      {
        'package': 'chromium/third_party/android_toolchain/android_toolchain',
        'version': Var('android_toolchain_version'),
      },
    ],
    'condition': 'checkout_android_native_support',
    'dep_type': 'cipd',
  },

  'third_party/catapult': {
    'url': Var('chromium_git') + '/catapult.git@' + Var('catapult_revision'),
    'condition': 'checkout_android',
  },

  'third_party/clang-format/script':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/clang/tools/clang-format.git@' +
        Var('clang_format_revision'),

  'third_party/cpu_features/src': {
    'url': Var('chromium_git') +
        '/external/github.com/google/cpu_features.git@' +
        Var('cpu_features_revision'),
    'condition': 'checkout_android',
  },

  'third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git@' +
        Var('depot_tools_revision'),

  'third_party/fast_float/src':
    Var('chromium_git') + '/external/github.com/fastfloat/fast_float.git@' +
        Var('fast_float_revision'),

  'third_party/fp16/src':
    Var('chromium_git') + '/external/github.com/Maratyszcza/FP16.git@' +
        Var('fp16_revision'),

  'third_party/freetype/src':
    Var('chromium_git') + '/chromium/src/third_party/freetype2.git@' +
        Var('freetype_revision'),

  'third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git@' +
        Var('gtest_revision'),

  'third_party/highway/src':
    Var('chromium_git') + '/external/github.com/google/highway.git@' +
        Var('highway_revision'),

  'third_party/icu':
    Var('chromium_git') + '/chromium/deps/icu.git@' + Var('icu_revision'),

  'third_party/instrumented_libs':
    Var('chromium_git') +
        '/chromium/third_party/instrumented_libraries.git@' +
        Var('instrumented_lib_revision'),

  'third_party/jinja2':
    Var('chromium_git') + '/chromium/src/third_party/jinja2.git@' +
        Var('jinja2_revision'),

  'third_party/libc++/src':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libcxx.git@' +
        Var('libcxx_revision'),

  'third_party/libc++abi/src':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libcxxabi.git@' +
        Var('libcxxabi_revision'),

  'third_party/libunwind/src':
    Var('chromium_git') +
        '/external/github.com/llvm/llvm-project/libunwind.git@' +
        Var('libunwind_revision'),

  'third_party/libjpeg_turbo':
    Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git@' +
        Var('jpeg_turbo_revision'),

  'third_party/libpng':
    Var('chromium_git') + '/chromium/src/third_party/libpng.git@' +
        Var('libpng_revision'),

  'third_party/llvm-build/Release+Asserts': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        # The Android libclang_rt.builtins libraries are currently only included in the Linux clang package.
        'object_name': 'Linux_x64/clang-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'e649b5ae7db215ede1d17d63e60f187c9773bc3bb465e4ff25148f58783c0588',
        'size_bytes': 54177680,
        'generation': 1736382900858370,
        'condition': '(host_os == "linux" or checkout_android) and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-tidy-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '98aeee7a6ea7b555868c16d009f0232c421ef88cb5a6a60c38b78255dd4faced',
        'size_bytes': 13149440,
        'generation': 1736382900905802,
        'condition': 'host_os == "linux" and checkout_clang_tidy and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clangd-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '767ad010ec7d99a62a2c6535cca8f3bc55c4866b2619462fb303e0bc9a5f1977',
        'size_bytes': 27687240,
        'generation': 1736382900911727,
        'condition': 'host_os == "linux" and checkout_clangd and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvm-code-coverage-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'b61cee4a240859bab2b541721a5afd8333c0fc4eea2e568a63cf71529efefa76',
        'size_bytes': 2289668,
        'generation': 1736382900990620,
        'condition': 'host_os == "linux" and checkout_clang_coverage_tools and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'a2eb777a2013c9d647c957b9cbb3d3937b66c6443ed9118d8a1eeb3fea6250d3',
        'size_bytes': 5348020,
        'generation': 1736382900960786,
        'condition': '(checkout_linux or checkout_mac or checkout_android and host_os != "mac") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '40b2964db638f50f62c410202a0e150809b521eab8d764344dfcc82ce2fedda1',
        'size_bytes': 49122796,
        'generation': 1736382902685415,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '6dd34f8c8b70a1bf1453a74204c62669abe84e8e7f9625cbc16327b3fab8b943',
        'size_bytes': 972748,
        'generation': 1736382909396308,
        'condition': 'checkout_mac and not host_os == "mac"',
      },
      {
        'object_name': 'Mac/clang-tidy-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '194df82314e98d9c2b8c3d0810706bdedcdd60997b622eea07a1628ab94f28c3',
        'size_bytes': 12765652,
        'generation': 1736382902742379,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac/clangd-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'a151f0d3958c8f469339c57f60e336cbf066fa94c5c841ac6b8e05cd09077890',
        'size_bytes': 26561888,
        'generation': 1736382902744981,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clangd',
      },
      {
        'object_name': 'Mac/llvm-code-coverage-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'e14e803b4841a7a392f960278e7225b61453f1cca3086e7cdfb1c3cee813a85d',
        'size_bytes': 2250480,
        'generation': 1736382902843064,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '11238b743b1c3157b64f261118ee0884f5b7f5f9afc235d4c47c1afc52556b1b',
        'size_bytes': 43127752,
        'generation': 1736382911237265,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/clang-tidy-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '00a42f1c03272d8f9e0c9531712110422936fb9365c66206becdc7dfa27500d8',
        'size_bytes': 11362620,
        'generation': 1736382911277357,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac_arm64/clangd-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'eb2c179811101e49a6156b55624a6292688c0a4ef1e79d0b05ef03f2a17f405f',
        'size_bytes': 22998836,
        'generation': 1736382911247931,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clangd',
      },
      {
        'object_name': 'Mac_arm64/llvm-code-coverage-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '9c06d812bf98fc1ca1de9afaf031efa2334b3c1039969ee90689a810726db83c',
        'size_bytes': 1977132,
        'generation': 1736382911314127,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/clang-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '554d4d0b22b7c682dd05bb912b9f2c1a13248d38660f3f1e15444ef0970fb0e5',
        'size_bytes': 46344548,
        'generation': 1736382920155077,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-tidy-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'fbb40ae15b7c2fc408d8329eb6274d3422c6fb4110f27655adb8cd642d7b62b5',
        'size_bytes': 12970212,
        'generation': 1736382920246226,
        'condition': 'host_os == "win" and checkout_clang_tidy',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '98a76f1ec725bbf112fe91ba007bc39ca1efe1f1923d1c5a7cf2812e8c34b218',
        'size_bytes': 2458356,
        'generation': 1736382926959753,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/clangd-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': 'f766b3623240184b7d668b8e4e9c6070f6c56ee676cfed9a98375653922ccda8',
        'size_bytes': 25602964,
        'generation': 1736382920261395,
       'condition': 'host_os == "win" and checkout_clangd',
      },
      {
        'object_name': 'Win/llvm-code-coverage-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '7f23a02c44dac5fb1139cc5943fc764b7c49bab152c76a416785f1413cfbaf0d',
        'size_bytes': 2361572,
        'generation': 1736382920285247,
        'condition': 'host_os == "win" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-20-init-17108-g29ed6000-1.tar.xz',
        'sha256sum': '8884520329694e6f35ab9790f65a474e04bafc541d6d40592f43356f44ee0d40',
        'size_bytes': 5439908,
        'generation': 1736382920291826,
        'condition': 'checkout_linux or checkout_mac or checkout_android and host_os == "win"',
      },
    ]
  },

  'third_party/llvm-libc/src':
    Var('chromium_git') + '/external/github.com/llvm/llvm-project/libc.git@' +
        Var('llvm_libc_revision'),

  'third_party/markupsafe':
    Var('chromium_git') + '/chromium/src/third_party/markupsafe.git@' +
        Var('markupsafe_revision'),

  'third_party/nasm':
    Var('chromium_git') + '/chromium/deps/nasm.git@' +
        Var('nasm_source_revision'),

  'third_party/ninja': {
    'packages': [
      {
        # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'dep_type': 'cipd',
  },

  'third_party/rust': {
    'url': Var('chromium_git') + '/chromium/src/third_party/rust@' +
        Var('rust_revision'),
    'condition': 'checkout_rust',
  },

  'third_party/rust-toolchain': {
    'dep_type': 'gcs',
    'bucket': 'chromium-browser-clang',
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-ad211ced81509462cdfe4c29ed10f97279a0acae-1-llvmorg-20-init-17108-g29ed6000.tar.xz',
        'sha256sum': '1363529d7285ca8623c6408d49419921d459bbc461fa26ae57f5b5a620a57cc2',
        'size_bytes': 124591376,
        'generation': 1736456746490086,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-ad211ced81509462cdfe4c29ed10f97279a0acae-1-llvmorg-20-init-17108-g29ed6000.tar.xz',
        'sha256sum': 'bc2446c14bf6316ab2a5797b65daf229aecb65e1fa25184ec740137196ec111e',
        'size_bytes': 117535632,
        'generation': 1736456748324065,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-ad211ced81509462cdfe4c29ed10f97279a0acae-1-llvmorg-20-init-17108-g29ed6000.tar.xz',
        'sha256sum': '879947f21091c62096297c34c3e0a3f2e1f79260fb9b3c3c40091d62c65fb04e',
        'size_bytes': 102626588,
        'generation': 1736456750286270,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-ad211ced81509462cdfe4c29ed10f97279a0acae-1-llvmorg-20-init-17108-g29ed6000.tar.xz',
        'sha256sum': '294a0a338bc46626b38d0d243f6f9a308dc2517cbf8961e2dc0edbf51b8d0ae9',
        'size_bytes': 176895424,
        'generation': 1736456752082017,
        'condition': 'host_os == "win"',
      },
    ],
  },

  'third_party/skia': {
    'url': Var('skia_git') + '/skia.git@' + Var('skia_revision'),
    'condition': 'checkout_skia',
  },

  'third_party/test_fonts':
    Var('chromium_git') + '/chromium/src/third_party/test_fonts.git@' +
        Var('test_fonts_revision'),

  'third_party/zlib':
    Var('chromium_git') + '/chromium/src/third_party/zlib.git@' +
        Var('zlib_revision'),

  'tools/clang':
    Var('chromium_git') + '/chromium/src/tools/clang@' + Var('clang_revision'),

  'tools/code_coverage':
    Var('chromium_git') + '/chromium/src/tools/code_coverage.git@' +
        Var('code_coverage_revision'),

  'tools/memory':
    Var('chromium_git') + '/chromium/src/tools/memory@' +
        Var('tools_memory_revision'),

  'tools/resultdb': {
    'packages': [
      {
        'package': 'infra/tools/result_adapter/${{platform}}',
        'version': Var('resultdb_version'),
      },
    ],
    'dep_type': 'cipd',
  },

  'tools/rust': {
    'url': Var('chromium_git') + '/chromium/src/tools/rust@' +
        Var('tools_rust_revision'),
    'condition': 'checkout_rust',
  },

  'tools/skia_goldctl/linux': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/linux-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_linux',
  },

  'tools/skia_goldctl/mac_amd64': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/mac-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_mac',
  },

  'tools/skia_goldctl/mac_arm64': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/mac-arm64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_mac',
  },

  'tools/skia_goldctl/win': {
    'packages': [
      {
        'package': 'skia/tools/goldctl/windows-amd64',
        'version': Var('goldctl_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'checkout_win',
  },

  'v8': {
    'url': Var('chromium_git') + '/v8/v8.git@' + Var('v8_revision'),
    'condition': 'checkout_v8',
  },

}

recursedeps = [
  'build',
  'buildtools',
  'third_party/instrumented_libs',
]

include_rules = [
  # Basic stuff that everyone can use.
  # Note: public is not here because core cannot depend on public.
  '+build/build_config.h',
  '+constants',
  '+testing',

  # Abseil is allowed by default, but some features are banned. See Chromium's
  # //styleguide/c++/c++-features.md.
  '+third_party/abseil-cpp',
  '-third_party/abseil-cpp/absl/algorithm/container.h',
  '-third_party/abseil-cpp/absl/base/no_destructor.h',
  '-third_party/abseil-cpp/absl/base/nullability.h',
  '-third_party/abseil-cpp/absl/container',
  '+third_party/abseil-cpp/absl/container/inlined_vector.h',
  '-third_party/abseil-cpp/absl/crc',
  '-third_party/abseil-cpp/absl/flags',
  '-third_party/abseil-cpp/absl/functional/any_invocable.h',
  '-third_party/abseil-cpp/absl/functional/bind_front.h',
  '-third_party/abseil-cpp/absl/functional/function_ref.h',
  '-third_party/abseil-cpp/absl/functional/overload.h',
  '-third_party/abseil-cpp/absl/hash',
  '-third_party/abseil-cpp/absl/log',
  '-third_party/abseil-cpp/absl/random',
  '-third_party/abseil-cpp/absl/status/statusor.h',
  '-third_party/abseil-cpp/absl/strings',
  '+third_party/abseil-cpp/absl/strings/ascii.h',
  '+third_party/abseil-cpp/absl/strings/cord.h',
  '-third_party/abseil-cpp/absl/synchronization',
  '-third_party/abseil-cpp/absl/time',
  '-third_party/abseil-cpp/absl/types/any.h',
  '-third_party/abseil-cpp/absl/types/optional.h',
  '-third_party/abseil-cpp/absl/types/span.h',
]

specific_include_rules = {
  # Allow embedder tests to use public APIs.
  '(.*embeddertest\.cpp)': [
    '+public',
  ]
}

hooks = [
  {
    # Ensure that the DEPS'd "depot_tools" has its self-update capability
    # disabled.
    'name': 'disable_depot_tools_selfupdate',
    'pattern': '.',
    'action': [ 'python3',
                'third_party/depot_tools/update_depot_tools_toggle.py',
                '--disable',
    ],
  },
  {
    # Case-insensitivity for the Win SDK. Must run before win_toolchain below.
    'name': 'ciopfs_linux',
    'pattern': '.',
    'condition': 'checkout_win and host_os == "linux"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang/ciopfs',
                '-s', 'build/ciopfs.sha1',
    ]
  },
  {
    # Update the Windows toolchain if necessary.  Must run before 'clang' below.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'build/vs_toolchain.py', 'update', '--force'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['python3', 'build/mac_toolchain.py'],
  },
  # Pull dsymutil binaries using checked-in hashes.
  {
    'name': 'dsymutil_mac_arm64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "arm64"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang',
                '-s', 'tools/clang/dsymutil/bin/dsymutil.arm64.sha1',
                '-o', 'tools/clang/dsymutil/bin/dsymutil',
    ],
  },
  {
    'name': 'dsymutil_mac_x64',
    'pattern': '.',
    'condition': 'host_os == "mac" and host_cpu == "x64"',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-browser-clang',
                '-s', 'tools/clang/dsymutil/bin/dsymutil.x64.sha1',
                '-o', 'tools/clang/dsymutil/bin/dsymutil',
    ],
  },
  {
    'name': 'test_fonts',
    'pattern': '.',
    'action': [ 'python3',
                'third_party/depot_tools/download_from_google_storage.py',
                '--no_resume',
                '--extract',
                '--no_auth',
                '--bucket', 'chromium-fonts',
                '-s', 'third_party/test_fonts/test_fonts.tar.gz.sha1',
    ],
  },
  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'action': ['python3', 'build/util/lastchange.py',
               '-o', 'build/util/LASTCHANGE'],
  },
  # Configure remote exec cfg files
  {
    # Use luci_auth if on windows and using chrome-untrusted project
    'name': 'download_and_configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'download_remoteexec_cfg and host_os == "win"',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--use_luci_auth_credshelper',
               '--quiet',
               ],
  },  {
    'name': 'download_and_configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'download_remoteexec_cfg and not host_os == "win"',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--quiet',
               ],
  },
  {
    'name': 'configure_reclient_cfgs',
    'pattern': '.',
    'condition': 'not download_remoteexec_cfg',
    'action': ['python3',
               'buildtools/reclient_cfgs/configure_reclient_cfgs.py',
               '--rbe_instance',
               Var('rbe_instance'),
               '--reproxy_cfg_template',
               'reproxy.cfg.template',
               '--rewrapper_cfg_project',
               Var('rewrapper_cfg_project'),
               '--skip_remoteexec_cfg_fetch',
               '--quiet',
               ],
  },
]
