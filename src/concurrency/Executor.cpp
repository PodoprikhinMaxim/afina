#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {
	Executor::Executor(std::size_t low, std::size_t high, std::size_t max_tasks, std::chrono::milliseconds time)
        : low_watermark(low), high_watermark(high), max_queue_size(max_tasks), idle_time(time) {
		std::lock_guard<std::mutex> lock(mutex);
		for(std::size_t i = 0; i < low_watermark; i++)
		{
			std::thread{perform, this}.detach();
		}
		current_threads = low_watermark;
		current_threads_running = low_watermark;
	}
	Executor::~Executor() {}
	void Stop(bool await = false) {
		std::lock_guard<std::mutex> lock(mutex);
		state = State::KStopping;
		while (std.threads.size > 0) {
			_stop_cv.wait(lock);
		}
		state = State::KStopped;
	}

	void _perform_task(Executor* exec)
	{
		while (exec->state == Executor::State::kRun) {
			std::function<void()> task;
			auto time_until = std::chrono::system_clock::now()
				+ std::chrono::milliseconds(exec->idle_time);

			{
				std::unique_lock<std::mutex> lock(exec->mutex);
				while (exec->_tasks.empty() && exec->state == Executor::State::kRun) {
					exec->_current_threads_working--;
					if (exec->empty_condition.wait_until(lock, time_until) == std::cv_status::timeout) {
						if (exec->_current_threads > exec->low_watermark) {
							exec->_current_threads--;
                           	break;
						}
						else {
							exec->empty_condition.wait(lock);
						}
					}
						exec->_current_threads_working++;
				}
				task = exec->_tasks.front();
				exec->_tasks.pop_front();
			}

				task();
		}

		{
			std::unique_lock<std::mutex> lock(exec->_mutex);
			exec->_current_threads--;
			if (exec->_current_threads == 0) {
				exec->_stop_cv.notify_all();
			}
		}
}
} // namespace Afina
