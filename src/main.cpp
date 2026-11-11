// Heading NaN Guard 1.0.1 -- stops a class of permanent freezes in Skyrim SE/AE (public domain, see LICENSE).
//
// The engine's vector -> heading helper (Address Library ID 70172; 1.6.1170 RVA 0xD17400) computes atan(x/y), adds pi when
// y < 0, then wraps the angle into [0, 2pi] with `while (a < 0) a += 2pi;`. Every comparison with NaN is "unordered", so
// a NaN input makes that loop spin forever. It runs on the AI/movement job threads (MovementControllerAI ->
// IMovementPathManagerAgent -> MovementPlannerArbiter -> path follower update, ID 90711), and the main thread then waits
// for that job forever (WaitForJobTask, ID 69516): the game freezes with no crash log. Any mod that hands an actor a NaN
// movement direction can trigger it.
//
// This plugin replaces the helper with the same maths (atan(x / y), +pi when y < 0; y == 0: pi/2 when x > 0, else 3pi/2;
// wrapped into [0, 2pi]) and returns 0 for a NaN/infinite input, logging the caller (Documents/My Games/Skyrim Special
// Edition/SKSE/HeadingNaNGuard.log) so the source of the NaN can be traced. The patch is only applied when the function's
// first bytes match the 1.6.1170 code; otherwise it logs and does nothing.
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <intrin.h>

namespace
{
	constexpr float kPi = 3.1415927f;
	constexpr float kTwoPi = 6.2831855f;
	constexpr float kHalfPi = 1.5707964f;
	constexpr float kThreeHalfPi = 4.712389f;

	std::atomic<std::uint64_t> g_hits{ 0 };
	std::uintptr_t g_base = 0;

	float __fastcall HeadingAngle(const float* a_vec)
	{
		const float x = a_vec[0];
		const float y = a_vec[1];
		if (!std::isfinite(x) || !std::isfinite(y)) [[unlikely]] {
			const auto n = ++g_hits;
			if (n <= 50 || n % 1000 == 0) {
				const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
				SKSE::log::warn("bad vector ({}, {}) from SkyrimSE.exe+{:X} (hit {})", x, y, caller - g_base, n);
			}
			return 0.0f;
		}
		float r;
		if (y == 0.0f) {
			if (x > 0.0f) {
				return kHalfPi;
			}
			r = kThreeHalfPi;
		} else {
			r = std::atan(x / y);
			if (y < 0.0f) {
				r += kPi;
			}
		}
		while (r < 0.0f) {
			r += kTwoPi;
		}
		while (r > kTwoPi) {
			r -= kTwoPi;
		}
		return r;
	}

	void SetupLog()
	{
		auto dir = SKSE::log::log_directory();
		if (!dir) return;
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((*dir / "HeadingNaNGuard.log").string(), true);
		auto log = std::make_shared<spdlog::logger>("log", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
	}

	bool Install()
	{
		g_base = REL::Module::get().base();
		REL::Relocation<std::uintptr_t> target{ REL::ID(70172) };
		// 1.6.1170 prologue: sub rsp,58h / movss xmm0,[rcx] / movaps [rsp+40h],xmm6
		constexpr std::uint8_t expected[] = { 0x48, 0x83, 0xEC, 0x58, 0xF3, 0x0F, 0x10, 0x01, 0x0F, 0x29, 0x74, 0x24, 0x40 };
		if (std::memcmp(reinterpret_cast<const void*>(target.address()), expected, sizeof(expected)) != 0) {
			SKSE::log::error("heading helper at SkyrimSE.exe+{:X} doesn't match the expected code: NOT patched", target.address() - g_base);
			return false;
		}
		SKSE::AllocTrampoline(14);
		SKSE::GetTrampoline().write_branch<5>(target.address(), &HeadingAngle);
		SKSE::log::info("heading helper at SkyrimSE.exe+{:X} replaced (NaN-safe)", target.address() - g_base);
		return true;
	}
}

extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version(1, 0, 1, 0));
	v.PluginName("HeadingNaNGuard");
	v.AuthorName("WhiteLionNeren & Claude");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });
	return v;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "HeadingNaNGuard";
	a_info->version = 1;
	return true;
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, false);
	SetupLog();
	SKSE::log::info("Heading NaN Guard 1.0.1");
	Install();
	return true;
}
