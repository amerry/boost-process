// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_INITIALIZERS_ASYNC_OUT_HPP
#define BOOST_PROCESS_WINDOWS_INITIALIZERS_ASYNC_OUT_HPP

#include <boost/detail/winapi/process.hpp>
#include <boost/detail/winapi/handles.hpp>
#include <boost/process/windows/pipe.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <boost/asio/read.hpp>
#include <istream>
#include <memory>

namespace boost { namespace process { namespace detail { namespace windows {


template <typename Executor>
void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 1>, std::integral_constant<int, -1>)
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template <typename Executor>
void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 2>, std::integral_constant<int, -1>)
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);


    e.startup_info.hStdError = handle;
    e.startup_info.dwFlags  |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}

template <typename Executor>
void apply_out_handles(Executor &e, void* handle, std::integral_constant<int, 1>, std::integral_constant<int, 2>)
{
    boost::detail::winapi::SetHandleInformation(handle,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_,
            boost::detail::winapi::HANDLE_FLAG_INHERIT_);

    e.startup_info.hStdOutput = handle;
    e.startup_info.hStdError  = handle;
    e.startup_info.dwFlags   |= ::boost::detail::winapi::STARTF_USESTDHANDLES_;
    e.inherit_handles = true;
}



template<int p1, int p2>
struct async_out_stream : public ::boost::process::detail::windows::async_handler
{
    std::shared_ptr<boost::asio::windows::stream_handle> stream_handle;

    struct res_t
    {
        boost::asio::streambuf buf;
        boost::process::pipe pipe = boost::process::pipe::create_async();
    };

    mutable std::shared_ptr<res_t> res = std::make_shared<res_t>();
    //because the pipe will be moved later on, but i might need the source at another point.
    boost::iostreams::file_descriptor_sink sink = res->pipe.sink();

    async_out_stream(std::istream & is)
    {
        is.rdbuf(&res->buf);
    }
    template <typename Executor>
    inline void on_success(Executor &exec) const
    {
        boost::asio::io_service &is_ser = get_io_service(exec.seq);
        auto & res = this->res;
        auto& stream_handle = this->stream_handle;
        boost::asio::async_read(*stream_handle, res->buf,
                [res, stream_handle](const boost::system::error_code&, std::size_t size){});
    }

    template<typename Executor>
    auto on_exit_handler(Executor & exec)
    {
        auto  stream_handle = this->stream_handle;
        auto res = this->res;
        return [stream_handle, res](const std::error_code & ec)
                {
                    boost::asio::io_service & ios = stream_handle->get_io_service();
                    ios.post([stream_handle]{stream_handle->close();});

                };

    };
    template <typename WindowsExecutor>
    void on_setup(WindowsExecutor &exec)
    {
        stream_handle = std::make_shared<boost::asio::windows::stream_handle>(get_io_service(exec.seq), res->pipe.source().handle());
        apply_out_handles(exec, sink.handle(), std::integral_constant<int, p1>(), std::integral_constant<int, p2>());
    }
};


}}}}

#endif
