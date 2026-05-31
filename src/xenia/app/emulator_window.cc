/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/emulator_window.h"

#include "xenia/app/first_run_wizard.h"
#include "xenia/app/guide_overlay.h"
#include "xenia/app/launcher_dashboard.h"
#include "xenia/app/library/game_library.h"
#include "xenia/app/library_settings_dialog.h"
#include "xenia/app/preferences_dialog.h"

#include "third_party/imgui/imgui.h"
#include "third_party/stb/stb_image_write.h"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wabsolute-value"
#endif
#include "third_party/tomlplusplus/toml.hpp"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <string>
#include "xenia/app/console_settings_dialog.h"
#include "xenia/base/assert.h"
#include "xenia/base/clock.h"
#include "xenia/base/cvar.h"
#include "xenia/base/debugging.h"
#include "xenia/base/diagnostics.h"
#include "xenia/base/filesystem.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"
#include "xenia/base/profiling.h"
#include "xenia/base/system.h"
#include "xenia/base/threading.h"
#include "xenia/config.h"
#include "xenia/cpu/processor.h"
#include "xenia/emulator.h"
#include "xenia/gpu/command_processor.h"
#include "xenia/gpu/graphics_system.h"
#include "xenia/hid/input_system.h"
#include "xenia/kernel/xam/profile_manager.h"
#include "xenia/kernel/xam/xam_module.h"
#include "xenia/kernel/xam/xam_state.h"
#include "xenia/kernel/xconfig.h"
#include "xenia/ui/file_picker.h"
#include "xenia/ui/graphics_provider.h"
#include "xenia/ui/imgui_dialog.h"
#include "xenia/ui/imgui_drawer.h"
#include "xenia/ui/imgui_host_notification.h"
#include "xenia/ui/immediate_drawer.h"
#include "xenia/ui/presenter.h"

#if XE_PLATFORM_WIN32
#include <TlHelp32.h>
#include "xenia/base/platform_win.h"
#endif
#include "xenia/ui/ui_event.h"
#include "xenia/ui/virtual_key.h"
#if XE_PLATFORM_WIN32
#include "xenia/ui/window_win.h"
#endif  // XE_PLATFORM_WIN32

#include "version.h"

DECLARE_bool(debug);

DECLARE_string(hid);

DECLARE_bool(guide_button);

DECLARE_bool(clear_memory_page_state);

DECLARE_string(readback_resolve);

DECLARE_bool(readback_memexport);

DECLARE_int32(log_level);
DECLARE_uint32(log_mask);
DECLARE_bool(flush_log);

DEFINE_bool(fullscreen, false, "Whether to launch the emulator in fullscreen.",
            "Display");

DEFINE_bool(controller_hotkeys, false, "Hotkeys for Xbox and PS controllers.",
            "General");

DEFINE_bool(auto_check_updates, true,
            "Automatically check for updates on startup and notify if any are "
            "available.",
            "General");

DEFINE_string(
    postprocess_antialiasing, "",
    "Post-processing anti-aliasing effect to apply to the image output of the "
    "game.\n"
    "Using post-process anti-aliasing is heavily recommended when AMD "
    "FidelityFX Contrast Adaptive Sharpening or Super Resolution 1.0 is "
    "active.\n"
    "Use: [none, fxaa, fxaa_extreme]\n"
    " none (or any value not listed here):\n"
    "  Don't alter the original image.\n"
    " fxaa:\n"
    "  NVIDIA Fast Approximate Anti-Aliasing 3.11, normal quality preset (12)."
    "\n"
    " fxaa_extreme:\n"
    "  NVIDIA Fast Approximate Anti-Aliasing 3.11, extreme quality preset "
    "(39).",
    "Display");
DEFINE_string(
    postprocess_scaling_and_sharpening, "",
    "Post-processing effect to use for resampling and/or sharpening of the "
    "final display output.\n"
    "Use: [bilinear, cas, fsr]\n"
    " bilinear (or any value not listed here):\n"
    "  Original image at 1:1, simple bilinear stretching for resampling.\n"
    " cas:\n"
    "  Use AMD FidelityFX Contrast Adaptive Sharpening (CAS) for sharpening "
    "at scaling factors of up to 2x2, with additional bilinear stretching for "
    "larger factors.\n"
    " fsr:\n"
    "  Use AMD FidelityFX Super Resolution 1.0 (FSR) for highest-quality "
    "upscaling, or AMD FidelityFX Contrast Adaptive Sharpening for sharpening "
    "while not scaling or downsampling.\n"
    "  For scaling by factors of more than 2x2, multiple FSR passes are done.",
    "Display");
DEFINE_double(
    postprocess_ffx_cas_additional_sharpness,
    xe::ui::Presenter::GuestOutputPaintConfig::kCasAdditionalSharpnessDefault,
    "Additional sharpness for AMD FidelityFX Contrast Adaptive Sharpening "
    "(CAS), from 0 to 1.\n"
    "Higher is sharper.",
    "Display");
DEFINE_uint32(
    postprocess_ffx_fsr_max_upsampling_passes,
    xe::ui::Presenter::GuestOutputPaintConfig::kFsrMaxUpscalingPassesMax,
    "Maximum number of upsampling passes performed in AMD FidelityFX Super "
    "Resolution 1.0 (FSR) before falling back to bilinear stretching after the "
    "final pass.\n"
    "Each pass upscales only to up to 2x2 the previous size. If the game "
    "outputs a 1280x720 image, 1 pass will upscale it to up to 2560x1440 "
    "(below 4K), after 2 passes it will be upscaled to a maximum of 5120x2880 "
    "(including 3840x2160 for 4K), and so on.\n"
    "This variable has no effect if the display resolution isn't very high, "
    "but may be reduced on resolutions like 4K or 8K in case the performance "
    "impact of multiple FSR upsampling passes is too high, or if softer edges "
    "are desired.\n"
    "The default value is the maximum internally supported by Xenia.",
    "Display");
DEFINE_double(
    postprocess_ffx_fsr_sharpness_reduction,
    xe::ui::Presenter::GuestOutputPaintConfig::kFsrSharpnessReductionDefault,
    "Sharpness reduction for AMD FidelityFX Super Resolution 1.0 (FSR), in "
    "stops.\n"
    "Lower is sharper.",
    "Display");
// Dithering to 8bpc is enabled by default since the effect is minor, only
// effects what can't be shown normally by host displays, and nothing is changed
// by it for 8bpc source without resampling.
DEFINE_bool(
    postprocess_dither, true,
    "Dither the final image output from the internal precision to 8 bits per "
    "channel so gradients are smoother.\n"
    "On a 10bpc display, the lower 2 bits will still be kept, but noise will "
    "be added to them - disabling may be recommended for 10bpc, but it "
    "depends on the 10bpc displaying capabilities of the actual display used.",
    "Display");

DEFINE_int32(recent_titles_entry_amount, 10,
             "Allows user to define how many titles is saved in list of "
             "recently played titles.",
             "General");
DEFINE_bool(disable_doubleclick_fullscreen, false,
            "Allows the user to disable the behavior where a fast double-click "
            "causes Xenia to enter fullscreen mode.",
            "General");

