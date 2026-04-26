#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <thread>
#include <DbgHelp.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <Psapi.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Psapi.lib")

#include "sdk/sdk.hpp"

#ifndef DNA_BUILD_ID
#define DNA_BUILD_ID __DATE__ " " __TIME__
#endif
//#include "utils/xor.hpp"
#include "menu/menu.hpp"
#include "features/scripts/scripts.hpp"
#include "features/events/events.hpp"
#include "hooks/hooks.hpp"
#include "menu/config/config.hpp"
#include "features/misc/misc.hpp"
#include "sdk/netvars/netvars.hpp"
#include "features/skins/skins.hpp"
#include "features/movement/movement.hpp"
#include "includes/discord/discord_rpc.h"

namespace {
    bool can_read_address(const void* address, SIZE_T bytes = 1) {
        if (!address || !bytes)
            return false;

        const char* base = static_cast<const char*>(address);
        const char* end_inclusive = base + bytes - 1;

        auto query_ok = [](const void* p, MEMORY_BASIC_INFORMATION& mbi) -> bool {
            if (!VirtualQuery(p, &mbi, sizeof(mbi)))
                return false;
            if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS))
                return false;

            const DWORD read_mask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            return (mbi.Protect & read_mask) != 0;
        };

        MEMORY_BASIC_INFORMATION mbi{};
        if (!query_ok(base, mbi))
            return false;

        const char* region_end = static_cast<const char*>(mbi.BaseAddress) + mbi.RegionSize - 1;
        if (end_inclusive > region_end) {
            // Range spans regions; require tail to be readable too.
            MEMORY_BASIC_INFORMATION mbi2{};
            if (!query_ok(end_inclusive, mbi2))
                return false;
        }

        return true;
    }

    std::string current_timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_s(&tm_buf, &t);

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    std::string format_exception_type(DWORD code) {
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: return "Access Violation";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array Bounds Exceeded";
        case EXCEPTION_BREAKPOINT: return "Breakpoint";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype Misalignment";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float Divide By Zero";
        case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal Instruction";
        case EXCEPTION_IN_PAGE_ERROR: return "In Page Error";
        case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer Divide By Zero";
        case EXCEPTION_PRIV_INSTRUCTION: return "Privileged Instruction";
        case EXCEPTION_STACK_OVERFLOW: return "Stack Overflow";
        default: return "Unknown";
        }
    }

    void log_hex_dump(std::ofstream& log_file, const char* title, const void* ptr, size_t len, size_t max_bytes = 256) {
        log_file << title << " (" << std::dec << len << " bytes, showing " << (std::min)(len, max_bytes) << ")\n";
        if (!ptr || !len) {
            log_file << "  <null or zero length>\n";
            return;
        }

        const auto* p = static_cast<const unsigned char*>(ptr);
        size_t n = (std::min)(len, max_bytes);
        for (size_t i = 0; i < n; i += 16) {
            log_file << "  " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uintptr_t>(p + i) << ": ";
            for (size_t j = 0; j < 16; ++j) {
                if (i + j < n)
                    log_file << std::setw(2) << std::setfill('0') << int(p[i + j]) << " ";
                else
                    log_file << "   ";
            }
            log_file << " |";
            for (size_t j = 0; j < 16 && i + j < n; ++j) {
                unsigned char c = p[i + j];
                log_file << (c >= 32 && c < 127 ? char(c) : '.');
            }
            log_file << "|\n";
        }
    }

    void log_loaded_modules(std::ofstream& log_file, size_t max_modules = 80) {
        log_file << "\nLoaded Modules (truncated):\n";

        HMODULE mods[1024]{};
        DWORD needed = 0;
        if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed)) {
            log_file << "  <EnumProcessModules failed>\n";
            return;
        }

        const UINT count = needed / sizeof(HMODULE);
        const UINT limit = static_cast<UINT>((std::min)(size_t(count), max_modules));
        for (UINT i = 0; i < limit; ++i) {
            char name[MAX_PATH]{};
            MODULEINFO mi{};
            if (!GetModuleInformation(GetCurrentProcess(), mods[i], &mi, sizeof(mi)))
                continue;
            if (!GetModuleFileNameA(mods[i], name, MAX_PATH))
                continue;

            log_file << "  " << std::setw(3) << std::dec << i << ": 0x" << std::hex << std::uppercase
                << reinterpret_cast<uintptr_t>(mi.lpBaseOfDll) << " size=0x" << mi.SizeOfImage << " "
                << name << "\n";
        }
        if (count > limit)
            log_file << "  ... (" << std::dec << (count - limit) << " more)\n";
    }
}

