#pragma once

namespace components
{
	class rtx_fixed_function final : public component
	{
	public:
		rtx_fixed_function();
		const char* get_name() override { return "rtx_fixed_function"; };

		static inline IDirect3DVertexBuffer9* dynamic_codemesh_vb = nullptr;

		static void copy_fx_buffer();
	};
}