namespace xe {
namespace app {

namespace {

const char* LifecycleStateLabel(LifecycleState state) {
  switch (state) {
    case LifecycleState::Bare:
      return "Bare";
    case LifecycleState::Ready:
      return "Ready";
    case LifecycleState::Launching:
      return "Launching";
    case LifecycleState::Running:
      return "Running";
    case LifecycleState::Paused:
      return "Paused";
    case LifecycleState::Terminating:
      return "Terminating";
    case LifecycleState::Relaunching:
      return "Relaunching";
    case LifecycleState::Respawning:
      return "Respawning";
    case LifecycleState::Exiting:
      return "Exiting";
    default:
      return "Unknown";
  }
}

}  // namespace

using xe::ui::FileDropEvent;
using xe::ui::KeyEvent;
using xe::ui::MenuItem;
using xe::ui::UIEvent;

using namespace xe::hid;
using namespace xe::gpu;

constexpr std::string_view kRecentlyPlayedTitlesFilename = "recent.toml";
constexpr std::string_view kBaseTitle = "Xenia-canary";

EmulatorWindow::EmulatorWindow(Emulator* emulator,
                               ui::WindowedAppContext& app_context,
                               uint32_t width, uint32_t height)
    : emulator_(emulator),
      app_context_(app_context),
      window_listener_(*this),
      window_(ui::Window::Create(app_context, kBaseTitle, width, height)),
      imgui_drawer_(
          std::make_shared<ui::ImGuiDrawer>(window_.get(), kZOrderImGui)),
      display_config_game_config_load_callback_(
          new DisplayConfigGameConfigLoadCallback(*emulator, *this)),
      graphics_settings_game_config_load_callback_(
          new GraphicsSettingsGameConfigLoadCallback(*emulator, *this)) {
  base_title_ = std::string(kBaseTitle) +
#ifdef DEBUG
#if _NO_DEBUG_HEAP == 1
                " DEBUG"
#else
                " CHECKED"
#endif
#endif
                " ("
#ifdef XE_BUILD_IS_PR
                "PR#" XE_BUILD_PR_NUMBER " - "
#endif
                XE_BUILD_BRANCH "@" XE_BUILD_COMMIT_SHORT " on " XE_BUILD_DATE
                ")";

  updater_ = std::make_shared<Updater>("AdrianCassar", "xenia-canary");

  LoadRecentlyLaunchedTitles();
}

std::unique_ptr<EmulatorWindow> EmulatorWindow::Create(
    Emulator* emulator, ui::WindowedAppContext& app_context, uint32_t width,
    uint32_t height) {
  assert_true(app_context.IsInUIThread());
  std::unique_ptr<EmulatorWindow> emulator_window(
      new EmulatorWindow(emulator, app_context, width, height));
  if (!emulator_window->Initialize()) {
    return nullptr;
  }
  return emulator_window;
}

void EmulatorWindow::ShutdownUpdaterDialog() {
  cancel_request = true;
  updater_dialog_.reset();
}

EmulatorWindow::~EmulatorWindow() {
  ShutdownUpdaterDialog();
  // Stop the hotkey listener and wait for it to exit; it touches members of
  // this window and Thread::reset() does not join.
  hotkeys_listener_running_ = false;
  if (Gamepad_HotKeys_Listener) {
    xe::threading::Wait(Gamepad_HotKeys_Listener.get(), false);
    Gamepad_HotKeys_Listener.reset();
  }

  // Notify the ImGui drawer that the immediate drawer is being destroyed.
  ShutdownGraphicsSystemPresenterPainting();
}

ui::Presenter* EmulatorWindow::GetGraphicsSystemPresenter() const {
  gpu::GraphicsSystem* graphics_system = emulator_->graphics_system();
  return graphics_system ? graphics_system->presenter() : nullptr;
}

void EmulatorWindow::SetupGraphicsSystemPresenterPainting() {
  ShutdownGraphicsSystemPresenterPainting();

  if (!window_) {
    return;
  }

  ui::Presenter* presenter = GetGraphicsSystemPresenter();
  if (!presenter) {
    return;
  }

  ApplyDisplayConfigForCvars();

  window_->SetPresenter(presenter);

  immediate_drawer_ =
      emulator_->graphics_system()->provider()->CreateImmediateDrawer();
  if (immediate_drawer_) {
    immediate_drawer_->SetPresenter(presenter);
    imgui_drawer_->SetPresenterAndImmediateDrawer(presenter,
                                                  immediate_drawer_.get());
    Profiler::SetUserIO(kZOrderProfiler, window_.get(), presenter,
                        immediate_drawer_.get());
  }

  // Pick up any pending launcher request that was deferred earlier because
  // the presenter wasn't bound yet (cold start: ShowLauncher in
  // OnEmulatorInitialized; relaunch: ShowLauncher from the queued
  // OnLifecycleChange(Ready)).
  if (emulator_->lifecycle_state() == LifecycleState::Ready) {
    ShowLauncher();
  }
}

void EmulatorWindow::ShutdownGraphicsSystemPresenterPainting() {
  Profiler::SetUserIO(kZOrderProfiler, window_.get(), nullptr, nullptr);
  imgui_drawer_->SetPresenterAndImmediateDrawer(nullptr, nullptr);
  immediate_drawer_.reset();
  if (window_) {
    window_->SetPresenter(nullptr);
  }
}

void EmulatorWindow::OnEmulatorInitialized() {
  if (!emulator_->kernel_state()
           ->xam_state()
           ->profile_manager()
           ->GetAccountCount()) {
    new NoProfileDialog(imgui_drawer_.get(), this);
    disable_hotkeys_ = true;
  }

  emulator_initialized_ = true;
  window_->SetMainMenuEnabled(true);
  // When the user can see that the emulator isn't initializing anymore (the
  // menu isn't disabled), enter fullscreen if requested.
  if (cvars::fullscreen) {
    SetFullscreen(true);
  }

  if (IsUseNexusForGameBarEnabled()) {
    XELOGE(
        "Xbox Gamebar Enabled, using BACK button instead of GUIDE for "
        "controller hotkeys!!!");
  }

  // Create a thread to listen for controller hotkeys.
  if (cvars::controller_hotkeys) {
    hotkeys_listener_running_ = true;
    Gamepad_HotKeys_Listener =
        threading::Thread::Create({}, [&] { GamepadHotKeys(); });
    Gamepad_HotKeys_Listener->set_name("Gamepad HotKeys Listener");
  }

  game_library_ =
      std::make_unique<library::GameLibrary>(emulator_->storage_root());
  game_library_->Load();

  std::vector<std::pair<std::string, std::filesystem::path>> recent;
  for (const auto& t : recently_launched_titles_) {
    recent.emplace_back(t.title_name, t.path_to_file);
  }
  game_library_->ImportRecentTitles(recent);

  if (!game_library_->watched_directories().empty()) {
    game_library_->StartScan();
  }

  emulator_->set_presenter_setup_callback(
      [this]() { SetupGraphicsSystemPresenterPainting(); });
  // Symmetric counterpart of presenter_setup_callback_: invoked on the UI
  // thread by Emulator::ShutdownSubsystems BEFORE the GraphicsSystem (and
  // its Presenter) are destroyed. We must drop every UI-side pointer into
  // the presenter here — otherwise the next ShowLauncher() walks a freed
  // std::map inside Presenter::AddUIDrawerFromUIThread.
  emulator_->set_presenter_teardown_callback(
      [this]() { ShutdownGraphicsSystemPresenterPainting(); });

  title_launch_dispatcher_ =
      std::make_unique<TitleLaunchDispatcher>(*emulator_, *this);

  emulator_->on_lifecycle_change.AddListener(
      [this](LifecycleState old_state, LifecycleState new_state) {
        if (app_context_.IsInUIThread()) {
          OnLifecycleChange(old_state, new_state);
        } else {
          app_context_.CallInUIThread([this, old_state, new_state]() {
            OnLifecycleChange(old_state, new_state);
          });
        }
      });

  ShowLauncher();

  if (FirstRunWizardDialog::IsNeeded(emulator_->storage_root())) {
    new FirstRunWizardDialog(imgui_drawer_.get(), *this);
  }

#if !defined(DEBUG) && !defined(XE_BUILD_IS_PR)
  if (cvars::auto_check_updates) {
    auto callback = [this](CheckForUpdateInfo update_info) {
      if (update_info.update_available) {
        app_context_.CallInUIThread([this, update_info]() {
          ShowUpdateAvailableDialog(update_info.metadata.commit_hash,
                                    update_info.metadata.commit_date);
        });
      }
    };
    update_info_ = updater_->StartupUpdateCheckAsync(cancel_request, callback);
  }
#endif
}

void EmulatorWindow::ShowUpdateAvailableDialog(const std::string& commit,
                                               const std::string& date) {
  const std::string short_commit = commit.substr(0, 9);
  const std::string message = fmt::format(
      "Date: {} ({})\n\n"
      "You can update via the Netplay -> Update Checker menu",
      date, short_commit);

  new xe::ui::HostNotificationWindow(imgui_drawer_.get(), "Update Available",
                                     message, 0, 9);
}

void EmulatorWindow::OnLifecycleChange(LifecycleState old_state,
                                       LifecycleState new_state) {
  XELOGW("EmulatorWindow::OnLifecycleChange: {} -> {} (UI thread={})",
         LifecycleStateLabel(old_state), LifecycleStateLabel(new_state),
         app_context_.IsInUIThread());
  switch (new_state) {
    case LifecycleState::Ready:
      launcher_overlay_requested_ = false;
      ShowLauncher();
      break;
    case LifecycleState::Launching:
      launcher_overlay_requested_ = false;
      HideLauncher();
      break;
    case LifecycleState::Running:
    case LifecycleState::Paused:
      if (!launcher_overlay_requested_) {
        HideLauncher();
      }
      break;
    case LifecycleState::Relaunching:
    case LifecycleState::Terminating:
    case LifecycleState::Respawning:
    case LifecycleState::Exiting:
    case LifecycleState::Bare:
      break;
  }
}

void EmulatorWindow::EmulatorWindowListener::OnClosing(ui::UIEvent& e) {
  emulator_window_.app_context_.QuitFromUIThread();
}

void EmulatorWindow::EmulatorWindowListener::OnFileDrop(ui::FileDropEvent& e) {
  emulator_window_.FileDrop(e.filename());
}

void EmulatorWindow::EmulatorWindowListener::OnKeyDown(ui::KeyEvent& e) {
  emulator_window_.OnKeyDown(e);
}

void EmulatorWindow::EmulatorWindowListener::OnMouseDown(ui::MouseEvent& e) {
  emulator_window_.OnMouseDown(e);
}

void EmulatorWindow::EmulatorWindowListener::OnMouseUp(ui::MouseEvent& e) {
  emulator_window_.OnMouseUp(e);
}

#if XE_PLATFORM_ANDROID
void EmulatorWindow::EmulatorWindowListener::OnTouchEvent(ui::TouchEvent& e) {
  auto* emulator = emulator_window_.emulator();
  auto* input = emulator ? emulator->input_system() : nullptr;
  auto* window = emulator_window_.window();
  if (!input || !window) {
    return;
  }
  const uint32_t width = std::max(window->GetActualPhysicalWidth(), 1u);
  const uint32_t height = std::max(window->GetActualPhysicalHeight(), 1u);
  const float norm_x = (2.0f * e.x() / static_cast<float>(width)) - 1.0f;
  const float norm_y = 1.0f - (2.0f * e.y() / static_cast<float>(height));
  const bool down = e.action() == ui::TouchEvent::Action::kDown ||
                    e.action() == ui::TouchEvent::Action::kMove;
  input->OnAndroidTouch(norm_x, norm_y, down);
}
#endif

void EmulatorWindow::EmulatorWindowListener::OnUsbDeviceChanged(
    bool is_arrival) {
  if (!emulator_window_.emulator()) {
    return;
  }

  if (!emulator_window_.emulator()->input_system()) {
    return;
  }

  auto* portal = emulator_window_.emulator()->input_system()->GetPortal();
  if (!portal) {
    return;
  }

  if (is_arrival) {
    portal->OnDeviceArrival();
  } else {
    portal->OnDeviceRemoval();
  }
}

void EmulatorWindow::DisplayConfigGameConfigLoadCallback::PostGameConfigLoad() {
  emulator_window_.ApplyDisplayConfigForCvars();
}

void EmulatorWindow::GraphicsSettingsGameConfigLoadCallback::
    PostGameConfigLoad() {
  emulator_window_.ApplyDisplayConfigForCvars();
}

void EmulatorWindow::ContentInstallDialog::OnDraw(ImGuiIO& io) {
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(20, 20), ImGuiCond_FirstUseEver);

  bool dialog_open = true;
  if (!ImGui::Begin(
          fmt::format("Installation Progress###{}", window_id_).c_str(),
          &dialog_open,
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
              ImGuiWindowFlags_HorizontalScrollbar)) {
    Close();
    ImGui::End();
    return;
  }

  bool is_everything_installed = true;
  for (const auto& entry : *installation_entries_) {
    ImGui::BeginTable(fmt::format("table_{}", entry.name_).c_str(), 2);
    ImGui::TableNextRow(0);
    ImGui::TableSetColumnIndex(0);
    if (entry.icon_) {
      ImGui::Image(reinterpret_cast<ImTextureID>(entry.icon_.get()),
                   ui::default_image_icon_size);
    } else {
      ImGui::Dummy(ui::default_image_icon_size);
    }
    ImGui::TableNextColumn();

    ImGui::Text("Name: %s", entry.name_.c_str());
    ImGui::Text("Installation Path:");
    ImGui::SameLine();
    if (ImGui::TextLink(
            xe::path_to_utf8(entry.data_installation_path_).c_str())) {
      LaunchFileExplorer(emulator_window_.emulator_->content_root() /
                         entry.data_installation_path_);
    }

    if (entry.content_type_ != xe::XContentType::kInvalid) {
      ImGui::Text("Content Type: %s",
                  XContentTypeMap.at(entry.content_type_).c_str());
    }

    std::string result = fmt::format(
        "Status: {}", xe::Emulator::installStateStringName[static_cast<uint8_t>(
                          entry.installation_state_)]);

    if (entry.installation_state_ == xe::Emulator::InstallState::failed) {
      result += fmt::format(" - {} ({:08X})",
                            entry.installation_error_message_.c_str(),
                            entry.installation_result_);
    }

    ImGui::Text("%s", result.c_str());
    ImGui::EndTable();

    if (entry.content_size_ > 0) {
      ImGui::ProgressBar(static_cast<float>(entry.currently_installed_size_) /
                         entry.content_size_);

      if (entry.currently_installed_size_ != entry.content_size_ &&
          entry.installation_result_ == X_ERROR_SUCCESS) {
        is_everything_installed = false;
      }
    } else {
      ImGui::ProgressBar(0.0f);
    }

    if (installation_entries_->size() > 1) {
      ImGui::Separator();
    }
  }
  ImGui::Spacing();

  ImGui::BeginDisabled(!is_everything_installed);
  if (ImGui::Button("Close")) {
    ImGui::EndDisabled();
    Close();
    ImGui::End();
    return;
  }
  ImGui::EndDisabled();

  if (!dialog_open && is_everything_installed) {
    Close();
    ImGui::End();
    return;
  }
  ImGui::End();
}

void EmulatorWindow::XMPConfigDialog::OnDraw(ImGuiIO& io) {
  ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(20, 20), ImGuiCond_FirstUseEver);

  bool dialog_open = true;
  if (!ImGui::Begin("Audio Player Menu", &dialog_open,
                    ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    Close();
    ImGui::End();
    return;
  }

  auto audio_player = emulator_window_.emulator_->audio_media_player();
  using xmp_state = kernel::xam::apps::XmpApp::State;
  if (audio_player) {
    ImGui::Text("Audio player status:");
    ImGui::SameLine();
    switch (audio_player->GetState()) {
      case xmp_state::kIdle:
        ImGui::Text("Idle");
        break;
      case xmp_state::kPaused:
        ImGui::Text("Paused");
        break;
      case xmp_state::kPlaying:
        ImGui::Text("Playing");
        break;
      default:
        break;
    }

    if (audio_player->IsPlaying()) {
      if (ImGui::Button("Pause")) {
        audio_player->Pause();
      }
    } else if (audio_player->IsPaused()) {
      if (ImGui::Button("Resume")) {
        audio_player->Continue();
      }
    }

    volume_ =
        emulator_window_.emulator_->audio_media_player()->GetVolume()->load();

    if (ImGui::SliderFloat("Audio player volume", &volume_, 0.0f, 1.0f,
                           "%.2f")) {
      audio_player->SetVolume(volume_);
    }
  }

  ImGui::End();

  if (!dialog_open) {
    Close();
    emulator_window_.xmp_config_dialog_.release();
    return;
  }
}

bool EmulatorWindow::Initialize() {
  window_->AddListener(&window_listener_);
  window_->AddInputListener(&window_listener_, kZOrderEmulatorWindowInput);

  // Main menu.
  // FIXME: This code is really messy.
  auto main_menu = MenuItem::Create(MenuItem::Type::kNormal);
  auto file_menu = MenuItem::Create(MenuItem::Type::kPopup, "&File");
  auto recent_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Open Recent");
  auto zar_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Zar Package");
  FillRecentlyLaunchedTitlesMenu(recent_menu.get());
  {
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Open...", "Ctrl+O",
                         std::bind(&EmulatorWindow::FileOpen, this)));
    file_menu->AddChild(std::move(recent_menu));
    file_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Install Content...",
                         std::bind(&EmulatorWindow::InstallContent, this)));
    zar_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Create",
                         std::bind(&EmulatorWindow::CreateZarchive, this)));
    zar_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Extract",
                         std::bind(&EmulatorWindow::ExtractZarchive, this)));
    file_menu->AddChild(std::move(zar_menu));
#ifdef DEBUG
    file_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Close",
                         std::bind(&EmulatorWindow::FileClose, this)));
