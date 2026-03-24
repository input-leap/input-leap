/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base/Log.h"
#include "platform/PortalRemoteDesktop.h"
#include "inputleap/IClipboard.h"

#include <sys/un.h> // for EIS fd hack, remove
#include <sys/socket.h> // for EIS fd hack, remove
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <zlib.h>

namespace inputleap {

/// Convert kBitmap data (BITMAPINFOHEADER + BGR/BGRA pixels, bottom-up) to PNG.
/// Returns empty string on failure.
static std::string bmp_to_png(const std::string& bmp_data)
{
    if (bmp_data.size() < 40) return {};

    const auto* raw = reinterpret_cast<const uint8_t*>(bmp_data.data());

    // Parse BITMAPINFOHEADER (40 bytes, little-endian)
    auto read_u32 = [](const uint8_t* p) -> uint32_t {
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    };
    auto read_s32 = [&](const uint8_t* p) -> int32_t {
        return static_cast<int32_t>(read_u32(p));
    };
    auto read_u16 = [](const uint8_t* p) -> uint16_t {
        return p[0] | (p[1] << 8);
    };

    uint32_t header_size = read_u32(raw + 0);
    int32_t  width       = read_s32(raw + 4);
    int32_t  height      = read_s32(raw + 8);
    uint16_t bpp         = read_u16(raw + 14);
    uint32_t compression = read_u32(raw + 16);

    if (header_size < 40 || width <= 0 || compression != 0 /* BI_RGB */) {
        LOG_WARN("Clipboard: BMP→PNG: unsupported format (header=%u, w=%d, h=%d, bpp=%u, comp=%u)",
                 header_size, width, height, bpp, compression);
        return {};
    }
    if (bpp != 24 && bpp != 32) {
        LOG_WARN("Clipboard: BMP→PNG: unsupported bpp=%u (need 24 or 32)", bpp);
        return {};
    }

    bool bottom_up = (height > 0);
    uint32_t abs_height = bottom_up ? height : -height;
    uint32_t channels = (bpp == 32) ? 4 : 3;
    uint8_t  png_color_type = (bpp == 32) ? 6 /* RGBA */ : 2 /* RGB */;
    uint32_t bmp_row_bytes = ((width * bpp + 31) / 32) * 4; // BMP rows are 4-byte aligned

    const uint8_t* pixels = raw + header_size;
    size_t pixel_data_size = bmp_data.size() - header_size;
    if (pixel_data_size < (size_t)bmp_row_bytes * abs_height) {
        LOG_WARN("Clipboard: BMP→PNG: pixel data too small");
        return {};
    }

    // Build raw PNG image data (filter byte + RGB/RGBA pixels per row, top-down)
    size_t png_row_bytes = 1 + width * channels; // 1 byte filter + pixel data
    std::vector<uint8_t> png_raw(png_row_bytes * abs_height);
    for (uint32_t y = 0; y < abs_height; ++y) {
        uint32_t src_y = bottom_up ? (abs_height - 1 - y) : y;
        const uint8_t* src_row = pixels + src_y * bmp_row_bytes;
        uint8_t* dst_row = png_raw.data() + y * png_row_bytes;
        dst_row[0] = 0; // filter: None
        for (int32_t x = 0; x < width; ++x) {
            const uint8_t* src_px = src_row + x * (bpp / 8);
            uint8_t* dst_px = dst_row + 1 + x * channels;
            // BMP is BGR(A), PNG is RGB(A)
            dst_px[0] = src_px[2]; // R
            dst_px[1] = src_px[1]; // G
            dst_px[2] = src_px[0]; // B
            if (channels == 4)
                dst_px[3] = src_px[3]; // A
        }
    }

    // Compress with zlib (deflate)
    uLongf compressed_size = compressBound(png_raw.size());
    std::vector<uint8_t> compressed(compressed_size);
    int zret = compress2(compressed.data(), &compressed_size,
                         png_raw.data(), png_raw.size(), Z_DEFAULT_COMPRESSION);
    if (zret != Z_OK) {
        LOG_WARN("Clipboard: BMP→PNG: zlib compress failed (%d)", zret);
        return {};
    }

    // Build PNG file
    std::string png;
    png.reserve(8 + 25 + compressed_size + 24 + 12);

    // Helper: write big-endian uint32
    auto write_be32 = [&](uint32_t v) {
        char b[4] = {(char)(v >> 24), (char)(v >> 16), (char)(v >> 8), (char)v};
        png.append(b, 4);
    };

    // Helper: write PNG chunk (type + data + CRC)
    auto write_chunk = [&](const char type[4], const uint8_t* data, size_t len) {
        write_be32(len);
        png.append(type, 4);
        if (len > 0) png.append(reinterpret_cast<const char*>(data), len);
        // CRC over type + data
        uint32_t crc = crc32(0, reinterpret_cast<const Bytef*>(type), 4);
        if (len > 0) crc = crc32(crc, data, len);
        write_be32(crc);
    };

    // PNG signature
    const char sig[] = "\x89PNG\r\n\x1a\n";
    png.append(sig, 8);

    // IHDR chunk
    uint8_t ihdr[13];
    ihdr[0] = (width >> 24); ihdr[1] = (width >> 16); ihdr[2] = (width >> 8); ihdr[3] = width;
    ihdr[4] = (abs_height >> 24); ihdr[5] = (abs_height >> 16); ihdr[6] = (abs_height >> 8); ihdr[7] = abs_height;
    ihdr[8] = 8; // bit depth
    ihdr[9] = png_color_type;
    ihdr[10] = 0; // compression
    ihdr[11] = 0; // filter
    ihdr[12] = 0; // interlace
    write_chunk("IHDR", ihdr, 13);

    // IDAT chunk (compressed pixel data)
    write_chunk("IDAT", compressed.data(), compressed_size);

    // IEND chunk
    write_chunk("IEND", nullptr, 0);

    LOG_DEBUG("Clipboard: BMP→PNG converted %dx%d %ubpp → %zu bytes PNG",
              width, abs_height, bpp, png.size());
    return png;
}

/// Convert PNG data to kBitmap format (BITMAPINFOHEADER + BGR/BGRA pixels, bottom-up).
/// Uses zlib to decompress the IDAT chunks. Returns empty string on failure.
static std::string png_to_bmp(const std::string& png_data)
{
    if (png_data.size() < 8 + 25) return {};

    const auto* p = reinterpret_cast<const uint8_t*>(png_data.data());
    size_t len = png_data.size();

    // Verify PNG signature
    const uint8_t sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (memcmp(p, sig, 8) != 0) return {};

    auto read_be32 = [](const uint8_t* d) -> uint32_t {
        return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
    };

    // Parse IHDR
    size_t pos = 8;
    if (pos + 8 > len) return {};
    uint32_t ihdr_len = read_be32(p + pos);
    if (memcmp(p + pos + 4, "IHDR", 4) != 0 || ihdr_len != 13) return {};
    pos += 8;
    if (pos + 13 > len) return {};

    uint32_t width      = read_be32(p + pos);
    uint32_t height     = read_be32(p + pos + 4);
    uint8_t  bit_depth  = p[pos + 8];
    uint8_t  color_type = p[pos + 9];
    uint8_t  interlace  = p[pos + 12];
    pos += 13 + 4; // skip data + CRC

    if (bit_depth != 8 || interlace != 0) {
        LOG_DEBUG("Clipboard: PNG->BMP: unsupported PNG (depth=%u, interlace=%u)",
                  bit_depth, interlace);
        return {};
    }
    if (color_type != 2 && color_type != 6) { // RGB or RGBA only
        LOG_DEBUG("Clipboard: PNG->BMP: unsupported color type %u", color_type);
        return {};
    }

    uint32_t png_channels = (color_type == 6) ? 4 : 3;

    // Collect all IDAT chunks
    std::string idat_data;
    while (pos + 8 <= len) {
        uint32_t chunk_len = read_be32(p + pos);
        pos += 8;
        if (pos + chunk_len + 4 > len) break;
        if (memcmp(p + pos - 4, "IDAT", 4) == 0) {
            idat_data.append(reinterpret_cast<const char*>(p + pos), chunk_len);
        } else if (memcmp(p + pos - 4, "IEND", 4) == 0) {
            break;
        }
        pos += chunk_len + 4; // data + CRC
    }

    if (idat_data.empty()) return {};

    // Decompress IDAT (zlib)
    size_t raw_size = (size_t)(1 + width * png_channels) * height;
    std::vector<uint8_t> raw(raw_size);
    uLongf dest_len = raw_size;
    int zret = uncompress(raw.data(), &dest_len,
                          reinterpret_cast<const Bytef*>(idat_data.data()),
                          idat_data.size());
    if (zret != Z_OK) {
        LOG_WARN("Clipboard: PNG->BMP: zlib uncompress failed (%d)", zret);
        return {};
    }

    // Convert to BMP (BGR/BGRA, bottom-up) with PNG row filter reversal
    uint32_t bmp_bpp = (png_channels == 4) ? 32 : 24;
    uint32_t bmp_row_bytes = ((width * bmp_bpp + 31) / 32) * 4;
    size_t bmp_size = 40 + (size_t)bmp_row_bytes * height;
    std::string bmp(bmp_size, '\0');
    auto* out = reinterpret_cast<uint8_t*>(&bmp[0]);

    auto write_le32 = [](uint8_t* d, uint32_t v) {
        d[0] = v; d[1] = v >> 8; d[2] = v >> 16; d[3] = v >> 24;
    };
    auto write_le16 = [](uint8_t* d, uint16_t v) {
        d[0] = v; d[1] = v >> 8;
    };

    // BITMAPINFOHEADER (40 bytes)
    memset(out, 0, 40);
    write_le32(out + 0, 40);
    write_le32(out + 4, width);
    write_le32(out + 8, height); // positive = bottom-up
    write_le16(out + 12, 1);    // planes
    write_le16(out + 14, bmp_bpp);
    write_le32(out + 20, bmp_row_bytes * height);

    uint32_t png_row_stride = 1 + width * png_channels;
    std::vector<uint8_t> prev_row(width * png_channels, 0);

    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* src_row = raw.data() + y * png_row_stride;
        uint8_t filter_type = src_row[0];
        const uint8_t* src_px = src_row + 1;

        std::vector<uint8_t> cur_row(width * png_channels);
        for (uint32_t i = 0; i < width * png_channels; ++i) {
            uint8_t a = (i >= png_channels) ? cur_row[i - png_channels] : 0;
            uint8_t b = prev_row[i];
            uint8_t c = (i >= png_channels) ? prev_row[i - png_channels] : 0;

            switch (filter_type) {
                case 0: cur_row[i] = src_px[i]; break;
                case 1: cur_row[i] = src_px[i] + a; break;
                case 2: cur_row[i] = src_px[i] + b; break;
                case 3: cur_row[i] = src_px[i] + ((a + b) / 2); break;
                case 4: {
                    int pp = (int)a + (int)b - (int)c;
                    int pa = abs(pp - (int)a), pb = abs(pp - (int)b), pc = abs(pp - (int)c);
                    cur_row[i] = src_px[i] + ((pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c));
                    break;
                }
                default: cur_row[i] = src_px[i]; break;
            }
        }
        prev_row = cur_row;

        // Write BMP row (bottom-up, BGR/BGRA)
        uint8_t* dst_row = out + 40 + (height - 1 - y) * bmp_row_bytes;
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t* sp = cur_row.data() + x * png_channels;
            uint8_t* dp = dst_row + x * (bmp_bpp / 8);
            dp[0] = sp[2]; dp[1] = sp[1]; dp[2] = sp[0]; // BGR
            if (png_channels == 4) dp[3] = sp[3];
        }
    }

    LOG_DEBUG("Clipboard: PNG->BMP converted %ux%u -> %zu bytes BMP", width, height, bmp.size());
    return bmp;
}

