#include <Windows.h>
#include <WinUser.h>
#include <iostream>
#include <bitset>

struct io_state_t {
    char p1_ul : 1;
    char p1_dl : 1;
    char p1_ur : 1;
    char p1_dr : 1;
    char p1_cn : 1;
    char pad0 : 3;
    char pad1[0x12];
    char p2_ul : 1;
    char p2_dl : 1;
    char p2_ur : 1;
    char pad3 : 1;
    char p2_dr : 1;
    char p2_cn : 1;
    char pad2 : 2;
};

struct other_io_state_t {
    char pad0 : 1;
    char service : 1;
};

template <typename T>
T get(uint32_t offset) {
    static uintptr_t base = (uintptr_t)GetModuleHandle(NULL);
    return (T)(base + offset);
}

void add_coin() {
    static auto play_sound = get<bool(*)(uint32_t, int)>(0xBFA0);
    static auto coin_sound = get<uint32_t*>(0x19428);
    static auto coin_count = get<uint32_t*>(0x1AB24);

    if (*coin_count < 9) {
        play_sound(*coin_sound, 0);
        (*coin_count)++;
    }
}

WNDPROC o_proc = nullptr;
LRESULT __stdcall WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static io_state_t* state = get<io_state_t*>(0x1AB29);
    static other_io_state_t* other_state = get<other_io_state_t*>(0x1AB28);

    // Game stays running even after you exit the window.
    if (uMsg == WM_CLOSE)
        ExitProcess(0);

    if (uMsg != WM_KEYDOWN && uMsg != WM_SYSKEYDOWN && uMsg != WM_KEYUP)
        return CallWindowProcA(o_proc, hWnd, uMsg, wParam, lParam);

    bool enabled = uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN;

    // Repeated key
    if (enabled && (lParam & 0x40000000) != 0) {
        return CallWindowProcA(o_proc, hWnd, uMsg, wParam, lParam);
    }

    switch (wParam) {
    case 'Q':
        state->p1_ul = enabled;
        break;
    case 'E':
        state->p1_ur = enabled;
        break;
    case 'S':
        state->p1_cn = enabled;
        break;
    case 'Z':
        state->p1_dl = enabled;
        break;
    case 'C':
        state->p1_dr = enabled;
        break;
    case VK_NUMPAD7:
        state->p2_ul = enabled;
        break;
    case VK_NUMPAD9:
        state->p2_ur = enabled;
        break;
    case VK_NUMPAD5:
        state->p2_cn = enabled;
        break;
    case VK_NUMPAD1:
        state->p2_dl = enabled;
        break;
    case VK_NUMPAD3:
        state->p2_dr = enabled;
        break;
    case VK_F1:
        if (enabled)
            add_coin();
        break;
    case VK_F2:
        other_state->service = enabled;
        break;
    }

    return CallWindowProcA(o_proc, hWnd, uMsg, wParam, lParam);
}

void patch_input_reads() {
    static uintptr_t func = get<uintptr_t>(0xA6E0);
    static int inst_offsets[] = {
        // Player 1
        0xAB,
        0xCB,
        0xE8,
        0x108,
        0x125,
        0x145,
        0x162,
        0x182,
        0x19F,
        0x1BF,

        // Player 2
        0x2E,
        0x1EB,
        0x203,
        0x220,
        0x240,
        0x25D,
        0x27D,
        0x29A,
        0x2BA,
        0x2D7,
        0x2F7,
    };

    VirtualProtect((void*)func, 4096, PAGE_EXECUTE_READWRITE, NULL);

    for (int i = 0; i < 21; i++) {
        memset((void*)(func + inst_offsets[i]), 0x90, 6);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        HWND window = NULL;

        while (!window) {
            window = FindWindowA("Gangster", NULL);
            Sleep(100);
        }

        o_proc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

        patch_input_reads();
    }

    return TRUE;
}

