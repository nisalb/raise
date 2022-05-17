extern "C" {
#include "vector.h"
}

#include <gtest/gtest.h>

TEST(VectorTest, VectorCreated)
{
	struct vector *v = vector_new(10, sizeof(int));
	EXPECT_NE(v, nullptr);
	vector_free(&v, NULL);
}

TEST(VectorTest, VectorIsFreed)
{
	struct vector *v = vector_new(10, sizeof(int));
	EXPECT_NE(v, nullptr);
	vector_free(&v, NULL);
	EXPECT_EQ(v, nullptr);
}

TEST(VectorTest, ShouldReturnSize)
{
	struct vector *v = vector_new(10, sizeof(int));
	EXPECT_NE(v, nullptr);
	size_t size = vector_size(v);
	EXPECT_EQ(size, 0);
	vector_free(&v, NULL);
}
