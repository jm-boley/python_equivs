#ifndef PYGEN_H_
#define PYGEN_H_
#pragma once

#include <functional>
#include <iterator>
#include <utility>
#include <type_traits>
#include <memory>
#include <chrono>
#include <random>
#include <exception>

namespace PythonicUtils {

template <typename _Rs>
class GenIter;

template <typename _Rs>
class Generator
{
    inline void throw_if_expired()
        { if (stopped_) throw std::runtime_error("begin() called on expired generator"); }

public:
    using iter = GenIter<_Rs>; // TODO:
                               // Poke through C++ stdlib and figure out how STL container
                               // type traits are handled

    Generator(std::function<_Rs(bool*)>& func)
    : func_(func),
      uid_(0),
      stopped_(false)
    {
        std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::exponential_distribution<double> edistr(3.5);
        uid_ = edistr(rng);
    }

    Generator(const Generator&) = delete;

    Generator(Generator&& other)
    : func_(other.func_),
      uid_(other.uid_),
      stopped_(other.stopped_)
    {
        throw_if_expired();     // !!! Make sure this is a good idea !!!
                                // Is it necessary? begin() throws...
        other.uid_ = 0;
        other.stopped_ = true;
    }

    GenIter<_Rs> begin()
    {
        throw_if_expired();
        return GenIter<_Rs>(func_, &uid_, &stopped_);
    }
    GenIter<_Rs> end() { return GenIter<_Rs>(&uid_, &stopped_); }

    void reset() { stopped_ = false; }

private:
    std::function<_Rs(bool*)>& func_;
    double uid_;
    bool stopped_;
};

template <typename _Rs>
class GenIter : public std::iterator<std::forward_iterator_tag, _Rs>
{
    friend class Generator<_Rs>;

    inline void incr_throw_uid_invalid_stopped()
        { if (this->uid_ == nullptr || *(this->stop_))
            throw std::runtime_error("GenIter: Attempted to increment an invalidated iterator"); }

public:
//    using iterator_category = std::forward_iterator_tag;

    GenIter(std::function<_Rs(bool*)>& func, double* gid, bool* stop) noexcept
    : gen_func_(func),
      result_(func(stop)),
      uid_(gid),
      stop_(stop)
    {}

    GenIter(const GenIter& other)
    : gen_func_(other.gen_func_),
      result_(other.result_),
      uid_(other.uid_),
      stop_(other.stop_)
    {
        if (other.uid_ == nullptr)
            throw std::invalid_argument("GenIter: Attempted to copy-construct from an invalidated iterator");
    }

    GenIter(GenIter&& other)
    : gen_func_(other.gen_func_),
      result_(other.result_),
      uid_(other.uid_),
      stop_(other.stop_)
    {
        if (other.uid_ == nullptr)
            throw std::invalid_argument("GenIter: Attempted to move-construct from an invalidated iterator");
        
        other.uid_ = nullptr;
        other.stop_ = nullptr;
    }
    
    GenIter& operator=(const GenIter& other)
    {
        if (this != &other) {
            if (other.uid_ == nullptr)
                throw std::invalid_argument("GenIter: Attempted to copy-assign from an invalidated iterator");
            
            this->gen_func_ = other.gen_func_;
            this->result_ = other.result_;
            this->uid_ = other.uid_;
            this->stop_ = other.stop_;
        }
    }

    GenIter& operator=(GenIter&& other)
    {
        if (this != &other) {
            if (other.uid_ == nullptr)
                throw std::invalid_argument("GenIter: Attempted to move-assign from an invalidated iterator");
            
            this->gen_func_ = other.gen_func_;
            this->result_ = other.result_;
            this->uid_ = other.uid_;
            this->stop_ = other.stop_;

            other.uid_ = nullptr;
            other.stop_ = nullptr;
        }
    }

    GenIter& operator++()
    {
        // This is probably bad behavior, look into idiomatic handling of client use of invalidated iterators
        incr_throw_uid_invalid_stopped();
        
        result_ = gen_func_(stop_);
        return *this;
    }

    GenIter operator++(int)
    {
        // This is probably bad behavior, look into idiomatic handling of client use of invalidated iterators
        incr_throw_uid_invalid_stopped();

        auto rval = GenIter(*this);
        result_ = gen_func_(stop_);
        return rval;
    }

    bool operator!=(const GenIter& other) const
    {
        if (other.uid_ == nullptr) return true;
        return *uid_ != *other.uid_ || !(*stop_);
    }

    bool operator!=(const GenIter&& other) const
    {
        if (other.uid_ == nullptr) return true;
        return *uid_ != *other.uid_ || !(*stop_);
    }
    
    _Rs operator*() const
    {
        if (this->uid_ == nullptr || *(this->stop_))
            throw std::runtime_error("GenIter: Attempted to access an invalidated iterator");

        return result_;
    }

private:
    // Special empty lambda used when Generator class wants to construct an end() iterator
    static std::function<_Rs(bool*)>& empty_lambda()
    {
        static std::function<_Rs(bool*)> rval = [](bool*) {
            return _Rs();
        };
        return rval;
    }

    // Generator-only constructor, used when end() is called
    GenIter(double* gid, bool* stop) noexcept
    : gen_func_(empty_lambda()),
      result_(_Rs()),
      uid_(gid),
      stop_(stop)
    {}

    std::function<_Rs(bool*)>& gen_func_;
    _Rs result_;      // This is a problem if copy-construction is expensive
    double* uid_;
    bool* stop_;
};

template <typename _Rs>
Generator<_Rs> make_generator(std::function<_Rs(bool*)>& func)
{
    return Generator<_Rs> {func};
}

}  # PythonicUtils

#endif
