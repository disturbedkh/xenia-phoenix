/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_EMULATOR_H_
#define XENIA_EMULATOR_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "xenia/apu/audio_media_player.h"
#include "xenia/base/delegate.h"
#include "xenia/base/exception_handler.h"
#include "xenia/base/threading.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/netplay/network_adapter_manager.h"
#include "xenia/kernel/netplay/upnp.h"
#include "xenia/kernel/netplay/xlive_api.h"
#include "xenia/kernel/util/game_info_database.h"
#include "xenia/kernel/util/xlast.h"
#include "xenia/memory.h"
#include "xenia/patcher/patcher.h"
#include "xenia/patcher/plugin_loader.h"
#include "xenia/ui/immediate_drawer.h"
#include "xenia/vfs/device.h"
#include "xenia/vfs/virtual_file_system.h"
#include "xenia/xbox.h"

namespace xe {
namespace apu {
class AudioSystem;
}  // namespace apu
namespace cpu {
class ExportResolver;
class Processor;
class ThreadState;
}  // namespace cpu
namespace gpu {
class GraphicsSystem;
}  // namespace gpu
namespace hid {
class InputDriver;
class InputSystem;
}  // namespace hid
namespace ui {
class ImGuiDrawer;
class Window;
}  // namespace ui
}  // namespace xe

namespace xe {

constexpr fourcc_t kEmulatorSaveSignature = make_fourcc("XSAV");
static constexpr std::string_view kDefaultGameSymbolicLink = "GAME:";
static constexpr std::string_view kDefaultPartitionSymbolicLink = "D:";
static constexpr std::string_view kDefaultUpdateSymbolicLink = "UPDATE:";

// High-level emulator lifecycle (launcher vs title vs relaunch).
enum class LifecycleState {
  Bare,
  Ready,
  Launching,
  Running,
  Paused,
  Terminating,
  Relaunching,
  Respawning,
  Exiting,
};

// The main type that runs the whole emulator.
// This is responsible for initializing and managing all the various subsystems.
class Emulator {
 public:
  // This is the class for the top-level callbacks. They may be called in an
  // undefined order, so among them there must be no dependencies on each other,
  // especially hierarchical ones. If hierarchical handling is needed, for
  // instance, if a specific implementation of a subsystem needs to handle
  // changes, but the entire implementation must be reloaded, the implementation
  // in this example _must not_ register / unregister its own callback - rather,
  // the proper ordering and hierarchy should be constructed in a single
  // callback (in this example, for the whole subsystem).
  //
  // All callbacks must be created and destroyed in the UI thread only (or the
  // thread that takes its place in the architecture of the specific app if
  // there's no UI), as they are invoked in the UI thread.
  class GameConfigLoadCallback {
   public:
    GameConfigLoadCallback(Emulator& emulator);
    GameConfigLoadCallback(const GameConfigLoadCallback& callback) = delete;
    GameConfigLoadCallback& operator=(const GameConfigLoadCallback& callback) =
        delete;
    virtual ~GameConfigLoadCallback();

    // The callback is invoked in the UI thread (or the thread that takes its
    // place in the architecture of the specific app if there's no UI).
    virtual void PostGameConfigLoad() = 0;

   protected:
    Emulator& emulator() const { return emulator_; }

   private:
    Emulator& emulator_;
  };

  explicit Emulator(const std::filesystem::path& command_line,
                    const std::filesystem::path& storage_root,
                    const std::filesystem::path& content_root,
                    const std::filesystem::path& cache_root);
  ~Emulator();

  // Full command line used when launching the process.
  const std::filesystem::path& command_line() const { return command_line_; }

  // Folder persistent internal emulator data is stored in.
  const std::filesystem::path& storage_root() const { return storage_root_; }

  // Folder guest content is stored in.
  const std::filesystem::path& content_root() const { return content_root_; }

  // Folder files safe to remove without significant side effects are stored in.
  const std::filesystem::path& cache_root() const { return cache_root_; }

  // Name of the title in the default language.
  const std::string& title_name() const { return title_name_; }

  // Version of the title as a string.
  const std::string& title_version() const { return title_version_; }

  // Currently running title ID
  uint32_t title_id() const {
    return !title_id_.has_value() ? 0 : title_id_.value();
  }

  // Are we currently running a title?
  bool is_title_open() const { return title_id_.has_value(); }

  LifecycleState lifecycle_state() const {
    return lifecycle_state_.load(std::memory_order_acquire);
  }

  const std::filesystem::path& last_launch_path() const {
    return last_launch_path_;
  }

  // Window used for displaying graphical output. Can be null.
  ui::Window* display_window() const { return display_window_; }

  // ImGui drawer for various kinds of dialogs requested by the guest. Can be
  // null.
  ui::ImGuiDrawer* imgui_drawer() const { return imgui_drawer_; }