extern "C" LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exception_info) {
    if (!exception_info || !exception_info->ExceptionRecord || !exception_info->ContextRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD exception_code = exception_info->ExceptionRecord->ExceptionCode;

    // Filter out noisy debug print exceptions and other non-critical ones
    if (exception_code == 0x40010006 || exception_code == 0x4001000A || 
        exception_code == 0x4000001F || exception_code == 0x80000003) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::filesystem::create_directories("C:/dna/crash");

    const std::string stamp = current_timestamp();
    const DWORD pid = GetCurrentProcessId();
    const DWORD tid = GetCurrentThreadId();
    const std::string file_base = "C:/dna/crash/crash_" + stamp + "_pid" + std::to_string(pid) + "_tid" + std::to_string(tid);
    const std::string log_path = file_base + ".txt";
    const std::string dump_path = file_base + ".dmp";

    std::ofstream log_file(log_path, std::ios::out | std::ios::trunc);
    if (log_file.is_open()) {
        log_file << "--------------------------------------------------\n";
        log_file << "Crash File: " << log_path << "\n";
        log_file << "Dump File: " << dump_path << "\n";
        log_file << "Crash detected at: " << stamp << "\n";
        log_file << "Process ID: " << pid << "  Thread ID: " << tid << "\n";
        log_file << "Exception Code: 0x" << std::hex << std::uppercase << exception_info->ExceptionRecord->ExceptionCode << "\n";
        log_file << "Exception Address: 0x" << std::hex << std::uppercase << exception_info->ExceptionRecord->ExceptionAddress << "\n";
        log_file << "Exception Type: " << format_exception_type(exception_code) << "\n";
        log_file << "Exception Flags: 0x" << std::hex << std::uppercase << exception_info->ExceptionRecord->ExceptionFlags << "\n";
        log_file << "Exception Parameters: " << std::dec << exception_info->ExceptionRecord->NumberParameters << "\n";
        for (DWORD i = 0; i < exception_info->ExceptionRecord->NumberParameters; ++i) {
            log_file << "  Param[" << i << "]: 0x" << std::hex << std::uppercase
                << static_cast<uintptr_t>(exception_info->ExceptionRecord->ExceptionInformation[i]) << "\n";
        }

        log_file << "\nBuild / Process:\n";
        log_file << "  DNA_BUILD_ID: " << DNA_BUILD_ID << "\n";

        char exe_path[MAX_PATH]{};
        if (GetModuleFileNameA(NULL, exe_path, MAX_PATH))
            log_file << "  EXE: " << exe_path << "\n";

        char cwd_buf[MAX_PATH]{};
        DWORD cwd_len = GetCurrentDirectoryA(MAX_PATH, cwd_buf);
        if (cwd_len > 0 && cwd_len < MAX_PATH)
            log_file << "  CWD: " << cwd_buf << "\n";

        log_file << "  CMD: " << GetCommandLineA() << "\n";

        // Get module name from address
        HMODULE h_module;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)exception_info->ExceptionRecord->ExceptionAddress, &h_module)) {
            char module_path[MAX_PATH];
            if (GetModuleFileNameA(h_module, module_path, MAX_PATH)) {
                std::string path(module_path);
                log_file << "Faulting Module: " << path.substr(path.find_last_of("\\/") + 1) << "\n";
                const auto module_base = reinterpret_cast<uintptr_t>(h_module);
                const auto exception_addr = reinterpret_cast<uintptr_t>(exception_info->ExceptionRecord->ExceptionAddress);
                log_file << "Faulting Module Base: 0x" << std::hex << std::uppercase << module_base << "\n";
                log_file << "Faulting Module Offset: 0x" << std::hex << std::uppercase << (exception_addr - module_base) << "\n";
            }
        }

        log_file << "Last Hook: " << g::last_hook << "\n";
        log_file << "Last Event: " << g::last_event << "\n";
        log_file << "Last FrameStage: " << std::dec << g::last_frame_stage << "\n";

        if (g::local) {
            log_file << "Local Player: 0x" << std::hex << (uintptr_t)g::local << " (Team: " << std::dec << g::local->team() << ", Alive: " << (g::local->is_alive() ? "Yes" : "No") << ")\n";
        } else {
            log_file << "Local Player: NULL\n";
        }

        if (interfaces::client_state) {
            log_file << "ClientState: signon_state_count=" << std::dec << interfaces::client_state->signon_state_count
                     << " delta_tick=" << interfaces::client_state->delta_tick
                     << " max_clients=" << interfaces::client_state->max_clients << "\n";
        } else {
            log_file << "ClientState: NULL\n";
        }

        if (interfaces::engine) {
            log_file << "Engine: connected=" << (interfaces::engine->is_connected() ? "Yes" : "No")
                     << " in_game=" << (interfaces::engine->is_in_game() ? "Yes" : "No") << "\n";
        } else {
            log_file << "Engine: NULL\n";
        }

        if (exception_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && exception_info->ExceptionRecord->NumberParameters >= 2) {
            log_file << (exception_info->ExceptionRecord->ExceptionInformation[0] == 0 ? "Read from" : "Write to")
                     << " address 0x" << std::hex << std::uppercase << exception_info->ExceptionRecord->ExceptionInformation[1] << "\n";
        }

        log_file << "Active Mode: " << menu::menu_variant_profile(menu::active_menu_variant()).display_name << "\n";

        log_file << "Watermark Nickname: " << c::movement::watermark_nickname << "\n";

        log_file << "\nKey Interfaces (pointers):\n";
        log_file << "  client.dll: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll")) << "\n";
        log_file << "  engine.dll: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(GetModuleHandleA("engine.dll")) << "\n";
        log_file << "  tier0.dll: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(GetModuleHandleA("tier0.dll")) << "\n";
        log_file << "  vstdlib.dll: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(GetModuleHandleA("vstdlib.dll")) << "\n";
        log_file << "  interfaces::client: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::client) << "\n";
        log_file << "  interfaces::engine: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::engine) << "\n";
        log_file << "  interfaces::material_system: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::material_system) << "\n";
        log_file << "  interfaces::model_render: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::model_render) << "\n";
        log_file << "  interfaces::key_values_system: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::key_values_system) << "\n";
        log_file << "  interfaces::client_state: 0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(interfaces::client_state) << "\n";

        // Write registers
        log_file << "\nRegisters:\n";
        log_file << "EAX: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Eax << " ";
        log_file << "EBX: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Ebx << " ";
        log_file << "ECX: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Ecx << " ";
        log_file << "EDX: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Edx << "\n";
        log_file << "ESI: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Esi << " ";
        log_file << "EDI: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Edi << " ";
        log_file << "EBP: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Ebp << " ";
        log_file << "ESP: 0x" << std::setw(8) << std::setfill('0') << exception_info->ContextRecord->Esp << "\n";

        // Stack trace
        log_file << "\nStack Trace:\n";
        CONTEXT context = *exception_info->ContextRecord;
        STACKFRAME64 frame = { 0 };
        frame.AddrPC.Offset = context.Eip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = context.Ebp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = context.Esp;
        frame.AddrStack.Mode = AddrModeFlat;

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(process, NULL, TRUE);

        for (int i = 0; i < 20; ++i) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, thread, &frame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                break;

            log_file << "  " << i << ": 0x" << std::hex << std::uppercase << frame.AddrPC.Offset;

            char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
                log_file << " [" << symbol->Name << "+0x" << std::hex << displacement << "]";
            }

            IMAGEHLP_MODULE64 module_info = { 0 };
            module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
            if (SymGetModuleInfo64(process, frame.AddrPC.Offset, &module_info)) {
                log_file << " in " << module_info.ModuleName;
            }
            log_file << "\n";
        }

        log_file << "\nStack Memory (ESP):\n";
        const auto* esp_ptr = reinterpret_cast<uintptr_t*>(exception_info->ContextRecord->Esp);
        for (int i = 0; i < 48; ++i) {
            const void* addr = esp_ptr + i;
            log_file << "  [ESP+" << std::hex << std::uppercase << (i * sizeof(uintptr_t)) << "] @" << addr;
            if (can_read_address(addr)) {
                log_file << " = 0x" << std::hex << std::uppercase << *(esp_ptr + i) << "\n";
            }
            else {
                log_file << " = <unreadable>\n";
            }
        }

        log_file << "\nCode Bytes @ EIP (best effort):\n";
        {
            const void* eip = reinterpret_cast<const void*>(exception_info->ContextRecord->Eip);
            if (can_read_address(eip, 64))
                log_hex_dump(log_file, "EIP", eip, 64, 64);
            else
                log_file << "  <unreadable>\n";
        }

        log_file << "\nCode Bytes @ faulting ExceptionAddress (best effort):\n";
        {
            const void* fa = exception_info->ExceptionRecord->ExceptionAddress;
            if (can_read_address(fa, 64))
                log_hex_dump(log_file, "FA", fa, 64, 64);
            else
                log_file << "  <unreadable>\n";
        }

        log_loaded_modules(log_file, 96);

        SymCleanup(process);
        log_file << "--------------------------------------------------\n\n";
        log_file.close();
    }

    // Also try to create a minidump
    HANDLE dump_file = CreateFileA(dump_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dump_file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dump_exception_info;
        dump_exception_info.ThreadId = tid;
        dump_exception_info.ExceptionPointers = exception_info;
        dump_exception_info.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            pid,
            dump_file,
            static_cast<MINIDUMP_TYPE>(
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                // MINIDUMP_TYPE flags vary slightly by SDK; use numeric masks for compatibility.
                static_cast<MINIDUMP_TYPE>(0x00020000) // indirectly referenced memory
            ),
            &dump_exception_info,
            NULL,
            NULL);
        CloseHandle(dump_file);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static bool bUnloading = false;

