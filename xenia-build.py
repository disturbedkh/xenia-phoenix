#!/usr/bin/env python3

# Copyright 2025 Ben Vanik. All Rights Reserved.

"""Main build script and tooling for xenia.

Run with --help or no arguments for possible commands.
"""
from datetime import datetime
from multiprocessing import Pool
from functools import partial
from argparse import ArgumentParser, ArgumentTypeError
from glob import glob
from json import loads as jsonloads
import os
from re import findall as re_findall
import platform
from shutil import rmtree
import subprocess
import sys
import stat
import enum

__author__ = "ben.vanik@gmail.com (Ben Vanik)"


self_path = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(self_path, "tools", "build"))
import xenia_paths as xp

# TODO: Automate getting these entries
XENIA_TARGETS = [
    "aes_128",
    "capstone",
    "dxbc",
    "discord-rpc",
    "fmt",
    "glslang-spirv",
    "imgui",
    "libavcodec",
    "libavformat",
    "libavutil",
    "mspack",
    "pugixml",
    "snappy",
    "xxhash",
    "zarchive",
    "zlib-ng",
    "zstd",
    "xenia-app",
    "xenia-app-discord",
    "xenia-apu",
    "xenia-apu-alsa",
    "xenia-apu-nop",
    "xenia-apu-sdl",
    "xenia-base",
    "xenia-core",
    "xenia-cpu",
    "xenia-cpu-backend-x64",
    "xenia-debug-ui",
    "xenia-gpu",
    "xenia-gpu-null",
    "xenia-gpu-vulkan",
    "xenia-gpu-vulkan-spirv-shaders",
    "xenia-helper-sdl",
    "xenia-hid",
    "xenia-hid-nop",
    "xenia-hid-sdl",
    "xenia-hid-skylander",
    "xenia-kernel",
    "xenia-patcher",
    "xenia-ui",
    "xenia-ui-vulkan",
    "xenia-ui-vulkan-spirv-shaders",
    "xenia-vfs",
]

class bcolors:
#    HEADER = "\033[95m"
#    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
#    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
#    BOLD = "\033[1m"
#    UNDERLINE = "\033[4m"

def print_error(text: str):
    print(f"{bcolors.FAIL}ERROR: {text}{bcolors.ENDC}")

def print_warning(text: str):
    print(f"{bcolors.WARNING}WARNING: {text}{bcolors.ENDC}")

class ResultStatus(enum.Enum):
    SUCCESS = enum.auto()
    FAILURE = enum.auto()

def print_status(status: ResultStatus):
    if status == ResultStatus.SUCCESS:
        print(f"{bcolors.OKCYAN}Success!{bcolors.ENDC}")
    elif status == ResultStatus.FAILURE:
        print(f"{bcolors.FAIL}Error!{bcolors.ENDC}")


