
#pragma once

#include "IXWindowsImpl.h"

class XWindowsImpl : public IXWindowsImpl {
public:

    Status XInitThreads() override;
    XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler) override;
    Window do_DefaultRootWindow(Display* display) override;
    int XCloseDisplay(Display* display) override;
    int XTestGrabControl(Display* display, Bool impervious) override;
    void XDestroyIC(XIC ic) override;
    Status XCloseIM(XIM im) override;
    int XDestroyWindow(Display* display, Window w) override;
    int XGetKeyboardControl(Display* display, XKeyboardState* value_return) override;
    int XMoveWindow(Display* display, Window w, int x, int y) override;
    int XMapRaised(Display* display, Window w) override;
    void XUnsetICFocus(XIC ic) override;
    int XUnmapWindow(Display* display, Window w) override;
    int XSetInputFocus(Display* display, Window focus, int revert_to, Time time) override;
    Bool DPMSQueryExtension(Display* display, int* event_base, int* error_base) override;
    Bool DPMSCapable(Display* display) override;
    Status DPMSInfo(Display* display, CARD16* power_level, BOOL* state) override;
    Status DPMSForceLevel(Display* display, CARD16 level) override;
    int XGetInputFocus(Display* display, Window* focus_return, int* revert_to_return) override;
    void XSetICFocus(XIC ic) override;
    Bool XQueryPointer(Display* display, Window w, Window* root_return, Window* child_return,
                       int* root_x_return, int* root_y_return, int* win_x_return,
                       int* win_y_return, unsigned int* mask_return) override;
    void XLockDisplay(Display* display) override;
    Bool XCheckMaskEvent(Display* display, long event_mask, XEvent* event_return) override;
    XModifierKeymap* XGetModifierMapping(Display* display) override;
    int XGrabKey(Display* display, int keycode, unsigned int modifiers, Window grab_window,
                 int owner_events, int pointer_made, int keyboard_mode) override;
    int XFreeModifiermap(XModifierKeymap* modmap) override;
    int XUngrabKey(Display* display, int keycode, unsigned int modifiers,
                   Window grab_window) override;
    int XTestFakeButtonEvent(Display* display, unsigned int button, int is_press,
                             unsigned long delay) override;
    int XFlush(Display* display) override;
    int XWarpPointer(Display* display, Window src_w, Window dest_w, int src_x, int src_y,
                     unsigned int src_width, unsigned int src_height, int dest_x,
                     int dest_y) override;
    int XTestFakeRelativeMotionEvent(Display* display, int x, int y, unsigned long delay) override;
    KeyCode XKeysymToKeycode(Display* display, KeySym keysym) override;
    int XTestFakeKeyEvent(Display* display, unsigned int keycode, int is_press,
                          unsigned long delay) override;
    Display* XOpenDisplay(_Xconst char* display_name) override;
    Bool XQueryExtension(Display* display, const char* name, int* major_opcode_return,
                         int* first_event_return, int* first_error_return) override;
    Bool XkbLibraryVersion(int* libMajorRtrn, int* libMinorRtrn) override;
    Bool XkbQueryExtension(Display* display, int* opcodeReturn, int* eventBaseReturn,
                           int* errorBaseReturn, int* majorRtrn, int* minorRtrn) override;
    Bool XkbSelectEvents(Display* display, unsigned int deviceID, unsigned int affect,
                         unsigned int values) override;
    Bool XkbSelectEventDetails(Display* display, unsigned int deviceID, unsigned int eventType,
                               unsigned long affect, unsigned long details) override;
    Bool XRRQueryExtension(Display* display, int* event_base_return,
                           int* error_base_return) override;
    void XRRSelectInput(Display *display, Window window, int mask) override;
    Bool XineramaQueryExtension(Display* display, int* event_base, int* error_base) override;
    Bool XineramaIsActive(Display* display) override;
    void* XineramaQueryScreens(Display* display, int* number) override;
    Window XCreateWindow(Display* display, Window parent, int x, int y, unsigned int width,
                         unsigned int height, unsigned int border_width, int depth,
                         unsigned int klass, Visual* visual, unsigned long valuemask,
                         XSetWindowAttributes* attributes) override;
    XIM XOpenIM(Display* display, _XrmHashBucketRec* rdb, char* res_name, char* res_class) override;
    char* XGetIMValues(XIM im, const char* type, void* ptr) override;
    XIC XCreateIC(XIM im, const char* type1, unsigned long data1, const char* type2,
                  unsigned long data2) override;
    char* XGetICValues(XIC ic, const char* type, unsigned long* mask) override;
    Status XGetWindowAttributes(Display* display, Window w, XWindowAttributes* attrs) override;
    int XSelectInput(Display* display, Window w, long event_mask) override;
    Bool XCheckIfEvent(Display* display, XEvent* event,
                       Bool (*predicate)(Display *, XEvent *, XPointer), XPointer arg) override;
    Bool XFilterEvent(XEvent* event, Window window) override;
    Bool XGetEventData(Display* display, XGenericEventCookie* cookie) override;
    void XFreeEventData(Display* display, XGenericEventCookie* cookie) override;
    int XDeleteProperty(Display* display, Window w, Atom property) override;
    int XResizeWindow(Display* display, Window w, unsigned int width, unsigned int height) override;
    int XMaskEvent(Display* display, long event_mask, XEvent* event_return) override;
    Status XQueryBestCursor(Display* display, Drawable d, unsigned int width, unsigned int height,
                            unsigned int* width_return, unsigned int* height_return) override;
    Pixmap XCreateBitmapFromData(Display* display, Drawable d, const char* data, unsigned int width,
                                 unsigned int height) override;
    Cursor XCreatePixmapCursor(Display* display, Pixmap source, Pixmap mask,
                               XColor* foreground_color, XColor* background_color,
                               unsigned int x, unsigned int y) override;
    int XFreePixmap(Display* display, Pixmap pixmap) override;
    Status XQueryTree(Display* display, Window w, Window* root_return, Window* parent_return,
                      Window** children_return, unsigned int* nchildren_return) override;
    int XmbLookupString(XIC ic, XKeyPressedEvent* event, char* buffer_return, int bytes_buffer,
                        KeySym* keysym_return, int* status_return) override;
    int XLookupString(XKeyEvent* event_struct, char* buffer_return, int bytes_buffer,
                      KeySym* keysym_return, XComposeStatus* status_in_out) override;
    Status XSendEvent(Display* display, Window w, Bool propagate, long event_mask,
                      XEvent* event_send) override;
    int XSync(Display* display, Bool discard) override;
    int XGetPointerMapping(Display* display, unsigned char* map_return, int nmap) override;
    int XGrabKeyboard(Display* display, Window grab_window, Bool owner_events, int pointer_mode,
                      int keyboard_mode, Time time) override;
    int XGrabPointer(Display* display, Window grab_window, Bool owner_events,
                     unsigned int event_mask, int  pointer_mode, int keyboard_mode,
                     Window confine_to, Cursor cursor, Time time) override;
    int XUngrabKeyboard(Display* display, Time time) override;
    int XPending(Display* display) override;
    int XPeekEvent(Display* display, XEvent* event_return) override;
    Status XkbRefreshKeyboardMapping(XkbMapNotifyEvent* event) override;
    int XRefreshKeyboardMapping(XMappingEvent* event_map) override;
    int XISelectEvents(Display* display, Window w, XIEventMask* masks, int num_masks) override;
    Atom XInternAtom(Display* display, _Xconst char* atom_name, Bool only_if_exists) override;
    int XGetScreenSaver(Display* display, int* timeout_return, int*  interval_return,
                        int* prefer_blanking_return, int* allow_exposures_return) override;
    int XSetScreenSaver(Display* display, int timeout, int interval, int prefer_blanking,
                        int allow_exposures) override;
    int XForceScreenSaver(Display* display, int mode) override;
    int XFree(void* data) override;
    Status DPMSEnable(Display* display) override;
    Status DPMSDisable(Display* display) override;
    int XSetSelectionOwner(Display* display, Atom selection, Window w, Time time) override;
    Window XGetSelectionOwner(Display* display, Atom selection) override;
    Atom* XListProperties(Display* display, Window w, int* num_prop_return) override;
    char* XGetAtomName(Display* display, Atom atom) override;
    void XkbFreeKeyboard(XkbDescPtr xkb, unsigned int which, Bool freeDesc) override;
    XkbDescPtr XkbGetMap(Display* display, unsigned int which, unsigned int deviceSpec) override;
    Status XkbGetState(Display* display, unsigned int deviceSet, XkbStatePtr rtrnState) override;
    int XQueryKeymap(Display* display, char keys_return[32]) override;
    Status XkbGetUpdatedMap(Display* display, unsigned int which, XkbDescPtr desc) override;
    Bool XkbLockGroup(Display* display, unsigned int deviceSpec, unsigned int group) override;
    int XDisplayKeycodes(Display* display, int* min_keycodes_return,
                         int* max_keycodes_return) override;
    KeySym* XGetKeyboardMapping(Display* display, unsigned int first_keycode, int keycode_count,
                                int* keysyms_per_keycode_return) override;
    int do_XkbKeyNumGroups(XkbDescPtr m_xkb, KeyCode desc) override;
    XkbKeyTypePtr do_XkbKeyKeyType(XkbDescPtr m_xkb, KeyCode keycode, int eGroup) override;
    KeySym do_XkbKeySymEntry(XkbDescPtr m_xkb, KeyCode keycode, int level, int eGroup) override;
    Bool do_XkbKeyHasActions(XkbDescPtr m_xkb, KeyCode keycode) override;
    XkbAction* do_XkbKeyActionEntry(XkbDescPtr m_xkb, KeyCode keycode, int level,
                                    int eGroup) override;
    unsigned char do_XkbKeyGroupInfo(XkbDescPtr m_xkb, KeyCode keycode) override;
    int XNextEvent(Display* display, XEvent* event_return) override;
};