#endif  // #ifdef DEBUG
    file_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    file_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Show content directory...",
        std::bind(&EmulatorWindow::ShowContentDirectory, this)));
    file_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Toggle &Launcher", "F9",
                         std::bind(&EmulatorWindow::ToggleLauncher, this)));
    file_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "E&xit", "Alt+F4",
                         [this]() { window_->RequestClose(); }));
  }
  main_menu->AddChild(std::move(file_menu));

  auto library_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Library");
  {
    library_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Toggle &launcher", "F9",
                         std::bind(&EmulatorWindow::ToggleLauncher, this)));
    library_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Add game &folder...",
        std::bind(&EmulatorWindow::ToggleLibrarySettingsDialog, this)));
    library_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Rescan library", [this]() {
          if (game_library_) {
            game_library_->StartScan();
          }
        }));
    library_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    library_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Manage folders...",
        std::bind(&EmulatorWindow::ToggleLibrarySettingsDialog, this)));
  }
  main_menu->AddChild(std::move(library_menu));

  auto settings_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Settings");
  {
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Preferences...", "Ctrl+,",
        std::bind(&EmulatorWindow::ToggleGeneralSettings, this)));
    settings_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&General...",
        std::bind(&EmulatorWindow::ToggleGeneralSettings, this)));
    settings_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Graphics...", "F7",
                         std::bind(&EmulatorWindow::TogglePreferencesDialog,
                                   this, PreferencesTab::kGraphics, false)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Video && Display...", "F6",
        std::bind(&EmulatorWindow::TogglePreferencesDialog, this,
                  PreferencesTab::kVideoDisplay, false)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Audio...",
        std::bind(&EmulatorWindow::ToggleAudioSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Input...",
        std::bind(&EmulatorWindow::ToggleInputSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Storage && &Paths...",
        std::bind(&EmulatorWindow::ToggleStorageSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "CPU && &System...",
        std::bind(&EmulatorWindow::ToggleCpuSystemSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Logging...",
        std::bind(&EmulatorWindow::ToggleLoggingSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Ad&vanced...",
        std::bind(&EmulatorWindow::ToggleAdvancedSettings, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Game Overrides...",
        std::bind(&EmulatorWindow::ToggleGameOverridesSettings, this)));
    settings_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    settings_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Xbox console...",
                         std::bind(&EmulatorWindow::TogglePreferencesDialog,
                                   this, PreferencesTab::kXboxConsole, false)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Profile...",
        std::bind(&EmulatorWindow::ToggleProfilesConfigDialog, this)));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&XMP...",
        std::bind(&EmulatorWindow::ToggleXMPConfigDialog, this)));
    settings_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    settings_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Open &config file...",
        std::bind(&EmulatorWindow::OpenConfigFileLocation, this)));
  }
  main_menu->AddChild(std::move(settings_menu));

  auto tools_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Tools");
  {
    tools_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Reset Time Scalar", "Numpad *",
        std::bind(&EmulatorWindow::CpuTimeScalarReset, this)));
    tools_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Time Scalar /= 2", "Numpad -",
        std::bind(&EmulatorWindow::CpuTimeScalarSetHalf, this)));
    tools_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Time Scalar *= 2", "Numpad +",
        std::bind(&EmulatorWindow::CpuTimeScalarSetDouble, this)));
    tools_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    tools_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Toggle controller vibration", "",
        std::bind(&EmulatorWindow::ToggleControllerVibration, this)));
    tools_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Display controller hotkeys", "",
        std::bind(&EmulatorWindow::DisplayHotKeysConfig, this)));
    tools_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    tools_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Fullscreen", "F11",
                         std::bind(&EmulatorWindow::ToggleFullscreen, this)));
    tools_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Take &Screenshot", "F12",
                         std::bind(&EmulatorWindow::TakeScreenshot, this)));
  }
  main_menu->AddChild(std::move(tools_menu));

  // Netplay menu.
  auto netplay_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Netplay");
  {
    netplay_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Status", "",
        std::bind(&EmulatorWindow::ToggleNetplayStatusDialog, this)));
    netplay_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Settings", "",
        std::bind(&EmulatorWindow::ToggleNetplaySettingsDialog, this)));
    netplay_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&Manager", "",
        std::bind(&EmulatorWindow::ToggleFriendsDialog, this)));
  }
  main_menu->AddChild(std::move(netplay_menu));

  // Help menu.
  auto help_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Help");
  {
    help_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "FA&Q...", "F1",
                         std::bind(&EmulatorWindow::ShowFAQ, this)));
    help_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    help_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "Game &compatibility...",
                         std::bind(&EmulatorWindow::ShowCompatibility, this)));
    help_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    help_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Build commit on GitHub...", "F2",
        std::bind(&EmulatorWindow::ShowBuildCommit, this)));
    help_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "Recent changes on GitHub...", []() {
          LaunchWebBrowser(
              "https://github.com/xenia-canary/xenia-canary/"
              "compare/" XE_BUILD_COMMIT "..." XE_BUILD_BRANCH);
        }));
    help_menu->AddChild(MenuItem::Create(MenuItem::Type::kSeparator));
    help_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, "&About...",
        []() { LaunchWebBrowser("https://xenia.jp/about/"); }));
  }
  main_menu->AddChild(std::move(help_menu));
  main_menu->AddChild(CreateDevMenu());

  window_->SetMainMenu(std::move(main_menu));

  window_->SetMainMenuEnabled(false);

  UpdateTitle();

  if (!window_->Open()) {
    XELOGE("Failed to open the platform window");
    return false;
  }

  Profiler::SetUserIO(kZOrderProfiler, window_.get(), nullptr, nullptr);

  return true;
}

const char* EmulatorWindow::GetCvarValueForSwapPostEffect(
    gpu::CommandProcessor::SwapPostEffect effect) {
  switch (effect) {
    case gpu::CommandProcessor::SwapPostEffect::kFxaa:
      return "fxaa";
    case gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme:
      return "fxaa_extreme";
    default:
      return "";
  }
}

gpu::CommandProcessor::SwapPostEffect
EmulatorWindow::GetSwapPostEffectForCvarValue(const std::string& cvar_value) {
  if (cvar_value == GetCvarValueForSwapPostEffect(
                        gpu::CommandProcessor::SwapPostEffect::kFxaa)) {
    return gpu::CommandProcessor::SwapPostEffect::kFxaa;
  }
  if (cvar_value == GetCvarValueForSwapPostEffect(
                        gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme)) {
    return gpu::CommandProcessor::SwapPostEffect::kFxaaExtreme;
  }
  return gpu::CommandProcessor::SwapPostEffect::kNone;
}

const char* EmulatorWindow::GetCvarValueForGuestOutputPaintEffect(
    ui::Presenter::GuestOutputPaintConfig::Effect effect) {
  switch (effect) {
    case ui::Presenter::GuestOutputPaintConfig::Effect::kCas:
      return "cas";
    case ui::Presenter::GuestOutputPaintConfig::Effect::kFsr:
      return "fsr";
    default:
      return "";
  }
}

ui::Presenter::GuestOutputPaintConfig::Effect
EmulatorWindow::GetGuestOutputPaintEffectForCvarValue(
    const std::string& cvar_value) {
  if (cvar_value == GetCvarValueForGuestOutputPaintEffect(
                        ui::Presenter::GuestOutputPaintConfig::Effect::kCas)) {
    return ui::Presenter::GuestOutputPaintConfig::Effect::kCas;
  }
  if (cvar_value == GetCvarValueForGuestOutputPaintEffect(
                        ui::Presenter::GuestOutputPaintConfig::Effect::kFsr)) {
    return ui::Presenter::GuestOutputPaintConfig::Effect::kFsr;
  }
  return ui::Presenter::GuestOutputPaintConfig::Effect::kBilinear;
}

ui::Presenter::GuestOutputPaintConfig
EmulatorWindow::GetGuestOutputPaintConfigForCvars() {
  ui::Presenter::GuestOutputPaintConfig paint_config;
  paint_config.SetAllowOverscanCutoff(true);
  paint_config.SetEffect(GetGuestOutputPaintEffectForCvarValue(
      cvars::postprocess_scaling_and_sharpening));
  paint_config.SetCasAdditionalSharpness(
      float(cvars::postprocess_ffx_cas_additional_sharpness));
  paint_config.SetFsrMaxUpsamplingPasses(
      cvars::postprocess_ffx_fsr_max_upsampling_passes);
  paint_config.SetFsrSharpnessReduction(
      float(cvars::postprocess_ffx_fsr_sharpness_reduction));
  paint_config.SetDither(cvars::postprocess_dither);
  return paint_config;
}

void EmulatorWindow::ApplyDisplayConfigForCvars() {
  gpu::GraphicsSystem* graphics_system = emulator_->graphics_system();
  if (!graphics_system) {
    return;
  }

  gpu::CommandProcessor* command_processor =
      graphics_system->command_processor();
  if (command_processor) {
    command_processor->SetDesiredSwapPostEffect(
        GetSwapPostEffectForCvarValue(cvars::postprocess_antialiasing));
  }

  ui::Presenter* presenter = graphics_system->presenter();
  if (presenter) {
    presenter->SetGuestOutputPaintConfigFromUIThread(
        GetGuestOutputPaintConfigForCvars());
  }
}

void EmulatorWindow::OnKeyDown(ui::KeyEvent& e) {
  if (!emulator_initialized_) {
    return;
  }

  switch (e.virtual_key()) {
    case ui::VirtualKey::kO: {
      if (!e.is_ctrl_pressed()) {
        return;
      }
      FileOpen();
    } break;
    case ui::VirtualKey::kMultiply: {
      CpuTimeScalarReset();
    } break;
    case ui::VirtualKey::kSubtract: {
      CpuTimeScalarSetHalf();
    } break;
    case ui::VirtualKey::kAdd: {
      CpuTimeScalarSetDouble();
    } break;

    case ui::VirtualKey::kF3: {
      Profiler::ToggleDisplay();
    } break;

    case ui::VirtualKey::kF4: {
      GpuTraceFrame();
    } break;
    case ui::VirtualKey::kF5: {
      GpuClearCaches();
    } break;

    case ui::VirtualKey::kOemComma: {
      if (e.is_ctrl_pressed()) {
        ToggleGeneralSettings();
      }
    } break;
    case ui::VirtualKey::kF6: {
      TogglePreferencesDialog(PreferencesTab::kVideoDisplay, false);
    } break;
    case ui::VirtualKey::kF7: {
      if (e.prev_state()) {
        break;
      }
#ifdef DEBUG
      if (e.is_ctrl_pressed()) {
        emulator()->SaveToFile("test.sav");
        break;
      }
#endif
      ToggleGraphicsSettingsDialogFromKeyboard();
    } break;
    case ui::VirtualKey::kF11: {
      ToggleFullscreen();
    } break;
    case ui::VirtualKey::kF12: {
      TakeScreenshot();
    } break;

    case ui::VirtualKey::kEscape: {
      // Allow users to escape fullscreen (but not enter it).
      if (!window_->IsFullscreen()) {
        return;
      }
      SetFullscreen(false);
    } break;

#ifdef DEBUG
    case ui::VirtualKey::kF8: {
      // Restore from file
      // TODO: Choose path from user
      // TODO: Spawn a new thread to do this.
      emulator()->RestoreFromFile("test.sav");
    } break;
#endif  // #ifdef DEBUG

    case ui::VirtualKey::kPause: {
      CpuBreakIntoDebugger();
    } break;
    case ui::VirtualKey::kCancel: {
      CpuBreakIntoHostDebugger();
    } break;

    case ui::VirtualKey::kF1: {
      ShowFAQ();
    } break;

    case ui::VirtualKey::kF2: {
      ShowBuildCommit();
    } break;

    case ui::VirtualKey::kF9: {
      ToggleLauncher();
    } break;

    case ui::VirtualKey::kTab: {
      if (emulator_->is_title_open()) {
        ToggleGuideOverlay();
      }
    } break;

    default:
      return;
  }

  e.set_handled(true);
}

void EmulatorWindow::OnMouseDown(const ui::MouseEvent& e) {
  if (imgui_drawer_->IsAnyDialogOpen()) {
    return;
  }

  if (e.button() == ui::MouseEvent::Button::kLeft) {
    ToggleFullscreenOnDoubleClick();
  }
}

void EmulatorWindow::OnMouseUp(const ui::MouseEvent& e) {
  last_mouse_up = steady_clock::now();
}

void EmulatorWindow::TakeScreenshot() {
  xe::ui::RawImage image;

  imgui_drawer_->EnableNotifications(false);

  if (!GetGraphicsSystemPresenter()->CaptureGuestOutput(image) ||
      GetGraphicsSystemPresenter() == nullptr) {
    XELOGE("Failed to capture guest output for screenshot");
    return;
  }

  imgui_drawer_->EnableNotifications(true);
  ExportScreenshot(image);
}

void EmulatorWindow::ExportScreenshot(const xe::ui::RawImage& image) {
  auto t = std::time(nullptr);

  // The format is: Year-Month-DayTHours-Minutes-Seconds based off ISO 8601
  std::string datetime =
      fmt::format("{:%Y-%m-%dT%H-%M-%S}", *std::localtime(&t));

  // Get the title id of the game because some titles contain characters that
  // cannot be used as a directory
  std::string title_id;
  if (emulator()->title_id()) {
    title_id = fmt::format("{:08X}", emulator()->title_id());
  } else {
    XELOGE("Failed to get the current title id");
    return;
  }

  // Find where xenia.exe or xenia_canary.exe is located and create a
  // screenshots folder
  auto screenshot_path =
      (xe::filesystem::GetExecutableFolder() / "screenshots" / title_id);

  if (!std::filesystem::exists(screenshot_path)) {
    std::filesystem::create_directories(screenshot_path);
  }

  std::string filename = fmt::format("{} - {}.png", title_id, datetime);
  SaveImage(screenshot_path / filename, image);

  const std::string notification_text =
      fmt::format("Screenshot saved: {}", filename);

  app_context_.CallInUIThread([&, notification_text]() {
    new xe::ui::HostNotificationWindow(imgui_drawer(), "Screenshot Created!",
                                       notification_text, 0);
  });
}

// Converts a RawImage into a PNG file
void EmulatorWindow::SaveImage(const std::filesystem::path& filepath,
                               const xe::ui::RawImage& image) {
  auto file = std::ofstream(filepath, std::ios::binary);
  if (!file.is_open()) {
    XELOGE("Failed to open file for writing: {}", filepath);
    return;
  }

  auto result = stbi_write_png_to_func(
      [](void* context, void* data, int size) {
        auto file = reinterpret_cast<std::ofstream*>(context);
        file->write(reinterpret_cast<const char*>(data), size);
      },
      &file, image.width, image.height, 4, image.data.data(),
      (int)image.stride);
  if (result == 0) {
    XELOGE("Failed to write PNG to file: {}", filepath);
    return;
  }
}