  // Guest memory system modelling the RAM (both virtual and physical) of the
  // system.
  Memory* memory() const { return memory_.get(); }

  // Virtualized processor that can execute PPC code.
  cpu::Processor* processor() const { return processor_.get(); }

  // Audio hardware emulation for decoding and playback.
  apu::AudioSystem* audio_system() const { return audio_system_.get(); }

  // Xbox media player (XMP) emulation for WMA and MP3 playback.
  apu::AudioMediaPlayer* audio_media_player() const {
    return audio_media_player_.get();
  }

  // GPU emulation for command list processing.
  gpu::GraphicsSystem* graphics_system() const {
    return graphics_system_.get();
  }

  // Recreate GPU backend (e.g. after internal resolution scale change). Not
  // supported while a title is running.
  X_STATUS RecreateGraphicsSystem();

  // Human-interface Device (HID) adapters for controllers.
  hid::InputSystem* input_system() const { return input_system_.get(); }

  // Kernel function export table used to resolve exports when JITing code.
  cpu::ExportResolver* export_resolver() const {
    return export_resolver_.get();
  }

  // File systems mapped to disc images, folders, etc for games and save data.
  vfs::VirtualFileSystem* file_system() const { return file_system_.get(); }

  // The 'kernel', tracking all kernel objects and other state.
  // This is effectively the guest operating system.
  kernel::KernelState* kernel_state() const { return kernel_state_.get(); }

  patcher::Patcher* patcher() const { return patcher_.get(); }

  patcher::PluginLoader* plugin_loader() const { return plugin_loader_.get(); }

  kernel::util::GameInfoDatabase* game_info_database() const {
    return game_info_database_.get();
  }

  kernel::NetworkAdapterManager* GetNetworkAdapterManager() {
    return network_adapter_manager_.get();
  }

  kernel::UPnP* GetUPnP() { return upnp_.get(); }
  void ShutdownUPnP() { upnp_.reset(); }

  kernel::XLiveAPI* GetXboxLiveAPI() { return xbox_live_api_.get(); }

  // Bare-essentials init: memory, cpu, vfs, kernel. Stores subsystem factories;
  // call SetupSubsystems after per-game cvar overrides are applied.
  X_STATUS Setup(
      ui::Window* display_window, ui::ImGuiDrawer* imgui_drawer,
      bool require_cpu_backend,
      std::function<std::unique_ptr<apu::AudioSystem>(cpu::Processor*)>
          audio_system_factory,
      std::function<std::unique_ptr<gpu::GraphicsSystem>()>
          graphics_system_factory,
      std::function<std::vector<std::unique_ptr<hid::InputDriver>>(ui::Window*)>
          input_driver_factory);

  // Creates GPU/APU from stored factories and attaches HID drivers.
  X_STATUS SetupSubsystems();

  // Tears down GPU/APU/media player; kernel and vfs stay alive.
  void ShutdownSubsystems();

  const std::string& active_gpu_backend() const { return active_gpu_backend_; }
  const std::string& active_apu_backend() const { return active_apu_backend_; }

  // Full teardown (used by relaunch and process exit).
  void Shutdown();

  void set_mount_standard_drives_callback(std::function<void()> callback) {
    mount_standard_drives_callback_ = std::move(callback);
  }
  void SetPendingBackends(std::string gpu, std::string apu);
  void MountStandardDrives();

  // Terminates the currently running title.
  X_STATUS TerminateTitle();

  // Full in-process relaunch. Must run on a non-guest thread.
  void RelaunchTitle(const std::string& host_path,
                     const std::string& launch_module, uint32_t launch_flags,
                     std::vector<uint8_t> launch_data);

  // Stop title and return kernel to idle (launcher visible). Non-guest thread.
  void ResetTitle();

  // Queue work on the dedicated lifecycle worker (joinable, not detached).
  void PostToLifecycleWorker(std::function<void()> work);
  void ShutdownLifecycleWorker();

  // Called on the UI thread after SetupSubsystems during relaunch/reset.
  void set_presenter_setup_callback(std::function<void()> callback) {
    presenter_setup_callback_ = std::move(callback);
  }
  // Called on the UI thread BEFORE ShutdownSubsystems destroys the
  // GraphicsSystem (and its Presenter). The UI must drop every pointer it
  // holds into the presenter (window swap chain, ImGuiDrawer, immediate
  // drawer, profiler IO, etc.) here — otherwise ShowLauncher on the next
  // Ready transition will dereference freed memory.
  void set_presenter_teardown_callback(std::function<void()> callback) {
    presenter_teardown_callback_ = std::move(callback);
  }

  const std::unique_ptr<vfs::Device> CreateVfsDevice(
      const std::filesystem::path& path, const std::string_view mount_path);