struct MimeMapping {
    IClipboard::EFormat format;
    std::vector<std::string> mime_types;
};

// MIME type mapping table — extend here to add clipboard format support.
// First MIME type in each list is preferred (offered first in SetSelection).
// When reading (SelectionRead), we try each MIME type in order until one matches
// what the clipboard owner offers.
// Image formats are passthrough (raw bytes, no conversion needed).
static const std::vector<MimeMapping> kMimeMappings = {
    {IClipboard::kText, {"text/plain;charset=utf-8", "text/plain", "UTF8_STRING", "STRING"}},
    {IClipboard::kHTML, {"text/html", "text/html;charset=utf-8"}},
    {IClipboard::kPNG,  {"image/png"}},
    {IClipboard::kBitmap, {"image/bmp", "image/x-bmp"}},
    {IClipboard::kJpeg, {"image/jpeg"}},
    {IClipboard::kTiff, {"image/tiff"}},
    {IClipboard::kWebp, {"image/webp"}},
};

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen,
                                         IEventQueue* events,
                                         bool clipboard_only) :
    screen_(screen),
    events_(events),
    portal_(xdp_portal_new()),
    clipboard_only_(clipboard_only)
{
    glib_main_loop_ = g_main_loop_new(nullptr, true);
    glib_thread_ = new Thread([this](){ glib_thread(); });

    reconnect(0);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
    if (g_main_loop_is_running(glib_main_loop_))
        g_main_loop_quit(glib_main_loop_);

    if (glib_thread_ != nullptr) {
        glib_thread_->cancel();
        glib_thread_->wait();
        delete glib_thread_;
        glib_thread_ = nullptr;

        g_main_loop_unref(glib_main_loop_);
        glib_main_loop_ = nullptr;
    }

    cleanup_clipboard();

    if (session_signal_id_)
        g_signal_handler_disconnect(session_, session_signal_id_);
    if (session_ != nullptr)
        g_object_unref(session_);
    g_object_unref(portal_);

    free(session_restore_token_);
}

