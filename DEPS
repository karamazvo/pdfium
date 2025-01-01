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
  'reclient_version': 're_client_version:0.171.0.1869ad5c-gomaip',

  'chromium_git': 'https://chromium.googlesource.com',
  'pdfium_git': 'https://pdfium.googlesource.com',
  'skia_git': 'https://skia.googlesource.com',

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling abseil
  # and whatever else without interference from each other.
  'abseil_revision': '0b76dfe4fffd3f49efacde5f3268b02f9c060ef0',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling android_toolchain
  # and whatever else without interference from each other.
  'android_toolchain_version': 'Idl-vYnWGnM8K3XJhM3h6zjYVDXlnljVz3FE00V9IM8C',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling build
  # and whatever else without interference from each other.
  'build_revision': '84fa09ff44bbdeef23b01d87078a63dac04dae8b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling buildtools
  # and whatever else without interference from each other.
  'buildtools_revision': '3d0ed6dead278a4c5cf2fecc4e3b8cd19b0a8369',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': '6a0960fe97abb929b6768af4137a006e785fd663',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang format
  # and whatever else without interference from each other.
  'clang_format_revision': '3c0acd2d4e73dd911309d9e970ba09d58bf23a62',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling clang
  # and whatever else without interference from each other.
  'clang_revision': '634f2acc7bb334d2e6da78e206220f9c00e2e66a',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'code_coverage_revision': '719f1eba43796fa59c79c2cb5541bca82998a11d',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling code_coverage
  # and whatever else without interference from each other.
  'cpu_features_revision': '936b9ab5515dead115606559502e3864958f7f6e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling depot_tools
  # and whatever else without interference from each other.
  'depot_tools_revision': '8d20c1e0b56c68b14878b597080484f9445962b4',
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
  'freetype_revision': '0ae7e607370cc66218ccfacf5de4db8a35424c2f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling GN CIPD package version
  # and whatever else without interference from each other.
  'gn_version': 'git_revision:468c6128db7fabe32a29d4753460ef53594406fc',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling goldctl CIPD package version
  # and whatever else without interference from each other.
  'goldctl_version': 'git_revision:db814b551104336b5e0dc7053ae2e4f5aa4bed65',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling gtest
  # and whatever else without interference from each other.
  'gtest_revision': 'd144031940543e15423a25ae5a8a74141044862f',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling icu
  # and whatever else without interference from each other.
  'icu_revision': '4239b1559d11d4fa66c100543eda4161e060311e',
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
  'libcxx_revision': 'eb760140f3825eec128ac24306e95469a9df4e3e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libc++abi
  # and whatever else without interference from each other.
  'libcxxabi_revision': '6c4fa00e4becc30085cbc8b44c2764ef307daae2',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libpng
  # and whatever else without interference from each other.
  'libpng_revision': 'cf7c36ed084cd051100c4a1597a901c08377b835',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libunwind
  # and whatever else without interference from each other.
  'libunwind_revision': '5b01ea4a6f3b666b7d190e7cb7c31db2ed4d94ce',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling llvm-libc
  # and whatever else without interference from each other.
  'llvm_libc_revision': '4c70d6b5a62aa6981ba6b2e905b90bd650fb3626',
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
  'partition_allocator_revision': 'c551156ef8ccc090e52941d58c22647326066ff3',
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
  'rust_revision': 'cecd4aabb2c2cc4342021f141997281a4f234faa',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling skia
  # and whatever else without interference from each other.
  'skia_revision': '3db026d62805afc2b8ba44de25ebec3154062115',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling test_fonts
  # and whatever else without interference from each other.
  'test_fonts_revision': '7f51783942943e965cd56facf786544ccfc07713',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling testing_rust
  # and whatever else without interference from each other.
  'testing_rust_revision': '347b3c20a65d76c83757a9f9db5f09d316115907',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_memory
  # and whatever else without interference from each other.
  'tools_memory_revision': '8e9b58419b41fe0d9dfe2e870f3164f446a0d7ab',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling tools_rust
  # and whatever else without interference from each other.
  'tools_rust_revision': 'f7e4078bb01913cc6c66eed10b6a6f45d5ee8027',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling v8
  # and whatever else without interference from each other.
  'v8_revision': '313e6ed365288a983b27286c054da4d989783a71',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling zlib
  # and whatever else without interference from each other.
  'zlib_revision': '82a5fecf8aae8f288267cfdb2d29c9ebf7b37e59',
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
        'object_name': 'Linux_x64/clang-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'e2cc96aef6e4c90525bb47401e3310ab2012063e7090df609b6e7ee4e0cedcf5',
        'size_bytes': 54419828,
        'generation': 1733131121091279,
        'condition': '(host_os == "linux" or checkout_android) and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clang-tidy-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '6772829b59e79aae1e6f5d68c2be1d7d9610cbd879a0c02d132eda8bab224f5c',
        'size_bytes': 13097604,
        'generation': 1733131121366758,
        'condition': 'host_os == "linux" and checkout_clang_tidy and non_git_source',
      },
      {
        'object_name': 'Linux_x64/clangd-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '63475ffe32d4ae2432d1dd1eeb48bb69794ddc4f78ff15a913f77019d5e5b365',
        'size_bytes': 27679064,
        'generation': 1733131121482543,
        'condition': 'host_os == "linux" and checkout_clangd and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvm-code-coverage-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'b7128ce662619356de464d1c37a70342707d23ca8549c34e8e487e2d5638deb4',
        'size_bytes': 2393880,
        'generation': 1733131121884494,
        'condition': 'host_os == "linux" and checkout_clang_coverage_tools and non_git_source',
      },
      {
        'object_name': 'Linux_x64/llvmobjdump-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '1e13a5c0085db969256e237ea93e57b373589e3342a21f353444c7978ffe5392',
        'size_bytes': 5541520,
        'generation': 1733131121622535,
        'condition': '(checkout_linux or checkout_mac or checkout_android and host_os != "mac") and non_git_source',
      },
      {
        'object_name': 'Mac/clang-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '319ece3dde9edc65dea6fd7ef12feb26c78c6c775749f0c66ef8da0c73bf02ea',
        'size_bytes': 48880736,
        'generation': 1733131123302515,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac/clang-mac-runtime-library-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '3a564112c5d7174c036145052e88fba6b2bcd29f5818fba8a91ce0fce4688be7',
        'size_bytes': 977584,
        'generation': 1733131140922386,
        'condition': 'checkout_mac and not host_os == "mac"',
      },
      {
        'object_name': 'Mac/clang-tidy-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '6a53994ba0a1681097218acc01530a021be8332aa7f6779be3fa7fe04f873a62',
        'size_bytes': 12673340,
        'generation': 1733131123665839,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac/clangd-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '4e24ba1dcef6b0b59176167d8ef56d338fa86c97726ab4b9ab52db0950b99653',
        'size_bytes': 26074940,
        'generation': 1733131123670297,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clangd',
      },
      {
        'object_name': 'Mac/llvm-code-coverage-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'e228350dad97e33bcb9b5490d2b247284374d993bc383606a0dc01e77bf58457',
        'size_bytes': 2255152,
        'generation': 1733131124117880,
        'condition': 'host_os == "mac" and host_cpu == "x64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Mac_arm64/clang-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'de28b9321ac84ebcb122e9c2dd4d743efbb3812d19a45b465c3e86de448b2e26',
        'size_bytes': 42969444,
        'generation': 1733131142308815,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Mac_arm64/clang-tidy-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '8c6fad4b674772df164223546af7e37d64958a5d4ccf4c6a85310b82a2f8ad3b',
        'size_bytes': 11274400,
        'generation': 1733131142586873,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_tidy',
      },
      {
        'object_name': 'Mac_arm64/clangd-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'fa2bd872d38a92c5b5fb93d5fd2a6820d067096856abf3b0a0c129f81cc0f51b',
        'size_bytes': 22709972,
        'generation': 1733131142721437,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clangd',
      },
      {
        'object_name': 'Mac_arm64/llvm-code-coverage-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': 'a74fc47a3475e24898048155638a28db4c001cf88d1b3c072616b8aa13873ad5',
        'size_bytes': 1980832,
        'generation': 1733131143158718,
        'condition': 'host_os == "mac" and host_cpu == "arm64" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/clang-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '1a651c9c3e64ad8c1e2417c527226ef93ebf15a096b9daba2c92208137a9ef21',
        'size_bytes': 46202484,
        'generation': 1733131163286134,
        'condition': 'host_os == "win"',
      },
      {
        'object_name': 'Win/clang-tidy-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '780c66e1c39c2cce937887944d32176483fd08b2f4990d20816293be5d95d37b',
        'size_bytes': 12901244,
        'generation': 1733131163613393,
        'condition': 'host_os == "win" and checkout_clang_tidy',
      },
      {
        'object_name': 'Win/clang-win-runtime-library-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '57dab4c3dc4f70b0249643979d40249f76cfe85c56c5773bbf65efd170f5e6b3',
        'size_bytes': 2477936,
        'generation': 1733131182887719,
        'condition': 'checkout_win and not host_os == "win"',
      },
      {
        'object_name': 'Win/clangd-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '462df1784a09e1d7fd7c7ff62856c8bc9ab68c4ecb59de375c1cfcaaf88cc80f',
        'size_bytes': 25482156,
        'generation': 1733131163767856,
       'condition': 'host_os == "win" and checkout_clangd',
      },
      {
        'object_name': 'Win/llvm-code-coverage-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '10b8ec087f9bc4bd882134a33b71cbd315b5b65e615d1ec05abcf6ddda78f687',
        'size_bytes': 2402404,
        'generation': 1733131164203293,
        'condition': 'host_os == "win" and checkout_clang_coverage_tools',
      },
      {
        'object_name': 'Win/llvmobjdump-llvmorg-20-init-13894-g8cb44859-1.tar.xz',
        'sha256sum': '6249c10ff32bdb66d02fe44b08d50ae7e83baaea9981f158c7f3e2cccb112ab0',
        'size_bytes': 5543640,
        'generation': 1733131163939384,
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
        'object_name': 'Linux_x64/rust-toolchain-a522d78598415cdd614ccc6d961160f192f64b5c-1-llvmorg-20-init-13894-g8cb44859.tar.xz',
        'sha256sum': 'dce38497f0f07e48fe15f0a7565baf7bd77f824ced74b888182acfa920253307',
        'size_bytes': 124315836,
        'generation': 1733131115209007,
        'condition': 'host_os == "linux" and non_git_source',
      },
      {
        'object_name': 'Mac/rust-toolchain-a522d78598415cdd614ccc6d961160f192f64b5c-1-llvmorg-20-init-13894-g8cb44859.tar.xz',
        'sha256sum': 'eadff4d1eef4ce971c2ba13c292ed03862e65e5d58be23a91a135b17bede41dc',
        'size_bytes': 116695668,
        'generation': 1733131116590926,
        'condition': 'host_os == "mac" and host_cpu == "x64"',
      },
      {
        'object_name': 'Mac_arm64/rust-toolchain-a522d78598415cdd614ccc6d961160f192f64b5c-1-llvmorg-20-init-13894-g8cb44859.tar.xz',
        'sha256sum': '1fe555df5c0772721334e508cae0d1d7e59409c264a11bb4be2db5ab6ec75296',
        'size_bytes': 101961216,
        'generation': 1733131117910605,
        'condition': 'host_os == "mac" and host_cpu == "arm64"',
      },
      {
        'object_name': 'Win/rust-toolchain-a522d78598415cdd614ccc6d961160f192f64b5c-1-llvmorg-20-init-13894-g8cb44859.tar.xz',
        'sha256sum': '70364b270bcc75bb30a572ca40a85f67c354409171c7018caee4a446b4abd3db',
        'size_bytes': 176646316,
        'generation': 1733131119397229,
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
