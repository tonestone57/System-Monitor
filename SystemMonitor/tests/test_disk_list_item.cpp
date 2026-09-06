#include <cassert>
#include <iostream>
#include <cstring>
#include "HaikuMocks.h"
#include "../DiskListItem.h"

BFont* be_bold_font = nullptr;
bool DiskListItem::sSortAscending = true;

void test_compare_device() {
	DiskListItem item1(1, "disk1", "/mnt/1", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);
	DiskListItem item2(2, "disk2", "/mnt/2", "ext4", 200, 100, 100, 50.0, nullptr, nullptr);
	DiskListItem item1_dup(3, "disk1", "/mnt/1", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	assert(DiskListItem::CompareDevice(&p1, &p2) < 0);
	assert(DiskListItem::CompareDevice(&p2, &p1) > 0);
	assert(DiskListItem::CompareDevice(&p1, &p1_dup) == 0);
}

void test_compare_mount() {
	DiskListItem item1(1, "disk1", "/mnt/a", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);
	DiskListItem item2(2, "disk2", "/mnt/b", "ext4", 200, 100, 100, 50.0, nullptr, nullptr);
	DiskListItem item1_dup(3, "disk1", "/mnt/a", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	assert(DiskListItem::CompareMount(&p1, &p2) < 0);
	assert(DiskListItem::CompareMount(&p2, &p1) > 0);
	assert(DiskListItem::CompareMount(&p1, &p1_dup) == 0);
}

void test_compare_fs() {
	DiskListItem item1(1, "disk1", "/mnt/1", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);
	DiskListItem item2(2, "disk2", "/mnt/2", "ext4", 200, 100, 100, 50.0, nullptr, nullptr);
	DiskListItem item1_dup(3, "disk1", "/mnt/1", "bfs", 100, 50, 50, 50.0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	assert(DiskListItem::CompareFS(&p1, &p2) < 0);
	assert(DiskListItem::CompareFS(&p2, &p1) > 0);
	assert(DiskListItem::CompareFS(&p1, &p1_dup) == 0);
}

void test_compare_total() {
	DiskListItem item1(1, "d1", "/m1", "fs", 100, 0, 0, 0, nullptr, nullptr);
	DiskListItem item2(2, "d2", "/m2", "fs", 200, 0, 0, 0, nullptr, nullptr);
	DiskListItem item1_dup(3, "d3", "/m3", "fs", 100, 0, 0, 0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	DiskListItem::sSortAscending = true;
	assert(DiskListItem::CompareTotal(&p1, &p2) < 0);
	assert(DiskListItem::CompareTotal(&p2, &p1) > 0);
	assert(DiskListItem::CompareTotal(&p1, &p1_dup) == 0);

	DiskListItem::sSortAscending = false;
	assert(DiskListItem::CompareTotal(&p1, &p2) > 0);
	assert(DiskListItem::CompareTotal(&p2, &p1) < 0);
}

void test_compare_used() {
	DiskListItem item1(1, "d1", "/m1", "fs", 1000, 100, 0, 0, nullptr, nullptr);
	DiskListItem item2(2, "d2", "/m2", "fs", 1000, 200, 0, 0, nullptr, nullptr);
	DiskListItem item1_dup(3, "d3", "/m3", "fs", 1000, 100, 0, 0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	DiskListItem::sSortAscending = true;
	assert(DiskListItem::CompareUsed(&p1, &p2) < 0);
	assert(DiskListItem::CompareUsed(&p2, &p1) > 0);
	assert(DiskListItem::CompareUsed(&p1, &p1_dup) == 0);

	DiskListItem::sSortAscending = false;
	assert(DiskListItem::CompareUsed(&p1, &p2) > 0);
	assert(DiskListItem::CompareUsed(&p2, &p1) < 0);
}

void test_compare_free() {
	DiskListItem item1(1, "d1", "/m1", "fs", 1000, 0, 100, 0, nullptr, nullptr);
	DiskListItem item2(2, "d2", "/m2", "fs", 1000, 0, 200, 0, nullptr, nullptr);
	DiskListItem item1_dup(3, "d3", "/m3", "fs", 1000, 0, 100, 0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	DiskListItem::sSortAscending = true;
	assert(DiskListItem::CompareFree(&p1, &p2) < 0);
	assert(DiskListItem::CompareFree(&p2, &p1) > 0);
	assert(DiskListItem::CompareFree(&p1, &p1_dup) == 0);

	DiskListItem::sSortAscending = false;
	assert(DiskListItem::CompareFree(&p1, &p2) > 0);
	assert(DiskListItem::CompareFree(&p2, &p1) < 0);
}

void test_compare_usage() {
	DiskListItem item1(1, "d1", "/m1", "fs", 1000, 0, 0, 10.0, nullptr, nullptr);
	DiskListItem item2(2, "d2", "/m2", "fs", 1000, 0, 0, 20.0, nullptr, nullptr);
	DiskListItem item1_dup(3, "d3", "/m3", "fs", 1000, 0, 0, 10.0, nullptr, nullptr);

	const DiskListItem* p1 = &item1;
	const DiskListItem* p2 = &item2;
	const DiskListItem* p1_dup = &item1_dup;

	DiskListItem::sSortAscending = true;
	assert(DiskListItem::CompareUsage(&p1, &p2) < 0);
	assert(DiskListItem::CompareUsage(&p2, &p1) > 0);
	assert(DiskListItem::CompareUsage(&p1, &p1_dup) == 0);

	DiskListItem::sSortAscending = false;
	assert(DiskListItem::CompareUsage(&p1, &p2) > 0);
	assert(DiskListItem::CompareUsage(&p2, &p1) < 0);
}

int main() {
	std::cout << "Testing DiskListItem sorting functions..." << std::endl;

	test_compare_device();
	test_compare_mount();
	test_compare_fs();
	test_compare_total();
	test_compare_used();
	test_compare_free();
	test_compare_usage();

	std::cout << "All DiskListItem sorting tests passed!" << std::endl;
	return 0;
}
