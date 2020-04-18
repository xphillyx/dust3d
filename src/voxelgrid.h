#ifndef DUST3D_VOXEL_GRID_H
#define DUST3D_VOXEL_GRID_H
#include <QtGlobal>
#include <map>

template<typename T>
class VoxelGrid
{
public:
    struct Voxel
    {
        qint8 x;
        qint8 y;
        qint8 z;
        
        bool operator<(const Voxel &right) const
        {
            if (x < right.x)
                return true;
            if (x > right.x)
                return false;
            if (y < right.y)
                return true;
            if (y > right.y)
                return false;
            if (z < right.z)
                return true;
            if (z > right.z)
                return false;
            return false;
        }
        
        bool operator==(const Voxel &right) const
        {
            return x == right.x &&
                y == right.y &&
                z == right.z;
        }
    };
    
    T query(qint8 x, qint8 y, qint8 z)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return T();
        return findResult->second;
    }
    
    T add(qint8 x, qint8 y, qint8 z, T value)
    {
        auto insertResult = m_grid.insert(std::make_pair(Voxel {x, y, z}, value));
        if (insertResult.second)
            return value;
        insertResult.first->second = insertResult.first->second + value;
        return insertResult.first->second;
    }
    
    T sub(qint8 x, qint8 y, qint8 z, T value)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return T();
        findResult->second = findResult->second - value;
        if (findResult->second == T()) {
            m_grid.erase(findResult);
            return T();
        }
        return findResult->second;
    }
    
    void reset(qint8 x, qint8 y, qint8 z)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return;
        m_grid.erase(findResult);
    }
    
private:
    std::map<Voxel, T> m_grid;
};

#endif