void EmulatorWindow::ToggleFullscreenOnDoubleClick() {
  if (cvars::disable_doubleclick_fullscreen) {
    return;
  }

  // this function tests if user has double clicked.
  // if double click was achieved the fullscreen gets toggled
  const auto now = steady_clock::now();  // current mouse event time
  constexpr int16_t mouse_down_max_threshold = 250;
  constexpr int16_t mouse_up_max_threshold = 250;
  constexpr int16_t mouse_up_down_max_delta = 100;
  // max delta to prevent 'chaining' of double clicks with next mouse events

  const auto last_mouse_down_delta = diff_in_ms(now, last_mouse_down);
  if (last_mouse_down_delta >= mouse_down_max_threshold) {
    last_mouse_down = now;
    return;
  }

  const auto last_mouse_up_delta = diff_in_ms(now, last_mouse_up);
  const auto mouse_event_deltas = diff_in_ms(last_mouse_up, last_mouse_down);
  if (last_mouse_up_delta >= mouse_up_max_threshold) {
    return;
  }

  if (mouse_event_deltas < mouse_up_down_max_delta) {
    ToggleFullscreen();
  }
}

void EmulatorWindow::FileDrop(const std::filesystem::path& path) {
  if (!emulator_initialized_) {
    return;
  }

  RunTitle(path);
}

void EmulatorWindow::FileOpen() {
  std::filesystem::path path;

  auto file_picker = xe::ui::FilePicker::Create();
  file_picker->set_mode(ui::FilePicker::Mode::kOpen);
  file_picker->set_type(ui::FilePicker::Type::kFile);
  file_picker->set_multi_selection(false);
  file_picker->set_title("Select Content Package");
  file_picker->set_extensions({
      {"Supported Files", "*.iso;*.xex;*.zar;*.*"},
      {"Disc Image (*.iso)", "*.iso"},
      {"Disc Archive (*.zar)", "*.zar"},
      {"Xbox Executable (*.xex)", "*.xex"},
      //{"Content Package (*.xcp)", "*.xcp" },
      {"All Files (*.*)", "*.*"},
  });
  if (file_picker->Show(window_.get())) {
    auto selected_files = file_picker->selected_files();
    if (!selected_files.empty()) {
      path = selected_files[0];
    }
    // Only run the title if a file is selected
    RunTitle(path);
  }
}

void EmulatorWindow::FileClose() {
  if (!emulator_->is_title_open()) {
    XELOGW("FileClose: no title open, showing launcher");
    ShowLauncher();
    return;
  }
  XELOGW("FileClose: posting ResetTitle to lifecycle worker");
  emulator_->PostToLifecycleWorker([em = emulator_]() { em->ResetTitle(); });
}

void EmulatorWindow::InstallContent() {
  std::vector<std::filesystem::path> paths;

  auto file_picker = xe::ui::FilePicker::Create();
  file_picker->set_mode(ui::FilePicker::Mode::kOpen);
  file_picker->set_type(ui::FilePicker::Type::kFile);
  file_picker->set_multi_selection(true);
  file_picker->set_title("Select Content Package");
  file_picker->set_extensions({
      {"All Files (*.*)", "*.*"},
  });
  if (file_picker->Show(window_.get())) {
    paths = file_picker->selected_files();
  }

  if (paths.empty()) {
    return;
  }

  std::shared_ptr<std::vector<Emulator::ContentInstallEntry>>
      content_installation_status =
          std::make_shared<std::vector<Emulator::ContentInstallEntry>>();

  for (const auto& path : paths) {
    content_installation_status->push_back({path});
  }

  for (auto& entry : *content_installation_status) {
    emulator_->ProcessContentPackageHeader(entry.path_, entry);
  }

  auto installationThread = std::thread([this, content_installation_status] {
    for (auto& entry : *content_installation_status) {
      emulator_->InstallContentPackage(entry.path_, entry);
    }
  });
  installationThread.detach();

  new ContentInstallDialog(imgui_drawer_.get(), *this,
                           content_installation_status);
}

void EmulatorWindow::ExtractZarchive() {
  std::vector<std::filesystem::path> zarchive_files;
  std::filesystem::path extract_dir;

  auto file_picker = xe::ui::FilePicker::Create();
  file_picker->set_mode(ui::FilePicker::Mode::kOpen);
  file_picker->set_type(ui::FilePicker::Type::kFile);
  file_picker->set_multi_selection(true);
  file_picker->set_title("Select Zar Package");
  file_picker->set_extensions({
      {"Zarchive Files (*.zar)", "*.zar"},
  });

  if (file_picker->Show(window_.get())) {
    zarchive_files = file_picker->selected_files();
  }

  if (zarchive_files.empty()) {
    return;
  }

  file_picker->set_type(ui::FilePicker::Type::kDirectory);
  file_picker->set_title("Select Directory to Extract");

  if (file_picker->Show(window_.get())) {
    extract_dir = file_picker->selected_files().front();
  }

  if (extract_dir.empty()) {
    return;
  }

  std::string extract_overview = "";

  for (auto& zarchive_file_path : zarchive_files) {
    extract_overview += "\n" + path_to_utf8(zarchive_file_path);
  }

  app_context_.CallInUIThread([&]() {
    new xe::ui::HostNotificationWindow(imgui_drawer(), "Extracting...",
                                       string_util::trim(extract_overview), 0);
  });

  auto run = [this, extract_dir, zarchive_files]() -> void {
    std::string summary = "";

    for (auto& zarchive_file_path : zarchive_files) {
      // Normalize the path and make absolute.
      auto abs_path = std::filesystem::absolute(zarchive_file_path);
      std::filesystem::path abs_extract_dir;

      if (zarchive_files.size() > 1) {
        abs_extract_dir =
            std::filesystem::absolute((extract_dir / abs_path.stem()));
      } else {
        abs_extract_dir = std::filesystem::absolute(extract_dir);
      }

      XELOGI("Extracting zar package: {}\n",
             zarchive_file_path.filename().string());

      auto result =
          emulator_->ExtractZarchivePackage(abs_path, abs_extract_dir);

      if (result != X_STATUS_SUCCESS) {
        std::error_code ec;

        if (!std::filesystem::is_empty(abs_extract_dir)) {
          std::filesystem::remove(abs_extract_dir, ec);
        }

        summary += fmt::format("\nFailed: {}", zarchive_file_path);

        XELOGE("Failed to extract Zarchive package.", result);
      } else {
        summary += fmt::format("\nSuccess: {}", abs_extract_dir);
      }
    }

    new xe::ui::HostNotificationWindow(imgui_drawer(), "Zar Extraction Summary",
                                       string_util::trim(summary), 0);
  };

  auto zarThread = std::thread(run);
  zarThread.detach();
}

void EmulatorWindow::CreateZarchive() {
  std::vector<std::filesystem::path> content_dirs;
  std::filesystem::path zarchive_dir;

  auto file_picker = xe::ui::FilePicker::Create();
  file_picker->set_mode(ui::FilePicker::Mode::kOpen);
  file_picker->set_type(ui::FilePicker::Type::kDirectory);
  file_picker->set_multi_selection(true);
  file_picker->set_title("Select Contents");

  if (file_picker->Show(window_.get())) {
    content_dirs = file_picker->selected_files();
  }

  if (content_dirs.empty()) {
    return;
  }

  if (content_dirs.size() == 1) {
    file_picker->set_mode(ui::FilePicker::Mode::kSave);
    file_picker->set_type(ui::FilePicker::Type::kFile);
    file_picker->set_multi_selection(false);
    file_picker->set_file_name(content_dirs.front().filename().string());
    file_picker->set_default_extension("zar");
    file_picker->set_title("Zarchive File");
    file_picker->set_extensions({
        {"Zarchive File (*.zar)", "*.zar"},
    });
  } else {
    file_picker->set_title("Output Directory");
  }

  if (file_picker->Show(window_.get())) {
    zarchive_dir = file_picker->selected_files().front();
  }

  if (zarchive_dir.empty()) {
    return;
  }

  std::string create_overview = "";

  std::map<std::filesystem::path, std::filesystem::path> zarchive_files{};

  for (auto& content_path : content_dirs) {
    // Normalize the path and make absolute.
    auto abs_content_dir = std::filesystem::absolute(content_path);
    std::filesystem::path abs_zarchive_file;

    if (content_dirs.size() > 1) {
      abs_zarchive_file = std::filesystem::absolute(
          (zarchive_dir / abs_content_dir.filename().concat(".zar")));
    } else {
      abs_zarchive_file = std::filesystem::absolute(zarchive_dir);
    }

    zarchive_files[content_path] = abs_zarchive_file;

    create_overview += "\n" + path_to_utf8(abs_zarchive_file);
  }

  app_context_.CallInUIThread([&]() {
    new xe::ui::HostNotificationWindow(imgui_drawer(), "Creating...",
                                       string_util::trim(create_overview), 0);
  });

  auto run = [this, zarchive_files]() -> void {
    std::string summary = "";

    for (auto const& [content_path, zarchive_file] : zarchive_files) {
      // Normalize the path and make absolute.
      auto abs_content_dir = std::filesystem::absolute(content_path);

      XELOGI("Creating zar package: {}\n", zarchive_file.filename().string());

      auto result =
          emulator_->CreateZarchivePackage(abs_content_dir, zarchive_file);

      if (result != X_ERROR_SUCCESS) {
        std::error_code ec;

        // delete incomplete output file
        std::filesystem::remove(zarchive_file, ec);

        summary += fmt::format("\nFailed: {}", abs_content_dir);

        XELOGE("Failed to create Zarchive package.", result);
      } else {
        summary += fmt::format("\nSuccess: {}", zarchive_file);
      }
    }

    new xe::ui::HostNotificationWindow(imgui_drawer(), "Zar Creation Summary",
                                       string_util::trim(summary), 0);
  };

  auto zarThread = std::thread(run);
  zarThread.detach();
}

void EmulatorWindow::ShowContentDirectory() {
  auto content_root = emulator_->content_root();

  if (!std::filesystem::exists(content_root)) {
    std::filesystem::create_directories(content_root);
  }

  LaunchFileExplorer(content_root);
}

void EmulatorWindow::CpuTimeScalarReset() {
  Clock::set_guest_time_scalar(1.0);
  UpdateTitle();
}

void EmulatorWindow::CpuTimeScalarSetHalf() {
  Clock::set_guest_time_scalar(Clock::guest_time_scalar() / 2.0);
  UpdateTitle();
}

void EmulatorWindow::CpuTimeScalarSetDouble() {
  Clock::set_guest_time_scalar(Clock::guest_time_scalar() * 2.0);
  UpdateTitle();
}

void EmulatorWindow::CpuBreakIntoDebugger() {
  if (!cvars::debug) {
    xe::ui::ImGuiDialog::ShowMessageBox(imgui_drawer_.get(), "Xenia Debugger",
                                        "Xenia must be launched with the "
                                        "--debug flag in order to enable "
                                        "debugging.");
    return;
  }
  auto processor = emulator()->processor();
  if (processor->execution_state() == cpu::ExecutionState::kRunning) {
    // Currently running, so interrupt (and show the debugger).
    processor->Pause();
  } else {
    // Not running, so just bring the debugger into focus.
    processor->ShowDebugger();
  }
}

void EmulatorWindow::CpuBreakIntoHostDebugger() { xe::debugging::Break(); }

void EmulatorWindow::GpuTraceFrame() {
  emulator()->graphics_system()->RequestFrameTrace();
}

void EmulatorWindow::GpuClearCaches() {
  emulator()->graphics_system()->ClearCaches();
}

void EmulatorWindow::SetFullscreen(bool fullscreen_) {
  if (window_->IsFullscreen() == fullscreen_) {
    return;
  }

  OVERRIDE_bool(fullscreen, fullscreen_);

  window_->SetFullscreen(fullscreen_);
  window_->SetCursorVisibility(fullscreen_
                                   ? ui::Window::CursorVisibility::kAutoHidden
                                   : ui::Window::CursorVisibility::kVisible);
}

void EmulatorWindow::ToggleFullscreen() {
  SetFullscreen(!window_->IsFullscreen());
}

void EmulatorWindow::SetAutoCheckForUpdates(bool state) {
  OVERRIDE_bool(auto_check_updates, state);
}

void EmulatorWindow::UpdateCompletionNotification() {
  app_context_.CallInUIThread([&]() {
    const std::string message = fmt::format(
        "Build Date: {} ({})", XE_BUILD_DATE, XE_BUILD_COMMIT_SHORT);
    new xe::ui::HostNotificationWindow(imgui_drawer(), "Update Completed",
                                       message, 0, 9);
  });
}

void EmulatorWindow::ScheduleClosePreferencesDialog() {
  if (!preferences_dialog_ || preferences_close_pending_) {
    return;
  }
  preferences_close_pending_ = true;
  PreferencesDialog* const dialog = preferences_dialog_.get();
  app_context_.CallInUIThreadDeferred([this, dialog]() {
    preferences_close_pending_ = false;
    if (preferences_dialog_.get() != dialog) {
      return;
    }
    preferences_dialog_.reset();
  });
}

