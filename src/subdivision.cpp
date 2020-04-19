#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/far/primvarRefiner.h>
#include "subdivision.h"

using namespace OpenSubdiv;

struct Vertex {

    // Minimal required interface ----------------------
    Vertex() { }

    Vertex(Vertex const & src) {
        _position[0] = src._position[0];
        _position[1] = src._position[1];
        _position[2] = src._position[2];
    }

    void Clear( void * =0 ) {
        _position[0]=_position[1]=_position[2]=0.0f;
    }

    void AddWithWeight(Vertex const & src, float weight) {
        _position[0]+=weight*src._position[0];
        _position[1]+=weight*src._position[1];
        _position[2]+=weight*src._position[2];
    }

    // Public interface ------------------------------------
    void SetPosition(float x, float y, float z) {
        _position[0]=x;
        _position[1]=y;
        _position[2]=z;
    }

    const float * GetPosition() const {
        return _position;
    }

private:
    float _position[3];
};

void subdivision(const std::vector<QVector3D> &vertices, 
    const std::vector<std::vector<size_t>> &faces,
    std::vector<QVector3D> *outputVertices,
    std::vector<std::vector<size_t>> *outputFaces)
{
    std::vector<int> vertsPerFace(faces.size());
    for (size_t i = 0; i < faces.size(); ++i)
        vertsPerFace[i] = (int)faces[i].size();
    std::vector<int> vertIndices;
    for (const auto &it: faces) {
        for (const auto &index: it)
            vertIndices.push_back((int)index);
    }
    
    typedef Far::TopologyDescriptor Descriptor;

    Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

    Sdc::Options options;
    options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);

    Descriptor desc;
    desc.numVertices  = vertices.size();
    desc.numFaces     = faces.size();
    desc.numVertsPerFace = vertsPerFace.data();
    desc.vertIndicesPerFace  = vertIndices.data();


    // Instantiate a Far::TopologyRefiner from the descriptor
    Far::TopologyRefiner * refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc,
                                            Far::TopologyRefinerFactory<Descriptor>::Options(type, options));

    int maxlevel = 2;

    // Uniformly refine the topology up to 'maxlevel'
    refiner->RefineUniform(Far::TopologyRefiner::UniformOptions(maxlevel));


    // Allocate a buffer for vertex primvar data. The buffer length is set to
    // be the sum of all children vertices up to the highest level of refinement.
    std::vector<Vertex> vbuffer(refiner->GetNumVerticesTotal());
    Vertex * verts = &vbuffer[0];


    // Initialize coarse mesh positions
    int nCoarseVerts = vertices.size();
    for (int i = 0; i < nCoarseVerts; ++i) {
        const auto &src = vertices[i];
        verts[i].SetPosition(src.x(), src.y(), src.z());
    }


    // Interpolate vertex primvar data
    Far::PrimvarRefiner primvarRefiner(*refiner);

    Vertex * src = verts;
    for (int level = 1; level <= maxlevel; ++level) {
        Vertex * dst = src + refiner->GetLevel(level-1).GetNumVertices();
        primvarRefiner.Interpolate(level, src, dst);
        src = dst;
    }


    { // Output OBJ of the highest level refined -----------

        Far::TopologyLevel const & refLastLevel = refiner->GetLevel(maxlevel);

        int nverts = refLastLevel.GetNumVertices();
        int nfaces = refLastLevel.GetNumFaces();

        // Print vertex positions
        int firstOfLastVerts = refiner->GetNumVerticesTotal() - nverts;

        for (int vert = 0; vert < nverts; ++vert) {
            float const * pos = verts[firstOfLastVerts + vert].GetPosition();
            outputVertices->push_back(QVector3D(pos[0], pos[1], pos[2]));
            //printf("v %f %f %f\n", pos[0], pos[1], pos[2]);
        }

        // Print faces
        for (int face = 0; face < nfaces; ++face) {

            Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);

            // all refined Catmark faces should be quads
            assert(fverts.size()==4);
            
            std::vector<size_t> newFace;
            //printf("f ");
            for (int vert=0; vert<fverts.size(); ++vert) {
                newFace.push_back(fverts[vert]);
                //printf("%d ", fverts[vert]+1); // OBJ uses 1-based arrays...
            }
            outputFaces->push_back(newFace);
            //printf("\n");
        }
    }
}