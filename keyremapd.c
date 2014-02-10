// keyremapd.c
// Simple Keyboard Remap Daemon
//
// (c) 2014 Adam Strzelecki
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <AvailabilityMacros.h>
#include <stdio.h>

#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/IOLLEvent.h>

static CFMachPortRef eventTapPort;

// FIXME: it this declared anywhere ?
static const CGEventFlags kControlMask /*******/ = 0x00000001;
static const CGEventFlags kLeftShiftMask /*****/ = 0x00000002;
static const CGEventFlags kRightShiftMask /****/ = 0x00000004;
static const CGEventFlags kLeftCommandMask /***/ = 0x00000008;
static const CGEventFlags kRightCommandMask /**/ = 0x00000010;
static const CGEventFlags kLeftOptionMask /****/ = 0x00000020;
static const CGEventFlags kRightOptionMask /***/ = 0x00000040;

static const CGKeyCode kRightCommandCode /*****/ = 0x36;
static const CGKeyCode kRightOptionCode /******/ = 0x3D;
static const CGKeyCode kKeyPadEnterCode /******/ = 0x4C;

static CGEventRef handleEvent(CGEventTapProxy proxy, CGEventType type,
                              CGEventRef event, void *arg) {
  CGEventFlags flags = CGEventGetFlags(event);

  if (type == kCGEventTapDisabledByTimeout) {
    fprintf(stderr, "kCGEventTapDisabledByTimeout, Enabling Event Tap");
    CGEventTapEnable(eventTapPort, true);
    return event;
  }

  if (type == kCGEventTapDisabledByUserInput) {
    fprintf(stderr, "kCGEventTapDisabledByUserInput, Enabling Event Tap");
    CGEventTapEnable(eventTapPort, true);
    return NULL;
  }

  CGKeyCode keyCode =
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

  if (type == kCGEventFlagsChanged) {
    // map right Option to keypad Enter on press/release
    if (keyCode == kRightOptionCode) {
      CGEventRef newEvent = CGEventCreateKeyboardEvent(
          NULL, kKeyPadEnterCode, (flags & kRightOptionMask));
      CGEventPost(kCGSessionEventTap, newEvent);
      CFRelease(newEvent);
    }
  } else {
    // remap right Command to right Option
    if ((flags & kRightCommandMask) && (flags & kCGEventFlagMaskCommand)) {
      CGEventSetFlags(event,
                      (flags & ~(kCGEventFlagMaskCommand | kRightCommandMask)) |
                          kCGEventFlagMaskAlternate | kRightOptionMask);
    }
  }

  return event;
}

static int installEventTap(void) {
  CFRunLoopSourceRef source;

  eventTapPort = CGEventTapCreate(
      kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
      CGEventMaskBit(kCGEventFlagsChanged) | CGEventMaskBit(kCGEventKeyDown) |
          CGEventMaskBit(kCGEventKeyUp),
      handleEvent, NULL);

  if (eventTapPort == NULL) {
    fprintf(stderr, "error: failed to create event tap\n");
    return 0;
  }

  source = CFMachPortCreateRunLoopSource(NULL, eventTapPort, 0L);

  if (source == NULL) {
    fprintf(stderr, "error: no event src\n");
    return 0;
  }

  CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
  CFRelease(source);

  return 1;
}

int run = 1;

void signalHandler(int sig) {
  switch (sig) {
  case SIGHUP:
  case SIGTERM:
  case SIGINT:
  case SIGQUIT:
    run = 0;
    CFRunLoopStop(CFRunLoopGetCurrent());
    break;
  default:
    fprintf(stderr, "uhandled signal (%d) %s\n", sig, strsignal(sig));
    break;
  }
}

int main(int argc, char const *argv[]) {
  if (!AXIsProcessTrusted()) {
    fprintf(
        stderr,
        "error: enable keyremapd Accessibility in "
        "System Preferences > Security & Privacy > Privacy > Accessibility");
    CFUserNotificationDisplayNotice(
        0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL,
        CFSTR("Allow keyremapd Accessibility Access"),
        CFSTR("This setting can be enabled in System Preferences via the "
              "Security & Privacy preferences pane, Privacy tab, Accessibility "
              "setting"),
        CFSTR("OK"));
    return EXIT_FAILURE;
  }
  if (!installEventTap()) {
    CFUserNotificationDisplayNotice(
        0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL,
        CFSTR("Cannot Install keyremapd"),
        CFSTR("It is not possible to install event handler, check Console "
              "for errors"),
        CFSTR("OK"));
    return EXIT_FAILURE;
  }
  signal(SIGHUP, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGQUIT, signalHandler);
  while (run) {
    CFRunLoopRun();
  }
  return 0;
}
