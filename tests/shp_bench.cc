#include <filesystem>

#include <benchmark/benchmark.h>
#include "shapefil.h"

namespace fs = std::filesystem;

namespace
{

static const auto kTestData = fs::path{"shape_eg_data"};

template <int fastMode> static void Benchmark_ReadAll(benchmark::State &state)
{
    const auto filename = kTestData / "mpatch3.shp";
    const auto handle = SHPOpen(filename.string().c_str(), "rb");
    SHPSetFastModeReadObject(handle, fastMode);
    int nEntities;
    SHPGetInfo(handle, &nEntities, nullptr, nullptr, nullptr);
    for (auto _ : state)
    {
        for (int i = 0; i < nEntities; ++i)
        {
            auto obj = SHPReadObject(handle, i);
            SHPDestroyObject(obj);
        }
    }
    SHPClose(handle);
}

constexpr auto Benchmark_ReadAllSlow = Benchmark_ReadAll<0>;
constexpr auto Benchmark_ReadAllFast = Benchmark_ReadAll<1>;

BENCHMARK(Benchmark_ReadAllSlow);
BENCHMARK(Benchmark_ReadAllFast);

}  // namespace

BENCHMARK_MAIN();