gboolean PortalRemoteDesktop::timeout_handler()
{
    return true; // keep re-triggering
}

void PortalRemoteDesktop::reconnect(unsigned int timeout)
{
    auto init_cb = [](gpointer data) -> gboolean
    {
        return reinterpret_cast<PortalRemoteDesktop*>(data)->init_remote_desktop_session();
    };

    if (timeout > 0)
        g_timeout_add(timeout, init_cb, this);
    else
        g_idle_add(init_cb, this);
}

void PortalRemoteDesktop::cb_session_closed(XdpSession* session)
{
    LOG_ERR("Our RemoteDesktop session was closed, re-connecting.");
    cleanup_clipboard();
    g_signal_handler_disconnect(session, session_signal_id_);
    session_signal_id_ = 0;
    if (!clipboard_only_) {
        events_->add_event(EventType::EI_SESSION_CLOSED, screen_->get_event_target());
    }

    // gcc warning "Suspicious usage of 'sizeof(A*)'" can be ignored
    g_clear_object(&session_);

    reconnect(1000);
}

void PortalRemoteDesktop::cb_session_started(GObject* object, GAsyncResult* res)
{
    g_autoptr(GError) error = nullptr;
    auto session = XDP_SESSION(object);
    auto success = xdp_session_start_finish(session, res, &error);
    if (!success) {
        LOG_ERR("Failed to start session");
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
        return;
    }

    session_restore_token_ = xdp_session_get_restore_token(session);

    if (!clipboard_only_) {
        // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
        // version in the impl.portal), i.e. you'll need an updated compositor on
        // top of everything...
        auto fd = -1;
#if HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS
        fd = xdp_session_connect_to_eis(session, &error);
#endif
        if (fd < 0) {
            g_main_loop_quit(glib_main_loop_);
            events_->add_event(EventType::QUIT);
            return;
        }

        // Socket ownership is transferred to the EiScreen
        events_->add_event(EventType::EI_SCREEN_CONNECTED_TO_EIS, screen_->get_event_target(),
                            create_event_data<int>(fd));
    }

}

