#include "vm/test/test.hpp"
#include "api/embed.h"

class TestEmbedding : public CxxTest::TestSuite {
public:
  void test_create_and_close_context() {
    int i = 0;
    rbx_ctx ctx = rbx_create_context(reinterpret_cast<uintptr_t>(&i));
    TS_ASSERT(ctx != NULL);
    rbx_close_ctx(ctx);
  }
  
  void test_multiple_sequential_contexts() {
    int i = 0;

    rbx_ctx ctx1 = rbx_create_context(reinterpret_cast<uintptr_t>(&i));
    TS_ASSERT(ctx1 != NULL);
    rbx_close_ctx(ctx1);
    
    rbx_ctx ctx2 = rbx_create_context(reinterpret_cast<uintptr_t>(&i));
    TS_ASSERT(ctx2 != NULL);
    rbx_close_ctx(ctx2);
  }
};
