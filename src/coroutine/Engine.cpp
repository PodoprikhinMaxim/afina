#include <afina/coroutine/Engine.h>

#include <cstring>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char cur_address;
    ctx.Hight = &cur_address;
    ctx.Low = StackBottom;
    if (ctx.Hight < ctx.Low) {
        std::swap(ctx.Hight, ctx.Low);
    }
    std::size_t need_size = ctx.Hight - ctx.Low;
    auto &size = std::get<1>(ctx.Stack);
    auto &stack = std::get<0>(ctx.Stack);
    if (size < need_size) {
        if (stack != nullptr) {
            delete stack;
        }
        stack = new char[need_size];
    }
    std::memcpy(stack, ctx.Low, need_size);
    ctx.Stack = std::make_tuple(stack, need_size);
}

void Engine::Restore(context &ctx) {
    char cur_address;
    // because we don't know what is direction of growing
    while ((ctx.Low < &cur_address) && (ctx.Hight > &cur_address)) {
        Restore(ctx);
    }

    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    Store(*cur_routine);
    auto routine_todo = alive;
    if (routine_todo == cur_routine) {
        if (alive->next != nullptr) {
            routine_todo = alive->next;
            sched(static_cast<void *>(routine_todo));
        }
    }
}

void Engine::sched(void *routine_) {
    if (routine_ == cur_routine) {
        return;
    } else if (routine_ == nullptr) {
        yield();
    }
    if (setjmp(cur_routine->Environment) == 0) {
        Store(*cur_routine);
        context *ctx = static_cast<context *>(routine_);
        Restore(*ctx);
    }
}

} // namespace Coroutine
} // namespace Afina