void PortalRemoteDesktop::cb_init_remote_desktop_session(GObject* object, GAsyncResult* res)
{
    LOG_DEBUG("Session ready");
    g_autoptr(GError) error = nullptr;

    auto session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
    if (!session) {
        LOG_ERR("Failed to initialize RemoteDesktop session: %s", error->message);
        // This was the first attempt to connect to the RD portal - quit if that fails.
        if (session_iteration_ == 0) {
            g_main_loop_quit(glib_main_loop_);
            events_->add_event(EventType::QUIT);
        } else {
            this->reconnect(1000);
        }
        return;
    }

    session_ = session;
    ++session_iteration_;

    // FIXME: the lambda trick doesn't work here for unknown reasons, we need
    // the static function
    session_signal_id_ = g_signal_connect(G_OBJECT(session), "closed",
                                         G_CALLBACK(cb_session_closed_cb),
                                         this);

    // Request clipboard access BEFORE starting the session
    // (the Clipboard portal requires this)
    request_clipboard();

    LOG_DEBUG("Session ready, starting");
    xdp_session_start(session,
                      nullptr, // parent
                      nullptr, // cancellable
                      [](GObject *obj, GAsyncResult *res, gpointer data) {
                          reinterpret_cast<PortalRemoteDesktop*>(data)->cb_session_started(obj, res);
                      },
                      this);
}

#if !defined(HAVE_LIBPORTAL_CREATE_REMOTE_DESKTOP_SESSION_FULL)
static inline void
xdp_portal_create_remote_desktop_session_full(XdpPortal              *portal,
                                              XdpDeviceType           devices,
                                              XdpOutputType           outputs,
                                              XdpRemoteDesktopFlags   flags,
                                              XdpCursorMode           cursor_mode,
                                              XdpPersistMode          _unused1,
                                              const char             *_unused2,
                                              GCancellable           *cancellable,
                                              GAsyncReadyCallback     callback,
                                              gpointer                data)
{
    xdp_portal_create_remote_desktop_session(portal, devices, outputs,
                                             flags, cursor_mode, cancellable,
                                             callback, data);
}
#endif


gboolean PortalRemoteDesktop::init_remote_desktop_session()
{
    LOG_DEBUG("Setting up the RemoteDesktop session with restore token %s", session_restore_token_);
    xdp_portal_create_remote_desktop_session_full(
                portal_,
                static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD),
                XDP_OUTPUT_NONE,
                XDP_REMOTE_DESKTOP_FLAG_NONE,
                XDP_CURSOR_MODE_HIDDEN,
                XDP_PERSIST_MODE_TRANSIENT,
                session_restore_token_,
                nullptr, // cancellable
                [](GObject *obj, GAsyncResult *res, gpointer data) {
                    reinterpret_cast<PortalRemoteDesktop*>(data)->cb_init_remote_desktop_session(obj, res);
                },
                this);

    return false; // don't reschedule
}

void PortalRemoteDesktop::glib_thread()
{
    auto context = g_main_loop_get_context(glib_main_loop_);

    while (g_main_loop_is_running(glib_main_loop_)) {
        Thread::testCancel();
        g_main_context_iteration(context, true);
    }
}

// ---------------------------------------------------------------------------
// Clipboard implementation via org.freedesktop.portal.Clipboard
// ---------------------------------------------------------------------------