static DWORD WINAPI on_attach(void* instance) {
    SetUnhandledExceptionFilter(CrashHandler);
    srand(static_cast<unsigned int>(time(NULL)));
    [&]() {	
        interfaces::initialize();
        sdk::hooks::init();
        c::create_directory();
        cvar::init();
        route = std::make_unique<savingroute>("records");
        mplayer.sessionManager.reset();
        mplayer.session.reset();
        mplayer.thumbnail.reset();
        c_discord::get().update();
        interfaces::console->console_color_printf({ 242, 242, 242, 255 }, ("[dna] "));
        interfaces::console->console_printf(std::string("Movement Extention. Version [Beta]").append(" \n").c_str());
        }();

        while (!GetAsyncKeyState(VK_END)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        bUnloading = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(600)); // wait for thread to exit

        sdk::hooks::unload();
        FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 1);
        return 0;
}

static DWORD WINAPI GetNowPlayingInfoAndSaveAlbumArt(void* instance)
{
    while (!bUnloading) {
        if (!c::misc::show_spotify_currently_playing) {
            Sleep(500);
            continue;
        }
        
        mplayer.Update(interfaces::device);
        Sleep(500);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread1 = CreateThread(
            nullptr,
            0,
            on_attach,
            hModule,
            0,
            nullptr
        );

        HANDLE hThread2 = CreateThread(
            nullptr,
            0,
            GetNowPlayingInfoAndSaveAlbumArt,
            hModule,
            0,
            nullptr
        );

        // Close both handles safely
        if (hThread1)
            CloseHandle(hThread1);

        if (hThread2)
            CloseHandle(hThread2);
    }

    return TRUE;
}
