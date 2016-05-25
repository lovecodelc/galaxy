// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "collector.h"
#include "util/error_code.h"
#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"
#include "thread.h"
#include "timer.h"
#include "thread_pool.h"

#include <stdint.h>

#include <vector>


namespace baidu {
namespace galaxy {
namespace collector {

class CollectorEngine {
public:
    ~CollectorEngine();
    static boost::shared_ptr<CollectorEngine> GetInstance();
    baidu::galaxy::util::ErrorCode Register(boost::shared_ptr<Collector> collector);
    void Unregister(boost::shared_ptr<Collector> collector);
    int Setup();
    void TearDown();

private:
    CollectorEngine();
    static boost::shared_ptr<CollectorEngine> instance_;

    class RuntimeCollector {
    public:
        RuntimeCollector(boost::shared_ptr<Collector> collector) :
            collector_(collector),
            next_time_(0),
            is_running_(false) {
        }

        bool Expired(int64_t now) {
            boost::mutex::scoped_lock lock(mutex_);
            return next_time_ <= now;
        }

        bool IsRunning() {
            boost::mutex::scoped_lock lock(mutex_);
            return is_running_;
        }

        void SetRunning(bool r) {
            boost::mutex::scoped_lock lock(mutex_);
            is_running_ = r;
        }

        void UpdateNextRuntime() {
            boost::mutex::scoped_lock lock(mutex_);
            next_time_ += collector_->Cycle() * 1000000L;
            int64_t now = baidu::common::timer::get_micros();

            if (next_time_ < now) {
                next_time_ = now;
            }

            if (next_time_ > now + collector_->Cycle() * 1000000L * 2L) {
                next_time_ = now;
            }

        }

        boost::shared_ptr<baidu::galaxy::collector::Collector> GetCollector() {
            return collector_;
        }

    private:
        boost::shared_ptr<baidu::galaxy::collector::Collector> collector_;
        int64_t next_time_;
        bool is_running_;
        boost::mutex mutex_;
    };

    void CollectRoutine(boost::shared_ptr<RuntimeCollector> rc);
    void CollectMainThreadRoutine();
    std::vector<boost::shared_ptr<RuntimeCollector> > collectors_;
    boost::mutex mutex_;
    bool running_;
    baidu::common::ThreadPool collector_pool_;
    baidu::common::Thread main_collect_thread_;;

};
}
}
}