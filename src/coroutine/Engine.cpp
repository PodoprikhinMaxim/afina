#include <afina/coroutine/Engine.h>

#include <cstring>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char cur_address;
    ctx.Hight = StackBottom;
    ctx.Low = StackBottom;
    if (StackBottom < &cur_address) {
        ctx.Hight = &cur_address;
    } else {
        ctx.Low = &cur_address;
    }
    auto need_size = ctx.Hight - ctx.Low;
    auto &size = std::get<1>(ctx.Stack);
    auto &buf = std::get<0>(ctx.Stack);
    if (size < need_size) {
        delete[] buf;
        buf = new char[need_size];
        size = need_size;
        
    }
    memcpy(buf, ctx.Low, need_size);
}

void Engine::Restore(context &ctx) {
    char cur_address;
    // because we don't know what is direction of growing
    if ((ctx.Low < &cur_address) && (ctx.Hight > &cur_address)) {
        Restore(ctx);
    }
    auto &buf = std::get<0>(ctx.Stack);
    auto size = std::get<1>(ctx.Stack);
    memcpy(ctx.Low, buf, size);
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    if (alive == nullptr) {
        return;
    }
    auto routine_todo = alive;
    if (routine_todo == cur_routine) {
        if (alive->next != nullptr) {
            routine_todo = alive->next;
        } else {
            return;
        }
    }
    Enter(*routine_todo);
}

void Engine::sched(void *routine_) {
    if (routine_ == cur_routine) {
        return;
    } else if (routine_ == nullptr) {
        yield();
    }
    Enter(*(static_cast<context *>(routine_)));
}

void Engine::Enter(context &ctx) {
    if (cur_routine != nullptr && cur_routine != idle_ctx) {
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }
        Store(*cur_routine);
    }
    cur_routine = &ctx;
    Restore(ctx);
}

} // namespace Coroutine
} // namespace Afina
