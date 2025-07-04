#pragma once

#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <mutex>
#include <string>

namespace spdlog
{
namespace sinks
{

    //
    // Rotating file sink based on size
    //
    template <typename Mutex>
    class custom_rotating_file_sink final : public base_sink<Mutex>
    {
    public:
        custom_rotating_file_sink(filename_t base_filename, std::size_t max_size,
                                  std::size_t max_files, bool rotate_on_open = false,
                                  const file_event_handlers &event_handlers = {});
        static filename_t calc_filename(const filename_t &filename, std::size_t index);
        filename_t filename();
        void rotate_now();

    protected:
        void sink_it_(const details::log_msg &msg) override;
        void flush_() override;

    private:
        void rotate_();

        // delete the target if exists, and rename the src file  to target
        // return true on success, false otherwise.
        bool rename_file_(const filename_t &src_filename, const filename_t &target_filename);

        filename_t base_filename_;
        std::size_t max_size_;
        std::size_t max_files_;
        std::size_t current_size_;
        details::file_helper file_helper_;
    };

    using custom_rotating_file_sink_mt = custom_rotating_file_sink<std::mutex>;
    using custom_rotating_file_sink_st = custom_rotating_file_sink<details::null_mutex>;

} // namespace sinks

//
// factory functions
//

template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger>
custom_rotating_logger_mt(const std::string &logger_name, const filename_t &filename,
                          size_t max_file_size, size_t max_files, bool rotate_on_open = false,
                          const file_event_handlers &event_handlers = {})
{
    return Factory::template create<sinks::custom_rotating_file_sink_mt>(
        logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}

template <typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<logger>
custom_rotating_logger_st(const std::string &logger_name, const filename_t &filename,
                          size_t max_file_size, size_t max_files, bool rotate_on_open = false,
                          const file_event_handlers &event_handlers = {})
{
    return Factory::template create<sinks::custom_rotating_file_sink_st>(
        logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#    include "custom_rotating_file_sink-inl.h"
#endif