void EmulatorWindow::ScheduleCloseLibrarySettingsDialog() {
  if (!library_settings_dialog_ || library_settings_close_pending_) {
    return;
  }
  library_settings_close_pending_ = true;
  LibrarySettingsDialog* const dialog = library_settings_dialog_.get();
  app_context_.CallInUIThreadDeferred([this, dialog]() {
    library_settings_close_pending_ = false;
    if (library_settings_dialog_.get() == dialog) {
      library_settings_dialog_.reset();
    }
  });
}

void EmulatorWindow::OnLibrarySettingsDialogClosed(ui::ImGuiDialog* dialog) {
  if (library_settings_dialog_.get() == dialog) {
    library_settings_dialog_.release();
  }
}

void EmulatorWindow::TogglePreferencesDialog(PreferencesTab tab,
                                             bool per_game_mode) {
  preferences_close_pending_ = false;
  if (!preferences_dialog_) {
    preferences_open_tab_ = tab;
    preferences_per_game_mode_ = per_game_mode;
    preferences_dialog_ =
        std::make_unique<PreferencesDialog>(imgui_drawer_.get(), *this, tab);
    preferences_dialog_->SetPerGameMode(per_game_mode);
  } else {
    preferences_dialog_->SetInitialTab(tab);
    preferences_dialog_->SetPerGameMode(per_game_mode);
  }
}

void EmulatorWindow::ToggleDisplayConfigDialog() {
  TogglePreferencesDialog(PreferencesTab::kVideoDisplay, false);
}

void EmulatorWindow::ToggleAudioSettings() {
  TogglePreferencesDialog(PreferencesTab::kAudio, false);
}

void EmulatorWindow::ToggleInputSettings() {
  TogglePreferencesDialog(PreferencesTab::kInput, false);
}

void EmulatorWindow::ToggleStorageSettings() {
  TogglePreferencesDialog(PreferencesTab::kStorage, false);
}

void EmulatorWindow::ToggleGeneralSettings() {
  TogglePreferencesDialog(PreferencesTab::kGeneral, false);
}

void EmulatorWindow::ToggleCpuSystemSettings() {
  TogglePreferencesDialog(PreferencesTab::kCpuSystem, false);
}

void EmulatorWindow::ToggleLoggingSettings() {
  TogglePreferencesDialog(PreferencesTab::kLogging, false);
}

void EmulatorWindow::ToggleAdvancedSettings() {
  TogglePreferencesDialog(PreferencesTab::kAdvanced, false);
}

void EmulatorWindow::ToggleGameOverridesSettings() {
  TogglePreferencesDialog(PreferencesTab::kGameOverrides, true);
}

void EmulatorWindow::OpenConfigFileLocation() {
  if (!config::config_path.empty() &&
      std::filesystem::exists(config::config_path)) {
    LaunchFileExplorer(config::config_path);
  } else if (!config::config_folder.empty()) {
    LaunchFileExplorer(config::config_folder);
  }
}

