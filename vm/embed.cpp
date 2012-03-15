#include "api/embed.h"

#include "builtin/object.hpp"
#include "builtin/string.hpp"
#include "builtin/array.hpp"
#include "builtin/module.hpp"

rbx_ctx rbx_create_context(uintptr_t stack_start) {
  Environment* ctx = new Environment(0, NULL);
  ctx->setup_cpp_terminate();
  
  const char* runtime = getenv("RBX_RUNTIME");
  if(!runtime) runtime = RBX_RUNTIME;
  std::string runtime_str(runtime);
  
  ctx->state->vm()->set_root_stack(stack_start, VM::cStackDepthMax);
  
  ctx->load_platform_conf(runtime_str);
  ctx->boot_vm();
  ctx->state->vm()->initialize_config();
  ctx->load_tool();

  if(LANGUAGE_20_ENABLED(ctx->state)) {
    runtime_str += "/20";
  } else if(LANGUAGE_19_ENABLED(ctx->state)) {
    runtime_str += "/19";
  } else {
    runtime_str += "/18";
  }
  RUBINIUS_SET_CONST(ctx, "RUNTIME_PATH", String::create(ctx->state,
                                                         runtime_str.c_str(),
                                                         runtime_str.size()));
  ctx->load_kernel(runtime_str);
  ctx->start_signals();

  ctx->run_file(runtime_str + "/loader.rbc");
  ctx->run_file(runtime_str + "/embed_loader.rbc");
  
  rbx_obj loader = RUBINIUS_GET_CONST_SYM(ctx, "EmbedLoader");
  rbx_send(ctx, loader, "load_compiler", 0);
  
  return ctx;
}

bool rbx_require_file(rbx_ctx ctx, const char * file) {
  rbx_obj loader = RUBINIUS_GET_CONST_SYM(ctx, "EmbedLoader");
  rbx_send(ctx, loader, "require_file", 1, String::create(ctx->state, file));
  
  return true;
}

bool rbx_eval_file(rbx_ctx ctx, const char * file) {
  rbx_obj loader = RUBINIUS_GET_CONST_SYM(ctx, "EmbedLoader");
  rbx_send(ctx, loader, "eval_file", 1, String::create(ctx->state, file));
  
  return true;
}

bool rbx_eval(rbx_ctx ctx, const char * code) {
  rbx_obj loader = RUBINIUS_GET_CONST_SYM(ctx, "EmbedLoader");
  rbx_send(ctx, loader, "eval_script", 1, String::create(ctx->state, code));
  
  return true;
}

rbx_obj rbx_send(rbx_ctx ctx, rbx_obj recv, const char* msg, int argc, ...) {
  va_list args;
  va_start(args, argc);
  
  Array* arg_ary = Array::create(ctx->state, argc);
  for(int i=0; i<argc; i++)
    arg_ary->append(ctx->state, va_arg(args, Object*));
  
  va_end(args);
  
  rbx_obj ret = recv->send(ctx->state, 0, ctx->state->symbol(msg), arg_ary);
  return ret;
}

void rbx_close_ctx(rbx_ctx ctx) {
  ctx->halt(ctx->state);
  delete ctx;
}