  X_STATUS MountPath(const std::filesystem::path& path,
                     const std::string_view mount_path);

  enum class FileSignatureType {
    XEX1,
    XEX2,
    ELF,
    CON,
    LIVE,
    PIRS,
    XISO,
    ZAR,
    EXE,
    Unknown
  };

  // Determine the executable signature
  FileSignatureType GetFileSignature(const std::filesystem::path& path);

  // Launches a game from the given file path.
  // This will attempt to infer the type of the given file (such as an iso, etc)
  // using heuristics.
  X_STATUS LaunchPath(const std::filesystem::path& path);

  // Launches a game from a .xex file by mounting the containing folder as if it
  // was an extracted STFS container.
  X_STATUS LaunchXexFile(const std::filesystem::path& path);

  // Launches a game from a disc image file (.iso, etc).
  X_STATUS LaunchDiscImage(const std::filesystem::path& path);

  // Launches a game from a disc archive file (.zar, etc).
  X_STATUS LaunchDiscArchive(const std::filesystem::path& path);

  // Launches a game from an STFS container file.
  X_STATUS LaunchStfsContainer(const std::filesystem::path& path);

  X_STATUS LaunchDefaultModule(const std::filesystem::path& path);

  enum class InstallState : uint8_t {
    preparing,
    pending,
    installing,
    installed,
    failed
  };

  constexpr static std::string_view installStateStringName[5] = {
      "Preparing", "Pending", "Installing", "Success", "Failed"};

  struct ContentInstallEntry {
    ContentInstallEntry(std::filesystem::path path) : path_(path) {};

    std::string name_{};
    std::filesystem::path path_;
    std::filesystem::path data_installation_path_;
    std::filesystem::path header_installation_path_;

    uint64_t content_size_ = 0;
    uint64_t currently_installed_size_ = 0;
    XContentType content_type_{};

    InstallState installation_state_{};
    X_STATUS installation_result_{};
    std::string installation_error_message_{};

    std::unique_ptr<ui::ImmediateTexture> icon_;
  };

  // Migrates data from content to content/xuid with respect to common data.
  X_STATUS DataMigration(const uint64_t xuid);

  X_STATUS ProcessContentPackageHeader(const std::filesystem::path& path,
                                       ContentInstallEntry& installation_info);

  // Extract content of package to content specific directory.
  X_STATUS InstallContentPackage(const std::filesystem::path& path,
                                 ContentInstallEntry& installation_info);

  // Extract content of zar package to desired directory.
  X_STATUS ExtractZarchivePackage(const std::filesystem::path& path,
                                  const std::filesystem::path& extract_dir);

  // Pack contents of a folder into a zar package.
  X_STATUS CreateZarchivePackage(const std::filesystem::path& inputDirectory,
                                 const std::filesystem::path& outputFile);

  struct PackContext {
    std::filesystem::path outputFilePath;
    std::ofstream currentOutputFile;
    bool hasError{false};
  };

  void Pause();
  void Resume();
  bool is_paused() const { return paused_; }
  bool SaveToFile(const std::filesystem::path& path);
  bool RestoreFromFile(const std::filesystem::path& path);

  // The game can request another title to be loaded.
  const std::filesystem::path GetNewDiscPath(std::string window_message = "");

  void WaitUntilExit();

  // Breaks WaitUntilExit and unblocks restore/relaunch waits during app
  // shutdown.
  void RequestShutdown();

 public:
  xe::Delegate<uint32_t, const std::string_view> on_launch;
  xe::Delegate<bool> on_shader_storage_initialization;
  xe::Delegate<> on_patch_apply;
  xe::Delegate<> on_terminate;
  xe::Delegate<> on_exit;
  xe::Delegate<LifecycleState, LifecycleState> on_lifecycle_change;
  xe::Delegate<const std::string_view, const std::u16string_view>
      on_presence_change;
  xe::Delegate<const kernel::XSESSION_INFO*, uint32_t, uint32_t, uint64_t>
      on_session_change;
  xe::Delegate<> on_before_shutdown;

  using LaunchNewTitleCallback = std::function<void(
      const std::string& host_path, const std::string& launch_module,
      uint32_t launch_flags, const std::string& launch_data_hex)>;
  LaunchNewTitleCallback on_launch_new_title() const {
    return on_launch_new_title_;
  }
  void set_on_launch_new_title(LaunchNewTitleCallback callback) {
    on_launch_new_title_ = std::move(callback);
  }

  void SetLifecycleState(LifecycleState new_state);

 private:
  enum : uint64_t { EmulatorFlagDisclaimerAcknowledged = 1ULL << 0 };
  static uint64_t GetPersistentEmulatorFlags();
  static void SetPersistentEmulatorFlags(uint64_t new_flags);
  static bool ExceptionCallbackThunk(Exception* ex, void* data);
  bool ExceptionCallback(Exception* ex);