void EmulatorWindow::ToggleProfilesConfigDialog() {
  if (!profile_config_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    profile_config_dialog_ =
        std::make_unique<ProfileConfigDialog>(imgui_drawer_.get(), this);
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (profile_config_dialog_->IsClosing()) {
      profile_config_dialog_.release();
    } else {
      profile_config_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleGamerpicBrowserDialog() {
  if (!gamerpic_browser_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    gamerpic_browser_dialog_ =
        TitleGamerpicBrowser::Create(imgui_drawer_.get(), this);
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (gamerpic_browser_dialog_->IsClosing()) {
      gamerpic_browser_dialog_.release();
    } else {
      gamerpic_browser_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleXMPConfigDialog() {
  if (!xmp_config_dialog_) {
    xmp_config_dialog_ = std::unique_ptr<XMPConfigDialog>(
        new XMPConfigDialog(imgui_drawer_.get(), *this));
  } else {
    xmp_config_dialog_.reset();
  }
}

void EmulatorWindow::ToggleGraphicsSettingsDialogFromKeyboard() {
#if XE_PLATFORM_WIN32
  if (auto* win32_window = dynamic_cast<ui::Win32Window*>(window_.get())) {
    win32_window->PostUiTask([this]() {
      TogglePreferencesDialog(PreferencesTab::kGraphics, false);
    });
    return;
  }
#endif  // XE_PLATFORM_WIN32
  app_context_.CallInUIThreadDeferred(
      [this]() { TogglePreferencesDialog(PreferencesTab::kGraphics, false); });
}

void EmulatorWindow::ToggleGraphicsSettingsDialog() {
  TogglePreferencesDialog(PreferencesTab::kGraphics, false);
}

void EmulatorWindow::ToggleLibrarySettingsDialog() {
  if (!game_library_) {
    return;
  }
  library_settings_close_pending_ = false;
  if (!library_settings_dialog_) {
    library_settings_dialog_ = std::make_unique<LibrarySettingsDialog>(
        imgui_drawer_.get(), *this, *game_library_);
  } else {
    library_settings_dialog_.reset();
  }
}

void EmulatorWindow::ShowLauncher() {
  if (!game_library_) {
    return;
  }
  // Belt-and-braces guard against a relaunch race: if the GraphicsSystem
  // hasn't rebound a Presenter to our window yet, constructing the dashboard
  // dialog would call ImGuiDrawer::AddDialog → Presenter::AddUIDrawerFromUI
  // Thread on a dangling presenter pointer. Defer; SetupGraphicsSystem
  // PresenterPainting will re-call ShowLauncher() once everything is wired
  // up (immediate_drawer_ being non-null is the canonical "presenter is
  // bound to ImGuiDrawer" signal — set in SetupGraphicsSystemPresenterPainting,
  // cleared in ShutdownGraphicsSystemPresenterPainting).
  if (!immediate_drawer_) {
    XELOGW(
        "ShowLauncher: deferring — presenter not yet bound to ImGuiDrawer "
        "(UI thread={}, already_shown={})",
        app_context_.IsInUIThread(), launcher_dashboard_ != nullptr);
    return;
  }
  const bool overlay = emulator_->is_title_open();
  XELOGW("ShowLauncher (UI thread={}, already_shown={}, overlay={})",
         app_context_.IsInUIThread(), launcher_dashboard_ != nullptr, overlay);
  if (!launcher_dashboard_) {
    launcher_dashboard_ = std::make_unique<LauncherDashboardDialog>(
        imgui_drawer_.get(), *this, *game_library_,
        ResolvePatchesRoot(emulator_->storage_root()));
    launcher_dashboard_->SetOnOpenLibrarySettings(
        std::bind(&EmulatorWindow::ToggleLibrarySettingsDialog, this));
    launcher_dashboard_->SetOnConfigureTitle([this](uint32_t /*title_id*/) {
      TogglePreferencesDialog(PreferencesTab::kGraphics, true);
    });
    launcher_dashboard_->SetOnOpenPreferences(
        std::bind(&EmulatorWindow::TogglePreferencesDialog, this,
                  PreferencesTab::kGraphics, false));
    launcher_dashboard_->SetOnOpenNetplaySettings(
        std::bind(&EmulatorWindow::ToggleNetplaySettingsDialog, this));
    launcher_dashboard_->SetOnOpenFriends(
        std::bind(&EmulatorWindow::ToggleFriendsDialog, this));
    launcher_dashboard_->SetOnOpenGuide(
        std::bind(&EmulatorWindow::ToggleGuideOverlay, this));
    launcher_dashboard_->SetOnManageProfiles(
        std::bind(&EmulatorWindow::ToggleProfilesConfigDialog, this));
  }
}

std::filesystem::path EmulatorWindow::ResolvePatchesRoot(
    const std::filesystem::path& storage_root) {
  std::error_code ec;
  auto has_patches = [&](const std::filesystem::path& root) {
    return std::filesystem::is_directory(root / "patches", ec);
  };
  if (has_patches(storage_root)) {
    return storage_root;
  }
  const std::filesystem::path candidates[] = {
      storage_root / "game-patches",
      storage_root / ".." / "game-patches",
      storage_root / ".." / ".." / "game-patches",
      storage_root / ".." / ".." / ".." / "game-patches",
  };
  for (const auto& candidate : candidates) {
    const auto normalized = candidate.lexically_normal();
    if (has_patches(normalized)) {
      return normalized;
    }
  }
  return storage_root;
}

void EmulatorWindow::ToggleGuideOverlay() {
  if (!guide_overlay_) {
    guide_overlay_ =
        std::make_unique<GuideOverlayDialog>(imgui_drawer_.get(), *this);
    guide_overlay_->SetOpen(true);
  } else {
    const bool next = !guide_overlay_->IsUserOpen();
    guide_overlay_->SetOpen(next);
    if (!next) {
      guide_overlay_.reset();
    }
  }
}

void EmulatorWindow::AddGameLibraryFolder(const std::filesystem::path& path) {
  if (!game_library_ || path.empty()) {
    return;
  }
  game_library_->AddWatchedDirectory(path);
  game_library_->Save();
  game_library_->StartScan();
}

void EmulatorWindow::HideLauncher() {
  XELOGW("HideLauncher (UI thread={}, present={}, overlay_requested={})",
         app_context_.IsInUIThread(), launcher_dashboard_ != nullptr,
         launcher_overlay_requested_);
  launcher_dashboard_.reset();
}

void EmulatorWindow::ToggleLauncher() {
  if (launcher_dashboard_) {
    launcher_overlay_requested_ = false;
    HideLauncher();
    return;
  }
  launcher_overlay_requested_ = true;
  ShowLauncher();
}

void EmulatorWindow::ToggleConsoleSettingsDialog() {
  if (!console_settings_dialog_) {
    console_settings_dialog_ =
        std::unique_ptr<ConsoleSettingsDialog>(new ConsoleSettingsDialog(
            imgui_drawer_.get(), *this, emulator_->kernel_state()->xconfig()));
  } else {
    if (console_settings_dialog_->IsClosing()) {
      console_settings_dialog_.release();
    } else {
      console_settings_dialog_.reset();
    }
  }
}

void EmulatorWindow::ToggleFriendsDialog() {
  if (!friends_manager_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    friends_manager_dialog_ =
        std::make_unique<ManagerDialog>(imgui_drawer_.get(), this);
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (friends_manager_dialog_->IsClosing()) {
      friends_manager_dialog_.release();
    } else {
      friends_manager_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleNetplaySettingsDialog() {
  if (!netplay_settings_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    netplay_settings_dialog_ = std::make_unique<NetplaySettingsDialog>(
        imgui_drawer_.get(), this, emulator_->GetNetworkAdapterManager());
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (netplay_settings_dialog_->IsClosing()) {
      netplay_settings_dialog_.release();
    } else {
      netplay_settings_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleNetplayStatusDialog() {
  if (!netplay_status_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    netplay_status_dialog_ = std::make_unique<NetplayStatusDialog>(
        imgui_drawer_.get(), this, emulator_->GetNetworkAdapterManager());
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (netplay_status_dialog_->IsClosing()) {
      netplay_status_dialog_.release();
    } else {
      netplay_status_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleUpdaterDialog() {
  if (!updater_dialog_) {
    const bool auto_check_update =
        update_info_.valid() ? update_info_.get().update_available : false;

    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    updater_dialog_ = std::make_unique<UpdaterDialog>(
        updater_, auto_check_update, imgui_drawer_.get(), this);
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (updater_dialog_->IsClosing()) {
      updater_dialog_.release();
    } else {
      updater_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleCompletionDialog() {
  if (!updater_completion_dialog_) {
    disable_hotkeys_ = true;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 1);
    updater_completion_dialog_ = std::make_unique<UpdaterCompletionDialog>(
        imgui_drawer_.get(), this, false);
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_++;
  } else {
    disable_hotkeys_ = false;
    emulator_->kernel_state()->BroadcastNotification(kXNotificationSystemUI, 0);
    if (updater_completion_dialog_->IsClosing()) {
      updater_completion_dialog_.release();
    } else {
      updater_completion_dialog_.reset();
    }
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }
}

void EmulatorWindow::ToggleControllerVibration() {
  auto input_sys = emulator()->input_system();
  if (input_sys) {
    auto input_lock = input_sys->lock();

    input_sys->ToggleVibration();

    if (emulator_->kernel_state()) {
      emulator_->kernel_state()->BroadcastNotification(
          kXNotificationSystemProfileSettingChanged,
          static_cast<uint32_t>(input_sys->GetConnectedSlots().count()));
    }
  }
}

void EmulatorWindow::ShowCompatibility() {
  const std::string_view base_url =
      "https://github.com/xenia-canary/game-compatibility/issues";
  std::string url;
  // Avoid searching for a title ID of "00000000".
  uint32_t title_id = emulator_->title_id();
  if (!title_id) {
    url = base_url;
  } else {
    url = fmt::format("{}?q=is%3Aissue+is%3Aopen+{:08X}", base_url, title_id);
  }
  LaunchWebBrowser(url);
}

void EmulatorWindow::ShowFAQ() {
  LaunchWebBrowser("https://github.com/xenia-canary/xenia-canary/wiki/FAQ");
}

void EmulatorWindow::ShowBuildCommit() {
#ifdef XE_BUILD_IS_PR
  LaunchWebBrowser(
      "https://github.com/xenia-canary/xenia-canary/pull/" XE_BUILD_PR_NUMBER);
#else
  LaunchWebBrowser(
      "https://github.com/xenia-canary/xenia-canary/commit/" XE_BUILD_COMMIT);
#endif
}

void EmulatorWindow::UpdateTitle() {
  xe::StringBuffer sb;
  sb.Append(base_title_);

  // Title information, if available
  if (emulator()->is_title_open()) {
    sb.AppendFormat(" | [{:08X}", emulator()->title_id());
    auto title_version = emulator()->title_version();
    if (!title_version.empty()) {
      sb.Append(" v");
      sb.Append(title_version);
    }
    sb.Append("]");

    auto title_name = emulator()->title_name();
    if (!title_name.empty()) {
      sb.Append(" ");
      sb.Append(title_name);
    }
  }

  // Graphics system name, if available
  auto graphics_system = emulator()->graphics_system();
  if (graphics_system) {
    auto graphics_name = graphics_system->name();
    if (!graphics_name.empty()) {
      sb.Append(" <");
      sb.Append(graphics_name);
      sb.Append(">");
    }
  }

  if (Clock::guest_time_scalar() != 1.0) {
    sb.AppendFormat(" (@{:.2f}x)", Clock::guest_time_scalar());
  }

  if (initializing_shader_storage_) {
    sb.Append(" (Preloading shaders\u2026)");
  }

  patcher::Patcher* patcher = emulator()->patcher();
  if (patcher && patcher->IsAnyPatchApplied()) {
    sb.Append(" [Patches Applied]");
  }

  patcher::PluginLoader* pluginloader = emulator()->plugin_loader();
  if (pluginloader && pluginloader->IsAnyPluginLoaded()) {
    sb.Append(" [Plugins Loaded]");
  }

  if (dev_mode_active_) {
    sb.Append(" [DEV]");
  }

  window_->SetTitle(sb.to_string_view());
}

void EmulatorWindow::SetInitializingShaderStorage(bool initializing) {
  if (initializing_shader_storage_ == initializing) {
    return;
  }
  initializing_shader_storage_ = initializing;
  UpdateTitle();
}

// Notes:
// SDL and XInput both support the guide button
//
// Assumes titles do not use the guide button.
// For titles that do such as dashboards these titles could be excluded based on
// their title ID.
//
// Xbox Gamebar:
// If the Xbox Gamebar overlay is enabled Windows will consume the guide
// button's input, this can be seen using hid-demo.
//
// Workaround: Detect if the Xbox Gamebar overlay is enabled then use the BACK
// button instead of the GUIDE button. Therefore BACK and GUIDE are reserved
// buttons for hotkeys.
//
// This is not an issue with DualShock controllers because Windows will not
// open the gamebar overlay using the PlayStation menu button.
//
// Xbox One S Controller:
// The guide button on this controller is very buggy no idea why.
// Using xinput usually registers after a double tap.
// Doesn't work at all using SDL.
// Needs more testing.
//
// Steam:
// If guide button focus is enabled steam will open.
// Steam uses BACK + GUIDE to open an On-Screen keyboard, however this is not a
// problem since both these buttons are reserved.
const std::map<int, EmulatorWindow::ControllerHotKey> controller_hotkey_map = {
    // Must use the Guide Button for all pass through hotkeys
    {X_INPUT_GAMEPAD_A | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ReadbackResolve,
         "A + Guide = Toggle Readback Resolve", true)},
    {X_INPUT_GAMEPAD_B | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ToggleLogging,
         "B + Guide = Toggle between loglevel set in config and the 'Disabled' "
         "loglevel.",
         true, true)},
    {X_INPUT_GAMEPAD_Y | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ToggleFullscreen,
         "Y + Guide = Toggle Fullscreen", true)},
    {X_INPUT_GAMEPAD_X | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ClearMemoryPageState,
         "X + Guide = Toggle Clear Memory Page State", true)},

    {X_INPUT_GAMEPAD_RIGHT_SHOULDER | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ClearGPUCache,
         "Right Shoulder + Guide = Clear GPU Cache", true)},
    {X_INPUT_GAMEPAD_LEFT_SHOULDER | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ToggleControllerVibration,
         "Left Shoulder + Guide = Toggle Controller Vibration", true)},

    // CPU Time Scalar with no rumble feedback
    {X_INPUT_GAMEPAD_DPAD_DOWN | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::CpuTimeScalarSetHalf,
         "D-PAD Down + Guide = Half CPU Scalar")},
    {X_INPUT_GAMEPAD_DPAD_UP | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::CpuTimeScalarSetDouble,
         "D-PAD Up + Guide = Double CPU Scalar")},
    {X_INPUT_GAMEPAD_DPAD_RIGHT | X_INPUT_GAMEPAD_GUIDE,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::CpuTimeScalarReset,
         "D-PAD Right + Guide = Reset CPU Scalar")},

    // non-pass through hotkeys
    {X_INPUT_GAMEPAD_Y, EmulatorWindow::ControllerHotKey(
                            EmulatorWindow::ButtonFunctions::ToggleFullscreen,
                            "Y = Toggle Fullscreen", true, false)},
    {X_INPUT_GAMEPAD_START, EmulatorWindow::ControllerHotKey(
                                EmulatorWindow::ButtonFunctions::RunTitle,
                                "Start = Run Selected Title", false, false)},
    {X_INPUT_GAMEPAD_BACK | X_INPUT_GAMEPAD_START,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::ToggleLogging,
         "Back + Start = Toggle between loglevel set in config and the "
         "'Disabled' loglevel.",
         false, false)},
    {X_INPUT_GAMEPAD_DPAD_DOWN,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::IncTitleSelect,
         "D-PAD Down = Title Selection +1", true, false)},
    {X_INPUT_GAMEPAD_DPAD_UP,
     EmulatorWindow::ControllerHotKey(
         EmulatorWindow::ButtonFunctions::DecTitleSelect,
         "D-PAD Up = Title Selection -1", true, false)}};

EmulatorWindow::ControllerHotKey EmulatorWindow::ProcessControllerHotkey(
    int buttons) {
  // Default return value
  EmulatorWindow::ControllerHotKey Unknown_hotkey = {};

  if (buttons == 0) {
    return Unknown_hotkey;
  }

  if (disable_hotkeys_.load()) {
    return Unknown_hotkey;
  }

  // Hotkey cool-down to prevent toggling too fast
  constexpr std::chrono::milliseconds delay(75);

  // If the Xbox Gamebar is enabled or the Guide button is disabled then
  // replace the Guide button with the Back button without redeclaring the key
  // mappings
  if (IsUseNexusForGameBarEnabled() || !cvars::guide_button) {
    if ((buttons & X_INPUT_GAMEPAD_BACK) == X_INPUT_GAMEPAD_BACK) {
      buttons &= ~X_INPUT_GAMEPAD_BACK;
      buttons |= X_INPUT_GAMEPAD_GUIDE;
    }
  }

  auto it = controller_hotkey_map.find(buttons);
  if (it == controller_hotkey_map.end()) {
    return Unknown_hotkey;
  }

  // Do not activate hotkeys that are not intended for activation during
  // gameplay
  if (emulator_->is_title_open()) {
    // If non-pass through (menu hoykeys) or hotkeys disabled then return
    if (!it->second.title_passthru || !cvars::controller_hotkeys) {
      return Unknown_hotkey;
    }
  }

  std::string notificationTitle = "";
  std::string notificationDesc = "";

  EmulatorWindow::ControllerHotKey button_combination = it->second;

  switch (button_combination.function) {
    case ButtonFunctions::ToggleFullscreen:
      app_context().CallInUIThread([this]() { ToggleFullscreen(); });

      // Extra Sleep
      xe::threading::Sleep(delay);
      break;
    case ButtonFunctions::RunTitle: {
      if (selected_title_index == -1) {
        selected_title_index++;
      }

      if (selected_title_index < recently_launched_titles_.size()) {
        app_context().CallInUIThread([this]() {
          RunTitle(
              recently_launched_titles_[selected_title_index].path_to_file);
        });
      }
    } break;
    case ButtonFunctions::ClearMemoryPageState:
      ToggleGPUSetting(GPUSetting::ClearMemoryPageState);

      // Assume the user wants ClearCaches as well
      if (cvars::clear_memory_page_state) {
        GpuClearCaches();
      }

      notificationTitle = "Toggle Clear Memory Page State";
      notificationDesc =
          cvars::clear_memory_page_state ? "Enabled" : "Disabled";

      // Extra Sleep
      xe::threading::Sleep(delay);
      break;
    case ButtonFunctions::ReadbackResolve:
      CycleReadbackResolve();

      notificationTitle = "Readback Resolve Mode";
      notificationDesc = cvars::readback_resolve;

      // Extra Sleep
      xe::threading::Sleep(delay);
      break;
    case ButtonFunctions::CpuTimeScalarSetHalf:
      CpuTimeScalarSetHalf();

      notificationTitle = "Time Scalar";
      notificationDesc =
          fmt::format("Decreased to {}", Clock::guest_time_scalar());
      break;
    case ButtonFunctions::CpuTimeScalarSetDouble:
      CpuTimeScalarSetDouble();

      notificationTitle = "Time Scalar";
      notificationDesc =
          fmt::format("Increased to {}", Clock::guest_time_scalar());
      break;
    case ButtonFunctions::CpuTimeScalarReset:
      CpuTimeScalarReset();

      notificationTitle = "Time Scalar";
      notificationDesc = fmt::format("Reset to {}", Clock::guest_time_scalar());
      break;
    case ButtonFunctions::ClearGPUCache:
      GpuClearCaches();

      notificationTitle = "Clear GPU Cache";
      notificationDesc = "Complete";

      // Extra Sleep
      xe::threading::Sleep(delay);
      break;
    case ButtonFunctions::ToggleControllerVibration: {
      ToggleControllerVibration();

      bool vibration = false;

      auto input_sys = emulator()->input_system();
      if (input_sys) {
        vibration = input_sys->GetVibrationCvar();
      }

      notificationTitle = "Toggle Controller Vibration";
      notificationDesc = vibration ? "Enabled" : "Disabled";

      // Extra Sleep
      xe::threading::Sleep(delay);
    } break;
    case ButtonFunctions::IncTitleSelect:
      selected_title_index++;
      break;
    case ButtonFunctions::DecTitleSelect:
      selected_title_index--;
      break;
    case ButtonFunctions::ToggleLogging: {
      logging::ToggleLogLevel();

      notificationTitle = "Toggle Logging";

      LogLevel level = static_cast<LogLevel>(logging::internal::GetLogLevel());
      notificationDesc = level == LogLevel::Disabled ? "Disabled" : "Enabled";
    } break;
    case ButtonFunctions::Unknown:
    default:
      break;
  }

  if ((button_combination.function == ButtonFunctions::IncTitleSelect ||
       button_combination.function == ButtonFunctions::DecTitleSelect) &&
      recently_launched_titles_.size() > 0) {
    selected_title_index =
        std::clamp(selected_title_index, 0,
                   static_cast<int32_t>(recently_launched_titles_.size() - 1));

    // Must clear dialogs to prevent stacking
    ClearDialogs();

    // Titles may contain Unicode characters such as At World’s End
    // Must use ImGUI font that can render these Unicode characters
    std::string title_name;

    // Use filename if title name is empty
    if (recently_launched_titles_[selected_title_index].title_name.empty()) {
      title_name = recently_launched_titles_[selected_title_index]
                       .path_to_file.filename()
                       .string();
    } else {
      title_name = recently_launched_titles_[selected_title_index].title_name;
    }

    std::string title = fmt::format(
        "{}: {}\n\n{}", selected_title_index + 1, title_name,
        controller_hotkey_map.find(X_INPUT_GAMEPAD_START)->second.pretty);

    xe::ui::ImGuiDialog::ShowMessageBox(imgui_drawer_.get(), "Title Selection",
                                        title);
  }

  if (!notificationTitle.empty()) {
    app_context_.CallInUIThread(
        [imgui_drawer = imgui_drawer(), notificationTitle, notificationDesc]() {
          new xe::ui::HostNotificationWindow(imgui_drawer, notificationTitle,
                                             notificationDesc, 0);
        });
  }

  xe::threading::Sleep(delay);

  return it->second;
}

void EmulatorWindow::VibrateController(xe::hid::InputSystem* input_sys,
                                       uint32_t user_index,
                                       bool toggle_rumble) {
  constexpr std::chrono::milliseconds rumble_duration(100);

  // Hold lock while sleeping this thread for the duration of the rumble,
  // otherwise the rumble may fail.
  auto input_lock = input_sys->lock();

  X_INPUT_VIBRATION vibration = {};

  vibration.left_motor_speed = toggle_rumble ? UINT16_MAX : 0;
  vibration.right_motor_speed = toggle_rumble ? UINT16_MAX : 0;

  input_sys->SetState(user_index, &vibration);

  // Vibration duration
  if (toggle_rumble) {
    xe::threading::Sleep(rumble_duration);
  }
}

void EmulatorWindow::GamepadHotKeys() {
  X_INPUT_STATE state;

  constexpr std::chrono::milliseconds thread_delay(75);

  auto input_sys = emulator_->input_system();

  if (input_sys) {
    while (hotkeys_listener_running_) {
      // Collect controller states while holding the lock
      std::array<std::pair<bool, X_INPUT_STATE>, XUserMaxUserCount>
          controller_states;
      {
        auto input_lock = input_sys->lock();
        for (uint32_t user_index = 0; user_index < XUserMaxUserCount;
             ++user_index) {
          X_RESULT result = input_sys->GetState(
              user_index, X_INPUT_FLAG::X_INPUT_FLAG_GAMEPAD, &state);
          controller_states[user_index] = {result == X_ERROR_SUCCESS, state};
        }
      }  // Lock is released here when input_lock goes out of scope

      // Process hotkeys without holding the lock
      for (uint32_t user_index = 0; user_index < XUserMaxUserCount;
           ++user_index) {
        if (controller_states[user_index].first) {
          if (ProcessControllerHotkey(
                  controller_states[user_index].second.gamepad.buttons)
                  .rumble) {
            // Enable Vibration
            VibrateController(input_sys, user_index, true);

            // Disable Vibration
            VibrateController(input_sys, user_index, false);
          }
        }
      }

      xe::threading::Sleep(thread_delay);
    }
  }
}

void EmulatorWindow::ToggleGPUSetting(gpu::GPUSetting setting) {
  switch (setting) {
    case GPUSetting::ClearMemoryPageState:
      SaveGPUSetting(GPUSetting::ClearMemoryPageState,
                     !cvars::clear_memory_page_state);
      break;
    case GPUSetting::ReadbackMemexport:
      SaveGPUSetting(GPUSetting::ReadbackMemexport, !cvars::readback_memexport);
      break;
  }
}

void EmulatorWindow::CycleReadbackResolve() {
  const std::string& current = cvars::readback_resolve;
  if (current == "fast") {
    gpu::SetReadbackResolveMode("full");
  } else if (current == "full") {
    gpu::SetReadbackResolveMode("none");
  } else {
    gpu::SetReadbackResolveMode("fast");
  }
}

void EmulatorWindow::DisplayHotKeysConfig() {
  std::string msg = "";
  std::string msg_passthru = "";

  bool guide_enabled = !IsUseNexusForGameBarEnabled() && cvars::guide_button;

  for (auto const& [key, val] : controller_hotkey_map) {
    std::string pretty_text = val.pretty;

    if (!guide_enabled) {
      pretty_text = std::regex_replace(
          pretty_text, std::regex("Guide", std::regex_constants::icase),
          "Back");
    }

    if (emulator_->is_title_open() && !val.title_passthru) {
      pretty_text += " (Disabled)";
    }

    if (val.title_passthru && !cvars::controller_hotkeys) {
      pretty_text += " (Disabled)";
    }

    if (val.title_passthru) {
      msg += pretty_text + "\n";
    } else {
      msg_passthru += pretty_text + "\n";
    }
  }

  // Add Title
  msg.insert(0, "Gameplay Hotkeys\n");

  // Prepend non-passthru hotkeys
  msg_passthru += "\n";
  msg.insert(0, msg_passthru);
  msg += "\n";

  msg += "Readback Resolve: " + cvars::readback_resolve;
  msg += "\n";

  msg += "Clear Memory Page State: " +
         xe::string_util::BoolToString(cvars::clear_memory_page_state);
  msg += "\n";

  msg += "Controller Hotkeys: " +
         xe::string_util::BoolToString(cvars::controller_hotkeys);

  ClearDialogs();
  xe::ui::ImGuiDialog::ShowMessageBox(imgui_drawer_.get(), "Controller Hotkeys",
                                      msg);
}

std::string EmulatorWindow::CanonicalizeFileExtension(
    const std::filesystem::path& path) {
  return xe::utf8::lower_ascii(xe::path_to_utf8(path.extension()));
}

xe::X_STATUS EmulatorWindow::DispatchLaunch(LaunchRequest request) {
  if (!title_launch_dispatcher_) {
    title_launch_dispatcher_ =
        std::make_unique<TitleLaunchDispatcher>(*emulator_, *this);
  }
  return title_launch_dispatcher_->Dispatch(std::move(request));
}

xe::X_STATUS EmulatorWindow::RunTitle(
    const std::filesystem::path& path_to_file) {
  std::filesystem::path launch_path = path_to_file;
#if XE_PLATFORM_ANDROID
  if (!launch_path.empty() &&
      xe::filesystem::IsAndroidContentUri(xe::path_to_utf8(launch_path))) {
    launch_path = xe::filesystem::StageAndroidLaunchPath(launch_path);
  }
#endif

  std::error_code ec = {};
  const bool titleExists =
      !launch_path.empty() && std::filesystem::exists(launch_path, ec);

  if (launch_path.empty() || !titleExists) {
    std::string log_msg =
        fmt::format("Failed to launch title path is {}.",
                    launch_path.empty() ? "empty" : "invalid");

    if (!launch_path.empty() && !titleExists) {
      log_msg.append(fmt::format("\nProvided Path: {}", launch_path));
    }

    if (ec) {
      log_msg.append(fmt::format("\nExtended message info: {} ({:08X})",
                                 ec.message(), ec.value()));
    }

    XELOGE("{}", log_msg);

    ClearDialogs();

    xe::ui::ImGuiDialog::ShowMessageBox(imgui_drawer_.get(),
                                        "Title Launch Failed!", log_msg);

    return X_STATUS_NO_SUCH_FILE;
  }

  bool launch_expected = false;
  if (!title_launch_in_progress_.compare_exchange_strong(launch_expected,
                                                         true)) {
    return X_STATUS_UNSUCCESSFUL;
  }
  struct LaunchInProgressReset {
    std::atomic<bool>& flag;
    ~LaunchInProgressReset() { flag.store(false); }
  } launch_in_progress_reset{title_launch_in_progress_};

  auto abs_path = std::filesystem::absolute(launch_path);

  auto extension = CanonicalizeFileExtension(abs_path);

  if (extension == ".7z" || extension == ".zip" || extension == ".rar" ||
      extension == ".tar" || extension == ".gz") {
    xe::ShowSimpleMessageBox(
        xe::SimpleMessageBoxType::Error,
        fmt::format(
            "Unsupported format!\n"
            "Xenia does not support running software in an archived format."));

    return X_STATUS_UNSUCCESSFUL;
  }

  LaunchRequest req;
  req.path = abs_path;
  req.source = LaunchRequestSource::kLibrary;
  req.kind = emulator_->is_title_open() ? LaunchRequestKind::kRelaunch
                                        : LaunchRequestKind::kNewTitle;

  auto result = DispatchLaunch(std::move(req));

  disable_hotkeys_ = false;

  if (profile_config_dialog_) {
    profile_config_dialog_.reset();
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_--;
  }

  if (preferences_dialog_) {
    preferences_dialog_.reset();
  }

  ClearDialogs();

  if (result) {
    XELOGE("Failed to launch target: {:08X} ({})", result,
           xe::path_to_utf8(abs_path));

    std::string message =
        "Failed to launch title.\n\nCheck xenia.log for technical details.";
    if (result == X_STATUS_NOT_SUPPORTED) {
      message = fmt::format(
          "Failed to launch title (unsupported or incomplete path).\n\n"
          "Path:\n{}\n\n"
          "If the path contains spaces, quote it when launching from a shell "
          "(for example \"D:\\Xbox 360\\Games\\title.iso\"), or use "
          "phoenixctl / launch_bf2_triage.ps1.",
          xe::path_to_utf8(abs_path));
    }

    xe::ui::ImGuiDialog::ShowMessageBox(imgui_drawer_.get(),
                                        "Title Launch Failed!", message);

    emulator_->file_system()->Clear();
    if (emulator_->lifecycle_state() == LifecycleState::Ready) {
      ShowLauncher();
    }
  } else {
    AddRecentlyLaunchedTitle(path_to_file, emulator_->title_name());

    if (game_library_) {
      game_library_->RecordPlay(launch_path, emulator_->title_id(),
                                emulator_->title_name());

      if (auto* db = emulator_->game_info_database()) {
        auto* entry = game_library_->FindByPath(launch_path);
        if (entry && entry->custom_icon_path.empty() &&
            entry->icon_path.empty()) {
          auto icon = db->GetIcon();
          if (!icon.empty()) {
            game_library_->CacheIcon(emulator_->title_id(), icon);
          }
        }
      }
    }

    auto xam =
        emulator_->kernel_state()->GetKernelModule<kernel::xam::XamModule>(
            "xam.xex");

    xam->loader_data().host_path = xe::path_to_utf8(abs_path);
  }

  return result;
}

void EmulatorWindow::RequestLaunchTitle(
    const std::filesystem::path& path_to_file) {
#if XE_PLATFORM_WIN32
  if (auto* win32_window = dynamic_cast<ui::Win32Window*>(window_.get())) {
    win32_window->PostUiTask(
        [this, path_to_file]() { RunTitle(path_to_file); });
    return;
  }
#endif  // XE_PLATFORM_WIN32
  app_context_.CallInUIThreadDeferred(
      [this, path_to_file]() { RunTitle(path_to_file); });
}

void EmulatorWindow::RunPreviouslyPlayedTitle() {
  if (recently_launched_titles_.size() >= 1) {
    RunTitle(recently_launched_titles_[0].path_to_file);
  }
}

void EmulatorWindow::FillRecentlyLaunchedTitlesMenu(
    xe::ui::MenuItem* recent_menu) {
  for (int i = 0; i < recently_launched_titles_.size(); ++i) {
    std::string hotkey = (i == 0) ? "F9" : "";

    const RecentTitleEntry& entry = recently_launched_titles_[i];
    const std::string item_text = entry.title_name.empty()
                                      ? entry.path_to_file.string()
                                      : entry.title_name;

    recent_menu->AddChild(MenuItem::Create(
        MenuItem::Type::kString, item_text, hotkey,
        std::bind(&EmulatorWindow::RunTitle, this, entry.path_to_file)));
  }
}

void EmulatorWindow::LoadRecentlyLaunchedTitles() {
  std::ifstream file(emulator()->storage_root() /
                     kRecentlyPlayedTitlesFilename);
  if (!file.is_open()) {
    return;
  }

  toml::parse_result parsed_file;
  try {
    parsed_file = toml::parse(file);
  } catch (toml::parse_error& exception) {
    XELOGE("Cannot parse file: recent.toml. Error: {}", exception.what());
    return;
  }

  if (parsed_file.is_table()) {
    for (const auto& [index, entry] : *parsed_file.as_table()) {
      if (!entry.is_table()) {
        continue;
      }

      const toml::table* entry_table = entry.as_table();

      std::string title_name =
          entry_table->get_as<std::string>("title_name")->get();
      std::string path = entry_table->get_as<std::string>("path")->get();
      std::time_t last_run_time =
          entry_table->get_as<int64_t>("last_run_time")->get();

      std::error_code ec = {};
      if (path.empty() || !std::filesystem::exists(path, ec)) {
        continue;
      }

      recently_launched_titles_.push_back({title_name, path, last_run_time});
    }
  }
}

void EmulatorWindow::AddRecentlyLaunchedTitle(
    std::filesystem::path path_to_file, std::string title_name) {
  if (cvars::recent_titles_entry_amount <= 0) {
    return;
  }

  // Check if game is already on list and pop it to front
  auto entry_index = std::find_if(recently_launched_titles_.cbegin(),
                                  recently_launched_titles_.cend(),
                                  [&title_name](const RecentTitleEntry& entry) {
                                    return entry.title_name == title_name;
                                  });
  if (entry_index != recently_launched_titles_.cend()) {
    recently_launched_titles_.erase(entry_index);
  }

  recently_launched_titles_.insert(recently_launched_titles_.cbegin(),
                                   {title_name, path_to_file, time(nullptr)});
  // Serialize to toml
  auto toml_table = toml::table();

  uint8_t index = 0;
  for (const RecentTitleEntry& entry : recently_launched_titles_) {
    auto entry_table = toml::table();

    // Fill entry under specific index.
    std::string str_path = xe::path_to_utf8(entry.path_to_file);
    entry_table.insert("title_name", entry.title_name);
    entry_table.insert("path", str_path);
    entry_table.insert("last_run_time", entry.last_run_time);

    toml_table.insert(std::to_string(index++), entry_table);

    if (index >= cvars::recent_titles_entry_amount) {
      break;
    }
  }
  // Open and write serialized data.
  std::ofstream file(emulator()->storage_root() / kRecentlyPlayedTitlesFilename,
                     std::ofstream::trunc);
  file << toml_table;
  file.close();
}

void EmulatorWindow::ClearDialogs() {
  if (profile_config_dialog_) {
    profile_config_dialog_.reset();
  }

  if (gamerpic_browser_dialog_) {
    gamerpic_browser_dialog_.reset();
  }

  if (preferences_dialog_) {
    preferences_dialog_.reset();
  }
  if (library_settings_dialog_) {
    library_settings_dialog_.reset();
  }
  if (console_settings_dialog_) {
    console_settings_dialog_.reset();
  }
  if (netplay_settings_dialog_) {
    netplay_settings_dialog_.reset();
  }
  if (netplay_status_dialog_) {
    netplay_status_dialog_.reset();
  }
  if (friends_manager_dialog_) {
    friends_manager_dialog_.reset();
  }
  if (updater_dialog_) {
    updater_dialog_.reset();
  }
  if (updater_completion_dialog_) {
    updater_completion_dialog_.reset();
  }

  imgui_drawer_.get()->ClearDialogs();
  if (emulator_->kernel_state() && emulator_->kernel_state()->xam_state()) {
    emulator_->kernel_state()->xam_state()->xam_dialogs_shown_ = 0;
  }
}

void EmulatorWindow::ClearMenuChildren(ui::MenuItem* menu, size_t& item_count) {
  while (item_count > 0) {
    if (ui::MenuItem* child = menu->child(0)) {
      menu->RemoveChild(child);
    }
    --item_count;
  }
  item_count = 0;
}

std::unique_ptr<ui::MenuItem> EmulatorWindow::CreateDevMenu() {
  auto dev_menu = ui::MenuItem::Create(ui::MenuItem::Type::kPopup, "&Dev");

  {
    auto item =
        ui::MenuItem::Create(ui::MenuItem::Type::kString, "  Dev Mode (off)",
                             std::bind(&EmulatorWindow::ToggleDevMode, this));
    dev_mode_menu_item_ = item.get();
    dev_menu->AddChild(std::move(item));
  }

  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));

  log_level_submenu_ =
      ui::MenuItem::Create(ui::MenuItem::Type::kPopup, "Log &Level").release();
  dev_menu->AddChild(log_level_submenu_);
  RebuildLogLevelSubmenu();

  log_channels_submenu_ =
      ui::MenuItem::Create(ui::MenuItem::Type::kPopup, "Log &Channels")
          .release();
  dev_menu->AddChild(log_channels_submenu_);
  RebuildLogChannelsSubmenu();

  {
    auto item = ui::MenuItem::Create(
        ui::MenuItem::Type::kString, "[ ] Force Flush on Error/Warning",
        std::bind(&EmulatorWindow::ToggleForceFlushOnError, this));
    force_flush_menu_item_ = item.get();
    dev_menu->AddChild(std::move(item));
  }
  RebuildForceFlushMenuItem();

  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));

#if XE_OPTION_PROFILING
  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kString,
                                          "Toggle Profiler &Display", "F3",
                                          []() { Profiler::ToggleDisplay(); }));
  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kString,
                                          "&Pause/Resume Profiler", "`",
                                          []() { Profiler::TogglePause(); }));
  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));
