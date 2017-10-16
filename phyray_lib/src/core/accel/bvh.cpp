#include <core/phyr.h>
#include <core/phyr_mem.h>
#include <core/accel/bvh.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace phyr {

void AccelBVH::constructBVH() {
    if (objectList.size() == 0) return;

    // Initiate object info list
    size_t sz = objectList.size();
    std::vector<BVHObjectInfo> objectInfoList(sz);
    for (size_t i = 0; i < sz; i++) {
        objectInfoList[i] = { i, objectList[i]->worldBounds() };
    }

    // Create a BVH Tree from the object info list
    int nodeCount = 0;
    // Memory pool with a block size of 1MB
    MemoryPool pool(1024 * 1024);
    // Stores the ordered permutation of objectList as defined by the recursive BVH algorithm
    std::vector<std::shared_ptr<Object>> orderedObjectList;

    // Recursively build the BVH Tree
    BVHTreeNode* root = constructBVHRecursive(pool, objectInfoList, 0, sz,
                                              &nodeCount, orderedObjectList);
    objectList.swap(orderedObjectList);
}

constexpr int nBins = 12;
typedef struct _BinInfo BinInfo;

struct _BinInfo { int freq; Bounds3f bounds; };

BVHTreeNode*
AccelBVH::constructBVHRecursive(MemoryPool& pool, std::vector<BVHObjectInfo>& objectInfoList,
                                int startIdx, int end, int* nodeCount,
                                std::vector<std::shared_ptr<Object>>& orderedObjectList) const {
    // Check for trivial invalid case
    if (end - startIdx <= 0) return nullptr;

    (*nodeCount)++;
    BVHTreeNode* node = pool.alloc<BVHTreeNode>();

    // Build bounds of all the objects within this range
    Bounds3f nodeBound;
    for (int i = startIdx; i < end; i++)
        nodeBound = unionBounds(nodeBound, objectInfoList[i].bounds);

#define CREATE_LEAF_NODE() \
    int offset = orderedObjectList.size(); \
    for (int i = startIdx; i < end; i++) { \
        int idx = objectInfoList[startIdx].objectIdx; \
        orderedObjectList.push_back(objectList[idx]); \
    } \
    node->createLeafNode(offset, range, nodeBound); \
    return node;

    int range = end - startIdx;
    if (range == 1) {
        CREATE_LEAF_NODE();
    } else {
        Bounds3f centroidBounds;
        for (int i = startIdx; i < end; i++)
            centroidBounds = unionBounds(centroidBounds, objectInfoList[i].centroid);
        int maxDim = centroidBounds.maximumExtent();

        // Check if all centroids lie at the same point
        if (centroidBounds.pMin[maxDim] == centroidBounds.pMax[maxDim]) {
            CREATE_LEAF_NODE();
        } else {
            int mid = (startIdx + end) / 2;

            // Partition primitives
            if (range <= 2) {
                // Partition into equally sized subsets
                std::nth_element(&objectInfoList[startIdx], &objectInfoList[mid],
                                 &objectInfoList[end-1] + 1,
                                 [maxDim](const BVHObjectInfo& a, const BVHObjectInfo& b) {
                                     return a.centroid[maxDim] < b.centroid[maxDim];
                                 });
            } else {
                BinInfo bins[nBins];
                // Initialize bins
                for (int i = startIdx; i < end; i++) {
                    int bidx = centroidBounds.offset(objectInfoList[i].centroid)[maxDim] * nBins;
                    if (bidx == nBins) bidx--; bins[bidx].freq++;
                    bins[bidx].bounds = unionBounds(bins[bidx].bounds, objectInfoList[i].bounds);
                }

                // Compute optimum split
                Real cost, minCost = MaxReal, splitIdx = 0;
                Real nodeBoundSurfaceArea = nodeBound.surfaceArea();

                /* @todo Optimize the O(n^2) runtime */
                for (int i = 0; i < nBins - 1; i++) {
                    Bounds3f bl, br;
                    int f0 = 0, f1 = 0, j;

                    for (j = 0; j <= i; j++) {
                        bl = unionBounds(bl, bins[j].bounds);
                        f0 += bins[j].freq;
                    }

                    for (j = i + 1; j < nBins; j++) {
                        br = unionBounds(br, bins[j].bounds);
                        f1 += bins[j].freq;
                    }

                    // Assumed cost of intersection = 1; cost of traversal = 1/8
                    cost = .125f + (f0 * bl.surfaceArea() + f1 * br.surfaceArea()) / nodeBoundSurfaceArea;
                    if (cost < minCost) { minCost = cost; splitIdx = i; }
                }

                // Since cost of intersection is 1, cost of a leaf is {range}
                Real leafCost = range;
                if (range > maxObjectsPerNode || minCost < leafCost) {
                    // Split objects around optimum split
                    BVHObjectInfo* nmid = std::partition(
                                &objectInfoList[startIdx], &objectInfoList[end-1] + 1,
                                [=](const BVHObjectInfo& info) {
                                    int bidx = centroidBounds.offset(info.centroid)[maxDim] * nBins;
                                    if (bidx == nBins) bidx--;
                                    return bidx <= splitIdx;
                                });
                    mid = nmid - &objectInfoList[0];
                } else {
                    CREATE_LEAF_NODE();
                }
            }

            BVHTreeNode* lc = constructBVHRecursive(pool, objectInfoList, startIdx, mid,
                                                    nodeCount, orderedObjectList);
            BVHTreeNode* rc = constructBVHRecursive(pool, objectInfoList, mid, end,
                                                    nodeCount, orderedObjectList);
            node->createInteriorNode(maxDim, lc, rc);
        }
    }

#undef CREATE_LEAF_NODE

    return node;
}

}  // namespace phyr

#pragma GCC diagnostic pop
