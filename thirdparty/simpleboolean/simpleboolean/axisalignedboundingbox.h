#ifndef SIMPLEBOOLEAN_AXIS_ALIGNED_BOUNDING_BOX_H
#define SIMPLEBOOLEAN_AXIS_ALIGNED_BOUNDING_BOX_H
#include <simpleboolean/meshdatatype.h>

namespace simpleboolean
{
  
class AxisAlignedBoudingBox
{
public:
    void update(const Vertex &vertex);
    const Vertex &lowerBound() const;
    const Vertex &upperBound() const;
    Vertex &lowerBound();
    Vertex &upperBound();
    bool intersectWith(const AxisAlignedBoudingBox &other) const;
    bool intersectWithAt(const AxisAlignedBoudingBox &other, AxisAlignedBoudingBox *result) const;
    const Vertex &center() const;
    void updateCenter();
    
private:
    Vertex m_min = {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };
    Vertex m_max = {
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
    };
    Vertex m_sum;
    size_t m_num = 0;
    Vertex m_center;
};
  
}


#endif