#endif

  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "GPU &Trace Frame", "F4",
      std::bind(&EmulatorWindow::GpuTraceFrame, this)));
  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "GPU &Clear Runtime Caches", "F5",
      std::bind(&EmulatorWindow::GpuClearCaches, this)));

  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));

  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "&Break and Show Guest Debugger",
      "Pause/Break", std::bind(&EmulatorWindow::CpuBreakIntoDebugger, this)));
  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "&Break into Host Debugger",
      "Ctrl+Pause/Break",
      std::bind(&EmulatorWindow::CpuBreakIntoHostDebugger, this)));

  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));

  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "Dump Host Thread State Now",
      "Ctrl+Shift+F6",
      std::bind(&EmulatorWindow::DumpHostThreadStateNow, this)));
  dev_menu->AddChild(
      ui::MenuItem::Create(ui::MenuItem::Type::kString, "Force Flush Log Now",
                           std::bind(&EmulatorWindow::ForceFlushLogNow, this)));

  dev_menu->AddChild(ui::MenuItem::Create(ui::MenuItem::Type::kSeparator));

  dev_menu->AddChild(
      ui::MenuItem::Create(ui::MenuItem::Type::kString, "Open &Log Folder...",
                           std::bind(&EmulatorWindow::OpenLogFolder, this)));
  dev_menu->AddChild(ui::MenuItem::Create(
      ui::MenuItem::Type::kString, "Open Latest &Crash Report...",
      std::bind(&EmulatorWindow::OpenLatestCrashReport, this)));

  RebuildDevModeMenuItem();

  return dev_menu;
}