# Detect if building on Android via Termux.
host_linux_platform_is_android = False
if sys.platform == "linux":
    try:
        host_linux_platform_is_android = subprocess.Popen(
            ["uname", "-o"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True).communicate()[0] == "Android\n"
    except Exception:
        pass


def import_subprocess_environment(args):
    popen = subprocess.Popen(
        args, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    variables, _ = popen.communicate()

    envvars_to_save = (
        "DEVENVDIR",
        "INCLUDE",
        "LIB",
        "LIBPATH",
        "PATH",
        "PATHEXT",
        "SYSTEMROOT",
        "TEMP",
        "TMP",
        "VCINSTALLDIR",
        "WindowsSdkDir",
        "PROGRAMFILES",
        "ProgramFiles(x86)",
        "VULKAN_SDK"
        "CC",
        "CXX",
        )

    # Extract and parse environment variables from stdout
    for line in variables.splitlines():
        if line.find("=") != -1:
            for envvar in envvars_to_save:
                var, setting = line.split("=", 1)

                var = var.upper()
                envvar = envvar.upper()

                if envvar == var:
                    if envvar == "PATH":
                        setting = f"{os.path.dirname(sys.executable)}{os.pathsep}{setting}"

                    os.environ[var] = setting
                    break

VSVERSION_MINIMUM = 2022
def import_vs_environment():
    """Finds the installed Visual Studio version and imports
    interesting environment variables into os.environ.

    Returns:
      A version such as 2022 or None if no installation is found.
    """

    if sys.platform != "win32":
        return None

    version = None
    install_path = None
    env_tool_args = None

    vswhere = subprocess.check_output(
        "tools/vswhere/vswhere.exe -version \"[17,)\" -latest -prerelease -format json -utf8 -products"
        " Microsoft.VisualStudio.Product.Enterprise"
        " Microsoft.VisualStudio.Product.Professional"
        " Microsoft.VisualStudio.Product.Community"
        " Microsoft.VisualStudio.Product.BuildTools",
        encoding="utf-8",
    )
    if vswhere:
        vswhere = jsonloads(vswhere)
    if vswhere and len(vswhere) > 0:
        version = int(vswhere[0].get("catalog", {}).get("productLineVersion", VSVERSION_MINIMUM))
        install_path = vswhere[0].get("installationPath", None)

    vsdevcmd_path = os.path.join(install_path, "Common7", "Tools", "VsDevCmd.bat")
    if os.access(vsdevcmd_path, os.X_OK):
        env_tool_args = [vsdevcmd_path, "-arch=amd64", "-host_arch=amd64", "&&", "set"]
    else:
        vcvars_path = os.path.join(install_path, "VC", "Auxiliary", "Build", "vcvarsall.bat")
        env_tool_args = [vcvars_path, "x64", "&&", "set"]

    if not version:
        return None

    import_subprocess_environment(env_tool_args)
    os.environ["VSVERSION"] = f"{version}"
    return version


vs_version = import_vs_environment()

default_branch = "canary_experimental"

def setup_vulkan_sdk():
    """Setup Vulkan SDK environment variables if not already set.

    Returns:
        True if Vulkan SDK is available and valid, False otherwise.
    """
    # Check if VULKAN_SDK is already set and valid
    existing_vulkan_sdk = os.environ.get("VULKAN_SDK")
    if existing_vulkan_sdk:
        if os.path.exists(existing_vulkan_sdk):
            if has_bin("spirv-opt"):
                print(f"VULKAN_SDK is set to {existing_vulkan_sdk}")
                return True
            print_warning(f"VULKAN_SDK is set to {existing_vulkan_sdk} but spirv-opt not found in PATH")
        else:
            print_warning(f"VULKAN_SDK is set to {existing_vulkan_sdk} but directory does not exist")
        return False

    if sys.platform != "win32":
        # On Linux, find spirv-opt in PATH and set VULKAN_SDK based on its location
        spirv_opt_path = get_bin("spirv-opt")
        if spirv_opt_path:
            # spirv-opt is typically in $VULKAN_SDK/bin/, so get parent directory
            spirv_bin_dir = os.path.dirname(spirv_opt_path)
            vulkan_sdk = os.path.dirname(spirv_bin_dir)
            os.environ["VULKAN_SDK"] = vulkan_sdk
            print(f"Found Vulkan SDK at {vulkan_sdk} (from spirv-opt location)")
            return True
        return False

    # Windows: Check if Vulkan SDK is installed at the default location
    vulkan_base = "C:\\VulkanSDK"
    if not os.path.exists(vulkan_base):
        return False

    try:
        subdirs = [d for d in os.listdir(vulkan_base)
                   if os.path.isdir(os.path.join(vulkan_base, d))]
        if not subdirs:
            return False

        vulkan_sdk = os.path.join(vulkan_base, subdirs[0])
        vulkan_bin = os.path.join(vulkan_sdk, "Bin")

        os.environ["VULKAN_SDK"] = vulkan_sdk
        os.environ["PATH"] = f"{vulkan_bin}{os.pathsep}{os.environ['PATH']}"

        print(f"Found Vulkan SDK at {vulkan_sdk}")
        return True
    except Exception:
        return False


def get_dir_newest_mtime(directory):
    """Get the newest modification time in a directory tree (files and dirs).

    Checks both files and directories to catch deletions/additions.
    """
    newest = 0
    try:
        for root, dirs, files in os.walk(directory):
            # Skip bytecode subdirectories when scanning source
            dirs[:] = [d for d in dirs if d != "bytecode"]
            # Check directory mtime (changes when files added/removed)
            mtime = os.path.getmtime(root)
            if mtime > newest:
                newest = mtime
            for name in files:
                mtime = os.path.getmtime(os.path.join(root, name))
                if mtime > newest:
                    newest = mtime
    except OSError:
        pass
    return newest


def get_dir_oldest_mtime(directory):
    """Get the oldest modification time in a directory tree (files and dirs).

    Checks both files and directories to catch deletions/additions.
    """
    oldest = float('inf')
    try:
        for root, dirs, files in os.walk(directory):
            # Check directory mtime
            mtime = os.path.getmtime(root)
            if mtime < oldest:
                oldest = mtime
            for name in files:
                mtime = os.path.getmtime(os.path.join(root, name))
                if mtime < oldest:
                    oldest = mtime
    except OSError:
        pass
    return oldest



def main():
    # Add self to the root search path.
    sys.path.insert(0, self_path)

    # Setup Vulkan SDK and check if available
    setup_vulkan_sdk()

    # Augment path to include our fancy things.
    os.environ["PATH"] += os.pathsep + os.pathsep.join([
        self_path,
        os.path.abspath(os.path.join("tools", "build")),
        ])

    # Check git exists.
    if not has_bin("git"):
        print_warning("Git should be installed and on PATH. Version info will be omitted from all binaries!\n")
    elif not git_is_repository():
        print_warning("The source tree is unversioned. Version info will be omitted from all binaries!\n")

    # Check python version.
    python_minimum_ver = 3,6
    if not sys.version_info[:2] >= (python_minimum_ver[0], python_minimum_ver[1]) or not sys.maxsize > 2**32:
        print_error(f"Python {python_minimum_ver[0]}.{python_minimum_ver[1]}+ 64-bit must be installed and on PATH")
        sys.exit(1)

    # Grab Visual Studio version and execute shell to set up environment.
    if sys.platform == "win32" and not vs_version:
        print_warning("Visual Studio not found!"
              "\nBuilding for Windows will not be supported."
              " Please refer to the building guide:"
              f"\nhttps://github.com/xenia-canary/xenia-canary/blob/{default_branch}/docs/building.md")

    # Setup main argument parser and common arguments.
    parser = ArgumentParser(prog="xenia-build.py")

    # Grab all commands and populate the argument parser for each.
    subparsers = parser.add_subparsers(title="subcommands",
                                       dest="subcommand")
    commands = discover_commands(subparsers)

    # If the user passed no args, die nicely.
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(1)

    # Gather any arguments that we want to pass to child processes.
    command_args = sys.argv[1:]
    pass_args = []
    try:
        pass_index = command_args.index("--")
        pass_args = command_args[pass_index + 1:]
        command_args = command_args[:pass_index]
    except Exception:
        pass

    # Parse command name and dispatch.
    args = vars(parser.parse_args(command_args))
    command_name = args["subcommand"]
    try:
        command = commands[command_name]
        return_code = command.execute(args, pass_args, os.getcwd())
    except Exception:
        raise
    sys.exit(return_code)


def print_box(msg):
    """Prints an important message inside a box
    """
    print(
        "┌{0:─^{2}}╖\n"
        "│{1: ^{2}}║\n"
        "╘{0:═^{2}}╝\n"
        .format("", msg, len(msg) + 2))


def has_bin(binary):
    """Checks whether the given binary is present.

    Args:
      binary: binary name (without .exe, etc).

    Returns:
      True if the binary exists.
    """
    bin_path = get_bin(binary)
    if not bin_path:
        return False
    return True


def get_bin(binary):
    """Checks whether the given binary is present and returns the path.

    Args:
      binary: binary name (without .exe, etc).

    Returns:
      Full path to the binary or None if not found.
    """
    for path in os.environ["PATH"].split(os.pathsep):
        path = path.strip("\"")
        exe_file = os.path.join(path, binary)
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            return exe_file
        exe_file += ".exe"
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            return exe_file
    return None


def shell_call(command, throw_on_error=True, stdout_path=None, stderr_path=None, shell=False):
    """Executes a shell command.

    Args:
      command: Command to execute, as a list of parameters.
      throw_on_error: Whether to throw an error or return the status code.
      stdout_path: File path to write stdout output to.
      stderr_path: File path to write stderr output to.

    Returns:
      If throw_on_error is False the status code of the call will be returned.
    """
    stdout_file = None
    if stdout_path:
        stdout_file = open(stdout_path, "w")
    stderr_file = None
    if stderr_path:
        stderr_file = open(stderr_path, "w")
    result = 0
    try:
        if throw_on_error:
            result = 1
            subprocess.check_call(command, shell=shell, stdout=stdout_file, stderr=stderr_file)
            result = 0
        else:
            result = subprocess.call(command, shell=shell, stdout=stdout_file, stderr=stderr_file)
    finally:
        if stdout_file:
            stdout_file.close()
        if stderr_file:
            stderr_file.close()
    return result


def generate_version_h(build_dir=None):
    """Generates version.h in the given build directory with current git info.
    """
    if build_dir is None:
        build_dir = get_build_dir()
    os.makedirs(build_dir, exist_ok=True)
    header_file = os.path.join(build_dir, "version.h")
    pr_number = None

    if git_is_repository():
        (branch_name, commit, commit_short) = git_get_head_info()

        if is_pull_request():
            pr_number = get_pr_number()
    else:
        branch_name = "tarball"
        commit = ":(-dont-do-this"
        commit_short = ":("

    # header
    contents_new = f"""// Autogenerated by xenia-build.py.
#ifndef GENERATED_VERSION_H_
#define GENERATED_VERSION_H_
#define XE_BUILD_BRANCH "{branch_name}"
#define XE_BUILD_COMMIT "{commit}"
#define XE_BUILD_COMMIT_SHORT "{commit_short}"
#define XE_BUILD_DATE __DATE__
"""

    # PR info (if available)
    if pr_number:
      contents_new += f"""#define XE_BUILD_IS_PR
#define XE_BUILD_PR_NUMBER "{pr_number}"
"""

    # footer
    contents_new += """#endif  // GENERATED_VERSION_H_
"""

    contents_old = None
    if os.path.exists(header_file) and os.path.getsize(header_file) < 1024:
        with open(header_file, "r") as f:
            contents_old = f.read()

    if contents_old != contents_new:
        with open(header_file, "w") as f:
            f.write(contents_new)


def generate_source_class(path):
    header_path = f"{path}.h"
    source_path = f"{path}.cc"

    if os.path.isfile(header_path) or os.path.isfile(source_path):
        print_error("Target file already exists")
        return 1

    if generate_source_file(header_path) > 0:
        return 1
    if generate_source_file(source_path) > 0:
        # remove header if source file generation failed
        os.remove(os.path.join(source_root, header_path))
        return 1

    return 0

def generate_source_file(path):
    """Generates a source file at the specified path containing copyright notice
    """
    copyright = f"""/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright {datetime.now().year} Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */"""

    if os.path.isfile(path):
        print_error("Target file already exists")
        return 1
    try:
        with open(path, "w") as f:
            f.write(copyright)
    except Exception as e:
        print_error(f"Could not write to file [path {path}]")
        return 1

    return 0



def git_get_head_info():
    """Queries the current branch and commit checksum from git.

    Returns:
      (branch_name, commit, commit_short)
      If the user is not on any branch the name will be 'detached'.
    """
    p = subprocess.Popen([
        "git",
        "symbolic-ref",
        "--short",
        "-q",
        "HEAD",
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    branch_name = stdout.decode("ascii").strip() or "detached"
    p = subprocess.Popen([
        "git",
        "rev-parse",
        "HEAD",
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    commit = stdout.decode("ascii").strip() or "unknown"
    p = subprocess.Popen([
        "git",
        "rev-parse",
        "--short",
        "HEAD",
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    commit_short = stdout.decode("ascii").strip() or "unknown"
    return branch_name, commit, commit_short


def git_is_repository():
    """Checks if git is available and this source tree is versioned.
    """
    if not has_bin("git"):
        return False
    return shell_call([
        "git",
        "rev-parse",
        "--is-inside-work-tree",
        ], throw_on_error=False, stdout_path=os.devnull, stderr_path=os.devnull) == 0

def is_pull_request():
    """Returns true if actions is building a pull request, otherwise false.
    """
    return os.getenv('GITHUB_EVENT_NAME') == 'pull_request'

def get_pr_number():
    """
    Returns the pull request number if the workflow is triggered by a PR, otherwise None.
    """
    github_ref = os.getenv('GITHUB_REF')

    if github_ref and github_ref.startswith('refs/pull/'):
        return github_ref.split('/')[2]

def git_submodule_update():
    """Runs a git submodule sync, init, and update.
    """
    if sys.platform == "linux":
        submodules_ignore = ["DirectX-Headers", "DirectXShaderCompiler"]
    else:
        submodules_ignore = None
    if submodules_ignore:
        with open(".gitmodules") as f:
            gitmodules = f.read()
        submodules = re_findall(r"(?<=path = )(?!third_party\/(?:" + "|".join(submodules_ignore) + r")).+", gitmodules)
    else:
        submodules = None
    # Sync submodule URLs from .gitmodules to local config
    shell_call([
        "git",
        "submodule",
        "sync",
        *(submodules or []),
        ])
    # Then update all submodules to their recorded commits
    shell_call([
        "git",
        "-c",
        "fetch.recurseSubmodules=on-demand",
        "submodule",
        "update",
        "--init",
        "--depth=1",
        "-j", f"{os.cpu_count()}",
        *(submodules or []),
        ])


def get_cc(cc=None):
    if sys.platform == "linux":
        if os.environ.get("CC"):
            if "gcc" in os.environ.get("CC"):
                return "gcc"
        return "clang"
    if sys.platform == "win32":
        return "msc"

def get_clang_format_binary():
    """Finds a clang-format binary. Aborts if none is found.

    Returns:
      A path to the clang-format executable.
    """
    clang_format_version_min = 19

    # Build list of all potential clang-format binaries
    all_binaries = []

    # Check versioned binaries from 21 down to min, preferring newer
    for version in range(21, clang_format_version_min - 1, -1):
        binary = f"clang-format-{version}"
        if has_bin(binary):
            all_binaries.append(binary)

    # Also check generic clang-format
    all_binaries.append("clang-format")

    # Add Windows-specific paths
    if sys.platform == "win32":
        if "VCINSTALLDIR" in os.environ:
            if is_amd64():
                all_binaries.append(os.path.join(os.environ["VCINSTALLDIR"], "Tools", "Llvm", "x64", "bin", "clang-format.exe"))
            elif is_arm():
                all_binaries.append(os.path.join(os.environ["VCINSTALLDIR"], "Tools", "Llvm", "arm64", "bin", "clang-format.exe"))

        all_binaries.append(os.path.join(os.environ["ProgramFiles"], "LLVM", "bin", "clang-format.exe"))

    # Find the highest version available
    best_binary = None
    best_version = 0

    for binary in all_binaries:
        if has_bin(binary):
            try:
                clang_format_out = subprocess.check_output([binary, "--version"], text=True)
                version = int(clang_format_out.split("version ")[1].split(".")[0])
                if version >= clang_format_version_min and version > best_version:
                    best_version = version
                    best_binary = binary
                    best_output = clang_format_out
            except:
                continue

    if best_binary:
        print(best_output)
        return best_binary

    print_error(f"clang-format {clang_format_version_min} or newer is not on PATH")
    sys.exit(1)


def normalize_target_arch(value):
    """Normalizes --target-arch values to canonical names (arm64, x64, or None)."""
    v = value.lower()
    if v in ("arm64", "aarch64", "a64"):
        return "arm64"
    if v in ("x64", "x86_64", "amd64", "x86"):
        return "x64"
    raise ArgumentTypeError(
        f"unknown architecture '{value}' (expected: arm64, aarch64, a64, x64, amd64, x86_64, x86)")

def is_amd64():
    return normalize_target_arch(platform.machine()) in ("x64", "x86_64", "amd64", "x86")

def is_arm():
    return normalize_target_arch(platform.machine()) == "arm64"

def get_build_dir(target_arch=None, os_name=None):
    """Returns the CMake binary directory for the given target architecture."""
    return xp.get_build_dir(os_name=os_name, target_arch=target_arch)


def iter_cmake_build_dirs():
    """Yield directories that contain a CMake cache (Build/ tree + legacy)."""
    seen = set()
    for legacy in xp.legacy_build_dirs():
        cache = os.path.join(legacy, "CMakeCache.txt")
        if os.path.isfile(cache):
            norm = os.path.normpath(legacy)
            if norm not in seen:
                seen.add(norm)
                yield legacy
    if not os.path.isdir(xp.BUILD_ROOT):
        return
    skip = frozenset({
        "Release", "Debug", "Checked", "obj", "bin", "CMakeFiles",
        "src", "third_party", "tools", "gputest",
    })
    for root, dirs, files in os.walk(xp.BUILD_ROOT):
        if "CMakeCache.txt" in files:
            norm = os.path.normpath(root)
            if norm not in seen:
                seen.add(norm)
                yield root
        dirs[:] = [d for d in dirs if d not in skip]


def run_cmake_configure_android(build_type="Release", extra_args=None):
    """Configures an Android NDK (arm64-v8a) build into Build/Android.

    Requires ANDROID_NDK_ROOT (or ANDROID_NDK_HOME) pointing at an NDK (r26c).
    Uses single-config Ninja so artifacts land at Build/Android/<Config>.
    """
    ndk = os.environ.get("ANDROID_NDK_ROOT") or os.environ.get("ANDROID_NDK_HOME")
    if not ndk or not os.path.isdir(ndk):
        print_error(
            "ANDROID_NDK_ROOT is not set or does not exist.\n"
            "  Install Android NDK r26c and set ANDROID_NDK_ROOT to its root.")
        return 1
    toolchain = os.path.join(ndk, "build", "cmake", "android.toolchain.cmake")
    if not os.path.isfile(toolchain):
        print_error(f"NDK toolchain not found at {toolchain}.")
        return 1

    build_dir = get_build_dir(os_name="Android")
    args = [
        "cmake",
        "-S", ".",
        "-B", build_dir,
        "-G", "Ninja",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain.replace(os.sep, '/')}",
        "-DANDROID_ABI=arm64-v8a",
        "-DANDROID_PLATFORM=android-26",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DXENIA_BUILD_TESTS=OFF",
        "-DXENIA_BUILD_MISC=OFF",
    ]
    if has_bin("sccache"):
        args += [
            "-DCMAKE_C_COMPILER_LAUNCHER=sccache",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=sccache",
        ]
    if extra_args:
        args += extra_args

    ret = subprocess.call(args)
    if ret == 0:
        generate_version_h(build_dir)
    return ret


def run_cmake_configure(build_type="Release", cc=None, build_tests=False,
                        extra_args=None, target_arch=None,
                        enable_itrace=False, enable_dtrace=False,
                        enable_ftrace=False, target_os=None):
    """Runs cmake configure on the project.

    Args:
      build_type: Build configuration (Debug, Release, Checked).
      cc: C compiler to use (e.g. 'clang', 'gcc').
      build_tests: If True, enables building test suites.
      extra_args: Additional arguments to pass to cmake (e.g. -D flags).
      target_arch: Target architecture override (e.g. 'arm64' for cross-compile).
      target_os: Target OS override ('android') for cross-OS builds.

    Returns:
      Return code from cmake.
    """
    if target_os == "android":
        return run_cmake_configure_android(build_type=build_type, extra_args=extra_args)

    # Cross-compilation via --target-arch is only supported on Windows where
    # we can locate the MSVC cross-compiler automatically.  On Linux it would
    # silently produce a native build in a differently-named directory.
    if target_arch is not None and sys.platform != "win32":
        is_native_arm64 = platform.machine() in ("ARM64", "aarch64")
        native_arch = "arm64" if is_native_arm64 else "x64"
        if target_arch != native_arch:
            print_error(
                f"Cross-compilation (--target-arch {target_arch}) is only "
                f"supported on Windows.\n"
                f"  The current host architecture is {native_arch}.")
            return 1

    build_dir = get_build_dir(target_arch)
    args = [
        "cmake",
        "-S", ".",
        "-B", build_dir,
        "-G", "Ninja Multi-Config",
    ]
    if sys.platform != "win32":
        if not cc:
            cc = get_cc(cc=cc)
        c_compiler = cc or os.environ.get("CC", "clang")
        cxx_compiler = (cc + "++") if cc else os.environ.get("CXX", "clang++")
        args += [
            f"-DCMAKE_C_COMPILER={c_compiler}",
            f"-DCMAKE_CXX_COMPILER={cxx_compiler}",
        ]
    elif platform.machine() in ("ARM64", "aarch64") or target_arch == "arm64":
        # Determine the effective target and the appropriate compiler/environment.
        is_native_arm64 = platform.machine() in ("ARM64", "aarch64")
        if target_arch == "x64" and is_native_arm64:
            # Cross-compiling from ARM64 to x64
            target = "x64"
            vcvars_arg = "arm64_amd64"
            processor = "AMD64"
            cl_glob = r"C:\Program Files\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\HostARM64\x64\cl.exe"
        else:
            # Targeting ARM64 (native or cross-compile from x64)
            target = "arm64"
            vcvars_arg = "x64_arm64"
            processor = "ARM64"
            cl_glob = r"C:\Program Files\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\arm64\cl.exe"

        cl_paths = sorted(glob(cl_glob))
        if cl_paths:
            cl_exe = cl_paths[-1]
            # Derive the VS install root from the compiler path:
            # .../VC/Tools/MSVC/<ver>/bin/Host<x>/target<y>/cl.exe -> .../VC
            vc_root = cl_exe
            for _ in range(7):  # walk up 7 levels to VC/
                vc_root = os.path.dirname(vc_root)
            vcvarsall = os.path.join(vc_root, "Auxiliary", "Build", "vcvarsall.bat")
            if os.path.exists(vcvarsall):
                print(f"  Setting up {target.upper()} build environment via: {vcvarsall}")
                cmd = f'"{vcvarsall}" {vcvars_arg} >nul 2>&1 && set'
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
                if result.returncode == 0:
                    for line in result.stdout.splitlines():
                        if "=" in line:
                            key, _, value = line.partition("=")
                            os.environ[key] = value
            args += [
                "-DCMAKE_SYSTEM_NAME=Windows",
                f"-DCMAKE_SYSTEM_PROCESSOR={processor}",
                f"-DCMAKE_C_COMPILER={cl_exe.replace(os.sep, '/')}",
                f"-DCMAKE_CXX_COMPILER={cl_exe.replace(os.sep, '/')}",
            ]
        else:
            print_error(
                f"{target.upper()} cross-compiler not found.\n"
                f"  Install 'MSVC {target.upper()} build tools' via Visual Studio Installer\n"
                f"  (component: Microsoft.VisualStudio.Component.VC.Tools.ARM64).")
            return 1
    if build_tests:
        args += ["-DXENIA_BUILD_TESTS=ON"]
    if has_bin("sccache"):
        args += [
            "-DCMAKE_C_COMPILER_LAUNCHER=sccache",
            "-DCMAKE_CXX_COMPILER_LAUNCHER=sccache",
        ]
    args += [f"-DXENIA_ENABLE_ITRACE={'ON' if enable_itrace else 'OFF'}"]
    args += [f"-DXENIA_ENABLE_DTRACE={'ON' if enable_dtrace else 'OFF'}"]
    args += [f"-DXENIA_ENABLE_FTRACE={'ON' if enable_ftrace else 'OFF'}"]
    if extra_args:
        args += extra_args

    ret = subprocess.call(args)

    if ret == 0:
        generate_version_h(build_dir)

    return ret


def get_build_bin_path(args):
    """Returns the absolute path to built binaries for the parsed arguments."""
    config = args["config"].title()
    rel = xp.get_bin_dir(target_arch=args.get("target_arch"), config=config)
    return os.path.join(self_path, rel)


def create_clion_workspace():
    """Creates some basic workspace information inside the .idea directory for first start.
    """
    if os.path.exists(".idea"):
        # No first start
        return False
    print("Generating CLion workspace files...")
    # Might become easier in the future: https://youtrack.jetbrains.com/issue/CPP-7911

    # Set the location of the CMakeLists.txt
    os.mkdir(".idea")
    with open(os.path.join(".idea", "misc.xml"), "w") as f:
        f.write("""<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CMakePythonSetting">
    <option name="pythonIntegrationState" value="YES" />
  </component>
  <component name="CMakeWorkspace" PROJECT_DIR="$PROJECT_DIR$" />
</project>
""")

    # Create workspace.xml file
    with open(os.path.join(".idea", "workspace.xml"), "w") as f:
        # Write CmakePresetLoader
        f.write("""<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CMakePresetLoader"><![CDATA[{
  "useNewFormat": true
}]]></component>
""")
        f.write("""  <component name="CMakeReloadState">
    <option name="reloaded" value="true" />
  </component>
""")

        # Write ConfigurationManager
        f.write("""  <component name="CMakeRunConfigurationManager">\n""")
        f.write("    <generated>\n")
        # Loop over every entry
        for target in XENIA_TARGETS:
            f.write(f'      <config projectName="xenia" targetName="{target}" />\n')
        f.write("    </generated>\n")
        f.write("  </component>\n")

        # Write CMakeSettings
        f.write("""  <component name="CMakeSettings">\n""")
        f.write("    <configurations>\n")
        f.write("""      <configuration PROFILE_NAME="default - debug" ENABLED="true" FROM_PRESET="true" GENERATION_DIR="$PROJECT_DIR$/Build/Windows/x64" />\n""")
        f.write("""      <configuration PROFILE_NAME="default - release" ENABLED="true" FROM_PRESET="true" GENERATION_DIR="$PROJECT_DIR$/Build/Windows/x64" />\n""")
        f.write("""      <configuration PROFILE_NAME="default - checked" ENABLED="true" FROM_PRESET="true" GENERATION_DIR="$PROJECT_DIR$/Build/Windows/x64" />\n""")
        f.write("    </configurations>\n")
        f.write("  </component>\n")

        # Write RunManager
        # Write basic xenia-app manually
        f.write("""  <component name="RunManager" selected="CMake Application.xenia-app">\n""")
        f.write("""    <configuration default="true" type="CLionExternalRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true">\n""")
        f.write("""      <method v="2">\n""")
        f.write("""        <option name="CLION.EXTERNAL.BUILD" enabled="true" />\n""")
        f.write("""      </method>\n""")
        f.write("""    </configuration>\n""")

        for target in XENIA_TARGETS:
            if target != "xenia-app":
                f.write(f'    <configuration name="{target}" type="CMakeRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true" PROJECT_NAME="xenia" TARGET_NAME="{target}" CONFIG_NAME="default - debug">\n')
            else:
                f.write(f'    <configuration name="{target}" type="CMakeRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true" PROJECT_NAME="xenia" TARGET_NAME="{target}" CONFIG_NAME="default - debug" RUN_TARGET_PROJECT_NAME="xenia" RUN_TARGET_NAME="{target}">\n')

            f.write("""      <method v="2">\n""")
            f.write("""        <option name="com.jetbrains.cidr.execution.CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask" enabled="true" />\n""")
            f.write("""      </method>\n""")
            f.write("""    </configuration>\n""")

        # Write itemvalue list
        f.write("""    <list>\n""")
        for target in XENIA_TARGETS:
            f.write(f'      <item itemvalue="CMake Application.{target}" />\n')

        f.write("""    </list>\n""")
        f.write("""  </component>\n""")
        f.write("""</project>\n""")

    os.makedirs(os.path.join(".idea", "codeStyles"), exist_ok=True)
    with open(os.path.join(".idea", "codeStyles", "Project.xml"), "w") as f:
        f.write("""<component name="ProjectCodeStyleConfiguration">
  <code_scheme name="Project">
    <RiderCodeStyleSettings>
      <option name="/Default/CodeStyle/CodeFormatting/CppClangFormat/EnableClangFormatSupport/@EntryValue" value="true" type="bool" />
    </RiderCodeStyleSettings>
    <clangFormatSettings>
      <option name="ENABLED" value="true" />
    </clangFormatSettings>
  </code_scheme>
</component>""")
    return True


def _doctor_check_clang():
    """Returns (ok, message)."""
    if sys.platform == "win32":
        return True, "Windows build uses MSVC (not checked here)."
    cc = os.environ.get("CC", "clang")
    if not has_bin(cc):
        return False, f"{cc} not found on PATH."
    try:
        out = subprocess.check_output([cc, "--version"], text=True, stderr=subprocess.STDOUT)
        version_line = out.splitlines()[0] if out else ""
        major = 0
        if "version " in out:
            ver = out.split("version ")[1].split(".")[0]
            major = int(ver.split()[0]) if ver.split()[0].isdigit() else 0
        if major and major < 19:
            return False, f"{cc} reports version {major}; Clang 19+ required ({version_line})."
        return True, version_line or f"{cc} found."
    except Exception as exc:
        return False, f"Failed to run {cc}: {exc}"


def _doctor_check_bin(name, apt_hint=None):
    if has_bin(name):
        return True, f"{name} found."
    msg = f"{name} not found on PATH."
    if apt_hint:
        msg += f" Try: sudo apt-get install {apt_hint}"
    return False, msg


def _doctor_check_pkg_config(module, apt_hint=None):
    if sys.platform == "win32":
        return True, "skipped on Windows"
    if not has_bin("pkg-config"):
        return False, "pkg-config not found."
    ret = subprocess.run(
        ["pkg-config", "--exists", module],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if ret.returncode == 0:
        return True, f"pkg-config {module} OK."
    msg = f"pkg-config module '{module}' missing."
    if apt_hint:
        msg += f" Try: sudo apt-get install {apt_hint}"
    return False, msg


def _doctor_check_spirv_opt():
    spirv_opt = get_bin("spirv-opt")
    if not spirv_opt:
        return False, (
            "spirv-opt not found. Install spirv-tools or LunarG Vulkan SDK "
            "(see docs/building.md)."
        )
    try:
        out = subprocess.check_output([spirv_opt, "--version"], text=True, stderr=subprocess.STDOUT)
    except Exception as exc:
        return False, f"spirv-opt failed: {exc}"
    supports_canonicalize = False
    try:
        result = subprocess.run(
            [spirv_opt, "--canonicalize-ids"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        stderr = result.stderr or ""
        supports_canonicalize = "unknown" not in stderr.lower() and "unrecognized" not in stderr.lower()
    except Exception:
        pass
    vulkan_sdk = os.environ.get("VULKAN_SDK", "")
    if supports_canonicalize:
        return True, f"{out.strip()} (supports --canonicalize-ids). VULKAN_SDK={vulkan_sdk or '(unset)'}"
    return False, (
        f"{out.strip()} — missing --canonicalize-ids support. "
        "Install LunarG Vulkan SDK and set VULKAN_SDK (build may still work with a fallback)."
    )


class Command(object):
    """Base type for commands.
    """

    def __init__(self, subparsers, name, help_short=None, help_long=None,
                 *args, **kwargs):
        """Initializes a command.

        Args:
          subparsers: Argument subparsers parent used to add command parsers.
          name: The name of the command exposed to the management script.
          help_short: Help text printed alongside the command when queried.
          help_long: Extended help text when viewing command help.
        """
        self.name = name
        self.help_short = help_short
        self.help_long = help_long

        self.parser = subparsers.add_parser(name,
                                            help=help_short,
                                            description=help_long)
        self.parser.set_defaults(command_handler=self)

    def execute(self, args, pass_args, cwd):
        """Executes the command.

        Args:
          args: Arguments hash for the command.
          pass_args: Arguments list to pass to child commands.
          cwd: Current working directory.

        Returns:
          Return code of the command.
        """
        return 1


def _doctor_check_vs_windows():
    if not vs_version:
        return False, "Visual Studio 2022 not found (install the Native Desktop C++ workload)."
    return True, f"Visual Studio {vs_version} detected."


def _doctor_check_clang_format():
    candidates = ["clang-format"]
    for v in range(21, 18, -1):
        candidates.append(f"clang-format-{v}")
    if sys.platform == "win32" and "VCINSTALLDIR" in os.environ:
        candidates.append(os.path.join(
            os.environ["VCINSTALLDIR"], "Tools", "Llvm", "x64", "bin", "clang-format.exe"))
    for c in candidates:
        if has_bin(c) or os.path.isfile(c):
            return True, f"{c} found."
    return False, "clang-format not found (needed for `xb lint`/`format`)."


def _doctor_check_docker():
    if not has_bin("docker"):
        return False, "not found (needed for Linux matrix cells via xb matrix)."
    try:
        r = subprocess.run(["docker", "info"], capture_output=True, text=True)
        if r.returncode == 0:
            return True, "daemon reachable."
        return False, "installed but daemon not running."
    except Exception as exc:
        return False, f"check failed: {exc}"


def _doctor_check_gh():
    if not has_bin("gh"):
        return False, "not found (needed for `xb ci`; https://cli.github.com/)."
    try:
        r = subprocess.run(["gh", "auth", "status"], capture_output=True, text=True)
        if r.returncode == 0:
            return True, "authenticated."
        return False, "present but not authenticated (run: gh auth login)."
    except Exception as exc:
        return False, f"check failed: {exc}"


def _doctor_check_msvc_arm64():
    cl = sorted(glob(
        r"C:\Program Files\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\arm64\cl.exe"))
    if cl:
        return True, "cross tools present."
    return False, "not installed (needed for the Windows ARM64 cell)."


def _doctor_check_ndk():
    ndk = os.environ.get("ANDROID_NDK_ROOT") or os.environ.get("ANDROID_NDK_HOME")
    if ndk and os.path.isdir(ndk):
        return True, f"{ndk}"
    return False, "ANDROID_NDK_ROOT not set (needed for the Android cell)."


class DoctorCommand(Command):
    """'doctor' command — verify build dependencies on all platforms."""

    def __init__(self, subparsers, *args, **kwargs):
        super(DoctorCommand, self).__init__(
            subparsers,
            name="doctor",
            help_short="Check build dependencies (especially on Linux).",
            help_long="Verifies toolchain, pkg-config modules, and Vulkan shader tools.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print("Xenia build environment check\n")
        checks = []
        if sys.platform == "win32":
            checks.append(("Visual Studio 2022", _doctor_check_vs_windows()))
            checks.append(("cmake", _doctor_check_bin("cmake", "cmake")))
            checks.append(("ninja", _doctor_check_bin("ninja", "ninja-build")))
            ok_vk = setup_vulkan_sdk()
            checks.append(("Vulkan SDK", (ok_vk, "configured" if ok_vk else "install from https://vulkan.lunarg.com/")))
            checks.append(("clang-format", _doctor_check_clang_format()))
        else:
            checks.append(("clang", _doctor_check_clang()))
            checks.append(("lld", _doctor_check_bin("ld.lld", "lld-20")))
            checks.append(("cmake", _doctor_check_bin("cmake", "cmake")))
            checks.append(("ninja", _doctor_check_bin("ninja", "ninja-build")))
            checks.append(("python3", _doctor_check_bin("python3", "python3")))
            checks.append(("pkg-config", _doctor_check_bin("pkg-config", "pkg-config")))
            checks.append(("gtk+-x11-3.0", _doctor_check_pkg_config("gtk+-x11-3.0", "libgtk-3-dev")))
            checks.append(("sdl2", _doctor_check_pkg_config("sdl2", "libsdl2-dev")))
            checks.append(("xcb", _doctor_check_pkg_config("xcb", "libx11-xcb-dev")))
            checks.append(("x11-xcb", _doctor_check_pkg_config("x11-xcb", "libx11-xcb-dev")))
            setup_vulkan_sdk()
            checks.append(("spirv-opt / VULKAN_SDK", _doctor_check_spirv_opt()))
            checks.append(("clang-format", _doctor_check_clang_format()))

        failed = 0
        for name, (ok, detail) in checks:
            status = "OK" if ok else "FAIL"
            color = bcolors.OKCYAN if ok else bcolors.FAIL
            print(f"  [{color}{status}{bcolors.ENDC}] {name}: {detail}")
            if not ok:
                failed += 1

        # Optional matrix / CI prerequisites — warn-only, never affect exit code.
        print("\nMatrix / CI prerequisites (optional):")
        warn_checks = [
            ("docker", _doctor_check_docker()),
            ("gh CLI", _doctor_check_gh()),
        ]
        if sys.platform == "win32":
            warn_checks.append(("MSVC ARM64 tools", _doctor_check_msvc_arm64()))
        warn_checks.append(("Android NDK", _doctor_check_ndk()))
        for name, (ok, detail) in warn_checks:
            status = "OK" if ok else "warn"
            color = bcolors.OKCYAN if ok else bcolors.WARNING
            print(f"  [{color}{status}{bcolors.ENDC}] {name}: {detail}")

        if sys.platform != "win32" and failed:
            print(
                f"\n{bcolors.WARNING}Suggested Ubuntu 24.04 packages (match CI):{bcolors.ENDC}\n"
                "  sudo apt-get install build-essential cmake ninja-build python3 pkg-config git \\\n"
                "    mesa-vulkan-drivers libc++-dev libc++abi-dev libgtk-3-dev liblz4-dev \\\n"
                "    libsdl2-dev libvulkan-dev libx11-xcb-dev libasound2-dev libfontconfig1-dev \\\n"
                "    libfuse2 spirv-tools clang-20 lld-20 llvm-20\n"
                "  export CC=clang-20 CXX=clang++-20\n"
            )

        print_status(ResultStatus.SUCCESS if failed == 0 else ResultStatus.FAILURE)
        return 1 if failed else 0


class SetupCommand(Command):
    """'setup' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(SetupCommand, self).__init__(
            subparsers,
            name="setup",
            help_short="Setup the build environment.",
            *args, **kwargs)
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")

    def execute(self, args, pass_args, cwd):
        print("Setting up the build environment...\n")

        # Setup submodules.
        print("- git submodule init / update...")
        if git_is_repository():
            git_submodule_update()
        else:
            print_warning("Git not available or not a repository. Dependencies may be missing.")

        print("\n- running cmake configure...")
        ret = run_cmake_configure(target_arch=args["target_arch"])
        print_status(ResultStatus.SUCCESS if not ret else ResultStatus.FAILURE)
        return ret


class PullCommand(Command):
    """'pull' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(PullCommand, self).__init__(
            subparsers,
            name="pull",
            help_short="Pulls the repo and all dependencies and rebases changes.",
            *args, **kwargs)
        self.parser.add_argument(
            "--merge", action="store_true",
             help=f"Merges on {default_branch} instead of rebasing.")

    def execute(self, args, pass_args, cwd):
        print("Pulling...\n")

        print(f"- switching to {default_branch}...")
        shell_call([
            "git",
            "checkout",
            default_branch,
            ])

        print("\n- pulling self...")
        if args["merge"]:
            shell_call([
                "git",
                "pull",
                ])
        else:
            shell_call([
                "git",
                "pull",
                "--rebase",
                ])

        print("\n- pulling dependencies...")
        git_submodule_update()

        print("\n- running cmake configure...")
        if run_cmake_configure() == 0:
            print_status(ResultStatus.SUCCESS)

        return 0


class PremakeCommand(Command):
    """'premake' command (now runs cmake configure).
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(PremakeCommand, self).__init__(
            subparsers,
            name="premake",
            help_short="Runs cmake configure to update all projects.",
            *args, **kwargs)
        self.parser.add_argument(
            "--cc", choices=["clang", "gcc", "msc"], default=None, help="Compiler toolchain")
        self.parser.add_argument(
            "--build-tests", action="store_true", default=False,
            help="Enables building test suites.")
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")

    def execute(self, args, pass_args, cwd):
        print("Running cmake configure...\n")
        ret = run_cmake_configure(cc=args["cc"],
                                  build_tests=args["build_tests"],
                                  target_arch=args["target_arch"])
        print_status(ResultStatus.SUCCESS if not ret else ResultStatus.FAILURE)

        return ret


# Friendly target aliases for `xb build <alias>`.
TARGET_ALIASES = {
    "app": ["xenia-app"],
    "tests": ["xenia-cpu-tests", "xenia-base-tests", "xenia-kernel-tests"],
    "fuzz": ["vmx128-fuzz"],
    "trace": ["xenia-gpu-d3d12-trace-dump"],
    "xma2": ["xma2-diff"],
}
_TEST_TARGETS = {"xenia-cpu-tests", "xenia-base-tests", "xenia-kernel-tests"}
_MISC_TARGETS = {
    "vmx128-fuzz", "xma2-diff",
    "xenia-gpu-d3d12-trace-dump", "xenia-gpu-vulkan-trace-dump",
}


def expand_target_aliases(targets):
    """Expands friendly aliases (app/tests/fuzz/trace/xma2) to real targets."""
    expanded = []
    for t in targets:
        expanded.extend(TARGET_ALIASES.get(t, [t]))
    return expanded


class BaseBuildCommand(Command):
    """Base command for things that require building.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(BaseBuildCommand, self).__init__(
            subparsers,
            *args, **kwargs)
        self.parser.add_argument(
            "--cc", choices=["clang", "gcc", "msc"], default=None, help="Compiler toolchain")
        self.parser.add_argument(
            "--config", choices=["checked", "debug", "release"], default="release",
            type=str.lower, help="Chooses the build configuration.")
        self.parser.add_argument(
            "--target", action="append", default=[],
            help="Builds only the given target(s). Aliases: app, tests, fuzz, "
                 "trace, xma2 (auto-enable tests/misc as needed).")
        self.parser.add_argument(
            "--force", action="store_true",
            help="Forces a full rebuild.")
        self.parser.add_argument(
            "--no_premake", action="store_true",
            help="Skips running cmake configure before building.")
        self.parser.add_argument(
            "--build-tests", action="store_true", default=False,
            help="Enables building test suites.")
        self.parser.add_argument(
            "--cmake-define", dest="cmake_defines", action="append",
            default=[], metavar="KEY=VALUE",
            help="Pass a CMake define (e.g. --cmake-define CMAKE_CXX_FLAGS=/DUSE_BCRYPT_RSA).")
        self.parser.add_argument(
            "--enable-itrace", dest="enable_itrace", action="store_true",
            default=False,
            help="Enables JIT per-instruction tracing to the log (sets "
                 "-DXENIA_ENABLE_ITRACE=ON). Very slow; for debugging only.")
        self.parser.add_argument(
            "--enable-dtrace", dest="enable_dtrace", action="store_true",
            default=False,
            help="Enables JIT per-operation data tracing to the log (sets "
                 "-DXENIA_ENABLE_DTRACE=ON). Very slow; for debugging only.")
        self.parser.add_argument(
            "--enable-ftrace", dest="enable_ftrace", action="store_true",
            default=False,
            help="Enables JIT per-function-call tracing to the log (sets "
                 "-DXENIA_ENABLE_FTRACE=ON). For debugging only.")
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")
        self.parser.add_argument(
            "--target-os", choices=["android"], default=None,
            help="Cross-build for another OS (currently: android, into Build/Android).")

    def execute(self, args, pass_args, cwd):
        config = args["config"].title()
        target_os = args.get("target_os")
        os_name = "Android" if target_os == "android" else None

        targets = expand_target_aliases(args["target"])

        extra_args = [f"-D{d}" for d in args["cmake_defines"]]
        # Auto-enable the cmake options implied by the requested targets.
        # (Android forces TESTS/MISC off, so skip inference there.)
        build_tests = args["build_tests"] or (
            target_os is None and any(t in _TEST_TARGETS for t in targets))
        if target_os is None and any(t in _MISC_TARGETS for t in targets):
            extra_args.append("-DXENIA_BUILD_MISC=ON")

        if not args["no_premake"]:
            print("- running cmake configure...")
            ret = run_cmake_configure(build_type=config, cc=args["cc"],
                                      build_tests=build_tests,
                                      target_arch=args["target_arch"],
                                      extra_args=extra_args,
                                      enable_itrace=args["enable_itrace"],
                                      enable_dtrace=args["enable_dtrace"],
                                      enable_ftrace=args["enable_ftrace"],
                                      target_os=target_os)
            if ret:
                return ret
            print("")

        build_dir = get_build_dir(args.get("target_arch"), os_name=os_name)
        print("- building (%s):%s..." % (
            "all" if not len(targets) else ", ".join(targets),
            args["config"]))
        build_args = [
            "cmake",
            "--build", build_dir,
            "--config", config,
        ]
        for target in targets:
            build_args += ["--target", target]
        if args["force"]:
            build_args += ["--clean-first"]
        result = subprocess.call(build_args + pass_args)
        if result != 0:
            print_error("Build failed with one or more errors.")
        return result


class BuildCommand(BaseBuildCommand):
    """'build' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(BuildCommand, self).__init__(
            subparsers,
            name="build",
            help_short="Builds the project with the default toolchain.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print(f"Building {args['config']}...\n")

        result = super(BuildCommand, self).execute(args, pass_args, cwd)

        print_status(ResultStatus.SUCCESS if not result else ResultStatus.FAILURE)
        if not result:
            print(f"Output: {get_build_bin_path(args)}")

        return result


class BuildShadersCommand(Command):
    """'buildshaders' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(BuildShadersCommand, self).__init__(
            subparsers,
            name="buildshaders",
            help_short="Generates shader binaries for inclusion in C++ files.",
            help_long="""
            Generates the shader binaries under src/*/shaders/bytecode/.
            Run after modifying any .hs/vs/ds/gs/ps/cs.glsl/hlsl/xesl files.
            Direct3D shaders can be built only on a Windows host.
            """,
            *args, **kwargs)
        self.parser.add_argument(
            "--target", action="append", choices=["dxbc", "spirv"], default=[],
            help="Builds only the given target(s).")

    def execute(self, args, pass_args, cwd):
        return build_shaders(args["target"])


def build_shaders(targets=None):
    """Builds shader bytecode. Called by BuildShadersCommand.

    Delegates to the per-file compile scripts in tools/build/.

    Args:
        targets: List of targets ("dxbc", "spirv"), or None/empty for all.

    Returns:
        0 on success, non-zero on error.
    """
    # Check if shaders need rebuilding by comparing source vs generated timestamps
    gpu_shaders = "src/xenia/gpu/shaders"
    ui_shaders = "src/xenia/ui/shaders"
    # DXBC directories only on Windows, SPIR-V everywhere
    bytecode_dirs = [
        "src/xenia/gpu/shaders/bytecode/vulkan_spirv",
        "src/xenia/ui/shaders/bytecode/vulkan_spirv",
    ]
    if sys.platform == "win32":
        bytecode_dirs.extend([
            "src/xenia/gpu/shaders/bytecode/d3d12_5_1",
            "src/xenia/ui/shaders/bytecode/d3d12_5_1",
        ])

    newest_source = max(get_dir_newest_mtime(gpu_shaders),
                       get_dir_newest_mtime(ui_shaders))
    oldest_generated = min((get_dir_oldest_mtime(d) for d in bytecode_dirs),
                          default=0)

    # If oldest_generated is inf, bytecode doesn't exist - need to generate
    if oldest_generated != float('inf') and newest_source <= oldest_generated:
        print("Shaders are up-to-date, skipping generation.")
        return 0

    # Clean old bytecode before regenerating to remove stale files from deleted sources
    clean_shader_bytecode()

    src_paths = [os.path.join(root, name)
                 for root, dirs, files in os.walk("src")
                 for name in files
                 if (name.endswith(".glsl") or
                     name.endswith(".hlsl") or
                     name.endswith(".xesl"))]
    if targets is None:
        targets = []
    all_targets = len(targets) == 0

    valid_stages = ["vs", "hs", "ds", "gs", "ps", "cs"]
    compile_spirv = os.path.join(self_path, "tools", "build", "compile_shader_spirv.py")
    compile_dxbc = os.path.join(self_path, "tools", "build", "compile_shader_dxbc.py")

    # Direct3D DXBC (Windows only).
    if (all_targets or "dxbc" in targets) and sys.platform == "win32":
        print("Building Direct3D 12 Shader Model 5.1 DXBC shaders...")
        for src_path in src_paths:
            src_name = os.path.basename(src_path)
            if ((not src_name.endswith(".hlsl") and
                 not src_name.endswith(".xesl")) or
                len(src_name) <= 8 or src_name[-8] != "."):
                continue
            identifier = src_name[:-5].replace(".", "_")
            if identifier[-2:] not in valid_stages:
                continue
            src_dir = os.path.dirname(src_path)
            output = os.path.join(src_dir, "bytecode", "d3d12_5_1", f"{identifier}.h")
            print(f"- {src_path} > d3d12_5_1")
            result = subprocess.call([sys.executable, compile_dxbc, src_path, output])
            if result != 0:
                return result

    # Vulkan SPIR-V.
    if all_targets or "spirv" in targets:
        print("Building Vulkan SPIR-V shaders...")
        for src_path in src_paths:
            src_name = os.path.basename(src_path)
            if ((not src_name.endswith(".glsl") and
                 not src_name.endswith(".xesl")) or
                len(src_name) <= 8 or src_name[-8] != "."):
                continue
            identifier = src_name[:-5].replace(".", "_")
            if identifier[-2:] not in valid_stages:
                continue
            src_dir = os.path.dirname(src_path)
            output = os.path.join(src_dir, "bytecode", "vulkan_spirv", f"{identifier}.h")
            print(f"- {src_path} > vulkan_spirv")
            result = subprocess.call([sys.executable, compile_spirv, src_path, output])
            if result != 0:
                return result

    return 0


class TestCommand(BaseBuildCommand):
    """'test' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(TestCommand, self).__init__(
            subparsers,
            name="test",
            help_short="Runs automated tests that have been built with `xb build`.",
            help_long="""
            To pass arguments to the test executables separate them with `--`.
            For example, you can run only the instr_foo.s tests with:
              $ xb test -- instr_foo
            """,
            *args, **kwargs)
        self.parser.add_argument(
            "--no_build", action="store_true",
            help="Don't build before running tests.")
        self.parser.add_argument(
            "--continue", action="store_true",
            help="Don't stop when a test errors, but continue running all.")

    def execute(self, args, pass_args, cwd):
        print("Testing...\n")

        # The test executables that will be built and run.
        test_targets = args["target"] or [
            "xenia-base-tests",
            "xenia-cpu-tests",
            "xenia-kernel-tests",
            ]
        args["target"] = test_targets

        # Build all targets (if desired).
        if not args["no_build"]:
            result = super(TestCommand, self).execute(args, [], cwd)
            if result:
                print("Failed to build, aborting test run.")
                return result

        # Ensure all targets exist before we run.
        test_executables = [
            get_bin(os.path.join(get_build_bin_path(args), test_target))
            for test_target in test_targets]
        for i in range(0, len(test_targets)):
            if test_executables[i] is None:
                print_error(f"Unable to find {test_targets[i]} - build it.")
                return 1

        # Prepare environment with Qt bin directory in PATH if available
        test_env = dict(os.environ)
        qt_dir = os.environ.get("QT_DIR")
        if qt_dir and sys.platform == "win32":
            qt_bin = os.path.join(qt_dir, "bin")
            if os.path.exists(qt_bin):
                test_env["PATH"] = f"{qt_bin}{os.pathsep}{test_env['PATH']}"
                print(f"- Qt bin directory added to PATH: {qt_bin}\n")

        # Run tests.
        any_failed = False
        for test_executable in test_executables:
            print(f"- {test_executable}")
            result = subprocess.call([test_executable] + pass_args, env=test_env)
            if result:
                any_failed = True
                if args["continue"]:
                    print_error("test failed but continuing due to --continue.")
                else:
                    print_error("test failed, aborting, use --continue to keep going.")
                    return result

        if any_failed:
            print_error("one or more tests failed.")
            result = 1
        return result


class GenTestsCommand(Command):
    """'gentests' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(GenTestsCommand, self).__init__(
            subparsers,
            name="gentests",
            help_short="Generates test binaries.",
            help_long="""
            Generates test binaries (under src/xenia/cpu/ppc/testing/bin/).
            Run after modifying test .s files.
            """,
            *args, **kwargs)

    def process_src_file(test_bin, ppc_as, ppc_objdump, ppc_ld, ppc_nm, src_file):
        def make_unix_path(p):
            """Forces a unix path separator style, as required by binutils.
            """
            return p.replace(os.sep, "/")

        src_name = os.path.splitext(os.path.basename(src_file))[0]
        obj_file = f"{os.path.join(test_bin, src_name)}.o"
        shell_call([
            ppc_as,
            "-a32",
            "-be",
            "-mregnames",
            "-ma2",
            "-maltivec",
            "-mvsx",
            "-mvmx128",
            "-R",
            f"-o{make_unix_path(obj_file)}",
            make_unix_path(src_file),
            ])
        dis_file = f"{os.path.join(test_bin, src_name)}.dis"
        shell_call([
            ppc_objdump,
            "--adjust-vma=0x100000",
            "-Ma2",
            "-Mvmx128",
            "-D",
            "-EB",
            make_unix_path(obj_file),
            ], stdout_path=dis_file)
        # Eat the first 4 lines to kill the file path that'll differ across machines.
        with open(dis_file) as f:
            dis_file_lines = f.readlines()
        with open(dis_file, "w") as f:
            f.writelines(dis_file_lines[4:])
        shell_call([
            ppc_ld,
            "-A powerpc:common32",
            "-melf32ppc",
            "-EB",
            "-nostdlib",
            "--oformat=binary",
            "-Ttext=0x80000000",
            "-e0x80000000",
            f"-o{make_unix_path(os.path.join(test_bin, src_name))}.bin",
            make_unix_path(obj_file),
            ])
        shell_call([
            ppc_nm,
            "--numeric-sort",
            make_unix_path(obj_file),
            ], stdout_path=f"{os.path.join(test_bin, src_name)}.map")

        return src_file

    def execute(self, args, pass_args, cwd):
        print("Generating test binaries...\n")

        # Use the same binutils path on all platforms
        binutils_path = os.path.join("third_party", "binutils", "bin")

        ppc_as = os.path.join(binutils_path, "powerpc-none-elf-as")
        ppc_ld = os.path.join(binutils_path, "powerpc-none-elf-ld")
        ppc_objdump = os.path.join(binutils_path, "powerpc-none-elf-objdump")
        ppc_nm = os.path.join(binutils_path, "powerpc-none-elf-nm")

        # Check if binutils exists (with .exe on Windows)
        ppc_as_check = ppc_as + (".exe" if sys.platform == "win32" else "")
        if not os.path.exists(ppc_as_check):
            print("Binaries are missing, binutils build required\n")
            binutils_dir = os.path.join("third_party", "binutils")
            shell_script = "build.sh"

            # Save current directory
            original_dir = os.getcwd()

            if sys.platform == "linux":
                # Set executable bit for build script before running it
                os.chdir(binutils_dir)
                os.chmod(shell_script, stat.S_IRUSR | stat.S_IWUSR |
                         stat.S_IXUSR | stat.S_IRGRP | stat.S_IROTH)
                shell_call([f"./{shell_script}"])
                os.chdir(original_dir)
            elif sys.platform == "win32":
                # On Windows, add Cygwin to PATH and run bash
                cygwin_bin = r"C:\cygwin64\bin"
                os.environ["PATH"] = f"{cygwin_bin}{os.pathsep}{os.environ['PATH']}"
                os.chdir(binutils_dir)
                shell_call(["bash", shell_script])
                os.chdir(original_dir)

        test_src = os.path.join("src", "xenia", "cpu", "ppc", "testing")
        test_bin = os.path.join(test_src, "bin")

        # Ensure the test output path exists.
        if not os.path.exists(test_bin):
            os.mkdir(test_bin)

        src_files = [os.path.join(root, name)
                     for root, dirs, files in os.walk("src")
                     for name in files
                     if (name.startswith("instr_") or name.startswith("seq_"))
                     and name.endswith((".s"))]

        any_errors = False

        pool_func = partial(GenTestsCommand.process_src_file, test_bin, ppc_as, ppc_objdump, ppc_ld, ppc_nm)
        with Pool() as pool:
            for src_file in pool.imap_unordered(pool_func, src_files):
                print(f"- {src_file}")

        if any_errors:
            print_error("failed to build one or more tests.")
            return 1

        return 0


class GpuTestCommand(BaseBuildCommand):
    """'gputest' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(GpuTestCommand, self).__init__(
            subparsers,
            name="gputest",
            help_short="Runs automated GPU diff tests against reference imagery.",
            help_long="""
            To pass arguments to the test executables separate them with `--`.
            """,
            *args, **kwargs)
        self.parser.add_argument(
            "--no_build", action="store_true",
            help="Don't build before running tests.")
        self.parser.add_argument(
            "--update_reference_files", action="store_true",
            help="Update all reference imagery.")
        self.parser.add_argument(
            "--generate_missing_reference_files", action="store_true",
            help="Create reference files for new traces.")

    def execute(self, args, pass_args, cwd):
        print("Testing...\n")

        # The test executables that will be built and run.
        test_targets = args["target"] or [
            "xenia-gpu-vulkan-trace-dump",
            ]
        args["target"] = test_targets

        # Build all targets (if desired).
        if not args["no_build"]:
            result = super(GpuTestCommand, self).execute(args, [], cwd)
            if result:
                print("Failed to build, aborting test run.")
                return result

        # Ensure all targets exist before we run.
        test_executables = [
            get_bin(os.path.join(get_build_bin_path(args), test_target))
            for test_target in test_targets]
        for i in range(0, len(test_targets)):
            if test_executables[i] is None:
                print_error(f"Unable to find {test_targets[i]} - build it.")
                return 1

        output_path = os.path.join(
            self_path, get_build_dir(args.get("target_arch")), "gputest")
        if os.path.isdir(output_path):
            rmtree(output_path)
        os.makedirs(output_path)
        print(f"Running tests and outputting to {output_path}...")

        reference_trace_root = os.path.join(self_path, "testdata",
                                            "reference-gpu-traces")

        # Run tests.
        any_failed = False
        result = shell_call([
            sys.executable,
            os.path.join(self_path, "tools", "gpu-trace-diff.py"),
            f"--executable={test_executables[0]}",
            f"--trace_path={os.path.join(reference_trace_root, 'traces')}",
            f"--output_path={output_path}",
            f"--reference_path={os.path.join(reference_trace_root, 'references')}",
            ] + (["--generate_missing_reference_files"] if args["generate_missing_reference_files"] else []) +
                (["--update_reference_files"] if args["update_reference_files"] else []) +
                            pass_args,
                            throw_on_error=False)
        if result:
            any_failed = True

        if any_failed:
            print_error("one or more tests failed.")
            result = 1
        print(f"Check {output_path}/results.html for more details.")
        return result


class CleanCommand(Command):
    """'clean' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(CleanCommand, self).__init__(
            subparsers,
            name="clean",
            help_short="Removes intermediate files and build outputs.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print("Cleaning build artifacts...")
        for build_dir in iter_cmake_build_dirs():
            print(f"- cleaning {build_dir}...")
            subprocess.call(["cmake", "--build", build_dir, "--target", "clean"])

        clean_generated_files()

        print_status(ResultStatus.SUCCESS)
        return 0


def clean_shader_bytecode():
    """Removes generated shader bytecode files."""
    bytecode_dirs = [
        "src/xenia/gpu/shaders/bytecode/d3d12_5_1",
        "src/xenia/gpu/shaders/bytecode/vulkan_spirv",
        "src/xenia/ui/shaders/bytecode/d3d12_5_1",
        "src/xenia/ui/shaders/bytecode/vulkan_spirv",
    ]
    for bytecode_dir in bytecode_dirs:
        if os.path.isdir(bytecode_dir):
            print(f"- removing {bytecode_dir}/...")
            rmtree(bytecode_dir)


def clean_generated_files():
    """Removes generated shader bytecode files."""
    clean_shader_bytecode()


class NukeCommand(Command):
    """'nuke' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(NukeCommand, self).__init__(
            subparsers,
            name="nuke",
            help_short="Removes all Build/ output.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print("Cleaning build artifacts...\n")
        for path in xp.all_build_dirs():
            if os.path.isdir(path):
                print(f"- removing {path}/...")
                rmtree(path)

        # Clean generated files
        clean_generated_files()

        print(f"\n- git reset to {default_branch}...")
        shell_call([
            "git",
            "reset",
            "--hard",
            default_branch,
            ])

        print("\n- running cmake configure...")
        run_cmake_configure()

        print_status(ResultStatus.SUCCESS)
        return 0


class CleanGeneratedCommand(Command):
    """'cleangenerated' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(CleanGeneratedCommand, self).__init__(
            subparsers,
            name="cleangenerated",
            help_short="Removes generated shader bytecode files.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print("Cleaning generated files...")
        clean_generated_files()
        print_status(ResultStatus.SUCCESS)
        return 0



def find_xenia_source_files():
    """Gets all xenia source files in the project.

    Returns:
      A list of file paths.
    """
    return [os.path.join(root, name)
            for root, dirs, files in os.walk("src")
            for name in files
            if name.endswith((".cc", ".c", ".h", ".inl", ".inc"))]


class LintCommand(Command):
    """'lint' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(LintCommand, self).__init__(
            subparsers,
            name="lint",
            help_short="Checks for lint errors with clang-format.",
            *args, **kwargs)
        self.parser.add_argument(
            "--all", action="store_true",
            help="Lint all files, not just those changed.")
        self.parser.add_argument(
            "--origin", action="store_true",
            help=f"Lints all files changed relative to origin/{default_branch}.")

    def execute(self, args, pass_args, cwd):
        clang_format_binary = get_clang_format_binary()

        difftemp = ".difftemp.txt"

        if args["all"]:
            all_files = find_xenia_source_files()
            all_files.sort()
            print(f"- linting {len(all_files)} files")
            any_errors = False
            for file_path in all_files:
                if os.path.exists(difftemp): os.remove(difftemp)
                ret = shell_call([
                    clang_format_binary,
                    "-output-replacements-xml",
                    "-style=file",
                    file_path,
                    ], throw_on_error=False, stdout_path=difftemp)
                with open(difftemp) as f:
                    had_errors = "<replacement " in f.read()
                if os.path.exists(difftemp): os.remove(difftemp)
                if had_errors:
                    any_errors = True
                    print(f"\n{file_path}")
                    shell_call([
                        clang_format_binary,
                        "-style=file",
                        file_path,
                        ], throw_on_error=False, stdout_path=difftemp)
                    shell_call([
                        sys.executable,
                        "tools/diff.py",
                        file_path,
                        difftemp,
                        difftemp,
                        ])
                    shell_call([
                        "type" if sys.platform == "win32" else "cat",
                        difftemp,
                        ], shell=True if sys.platform == "win32" else False)
                    if os.path.exists(difftemp):
                        os.remove(difftemp)
                    print("")
            if any_errors:
                print("\nERROR: 1+ diffs. Stage changes and run 'xb format' to fix.")
                return 1
            else:
                print("\nLinting completed successfully.")
                return 0
        else:
            print("- git-clang-format --diff")
            if os.path.exists(difftemp): os.remove(difftemp)
            ret = shell_call([
                sys.executable,
                "third_party/clang-format/git-clang-format",
                f"--binary={clang_format_binary}",
                f"--commit={'origin/canary_experimental' if args['origin'] else 'HEAD'}",
                "--style=file",
                "--diff",
                ], throw_on_error=False, stdout_path=difftemp)
            with open(difftemp) as f:
                contents = f.read()
                not_modified = "no modified files" in contents
                not_modified = not_modified or "did not modify" in contents
                f.close()
            if os.path.exists(difftemp): os.remove(difftemp)
            if not not_modified:
                any_errors = True
                print("")
                shell_call([
                    sys.executable,
                    "third_party/clang-format/git-clang-format",
                    f"--binary={clang_format_binary}",
                    f"--commit={'origin/canary_experimental' if args['origin'] else 'HEAD'}",
                    "--style=file",
                    "--diff",
                    ])
                print_error("1+ diffs. Stage changes and run 'xb format' to fix.")
                return 1
            else:
                print("Linting completed successfully.")
                return 0


class FormatCommand(Command):
    """'format' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(FormatCommand, self).__init__(
            subparsers,
            name="format",
            help_short="Reformats staged code with clang-format.",
            *args, **kwargs)
        self.parser.add_argument(
            "--all", action="store_true",
            help="Format all files, not just those changed.")
        self.parser.add_argument(
            "--origin", action="store_true",
            help=f"Formats all files changed relative to origin/{default_branch}.")

    def execute(self, args, pass_args, cwd):
        clang_format_binary = get_clang_format_binary()

        if args["all"]:
            all_files = find_xenia_source_files()
            all_files.sort()
            print(f"- clang-format [{len(all_files)} files]")
            any_errors = False
            for file_path in all_files:
                ret = shell_call([
                    clang_format_binary,
                    "-i",
                    "-style=file",
                    file_path,
                    ], throw_on_error=False)
                if ret:
                    any_errors = True
            if any_errors:
                print("\nERROR: 1+ clang-format calls failed."
                      " Ensure all files are staged.")
                return 1
            else:
                print("\nFormatting completed successfully.")
                return 0
        else:
            print("- git-clang-format")
            ret = shell_call([
                sys.executable,
                "third_party/clang-format/git-clang-format",
                f"--binary={clang_format_binary}",
                f"--commit={'origin/canary_experimental' if args['origin'] else 'HEAD'}",
                ], throw_on_error=False)
            if ret != 0:
                print("\nFiles were formatted. Please stage the changes:")
                print("  git status")
                print("  git add <files>")
                return 1
            print("")

        return 0


# TODO(benvanik): merge into linter, or as lint --anal?
class StyleCommand(Command):
    """'style' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(StyleCommand, self).__init__(
            subparsers,
            name="style",
            help_short="Runs the style checker on all code.",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        all_files = [file_path for file_path in find_xenia_source_files()
                     if not file_path.endswith("_test.cc")]
        print(f"- cpplint [{len(all_files)} files]")
        ret = shell_call([
            sys.executable,
            "third_party/cpplint/cpplint.py",
            "--output=vs7",
            #"--linelength=80",
            "--filter=-build/c++11,+build/include_alpha",
            "--root=src",
            ] + all_files, throw_on_error=False)
        if ret:
            print("\nERROR: 1+ cpplint calls failed.")
            return 1
        else:
            print("\nStyle linting completed successfully.")
            return 0


# TODO(benvanik): merge into linter, or as lint --anal?
def changed_source_files(base, exts=(".cc", ".c", ".h", ".inl", ".inc")):
    """Returns source files changed vs base (merge-base...HEAD) that still exist."""
    out = None
    for rev in (f"{base}...HEAD", base):
        try:
            out = subprocess.check_output(
                ["git", "diff", "--name-only", "--diff-filter=ACMR", rev],
                text=True, stderr=subprocess.DEVNULL)
            break
        except Exception:
            continue
    if out is None:
        return []
    files = []
    for line in out.splitlines():
        line = line.strip()
        if line.endswith(tuple(exts)) and os.path.isfile(line):
            files.append(os.path.normpath(line))
    return files


class TidyCommand(Command):
    """'tidy' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(TidyCommand, self).__init__(
            subparsers,
            name="tidy",
            help_short="Runs the clang-tidy checker on all code.",
            *args, **kwargs)
        self.parser.add_argument(
            "--fix", action="store_true",
            help="Applies suggested fixes, where possible.")
        self.parser.add_argument(
            "--changed", action="store_true",
            help="Only tidy .cc files changed vs --base (fast, for CI/pre-push).")
        self.parser.add_argument(
            "--base", default=f"origin/{default_branch}",
            help=f"Base ref for --changed (default origin/{default_branch}).")

    def execute(self, args, pass_args, cwd):
        # Run cmake configure to generate compile_commands.json for clang-tidy.
        # The Ninja generator emits compile_commands.json at the build root.
        run_cmake_configure()
        tool_root = get_build_dir()

        if args["changed"]:
            all_files = [f for f in changed_source_files(args["base"], exts=(".cc",))
                         if not f.endswith("_test.cc")
                         and (f.startswith("src" + os.sep) or f.startswith("src/"))]
            if not all_files:
                print(f"No changed .cc files vs {args['base']}; nothing to tidy.")
                return 0
        else:
            all_files = [file_path for file_path in find_xenia_source_files()
                         if not file_path.endswith("_test.cc")
                         and file_path.endswith(".cc")]

        any_errors = False
        for file in all_files:
            print(f"- clang-tidy {file}")
            ret = shell_call([
                "clang-tidy",
                "-p", tool_root,
                "-checks=" + ",".join([
                    "clang-analyzer-*",
                    "google-*",
                    "misc-*",
                    "modernize-*"
                    # TODO(benvanik): pick the ones we want - some are silly.
                    # "readability-*",
                ]),
                ] + (["-fix"] if args["fix"] else []) + [
                    file,
                ], throw_on_error=False)
            if ret:
                any_errors = True

        if any_errors:
            print("\nERROR: 1+ clang-tidy calls failed.")
            return 1
        else:
            print("\nTidy completed successfully.")
            return 0

class StubCommand(Command):
    """'stub' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(StubCommand, self).__init__(
            subparsers,
            name="stub",
            help_short="Create new file(s) in the xenia source tree and run premake",
            *args, **kwargs)
        self.parser.add_argument(
            "--file", default=None,
            help="Generate a source file at the provided location in the source tree")
        self.parser.add_argument(
            "--class", default=None,
            help="Generate a class pair (.cc/.h) at the provided location in the source tree")

    def execute(self, args, pass_args, cwd):
        root = os.path.dirname(os.path.realpath(__file__))
        source_root = os.path.join(root, os.path.normpath("src/xenia"))

        if args["class"]:
            path = os.path.normpath(os.path.join(source_root, args["class"]))
            target_dir = os.path.dirname(path)
            class_name = os.path.basename(path)

            status = generate_source_class(path)
            if status > 0:
                return status

            print(f"Created class '{class_name}' at {target_dir}")

        elif args["file"]:
            path = os.path.normpath(os.path.join(source_root, args["file"]))
            target_dir = os.path.dirname(path)
            file_name = os.path.basename(path)

            status = generate_source_file(path)
            if status > 0:
                return status

            print(f"Created file '{file_name}' at {target_dir}")

        else:
            print_error("Please specify a file/class to generate")
            return 1

        # Reconfigure to pick up the new source file.
        run_cmake_configure()
        return 0

class DevenvCommand(Command):
    """'devenv' command.
    """

    def __init__(self, subparsers, *args, **kwargs):
        super(DevenvCommand, self).__init__(
            subparsers,
            name="devenv",
            help_short="Launches the development environment.",
            *args, **kwargs)
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")

    def execute(self, args, pass_args, cwd):
        if sys.platform == "win32":
            if not vs_version:
                print_error("Visual Studio is not installed.");
                return 1
            print("Launching Visual Studio...")
        elif has_bin("clion") or has_bin("clion.sh"):
            print("Launching CLion...")
            create_clion_workspace()
        else:
            print("IDE not detected. CMakeLists.txt is in the project root.")

        target_arch = args.get("target_arch", None)

        print("\n- running cmake configure...")
        run_cmake_configure(target_arch=target_arch)

        print("\n- launching devenv...")
        if sys.platform == "win32":
            # Generate a VS .sln for IDE use (normal builds still use Ninja)
            is_native_arm64 = platform.machine() in ("ARM64", "aarch64")
            # Determine the effective target architecture
            if target_arch == "arm64":
                vs_arch = "ARM64"
            elif target_arch == "x64":
                vs_arch = "x64"
            elif is_native_arm64:
                vs_arch = "ARM64"
            else:
                vs_arch = "x64"

            is_cross = (vs_arch == "ARM64" and not is_native_arm64)
            vs_build_dir = os.path.join(get_build_dir(target_arch), "vs")

            cmake_args = [
                "cmake",
                "-S", ".",
                "-B", vs_build_dir,
                "-A", vs_arch,
                "-DXENIA_BUILD_TESTS=ON",
            ]

            if is_cross:
                # Cross-compiling from x64 to ARM64.
                # Use vswhere to find a VS installation with ARM64 C++ tools
                # and force the correct generator/instance since CMake might
                # otherwise pick a VS without ARM64 support.
                vs_generator_map = {
                    2019: "Visual Studio 16 2019",
                    2022: "Visual Studio 17 2022",
                }
                try:
                    vswhere_out = subprocess.check_output(
                        "tools/vswhere/vswhere.exe"
                        ' -version "[17,)" -latest -prerelease'
                        " -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64"
                        " -format json -utf8"
                        " -products"
                        " Microsoft.VisualStudio.Product.Enterprise"
                        " Microsoft.VisualStudio.Product.Professional"
                        " Microsoft.VisualStudio.Product.Community"
                        " Microsoft.VisualStudio.Product.BuildTools",
                        encoding="utf-8",
                    )
                    arm64_vs_list = jsonloads(vswhere_out) if vswhere_out else []
                except Exception:
                    arm64_vs_list = []

                if not arm64_vs_list:
                    print_error(
                        "No Visual Studio installation with ARM64 C++ build tools found.\n"
                        "  Install the 'MSVC ARM64/ARM64EC build tools' component\n"
                        "  via the Visual Studio Installer.")
                    return 1

                arm64_vs = arm64_vs_list[0]
                arm64_vs_path = arm64_vs.get("installationPath", "")
                arm64_vs_plv = int(arm64_vs.get("catalog", {}).get(
                    "productLineVersion", VSVERSION_MINIMUM))

                vs_generator = vs_generator_map.get(arm64_vs_plv)
                toolset_parts = ["host=x64"]
                if not vs_generator:
                    latest_known = max(vs_generator_map.keys())
                    vs_generator = vs_generator_map[latest_known]
                    print(f"  Note: VS {arm64_vs_plv} detected with ARM64 tools.")
                    print(f"  Using \"{vs_generator}\" generator with that instance.")
                    vc_dir = os.path.join(arm64_vs_path, "MSBuild", "Microsoft", "VC")
                    if os.path.isdir(vc_dir):
                        toolsets = sorted(d for d in os.listdir(vc_dir) if d.startswith("v"))
                        if toolsets:
                            toolset_parts.insert(0, toolsets[-1])

                cmake_args += [
                    "-G", vs_generator,
                    "-T", ",".join(toolset_parts),
                    "-DCMAKE_SYSTEM_PROCESSOR=ARM64",
                    f"-DCMAKE_GENERATOR_INSTANCE={arm64_vs_path}",
                ]

            ret = subprocess.call(cmake_args)
            if ret == 0:
                generate_version_h(vs_build_dir)
            # VS 2026+ generates .slnx, older versions generate .sln
            sln_path = os.path.join(vs_build_dir, "xenia.slnx")
            if not os.path.exists(sln_path):
                sln_path = os.path.join(vs_build_dir, "xenia.sln")
            if ret != 0 or not os.path.exists(sln_path):
                print_error(f"Failed to generate VS solution. Check cmake output above.")
                return 1
            print(f"Opening {sln_path} in Visual Studio...")
            shell_call(["devenv", sln_path])
        elif has_bin("clion"):
            shell_call(["clion", "."])
        elif has_bin("clion.sh"):
            shell_call(["clion.sh", "."])
        else:
            print("No supported IDE found. Open the project root in your IDE.")
            print("CMakeLists.txt and CMakePresets.json are in the project root.")
        print("")

        return 0


class WhereCommand(Command):
    """'where' / 'binpath' command — print resolved build output directory."""

    def __init__(self, subparsers, name="where", *args, **kwargs):
        super(WhereCommand, self).__init__(
            subparsers,
            name=name,
            help_short="Print resolved build output directory.",
            help_long="Prints the absolute path to built binaries for the given config/arch.",
            *args, **kwargs)
        self.parser.add_argument(
            "--config", choices=["checked", "debug", "release"], default="release",
            type=str.lower, help="Build configuration.")
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")

    def execute(self, args, pass_args, cwd):
        print(get_build_bin_path(args))
        return 0


class MigrateBuildCommand(Command):
    """'migrate-build' command — remove legacy build dirs after layout change."""

    def __init__(self, subparsers, *args, **kwargs):
        super(MigrateBuildCommand, self).__init__(
            subparsers,
            name="migrate-build",
            help_short="Remove legacy build/ directories (post Build/ consolidation).",
            *args, **kwargs)

    def execute(self, args, pass_args, cwd):
        print("Removing legacy build directories...\n")
        for path in xp.legacy_build_dirs():
            if os.path.isdir(path):
                print(f"- removing {path}/...")
                rmtree(path)
        print("\nReconfigure with: python xenia-build.py setup")
        print("Then build:       python xenia-build.py build --config=release")
        print_status(ResultStatus.SUCCESS)
        return 0


class RunCommand(Command):
    """'run' command — launch xenia_canary from the resolved output directory."""

    def __init__(self, subparsers, *args, **kwargs):
        super(RunCommand, self).__init__(
            subparsers,
            name="run",
            help_short="Run xenia_canary from the resolved build output directory.",
            *args, **kwargs)
        self.parser.add_argument(
            "--config", choices=["checked", "debug", "release"], default="release",
            type=str.lower, help="Build configuration.")
        self.parser.add_argument(
            "--target-arch", type=normalize_target_arch, default=None,
            help="Target architecture (arm64/aarch64, x64/amd64/x86_64/x86).")

    def execute(self, args, pass_args, cwd):
        bin_dir = get_build_bin_path(args)
        if sys.platform == "win32":
            exe = os.path.join(bin_dir, "xenia_canary.exe")
        else:
            exe = os.path.join(bin_dir, "xenia_canary")
        if not os.path.isfile(exe):
            print_error(f"Missing {exe} — build xenia-app first.")
            return 1
        print(f"- {exe}")
        return subprocess.call([exe] + pass_args)


def _check_commit_tags(base=None):
    """Mirrors Check_commit_message.yml: every commit subject vs base needs a [Tag]."""
    import re as _re
    base = base or f"origin/{default_branch}"
    try:
        out = subprocess.check_output(
            ["git", "log", "--format=%s", f"{base}..HEAD"],
            text=True, stderr=subprocess.DEVNULL)
    except Exception:
        return True, f"could not diff vs {base}; skipped"
    bad = [line.strip() for line in out.splitlines()
           if line.strip() and not _re.match(r"^\[.+?\]", line.strip())]
    if bad:
        return False, f"{len(bad)} commit(s) missing [Tag]: e.g. {bad[0]!r}"
    return True, ""


class PreflightCommand(Command):
    """'preflight' / 'green' — local CI parity checks before push."""

    def __init__(self, subparsers, name="preflight", *args, **kwargs):
        super(PreflightCommand, self).__init__(
            subparsers,
            name=name,
            help_short="Run local Tier0-style checks (predict GitHub-green).",
            *args, **kwargs)
        self.parser.add_argument(
            "--skip-build", action="store_true",
            help="Skip configure/build; only run tests against existing binaries.")
        self.parser.add_argument(
            "--config", choices=["checked", "debug", "release"], default="release",
            type=str.lower, help="Build configuration.")

    def execute(self, args, pass_args, cwd):
        print("Phoenix preflight (local CI parity)\n")
        failed = 0
        build_args = {
            "config": args["config"],
            "target_arch": None,
        }

        def step(name, ok, detail=""):
            nonlocal failed
            color = bcolors.OKCYAN if ok else bcolors.FAIL
            status = "OK" if ok else "FAIL"
            suffix = f": {detail}" if detail else ""
            print(f"  [{color}{status}{bcolors.ENDC}] {name}{suffix}")
            if not ok:
                failed += 1

        guard = subprocess.call(
            [sys.executable, os.path.join("tools", "build", "check_build_paths.py")])
        step("path guard", guard == 0)

        # clang-format lint (PR-blocking gate); skip cleanly if clang-format absent.
        if _doctor_check_clang_format()[0]:
            lret = subprocess.call([sys.executable, "xenia-build.py", "lint"])
            step("clang-format lint", lret == 0)
        else:
            step("clang-format lint", True, "skipped (clang-format not found)")

        # commit-message [Tag] prefix (mirrors Check_commit_message.yml).
        ok_tags, tag_detail = _check_commit_tags()
        step("commit-message [Tag]", ok_tags, tag_detail)

        if not args["skip_build"]:
            ret = run_cmake_configure(
                build_type=args["config"].title(),
                build_tests=True,
                extra_args=["-DXENIA_BUILD_MISC=ON"],
            )
            step("cmake configure", ret == 0)
            if ret == 0:
                build_dir = get_build_dir()
                bret = subprocess.call([
                    "cmake", "--build", build_dir,
                    "--config", args["config"].title(),
                    "--target", "xenia-cpu-tests", "vmx128-fuzz", "xma2-diff",
                ])
                step("cmake build (tier0 targets)", bret == 0)
        else:
            step("cmake build (tier0 targets)", True, "skipped")

        bin_dir = get_build_bin_path(build_args)
        cpu_name = "xenia-cpu-tests.exe" if sys.platform == "win32" else "xenia-cpu-tests"
        cpu = get_bin(os.path.join(bin_dir, cpu_name))
        if cpu:
            cret = subprocess.call([cpu])
            step("xenia-cpu-tests", cret == 0)
        else:
            step("xenia-cpu-tests", False, "binary missing")

        fuzz_name = "vmx128-fuzz.exe" if sys.platform == "win32" else "vmx128-fuzz"
        fuzz = get_bin(os.path.join(bin_dir, fuzz_name))
        if fuzz:
            fret = subprocess.call([
                fuzz, "--vmx128_fuzz_iters=50000", "--vmx128_fuzz_seed=3735928559",
            ])
            step("vmx128-fuzz 50k", fret == 0)
        else:
            step("vmx128-fuzz 50k", False, "binary missing")

        if sys.platform == "win32":
            gret = subprocess.call([
                sys.executable, "tools/gpu_replay_ci/run.py",
                "--build-dir", get_build_dir(),
                "--cross-path", "d3d12",
            ])
            step("gpu replay (RTV/ROV)", gret == 0)
        else:
            step("gpu replay (RTV/ROV)", True, "skipped on non-Windows")

        pytest = subprocess.call([
            sys.executable, "-m", "pytest", "tools/phoenixctl/tests", "-q",
        ], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        step("phoenixctl tests", pytest == 0)

        print("")
        if failed:
            print_error(f"preflight: {failed} check(s) failed")
            return 1
        print_status(ResultStatus.SUCCESS)
        return 0


def _powershell_argv():
    exe = "powershell" if sys.platform == "win32" else "pwsh"
    return [exe, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File"]


class MatrixCommand(Command):
    """'matrix' — build + check every locally-buildable cell (universal build)."""

    ALL_CELLS = [
        "windows-x64", "windows-arm64", "android", "linux-x64", "linux-arm64", "macos",
    ]

    def __init__(self, subparsers, *args, **kwargs):
        super(MatrixCommand, self).__init__(
            subparsers,
            name="matrix",
            help_short="Build + check every local cell (Win x64/ARM64, Android, Linux x64/ARM64).",
            help_long="Drives each cell through the existing per-cell build path and prints an "
                      "aggregated OK/FAIL/SKIP table. macOS is CI-only off a non-mac host (use xb ci).",
            *args, **kwargs)
        self.parser.add_argument(
            "--cells", default=None,
            help="Comma-separated subset of: " + ", ".join(self.ALL_CELLS) + ".")
        self.parser.add_argument(
            "--config", choices=["checked", "debug", "release"], default="release",
            type=str.lower, help="Build configuration.")
        self.parser.add_argument(
            "--build-only", action="store_true",
            help="Build each cell but skip its checks (faster).")

    def execute(self, args, pass_args, cwd):
        config = args["config"].title()
        build_only = args["build_only"]
        if args["cells"]:
            cells = [c.strip() for c in args["cells"].split(",") if c.strip()]
            unknown = [c for c in cells if c not in self.ALL_CELLS]
            if unknown:
                print_error(f"unknown cell(s): {', '.join(unknown)}")
                return 1
        else:
            cells = list(self.ALL_CELLS)

        print(f"Phoenix build matrix ({config})\n")
        results = []
        for cell in cells:
            print(f"=== {cell} ===")
            status, detail = self._run_cell(cell, config, build_only)
            results.append((cell, status, detail))
            print("")

        print("Matrix results:")
        any_fail = False
        for cell, status, detail in results:
            if status == "OK":
                color = bcolors.OKCYAN
            elif status == "SKIP":
                color = bcolors.WARNING
            else:
                color = bcolors.FAIL
                any_fail = True
            suffix = f" - {detail}" if detail else ""
            print(f"  [{color}{status}{bcolors.ENDC}] {cell}{suffix}")

        print("")
        if any_fail:
            print_error("matrix: one or more local cells failed.")
            return 1
        print_status(ResultStatus.SUCCESS)
        return 0

    def _xb(self, argv):
        return subprocess.call([sys.executable, "xenia-build.py"] + argv)

    def _run_cell(self, cell, config, build_only):
        cfg = config.lower()
        if cell == "windows-x64":
            if sys.platform != "win32":
                return ("SKIP", "host is not Windows")
            if self._xb(["build", f"--config={cfg}", "--target", "app",
                         "--target", "tests", "--target", "fuzz"]) != 0:
                return ("FAIL", "build")
            if build_only:
                return ("OK", "build-only")
            bin_dir = os.path.join(self_path, xp.get_bin_dir(config=config))
            cpu = os.path.join(bin_dir, "xenia-cpu-tests.exe")
            if os.path.isfile(cpu) and subprocess.call([cpu]) != 0:
                return ("FAIL", "cpu-tests")
            fuzz = os.path.join(bin_dir, "vmx128-fuzz.exe")
            if os.path.isfile(fuzz) and subprocess.call(
                    [fuzz, "--vmx128_fuzz_iters=50000", "--vmx128_fuzz_seed=3735928559"]) != 0:
                return ("FAIL", "vmx128")
            return ("OK", "")

        if cell == "windows-arm64":
            if sys.platform != "win32":
                return ("SKIP", "host is not Windows")
            if not _doctor_check_msvc_arm64()[0]:
                return ("SKIP", "MSVC ARM64 tools not installed")
            if self._xb(["build", "--target-arch", "arm64", f"--config={cfg}",
                         "--target", "app", "--target", "tests"]) != 0:
                return ("FAIL", "build")
            if build_only:
                return ("OK", "build-only")
            rc = subprocess.call(_powershell_argv() +
                                 [os.path.join("tools", "tier0", "arm64-verify.ps1"),
                                  "-Config", config])
            return ("OK" if rc == 0 else "FAIL", "" if rc == 0 else "verify")

        if cell == "android":
            ndk = os.environ.get("ANDROID_NDK_ROOT") or os.environ.get("ANDROID_NDK_HOME")
            if not ndk:
                return ("SKIP", "ANDROID_NDK_ROOT not set")
            if self._xb(["build", "--target-os", "android", f"--config={cfg}",
                         "--target", "app"]) != 0:
                return ("FAIL", "build")
            so = os.path.join(self_path, xp.get_bin_dir(os_name="Android", config=config),
                              "libxenia-app.so")
            return ("OK", "") if os.path.isfile(so) else ("FAIL", "missing libxenia-app.so")

        if cell in ("linux-x64", "linux-arm64"):
            ok_docker, why = _doctor_check_docker()
            if not ok_docker:
                return ("SKIP", f"docker unavailable ({why})")
            arch = "x64" if cell == "linux-x64" else "arm64"
            script = os.path.join("tools", "docker", "run-linux-build.ps1")
            ps = _powershell_argv() + [script, "-Config", cfg, "-Arch", arch]
            if build_only or arch == "arm64":
                ps.append("-SkipVerify")
            rc = subprocess.call(ps)
            return ("OK" if rc == 0 else "FAIL", "" if rc == 0 else "docker build")

        if cell == "macos":
            if sys.platform != "darwin":
                return ("SKIP", "CI-only off macOS host -> run xb ci")
            if self._xb(["build", f"--config={cfg}", "--target", "app", "--target", "tests"]) != 0:
                return ("FAIL", "build")
            if build_only:
                return ("OK", "build-only")
            rc = subprocess.call(["bash", os.path.join("tools", "tier0", "macos-verify.sh")])
            return ("OK" if rc == 0 else "FAIL", "" if rc == 0 else "verify")

        return ("SKIP", "unknown cell")


class CiCommand(Command):
    """'ci' — trigger the GitHub Actions Orchestrator and watch it green/red."""

    def __init__(self, subparsers, *args, **kwargs):
        super(CiCommand, self).__init__(
            subparsers,
            name="ci",
            help_short="Trigger + watch the GitHub matrix (true GitHub-green, incl. macOS).",
            *args, **kwargs)
        self.parser.add_argument(
            "--ref", default=None, help="Branch/ref to run (default: current branch).")
        self.parser.add_argument(
            "--workflow", default="Orchestrator.yml", help="Workflow file to dispatch.")
        self.parser.add_argument(
            "--no-watch", action="store_true", help="Dispatch only; do not watch.")

    def execute(self, args, pass_args, cwd):
        if not has_bin("gh"):
            print_error("gh CLI not found. Install https://cli.github.com/ then: gh auth login")
            return 1
        ref = args["ref"]
        if not ref:
            try:
                ref = subprocess.check_output(
                    ["git", "rev-parse", "--abbrev-ref", "HEAD"], text=True).strip()
            except Exception:
                print_error("could not determine current branch; pass --ref.")
                return 1
        wf = args["workflow"]
        print(f"- dispatching {wf} on {ref}...")
        if subprocess.call(["gh", "workflow", "run", wf, "--ref", ref]) != 0:
            print_error("gh workflow run failed (needs workflow_dispatch + push access).")
            return 1
        if args["no_watch"]:
            print("Dispatched. Watch with: gh run watch")
            return 0
        import time
        time.sleep(6)
        try:
            run_id = subprocess.check_output(
                ["gh", "run", "list", "--workflow", wf, "--branch", ref,
                 "--limit", "1", "--json", "databaseId", "--jq", ".[0].databaseId"],
                text=True).strip()
        except Exception:
            run_id = ""
        if not run_id:
            print("Dispatched, but could not resolve the run id; check: gh run list")
            return 0
        print(f"- watching run {run_id} (Ctrl-C stops watching; the run keeps going)...")
        rc = subprocess.call(["gh", "run", "watch", run_id, "--exit-status"])
        print_status(ResultStatus.SUCCESS if rc == 0 else ResultStatus.FAILURE)
        return rc


def discover_commands(subparsers):
    """Looks for all commands and returns a dictionary of them."""
    commands = {
        "doctor": DoctorCommand(subparsers),
        "setup": SetupCommand(subparsers),
        "pull": PullCommand(subparsers),
        "premake": PremakeCommand(subparsers),
        "build": BuildCommand(subparsers),
        "buildshaders": BuildShadersCommand(subparsers),
        "devenv": DevenvCommand(subparsers),
        "gentests": GenTestsCommand(subparsers),
        "test": TestCommand(subparsers),
        "gputest": GpuTestCommand(subparsers),
        "clean": CleanCommand(subparsers),
        "nuke": NukeCommand(subparsers),
        "cleangenerated": CleanGeneratedCommand(subparsers),
        "lint": LintCommand(subparsers),
        "format": FormatCommand(subparsers),
        "style": StyleCommand(subparsers),
        "tidy": TidyCommand(subparsers),
        "stub": StubCommand(subparsers),
        "where": WhereCommand(subparsers, "where"),
        "binpath": WhereCommand(subparsers, "binpath"),
        "run": RunCommand(subparsers),
        "preflight": PreflightCommand(subparsers, "preflight"),
        "green": PreflightCommand(subparsers, "green"),
        "matrix": MatrixCommand(subparsers),
        "ci": CiCommand(subparsers),
        "migrate-build": MigrateBuildCommand(subparsers),
        }
    return commands


if __name__ == "__main__":
    main()
