#include "GraphPartitioner.h"

GraphPartitioner::GraphPartitioner(uint32 num_elements, int32 min_partition_size, int32 max_partition_size) {
}

GraphPartitioner::GraphData* GraphPartitioner::NewGraph(uint32 num_adjacency) const {
    return nullptr;
}

void GraphPartitioner::AddAdjaceny(GraphData* graph, uint32 adj_index, idx_t cost) {
}

void GraphPartitioner::AddLocalityLinks(GraphData* graph, uint32 index, idx_t cost) {
}

void GraphPartitioner::Partition(GraphData* graph) {
}

void GraphPartitioner::ParititionStrict(GraphData* graph, bool enable_threaded) {
}

void GraphPartitioner::BisectGraph(GraphData* graph, GraphData* child_graphs[2]) {
}

void GraphPartitioner::RecursiveBisectGraph(GraphData* graph) {
}
