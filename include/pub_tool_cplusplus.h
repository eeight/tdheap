// TODO: license!
//


// TODO: move more c++ stuff here. 



void PushMallocCostCenter(const char *cc);
void PopMallocCostCenter();

// This class should be used like this: 
//
//  {
//     ScopedMallocCostCenter cc("some string");
//     do_some_operator_new_calls_including_use_of_stl_containers();
//  }
//  If the heap profiler is enabled, it will associate all calls to 
//  operator new inside the cope with "some string".
//
class ScopedMallocCostCenter {
 public:
  ScopedMallocCostCenter(const char *cc) {
#ifndef NDEBUG  // What is the right macro? 
      PushMallocCostCenter(cc);
#endif
  }
  ~ScopedMallocCostCenter() {
#ifndef NDEBUG
      PopMallocCostCenter();
#endif
  }
};
