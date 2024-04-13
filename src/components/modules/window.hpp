#pragma once

namespace components
{
	class window final : public component
	{
	public:
		window();
		const char* get_name() override { return "window"; }
		
		static int width();
		static int height();
		static int width(HWND window);
		static int height(HWND window);
		static void dimension(RECT* rect);
		static void dimension(HWND window, RECT* rect);
		static HWND get_window();
		static bool is_noborder();

	private:
		static HWND main_window_;
		static BOOL cursor_visible_;

		static void apply_cursor();

		static BOOL WINAPI message_handler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
		static int WINAPI show_cursor_hk(BOOL show);

		static void style_hook_stub();
		static HWND WINAPI create_main_window(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	};
}