void PortalRemoteDesktop::request_clipboard()
{
    g_autoptr(GError) error = nullptr;

    dbus_connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    if (!dbus_connection_) {
        LOG_WARN("Clipboard: failed to get DBus session bus: %s", error->message);
        return;
    }

    // Discover session handle via DBus introspection.
    // libportal's XdpSession does not expose the session handle publicly.
    // Session paths follow: /org/freedesktop/portal/desktop/session/{sender_token}/{id}
    // where sender_token is our DBus unique name with ':' removed and '.' replaced by '_'.
    const gchar* unique_name = g_dbus_connection_get_unique_name(dbus_connection_);
    if (!unique_name || unique_name[0] != ':') {
        LOG_WARN("Clipboard: could not get DBus unique name");
        g_object_unref(dbus_connection_);
        dbus_connection_ = nullptr;
        return;
    }
    std::string sender_token = unique_name + 1; // skip ':'
    for (auto& ch : sender_token) {
        if (ch == '.') ch = '_';
    }

    std::string parent_path = "/org/freedesktop/portal/desktop/session/" + sender_token;
    g_autoptr(GVariant) introspect_result = g_dbus_connection_call_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        parent_path.c_str(),
        "org.freedesktop.DBus.Introspectable",
        "Introspect",
        nullptr,
        G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE,
        5000, nullptr, &error);

    if (!introspect_result) {
        LOG_WARN("Clipboard: failed to introspect sessions: %s", error->message);
        g_object_unref(dbus_connection_);
        dbus_connection_ = nullptr;
        return;
    }

    // Parse XML to find the last session node (most recently created)
    const gchar* xml = nullptr;
    g_variant_get(introspect_result, "(&s)", &xml);

    // Find all <node name="..."/> entries — use the last one (our newest session)
    std::string last_node;
    std::string xml_str(xml);
    std::string search = "node name=\"";
    size_t pos = 0;
    while ((pos = xml_str.find(search, pos)) != std::string::npos) {
        pos += search.length();
        auto end = xml_str.find('"', pos);
        if (end != std::string::npos) {
            last_node = xml_str.substr(pos, end - pos);
            pos = end;
        }
    }

    if (last_node.empty()) {
        LOG_WARN("Clipboard: no session found under %s", parent_path.c_str());
        g_object_unref(dbus_connection_);
        dbus_connection_ = nullptr;
        return;
    }

    session_handle_ = parent_path + "/" + last_node;

    LOG_DEBUG("Clipboard: requesting clipboard on session %s", session_handle_.c_str());

    // Call org.freedesktop.portal.Clipboard.RequestClipboard(session_handle, {})
    g_autoptr(GVariant) result = g_dbus_connection_call_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        "RequestClipboard",
        g_variant_new("(oa{sv})", session_handle_.c_str(), nullptr),
        nullptr,       // reply type
        G_DBUS_CALL_FLAGS_NONE,
        5000,          // timeout ms
        nullptr,       // cancellable
        &error);

    if (!result) {
        LOG_WARN("Clipboard: RequestClipboard failed: %s", error->message);
        g_object_unref(dbus_connection_);
        dbus_connection_ = nullptr;
        session_handle_.clear();
        return;
    }

    // Subscribe to SelectionOwnerChanged signal.
    // Use nullptr for object_path to receive from any path — the portal may
    // emit signals on the desktop path or the session path depending on impl.
    selection_owner_changed_sub_ = g_dbus_connection_signal_subscribe(
        dbus_connection_,
        nullptr,       // sender (any)
        "org.freedesktop.portal.Clipboard",
        "SelectionOwnerChanged",
        nullptr,       // object_path (any)
        nullptr,       // arg0
        G_DBUS_SIGNAL_FLAGS_NONE,
        cb_selection_owner_changed,
        this,
        nullptr);      // user_data_free_func

    // Subscribe to SelectionTransfer signal
    selection_transfer_sub_ = g_dbus_connection_signal_subscribe(
        dbus_connection_,
        nullptr,       // sender (any)
        "org.freedesktop.portal.Clipboard",
        "SelectionTransfer",
        nullptr,       // object_path (any)
        nullptr,       // arg0
        G_DBUS_SIGNAL_FLAGS_NONE,
        cb_selection_transfer,
        this,
        nullptr);

    clipboard_enabled_ = true;
    LOG_NOTE("Clipboard: Wayland clipboard sharing enabled via XDG Portal");
}

void PortalRemoteDesktop::cleanup_clipboard()
{
    if (!dbus_connection_)
        return;

    if (selection_owner_changed_sub_) {
        g_dbus_connection_signal_unsubscribe(dbus_connection_, selection_owner_changed_sub_);
        selection_owner_changed_sub_ = 0;
    }
    if (selection_transfer_sub_) {
        g_dbus_connection_signal_unsubscribe(dbus_connection_, selection_transfer_sub_);
        selection_transfer_sub_ = 0;
    }

    g_object_unref(dbus_connection_);
    dbus_connection_ = nullptr;
    session_handle_.clear();
    clipboard_enabled_ = false;

    std::lock_guard<std::mutex> lock(clipboard_mutex_);
    available_mime_types_.clear();
}

