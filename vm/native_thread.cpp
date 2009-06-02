#include "native_thread.hpp"

#include "call_frame.hpp"
#include "arguments.hpp"
#include "exception.hpp"

#include "builtin/thread.hpp"
#include "builtin/string.hpp"
#include "builtin/class.hpp"
#include "builtin/symbol.hpp"
#include "object_utils.hpp"
#include "builtin/nativemethod.hpp"

namespace rubinius {
  NativeThread::NativeThread(VM* vm, size_t stack_size, pthread_t tid)
    : thread::Thread(stack_size, tid)
    , vm_(vm)
  {}

  void NativeThread::perform() {
    // Grab the GIL
    // (automatically unlocked at the end of this function)
    GlobalLock::LockGuard x(vm_->global_lock());

    NativeMethod::init_thread(vm_);

    // Register that when the perform returns and the thread is exitting, to
    // run delete on this object to free up the memory.
    set_delete_on_exit();

    CallFrame cf;
    cf.previous = NULL;
    cf.static_scope_ = NULL;
    cf.msg = NULL;
    cf.cm = NULL;
    cf.top_scope = NULL;
    cf.scope = NULL;
    cf.ip = 0;
    cf.stack_size = 0;
    cf.stk = NULL;

    vm_->set_stack_start(&cf);

    Object* ret = vm_->thread.get()->send(vm_, &cf, vm_->symbol("__run__"));

    if(!ret) {
      if(Exception* exc = try_as<Exception>(vm_->thread_state()->raise_value())) {
        std::cout << "Exception at thread toplevel:\n";
        String* message = exc->message();
        if(message->nil_p()) {
          std::cout << "<no message> (";
        } else {
          std::cout << exc->message()->c_str() << " (";
        }

        std::cout << exc->class_object(vm_)->name()->c_str(vm_) << ")\n\n";
        // This can blow up. Don't do it.
        // exc->print_locations(vm_);
      }
    }

    vm_->thread.get()->detach_native_thread();
    VM::discard(vm_);
    vm_ = NULL;
  }
}
