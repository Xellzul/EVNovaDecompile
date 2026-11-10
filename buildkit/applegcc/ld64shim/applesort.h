// applesort.h -- std::sort as libstdc++ 4.2.1 (Xcode 3.2.6) wrote it, so elements that compare equal come out in
// the order Apple's ld64 left them. Copied from DevSDK/usr/include/c++/4.2.1/bits/stl_algo.h (introsort, threshold
// 16, median of three, final insertion sort); the heap fallback is not reached for the sizes ld64 sorts here.
#ifndef APPLESORT_H
#define APPLESORT_H
#include <algorithm>
#include <iterator>
#include <cstdlib>

namespace applestl {

enum { threshold = 16 };

template <typename T, typename C>
inline const T& median(const T& a, const T& b, const T& c, C comp)
{
	if (comp(a, b))
		if (comp(b, c)) return b;
		else if (comp(a, c)) return c;
		else return a;
	else if (comp(a, c)) return a;
	else if (comp(b, c)) return c;
	else return b;
}

template <typename I, typename T, typename C>
I unguarded_partition(I first, I last, T pivot, C comp)
{
	while (true) {
		while (comp(*first, pivot)) ++first;
		--last;
		while (comp(pivot, *last)) --last;
		if (!(first < last)) return first;
		std::iter_swap(first, last);
		++first;
	}
}

template <typename I, typename T, typename C>
void unguarded_linear_insert(I last, T val, C comp)
{
	I next = last;
	--next;
	while (comp(val, *next)) {
		*last = *next;
		last = next;
		--next;
	}
	*last = val;
}

template <typename I, typename C>
void insertion_sort(I first, I last, C comp)
{
	typedef typename std::iterator_traits<I>::value_type V;
	if (first == last) return;
	for (I i = first + 1; i != last; ++i) {
		V val = *i;
		if (comp(val, *first)) {
			std::copy_backward(first, i, i + 1);
			*first = val;
		} else
			unguarded_linear_insert(i, val, comp);
	}
}

template <typename I, typename C>
void unguarded_insertion_sort(I first, I last, C comp)
{
	typedef typename std::iterator_traits<I>::value_type V;
	for (I i = first; i != last; ++i)
		unguarded_linear_insert(i, V(*i), comp);
}

template <typename I, typename C>
void final_insertion_sort(I first, I last, C comp)
{
	if (last - first > int(threshold)) {
		insertion_sort(first, first + int(threshold), comp);
		unguarded_insertion_sort(first + int(threshold), last, comp);
	} else
		insertion_sort(first, last, comp);
}

template <typename I, typename S, typename C>
void introsort_loop(I first, I last, S depth, C comp)
{
	typedef typename std::iterator_traits<I>::value_type V;
	while (last - first > int(threshold)) {
		if (depth == 0) {
			std::partial_sort(first, last, last, comp);
			return;
		}
		--depth;
		I cut = unguarded_partition(first, last, V(median(*first, *(first + (last - first) / 2), *(last - 1), comp)), comp);
		introsort_loop(cut, last, depth, comp);
		last = cut;
	}
}

template <typename I, typename C>
void sort(I first, I last, C comp)
{
	if (first != last) {
		int lg = 0;
		for (long n = last - first; n != 1; n >>= 1) ++lg;
		introsort_loop(first, last, lg * 2, comp);
		final_insertion_sort(first, last, comp);
	}
}

}

#endif