void PortalRemoteDesktop::on_selection_owner_changed(
    GDBusConnection* /*connection*/,
    const gchar* /*sender_name*/,
    const gchar* /*object_path*/,
    const gchar* /*interface_name*/,
    const gchar* /*signal_name*/,
    GVariant* parameters)
{
    // Signal signature: (oa{sv})
    // session_handle, options{mime_types: as, session_is_owner: b}
    const gchar* session_handle = nullptr;
    g_autoptr(GVariantIter) options_iter = nullptr;
    g_variant_get(parameters, "(&oa{sv})", &session_handle, &options_iter);

    // Check this signal is for our session
    if (session_handle_ != session_handle)
        return;

    // Extract mime_types from options
    std::vector<std::string> mime_types;
    bool session_is_owner = false;

    const gchar* key = nullptr;
    GVariant* value = nullptr;
    while (g_variant_iter_loop(options_iter, "{&sv}", &key, &value)) {
        if (g_strcmp0(key, "mime_types") == 0 && g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
            gsize n_types = 0;
            const gchar** types = g_variant_get_strv(value, &n_types);
            for (gsize i = 0; i < n_types; ++i) {
                mime_types.emplace_back(types[i]);
            }
            g_free(types);
        } else if (g_strcmp0(key, "session_is_owner") == 0 && g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
            session_is_owner = g_variant_get_boolean(value);
        }
    }

    // Log the offered MIME types for debugging
    std::string mime_list;
    for (const auto& mt : mime_types) {
        if (!mime_list.empty()) mime_list += ", ";
        mime_list += mt;
    }
    LOG_DEBUG("Clipboard: SelectionOwnerChanged with %zu mime types [%s], session_is_owner=%d",
              mime_types.size(), mime_list.c_str(), session_is_owner);

    // If we own the clipboard (we just set it via SetSelection), ignore.
    // Also use our own flag because KDE's portal doesn't always set session_is_owner.
    // KDE may fire multiple SelectionOwnerChanged events after SetSelection, so we
    // keep ignoring until we see an event that clearly comes from another app
    // (has mime types and we didn't just set it).
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        if (session_is_owner || we_own_clipboard_) {
            // Only clear the flag when we see mime_types (the "real" owner changed event).
            // The first event often has 0 mime_types, followed by one with mime_types.
            if (!mime_types.empty()) {
                we_own_clipboard_ = false;
            }
            LOG_DEBUG("Clipboard: ignoring SelectionOwnerChanged (we are the owner)");
            return;
        }
    }

    // Ignore events with no mime types (clipboard cleared/lost)
    if (mime_types.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        available_mime_types_ = std::move(mime_types);
    }

    // Notify InputLeap that clipboard has been grabbed by another application
    screen_->sendClipboardEvent(EventType::CLIPBOARD_GRABBED, kClipboardClipboard);
}

void PortalRemoteDesktop::on_selection_transfer(
    GDBusConnection* /*connection*/,
    const gchar* /*sender_name*/,
    const gchar* /*object_path*/,
    const gchar* /*interface_name*/,
    const gchar* /*signal_name*/,
    GVariant* parameters)
{
    // Signal signature: (osu) — session_handle, mime_type, serial
    const gchar* session_handle = nullptr;
    const gchar* mime_type = nullptr;
    guint32 serial = 0;
    g_variant_get(parameters, "(&o&su)", &session_handle, &mime_type, &serial);

    if (session_handle_ != session_handle)
        return;

    LOG_DEBUG("Clipboard: SelectionTransfer requested for mime=%s serial=%u", mime_type, serial);

    // Find the matching format for this mime type
    std::string data;
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        stored_clipboard_.open(0);
        for (const auto& mapping : kMimeMappings) {
            bool found = false;
            for (const auto& mt : mapping.mime_types) {
                if (mt == mime_type) {
                    if (stored_clipboard_.has(mapping.format)) {
                        data = stored_clipboard_.get(mapping.format);
                    }
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        stored_clipboard_.close();
    }

    // Call SelectionWrite to get an fd to write to
    g_autoptr(GError) error = nullptr;
    g_autoptr(GUnixFDList) fd_list = nullptr;
    g_autoptr(GVariant) result = g_dbus_connection_call_with_unix_fd_list_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        "SelectionWrite",
        g_variant_new("(ou)", session_handle_.c_str(), serial),
        G_VARIANT_TYPE("(h)"),
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        nullptr,       // in fd_list
        &fd_list,      // out fd_list
        nullptr,       // cancellable
        &error);

    if (!result) {
        LOG_WARN("Clipboard: SelectionWrite failed: %s", error->message);
        return;
    }

    gint32 fd_index = 0;
    g_variant_get(result, "(h)", &fd_index);
    int fd = g_unix_fd_list_get(fd_list, fd_index, &error);
    if (fd < 0) {
        LOG_WARN("Clipboard: failed to get fd from SelectionWrite: %s", error->message);
        return;
    }

    // Write the clipboard data to the fd
    bool success = true;
    if (!data.empty()) {
        const char* ptr = data.c_str();
        size_t remaining = data.size();
        while (remaining > 0) {
            ssize_t written = write(fd, ptr, remaining);
            if (written < 0) {
                if (errno == EINTR) continue;
                LOG_WARN("Clipboard: write to SelectionWrite fd failed: %s", strerror(errno));
                success = false;
                break;
            }
            ptr += written;
            remaining -= written;
        }
    }
    close(fd);

    // Notify that writing is done
    g_autoptr(GError) done_error = nullptr;
    g_autoptr(GVariant) done_result = g_dbus_connection_call_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        "SelectionWriteDone",
        g_variant_new("(oub)", session_handle_.c_str(), serial, success),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        nullptr,
        &done_error);

    if (!done_result) {
        LOG_WARN("Clipboard: SelectionWriteDone failed: %s", done_error->message);
    } else {
        LOG_DEBUG("Clipboard: SelectionTransfer completed for mime=%s (%zu bytes)", mime_type, data.size());
    }
}

