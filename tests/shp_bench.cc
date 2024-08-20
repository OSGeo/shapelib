#include <array>
#include <filesystem>

#include <benchmark/benchmark.h>
#include "shapefil.h"

namespace fs = std::filesystem;
namespace bm = benchmark;

namespace
{

static const auto kTestData = fs::path{"shape_eg_data"};

constexpr static auto filenames = std::array<const char *, 11>{
    "3dpoints.shp", "anno.shp",     "brklinz.shp", "CoHI_GCS12.shp",
    "csah.shp",     "masspntz.shp", "mpatch3.shp", "multipatch.shp",
    "multipnt.shp", "pline.shp",    "polygon.shp"};

static void Benchmark_ReadAll(bm::State &state)
{
    const auto filename = kTestData / filenames[state.range(1)];
    const auto handle = SHPOpen(filename.string().c_str(), "rb");
    SHPSetFastModeReadObject(handle, static_cast<int>(state.range(0)));
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
    state.SetItemsProcessed(state.iterations() * nEntities);
    SHPClose(handle);
}

BENCHMARK(Benchmark_ReadAll)
    ->ArgsProduct({{0, 1}, bm::CreateDenseRange(0, filenames.size() - 1, 1)})
    ->ArgNames({"fastMode", "fileId"})
    ->Unit(bm::kMicrosecond);

}  // namespace

BENCHMARK_MAIN();
