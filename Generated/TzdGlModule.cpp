#include "TzdInterpreter.h"
#include "TzdGlModule.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <mutex>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct GLWindowContext {
    HWND hwnd = NULL;
    HDC hdc = NULL;
    HGLRC hrc = NULL;
    int width = 800;
    int height = 600;
    bool shouldClose = false;
    bool keys[512] = {false};
    double mouseX = 0.0;
    double mouseY = 0.0;
    bool mouseButtons[8] = {false};
};

static std::unordered_map<HWND, GLWindowContext*> g_glWindows;
static std::mutex g_glMutex;
static bool g_glClassRegistered = false;

static LRESULT CALLBACK GLWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GLWindowContext* ctx = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_glMutex);
        auto it = g_glWindows.find(hwnd);
        if (it != g_glWindows.end()) ctx = it->second;
    }

    switch (msg) {
    case WM_CLOSE:
        if (ctx) ctx->shouldClose = true;
        return 0;

    case WM_SIZE:
        if (ctx) {
            ctx->width = LOWORD(lParam);
            ctx->height = HIWORD(lParam);
            if (ctx->height == 0) ctx->height = 1;
            wglMakeCurrent(ctx->hdc, ctx->hrc);
            glViewport(0, 0, ctx->width, ctx->height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(45.0, (double)ctx->width / (double)ctx->height, 0.1, 1000.0);
            glMatrixMode(GL_MODELVIEW);
        }
        return 0;

    case WM_KEYDOWN:
        if (ctx && wParam < 512) ctx->keys[wParam] = true;
        return 0;

    case WM_KEYUP:
        if (ctx && wParam < 512) ctx->keys[wParam] = false;
        return 0;

    case WM_MOUSEMOVE:
        if (ctx) {
            ctx->mouseX = (double)LOWORD(lParam);
            ctx->mouseY = (double)HIWORD(lParam);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (ctx) ctx->mouseButtons[0] = true;
        return 0;
    case WM_LBUTTONUP:
        if (ctx) ctx->mouseButtons[0] = false;
        return 0;
    case WM_RBUTTONDOWN:
        if (ctx) ctx->mouseButtons[1] = true;
        return 0;
    case WM_RBUTTONUP:
        if (ctx) ctx->mouseButtons[1] = false;
        return 0;
    case WM_MBUTTONDOWN:
        if (ctx) ctx->mouseButtons[2] = true;
        return 0;
    case WM_MBUTTONUP:
        if (ctx) ctx->mouseButtons[2] = false;
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static GLWindowContext* getGLContext(HWND hwnd) {
    std::lock_guard<std::mutex> lock(g_glMutex);
    auto it = g_glWindows.find(hwnd);
    if (it != g_glWindows.end()) return it->second;
    return nullptr;
}

static std::wstring toWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring res(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &res[0], size);
    return res;
}

void TzdGlModule::init(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f);
        v.name = name;
        interp->setGlobalVariable(name, v);
    };

    // -------------------------------------------------------------
    // Window & Context Lifecycle
    // -------------------------------------------------------------
    reg("gl_init_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "TzdLang OpenGL Window";
        int width = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 800;
        int height = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 600;

        HINSTANCE hInstance = GetModuleHandle(NULL);
        if (!g_glClassRegistered) {
            WNDCLASSEXW wc;
            ZeroMemory(&wc, sizeof(wc));
            wc.cbSize = sizeof(wc);
            wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = GLWndProc;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = L"TzdGLWindowClass";
            RegisterClassExW(&wc);
            g_glClassRegistered = true;
        }

        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        std::wstring wTitle = toWide(title);
        HWND hwnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            L"TzdGLWindowClass",
            wTitle.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            NULL, NULL, hInstance, NULL
        );

        if (!hwnd) return TzdValue();

        // 无论父进程是否以 SW_HIDE / windowsHide 模式启动，均强制将 OpenGL 渲染窗口正常显示到前台
        ShowWindow(hwnd, SW_SHOW);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        SetForegroundWindow(hwnd);

        HDC hdc = GetDC(hwnd);
        PIXELFORMATDESCRIPTOR pfd;
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int format = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, format, &pfd);

        HGLRC hrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hrc);

        // Default configurations
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearColor(0.12f, 0.12f, 0.18f, 1.0f);
        glClearDepth(1.0f);
        glViewport(0, 0, width, height);

        // 设置默认三维透视投影矩阵 (45度 FOV, 近截面0.1, 远截面1000.0)
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)width / (double)(height > 0 ? height : 1), 0.1, 1000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        GLWindowContext* ctx = new GLWindowContext();
        ctx->hwnd = hwnd;
        ctx->hdc = hdc;
        ctx->hrc = hrc;
        ctx->width = width;
        ctx->height = height;

        {
            std::lock_guard<std::mutex> lock(g_glMutex);
            g_glWindows[hwnd] = ctx;
        }

        return TzdValue((void*)hwnd);
    });

    reg("gl_swap_buffers", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx) return TzdValue(false);
        SwapBuffers(ctx->hdc);
        return TzdValue(true);
    });

    reg("gl_make_current", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx) return TzdValue(false);
        wglMakeCurrent(ctx->hdc, ctx->hrc);
        return TzdValue(true);
    });

    reg("gl_poll_events", [](const std::vector<TzdValue>& args) -> TzdValue {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!args.empty() && args[0].type == TzdValue::POINTER) {
            HWND hwnd = (HWND)args[0].ptrVal;
            GLWindowContext* ctx = getGLContext(hwnd);
            if (ctx && ctx->shouldClose) return TzdValue(false); // Indicates closed
        }
        return TzdValue(true);
    });

    reg("gl_window_should_close", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(true);
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx) return TzdValue(true);
        return TzdValue(ctx->shouldClose);
    });

    reg("gl_close_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_glMutex);
            auto it = g_glWindows.find(hwnd);
            if (it != g_glWindows.end()) {
                ctx = it->second;
                g_glWindows.erase(it);
            }
        }
        if (ctx) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(ctx->hrc);
            ReleaseDC(ctx->hwnd, ctx->hdc);
            DestroyWindow(ctx->hwnd);
            delete ctx;
            return TzdValue(true);
        }
        return TzdValue(false);
    });

    reg("gl_get_key", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int key = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx || key < 0 || key >= 512) return TzdValue(false);
        return TzdValue(ctx->keys[key]);
    });

    reg("gl_get_mouse_pos", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx) return TzdValue();
        std::vector<TzdValue> res = { TzdValue(ctx->mouseX), TzdValue(ctx->mouseY) };
        return TzdValue(res);
    });

    reg("gl_get_mouse_btn", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int btn = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx || btn < 0 || btn >= 8) return TzdValue(false);
        return TzdValue(ctx->mouseButtons[btn]);
    });

    reg("gl_get_window_size", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND hwnd = (HWND)args[0].ptrVal;
        GLWindowContext* ctx = getGLContext(hwnd);
        if (!ctx) return TzdValue();
        std::vector<TzdValue> res = { TzdValue((double)ctx->width), TzdValue((double)ctx->height) };
        return TzdValue(res);
    });

    reg("gl_set_window_title", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string title = TzdInterpreter::getAsString(args[1]);
        std::wstring wTitle = toWide(title);
        SetWindowTextW(hwnd, wTitle.c_str());
        return TzdValue(true);
    });

    // -------------------------------------------------------------
    // Core State & Buffers
    // -------------------------------------------------------------
    reg("gl_clear_color", [](const std::vector<TzdValue>& args) -> TzdValue {
        float r = (args.size() > 0) ? (float)TzdInterpreter::getAsDoubleInternal(args[0]) : 0.0f;
        float g = (args.size() > 1) ? (float)TzdInterpreter::getAsDoubleInternal(args[1]) : 0.0f;
        float b = (args.size() > 2) ? (float)TzdInterpreter::getAsDoubleInternal(args[2]) : 0.0f;
        float a = (args.size() > 3) ? (float)TzdInterpreter::getAsDoubleInternal(args[3]) : 1.0f;
        glClearColor(r, g, b, a);
        return TzdValue();
    });

    reg("gl_clear", [](const std::vector<TzdValue>& args) -> TzdValue {
        GLbitfield mask = (args.size() > 0) ? (GLbitfield)TzdInterpreter::getAsDoubleInternal(args[0]) : (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClear(mask);
        return TzdValue();
    });

    reg("gl_viewport", [](const std::vector<TzdValue>& args) -> TzdValue {
        int x = (args.size() > 0) ? (int)TzdInterpreter::getAsDoubleInternal(args[0]) : 0;
        int y = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 0;
        int w = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 800;
        int h = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 600;
        glViewport(x, y, w, h);
        return TzdValue();
    });

    reg("gl_enable", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glEnable((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_disable", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glDisable((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_depth_func", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glDepthFunc((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_blend_func", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 2) glBlendFunc((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]), (GLenum)TzdInterpreter::getAsDoubleInternal(args[1]));
        return TzdValue();
    });

    reg("gl_cull_face", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glCullFace((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_point_size", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glPointSize((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_line_width", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glLineWidth((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_shade_model", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glShadeModel((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    // -------------------------------------------------------------
    // Matrix & Transformations
    // -------------------------------------------------------------
    reg("gl_matrix_mode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glMatrixMode((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_load_identity", [](const std::vector<TzdValue>&) -> TzdValue {
        glLoadIdentity();
        return TzdValue();
    });

    reg("gl_push_matrix", [](const std::vector<TzdValue>&) -> TzdValue {
        glPushMatrix();
        return TzdValue();
    });

    reg("gl_pop_matrix", [](const std::vector<TzdValue>&) -> TzdValue {
        glPopMatrix();
        return TzdValue();
    });

    reg("gl_translate", [](const std::vector<TzdValue>& args) -> TzdValue {
        double x = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 0.0;
        double y = (args.size() > 1) ? TzdInterpreter::getAsDoubleInternal(args[1]) : 0.0;
        double z = (args.size() > 2) ? TzdInterpreter::getAsDoubleInternal(args[2]) : 0.0;
        glTranslated(x, y, z);
        return TzdValue();
    });

    reg("gl_rotate", [](const std::vector<TzdValue>& args) -> TzdValue {
        double angle = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 0.0;
        double x = (args.size() > 1) ? TzdInterpreter::getAsDoubleInternal(args[1]) : 0.0;
        double y = (args.size() > 2) ? TzdInterpreter::getAsDoubleInternal(args[2]) : 0.0;
        double z = (args.size() > 3) ? TzdInterpreter::getAsDoubleInternal(args[3]) : 1.0;
        glRotated(angle, x, y, z);
        return TzdValue();
    });

    reg("gl_scale", [](const std::vector<TzdValue>& args) -> TzdValue {
        double x = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 1.0;
        double y = (args.size() > 1) ? TzdInterpreter::getAsDoubleInternal(args[1]) : 1.0;
        double z = (args.size() > 2) ? TzdInterpreter::getAsDoubleInternal(args[2]) : 1.0;
        glScaled(x, y, z);
        return TzdValue();
    });

    reg("gl_ortho", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 6) {
            glOrtho(TzdInterpreter::getAsDoubleInternal(args[0]), TzdInterpreter::getAsDoubleInternal(args[1]),
                    TzdInterpreter::getAsDoubleInternal(args[2]), TzdInterpreter::getAsDoubleInternal(args[3]),
                    TzdInterpreter::getAsDoubleInternal(args[4]), TzdInterpreter::getAsDoubleInternal(args[5]));
        }
        return TzdValue();
    });

    reg("gl_perspective", [](const std::vector<TzdValue>& args) -> TzdValue {
        double fovy = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 45.0;
        double aspect = (args.size() > 1) ? TzdInterpreter::getAsDoubleInternal(args[1]) : 1.333333;
        double zNear = (args.size() > 2) ? TzdInterpreter::getAsDoubleInternal(args[2]) : 0.1;
        double zFar = (args.size() > 3) ? TzdInterpreter::getAsDoubleInternal(args[3]) : 1000.0;
        gluPerspective(fovy, aspect, zNear, zFar);
        return TzdValue();
    });

    reg("gl_look_at", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 9) {
            gluLookAt(
                TzdInterpreter::getAsDoubleInternal(args[0]), TzdInterpreter::getAsDoubleInternal(args[1]), TzdInterpreter::getAsDoubleInternal(args[2]),
                TzdInterpreter::getAsDoubleInternal(args[3]), TzdInterpreter::getAsDoubleInternal(args[4]), TzdInterpreter::getAsDoubleInternal(args[5]),
                TzdInterpreter::getAsDoubleInternal(args[6]), TzdInterpreter::getAsDoubleInternal(args[7]), TzdInterpreter::getAsDoubleInternal(args[8])
            );
        }
        return TzdValue();
    });

    // -------------------------------------------------------------
    // Primitives & Geometry Drawing
    // -------------------------------------------------------------
    reg("gl_begin", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) glBegin((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]));
        return TzdValue();
    });

    reg("gl_end", [](const std::vector<TzdValue>&) -> TzdValue {
        glEnd();
        return TzdValue();
    });

    reg("gl_vertex2f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 2) glVertex2f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]));
        return TzdValue();
    });

    reg("gl_vertex3f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3) glVertex3f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[2]));
        return TzdValue();
    });

    reg("gl_color3f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3) glColor3f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[2]));
        return TzdValue();
    });

    reg("gl_color4f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 4) glColor4f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[2]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[3]));
        return TzdValue();
    });

    reg("gl_color3ub", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3) glColor3ub((GLubyte)TzdInterpreter::getAsDoubleInternal(args[0]), (GLubyte)TzdInterpreter::getAsDoubleInternal(args[1]), (GLubyte)TzdInterpreter::getAsDoubleInternal(args[2]));
        return TzdValue();
    });

    reg("gl_tex_coord2f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 2) glTexCoord2f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]));
        return TzdValue();
    });

    reg("gl_normal3f", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3) glNormal3f((GLfloat)TzdInterpreter::getAsDoubleInternal(args[0]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[1]), (GLfloat)TzdInterpreter::getAsDoubleInternal(args[2]));
        return TzdValue();
    });

    // -------------------------------------------------------------
    // Textures & Lighting
    // -------------------------------------------------------------
    reg("gl_gen_texture", [](const std::vector<TzdValue>&) -> TzdValue {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        return TzdValue((double)tex);
    });

    reg("gl_bind_texture", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 2) {
            glBindTexture((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]), (GLuint)TzdInterpreter::getAsDoubleInternal(args[1]));
        }
        return TzdValue();
    });

    reg("gl_tex_parameteri", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3) {
            glTexParameteri((GLenum)TzdInterpreter::getAsDoubleInternal(args[0]), (GLenum)TzdInterpreter::getAsDoubleInternal(args[1]), (GLint)TzdInterpreter::getAsDoubleInternal(args[2]));
        }
        return TzdValue();
    });

    reg("gl_delete_texture", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (!args.empty()) {
            GLuint tex = (GLuint)TzdInterpreter::getAsDoubleInternal(args[0]);
            glDeleteTextures(1, &tex);
        }
        return TzdValue();
    });

    reg("gl_light", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3 && args[2].type == TzdValue::ARRAY) {
            GLenum light = (GLenum)TzdInterpreter::getAsDoubleInternal(args[0]);
            GLenum pname = (GLenum)TzdInterpreter::getAsDoubleInternal(args[1]);
            std::vector<float> params;
            for (const auto& item : args[2].arrVal) params.push_back((float)TzdInterpreter::getAsDoubleInternal(item));
            glLightfv(light, pname, params.data());
        }
        return TzdValue();
    });

    reg("gl_material", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() >= 3 && args[2].type == TzdValue::ARRAY) {
            GLenum face = (GLenum)TzdInterpreter::getAsDoubleInternal(args[0]);
            GLenum pname = (GLenum)TzdInterpreter::getAsDoubleInternal(args[1]);
            std::vector<float> params;
            for (const auto& item : args[2].arrVal) params.push_back((float)TzdInterpreter::getAsDoubleInternal(item));
            glMaterialfv(face, pname, params.data());
        }
        return TzdValue();
    });

    // -------------------------------------------------------------
    // High-Level 3D Shapes & Primitives
    // -------------------------------------------------------------
    reg("gl_draw_cube", [](const std::vector<TzdValue>& args) -> TzdValue {
        float s = (args.size() > 0) ? (float)TzdInterpreter::getAsDoubleInternal(args[0]) * 0.5f : 0.5f;

        glBegin(GL_QUADS);
        // Front Face
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s,  s);
        // Back Face
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s, -s);
        // Top Face
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        // Bottom Face
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
        // Right face
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( s, -s, -s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( s,  s, -s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( s,  s,  s);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( s, -s,  s);
        // Left Face
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-s, -s, -s);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-s, -s,  s);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-s,  s,  s);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-s,  s, -s);
        glEnd();
        return TzdValue();
    });

    reg("gl_draw_colored_cube", [](const std::vector<TzdValue>& args) -> TzdValue {
        float s = (args.size() > 0) ? (float)TzdInterpreter::getAsDoubleInternal(args[0]) * 0.5f : 0.5f;

        glBegin(GL_QUADS);
        // Front (Red)
        glColor3f(0.9f, 0.2f, 0.2f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
        // Back (Green)
        glColor3f(0.2f, 0.8f, 0.2f);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s); glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);
        // Top (Blue)
        glColor3f(0.2f, 0.4f, 0.9f);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s); glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);
        // Bottom (Yellow)
        glColor3f(0.9f, 0.9f, 0.2f);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-s, -s, -s); glVertex3f( s, -s, -s); glVertex3f( s, -s,  s); glVertex3f(-s, -s,  s);
        // Right (Cyan)
        glColor3f(0.2f, 0.9f, 0.9f);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f( s, -s, -s); glVertex3f( s,  s, -s); glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);
        // Left (Magenta)
        glColor3f(0.9f, 0.2f, 0.9f);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-s, -s, -s); glVertex3f(-s, -s,  s); glVertex3f(-s,  s,  s); glVertex3f(-s,  s, -s);
        glEnd();
        return TzdValue();
    });

    reg("gl_draw_sphere", [](const std::vector<TzdValue>& args) -> TzdValue {
        double r = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 1.0;
        int lats = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 24;
        int longs = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 24;
        if (lats < 3) lats = 3;
        if (longs < 3) longs = 3;

        for (int i = 0; i <= lats; ++i) {
            double lat0 = M_PI * (-0.5 + (double)(i - 1) / lats);
            double z0 = std::sin(lat0);
            double zr0 = std::cos(lat0);

            double lat1 = M_PI * (-0.5 + (double)i / lats);
            double z1 = std::sin(lat1);
            double zr1 = std::cos(lat1);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= longs; ++j) {
                double lng = 2.0 * M_PI * (double)j / longs;
                double x = std::cos(lng);
                double y = std::sin(lng);

                glNormal3f((GLfloat)(x * zr0), (GLfloat)(y * zr0), (GLfloat)z0);
                glTexCoord2f((GLfloat)j / longs, (GLfloat)(i - 1) / lats);
                glVertex3f((GLfloat)(r * x * zr0), (GLfloat)(r * y * zr0), (GLfloat)(r * z0));

                glNormal3f((GLfloat)(x * zr1), (GLfloat)(y * zr1), (GLfloat)z1);
                glTexCoord2f((GLfloat)j / longs, (GLfloat)i / lats);
                glVertex3f((GLfloat)(r * x * zr1), (GLfloat)(r * y * zr1), (GLfloat)(r * z1));
            }
            glEnd();
        }
        return TzdValue();
    });

    reg("gl_draw_cylinder", [](const std::vector<TzdValue>& args) -> TzdValue {
        double baseR = (args.size() > 0) ? TzdInterpreter::getAsDoubleInternal(args[0]) : 1.0;
        double topR = (args.size() > 1) ? TzdInterpreter::getAsDoubleInternal(args[1]) : 1.0;
        double height = (args.size() > 2) ? TzdInterpreter::getAsDoubleInternal(args[2]) : 2.0;
        int slices = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 24;

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slices; ++i) {
            double angle = 2.0 * M_PI * i / slices;
            double ca = std::cos(angle);
            double sa = std::sin(angle);

            glNormal3f((GLfloat)ca, 0.0f, (GLfloat)sa);
            glTexCoord2f((GLfloat)i / slices, 0.0f);
            glVertex3f((GLfloat)(baseR * ca), 0.0f, (GLfloat)(baseR * sa));

            glTexCoord2f((GLfloat)i / slices, 1.0f);
            glVertex3f((GLfloat)(topR * ca), (GLfloat)height, (GLfloat)(topR * sa));
        }
        glEnd();
        return TzdValue();
    });

    reg("gl_draw_grid", [](const std::vector<TzdValue>& args) -> TzdValue {
        float size = (args.size() > 0) ? (float)TzdInterpreter::getAsDoubleInternal(args[0]) : 10.0f;
        float step = (args.size() > 1) ? (float)TzdInterpreter::getAsDoubleInternal(args[1]) : 1.0f;
        if (step <= 0.0f) step = 1.0f;

        glBegin(GL_LINES);
        for (float i = -size; i <= size; i += step) {
            if (std::abs(i) < 1e-4f) glColor3f(0.5f, 0.5f, 0.6f);
            else glColor3f(0.25f, 0.25f, 0.32f);

            glVertex3f(i, 0.0f, -size);
            glVertex3f(i, 0.0f,  size);

            glVertex3f(-size, 0.0f, i);
            glVertex3f( size, 0.0f, i);
        }
        glEnd();
        return TzdValue();
    });

    reg("gl_draw_axes", [](const std::vector<TzdValue>& args) -> TzdValue {
        float len = (args.size() > 0) ? (float)TzdInterpreter::getAsDoubleInternal(args[0]) : 2.0f;
        glBegin(GL_LINES);
        // X Axis (Red)
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(len, 0.0f, 0.0f);
        // Y Axis (Green)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, len, 0.0f);
        // Z Axis (Blue)
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, len);
        glEnd();
        return TzdValue();
    });

    reg("gl_save_screenshot", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        std::string path = TzdInterpreter::getAsString(args[0]);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int w = viewport[2];
        int h = viewport[3];
        if (w <= 0 || h <= 0) return TzdValue(false);

        int rowStride = (w * 3 + 3) & ~3;
        std::vector<unsigned char> pixels(rowStride * h, 0);

        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels.data());

        BITMAPFILEHEADER bfh;
        ZeroMemory(&bfh, sizeof(bfh));
        bfh.bfType = 0x4D42; // "BM"
        bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (DWORD)(rowStride * h);
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        BITMAPINFOHEADER bih;
        ZeroMemory(&bih, sizeof(bih));
        bih.biSize = sizeof(bih);
        bih.biWidth = w;
        bih.biHeight = h;
        bih.biPlanes = 1;
        bih.biBitCount = 24;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = rowStride * h;

        FILE* f = nullptr;
        fopen_s(&f, path.c_str(), "wb");
        if (!f) return TzdValue(false);
        fwrite(&bfh, sizeof(bfh), 1, f);
        fwrite(&bih, sizeof(bih), 1, f);
        fwrite(pixels.data(), 1, pixels.size(), f);
        fclose(f);
        return TzdValue(true);
    });
}