std::string PortalRemoteDesktop::read_mime_type_from_portal(const std::string& mime_type)
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GUnixFDList) fd_list = nullptr;
    g_autoptr(GVariant) result = g_dbus_connection_call_with_unix_fd_list_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        "SelectionRead",
        g_variant_new("(os)", session_handle_.c_str(), mime_type.c_str()),
        G_VARIANT_TYPE("(h)"),
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        nullptr,       // in fd_list
        &fd_list,      // out fd_list
        nullptr,       // cancellable
        &error);

    if (!result) {
        LOG_DEBUG("Clipboard: SelectionRead for %s failed: %s", mime_type.c_str(), error->message);
        return {};
    }

    gint32 fd_index = 0;
    g_variant_get(result, "(h)", &fd_index);
    int fd = g_unix_fd_list_get(fd_list, fd_index, &error);
    if (fd < 0) {
        LOG_WARN("Clipboard: failed to get fd from SelectionRead: %s", error->message);
        return {};
    }

    // Read all data from the fd (limit to 100MB to prevent runaway reads)
    static constexpr size_t kMaxClipboardRead = 100 * 1024 * 1024;
    std::string data;
    char buf[4096];
    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            data.append(buf, n);
            if (data.size() > kMaxClipboardRead) {
                LOG_WARN("Clipboard: SelectionRead for %s exceeded size limit (%zu bytes), truncating",
                         mime_type.c_str(), data.size());
                break;
            }
        } else if (n == 0) {
            break; // EOF
        } else {
            if (errno == EINTR) continue;
            LOG_WARN("Clipboard: read from SelectionRead fd failed: %s", strerror(errno));
            break;
        }
    }
    close(fd);

    LOG_DEBUG("Clipboard: SelectionRead for %s returned %zu bytes", mime_type.c_str(), data.size());
    return data;
}

bool PortalRemoteDesktop::getClipboard(ClipboardID id, IClipboard* clipboard)
{
    if (!clipboard_enabled_ || id != kClipboardClipboard)
        return false;

    // Get the list of available mime types (set by SelectionOwnerChanged)
    std::vector<std::string> available;
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        available = available_mime_types_;
    }

    if (available.empty())
        return false;

    auto time = static_cast<IClipboard::Time>(0);
    if (!clipboard->open(time))
        return false;

    // Read clipboard data for each format.
    // For image formats, only read ONE (the most compact/preferred) to avoid
    // sending megabytes of redundant data over the network. The receiving side
    // can convert between image formats if needed. PNG is preferred as it's
    // lossless and typically compact (lesson from KDE Spectacle).
    bool got_any = false;
    bool got_image = false;
    for (const auto& mapping : kMimeMappings) {
        // Skip redundant image formats if we already got one
        bool is_image = (mapping.format == IClipboard::kPNG ||
                         mapping.format == IClipboard::kBitmap ||
                         mapping.format == IClipboard::kJpeg ||
                         mapping.format == IClipboard::kTiff ||
                         mapping.format == IClipboard::kWebp);
        if (is_image && got_image)
            continue;

        // Try each MIME type in preference order
        for (const auto& mime : mapping.mime_types) {
            // Check if this MIME type is offered by the clipboard owner
            bool offered = false;
            for (const auto& avail : available) {
                if (avail == mime) {
                    offered = true;
                    break;
                }
            }
            if (!offered)
                continue;

            auto data = read_mime_type_from_portal(mime);
            if (!data.empty()) {
                clipboard->add(mapping.format, data);
                got_any = true;
                if (is_image) {
                    got_image = true;
                    // If we got PNG, also provide as kBitmap (BMP) for Windows compatibility
                    if (mapping.format == IClipboard::kPNG) {
                        auto bmp_data = png_to_bmp(data);
                        if (!bmp_data.empty()) {
                            clipboard->add(IClipboard::kBitmap, bmp_data);
                        }
                    }
                }
                break; // got data for this format, move to next
            }
        }
    }

    clipboard->close();
    return got_any;
}