  void AddGameConfigLoadCallback(GameConfigLoadCallback* callback);
  void RemoveGameConfigLoadCallback(GameConfigLoadCallback* callback);

  std::string FindLaunchModule();

  X_STATUS CompleteLaunch(const std::filesystem::path& path,
                          const std::string_view module_path);

  // Phase 1 of ResetTitle/RelaunchTitle: terminate guest title XThreads only
  // (XThread instances that are NOT XHostThreads). Run before draining the
  // subsystem workers so the title stops feeding ringbuffers/audio queues.
  void TerminateTitleXThreads_(const char* phase_label);

  // Phase 3 of ResetTitle/RelaunchTitle: sweep any XHostThreads still alive
  // in the kernel object table after ShutdownSubsystems. In practice this
  // should be a no-op; logged as a warning if it isn't.
  void TerminateRemainingHostXThreads_(const char* phase_label);

  std::filesystem::path command_line_;
  std::filesystem::path last_launch_path_;
  std::filesystem::path storage_root_;
  std::filesystem::path content_root_;
  std::filesystem::path cache_root_;

  std::string title_name_;
  std::string title_version_;

  ui::Window* display_window_ = nullptr;
  ui::ImGuiDrawer* imgui_drawer_ = nullptr;

  std::unique_ptr<Memory> memory_;

  std::unique_ptr<cpu::Processor> processor_;
  std::unique_ptr<apu::AudioSystem> audio_system_;
  std::unique_ptr<apu::AudioMediaPlayer> audio_media_player_;
  std::unique_ptr<gpu::GraphicsSystem> graphics_system_;
  std::unique_ptr<hid::InputSystem> input_system_;

  std::unique_ptr<cpu::ExportResolver> export_resolver_;
  std::unique_ptr<vfs::VirtualFileSystem> file_system_;
  std::unique_ptr<patcher::Patcher> patcher_;
  std::unique_ptr<patcher::PluginLoader> plugin_loader_;

  std::unique_ptr<kernel::KernelState> kernel_state_;

  // Accessible only from the thread that invokes those callbacks (the UI thread
  // if the UI is available).
  std::vector<GameConfigLoadCallback*> game_config_load_callbacks_;
  // Using an index, not an iterator, because after the erasure, the adjustment
  // must be done for the vector element indices that would be in the iterator
  // range that would be invalidated.
  // SIZE_MAX if not currently in the game config load callback loop.
  size_t game_config_load_callback_loop_next_index_ = SIZE_MAX;

  kernel::object_ref<kernel::XThread> main_thread_;
  kernel::object_ref<kernel::XHostThread> plugin_loader_thread_;
  std::optional<uint32_t> title_id_;  // Currently running title ID
  std::unique_ptr<kernel::util::GameInfoDatabase> game_info_database_;
  std::unique_ptr<kernel::NetworkAdapterManager> network_adapter_manager_;
  std::unique_ptr<kernel::UPnP> upnp_;
  std::unique_ptr<kernel::XLiveAPI> xbox_live_api_;

  bool paused_;
  bool restoring_;
  bool relaunching_ = false;
  std::atomic<bool> shutdown_requested_{false};
  bool preserve_processor_cache_on_relaunch_ = false;
  threading::Fence restore_fence_;  // Fired on restore finish.
  std::unique_ptr<threading::Event> relaunch_complete_event_;

  std::atomic<LifecycleState> lifecycle_state_{LifecycleState::Bare};

  bool require_cpu_backend_ = false;
  std::function<std::unique_ptr<apu::AudioSystem>(cpu::Processor*)>
      audio_system_factory_;
  std::function<std::unique_ptr<gpu::GraphicsSystem>()>
      graphics_system_factory_;
  std::function<std::vector<std::unique_ptr<hid::InputDriver>>(ui::Window*)>
      input_driver_factory_;

  std::string active_gpu_backend_;
  std::string active_apu_backend_;

  LaunchNewTitleCallback on_launch_new_title_;
  std::function<void()> presenter_setup_callback_;
  std::function<void()> presenter_teardown_callback_;
  std::function<void()> mount_standard_drives_callback_;
  std::string pending_gpu_backend_;
  std::string pending_apu_backend_;

  std::thread lifecycle_worker_;
  std::mutex lifecycle_worker_mutex_;
  std::condition_variable lifecycle_worker_cv_;
  std::deque<std::function<void()>> lifecycle_worker_queue_;
  bool lifecycle_worker_quit_ = false;
  void LifecycleWorkerThreadMain();
};

}  // namespace xe

#endif  // XENIA_EMULATOR_H_
