/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/window_mac.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include "xenia/ui/surface_mac.h"
#include "xenia/ui/virtual_key.h"

@interface XeniaMetalView : NSView
@property(nonatomic, assign) xe::ui::MacWindow* xenia_window;
@end

@implementation XeniaMetalView
+ (Class)layerClass {
  return [CAMetalLayer class];
}
- (BOOL)isFlipped {
  return YES;
}
- (BOOL)acceptsFirstResponder {
  return YES;
}
- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if (self.window) {
    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    layer.contentsScale = self.window.backingScaleFactor;
    CGSize size = self.bounds.size;
    layer.drawableSize =
        CGSizeMake(size.width * layer.contentsScale, size.height * layer.contentsScale);
    if (self.xenia_window) {
      self.xenia_window->OnMacResize(uint32_t(layer.drawableSize.width),
                                     uint32_t(layer.drawableSize.height));
    }
  }
}
- (void)setFrameSize:(NSSize)newSize {
  [super setFrameSize:newSize];
  CAMetalLayer* layer = (CAMetalLayer*)self.layer;
  if (layer && self.window) {
    layer.contentsScale = self.window.backingScaleFactor;
    layer.drawableSize = CGSizeMake(newSize.width * layer.contentsScale,
                                    newSize.height * layer.contentsScale);
    if (self.xenia_window) {
      self.xenia_window->OnMacResize(uint32_t(layer.drawableSize.width),
                                     uint32_t(layer.drawableSize.height));
    }
  }
}
- (void)drawRect:(NSRect)dirtyRect {
  if (self.xenia_window) {
    self.xenia_window->OnMacResize(uint32_t(((CAMetalLayer*)self.layer).drawableSize.width),
                                     uint32_t(((CAMetalLayer*)self.layer).drawableSize.height));
  }
}
- (void)keyDown:(NSEvent*)event {
  (void)event;
}
- (void)keyUp:(NSEvent*)event {
  (void)event;
}
- (void)mouseDown:(NSEvent*)event {
  (void)event;
}
- (void)mouseUp:(NSEvent*)event {
  (void)event;
}
- (void)mouseMoved:(NSEvent*)event {
  (void)event;
}
@end

@interface XeniaWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) xe::ui::MacWindow* xenia_window;
@end

@implementation XeniaWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
  (void)sender;
  if (self.xenia_window) {
    self.xenia_window->OnMacClose();
  }
  return YES;
}
- (void)windowDidResize:(NSNotification*)notification {
  (void)notification;
}
- (void)windowDidBecomeKey:(NSNotification*)notification {
  (void)notification;
  if (self.xenia_window) {
    self.xenia_window->OnMacFocus(true);
  }
}
- (void)windowDidResignKey:(NSNotification*)notification {
  (void)notification;
  if (self.xenia_window) {
    self.xenia_window->OnMacFocus(false);
  }
}
@end

namespace xe {
namespace ui {

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context,
                                       const std::string_view title,
                                       uint32_t desired_logical_width,
                                       uint32_t desired_logical_height) {
  return std::make_unique<MacWindow>(app_context, title, desired_logical_width,
                                     desired_logical_height);
}

MacWindow::MacWindow(WindowedAppContext& app_context, const std::string_view title,
                     uint32_t desired_logical_width,
                     uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height) {}

MacWindow::~MacWindow() {
  EnterDestructor();
  if (ns_window_) {
    NSWindow* window = (__bridge_transfer NSWindow*)ns_window_;
    ns_window_ = nullptr;
    metal_view_ = nullptr;
    metal_layer_ = nullptr;
    [window close];
  }
}

bool MacWindow::OpenImpl() {
  NSRect frame =
      NSMakeRect(0, 0, CGFloat(GetDesiredLogicalWidth()), CGFloat(GetDesiredLogicalHeight()));
  NSWindow* window = [[NSWindow alloc]
      initWithContentRect:frame
                styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window setTitle:[NSString stringWithUTF8String:GetTitle().c_str()]];
  [window center];

  XeniaMetalView* view = [[XeniaMetalView alloc] initWithFrame:frame];
  view.wantsLayer = YES;
  view.xenia_window = this;
  [window setContentView:view];

  XeniaWindowDelegate* delegate = [[XeniaWindowDelegate alloc] init];
  delegate.xenia_window = this;
  window.delegate = delegate;

  ns_window_ = (__bridge_retained void*)window;
  metal_view_ = (__bridge void*)view;
  metal_layer_ = (__bridge void*)view.layer;

  [window makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];

  WindowDestructionReceiver destruction_receiver(this);
  CAMetalLayer* layer = (CAMetalLayer*)view.layer;
  OnActualSizeUpdate(uint32_t(layer.drawableSize.width),
                     uint32_t(layer.drawableSize.height), destruction_receiver);
  OnFocusUpdate(true, destruction_receiver);
  return true;
}

void MacWindow::RequestCloseImpl() {
  if (ns_window_) {
    [(NSWindow*)(__bridge id)ns_window_ performClose:nil];
  }
}

void MacWindow::ApplyNewFullscreen() {
  if (!ns_window_) {
    return;
  }
  NSWindow* window = (__bridge NSWindow*)ns_window_;
  if (IsFullscreen()) {
    [window toggleFullScreen:nil];
  }
}

void MacWindow::ApplyNewTitle() {
  if (ns_window_) {
    [(NSWindow*)(__bridge id)ns_window_
        setTitle:[NSString stringWithUTF8String:GetTitle().c_str()]];
  }
}

void MacWindow::ApplyNewMainMenu(MenuItem* old_main_menu) {
  (void)old_main_menu;
}

void MacWindow::FocusImpl() {
  if (ns_window_) {
    [(NSWindow*)(__bridge id)ns_window_ makeKeyAndOrderFront:nil];
  }
}

std::unique_ptr<Surface> MacWindow::CreateSurfaceImpl(
    Surface::TypeFlags allowed_types) {
  if ((allowed_types & Surface::kTypeFlag_MacMetalLayer) && metal_layer_) {
    return std::make_unique<MacMetalLayerSurface>(metal_layer_);
  }
  return nullptr;
}

void MacWindow::RequestPaintImpl() {
  if (metal_view_) {
    [(XeniaMetalView*)(__bridge id)metal_view_ setNeedsDisplay:YES];
  }
}

void MacWindow::OnMacClose() { RequestClose(); }

void MacWindow::OnMacResize(uint32_t width, uint32_t height) {
  WindowDestructionReceiver destruction_receiver(this);
  OnActualSizeUpdate(width, height, destruction_receiver);
}

void MacWindow::OnMacFocus(bool focused) {
  WindowDestructionReceiver destruction_receiver(this);
  OnFocusUpdate(focused, destruction_receiver);
}

std::unique_ptr<ui::MenuItem> MenuItem::Create(Type type, const std::string& text,
                                               const std::string& hotkey,
                                               std::function<void()> callback) {
  return std::make_unique<MacMenuItem>(type, text, hotkey, std::move(callback));
}

}  // namespace ui
}  // namespace xe