void EmulatorWindow::RebuildDevModeMenuItem() {
  if (!dev_mode_menu_item_) {
    return;
  }
  dev_mode_menu_item_->set_text(dev_mode_active_ ? "* Dev Mode (on)"
                                                 : "  Dev Mode (off)");
  window_->CompleteMainMenuItemsUpdate();
}

void EmulatorWindow::RebuildLogLevelSubmenu() {
  if (!log_level_submenu_) {
    return;
  }
  ClearMenuChildren(log_level_submenu_, log_level_submenu_items_);

  const auto add_level = [&](int level, const char* label) {
    const bool selected = ::cvars::log_level == level;
    const std::string text = std::string(selected ? "* " : "  ") + label;
    log_level_submenu_->AddChild(
        ui::MenuItem::Create(ui::MenuItem::Type::kString, text,
                             [this, level]() { SetLogLevel(level); }));
    ++log_level_submenu_items_;
  };

  add_level(0, "Error");
  add_level(1, "Warning");
  add_level(2, "Info");
  add_level(3, "Debug");

  window_->CompleteMainMenuItemsUpdate();
}

void EmulatorWindow::RebuildLogChannelsSubmenu() {
  if (!log_channels_submenu_) {
    return;
  }
  ClearMenuChildren(log_channels_submenu_, log_channels_submenu_items_);

  const auto add_channel = [&](uint32_t bit, const char* label) {
    const bool enabled = (::cvars::log_mask & bit) == 0;
    const std::string text = std::string(enabled ? "[x] " : "[ ] ") + label;
    log_channels_submenu_->AddChild(
        ui::MenuItem::Create(ui::MenuItem::Type::kString, text,
                             [this, bit]() { ToggleLogChannel(bit); }));
    ++log_channels_submenu_items_;
  };

  add_channel(xe::LogSrc::Kernel, "Kernel");
  add_channel(xe::LogSrc::Apu, "APU");
  add_channel(xe::LogSrc::Cpu, "CPU");
  add_channel(xe::LogSrc::Gpu, "GPU");

  window_->CompleteMainMenuItemsUpdate();
}

void EmulatorWindow::RebuildForceFlushMenuItem() {
  if (!force_flush_menu_item_) {
    return;
  }
  const bool enabled =
      diagnostics().force_flush_on_error.load(std::memory_order_relaxed);
  force_flush_menu_item_->set_text(enabled
                                       ? "[x] Force Flush on Error/Warning"
                                       : "[ ] Force Flush on Error/Warning");
  window_->CompleteMainMenuItemsUpdate();
}

void EmulatorWindow::ToggleDevMode() {
  if (!dev_mode_active_) {
    dev_mode_snapshot_.log_level = ::cvars::log_level;
    dev_mode_snapshot_.log_mask = ::cvars::log_mask;
    dev_mode_snapshot_.flush_log = ::cvars::flush_log;
    dev_mode_snapshot_.force_flush_on_error =
        ::cvars::diag_force_flush_on_error;
    dev_mode_snapshot_.subsystem_sentinels = ::cvars::diag_subsystem_sentinels;
    dev_mode_snapshot_.capture_log_tail_on_crash =
        ::cvars::diag_capture_log_tail_on_crash;

    ::cvars::log_level = 3;
    ::cvars::log_mask = 0;
    ::cvars::flush_log = true;
    ::cvars::diag_force_flush_on_error = true;
    ::cvars::diag_subsystem_sentinels = true;
    ::cvars::diag_capture_log_tail_on_crash = true;
    ::cvars::dev_mode = true;

    diagnostics().dev_mode.store(true, std::memory_order_relaxed);
    diagnostics().force_flush_on_error.store(true, std::memory_order_relaxed);
    diagnostics().subsystem_sentinels.store(true, std::memory_order_relaxed);
    diagnostics().capture_log_tail_on_crash.store(true,
                                                  std::memory_order_relaxed);

    dev_mode_active_ = true;
    XELOGW("Dev Mode enabled (log_level=Debug, all channels, sentinels on)");
  } else {
    ::cvars::log_level = dev_mode_snapshot_.log_level;
    ::cvars::log_mask = dev_mode_snapshot_.log_mask;
    ::cvars::flush_log = dev_mode_snapshot_.flush_log;
    ::cvars::diag_force_flush_on_error =
        dev_mode_snapshot_.force_flush_on_error;
    ::cvars::diag_subsystem_sentinels = dev_mode_snapshot_.subsystem_sentinels;
    ::cvars::diag_capture_log_tail_on_crash =
        dev_mode_snapshot_.capture_log_tail_on_crash;
    ::cvars::dev_mode = false;

    diagnostics().dev_mode.store(false, std::memory_order_relaxed);
    diagnostics().force_flush_on_error.store(
        dev_mode_snapshot_.force_flush_on_error, std::memory_order_relaxed);
    diagnostics().subsystem_sentinels.store(
        dev_mode_snapshot_.subsystem_sentinels, std::memory_order_relaxed);
    diagnostics().capture_log_tail_on_crash.store(
        dev_mode_snapshot_.capture_log_tail_on_crash,
        std::memory_order_relaxed);

    dev_mode_active_ = false;
    XELOGW("Dev Mode disabled (restored prior logging settings)");
  }

  RebuildDevModeMenuItem();
  RebuildLogLevelSubmenu();
  RebuildLogChannelsSubmenu();
  RebuildForceFlushMenuItem();
  UpdateTitle();
}

void EmulatorWindow::SetLogLevel(int32_t level) {
  ::cvars::log_level = level;
  XELOGI("Log level set to {}", level);
  RebuildLogLevelSubmenu();
}

void EmulatorWindow::ToggleLogChannel(uint32_t channel_bit) {
  ::cvars::log_mask = ::cvars::log_mask ^ channel_bit;
  RebuildLogChannelsSubmenu();
}

void EmulatorWindow::ToggleForceFlushOnError() {
  const bool enabled =
      !diagnostics().force_flush_on_error.load(std::memory_order_relaxed);
  ::cvars::diag_force_flush_on_error = enabled;
  diagnostics().force_flush_on_error.store(enabled, std::memory_order_relaxed);
  RebuildForceFlushMenuItem();
}

void EmulatorWindow::DumpHostThreadStateNow() {
#if XE_PLATFORM_WIN32
  XELOGW("--- host thread inventory (manual dump) ---");
  const DWORD pid = GetCurrentProcessId();
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    XELOGW("DumpHostThreadStateNow: CreateToolhelp32Snapshot failed");
    return;
  }

  THREADENTRY32 entry = {};
  entry.dwSize = sizeof(entry);

  using GetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PWSTR*);
  const auto get_thread_description =
      reinterpret_cast<GetThreadDescriptionFn>(GetProcAddress(
          GetModuleHandleW(L"kernel32.dll"), "GetThreadDescription"));

  if (Thread32First(snapshot, &entry)) {
    do {
      if (entry.th32OwnerProcessID != pid) {
        continue;
      }
      HANDLE thread_handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE,
                                        entry.th32ThreadID);
      std::string thread_name = "<unnamed>";
      if (thread_handle) {
        if (get_thread_description) {
          PWSTR description = nullptr;
          if (SUCCEEDED(get_thread_description(thread_handle, &description)) &&
              description) {
            thread_name = xe::to_utf8(std::u16string_view(
                reinterpret_cast<const char16_t*>(description),
                wcslen(description)));
            LocalFree(description);
          }
        }
        CloseHandle(thread_handle);
      }
      XELOGW("[host-thread] tid={:08X} name={}", entry.th32ThreadID,
             thread_name);
    } while (Thread32Next(snapshot, &entry));
  }
  CloseHandle(snapshot);
  xe::FlushLog();
#else
  XELOGW("DumpHostThreadStateNow: not implemented on this platform");
#endif
}

void EmulatorWindow::ForceFlushLogNow() {
  xe::FlushLog();
  XELOGI("Log flushed to disk");
}

void EmulatorWindow::OpenLogFolder() {
  const auto log_dir = xe::GetLogDirectory();
  if (log_dir.empty()) {
    XELOGW("OpenLogFolder: log directory not available yet");
    return;
  }
  xe::LaunchFileExplorer(log_dir);
}

void EmulatorWindow::OpenLatestCrashReport() {
  const auto log_dir = xe::GetLogDirectory();
  if (log_dir.empty()) {
    XELOGW("OpenLatestCrashReport: log directory not available yet");
    return;
  }

  std::filesystem::path latest_path;
  std::filesystem::file_time_type latest_time;
  bool found = false;

  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(log_dir, ec)) {
    if (ec) {
      break;
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto name = entry.path().filename().string();
    if (name.find("_unhandled.txt") == std::string::npos) {
      continue;
    }
    const auto mtime = entry.last_write_time(ec);
    if (!found || mtime > latest_time) {
      latest_time = mtime;
      latest_path = entry.path();
      found = true;
    }
  }

  if (!found) {
    XELOGW("OpenLatestCrashReport: no *_unhandled.txt in {}", log_dir);
    return;
  }

#if XE_PLATFORM_WIN32
  ShellExecuteW(nullptr, L"open", latest_path.c_str(), nullptr, nullptr,
                SW_SHOWNORMAL);
#else
  xe::LaunchFileExplorer(latest_path);
#endif
}

}  // namespace app
}  // namespace xe
