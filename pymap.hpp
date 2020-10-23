#ifndef PYMAP_H_
#define PYMAP_H_
#pragma once

#include <utility>
#include <type_traits>
#include <iterator>

//namespace PythonicUtils {
    template <typename _Func, typename _Range>
    auto pymap(_Func&& func, _Range&& range) {
        using std::begin;
        using std::end;
        using E = std::decay_t<decltype(std::forward<_Func>(func)(
            *begin(std::forward<_Range>(range))
        ))>;

        auto first = begin(std::forward<_Range>(range));
        auto last = end(std::forward<_Range>(range));

        std::vector<E> result;
        result.reserve(std::distance(first, last));
        for (; first != last; ++first) {
            result.push_back(std::forward<_Func>(func)(*first));
        }
        return result;
    }

    template <typename _Func, typename _Range>
    void pymap_mut(_Func&& func, _Range&& range) {
        using std::begin;
        using std::end;

        auto first = begin(std::forward<_Range>(range));
        auto last = end(std::forward<_Range>(range));

        std::size_t i = 0;
        for (; first != last; ++first) {
            range.at(i++) = std::forward<_Func>(func)(*first);
        }
    }
//}
#endif