bool PortalRemoteDesktop::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
    if (!clipboard_enabled_ || id != kClipboardClipboard)
        return false;

    // Store clipboard data for later SelectionTransfer requests.
    // If we have BMP but not PNG, convert BMP→PNG since Wayland compositors
    // (KDE/Klipper) handle image/png but not image/bmp via the clipboard portal.
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        if (clipboard != nullptr) {
            IClipboard::copy(&stored_clipboard_, clipboard);

            // Convert BMP→PNG if needed
            stored_clipboard_.open(0);
            bool has_bmp = stored_clipboard_.has(IClipboard::kBitmap);
            bool has_png = stored_clipboard_.has(IClipboard::kPNG);
            std::string bmp_data;
            if (has_bmp && !has_png) {
                bmp_data = stored_clipboard_.get(IClipboard::kBitmap);
            }
            stored_clipboard_.close();

            if (!bmp_data.empty()) {
                auto png_data = bmp_to_png(bmp_data);
                if (!png_data.empty()) {
                    stored_clipboard_.open(0);
                    stored_clipboard_.add(IClipboard::kPNG, png_data);
                    stored_clipboard_.close();
                }
            }
        } else {
            auto time = static_cast<IClipboard::Time>(0);
            stored_clipboard_.open(time);
            stored_clipboard_.clear();
            stored_clipboard_.close();
        }
    }

    // Check if we have image data — for images, use wl-copy directly since the
    // XDG Clipboard portal's SelectionTransfer doesn't work reliably for binary
    // data on KDE. wl-copy uses ext_data_control which works correctly.
    bool has_image = false;
    std::string png_data_for_wlcopy;
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        stored_clipboard_.open(0);
        if (stored_clipboard_.has(IClipboard::kPNG)) {
            png_data_for_wlcopy = stored_clipboard_.get(IClipboard::kPNG);
            has_image = true;
        }
        stored_clipboard_.close();
    }

    if (has_image && !png_data_for_wlcopy.empty()) {
        // Use wl-copy for image data (bypasses portal, uses ext_data_control)
        LOG_DEBUG("Clipboard: using wl-copy for image/png (%zu bytes)", png_data_for_wlcopy.size());
        int pipefd[2];
        if (pipe(pipefd) == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child: wl-copy --type image/png < pipe
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                // Close inherited fds to avoid holding server's listen socket
                for (int fd = 3; fd < 1024; ++fd) close(fd);
                execlp("wl-copy", "wl-copy", "--type", "image/png", nullptr);
                _exit(1);
            } else if (pid > 0) {
                // Parent: write PNG data to pipe
                close(pipefd[0]);
                const char* ptr = png_data_for_wlcopy.c_str();
                size_t remaining = png_data_for_wlcopy.size();
                while (remaining > 0) {
                    ssize_t written = write(pipefd[1], ptr, remaining);
                    if (written <= 0) {
                        if (errno == EINTR) continue;
                        break;
                    }
                    ptr += written;
                    remaining -= written;
                }
                close(pipefd[1]);
                // Don't wait — wl-copy runs in background to serve the clipboard
            } else {
                close(pipefd[0]);
                close(pipefd[1]);
                LOG_WARN("Clipboard: fork failed for wl-copy");
            }
        }

        // Suppress the SelectionOwnerChanged from wl-copy taking ownership
        {
            std::lock_guard<std::mutex> lock(clipboard_mutex_);
            we_own_clipboard_ = true;
        }
        return true;
    }

    // For text/HTML: use the portal clipboard (SelectionTransfer works for text)
    GVariantBuilder mime_builder;
    g_variant_builder_init(&mime_builder, G_VARIANT_TYPE("as"));
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        stored_clipboard_.open(0);
        for (const auto& mapping : kMimeMappings) {
            if (stored_clipboard_.has(mapping.format)) {
                for (const auto& mime : mapping.mime_types) {
                    g_variant_builder_add(&mime_builder, "s", mime.c_str());
                }
            }
        }
        stored_clipboard_.close();
    }

    // Build options dict with mime_types
    GVariantBuilder options_builder;
    g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options_builder, "{sv}", "mime_types",
                          g_variant_builder_end(&mime_builder));

    // Mark that we own the clipboard so we ignore the subsequent SelectionOwnerChanged
    {
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        we_own_clipboard_ = true;
    }

    // Call SetSelection
    g_autoptr(GError) error = nullptr;
    g_autoptr(GVariant) result = g_dbus_connection_call_sync(
        dbus_connection_,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        "SetSelection",
        g_variant_new("(oa{sv})", session_handle_.c_str(), &options_builder),
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        nullptr,
        &error);

    if (!result) {
        LOG_WARN("Clipboard: SetSelection failed: %s", error->message);
        std::lock_guard<std::mutex> lock(clipboard_mutex_);
        we_own_clipboard_ = false;
        return false;
    }

    LOG_DEBUG("Clipboard: SetSelection succeeded");
    return true;
}

} // namespace inputleap
