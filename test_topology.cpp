#include <stdio.h>
#include <kernel/OS.h>

int main() {
    printf("test_topology - entry\n");
    cpu_topology_node_info* topology = NULL;
    uint32_t topologyNodeCount = 0;
    if (get_cpu_topology_info(NULL, &topologyNodeCount) == B_OK && topologyNodeCount > 0) {
        printf("test_topology - topologyNodeCount: %u\n", topologyNodeCount);
        topology = new cpu_topology_node_info[topologyNodeCount];
        if (topology == NULL) {
            printf("test_topology - failed to allocate memory for cpu_topology_node_info\n");
            return 1;
        }
        uint32_t actualNodeCount = topologyNodeCount;
        if (get_cpu_topology_info(topology, &actualNodeCount) == B_OK) {
            printf("test_topology - got topology info\n");
        } else {
            printf("test_topology - failed to get topology info\n");
        }
        delete[] topology;
    } else {
        printf("test_topology - failed to get topology node count\n");
    }
    printf("test_topology - finished\n");
    return 0;
}
