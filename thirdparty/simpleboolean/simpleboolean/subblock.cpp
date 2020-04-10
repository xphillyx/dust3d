#include <simpleboolean/subblock.h>
#include <QDebug>
#include <queue>
#include <array>

namespace simpleboolean
{

bool SubBlock::createSubBlocks(const std::vector<SubSurface> &firstSubSurfaces,
        const std::vector<SubSurface> &secondSubSurfaces,
        std::vector<SubBlock> &subBlocks)
{
    if (firstSubSurfaces.empty())
        return false;
    
    using SubSurfaceNameMap = std::map<size_t, std::array<std::array<const SubSurface *, 2>, 2>>;
    
    auto buildSubSurfaceNameMap = [](const std::vector<SubSurface> &subSurfaces,
            size_t sourceMesh,
            SubSurfaceNameMap &nameMap) {
        for (const auto &subSurface: subSurfaces) {
            if (subSurface.isFrontSide)
                nameMap[subSurface.edgeLoopIndex][sourceMesh][0] = &subSurface;
            else
                nameMap[subSurface.edgeLoopIndex][sourceMesh][1] = &subSurface;
        }
    };
    SubSurfaceNameMap subSurfaceNameMap;
    buildSubSurfaceNameMap(firstSubSurfaces, 0, subSurfaceNameMap);
    buildSubSurfaceNameMap(secondSubSurfaces, 1, subSurfaceNameMap);
    
    std::map<size_t, std::map<int, bool>> cyclesTemplate;
    std::set<std::pair<size_t, size_t>> visited;
    for (size_t firstSubSurfaceIndex = 0; firstSubSurfaceIndex < firstSubSurfaces.size(); ++firstSubSurfaceIndex) {
        std::queue<std::tuple<size_t, size_t, size_t>> waitCycles;
        const auto &firstSubSurface = firstSubSurfaces[firstSubSurfaceIndex];
        waitCycles.push({firstSubSurface.edgeLoopIndex, 0, firstSubSurface.isFrontSide ? 0 : 1});
        while (!waitCycles.empty()) {
            auto cycle = waitCycles.front();
            waitCycles.pop();
            const auto &cycleName = std::get<0>(cycle);
            const auto &sourceMesh = std::get<1>(cycle);
            if (visited.find(std::make_pair(cycleName, sourceMesh)) != visited.end())
                continue;
            visited.insert(std::make_pair(cycleName, sourceMesh));
            auto oppositeMesh = 0 == sourceMesh ? 1 : 0;
            const auto &sideIndex = std::get<2>(cycle);
            auto &mapItem = subSurfaceNameMap[cycleName];
            const SubSurface *currentSubSurface = mapItem[sourceMesh][sideIndex];
            if (nullptr == currentSubSurface) {
                qDebug() << "Found invalid cycle";
                return false;
            }
            for (const auto &neighbor: currentSubSurface->owners) {
                cyclesTemplate[neighbor.first].insert({sourceMesh, neighbor.second});
                visited.insert(std::make_pair(neighbor.first, sourceMesh));
                auto &neighborMapItem = subSurfaceNameMap[neighbor.first];
                auto neighborSideIndex = neighbor.second ? 0 : 1;
                const SubSurface *neighborSubSurface = neighborMapItem[oppositeMesh][neighborSideIndex];
                if (nullptr == neighborSubSurface)
                    continue;
                for (const auto &neighborNeighbor: neighborSubSurface->owners) {
                    waitCycles.push({neighborNeighbor.first, oppositeMesh, neighborNeighbor.second});
                }
            }
        }
    }
    auto createSubBlockFromTemplate = [&](const std::map<size_t, std::map<int, bool>> &cycles,
            bool flipFirst, bool flipSecond) {
        SubBlock subBlock;
        for (const auto &cycle: cycles) {
            for (const auto &side: cycle.second) {
                const auto &sourceMesh = side.first;
                bool flipSide = (0 == sourceMesh && flipFirst) ||
                        (1 == sourceMesh && flipSecond);
                auto isFrontSide = flipSide ? (!side.second) : side.second;
                const auto &sideIndex = isFrontSide ? 0 : 1;
                auto &mapItem = subSurfaceNameMap[cycle.first];
                const SubSurface *currentSubSurface = mapItem[sourceMesh][sideIndex];
                if (nullptr == currentSubSurface)
                    continue;
                for (const auto &face: currentSubSurface->faces) {
                    subBlock.faces.insert({std::array<size_t, 3> {{face.indices[0], face.indices[1], face.indices[2]}}, sourceMesh});
                }
                subBlock.cycles[cycle.first].insert({sourceMesh, isFrontSide});
            }
        }
        return subBlock;
    };
    subBlocks.push_back(createSubBlockFromTemplate(cyclesTemplate, false, false));
    subBlocks.push_back(createSubBlockFromTemplate(cyclesTemplate, true, false));
    subBlocks.push_back(createSubBlockFromTemplate(cyclesTemplate, false, true));
    subBlocks.push_back(createSubBlockFromTemplate(cyclesTemplate, true, true));
    
    return true;
}

}

